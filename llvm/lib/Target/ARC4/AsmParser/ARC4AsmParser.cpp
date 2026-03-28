//===-- ARC4AsmParser.cpp - Parse ARC4 assembly to MCInst -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/ARC4MCTargetDesc.h"
#include "TargetInfo/ARC4TargetInfo.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCParser/AsmLexer.h"
#include "llvm/MC/MCParser/MCAsmParser.h"
#include "llvm/MC/MCParser/MCParsedAsmOperand.h"
#include "llvm/MC/MCParser/MCTargetAsmParser.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/SMLoc.h"
#include "llvm/Support/raw_ostream.h"
#include <memory>

using namespace llvm;

// Forward declarations for generated functions.
static MCRegister MatchRegisterName(StringRef Name);
static MCRegister MatchRegisterAltName(StringRef Name);

namespace {

struct ARC4Operand : public MCParsedAsmOperand {
  enum KindTy { Token, Register, Immediate } Kind;
  SMLoc StartLoc, EndLoc;

  StringRef Tok;
  MCRegister RegNum;
  const MCExpr *Expr;

  ARC4Operand(KindTy K, SMLoc S, SMLoc E) : Kind(K), StartLoc(S), EndLoc(E) {}

public:
  static std::unique_ptr<ARC4Operand> createToken(StringRef Str, SMLoc S) {
    auto Op = std::make_unique<ARC4Operand>(Token, S, S);
    Op->Tok = Str;
    return Op;
  }
  static std::unique_ptr<ARC4Operand> createReg(MCRegister Reg, SMLoc S,
                                                 SMLoc E) {
    auto Op = std::make_unique<ARC4Operand>(Register, S, E);
    Op->RegNum = Reg;
    return Op;
  }
  static std::unique_ptr<ARC4Operand> createImm(const MCExpr *Val, SMLoc S,
                                                 SMLoc E) {
    auto Op = std::make_unique<ARC4Operand>(Immediate, S, E);
    Op->Expr = Val;
    return Op;
  }

  bool isToken() const override { return Kind == Token; }
  bool isReg() const override { return Kind == Register; }
  bool isImm() const override { return Kind == Immediate; }
  bool isMem() const override { return false; }

  bool isSimm9() const {
    if (!isImm())
      return false;
    if (const auto *CE = dyn_cast<MCConstantExpr>(Expr)) {
      int64_t Val = CE->getValue();
      return Val >= -256 && Val <= 255;
    }
    return false;  // symbolic exprs don't fit in shimm
  }

  bool isLimm32() const { return isImm(); }
  bool isBrTarget20() const { return isImm(); }

  StringRef getToken() const {
    assert(Kind == Token);
    return Tok;
  }
  MCRegister getReg() const override {
    assert(Kind == Register);
    return RegNum;
  }

  SMLoc getStartLoc() const override { return StartLoc; }
  SMLoc getEndLoc() const override { return EndLoc; }

  void addRegOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1);
    Inst.addOperand(MCOperand::createReg(getReg()));
  }
  void addImmOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1);
    if (const auto *CE = dyn_cast<MCConstantExpr>(Expr))
      Inst.addOperand(MCOperand::createImm(CE->getValue()));
    else
      Inst.addOperand(MCOperand::createExpr(Expr));
  }

  void print(raw_ostream &OS, const MCAsmInfo &MAI) const override {
    switch (Kind) {
    case Token:
      OS << "Token: " << Tok;
      break;
    case Register:
      OS << "Register: " << RegNum;
      break;
    case Immediate:
      OS << "Immediate";
      break;
    }
  }
};

class ARC4AsmParser : public MCTargetAsmParser {
  MCRegister tryParseRegisterName(StringRef Name);
  bool parseOperand(OperandVector &Operands);
  bool parseRegOrImm(OperandVector &Operands);

  // Suffix state parsed from the mnemonic, used by matchAndEmitInstruction.
  int ParsedCondCode = 0;     // 5-bit condition code (0 = always)
  int ParsedFlagBit = 0;      // 1 = .f suffix present
  int ParsedDelaySlot = 0;    // 0=nd, 1=d, 2=jd
  bool ParsedHasExplicitSuffix = false;  // true if any dot-suffix was parsed

#define GET_ASSEMBLER_HEADER
#include "ARC4GenAsmMatcher.inc"

public:
  ARC4AsmParser(const MCSubtargetInfo &STI, MCAsmParser &Parser,
                const MCInstrInfo &MII, const MCTargetOptions &Options)
      : MCTargetAsmParser(Options, STI, MII) {
    setAvailableFeatures(ComputeAvailableFeatures(STI.getFeatureBits()));
  }

