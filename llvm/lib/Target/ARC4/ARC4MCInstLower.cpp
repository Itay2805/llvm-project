//===-- ARC4MCInstLower.cpp - Lower MachineInstr to MCInst -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Converts codegen-only MachineInstrs to MC-layer MCInsts. The codegen
// instructions have simplified operand lists; we map them to the MC
// instructions and append default F=0, NN=0, Q=0 operands as needed.
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
    return MCOperand(); // skip
  default:
    llvm_unreachable("Unknown operand type");
  }
}

// Map codegen-only opcodes to MC-layer opcodes.
// Returns 0 if no mapping needed (opcode passes through).
static unsigned mapCGtoMC(unsigned Opc, bool &NeedsRegFmtTrail,
                          bool &NeedsShimmTrail, bool &NeedsLimmTrail,
                          bool &IsMOVrr, bool &IsMOVri, bool &IsMOVli) {
  NeedsRegFmtTrail = NeedsShimmTrail = NeedsLimmTrail = false;
  IsMOVrr = IsMOVri = IsMOVli = false;

  switch (Opc) {
  // ALU reg-reg: append F=0, NN=0, Q=0
  case ARC4::CG_ADDrr: NeedsRegFmtTrail = true; return ARC4::ADD_rrr;
  case ARC4::CG_SUBrr: NeedsRegFmtTrail = true; return ARC4::SUB_rrr;
  case ARC4::CG_ANDrr: NeedsRegFmtTrail = true; return ARC4::AND_rrr;
  case ARC4::CG_ORrr:  NeedsRegFmtTrail = true; return ARC4::OR_rrr;
  case ARC4::CG_XORrr: NeedsRegFmtTrail = true; return ARC4::XOR_rrr;

  // ALU reg-imm: append SetFlags=0, NN=0
  case ARC4::CG_ADDri: NeedsShimmTrail = true; return ARC4::ADD_rrs;
  case ARC4::CG_SUBri: NeedsShimmTrail = true; return ARC4::SUB_rrs;
  case ARC4::CG_ANDri: NeedsShimmTrail = true; return ARC4::AND_rrs;
  case ARC4::CG_ORri:  NeedsShimmTrail = true; return ARC4::OR_rrs;

  // MOV register: AND dst, src, src → need special handling
  case ARC4::CG_MOVrr: IsMOVrr = true; return ARC4::AND_rrr;

  // MOV shimm: AND dst, shimm, shimm → special: B=C=sentinel, D=value
  case ARC4::CG_MOVri: IsMOVri = true; return ARC4::AND_rrs;

  // MOV limm: AND dst, limm → AND_rrl with B position unused
  case ARC4::CG_MOVli: IsMOVli = true; return ARC4::AND_rrl;

  // Load/Store: append trailing imms
  case ARC4::CG_LDri: NeedsShimmTrail = true; return ARC4::LD_rrs;
  case ARC4::CG_LDrr: NeedsRegFmtTrail = true; return ARC4::LD_rrr;
  case ARC4::CG_STri: return ARC4::ST_rrs; // store has no trailing imms

  // Compare: discard dest SUB with F=1
  case ARC4::CG_CMPrr: return 0; // handled specially
  case ARC4::CG_CMPri: return 0;

  // Branch/Jump/Return: pass through (handled by AsmPrinter or pattern)
  case ARC4::CG_RET:   return ARC4::J_r;
  case ARC4::CG_BR:    return ARC4::B;
  case ARC4::CG_BRcc:  return ARC4::B;
  case ARC4::CG_CALLi: return ARC4::JL_l;
  case ARC4::CG_CALLr: return ARC4::JL_r;

  default: return 0; // pass through as-is
  }
}

