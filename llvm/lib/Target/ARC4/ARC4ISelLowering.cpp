//===- ARC4ISelLowering.cpp - ARC4 DAG Lowering Impl ------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ARC4ISelLowering.h"
#include "ARC4.h"
#include "ARC4MachineFunctionInfo.h"
#include "ARC4SelectionDAGInfo.h"
#include "ARC4Subtarget.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/ValueTypes.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/Support/Debug.h"

#define DEBUG_TYPE "arc4-lower"

using namespace llvm;

#include "ARC4GenCallingConv.inc"

ARC4TargetLowering::ARC4TargetLowering(const TargetMachine &TM,
                                       const ARC4Subtarget &Subtarget)
    : TargetLowering(TM, Subtarget), Subtarget(Subtarget) {
  // Set up the register classes.
  addRegisterClass(MVT::i32, &ARC4::GPR32RegClass);

  // Compute derived properties from the register classes.
  computeRegisterProperties(Subtarget.getRegisterInfo());

  setStackPointerRegisterToSaveRestore(ARC4::SP);

  setSchedulingPreference(Sched::Source);

  setBooleanContents(ZeroOrOneBooleanContent);

  // Start with all operations as Expand.
  for (unsigned Opc = 0; Opc < ISD::BUILTIN_OP_END; ++Opc)
    setOperationAction(Opc, MVT::i32, Expand);

  // Legal operations that ARC4 has instructions for.
  setOperationAction(ISD::ADD, MVT::i32, Legal);
  setOperationAction(ISD::SUB, MVT::i32, Legal);
  setOperationAction(ISD::AND, MVT::i32, Legal);
  setOperationAction(ISD::OR, MVT::i32, Legal);
  setOperationAction(ISD::XOR, MVT::i32, Legal);
  setOperationAction(ISD::Constant, MVT::i32, Legal);
  setOperationAction(ISD::UNDEF, MVT::i32, Legal);
  setOperationAction(ISD::LOAD, MVT::i32, Legal);
  setOperationAction(ISD::STORE, MVT::i32, Legal);

  // Expand these explicitly.
  setOperationAction(ISD::BR_JT, MVT::Other, Expand);
  setOperationAction(ISD::BRCOND, MVT::Other, Expand);
  setOperationAction(ISD::STACKSAVE, MVT::Other, Expand);
  setOperationAction(ISD::STACKRESTORE, MVT::Other, Expand);

  setMaxAtomicSizeInBitsSupported(0);
}

SDValue ARC4TargetLowering::LowerOperation(SDValue Op,
                                           SelectionDAG &DAG) const {
  llvm_unreachable("unimplemented operation lowering");
  return SDValue();
}

//===----------------------------------------------------------------------===//
//  Formal Arguments
//===----------------------------------------------------------------------===//

SDValue ARC4TargetLowering::LowerFormalArguments(
    SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
    const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &dl,
    SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const {
  MachineFunction &MF = DAG.getMachineFunction();
  MachineRegisterInfo &RegInfo = MF.getRegInfo();

  SmallVector<CCValAssign, 16> ArgLocs;
  CCState CCInfo(CallConv, IsVarArg, MF, ArgLocs, *DAG.getContext());
  CCInfo.AnalyzeFormalArguments(Ins, CC_ARC4);

  for (unsigned i = 0, e = ArgLocs.size(); i != e; ++i) {
    CCValAssign &VA = ArgLocs[i];

    if (VA.isRegLoc()) {
      EVT RegVT = VA.getLocVT();
      unsigned VReg = RegInfo.createVirtualRegister(&ARC4::GPR32RegClass);
      RegInfo.addLiveIn(VA.getLocReg(), VReg);
      SDValue ArgIn = DAG.getCopyFromReg(Chain, dl, VReg, RegVT);
      InVals.push_back(ArgIn);
    } else {
      assert(VA.isMemLoc());
      unsigned ObjSize = VA.getLocVT().getStoreSize();
      int FI = MF.getFrameInfo().CreateFixedObject(ObjSize,
                                                     VA.getLocMemOffset(),
                                                     true);
      SDValue FIN = DAG.getFrameIndex(FI, MVT::i32);
      InVals.push_back(
          DAG.getLoad(VA.getLocVT(), dl, Chain, FIN,
                      MachinePointerInfo::getFixedStack(MF, FI)));
    }
  }

  return Chain;
}

//===----------------------------------------------------------------------===//
//  Return
//===----------------------------------------------------------------------===//

bool ARC4TargetLowering::CanLowerReturn(
    CallingConv::ID CallConv, MachineFunction &MF, bool IsVarArg,
    const SmallVectorImpl<ISD::OutputArg> &Outs, LLVMContext &Context,
    const Type *RetTy) const {
  SmallVector<CCValAssign, 16> RVLocs;
  CCState CCInfo(CallConv, IsVarArg, MF, RVLocs, Context);
  return CCInfo.CheckReturn(Outs, RetCC_ARC4);
}

SDValue
ARC4TargetLowering::LowerReturn(SDValue Chain, CallingConv::ID CallConv,
                                bool IsVarArg,
                                const SmallVectorImpl<ISD::OutputArg> &Outs,
                                const SmallVectorImpl<SDValue> &OutVals,
                                const SDLoc &dl, SelectionDAG &DAG) const {
  SmallVector<CCValAssign, 16> RVLocs;
  CCState CCInfo(CallConv, IsVarArg, DAG.getMachineFunction(), RVLocs,
                 *DAG.getContext());
  CCInfo.AnalyzeReturn(Outs, RetCC_ARC4);

  SDValue Glue;
  SmallVector<SDValue, 4> RetOps(1, Chain);

  // Copy the result values into the output registers.
  for (unsigned i = 0, e = RVLocs.size(); i != e; ++i) {
    CCValAssign &VA = RVLocs[i];
    assert(VA.isRegLoc() && "Can only return in registers!");
    Chain = DAG.getCopyToReg(Chain, dl, VA.getLocReg(), OutVals[i], Glue);
    Glue = Chain.getValue(1);
    RetOps.push_back(DAG.getRegister(VA.getLocReg(), VA.getLocVT()));
  }

  RetOps[0] = Chain; // Update chain.

  if (Glue.getNode())
    RetOps.push_back(Glue);

  return DAG.getNode(ARC4ISD::RET, dl, MVT::Other, RetOps);
}

//===----------------------------------------------------------------------===//
//  Call (stub — not needed for 'return 42')
//===----------------------------------------------------------------------===//

SDValue ARC4TargetLowering::LowerCall(TargetLowering::CallLoweringInfo &CLI,
                                      SmallVectorImpl<SDValue> &InVals) const {
  report_fatal_error("ARC4 call lowering not yet implemented");
}
