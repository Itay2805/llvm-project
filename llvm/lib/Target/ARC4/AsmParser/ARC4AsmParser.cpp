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
  // .f, .q (condition codes), .d/.nd/.jd (delay slots) are recorded as
  // trailing operands on the MCInst for the encoder to apply.
  SmallString<16> MnemonicBuf;

  // Tracked suffix state.
  int CondCode = 0;     // 5-bit condition code (0 = always)
  int FlagBit = 0;      // 1 = .f suffix present
  int DelaySlot = 0;    // 0=nd, 1=d, 2=jd

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
      FlagBit = 1;
      continue;
    }

    // Delay slot suffixes.
    if (Suffix == "nd") { DelaySlot = 0; continue; }
    if (Suffix == "d")  { DelaySlot = 1; continue; }
    if (Suffix == "jd") { DelaySlot = 2; continue; }

    // Sign extend, writeback, cache bypass - strip for now.
    if (Suffix == "x" || Suffix == "a" || Suffix == "di")
      continue;

    // Condition codes.
    int CC = mapConditionCode(Suffix);
    if (CC >= 0) {
      CondCode = CC;
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

  // Fix 5 (rss form): If we have pattern <mnemonic> <reg> <imm> <imm> where
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

  // Append trailing annotation operands for the MCCodeEmitter.
  // Convention: condition code, flag bit, delay slot — in that order.
  // Only append if any are non-default, to avoid bloating simple instructions.
  if (CondCode != 0 || FlagBit != 0 || DelaySlot != 0) {
    // We use ARC4Operand::createImm with MCConstantExpr to carry the values.
    // These will become MCOperand::createImm in the MCInst.
    // Marker: condition code
    Operands.push_back(ARC4Operand::createImm(
        MCConstantExpr::create(CondCode, getContext()), NameLoc, NameLoc));
    // Marker: flag bit
    Operands.push_back(ARC4Operand::createImm(
        MCConstantExpr::create(FlagBit, getContext()), NameLoc, NameLoc));
    // Marker: delay slot
    Operands.push_back(ARC4Operand::createImm(
        MCConstantExpr::create(DelaySlot, getContext()), NameLoc, NameLoc));
  }

  return false;
}

bool ARC4AsmParser::matchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                                            OperandVector &Operands,
                                            MCStreamer &Out,
                                            uint64_t &ErrorInfo,
                                            bool MatchingInlineAsm) {
  // If trailing annotation operands were appended (.f, .q, delay slot),
  // temporarily remove them so the matcher sees only real operands.
  // The annotations are always the last 3 operands when present.
  SmallVector<std::unique_ptr<MCParsedAsmOperand>, 3> Annotations;
  bool HasAnnotations = false;

  // Detect annotations: they are 3 trailing immediate operands added by
  // parseInstruction when any suffix (.f, .q, delay slot) was present.
  // Minimum layout: token + at least 0 real operands + 3 annotations = 4.
  if (Operands.size() >= 4) {
    size_t N = Operands.size();
    auto *A1 = static_cast<ARC4Operand *>(Operands[N - 3].get());
    auto *A2 = static_cast<ARC4Operand *>(Operands[N - 2].get());
    auto *A3 = static_cast<ARC4Operand *>(Operands[N - 1].get());
    // Annotations are immediates whose SMLoc matches the mnemonic (NameLoc).
    // We check that all three are immediates and share the same start location
    // (the mnemonic location set during parseInstruction).
    if (A1->isImm() && A2->isImm() && A3->isImm() &&
        A1->getStartLoc() == A2->getStartLoc() &&
        A2->getStartLoc() == A3->getStartLoc() &&
        A1->getStartLoc() == Operands[0]->getStartLoc()) {
      HasAnnotations = true;
      // Pop in reverse order.
      Annotations.push_back(std::move(Operands[N - 1]));
      Operands.pop_back();
      Annotations.push_back(std::move(Operands[N - 2]));
      Operands.pop_back();
      Annotations.push_back(std::move(Operands[N - 3]));
      Operands.pop_back();
    }
  }

  MCInst Inst;
  switch (MatchInstructionImpl(Operands, Inst, ErrorInfo, MatchingInlineAsm)) {
  case Match_Success:
    // Re-attach annotation operands to the MCInst for the encoder.
    if (HasAnnotations) {
      // Annotations were stored in reverse order: [delay, flag, cc].
      // We want cc, flag, delay in the MCInst.
      auto *CC = static_cast<ARC4Operand *>(Annotations[2].get());
      auto *Flag = static_cast<ARC4Operand *>(Annotations[1].get());
      auto *Delay = static_cast<ARC4Operand *>(Annotations[0].get());
      CC->addImmOperands(Inst, 1);
      Flag->addImmOperands(Inst, 1);
      Delay->addImmOperands(Inst, 1);
    }
    Out.emitInstruction(Inst, getSTI());
    return false;
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