  bool parseRegister(MCRegister &Reg, SMLoc &StartLoc,
                     SMLoc &EndLoc) override;
  ParseStatus tryParseRegister(MCRegister &Reg, SMLoc &StartLoc,
                               SMLoc &EndLoc) override;
  bool parseInstruction(ParseInstructionInfo &Info, StringRef Name,
                        SMLoc NameLoc, OperandVector &Operands) override;
  bool matchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                               OperandVector &Operands, MCStreamer &Out,
                               uint64_t &ErrorInfo,
                               bool MatchingInlineAsm) override;
};

} // end anonymous namespace

MCRegister ARC4AsmParser::tryParseRegisterName(StringRef Name) {
  MCRegister Reg = MatchRegisterName(Name);
  if (!Reg)
    Reg = MatchRegisterAltName(Name);
  return Reg;
}

bool ARC4AsmParser::parseRegister(MCRegister &Reg, SMLoc &StartLoc,
                                  SMLoc &EndLoc) {
  if (!tryParseRegister(Reg, StartLoc, EndLoc).isSuccess())
    return Error(StartLoc, "invalid register name");
  return false;
}

ParseStatus ARC4AsmParser::tryParseRegister(MCRegister &Reg, SMLoc &StartLoc,
                                            SMLoc &EndLoc) {
  StartLoc = getLexer().getLoc();
  if (getLexer().isNot(AsmToken::Identifier))
    return ParseStatus::NoMatch;
  Reg = tryParseRegisterName(getLexer().getTok().getIdentifier());
  if (!Reg)
    return ParseStatus::NoMatch;
  EndLoc = getLexer().getLoc();
  getParser().Lex();
  return ParseStatus::Success;
}

bool ARC4AsmParser::parseOperand(OperandVector &Operands) {
  SMLoc Start = getLexer().getLoc();

  // Memory operand: [base] or [base, offset]
  if (getLexer().is(AsmToken::LBrac)) {
    Operands.push_back(ARC4Operand::createToken("[", Start));
    getParser().Lex(); // eat '['

    // Parse first operand inside brackets (base).
    if (parseRegOrImm(Operands))
      return true;

    // If comma follows, parse second operand (offset).
    if (getLexer().is(AsmToken::Comma)) {
      getParser().Lex(); // eat ','
      if (parseRegOrImm(Operands))
        return true;
    }

    if (getLexer().isNot(AsmToken::RBrac))
      return Error(getLexer().getLoc(), "expected ']'");

    SMLoc End = getLexer().getLoc();
    Operands.push_back(ARC4Operand::createToken("]", End));
    getParser().Lex(); // eat ']'
    return false;
  }

  // Try register.
  if (getLexer().is(AsmToken::Identifier)) {
    MCRegister Reg = tryParseRegisterName(getLexer().getTok().getIdentifier());
    if (Reg) {
      SMLoc End = getLexer().getLoc();
      getParser().Lex();
      Operands.push_back(ARC4Operand::createReg(Reg, Start, End));
      return false;
    }
  }

  // Try immediate / expression.
  const MCExpr *Expr;
  if (!getParser().parseExpression(Expr)) {
    Operands.push_back(
        ARC4Operand::createImm(Expr, Start, getLexer().getLoc()));
    return false;
  }
  return true;
}

bool ARC4AsmParser::parseRegOrImm(OperandVector &Operands) {
  SMLoc Start = getLexer().getLoc();

  // Try register.
  if (getLexer().is(AsmToken::Identifier)) {
    MCRegister Reg = tryParseRegisterName(getLexer().getTok().getIdentifier());
    if (Reg) {
      SMLoc End = getLexer().getLoc();
      getParser().Lex();
      Operands.push_back(ARC4Operand::createReg(Reg, Start, End));
      return false;
    }
  }

  // Try immediate / expression.
  const MCExpr *Expr;
  if (!getParser().parseExpression(Expr)) {
    Operands.push_back(
        ARC4Operand::createImm(Expr, Start, getLexer().getLoc()));
    return false;
  }
  return true;
}

