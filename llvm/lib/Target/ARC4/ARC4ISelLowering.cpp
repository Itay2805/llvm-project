//===-- ARC4ISelLowering.cpp - ARC4 DAG Lowering Implementation -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ARC4ISelLowering.h"
#include "ARC4Subtarget.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"

using namespace llvm;

#include "ARC4GenCallingConv.inc"

ARC4TargetLowering::ARC4TargetLowering(const TargetMachine &TM,
                                       const ARC4Subtarget &STI)
    : TargetLowering(TM, STI), Subtarget(STI) {
  addRegisterClass(MVT::i32, &ARC4::GRRegClass);
  computeRegisterProperties(STI.getRegisterInfo());

  setStackPointerRegisterToSaveRestore(ARC4::R28);

  // No hardware multiply/divide
  setOperationAction(ISD::MUL, MVT::i32, LibCall);
  setOperationAction(ISD::MULHS, MVT::i32, Expand);
  setOperationAction(ISD::MULHU, MVT::i32, Expand);
  setOperationAction(ISD::UMUL_LOHI, MVT::i32, Expand);
  setOperationAction(ISD::SMUL_LOHI, MVT::i32, Expand);
  setOperationAction(ISD::SDIV, MVT::i32, LibCall);
  setOperationAction(ISD::UDIV, MVT::i32, LibCall);
  setOperationAction(ISD::SREM, MVT::i32, LibCall);
  setOperationAction(ISD::UREM, MVT::i32, LibCall);

  // Branch/select handling
  // BR_CC: custom-lower to CMP + conditional branch
  setOperationAction(ISD::BR_CC, MVT::i32, Custom);
  // SELECT_CC: custom-lower to CMP + conditional select sequence
  setOperationAction(ISD::SELECT_CC, MVT::i32, Custom);
  // SELECT and SETCC: keep legal (tablegen patterns or custom)
  // SETCC produces a 0/1 result via compare + conditional logic
  setOperationAction(ISD::SETCC, MVT::i32, Expand);
  setOperationAction(ISD::SELECT, MVT::i32, Custom);
  setOperationAction(ISD::BRCOND, MVT::Other, Expand);

  // Global addresses
  setOperationAction(ISD::GlobalAddress, MVT::i32, Custom);

  // Shifts: ARC4 has no barrel shifter (only shift-by-1 ASR/LSR).
  // Custom lowering: constant shifts → repeated shift-by-1 in DAG,
  // variable shifts → library calls (__ashlsi3, __lshrsi3, __ashrsi3).
  setOperationAction(ISD::SHL, MVT::i32, Custom);
  setOperationAction(ISD::SRL, MVT::i32, Custom);
  setOperationAction(ISD::SRA, MVT::i32, Custom);

  // Sign/zero extend loads
  setLoadExtAction(ISD::SEXTLOAD, MVT::i32, MVT::i8, Expand);
  setLoadExtAction(ISD::SEXTLOAD, MVT::i32, MVT::i16, Expand);
  setLoadExtAction(ISD::ZEXTLOAD, MVT::i32, MVT::i8, Expand);
  setLoadExtAction(ISD::ZEXTLOAD, MVT::i32, MVT::i16, Expand);

  // No 64-bit operations
  setOperationAction(ISD::SHL_PARTS, MVT::i32, Expand);
  setOperationAction(ISD::SRA_PARTS, MVT::i32, Expand);
  setOperationAction(ISD::SRL_PARTS, MVT::i32, Expand);

  // Expand some operations
  setOperationAction(ISD::ROTL, MVT::i32, Expand);
  setOperationAction(ISD::ROTR, MVT::i32, Expand);
  setOperationAction(ISD::CTLZ, MVT::i32, Expand);
  setOperationAction(ISD::CTTZ, MVT::i32, Expand);
  setOperationAction(ISD::CTPOP, MVT::i32, Expand);
  setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i1, Expand);
  setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i8, Expand);
  setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i16, Expand);

  setMinFunctionAlignment(Align(4));
}

