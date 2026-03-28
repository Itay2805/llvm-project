//===- ARC4ISelLowering.h - ARC4 DAG Lowering Interface ---------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ARC4_ARC4ISELLOWERING_H
#define LLVM_LIB_TARGET_ARC4_ARC4ISELLOWERING_H

#include "ARC4.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/TargetLowering.h"

namespace llvm {

class ARC4Subtarget;
class ARC4TargetMachine;

class ARC4TargetLowering : public TargetLowering {
public:
  explicit ARC4TargetLowering(const TargetMachine &TM,
                              const ARC4Subtarget &Subtarget);

  SDValue LowerOperation(SDValue Op, SelectionDAG &DAG) const override;

private:
  const ARC4Subtarget &Subtarget;

  SDValue LowerFormalArguments(SDValue Chain, CallingConv::ID CallConv,
                               bool isVarArg,
                               const SmallVectorImpl<ISD::InputArg> &Ins,
                               const SDLoc &dl, SelectionDAG &DAG,
                               SmallVectorImpl<SDValue> &InVals) const override;

  SDValue LowerReturn(SDValue Chain, CallingConv::ID CallConv, bool isVarArg,
                      const SmallVectorImpl<ISD::OutputArg> &Outs,
                      const SmallVectorImpl<SDValue> &OutVals, const SDLoc &dl,
                      SelectionDAG &DAG) const override;

  SDValue LowerCall(TargetLowering::CallLoweringInfo &CLI,
                    SmallVectorImpl<SDValue> &InVals) const override;

  bool CanLowerReturn(CallingConv::ID CallConv, MachineFunction &MF,
                      bool isVarArg,
                      const SmallVectorImpl<ISD::OutputArg> &ArgsFlags,
                      LLVMContext &Context, const Type *RetTy) const override;
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_ARC4_ARC4ISELLOWERING_H
