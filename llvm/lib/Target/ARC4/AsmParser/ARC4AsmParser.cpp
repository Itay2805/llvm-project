//===-- ARC4AsmParser.cpp - Parse ARC4 assembly to MCInst -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/ARC4MCTargetDesc.h"
#include "../TargetInfo/ARC4TargetInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCParser/AsmLexer.h"
#include "llvm/MC/MCParser/MCParsedAsmOperand.h"
#include "llvm/MC/MCParser/MCTargetAsmParser.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Casting.h"

#define DEBUG_TYPE "arc4-asm-parser"

using namespace llvm;

// ---------------------------------------------------------------------------
// Operand representation
// ---------------------------------------------------------------------------
namespace {

class ARC4Operand : public MCParsedAsmOperand {
  enum KindTy { k_Token, k_Register, k_Immediate } Kind;

  struct ImmOp {
    const MCExpr *Val;
  };
  union {
    StringRef    Tok;
    MCRegister   Reg;
    ImmOp        Imm;
  };
  SMLoc Start, End;

  explicit ARC4Operand(StringRef T, SMLoc S)
      : Kind(k_Token), Tok(T), Start(S), End(S) {}
  ARC4Operand(MCRegister R, SMLoc S, SMLoc E)
      : Kind(k_Register), Reg(R), Start(S), End(E) {}
  ARC4Operand(const MCExpr *V, SMLoc S, SMLoc E)
      : Kind(k_Immediate), Start(S), End(E) { Imm.Val = V; }

public:
  static std::unique_ptr<ARC4Operand> createToken(StringRef T, SMLoc S) {
    return std::unique_ptr<ARC4Operand>(new ARC4Operand(T, S));
  }
  static std::unique_ptr<ARC4Operand> createReg(MCRegister R, SMLoc S,
                                                  SMLoc E) {
    return std::unique_ptr<ARC4Operand>(new ARC4Operand(R, S, E));
  }
  static std::unique_ptr<ARC4Operand> createImm(const MCExpr *V, SMLoc S,
                                                  SMLoc E) {
    return std::unique_ptr<ARC4Operand>(new ARC4Operand(V, S, E));
  }

  bool isToken() const override { return Kind == k_Token; }
  bool isReg()   const override { return Kind == k_Register; }
  bool isImm()   const override { return Kind == k_Immediate; }
  bool isMem()   const override { return false; }

  bool isSimm9() const {
    if (!isImm()) return false;
    if (const MCConstantExpr *CE = dyn_cast<MCConstantExpr>(Imm.Val)) {
      int64_t V = CE->getValue();
      return V >= -256 && V <= 255;
    }
    return false; // expressions (labels) don't fit shimm
  }

  // Matches literal 0 for discard-destination forms (op 0, b, c)
  bool isImm0() const {
    if (!isImm()) return false;
    if (const MCConstantExpr *CE = dyn_cast<MCConstantExpr>(Imm.Val))
      return CE->getValue() == 0;
    return false;
  }

  StringRef getToken() const {
    assert(isToken());
    return Tok;
  }
  MCRegister getReg() const override {
    assert(isReg());
    return Reg;
  }
  const MCExpr *getImm() const {
    assert(isImm());
    return Imm.Val;
  }

  SMLoc getStartLoc() const override { return Start; }
  SMLoc getEndLoc()   const override { return End; }

  void addRegOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && isReg());
    Inst.addOperand(MCOperand::createReg(Reg));
  }

  void addImmOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && isImm());
    if (const MCConstantExpr *CE = dyn_cast<MCConstantExpr>(Imm.Val))
      Inst.addOperand(MCOperand::createImm(CE->getValue()));
    else
      Inst.addOperand(MCOperand::createExpr(Imm.Val));
  }

  void print(raw_ostream &O, const MCAsmInfo &MAI) const override {
    if (isToken())        O << "Tok(" << Tok << ")";
    else if (isReg())     O << "Reg(" << Reg.id() << ")";
    else                  O << "Imm(...)";
  }
};

} // namespace

