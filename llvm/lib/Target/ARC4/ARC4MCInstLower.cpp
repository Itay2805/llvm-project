//===- ARC4MCInstLower.cpp - ARC4 MachineInstr to MCInst --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Lower MachineInstr to MCInst, appending suffix operands as needed.
//
//===----------------------------------------------------------------------===//

#include "ARC4MCInstLower.h"
#include "ARC4TSFlags.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"

using namespace llvm;

ARC4MCInstLower::ARC4MCInstLower(MCContext *C, AsmPrinter &AsmPrinter)
    : Ctx(C), Printer(AsmPrinter) {}

MCOperand ARC4MCInstLower::LowerSymbolOperand(const MachineOperand &MO,
                                              MachineOperandType MOTy,
                                              unsigned Offset) const {
  const MCSymbol *Symbol;

  switch (MOTy) {
  case MachineOperand::MO_MachineBasicBlock:
    Symbol = MO.getMBB()->getSymbol();
    break;
  case MachineOperand::MO_GlobalAddress:
    Symbol = Printer.getSymbol(MO.getGlobal());
    Offset += MO.getOffset();
    break;
  case MachineOperand::MO_ExternalSymbol:
    Symbol = Printer.GetExternalSymbolSymbol(MO.getSymbolName());
    Offset += MO.getOffset();
    break;
  case MachineOperand::MO_BlockAddress:
    Symbol = Printer.GetBlockAddressSymbol(MO.getBlockAddress());
    Offset += MO.getOffset();
    break;
  case MachineOperand::MO_JumpTableIndex:
    Symbol = Printer.GetJTISymbol(MO.getIndex());
    break;
  case MachineOperand::MO_ConstantPoolIndex:
    Symbol = Printer.GetCPISymbol(MO.getIndex());
    Offset += MO.getOffset();
    break;
  default:
    llvm_unreachable("<unknown operand type>");
  }

  const MCSymbolRefExpr *MCSym = MCSymbolRefExpr::create(Symbol, *Ctx);

  if (!Offset)
    return MCOperand::createExpr(MCSym);

  const MCConstantExpr *OffsetExpr = MCConstantExpr::create(Offset, *Ctx);
  const MCBinaryExpr *Add = MCBinaryExpr::createAdd(MCSym, OffsetExpr, *Ctx);
  return MCOperand::createExpr(Add);
}

MCOperand ARC4MCInstLower::LowerOperand(const MachineOperand &MO,
                                        unsigned Offset) const {
  MachineOperandType MOTy = MO.getType();

  switch (MOTy) {
  default:
    llvm_unreachable("unknown operand type");
  case MachineOperand::MO_Register:
    if (MO.isImplicit())
      break;
    return MCOperand::createReg(MO.getReg());
  case MachineOperand::MO_Immediate:
    return MCOperand::createImm(MO.getImm() + Offset);
  case MachineOperand::MO_MachineBasicBlock:
  case MachineOperand::MO_GlobalAddress:
  case MachineOperand::MO_ExternalSymbol:
  case MachineOperand::MO_BlockAddress:
  case MachineOperand::MO_JumpTableIndex:
  case MachineOperand::MO_ConstantPoolIndex:
    return LowerSymbolOperand(MO, MOTy, Offset);
  case MachineOperand::MO_RegisterMask:
    break;
  }

  return {};
}

void ARC4MCInstLower::Lower(const MachineInstr *MI, MCInst &OutMI) const {
  OutMI.setOpcode(MI->getOpcode());

  // Lower all explicit operands from the MachineInstr.
  for (const MachineOperand &MO : MI->operands()) {
    MCOperand MCOp = LowerOperand(MO);
    if (MCOp.isValid())
      OutMI.addOperand(MCOp);
  }

  // Append suffix operands based on TSFlags.
  // MachineInstrs from the codegen pipeline don't have suffix operands;
  // the MCInst encoding expects them. We append default (0) values.
  const MCInstrDesc &Desc = MI->getDesc();
  uint64_t TSF = Desc.TSFlags;

  if (TSF & ARC4TSF::HasFQ) {
    // Already present from MachineInstr operands — the codegen emits them
    // because they're in the (ins) list. No need to append.
  } else if (TSF & ARC4TSF::HasFOnly) {
    // Same — f is in the (ins) list.
  } else if (TSF & ARC4TSF::HasQN) {
    // q, n in (ins) list.
  } else if (TSF & ARC4TSF::HasFQN) {
    // f, q, n in (ins) list.
  } else if (TSF & ARC4TSF::HasLdMod3) {
    // x, w, e in (ins) list.
  } else if (TSF & ARC4TSF::HasLdMod2) {
    // x, e in (ins) list.
  } else if (TSF & ARC4TSF::HasStMod2) {
    // v, D in (ins) list.
  } else if (TSF & ARC4TSF::HasStMod1) {
    // D in (ins) list.
  }
  // Suffix operands are already in the MachineInstr from ISel (they're
  // defined in the (ins) list of each instruction), so we just let them
  // flow through the normal operand lowering above.
}