/// Map a condition code suffix to its 5-bit encoding value.
/// Returns -1 if the suffix is not a recognized condition code.
static int mapConditionCode(StringRef Suffix) {
  return StringSwitch<int>(Suffix)
      .Case("al", 0)
      .Case("eq", 1).Case("z", 1)
      .Case("ne", 2).Case("nz", 2)
      .Case("pl", 3).Case("p", 3)
      .Case("mi", 4).Case("n", 4)
      .Case("cs", 5).Case("lo", 5).Case("c", 5)
      .Case("cc", 6).Case("hs", 6).Case("nc", 6)
      .Case("vs", 7).Case("v", 7)
      .Case("vc", 8).Case("nv", 8)
      .Case("gt", 9)
      .Case("ge", 10)
      .Case("lt", 11)
      .Case("le", 12)
      .Case("hi", 13)
      .Case("ls", 14)
      .Case("pnz", 15)
      .Default(-1);
}

bool ARC4AsmParser::parseInstruction(ParseInstructionInfo &Info, StringRef Name,
                                     SMLoc NameLoc,
                                     OperandVector &Operands) {
  // Strip mnemonic suffixes.
  // Load/store size suffixes transform the mnemonic: ld.b -> ldb, st.w -> stw.
  // .f, .q (condition codes), .d/.nd/.jd (delay slots) are stored in member
  // variables for matchAndEmitInstruction to use when filling suffix operands.
  SmallString<16> MnemonicBuf;

  // Reset suffix state for this instruction.
  ParsedCondCode = 0;
  ParsedFlagBit = 0;
  ParsedDelaySlot = 0;
  ParsedHasExplicitSuffix = false;

  // Process dot-separated suffixes from left to right.
  StringRef Remaining = Name;
  StringRef Mnemonic;
  std::tie(Mnemonic, Remaining) = Remaining.split('.');

  // Accumulate the base mnemonic, handling size suffixes that merge.
  MnemonicBuf = Mnemonic;
  while (!Remaining.empty()) {
    StringRef Suffix;
    std::tie(Suffix, Remaining) = Remaining.split('.');

    // Size suffixes for load/store: merge into base mnemonic.
    if (Suffix == "b" || Suffix == "w") {
      MnemonicBuf += Suffix;
      continue;
    }

    // Flag suffix.
    if (Suffix == "f") {
      ParsedFlagBit = 1;
      ParsedHasExplicitSuffix = true;
      continue;
    }

    // Delay slot suffixes.
    if (Suffix == "nd") { ParsedDelaySlot = 0; ParsedHasExplicitSuffix = true; continue; }
    if (Suffix == "d")  { ParsedDelaySlot = 1; ParsedHasExplicitSuffix = true; continue; }
    if (Suffix == "jd") { ParsedDelaySlot = 2; ParsedHasExplicitSuffix = true; continue; }

    // Sign extend, writeback, cache bypass - strip for now.
    if (Suffix == "x" || Suffix == "a" || Suffix == "di")
      continue;

    // Condition codes.
    int CC = mapConditionCode(Suffix);
    if (CC >= 0) {
      ParsedCondCode = CC;
      ParsedHasExplicitSuffix = true;
      continue;
    }

    // Unknown suffix: keep it attached (may be part of the mnemonic).
    MnemonicBuf += '.';
    MnemonicBuf += Suffix;
  }

  // If the mnemonic was modified, persist the new string in the MCContext.
  // Otherwise, use the original Name which is already stable.
  if (MnemonicBuf == Name)
    Operands.push_back(ARC4Operand::createToken(Name, NameLoc));
  else
    Operands.push_back(ARC4Operand::createToken(
        getContext().allocateString(MnemonicBuf), NameLoc));

  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    if (parseOperand(Operands))
      return true;
    while (getLexer().is(AsmToken::Comma)) {
      getParser().Lex();
      if (parseOperand(Operands))
        return true;
    }
  }

  if (getLexer().isNot(AsmToken::EndOfStatement))
    return Error(getLexer().getLoc(), "unexpected token in operand list");
  getParser().Lex();

  // Fix (rss form): If we have pattern <mnemonic> <reg> <imm> <imm> where
  // both immediates are equal and fit in 9 bits, collapse to <mnemonic> <reg>
  // <imm> so the matcher finds the rss variant.
  //
  // The rss instruction's AsmString uses "$shimm, $shimm" (same operand twice),
  // which the AsmMatcher can't handle for parsing. So we collapse duplicate
  // trailing immediates here and match via hidden InstAliases in TableGen.
  // Operands layout: [0]=token, [1]=reg, [2]=imm, [3]=imm
  if (Operands.size() == 4) {
    auto *Op1 = static_cast<ARC4Operand *>(Operands[1].get());
    auto *Op2 = static_cast<ARC4Operand *>(Operands[2].get());
    auto *Op3 = static_cast<ARC4Operand *>(Operands[3].get());
    if (Op1->isReg() && Op2->isImm() && Op3->isImm()) {
      const auto *CE2 = dyn_cast<MCConstantExpr>(Op2->Expr);
      const auto *CE3 = dyn_cast<MCConstantExpr>(Op3->Expr);
      if (CE2 && CE3 && CE2->getValue() == CE3->getValue()) {
        int64_t Val = CE2->getValue();
        if (Val >= -256 && Val <= 255) {
          // Drop the duplicate trailing immediate.
          Operands.pop_back();
        }
      }
    }
  }
  // Also handle the discard form: <mnemonic> 0, <imm>, <imm>
  // Operands: [0]=token, [1]=imm(0), [2]=imm, [3]=imm
  if (Operands.size() == 4) {
    auto *Op1 = static_cast<ARC4Operand *>(Operands[1].get());
    auto *Op2 = static_cast<ARC4Operand *>(Operands[2].get());
    auto *Op3 = static_cast<ARC4Operand *>(Operands[3].get());
    if (Op1->isImm() && Op2->isImm() && Op3->isImm()) {
      const auto *CE1 = dyn_cast<MCConstantExpr>(Op1->Expr);
      const auto *CE2 = dyn_cast<MCConstantExpr>(Op2->Expr);
      const auto *CE3 = dyn_cast<MCConstantExpr>(Op3->Expr);
      if (CE1 && CE1->getValue() == 0 && CE2 && CE3 &&
          CE2->getValue() == CE3->getValue()) {
        int64_t Val = CE2->getValue();
        if (Val >= -256 && Val <= 255) {
          Operands.pop_back();
        }
      }
    }
  }

  // Fix (discard form): Convert immediate 0 in the destination position to a
  // Token "0" so the AsmMatcher can match against discard instruction variants
  // (e.g. ADD_0rr) that have literal "0" in their AsmString.
  // Only apply to ALU3 and SOP mnemonics that have discard variants.
  // Layout: [0]=mnemonic, [1]=imm(0), ...
  {
    bool HasDiscardForm =
        StringSwitch<bool>(MnemonicBuf)
            .Case("add", true).Case("adc", true)
            .Case("sub", true).Case("sbc", true)
            .Case("and", true).Case("or", true)
            .Case("bic", true).Case("xor", true)
            .Case("asl", true).Case("asr", true)
            .Case("lsr", true).Case("ror", true)
            .Case("rrc", true).Case("sexb", true)
            .Case("sexw", true).Case("extb", true)
            .Case("extw", true)
            .Default(false);
    if (HasDiscardForm && Operands.size() >= 2) {
      auto *Op1 = static_cast<ARC4Operand *>(Operands[1].get());
      if (Op1->isImm()) {
        if (const auto *CE = dyn_cast<MCConstantExpr>(Op1->Expr)) {
          if (CE->getValue() == 0) {
            SMLoc S = Op1->getStartLoc();
            Operands[1] = ARC4Operand::createToken("0", S);
          }
        }
      }
    }
  }

  // Fix (store shimm forms): Collapse duplicate shimm operands in store
  // instructions so the AsmMatcher can find the shimm variants.
  //
  // The store shimm forms (srs, rss, sss) have tied operands in their
  // AsmString (e.g., "st $offset, [$b, $offset]") which the AsmMatcher
  // can't parse. We collapse duplicate bracket immediates here; the srs
  // and sss forms are handled in matchAndEmitInstruction via manual MCInst
  // construction after the initial match selects a limm variant.
  //
  // Inner bracket collapse: st ?, [imm, imm] where imms are equal and
  // shimm-sized -> st ?, [imm].  Matches ST_rss via existing AsmMatcher entry.
  StringRef MnBase = static_cast<ARC4Operand *>(Operands[0].get())->getToken();
  bool IsStore = (MnBase == "st" || MnBase == "stb" || MnBase == "stw");

  if (IsStore && Operands.size() == 6) {
    auto *O2 = static_cast<ARC4Operand *>(Operands[2].get());
    auto *O3 = static_cast<ARC4Operand *>(Operands[3].get());
    auto *O4 = static_cast<ARC4Operand *>(Operands[4].get());
    auto *O5 = static_cast<ARC4Operand *>(Operands[5].get());
    // Check: ?, [, imm, imm, ]
    if (O2->isToken() && O2->getToken() == "[" &&
        O3->isImm() && O4->isImm() &&
        O5->isToken() && O5->getToken() == "]") {
      const auto *CE3 = dyn_cast<MCConstantExpr>(O3->Expr);
      const auto *CE4 = dyn_cast<MCConstantExpr>(O4->Expr);
      if (CE3 && CE4 && CE3->getValue() == CE4->getValue()) {
        int64_t Val = CE3->getValue();
        if (Val >= -256 && Val <= 255) {
          // Remove the second bracket imm (index 4), shifting ] down.
          Operands.erase(Operands.begin() + 4);
        }
      }
    }
  }

  return false;
}