SDValue ARC4TargetLowering::LowerOperation(SDValue Op,
                                           SelectionDAG &DAG) const {
  switch (Op.getOpcode()) {
  case ISD::BR_CC:
    return LowerBR_CC(Op, DAG);
  case ISD::SELECT_CC:
    return LowerSELECT_CC(Op, DAG);
  case ISD::SELECT:
    return LowerSELECT(Op, DAG);
  case ISD::SHL:
  case ISD::SRL:
  case ISD::SRA:
    return LowerShift(Op, DAG);
  case ISD::GlobalAddress:
    return LowerGlobalAddress(Op, DAG);
  default:
    llvm_unreachable("unimplemented operation");
  }
}

const char *ARC4TargetLowering::getTargetNodeName(unsigned Opcode) const {
  switch (Opcode) {
  case ARC4ISD::CALL:     return "ARC4ISD::CALL";
  case ARC4ISD::RET_GLUE: return "ARC4ISD::RET_GLUE";
  case ARC4ISD::CMP:      return "ARC4ISD::CMP";
  case ARC4ISD::BR_CC:      return "ARC4ISD::BR_CC";
  case ARC4ISD::SELECT_CC:  return "ARC4ISD::SELECT_CC";
  case ARC4ISD::ASR1:       return "ARC4ISD::ASR1";
  case ARC4ISD::LSR1:       return "ARC4ISD::LSR1";
  default:                  return nullptr;
  }
}

Register ARC4TargetLowering::getRegisterByName(const char *RegName, LLT VT,
                                               const MachineFunction &MF) const {
  Register Reg = StringSwitch<unsigned>(RegName)
      .Case("sp", ARC4::R28)
      .Case("r28", ARC4::R28)
      .Default(0);
  if (Reg)
    return Reg;
  report_fatal_error(Twine("Invalid register name \"") + RegName + "\".");
}

//===----------------------------------------------------------------------===//
// Calling Convention Implementation
//===----------------------------------------------------------------------===//

