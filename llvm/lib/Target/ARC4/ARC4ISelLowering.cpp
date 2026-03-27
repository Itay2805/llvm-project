//===-- ARC4ISelLowering.cpp - ARC4 DAG Lowering Implementation -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ARC4ISelLowering.h"
#include "ARC4MachineFunctionInfo.h"
#include "ARC4Subtarget.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/IR/Function.h"

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
  setOperationAction(ISD::SDIV, MVT::i32, LibCall);
  setOperationAction(ISD::UDIV, MVT::i32, LibCall);
  setOperationAction(ISD::SREM, MVT::i32, LibCall);
  setOperationAction(ISD::UREM, MVT::i32, LibCall);

  // Branch/select handling
  setOperationAction(ISD::BR_CC, MVT::i32, Custom);
  setOperationAction(ISD::SELECT_CC, MVT::i32, Expand);
  setOperationAction(ISD::SELECT, MVT::i32, Expand);
  setOperationAction(ISD::SETCC, MVT::i32, Expand);
  setOperationAction(ISD::BRCOND, MVT::Other, Expand);

  // Global addresses
  setOperationAction(ISD::GlobalAddress, MVT::i32, Custom);

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
  case ARC4ISD::BR_CC:    return "ARC4ISD::BR_CC";
  default:                return nullptr;
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
  // Let LLVM expand SELECT_CC into branches. We set the action to Expand
  // in the constructor, but this is here as a safety net.
  return SDValue();
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