/// Returns true if the opcode is a branch or jump instruction
/// (opcodes 4-7: b, bl, lp, j/jl) where delay slot modifiers are valid.
static bool isBranchOrJump(unsigned Opc) {
  switch (Opc) {
  case ARC4::B:
  case ARC4::BL:
  case ARC4::LP_insn:
  case ARC4::J_r:
  case ARC4::J_l:
  case ARC4::JL_r:
  case ARC4::JL_l:
    return true;
  default:
    return false;
  }
}

/// Returns true if the instruction is a shimm form (uses 9-bit short immediate
/// in bits [8:0], which overlaps with the condition code field [4:0]).
/// Condition codes are NOT compatible with shimm forms.
static bool isShimmForm(unsigned Opc) {
  switch (Opc) {
  // ALU3 shimm variants
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
  // SOP shimm variants
  case ARC4::ASR_rs:  case ARC4::ASR_0s:
  case ARC4::LSR_rs:  case ARC4::LSR_0s:
  case ARC4::ROR_rs:  case ARC4::ROR_0s:
  case ARC4::RRC_rs:  case ARC4::RRC_0s:
  case ARC4::SEXB_rs: case ARC4::SEXB_0s:
  case ARC4::SEXW_rs: case ARC4::SEXW_0s:
  case ARC4::EXTB_rs: case ARC4::EXTB_0s:
  case ARC4::EXTW_rs: case ARC4::EXTW_0s:
  // Flag shimm
  case ARC4::FLAG_s:
  // Store forms (all stores use shimm offset in bits[8:0])
  case ARC4::ST_rrs:  case ARC4::ST_srs:  case ARC4::ST_rss:
  case ARC4::ST_sss:  case ARC4::ST_rls:  case ARC4::ST_lrs:
  case ARC4::ST_lls:  case ARC4::ST_sls:
  case ARC4::STB_rrs: case ARC4::STB_srs: case ARC4::STB_rss:
  case ARC4::STB_sss: case ARC4::STB_rls: case ARC4::STB_lrs:
  case ARC4::STB_lls: case ARC4::STB_sls:
  case ARC4::STW_rrs: case ARC4::STW_srs: case ARC4::STW_rss:
  case ARC4::STW_sss: case ARC4::STW_rls: case ARC4::STW_lrs:
  case ARC4::STW_lls: case ARC4::STW_sls:
  // Load shimm forms (opcode 1)
  case ARC4::LD_rs:   case ARC4::LD_ss:
  case ARC4::LDB_rs:  case ARC4::LDB_ss:
  case ARC4::LDW_rs:  case ARC4::LDW_ss:
    return true;
  default:
    return false;
  }
}