// ---------------------------------------------------------------------------
// Parser class
// ---------------------------------------------------------------------------
namespace {

class ARC4AsmParser : public MCTargetAsmParser {
  const MCRegisterInfo *MRI;
  MCAsmParser &Parser;

  MCAsmParser &getParser() const { return Parser; }
  AsmLexer    &getLexer()  const { return Parser.getLexer(); }
  SMLoc getLoc() const { return Parser.getTok().getLoc(); }

  /// Auto-generated match helpers (MatchRegisterName, MatchInstructionImpl, …)
#define GET_ASSEMBLER_HEADER
#include "ARC4GenAsmMatcher.inc"

  bool parseRegister(MCRegister &Reg, SMLoc &StartLoc, SMLoc &EndLoc) override;
  ParseStatus tryParseRegister(MCRegister &Reg, SMLoc &StartLoc,
                               SMLoc &EndLoc) override;

  bool parseInstruction(ParseInstructionInfo &Info, StringRef Name,
                        SMLoc NameLoc, OperandVector &Operands) override;

  bool matchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                               OperandVector &Operands, MCStreamer &Out,
                               uint64_t &ErrorInfo,
                               bool MatchingInlineAsm) override;

  ParseStatus parseDirective(AsmToken DirectiveID) override {
    return ParseStatus::NoMatch;
  }

  /// Parse a single non-bracket operand (register or immediate/expression).
  bool parseOperand(OperandVector &Operands);

public:
  ARC4AsmParser(const MCSubtargetInfo &STI, MCAsmParser &P,
                const MCInstrInfo &MII, const MCTargetOptions &Opts)
      : MCTargetAsmParser(Opts, STI, MII), Parser(P) {
    MCAsmParserExtension::Initialize(P);
    MRI = getContext().getRegisterInfo();
    setAvailableFeatures(ComputeAvailableFeatures(STI.getFeatureBits()));
  }
};

} // namespace

// ---------------------------------------------------------------------------
// Auto-generated register + matcher tables
// ---------------------------------------------------------------------------

#define GET_REGISTER_MATCHER
#define GET_MATCHER_IMPLEMENTATION
#include "ARC4GenAsmMatcher.inc"

// ---------------------------------------------------------------------------
// Register parsing
// ---------------------------------------------------------------------------

ParseStatus ARC4AsmParser::tryParseRegister(MCRegister &Reg, SMLoc &StartLoc,
                                             SMLoc &EndLoc) {
  if (getLexer().getKind() != AsmToken::Identifier)
    return ParseStatus::NoMatch;

  std::string NameLower = getLexer().getTok().getIdentifier().lower();
  StringRef Name(NameLower);
  Reg = MatchRegisterName(Name);
  if (!Reg)
    Reg = MatchRegisterAltName(Name);
  if (!Reg)
    return ParseStatus::NoMatch;

  StartLoc = getLexer().getTok().getLoc();
  EndLoc   = getLexer().getTok().getEndLoc();
  getLexer().Lex();
  return ParseStatus::Success;
}

bool ARC4AsmParser::parseRegister(MCRegister &Reg, SMLoc &StartLoc,
                                   SMLoc &EndLoc) {
  ParseStatus S = tryParseRegister(Reg, StartLoc, EndLoc);
  if (S.isSuccess()) return false;
  return Error(StartLoc, "invalid register name");
}

// ---------------------------------------------------------------------------
// Operand parsing
// ---------------------------------------------------------------------------

bool ARC4AsmParser::parseOperand(OperandVector &Operands) {
  // Try register first
  MCRegister Reg;
  SMLoc S = getLoc(), E;
  if (tryParseRegister(Reg, S, E).isSuccess()) {
    Operands.push_back(ARC4Operand::createReg(Reg, S, E));
    return false;
  }

  // Otherwise parse as expression (immediate or label)
  S = getLoc();
  const MCExpr *Expr;
  if (getParser().parseExpression(Expr))
    return Error(S, "expected register or immediate");

  E = getLexer().getLoc();
  Operands.push_back(ARC4Operand::createImm(Expr, S, E));
  return false;
}