SDValue ARC4TargetLowering::LowerFormalArguments(
    SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
    const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &DL,
    SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const {
  MachineFunction &MF = DAG.getMachineFunction();
  MachineRegisterInfo &RegInfo = MF.getRegInfo();

  SmallVector<CCValAssign, 16> ArgLocs;
  CCState CCInfo(CallConv, IsVarArg, MF, ArgLocs, *DAG.getContext());
  CCInfo.AnalyzeFormalArguments(Ins, CC_ARC4);

  for (auto &VA : ArgLocs) {
    if (VA.isRegLoc()) {
      Register VReg = RegInfo.createVirtualRegister(&ARC4::GRRegClass);
      RegInfo.addLiveIn(VA.getLocReg(), VReg);
      SDValue ArgValue = DAG.getCopyFromReg(Chain, DL, VReg, MVT::i32);
      InVals.push_back(ArgValue);
    } else {
      // Stack argument
      int FI = MF.getFrameInfo().CreateFixedObject(4, VA.getLocMemOffset(), true);
      SDValue FIN = DAG.getFrameIndex(FI, MVT::i32);
      InVals.push_back(
          DAG.getLoad(MVT::i32, DL, Chain, FIN, MachinePointerInfo()));
    }
  }
  return Chain;
}

SDValue ARC4TargetLowering::LowerReturn(
    SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
    const SmallVectorImpl<ISD::OutputArg> &Outs,
    const SmallVectorImpl<SDValue> &OutVals, const SDLoc &DL,
    SelectionDAG &DAG) const {
  SmallVector<CCValAssign, 16> RVLocs;
  CCState CCInfo(CallConv, IsVarArg, DAG.getMachineFunction(), RVLocs,
                 *DAG.getContext());
  CCInfo.AnalyzeReturn(Outs, RetCC_ARC4);

  SDValue Glue;
  SmallVector<SDValue, 4> RetOps;
  RetOps.push_back(Chain);

  for (unsigned i = 0; i < RVLocs.size(); ++i) {
    CCValAssign &VA = RVLocs[i];
    Chain = DAG.getCopyToReg(Chain, DL, VA.getLocReg(), OutVals[i], Glue);
    Glue = Chain.getValue(1);
    RetOps.push_back(DAG.getRegister(VA.getLocReg(), VA.getLocVT()));
  }

  RetOps[0] = Chain;
  if (Glue.getNode())
    RetOps.push_back(Glue);

  return DAG.getNode(ARC4ISD::RET_GLUE, DL, MVT::Other, RetOps);
}

SDValue ARC4TargetLowering::LowerCall(TargetLowering::CallLoweringInfo &CLI,
                                      SmallVectorImpl<SDValue> &InVals) const {
  SelectionDAG &DAG = CLI.DAG;
  SDLoc DL = CLI.DL;
  SmallVectorImpl<ISD::OutputArg> &Outs = CLI.Outs;
  SmallVectorImpl<SDValue> &OutVals = CLI.OutVals;
  SmallVectorImpl<ISD::InputArg> &Ins = CLI.Ins;
  SDValue Chain = CLI.Chain;
  SDValue Callee = CLI.Callee;
  CallingConv::ID CallConv = CLI.CallConv;
  bool IsVarArg = CLI.IsVarArg;
  MachineFunction &MF = DAG.getMachineFunction();

  // Disable tail calls for MVP - we don't handle them yet
  CLI.IsTailCall = false;

  SmallVector<CCValAssign, 16> ArgLocs;
  CCState CCInfo(CallConv, IsVarArg, MF, ArgLocs, *DAG.getContext());
  CCInfo.AnalyzeCallOperands(Outs, CC_ARC4);

  unsigned NumBytes = CCInfo.getStackSize();
  Chain = DAG.getCALLSEQ_START(Chain, NumBytes, 0, DL);

  SmallVector<std::pair<Register, SDValue>, 8> RegsToPass;
  SmallVector<SDValue, 8> MemOpChains;

  for (unsigned i = 0; i < ArgLocs.size(); ++i) {
    CCValAssign &VA = ArgLocs[i];
    SDValue Arg = OutVals[i];
    if (VA.isRegLoc()) {
      RegsToPass.push_back({VA.getLocReg(), Arg});
    } else {
      // Stack argument
      SDValue SP = DAG.getCopyFromReg(Chain, DL, ARC4::R28, MVT::i32);
      SDValue Addr =
          DAG.getNode(ISD::ADD, DL, MVT::i32, SP,
                      DAG.getConstant(VA.getLocMemOffset(), DL, MVT::i32));
      MemOpChains.push_back(
          DAG.getStore(Chain, DL, Arg, Addr, MachinePointerInfo()));
    }
  }

  if (!MemOpChains.empty())
    Chain = DAG.getNode(ISD::TokenFactor, DL, MVT::Other, MemOpChains);

  SDValue Glue;
  for (auto &Reg : RegsToPass) {
    Chain = DAG.getCopyToReg(Chain, DL, Reg.first, Reg.second, Glue);
    Glue = Chain.getValue(1);
  }

  SmallVector<SDValue, 8> Ops;
  Ops.push_back(Chain);

  // Handle callee address
  if (GlobalAddressSDNode *G = dyn_cast<GlobalAddressSDNode>(Callee))
    Callee = DAG.getTargetGlobalAddress(G->getGlobal(), DL, MVT::i32);
  else if (ExternalSymbolSDNode *E = dyn_cast<ExternalSymbolSDNode>(Callee))
    Callee = DAG.getTargetExternalSymbol(E->getSymbol(), MVT::i32);

  Ops.push_back(Callee);

  for (auto &Reg : RegsToPass)
    Ops.push_back(DAG.getRegister(Reg.first, Reg.second.getValueType()));

  const TargetRegisterInfo *TRI = Subtarget.getRegisterInfo();
  const uint32_t *Mask = TRI->getCallPreservedMask(MF, CallConv);
  Ops.push_back(DAG.getRegisterMask(Mask));

  if (Glue.getNode())
    Ops.push_back(Glue);

  SDVTList NodeTys = DAG.getVTList(MVT::Other, MVT::Glue);
  Chain = DAG.getNode(ARC4ISD::CALL, DL, NodeTys, Ops);
  Glue = Chain.getValue(1);

  Chain = DAG.getCALLSEQ_END(Chain, NumBytes, 0, Glue, DL);
  Glue = Chain.getValue(1);

  // Copy return value
  SmallVector<CCValAssign, 16> RVLocs;
  CCState RVInfo(CallConv, IsVarArg, MF, RVLocs, *DAG.getContext());
  RVInfo.AnalyzeCallResult(Ins, RetCC_ARC4);

  for (auto &VA : RVLocs) {
    Chain = DAG.getCopyFromReg(Chain, DL, VA.getLocReg(), VA.getValVT(), Glue)
                .getValue(1);
    InVals.push_back(Chain.getValue(0));
    Glue = Chain.getValue(2);
  }

  return Chain;
}

//===----------------------------------------------------------------------===//
// Custom lowering
//===----------------------------------------------------------------------===//

// Map ISD condition codes to ARC4 Q field values
static unsigned ARC4CC(ISD::CondCode CC) {
  switch (CC) {
  case ISD::SETEQ:  return 1;  // EQ
  case ISD::SETNE:  return 2;  // NE
  case ISD::SETGT:  return 9;  // GT (signed)
  case ISD::SETGE:  return 10; // GE (signed)
  case ISD::SETLT:  return 11; // LT (signed)
  case ISD::SETLE:  return 12; // LE (signed)
  case ISD::SETUGT: return 13; // HI (unsigned)
  case ISD::SETUGE: return 6;  // HS/CC (unsigned >=)
  case ISD::SETULT: return 5;  // LO/CS (unsigned <)
  case ISD::SETULE: return 14; // LS (unsigned <=)
  default: llvm_unreachable("unsupported condition code");
  }
}

SDValue ARC4TargetLowering::LowerBR_CC(SDValue Op, SelectionDAG &DAG) const {
  SDLoc DL(Op);
  SDValue Chain = Op.getOperand(0);
  ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(1))->get();
  SDValue LHS = Op.getOperand(2);
  SDValue RHS = Op.getOperand(3);
  SDValue Dest = Op.getOperand(4);

  SDValue Cmp = DAG.getNode(ARC4ISD::CMP, DL, MVT::Glue, LHS, RHS);

  return DAG.getNode(ARC4ISD::BR_CC, DL, MVT::Other, Chain, Dest,
                     DAG.getConstant(ARC4CC(CC), DL, MVT::i32), Cmp);
}