/// Try to match a store instruction to a shimm form (srs, sss, or sls) when
/// the AsmMatcher selected a limm form or failed to match.  Returns true if a
/// shimm form was successfully emitted.
///
/// Store shimm forms have tied operands in their AsmString which the
/// AsmMatcher cannot handle.  We detect the patterns here and manually
/// build the MCInst.
///
/// Pattern srs: "st val, [base, offset]" where val == offset, both shimm.
///   Operands (before annotation strip): [mnem, imm, [, reg, imm, ]]
///
/// Pattern sss: "st val, [base, offset]" where val == base == offset, all shimm.
///   After inner-bracket collapse in parser: [mnem, imm, [, imm, ]]
///   Original form: [mnem, imm, [, imm, imm, ]]  (all three equal)
///
/// Pattern sls: "st shimm, [limm]" — shimm value, limm address.
///   Operands: [mnem, imm_val, [, imm_addr, ]]  (5 operands)
///   The limm is adjusted: limm_encoded = addr - shimm.
static bool tryMatchStoreSRS(StringRef Mnemonic, OperandVector &Operands,
                             MCInst &Inst) {
  // Map mnemonic to srs/sss/sls opcode triples.
  unsigned SRSOpc = 0, SSSOpc = 0, SLSOpc = 0;
  if (Mnemonic == "st") {
    SRSOpc = ARC4::ST_srs;  SSSOpc = ARC4::ST_sss;  SLSOpc = ARC4::ST_sls;
  } else if (Mnemonic == "stb") {
    SRSOpc = ARC4::STB_srs; SSSOpc = ARC4::STB_sss; SLSOpc = ARC4::STB_sls;
  } else if (Mnemonic == "stw") {
    SRSOpc = ARC4::STW_srs; SSSOpc = ARC4::STW_sss; SLSOpc = ARC4::STW_sls;
  } else {
    return false;
  }

  // Pattern srs: [mnem, imm_val, [, reg_base, imm_offset, ]]  (6 operands)
  if (Operands.size() == 6) {
    auto *O1 = static_cast<ARC4Operand *>(Operands[1].get());
    auto *O2 = static_cast<ARC4Operand *>(Operands[2].get());
    auto *O3 = static_cast<ARC4Operand *>(Operands[3].get());
    auto *O4 = static_cast<ARC4Operand *>(Operands[4].get());
    auto *O5 = static_cast<ARC4Operand *>(Operands[5].get());
    if (O1->isImm() && O2->isToken() && O2->getToken() == "[" &&
        O3->isReg() && O4->isImm() &&
        O5->isToken() && O5->getToken() == "]") {
      const auto *CEVal = dyn_cast<MCConstantExpr>(O1->Expr);
      const auto *CEOff = dyn_cast<MCConstantExpr>(O4->Expr);
      if (CEVal && CEOff && CEVal->getValue() == CEOff->getValue()) {
        int64_t Val = CEVal->getValue();
        if (Val >= -256 && Val <= 255) {
          // Build ST_srs: (ins simm9:$offset, GPR32:$b)
          Inst.clear();
          Inst.setOpcode(SRSOpc);
          Inst.addOperand(MCOperand::createImm(Val));    // offset (= val)
          Inst.addOperand(MCOperand::createReg(O3->getReg())); // base
          return true;
        }
      }
    }
  }

  // 5-operand patterns: [mnem, imm_val, [, imm_addr, ]]
  if (Operands.size() == 5) {
    auto *O1 = static_cast<ARC4Operand *>(Operands[1].get());
    auto *O2 = static_cast<ARC4Operand *>(Operands[2].get());
    auto *O3 = static_cast<ARC4Operand *>(Operands[3].get());
    auto *O4 = static_cast<ARC4Operand *>(Operands[4].get());
    if (O1->isImm() && O2->isToken() && O2->getToken() == "[" &&
        O3->isImm() && O4->isToken() && O4->getToken() == "]") {
      const auto *CEVal = dyn_cast<MCConstantExpr>(O1->Expr);
      const auto *CEAddr = dyn_cast<MCConstantExpr>(O3->Expr);
      if (CEVal && CEAddr) {
        int64_t Val = CEVal->getValue();
        int64_t Addr = CEAddr->getValue();
        if (Val >= -256 && Val <= 255) {
          if (Val == Addr) {
            // Pattern sss: val == addr, both shimm.
            // Build ST_sss: (ins st_offset9:$offset)
            Inst.clear();
            Inst.setOpcode(SSSOpc);
            Inst.addOperand(MCOperand::createImm(Val));  // offset (= val = base)
            return true;
          }
          // Pattern sls: shimm value, limm address.
          // Build ST_sls: (ins limm32:$limm, st_offset9:$offset)
          // The limm is adjusted: limm_encoded = addr - shimm.
          Inst.clear();
          Inst.setOpcode(SLSOpc);
          Inst.addOperand(MCOperand::createImm(Addr - Val)); // adjusted limm
          Inst.addOperand(MCOperand::createImm(Val));        // offset (= val)
          return true;
        }
      }
    }
  }

  return false;
}

