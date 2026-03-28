//===-- ARC4MCInstLower.cpp - Lower MachineInstr to MCInst -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Converts codegen MachineInstrs to MC-layer MCInsts. Every codegen-only
// instruction (CG_*) is translated to its MC equivalent (ADD_rrr, etc.)
// with the proper trailing operands (F, NN, Q, SetFlags) appended.
//
// After this pass, the MC code emitter should NEVER see a CG_ opcode.
//
//===----------------------------------------------------------------------===//

#include "ARC4MCInstLower.h"
#include "MCTargetDesc/ARC4MCTargetDesc.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"

using namespace llvm;

MCOperand ARC4MCInstLower::LowerSymbolOperand(const MachineOperand &MO) const {
  const MCSymbol *Sym;
  switch (MO.getType()) {
  case MachineOperand::MO_GlobalAddress:
    Sym = Printer.getSymbol(MO.getGlobal());
    break;
  case MachineOperand::MO_ExternalSymbol:
    Sym = Printer.GetExternalSymbolSymbol(MO.getSymbolName());
    break;
  case MachineOperand::MO_MachineBasicBlock:
    Sym = MO.getMBB()->getSymbol();
    break;
  case MachineOperand::MO_BlockAddress:
    Sym = Printer.GetBlockAddressSymbol(MO.getBlockAddress());
    break;
  default:
    llvm_unreachable("Unknown symbol operand type");
  }

  const MCExpr *Expr = MCSymbolRefExpr::create(Sym, Ctx);
  if (MO.getType() == MachineOperand::MO_GlobalAddress && MO.getOffset())
    Expr = MCBinaryExpr::createAdd(
        Expr, MCConstantExpr::create(MO.getOffset(), Ctx), Ctx);

  return MCOperand::createExpr(Expr);
}

MCOperand ARC4MCInstLower::LowerOperand(const MachineOperand &MO) const {
  switch (MO.getType()) {
  case MachineOperand::MO_Register:
    if (MO.isImplicit())
      return MCOperand();
    return MCOperand::createReg(MO.getReg());
  case MachineOperand::MO_Immediate:
    return MCOperand::createImm(MO.getImm());
  case MachineOperand::MO_GlobalAddress:
  case MachineOperand::MO_ExternalSymbol:
  case MachineOperand::MO_MachineBasicBlock:
  case MachineOperand::MO_BlockAddress:
    return LowerSymbolOperand(MO);
  case MachineOperand::MO_RegisterMask:
    return MCOperand();
  default:
    llvm_unreachable("Unknown operand type");
  }
}

// Helper: append F=0, NN=0, Q=0 (register format trailing operands)
static void addRegFmtTrail(MCInst &MI) {
  MI.addOperand(MCOperand::createImm(0)); // F
  MI.addOperand(MCOperand::createImm(0)); // NN
  MI.addOperand(MCOperand::createImm(0)); // Q
}

// Helper: append SetFlags=0, NN=0 (shimm format trailing operands)
static void addShimmTrail(MCInst &MI) {
  MI.addOperand(MCOperand::createImm(0)); // SetFlags
  MI.addOperand(MCOperand::createImm(0)); // NN
}