SDValue ARC4TargetLowering::LowerSELECT_CC(SDValue Op,
                                            SelectionDAG &DAG) const {
  SDLoc DL(Op);
  SDValue LHS = Op.getOperand(0);
  SDValue RHS = Op.getOperand(1);
  SDValue TrueVal = Op.getOperand(2);
  SDValue FalseVal = Op.getOperand(3);
  ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(4))->get();

  // Lower SELECT_CC to: cmp LHS, RHS → BR_CC to select true/false
  // Emit as: CMP + SELECT(cc, true, false)
  SDValue Cmp = DAG.getNode(ARC4ISD::CMP, DL, MVT::Glue, LHS, RHS);
  SDValue CCVal = DAG.getConstant(ARC4CC(CC), DL, MVT::i32);
  return DAG.getNode(ARC4ISD::SELECT_CC, DL, Op.getValueType(),
                     TrueVal, FalseVal, CCVal, Cmp);
}

SDValue ARC4TargetLowering::LowerSELECT(SDValue Op,
                                         SelectionDAG &DAG) const {
  // SELECT(cond, true, false) → SELECT_CC(cond, 0, true, false, NE)
  SDLoc DL(Op);
  SDValue Cond = Op.getOperand(0);
  SDValue TrueVal = Op.getOperand(1);
  SDValue FalseVal = Op.getOperand(2);

  SDValue Zero = DAG.getConstant(0, DL, MVT::i32);
  SDValue Cmp = DAG.getNode(ARC4ISD::CMP, DL, MVT::Glue, Cond, Zero);
  SDValue CCVal = DAG.getConstant(ARC4CC(ISD::SETNE), DL, MVT::i32);
  return DAG.getNode(ARC4ISD::SELECT_CC, DL, Op.getValueType(),
                     TrueVal, FalseVal, CCVal, Cmp);
}

