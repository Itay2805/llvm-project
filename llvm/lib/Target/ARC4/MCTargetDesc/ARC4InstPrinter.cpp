//===-- ARC4InstPrinter.cpp - Convert ARC4 MCInst to asm ------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ARC4InstPrinter.h"
#include "ARC4MCTargetDesc.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

#define DEBUG_TYPE "asm-printer"

#define PRINT_ALIAS_INSTR
#include "ARC4GenAsmWriter.inc"

void ARC4InstPrinter::printRegName(raw_ostream &O, MCRegister Reg) {
  O << getRegisterName(Reg);
}

/// Condition code names indexed by 5-bit encoding value.
static const char *const CondCodeNames[] = {
    "",    ".eq", ".ne", ".pl", ".mi", ".cs", ".cc", ".vs",
    ".vc", ".gt", ".ge", ".lt", ".le", ".hi", ".ls", ".pnz",
};

/// Build the suffix string for the given MCInst by reading its suffix operands.
/// The suffix goes after the mnemonic: "add.eq.f", "ld.x.a.di", "b.ne.d".
static void buildSuffixString(const MCInst *MI, const MCInstrDesc &Desc,
                               SmallVectorImpl<char> &Out) {
  unsigned Opc = MI->getOpcode();
  unsigned ExpOps = Desc.getNumOperands();
  if (MI->getNumOperands() < ExpOps)
    return;

  raw_svector_ostream OS(Out);

  // === Jump: last 3 = f, q, n ===
  switch (Opc) {
  case ARC4::J_r: case ARC4::J_l:
  case ARC4::JL_r: case ARC4::JL_l: {
    int F = MI->getOperand(ExpOps - 3).getImm();
    int Q = MI->getOperand(ExpOps - 2).getImm();
    int N = MI->getOperand(ExpOps - 1).getImm();
    if (Q > 0 && Q < 16) OS << CondCodeNames[Q];
    if (F) OS << ".f";
    if (N == 1) OS << ".d";
    else if (N == 2) OS << ".jd";
    return;
  }
  default: break;
  }

  // === Branch: last 2 = q, n ===
  switch (Opc) {
  case ARC4::B: case ARC4::BL: case ARC4::LP_insn: {
    int Q = MI->getOperand(ExpOps - 2).getImm();
    int N = MI->getOperand(ExpOps - 1).getImm();
    if (Q > 0 && Q < 16) OS << CondCodeNames[Q];
    if (N == 1) OS << ".d";
    else if (N == 2) OS << ".jd";
    return;
  }
  default: break;
  }

  // === Flag: last 1 = q ===
  switch (Opc) {
  case ARC4::FLAG_r: case ARC4::FLAG_l: {
    int Q = MI->getOperand(ExpOps - 1).getImm();
    if (Q > 0 && Q < 16) OS << CondCodeNames[Q];
    return;
  }
  default: break;
  }

  // === Load opcode 0 with 3 modifiers: x, w, e ===
  switch (Opc) {
  case ARC4::LD_rr:  case ARC4::LD_rl:
  case ARC4::LDB_rr: case ARC4::LDB_rl:
  case ARC4::LDW_rr: case ARC4::LDW_rl: {
    int X = MI->getOperand(ExpOps - 3).getImm();
    int W = MI->getOperand(ExpOps - 2).getImm();
    int E = MI->getOperand(ExpOps - 1).getImm();
    if (X) OS << ".x";
    if (W) OS << ".a";
    if (E) OS << ".di";
    return;
  }
  default: break;
  }

  // === Load opcode 0 with 2 modifiers: x, e (limm base, no writeback) ===
  switch (Opc) {
  case ARC4::LD_lr:  case ARC4::LDB_lr: case ARC4::LDW_lr: {
    int X = MI->getOperand(ExpOps - 2).getImm();
    int E = MI->getOperand(ExpOps - 1).getImm();
    if (X) OS << ".x";
    if (E) OS << ".di";
    return;
  }
  default: break;
  }

  // === Load opcode 1 with 3 modifiers: X, W, E ===
  switch (Opc) {
  case ARC4::LD_rs:  case ARC4::LDB_rs: case ARC4::LDW_rs: {
    int X = MI->getOperand(ExpOps - 3).getImm();
    int W = MI->getOperand(ExpOps - 2).getImm();
    int E = MI->getOperand(ExpOps - 1).getImm();
    if (X) OS << ".x";
    if (W) OS << ".a";
    if (E) OS << ".di";
    return;
  }
  default: break;
  }

  // === Load opcode 1 with 2 modifiers: X, E (no writeback) ===
  switch (Opc) {
  case ARC4::LD_ss:  case ARC4::LD_l:
  case ARC4::LDB_ss: case ARC4::LDB_l:
  case ARC4::LDW_ss: case ARC4::LDW_l: {
    int X = MI->getOperand(ExpOps - 2).getImm();
    int E = MI->getOperand(ExpOps - 1).getImm();
    if (X) OS << ".x";
    if (E) OS << ".di";
    return;
  }
  default: break;
  }

  // === Store with 2 modifiers: v, D (reg base) ===
  switch (Opc) {
  case ARC4::ST_rrs:  case ARC4::ST_srs:  case ARC4::ST_lrs:
  case ARC4::STB_rrs: case ARC4::STB_srs: case ARC4::STB_lrs:
  case ARC4::STW_rrs: case ARC4::STW_srs: case ARC4::STW_lrs: {
    int V = MI->getOperand(ExpOps - 2).getImm();
    int D = MI->getOperand(ExpOps - 1).getImm();
    if (V) OS << ".a";
    if (D) OS << ".di";
    return;
  }
  default: break;
  }

  // === Store with 1 modifier: D (no writeback) ===
  switch (Opc) {
  case ARC4::ST_rss:  case ARC4::ST_sss:  case ARC4::ST_rls:
  case ARC4::ST_lls:  case ARC4::ST_sls:
  case ARC4::STB_rss: case ARC4::STB_sss: case ARC4::STB_rls:
  case ARC4::STB_lls: case ARC4::STB_sls:
  case ARC4::STW_rss: case ARC4::STW_sss: case ARC4::STW_rls:
  case ARC4::STW_lls: case ARC4::STW_sls: {
    int D = MI->getOperand(ExpOps - 1).getImm();
    if (D) OS << ".di";
    return;
  }
  default: break;
  }

  // === ALU/SOP shimm forms: last 1 = f ===
  switch (Opc) {
  case ARC4::ADD_rrs: case ARC4::ADD_rsr: case ARC4::ADD_rss:
  case ARC4::ADD_0rs: case ARC4::ADD_0sr: case ARC4::ADD_0ss:
  case ARC4::ADC_rrs: case ARC4::ADC_rsr: case ARC4::ADC_rss:
  case ARC4::ADC_0rs: case ARC4::ADC_0sr: case ARC4::ADC_0ss:
  case ARC4::SUB_rrs: case ARC4::SUB_rsr: case ARC4::SUB_rss:
  case ARC4::SUB_0rs: case ARC4::SUB_0sr: case ARC4::SUB_0ss:
  case ARC4::SBC_rrs: case ARC4::SBC_rsr: case ARC4::SBC_rss:
  case ARC4::SBC_0rs: case ARC4::SBC_0sr: case ARC4::SBC_0ss:
  case ARC4::AND_rrs: case ARC4::AND_rsr: case ARC4::AND_rss:
  case ARC4::AND_0rs: case ARC4::AND_0sr: case ARC4::AND_0ss:
  case ARC4::OR_rrs:  case ARC4::OR_rsr:  case ARC4::OR_rss:
  case ARC4::OR_0rs:  case ARC4::OR_0sr:  case ARC4::OR_0ss:
  case ARC4::BIC_rrs: case ARC4::BIC_rsr: case ARC4::BIC_rss:
  case ARC4::BIC_0rs: case ARC4::BIC_0sr: case ARC4::BIC_0ss:
  case ARC4::XOR_rrs: case ARC4::XOR_rsr: case ARC4::XOR_rss:
  case ARC4::XOR_0rs: case ARC4::XOR_0sr: case ARC4::XOR_0ss:
  case ARC4::ASR_rs:  case ARC4::ASR_0s:
  case ARC4::LSR_rs:  case ARC4::LSR_0s:
  case ARC4::ROR_rs:  case ARC4::ROR_0s:
  case ARC4::RRC_rs:  case ARC4::RRC_0s:
  case ARC4::SEXB_rs: case ARC4::SEXB_0s:
  case ARC4::SEXW_rs: case ARC4::SEXW_0s:
  case ARC4::EXTB_rs: case ARC4::EXTB_0s:
  case ARC4::EXTW_rs: case ARC4::EXTW_0s: {
    int F = MI->getOperand(ExpOps - 1).getImm();
    if (F) OS << ".f";
    return;
  }
  default: break;
  }

  // === ALU/SOP non-shimm forms: last 2 = f, q ===
  // This is the fallback for all remaining ALU/SOP instructions.
  if (ExpOps >= 2) {
    const MCOperand &OpF = MI->getOperand(ExpOps - 2);
    const MCOperand &OpQ = MI->getOperand(ExpOps - 1);
    if (OpF.isImm() && OpQ.isImm()) {
      int F = OpF.getImm();
      int Q = OpQ.getImm();
      if (Q > 0 && Q < 16) OS << CondCodeNames[Q];
      if (F) OS << ".f";
    }
  }
}