/// Set the suffix operands (f, q, n) in the MCInst. These are real instruction
/// operands that are not in the AsmString, so the matcher's ConvertToMCInst
/// fills them with default 0 values. We overwrite them with the parsed suffix
/// values from the mnemonic.
///
/// Suffix operands are always the LAST N operands in the MCInst:
///   ALU rrr/rrl/rlr, SOP rr/rl: last 2 = f, q
///   ALU rrs/rsr/rss, SOP rs:    last 1 = f
///   Branch B/BL/LP:              last 2 = q, n
///   Jump J_r/J_l/JL_r/JL_l:     last 3 = f, q, n
///   Flag_r/Flag_l:               last 1 = q
///   Everything else:             none
static void setSuffixOperands(MCInst &Inst, const MCInstrInfo &MCII,
                              int FlagBit, int CondCode, int DelaySlot) {
  unsigned Opc = Inst.getOpcode();
  unsigned N = Inst.getNumOperands();

  switch (Opc) {
  // Jump instructions: last 3 operands are f, q, n
  // Only overwrite if user explicitly specified the suffix.
  case ARC4::J_r: case ARC4::J_l:
  case ARC4::JL_r: case ARC4::JL_l:
    if (N >= 3) {
      if (FlagBit != 0) Inst.getOperand(N - 3).setImm(FlagBit);
      if (CondCode != 0) Inst.getOperand(N - 2).setImm(CondCode);
      if (DelaySlot != 0) Inst.getOperand(N - 1).setImm(DelaySlot);
    }
    return;

  // Branch instructions: last 2 operands are q, n
  case ARC4::B: case ARC4::BL: case ARC4::LP_insn:
    if (N >= 2) {
      if (CondCode != 0) Inst.getOperand(N - 2).setImm(CondCode);
      if (DelaySlot != 0) Inst.getOperand(N - 1).setImm(DelaySlot);
    }
    return;

  // Flag_r/Flag_l: last operand is q
  case ARC4::FLAG_r: case ARC4::FLAG_l:
    if (N >= 1 && CondCode != 0)
      Inst.getOperand(N - 1).setImm(CondCode);
    return;

  // FLAG_s has no suffix operands
  case ARC4::FLAG_s:
    return;

  default:
    break;
  }

  // Instructions with NO suffix operands: loads, stores, NOP, BRK, SLEEP, SWI.
  // Must check these BEFORE the shimm form check, since loads/stores also use
  // shimm but don't have an f operand.
  switch (Opc) {
  case ARC4::NOP: case ARC4::BRK: case ARC4::SLEEP: case ARC4::SWI:
  // All load instructions (opcode 0 and 1)
  case ARC4::LD_rr:  case ARC4::LD_rl:  case ARC4::LD_lr:
  case ARC4::LD_rs:  case ARC4::LD_ss:  case ARC4::LD_l:
  case ARC4::LDB_rr: case ARC4::LDB_rl: case ARC4::LDB_lr:
  case ARC4::LDB_rs: case ARC4::LDB_ss: case ARC4::LDB_l:
  case ARC4::LDW_rr: case ARC4::LDW_rl: case ARC4::LDW_lr:
  case ARC4::LDW_rs: case ARC4::LDW_ss: case ARC4::LDW_l:
  // All store instructions (opcode 2)
  case ARC4::ST_rrs:  case ARC4::ST_srs:  case ARC4::ST_rss:
  case ARC4::ST_sss:  case ARC4::ST_rls:  case ARC4::ST_lrs:
  case ARC4::ST_lls:  case ARC4::ST_sls:
  case ARC4::STB_rrs: case ARC4::STB_srs: case ARC4::STB_rss:
  case ARC4::STB_sss: case ARC4::STB_rls: case ARC4::STB_lrs:
  case ARC4::STB_lls: case ARC4::STB_sls:
  case ARC4::STW_rrs: case ARC4::STW_srs: case ARC4::STW_rss:
  case ARC4::STW_sss: case ARC4::STW_rls: case ARC4::STW_lrs:
  case ARC4::STW_lls: case ARC4::STW_sls:
    return;  // No suffix operands
  default:
    break;
  }

  // ALU/SOP instructions: check if shimm or non-shimm form.
  // Only overwrite suffix operands if the user explicitly specified the suffix.
  // Aliases (e.g., rlc => adc.f, cmp => sub.f 0) may have pre-set suffix
  // values that should not be overwritten with defaults.
  if (isShimmForm(Opc)) {
    // Shimm ALU/SOP forms: last operand is f (no q, bits overlap with shimm)
    if (N >= 1 && FlagBit != 0)
      Inst.getOperand(N - 1).setImm(FlagBit);
  } else if (N >= 2) {
    // Non-shimm ALU/SOP form: last 2 operands are f, q
    if (FlagBit != 0)
      Inst.getOperand(N - 2).setImm(FlagBit);
    if (CondCode != 0)
      Inst.getOperand(N - 1).setImm(CondCode);
  }
}