SDValue ARC4TargetLowering::LowerShift(SDValue Op, SelectionDAG &DAG) const {
  SDLoc DL(Op);
  SDValue Src = Op.getOperand(0);
  SDValue Amt = Op.getOperand(1);
  unsigned Opc = Op.getOpcode();

  // Constant shift: expand to repeated shift-by-1 operations.
  if (auto *AmtC = dyn_cast<ConstantSDNode>(Amt)) {
    unsigned N = AmtC->getZExtValue() & 31;
    if (N == 0)
      return Src;

    SDValue Result = Src;
    for (unsigned I = 0; I < N; I++) {
      if (Opc == ISD::SHL) {
        // SHL by 1 = ADD a, a
        Result = DAG.getNode(ISD::ADD, DL, MVT::i32, Result, Result);
      } else if (Opc == ISD::SRA) {
        // ASR by 1 — use target-specific node
        Result = DAG.getNode(ARC4ISD::ASR1, DL, MVT::i32, Result);
      } else {
        // LSR by 1 — use target-specific node
        Result = DAG.getNode(ARC4ISD::LSR1, DL, MVT::i32, Result);
      }
    }
    return Result;
  }

  // Variable shift: emit a library call.
  RTLIB::Libcall LC;
  switch (Opc) {
  case ISD::SHL: LC = RTLIB::SHL_I32; break;
  case ISD::SRL: LC = RTLIB::SRL_I32; break;
  case ISD::SRA: LC = RTLIB::SRA_I32; break;
  default: llvm_unreachable("unexpected shift");
  }
  MakeLibCallOptions CallOptions;
  SDValue Args[] = {Src, Amt};
  auto Call = makeLibCall(DAG, LC, MVT::i32, Args, CallOptions, DL);
  return Call.first;
}

SDValue ARC4TargetLowering::LowerGlobalAddress(SDValue Op,
                                               SelectionDAG &DAG) const {
  SDLoc DL(Op);
  const GlobalValue *GV = cast<GlobalAddressSDNode>(Op)->getGlobal();
  int64_t Offset = cast<GlobalAddressSDNode>(Op)->getOffset();
  // For MVP: just return the target global address directly.
  // The instruction selector will materialize it as a limm load.
  return DAG.getTargetGlobalAddress(GV, DL, MVT::i32, Offset);
}

//===----------------------------------------------------------------------===//
// Custom inserter for SELECT_CC pseudo
//===----------------------------------------------------------------------===//

MachineBasicBlock *ARC4TargetLowering::EmitInstrWithCustomInserter(
    MachineInstr &MI, MachineBasicBlock *MBB) const {
  assert(MI.getOpcode() == ARC4::CG_SELECT_CC && "Unexpected custom inserter");

  const TargetInstrInfo &TII = *Subtarget.getInstrInfo();
  DebugLoc DL = MI.getDebugLoc();

  // CG_SELECT_CC dst, trueVal, falseVal, cc
  Register Dst = MI.getOperand(0).getReg();
  Register TrueVal = MI.getOperand(1).getReg();
  Register FalseVal = MI.getOperand(2).getReg();
  unsigned CC = MI.getOperand(3).getImm();

  // Create the diamond:
  //   MBB:
  //     ...                  (flags already set by CMP before this)
  //     Bcc trueMBB, cc
  //   falseMBB:
  //     ... (fallthrough)
  //   trueMBB:
  //     dst = PHI(trueVal, MBB, falseVal, falseMBB)

  MachineFunction *MF = MBB->getParent();
  const BasicBlock *BB = MBB->getBasicBlock();

  MachineBasicBlock *FalseMBB = MF->CreateMachineBasicBlock(BB);
  MachineBasicBlock *SinkMBB = MF->CreateMachineBasicBlock(BB);

  MachineFunction::iterator It = ++MBB->getIterator();
  MF->insert(It, FalseMBB);
  MF->insert(It, SinkMBB);

  // Transfer rest of MBB to SinkMBB
  SinkMBB->splice(SinkMBB->begin(), MBB,
                  std::next(MachineBasicBlock::iterator(MI)), MBB->end());
  SinkMBB->transferSuccessorsAndUpdatePHIs(MBB);

  // MBB → conditional branch to SinkMBB (true path), fallthrough to FalseMBB
  MBB->addSuccessor(FalseMBB);
  MBB->addSuccessor(SinkMBB);

  // Emit conditional branch: Bcc SinkMBB
  BuildMI(MBB, DL, TII.get(ARC4::CG_BRcc))
      .addMBB(SinkMBB)
      .addImm(CC);

  // FalseMBB falls through to SinkMBB
  FalseMBB->addSuccessor(SinkMBB);

  // SinkMBB: PHI to select the result
  BuildMI(*SinkMBB, SinkMBB->begin(), DL, TII.get(ARC4::PHI), Dst)
      .addReg(TrueVal)
      .addMBB(MBB)
      .addReg(FalseVal)
      .addMBB(FalseMBB);

  MI.eraseFromParent();
  return SinkMBB;
}