bool ARC4AsmParser::parseInstruction(ParseInstructionInfo &Info, StringRef Name,
                                      SMLoc NameLoc, OperandVector &Operands) {
  // Mnemonic token
  Operands.push_back(ARC4Operand::createToken(Name, NameLoc));

  // No operands
  if (getLexer().is(AsmToken::EndOfStatement)) {
    getLexer().Lex();
    return false;
  }

  // Parse remaining operands, handling brackets as standalone tokens.
  while (true) {
    // Skip commas (they're SeparatorCharacters in the match table)
    if (getLexer().is(AsmToken::Comma)) {
      getLexer().Lex();
      continue;
    }

    if (getLexer().is(AsmToken::EndOfStatement))
      break;

    if (getLexer().is(AsmToken::LBrac)) {
      // '[' is a TokenizingCharacter — push it as a token operand
      SMLoc BracLoc = getLoc();
      Operands.push_back(ARC4Operand::createToken("[", BracLoc));
      getLexer().Lex(); // eat '['

      // Parse inner operands until ']'
      while (!getLexer().is(AsmToken::RBrac) &&
             !getLexer().is(AsmToken::EndOfStatement)) {
        if (getLexer().is(AsmToken::Comma)) {
          getLexer().Lex();
          continue;
        }
        if (parseOperand(Operands))
          return true;
      }

      if (getLexer().isNot(AsmToken::RBrac))
        return Error(getLoc(), "expected ']'");

      SMLoc RBracLoc = getLoc();
      Operands.push_back(ARC4Operand::createToken("]", RBracLoc));
      getLexer().Lex(); // eat ']'
      continue;
    }

    if (parseOperand(Operands))
      return true;
  }

  if (getLexer().is(AsmToken::EndOfStatement))
    getLexer().Lex();

  return false;
}

// ---------------------------------------------------------------------------
// Match and emit
// ---------------------------------------------------------------------------

bool ARC4AsmParser::matchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                                             OperandVector &Operands,
                                             MCStreamer &Out,
                                             uint64_t &ErrorInfo,
                                             bool MatchingInlineAsm) {
  MCInst Inst;
  unsigned Result =
      MatchInstructionImpl(Operands, Inst, ErrorInfo, MatchingInlineAsm);

  switch (Result) {
  case Match_Success: {
    // Add default zero immediates for any trailing encoding-only operands
    // (F, NN, Q, SetFlags) that are not in the AsmString but expected by
    // the MCCodeEmitter.
    const MCInstrDesc &Desc = MII.get(Inst.getOpcode());
    unsigned Expected = Desc.getNumOperands();
    while (Inst.getNumOperands() < Expected)
      Inst.addOperand(MCOperand::createImm(0));

    Inst.setLoc(IDLoc);
    Out.emitInstruction(Inst, *STI);
    return false;
  }
  case Match_MnemonicFail:
    return Error(IDLoc, "invalid instruction mnemonic");
  case Match_InvalidOperand: {
    SMLoc ErrLoc = IDLoc;
    if (ErrorInfo != ~0ULL && ErrorInfo < Operands.size())
      ErrLoc = Operands[ErrorInfo]->getStartLoc();
    return Error(ErrLoc, "invalid operand for instruction");
  }
  case Match_MissingFeature:
    return Error(IDLoc, "instruction requires an unsupported feature");
  default:
    return Error(IDLoc, "unrecognized instruction");
  }
}

// ---------------------------------------------------------------------------
// Registration
// ---------------------------------------------------------------------------

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void LLVMInitializeARC4AsmParser() {
  RegisterMCAsmParser<ARC4AsmParser> X(getTheARC4Target());
}