bool ARC4AsmParser::matchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                                            OperandVector &Operands,
                                            MCStreamer &Out,
                                            uint64_t &ErrorInfo,
                                            bool MatchingInlineAsm) {
  MCInst Inst;

  // Try to match store shimm forms (srs, sss) before the normal matcher.
  // These forms have tied operands in their AsmString that the AsmMatcher
  // cannot handle, so we detect and build them manually.
  StringRef Mnemonic =
      static_cast<ARC4Operand *>(Operands[0].get())->getToken();
  bool StoreShimmMatched = tryMatchStoreSRS(Mnemonic, Operands, Inst);

  if (StoreShimmMatched) {
    // Store shimm forms have no suffix operands (no f/q/n fields).
    // Validate: condition codes and delay slots are not allowed.
    if (ParsedCondCode != 0 && isShimmForm(Inst.getOpcode()))
      return Error(IDLoc,
                   "condition code not allowed with short immediate operand");
    if (ParsedDelaySlot != 0)
      return Error(IDLoc,
                   "delay slot modifier not allowed on this instruction");
    Out.emitInstruction(Inst, getSTI());
    return false;
  }

  switch (MatchInstructionImpl(Operands, Inst, ErrorInfo, MatchingInlineAsm)) {
  case Match_Success: {
    // Validate suffix compatibility before filling operands.
    if (ParsedCondCode != 0 && isShimmForm(Inst.getOpcode()))
      return Error(IDLoc,
                   "condition code not allowed with short immediate operand");
    if (ParsedDelaySlot != 0 && !isBranchOrJump(Inst.getOpcode()))
      return Error(IDLoc,
                   "delay slot modifier not allowed on this instruction");

    // Fill in the suffix operands (f, q, n) that aren't in the AsmString.
    // The matcher created an MCInst with only visible operands; we append
    // the suffix values so getBinaryCodeForInstr() can encode them.
    setSuffixOperands(Inst, MII, ParsedFlagBit, ParsedCondCode,
                       ParsedDelaySlot);

    // Default delay slot for JL_l: .jd (2) when no explicit suffix was parsed.
    if (Inst.getOpcode() == ARC4::JL_l && !ParsedHasExplicitSuffix) {
      // The delay slot is the last operand.
      unsigned NIdx = Inst.getNumOperands() - 1;
      Inst.getOperand(NIdx).setImm(2);  // .jd = 2
    }

    Out.emitInstruction(Inst, getSTI());
    return false;
  }
  case Match_MissingFeature:
    return Error(IDLoc,
                 "instruction requires a CPU feature not currently enabled");
  case Match_MnemonicFail:
    return Error(IDLoc, "unrecognized instruction mnemonic");
  case Match_InvalidOperand: {
    SMLoc ErrorLoc = IDLoc;
    if (ErrorInfo != ~0ULL && ErrorInfo < Operands.size())
      ErrorLoc = Operands[ErrorInfo]->getStartLoc();
    return Error(ErrorLoc, "invalid operand for instruction");
  }
  }
  llvm_unreachable("Unexpected match result");
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeARC4AsmParser() {
  RegisterMCAsmParser<ARC4AsmParser> X(getTheARC4Target());
}

#define GET_REGISTER_MATCHER
#define GET_MATCHER_IMPLEMENTATION
#include "ARC4GenAsmMatcher.inc"