void ARC4MCInstLower::Lower(const MachineInstr *MI, MCInst &OutMI) const {
  bool NeedsRegFmtTrail, NeedsShimmTrail, NeedsLimmTrail;
  bool IsMOVrr, IsMOVri, IsMOVli;
  unsigned MCOpc = mapCGtoMC(MI->getOpcode(), NeedsRegFmtTrail,
                             NeedsShimmTrail, NeedsLimmTrail,
                             IsMOVrr, IsMOVri, IsMOVli);

  if (MCOpc)
    OutMI.setOpcode(MCOpc);
  else
    OutMI.setOpcode(MI->getOpcode());

  // Special: MOV rr → AND dst, src, src (duplicate the source reg)
  if (IsMOVrr) {
    for (const MachineOperand &MO : MI->operands()) {
      MCOperand MCOp = LowerOperand(MO);
      if (MCOp.isValid()) {
        OutMI.addOperand(MCOp);
        if (MO.isReg() && !MO.isDef())
          OutMI.addOperand(MCOp); // duplicate src as both B and C
      }
    }
    // Append F=0, NN=0, Q=0
    OutMI.addOperand(MCOperand::createImm(0));
    OutMI.addOperand(MCOperand::createImm(0));
    OutMI.addOperand(MCOperand::createImm(0));
    return;
  }

  // Special: MOV ri → AND_rrs with shimm,shimm encoding
  // MCInst layout for AND_rrs: A(reg), B(reg), D(shimm), SetFlags, NN
  // But for MOV shimm, B should be shimm sentinel too.
  // The shimm-in-B form (AND_rsr) handles this: A(reg), D(shimm), C(reg), ...
  // Actually, for MOV a,shimm: use the _rrs form where the emitter sees
  // A=dest, B=shimm_sentinel(63), C=shimm_sentinel(63), D=value
  // But _rrs format expects A(reg), B(reg), D(simm9), SetFlags, NN
  // We need to emit: A=dst, B=<unused>, D=shimm, SetFlags=0, NN=0
  // The code emitter for shimm puts C=63(no-flag) when SetFlags=0.
  // For the B operand, we need it to also be shimm. Hmm.
  //
  // Simpler approach: just use the shimm-in-B form (_rsr) which has
  // A(reg), D(shimm), C(reg), SetFlags, NN. But we don't have a C register...
  //
  // Actually the cleanest: emit the AND_rrs as A=dst, B=dst, D=imm, SF=0, NN=0
  // This computes dst = dst AND imm which is WRONG for MOV.
  //
  // The REAL correct approach per spec: use AND_rrs but set B to also be a
  // shimm sentinel. Let's just output a text "mov" instruction and let the
  // assembler handle it... but we don't have a MOV in the assembler.
  //
  // Best approach: emit as literal text via the InstPrinter path and
  // handle it specially in the code emitter for the shimm format.
  // For now: emit AND_rrs with correct operand order and have the
  // code emitter detect the MOV pattern (B=63 sentinel).
  if (IsMOVri) {
    // Emit: AND_rrs dst, shimm, SetFlags=0, NN=0
    // The code emitter will see: A=dst_reg, B=... but we only have 2 operands
    // (dst, imm). We need to synthesize B.
    // Just pass through CG_MOVri and handle in code emitter.
    OutMI.setOpcode(MI->getOpcode());
    for (const MachineOperand &MO : MI->operands()) {
      MCOperand MCOp = LowerOperand(MO);
      if (MCOp.isValid())
        OutMI.addOperand(MCOp);
    }
    return;
  }

  // Special: MOV limm → pass through
  if (IsMOVli) {
    OutMI.setOpcode(MI->getOpcode());
    for (const MachineOperand &MO : MI->operands()) {
      MCOperand MCOp = LowerOperand(MO);
      if (MCOp.isValid())
        OutMI.addOperand(MCOp);
    }
    return;
  }

  // Special: CG_RET → J_r with B=r31(blink)
  if (MI->getOpcode() == ARC4::CG_RET) {
    OutMI.setOpcode(ARC4::J_r);
    OutMI.addOperand(MCOperand::createReg(ARC4::R31));
    OutMI.addOperand(MCOperand::createImm(0)); // F
    OutMI.addOperand(MCOperand::createImm(0)); // NN
    OutMI.addOperand(MCOperand::createImm(0)); // Q
    return;
  }

  // General case: lower operands
  for (const MachineOperand &MO : MI->operands()) {
    MCOperand MCOp = LowerOperand(MO);
    if (MCOp.isValid())
      OutMI.addOperand(MCOp);
  }

  // Append trailing default operands for MC-layer instructions
  if (NeedsRegFmtTrail) {
    // F=0, NN=0, Q=0
    OutMI.addOperand(MCOperand::createImm(0));
    OutMI.addOperand(MCOperand::createImm(0));
    OutMI.addOperand(MCOperand::createImm(0));
  } else if (NeedsShimmTrail) {
    // SetFlags=0, NN=0
    OutMI.addOperand(MCOperand::createImm(0));
    OutMI.addOperand(MCOperand::createImm(0));
  } else if (NeedsLimmTrail) {
    // F=0, NN=0, Q=0
    OutMI.addOperand(MCOperand::createImm(0));
    OutMI.addOperand(MCOperand::createImm(0));
    OutMI.addOperand(MCOperand::createImm(0));
  }
}