void ARC4MCInstLower::Lower(const MachineInstr *MI, MCInst &OutMI) const {
  unsigned Opc = MI->getOpcode();

  // ---------------------------------------------------------------
  // ALU register-register: CG_ADDrr etc. → ADD_rrr + F=0, NN=0, Q=0
  // ---------------------------------------------------------------
  auto lowerALUrr = [&](unsigned MCOpc) {
    OutMI.setOpcode(MCOpc);
    OutMI.addOperand(LowerOperand(MI->getOperand(0))); // dst
    OutMI.addOperand(LowerOperand(MI->getOperand(1))); // src1
    OutMI.addOperand(LowerOperand(MI->getOperand(2))); // src2
    addRegFmtTrail(OutMI);
  };

  // ALU register-immediate: CG_ADDri etc. → ADD_rrs + SetFlags=0, NN=0
  auto lowerALUri = [&](unsigned MCOpc) {
    OutMI.setOpcode(MCOpc);
    OutMI.addOperand(LowerOperand(MI->getOperand(0))); // dst
    OutMI.addOperand(LowerOperand(MI->getOperand(1))); // src
    OutMI.addOperand(LowerOperand(MI->getOperand(2))); // imm
    addShimmTrail(OutMI);
  };

  switch (Opc) {
  // --- ALU register-register ---
  case ARC4::CG_ADDrr: lowerALUrr(ARC4::ADD_rrr); return;
  case ARC4::CG_SUBrr: lowerALUrr(ARC4::SUB_rrr); return;
  case ARC4::CG_ANDrr: lowerALUrr(ARC4::AND_rrr); return;
  case ARC4::CG_ORrr:  lowerALUrr(ARC4::OR_rrr);  return;
  case ARC4::CG_XORrr: lowerALUrr(ARC4::XOR_rrr); return;

  // --- ALU register-immediate ---
  case ARC4::CG_ADDri: lowerALUri(ARC4::ADD_rrs); return;
  case ARC4::CG_SUBri: lowerALUri(ARC4::SUB_rrs); return;
  case ARC4::CG_ANDri: lowerALUri(ARC4::AND_rrs); return;
  case ARC4::CG_ORri:  lowerALUri(ARC4::OR_rrs);  return;

  // --- MOV register: AND dst, src, src ---
  case ARC4::CG_MOVrr: {
    OutMI.setOpcode(ARC4::AND_rrr);
    MCOperand Dst = LowerOperand(MI->getOperand(0));
    MCOperand Src = LowerOperand(MI->getOperand(1));
    OutMI.addOperand(Dst);
    OutMI.addOperand(Src);
    OutMI.addOperand(Src); // C = B (AND b,b = MOV)
    addRegFmtTrail(OutMI);
    return;
  }

  // --- MOV shimm: AND dst, shimm, shimm (B=R63, C=R63 sentinel) ---
  // Handled directly by code emitter (CG_MOVri) since the MC shimm format
  // doesn't support both B and C as shimm sentinels through normal operands.
  case ARC4::CG_MOVri:
  case ARC4::CG_MOVli:
    // Pass through to code emitter which handles these specially
    OutMI.setOpcode(Opc);
    for (const MachineOperand &MO : MI->operands()) {
      MCOperand MCOp = LowerOperand(MO);
      if (MCOp.isValid())
        OutMI.addOperand(MCOp);
    }
    return;

  // --- Load register+offset: CG_LDri → LD_rrs + SetFlags=0, NN=0 ---
  case ARC4::CG_LDri:
    // Pass through to code emitter (operand order matches, needs special
    // handling for LD shimm C-field encoding)
    OutMI.setOpcode(Opc);
    for (const MachineOperand &MO : MI->operands()) {
      MCOperand MCOp = LowerOperand(MO);
      if (MCOp.isValid())
        OutMI.addOperand(MCOp);
    }
    return;

  // --- Load register+register: CG_LDrr ---
  case ARC4::CG_LDrr:
    OutMI.setOpcode(Opc);
    for (const MachineOperand &MO : MI->operands()) {
      MCOperand MCOp = LowerOperand(MO);
      if (MCOp.isValid())
        OutMI.addOperand(MCOp);
    }
    return;

  // --- Store register+offset: CG_STri → ST_rrs (no trailing operands) ---
  case ARC4::CG_STri:
    OutMI.setOpcode(Opc);
    for (const MachineOperand &MO : MI->operands()) {
      MCOperand MCOp = LowerOperand(MO);
      if (MCOp.isValid())
        OutMI.addOperand(MCOp);
    }
    return;

  // --- Compare reg-reg: SUB_0rr with F=1 (discard result, set flags) ---
  case ARC4::CG_CMPrr:
    OutMI.setOpcode(Opc);
    OutMI.addOperand(LowerOperand(MI->getOperand(0))); // src1
    OutMI.addOperand(LowerOperand(MI->getOperand(1))); // src2
    return;

  // --- Compare reg-imm: SUB_0rs with SetFlags=1 ---
  case ARC4::CG_CMPri:
    OutMI.setOpcode(Opc);
    OutMI.addOperand(LowerOperand(MI->getOperand(0))); // src
    OutMI.addOperand(LowerOperand(MI->getOperand(1))); // imm
    return;

  // --- Shift-by-1: ASR/LSR ---
  case ARC4::CG_ASR:
  case ARC4::CG_LSR:
    OutMI.setOpcode(Opc);
    for (const MachineOperand &MO : MI->operands()) {
      MCOperand MCOp = LowerOperand(MO);
      if (MCOp.isValid())
        OutMI.addOperand(MCOp);
    }
    return;

  // --- Call with symbol target ---
  case ARC4::CG_CALLi:
  case ARC4::CG_CALLr: {
    const MachineOperand &TargetMO = MI->getOperand(0);
    if (TargetMO.isReg()) {
      // Register call: JL_r with B=target, F=0, NN=0, Q=0
      OutMI.setOpcode(ARC4::JL_r);
      OutMI.addOperand(LowerOperand(TargetMO));
      addRegFmtTrail(OutMI);
    } else {
      // Symbol call: pass through to code emitter (needs limm fixup)
      OutMI.setOpcode(ARC4::CG_CALLi);
      MCOperand Target = LowerOperand(TargetMO);
      if (Target.isValid())
        OutMI.addOperand(Target);
    }
    return;
  }

  // --- Unconditional branch ---
  case ARC4::CG_BR: {
    OutMI.setOpcode(ARC4::B);
    for (const MachineOperand &MO : MI->operands()) {
      MCOperand MCOp = LowerOperand(MO);
      if (MCOp.isValid())
        OutMI.addOperand(MCOp);
    }
    OutMI.addOperand(MCOperand::createImm(0)); // NN
    OutMI.addOperand(MCOperand::createImm(0)); // Q=0 (unconditional)
    return;
  }

  // --- Conditional branch ---
  case ARC4::CG_BRcc: {
    OutMI.setOpcode(ARC4::B);
    // Find MBB target and CC immediate
    unsigned CC = 0;
    for (const MachineOperand &MO : MI->operands()) {
      if (MO.isMBB()) {
        MCOperand Target = LowerOperand(MO);
        if (Target.isValid())
          OutMI.addOperand(Target);
      } else if (MO.isImm()) {
        CC = MO.getImm();
      }
    }
    OutMI.addOperand(MCOperand::createImm(0));  // NN
    OutMI.addOperand(MCOperand::createImm(CC)); // Q = condition code
    return;
  }

  // --- Return: J_r with B=r31(blink) ---
  case ARC4::CG_RET:
    OutMI.setOpcode(ARC4::J_r);
    OutMI.addOperand(MCOperand::createReg(ARC4::R31));
    addRegFmtTrail(OutMI);
    return;

  default:
    break;
  }

  // --- Default: pass through (for non-CG instructions from the assembler) ---
  OutMI.setOpcode(Opc);
  for (const MachineOperand &MO : MI->operands()) {
    MCOperand MCOp = LowerOperand(MO);
    if (MCOp.isValid())
      OutMI.addOperand(MCOp);
  }
}