void ARC4InstPrinter::printInst(const MCInst *MI, uint64_t Address,
                                StringRef Annot, const MCSubtargetInfo &STI,
                                raw_ostream &O) {
  const MCInstrDesc &Desc = MII.get(MI->getOpcode());

  // Build suffix string from operands.
  SmallString<16> Suffix;
  buildSuffixString(MI, Desc, Suffix);

  if (!Suffix.empty()) {
    // Print to temp buffer, then inject suffix after the mnemonic.
    SmallString<128> Tmp;
    raw_svector_ostream TmpO(Tmp);
    bool IsAlias = printAliasInstr(MI, Address, TmpO);
    if (!IsAlias)
      printInstruction(MI, Address, TmpO);

    // Suppress redundant .f on aliases that inherently set flags.
    // cmp = sub.f 0, rlc = adc.f — the alias name already implies .f.
    if (IsAlias && Suffix.ends_with(".f")) {
      StringRef S = Tmp.str();
      size_t MnStart = S.find_first_not_of(" \t");
      if (MnStart != StringRef::npos) {
        StringRef Mn = S.substr(MnStart).split('\t').first;
        if (Mn == "cmp" || Mn == "rlc")
          Suffix.resize(Suffix.size() - 2); // remove ".f"
      }
    }

    StringRef S = Tmp.str();
    // Skip leading whitespace (tab indentation from printInstruction).
    size_t MnStart = S.find_first_not_of(" \t");
    if (MnStart == StringRef::npos) {
      O << S;
    } else {
      // Find end of mnemonic.
      size_t MnEnd = S.find_first_of(" \t", MnStart);
      if (MnEnd == StringRef::npos) {
        // Mnemonic only, no operands.
        O << S.slice(0, MnStart) << S.slice(MnStart, StringRef::npos) << Suffix;
      } else {
        O << S.slice(0, MnEnd) << Suffix << S.slice(MnEnd, StringRef::npos);
      }
    }
  } else {
    if (!printAliasInstr(MI, Address, O))
      printInstruction(MI, Address, O);
  }

  printAnnotation(O, Annot);
}

void ARC4InstPrinter::printLdSsAddr(const MCInst *MI, unsigned OpNo,
                                     raw_ostream &O) {
  const MCOperand &Op = MI->getOperand(OpNo);
  if (Op.isImm())
    O << (Op.getImm() * 2);
  else if (Op.isExpr())
    MAI.printExpr(O, *Op.getExpr());
}

void ARC4InstPrinter::printOperand(const MCInst *MI, unsigned OpNo,
                                   raw_ostream &O) {
  const MCOperand &Op = MI->getOperand(OpNo);
  if (Op.isReg())
    printRegName(O, Op.getReg());
  else if (Op.isImm())
    O << Op.getImm();
  else if (Op.isExpr())
    MAI.printExpr(O, *Op.getExpr());
}
