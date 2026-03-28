//===-- ARC4AsmParser.cpp - Parse ARC4 assembly to MCInst -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/ARC4MCTargetDesc.h"
#include "TargetInfo/ARC4TargetInfo.h"
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

bool ARC4AsmParser::parseInstruction(ParseInstructionInfo &Info, StringRef Name,
                                     SMLoc NameLoc,
                                     OperandVector &Operands) {
  Operands.push_back(ARC4Operand::createToken(Name, NameLoc));

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
  return false;
}

bool ARC4AsmParser::matchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                                            OperandVector &Operands,
                                            MCStreamer &Out,
                                            uint64_t &ErrorInfo,
                                            bool MatchingInlineAsm) {
  MCInst Inst;
  switch (MatchInstructionImpl(Operands, Inst, ErrorInfo, MatchingInlineAsm)) {
  case Match_Success:
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
