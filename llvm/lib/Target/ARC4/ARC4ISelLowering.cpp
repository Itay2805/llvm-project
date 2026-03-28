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
#include "llvm/CodeGen/RuntimeLibcallUtil.h"
#include "llvm/CodeGen/ValueTypes.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/Support/Debug.h"

#define DEBUG_TYPE "arc4-lower"

using namespace llvm;

#include "ARC4GenCallingConv.inc"

//===----------------------------------------------------------------------===//
// ARC4 condition code mapping
//===----------------------------------------------------------------------===//

// ARC4 condition codes (encoded in bits [4:0] of branch/conditional instr):
//   0 = AL (always), 1 = EQ (Z), 2 = NE (!Z), 3 = PL (P, !N),
//   4 = MI (N), 5 = CS/HS (C), 6 = CC/LO (!C), 7 = VS (V), 8 = VC (!V),
//   9 = GT, 10 = GE, 11 = LT, 12 = LE, 13 = HI, 14 = LS
namespace ARC4CC {
enum CondCode {
  AL = 0,
  EQ = 1,
  NE = 2,
  PL = 3,
  MI = 4,
  HS = 5,
  LO = 6,
  VS = 7,
  VC = 8,
  GT = 9,
  GE = 10,
  LT = 11,
  LE = 12,
  HI = 13,
  LS = 14
};
} // namespace ARC4CC

static ARC4CC::CondCode getARC4CC(ISD::CondCode CC) {
  switch (CC) {
  case ISD::SETEQ:
  case ISD::SETUEQ:
    return ARC4CC::EQ;
  case ISD::SETNE:
  case ISD::SETUNE:
    return ARC4CC::NE;
  case ISD::SETGT:
    return ARC4CC::GT;
  case ISD::SETGE:
    return ARC4CC::GE;
  case ISD::SETLT:
    return ARC4CC::LT;
  case ISD::SETLE:
    return ARC4CC::LE;
  case ISD::SETUGT:
    return ARC4CC::HI;
  case ISD::SETUGE:
    return ARC4CC::HS;
  case ISD::SETULT:
    return ARC4CC::LO;
  case ISD::SETULE:
    return ARC4CC::LS;
  default:
    llvm_unreachable("Unhandled ISD condition code");
  }
}

//===----------------------------------------------------------------------===//
// Constructor
//===----------------------------------------------------------------------===//

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

  // Branches and comparisons — custom lower to ARC4-specific nodes.
  setOperationAction(ISD::BR_CC, MVT::i32, Custom);
  setOperationAction(ISD::SELECT_CC, MVT::i32, Custom);
  setOperationAction(ISD::BRCOND, MVT::Other, Expand);

  // Expand these explicitly.
  setOperationAction(ISD::BR_JT, MVT::Other, Expand);
  setOperationAction(ISD::STACKSAVE, MVT::Other, Expand);
  setOperationAction(ISD::STACKRESTORE, MVT::Other, Expand);

  // GlobalAddress — custom lower so legalize doesn't try to expand it.
  setOperationAction(ISD::GlobalAddress, MVT::i32, Custom);

  // Sign-extend-in-register: ARC4 has sexb (byte) and sexw (halfword).
  setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i8, Legal);
  setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i16, Legal);
  setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i1, Expand);

  // Sub-word loads: mark extending loads as Legal — we provide LDB/LDW
  // patterns for zextload, sextload, and extload.
  for (MVT VT : {MVT::i8, MVT::i16}) {
    setLoadExtAction(ISD::EXTLOAD, MVT::i32, VT, Legal);
    setLoadExtAction(ISD::ZEXTLOAD, MVT::i32, VT, Legal);
    setLoadExtAction(ISD::SEXTLOAD, MVT::i32, VT, Legal);
  }

  // Sub-word stores: truncating stores are legal (stb/stw).
  setTruncStoreAction(MVT::i32, MVT::i8, Legal);
  setTruncStoreAction(MVT::i32, MVT::i16, Legal);

  // Shifts: ARC4 base ISA only has single-bit shifts (asl/asr/lsr).
  // Use Custom lowering to emit a loop of single-bit shifts.
  setOperationAction(ISD::SHL, MVT::i32, Custom);
  setOperationAction(ISD::SRA, MVT::i32, Custom);
  setOperationAction(ISD::SRL, MVT::i32, Custom);

  // Multiply: no hardware multiplier in base ARC4. Use libcall (__mulsi3).
  setOperationAction(ISD::MUL, MVT::i32, LibCall);
  setOperationAction(ISD::MULHS, MVT::i32, Expand);
  setOperationAction(ISD::MULHU, MVT::i32, Expand);
  setOperationAction(ISD::SMUL_LOHI, MVT::i32, Expand);
  setOperationAction(ISD::UMUL_LOHI, MVT::i32, Expand);

  setMaxAtomicSizeInBitsSupported(0);
}

//===----------------------------------------------------------------------===//
//  LowerOperation dispatch
//===----------------------------------------------------------------------===//

SDValue ARC4TargetLowering::LowerOperation(SDValue Op,
                                           SelectionDAG &DAG) const {
  switch (Op.getOpcode()) {
  case ISD::SELECT_CC:
    return LowerSELECT_CC(Op, DAG);
  case ISD::BR_CC:
    return LowerBR_CC(Op, DAG);
  case ISD::GlobalAddress: {
    auto *N = cast<GlobalAddressSDNode>(Op);
    SDValue Addr = DAG.getTargetGlobalAddress(N->getGlobal(), SDLoc(N),
                                              MVT::i32, N->getOffset());
    // Wrap in a target node so ISel can match it.
    // For now, just materialize with an AND_rrl (mov from limm).
    return Addr;
  }
  case ISD::SHL:
  case ISD::SRA:
  case ISD::SRL:
    return LowerShift(Op, DAG);
  default:
    llvm_unreachable("unimplemented operation lowering");
  }
  return SDValue();
}

//===----------------------------------------------------------------------===//
//  SELECT_CC lowering
//===----------------------------------------------------------------------===//

SDValue ARC4TargetLowering::LowerSELECT_CC(SDValue Op,
                                            SelectionDAG &DAG) const {
  SDValue LHS = Op.getOperand(0);
  SDValue RHS = Op.getOperand(1);
  SDValue TVal = Op.getOperand(2);
  SDValue FVal = Op.getOperand(3);
  ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(4))->get();
  SDLoc dl(Op);

  ARC4CC::CondCode ArcCC = getARC4CC(CC);

  SDValue Cmp = DAG.getNode(ARC4ISD::CMP, dl, MVT::Glue, LHS, RHS);
  return DAG.getNode(ARC4ISD::CMOV, dl, TVal.getValueType(), TVal, FVal,
                     DAG.getConstant(ArcCC, dl, MVT::i32), Cmp);
}

//===----------------------------------------------------------------------===//
//  BR_CC lowering
//===----------------------------------------------------------------------===//

SDValue ARC4TargetLowering::LowerBR_CC(SDValue Op, SelectionDAG &DAG) const {
  SDValue Chain = Op.getOperand(0);
  ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(1))->get();
  SDValue LHS = Op.getOperand(2);
  SDValue RHS = Op.getOperand(3);
  SDValue Dest = Op.getOperand(4);
  SDLoc dl(Op);

  ARC4CC::CondCode ArcCC = getARC4CC(CC);

  return DAG.getNode(ARC4ISD::BRcc, dl, MVT::Other, Chain, Dest, LHS, RHS,
                     DAG.getConstant(ArcCC, dl, MVT::i32));
}

//===----------------------------------------------------------------------===//
//  Shift lowering — expand to libcall (__ashlsi3, __ashrsi3, __lshrsi3)
//===----------------------------------------------------------------------===//

SDValue ARC4TargetLowering::LowerShift(SDValue Op, SelectionDAG &DAG) const {
  EVT VT = Op.getValueType();
  SDLoc DL(Op);
  SDValue LHS = Op.getOperand(0);
  SDValue RHS = Op.getOperand(1);

  RTLIB::Libcall LC;
  switch (Op.getOpcode()) {
  case ISD::SHL:
    LC = RTLIB::getSHL(VT);
    break;
  case ISD::SRA:
    LC = RTLIB::getSRA(VT);
    break;
  case ISD::SRL:
    LC = RTLIB::getSRL(VT);
    break;
  default:
    llvm_unreachable("unexpected shift opcode");
  }

  TargetLowering::MakeLibCallOptions CallOptions;
  return makeLibCall(DAG, LC, VT, {LHS, RHS}, CallOptions, DL).first;
}

//===----------------------------------------------------------------------===//
//  EmitInstrWithCustomInserter — expand SELECT_CC pseudo
//===----------------------------------------------------------------------===//

MachineBasicBlock *
ARC4TargetLowering::EmitInstrWithCustomInserter(MachineInstr &MI,
                                                MachineBasicBlock *BB) const {
  switch (MI.getOpcode()) {
  case ARC4::SELECT_CC: {
    // SELECT_CC dst, tval, fval, cc
    // Expand to: diamond control flow with conditional moves.
    //
    // thisMBB:
    //   ...
    //   cmp (done before this pseudo by ISel)
    //   bne sinkMBB     ; branch if condition false
    // copy0MBB:
    //   ; fall through — tval is selected
    // sinkMBB:
    //   dst = phi [tval, copy0MBB], [fval, thisMBB]

    const TargetInstrInfo *TII = Subtarget.getInstrInfo();
    DebugLoc DL = MI.getDebugLoc();

    const BasicBlock *LLVM_BB = BB->getBasicBlock();
    MachineFunction::iterator It = ++BB->getIterator();

    MachineBasicBlock *ThisMBB = BB;
    MachineFunction *F = BB->getParent();
    MachineBasicBlock *Copy0MBB = F->CreateMachineBasicBlock(LLVM_BB);
    MachineBasicBlock *SinkMBB = F->CreateMachineBasicBlock(LLVM_BB);
    F->insert(It, Copy0MBB);
    F->insert(It, SinkMBB);

    // Transfer rest of BB to SinkMBB.
    SinkMBB->splice(SinkMBB->begin(), ThisMBB,
                    std::next(MachineBasicBlock::iterator(MI)), ThisMBB->end());
    SinkMBB->transferSuccessorsAndUpdatePHIs(ThisMBB);

    // Conditional branch to SinkMBB (false path).
    // For SELECT_CC, the flags are already set by the CMP instruction
    // preceding this pseudo. We need to branch on the INVERSE condition.
    unsigned CC = MI.getOperand(3).getImm();
    // Invert the condition: we branch to SinkMBB on the FALSE condition
    // (when the condition is NOT met, we skip Copy0MBB and go to SinkMBB
    // with fval). When the condition IS met, we fall through to Copy0MBB
    // with tval.
    unsigned InvCC;
    switch (CC) {
    case ARC4CC::EQ: InvCC = ARC4CC::NE; break;
    case ARC4CC::NE: InvCC = ARC4CC::EQ; break;
    case ARC4CC::GT: InvCC = ARC4CC::LE; break;
    case ARC4CC::GE: InvCC = ARC4CC::LT; break;
    case ARC4CC::LT: InvCC = ARC4CC::GE; break;
    case ARC4CC::LE: InvCC = ARC4CC::GT; break;
    case ARC4CC::HI: InvCC = ARC4CC::LS; break;
    case ARC4CC::HS: InvCC = ARC4CC::LO; break;
    case ARC4CC::LO: InvCC = ARC4CC::HS; break;
    case ARC4CC::LS: InvCC = ARC4CC::HI; break;
    default: InvCC = ARC4CC::NE; break;
    }

    // b<inv_cc> sinkMBB
    BuildMI(ThisMBB, DL, TII->get(ARC4::B))
        .addMBB(SinkMBB)
        .addImm(InvCC) // q = inverted condition
        .addImm(0);    // n = no nullify

    ThisMBB->addSuccessor(Copy0MBB);
    ThisMBB->addSuccessor(SinkMBB);

    // Copy0MBB is just a fall-through.
    Copy0MBB->addSuccessor(SinkMBB);

    // SinkMBB: phi node.
    Register DstReg = MI.getOperand(0).getReg();
    Register TValReg = MI.getOperand(1).getReg();
    Register FValReg = MI.getOperand(2).getReg();

    BuildMI(*SinkMBB, SinkMBB->begin(), DL, TII->get(ARC4::PHI), DstReg)
        .addReg(TValReg)
        .addMBB(Copy0MBB)
        .addReg(FValReg)
        .addMBB(ThisMBB);

    MI.eraseFromParent();
    return SinkMBB;
  }
  default:
    llvm_unreachable("Unexpected instruction for custom inserter");
  }
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
//  Call lowering — helper to copy return values
//===----------------------------------------------------------------------===//

static SDValue lowerCallResult(SDValue Chain, SDValue Glue,
                               const SmallVectorImpl<CCValAssign> &RVLocs,
                               const SDLoc &dl, SelectionDAG &DAG,
                               SmallVectorImpl<SDValue> &InVals) {
  for (unsigned i = 0, e = RVLocs.size(); i != e; ++i) {
    const CCValAssign &VA = RVLocs[i];
    assert(VA.isRegLoc() && "ARC4 only supports register return values");
    SDValue RetValue =
        DAG.getCopyFromReg(Chain, dl, VA.getLocReg(), VA.getValVT(), Glue);
    Chain = RetValue.getValue(1);
    Glue = RetValue.getValue(2);
    InVals.push_back(RetValue);
  }
  return Chain;
}

//===----------------------------------------------------------------------===//
//  Call
//===----------------------------------------------------------------------===//

SDValue ARC4TargetLowering::LowerCall(TargetLowering::CallLoweringInfo &CLI,
                                      SmallVectorImpl<SDValue> &InVals) const {
  SelectionDAG &DAG = CLI.DAG;
  SDLoc &dl = CLI.DL;
  SmallVectorImpl<ISD::OutputArg> &Outs = CLI.Outs;
  SmallVectorImpl<SDValue> &OutVals = CLI.OutVals;
  SmallVectorImpl<ISD::InputArg> &Ins = CLI.Ins;
  SDValue Chain = CLI.Chain;
  SDValue Callee = CLI.Callee;
  CallingConv::ID CallConv = CLI.CallConv;
  bool IsVarArg = CLI.IsVarArg;
  bool &IsTailCall = CLI.IsTailCall;

  IsTailCall = false; // No tail call support yet.

  // Analyze outgoing arguments.
  SmallVector<CCValAssign, 16> ArgLocs;
  CCState CCInfo(CallConv, IsVarArg, DAG.getMachineFunction(), ArgLocs,
                 *DAG.getContext());
  CCInfo.AnalyzeCallOperands(Outs, CC_ARC4);

  // Analyze return values.
  SmallVector<CCValAssign, 16> RVLocs;
  CCState RetCCInfo(CallConv, IsVarArg, DAG.getMachineFunction(), RVLocs,
                    *DAG.getContext());
  RetCCInfo.AllocateStack(CCInfo.getStackSize(), Align(4));
  RetCCInfo.AnalyzeCallResult(Ins, RetCC_ARC4);

  unsigned NumBytes = RetCCInfo.getStackSize();

  // Emit CALLSEQ_START.
  Chain = DAG.getCALLSEQ_START(Chain, NumBytes, 0, dl);

  SmallVector<std::pair<unsigned, SDValue>, 8> RegsToPass;
  SmallVector<SDValue, 8> MemOpChains;

  SDValue StackPtr;

  // Walk register/memory assignments.
  for (unsigned i = 0, e = ArgLocs.size(); i != e; ++i) {
    CCValAssign &VA = ArgLocs[i];
    SDValue Arg = OutVals[i];

    // Promote the value if needed.
    switch (VA.getLocInfo()) {
    default:
      llvm_unreachable("Unknown loc info!");
    case CCValAssign::Full:
      break;
    case CCValAssign::SExt:
      Arg = DAG.getNode(ISD::SIGN_EXTEND, dl, VA.getLocVT(), Arg);
      break;
    case CCValAssign::ZExt:
      Arg = DAG.getNode(ISD::ZERO_EXTEND, dl, VA.getLocVT(), Arg);
      break;
    case CCValAssign::AExt:
      Arg = DAG.getNode(ISD::ANY_EXTEND, dl, VA.getLocVT(), Arg);
      break;
    }

    if (VA.isRegLoc()) {
      RegsToPass.push_back(std::make_pair(VA.getLocReg(), Arg));
    } else {
      assert(VA.isMemLoc() && "Must be register or memory argument.");
      if (!StackPtr.getNode())
        StackPtr = DAG.getCopyFromReg(Chain, dl, ARC4::SP,
                                      getPointerTy(DAG.getDataLayout()));
      SDValue SOffset = DAG.getIntPtrConstant(VA.getLocMemOffset(), dl);
      SDValue PtrOff = DAG.getNode(ISD::ADD, dl,
                                   getPointerTy(DAG.getDataLayout()),
                                   StackPtr, SOffset);
      SDValue Store =
          DAG.getStore(Chain, dl, Arg, PtrOff, MachinePointerInfo());
      MemOpChains.push_back(Store);
    }
  }

  // Chain all store nodes together.
  if (!MemOpChains.empty())
    Chain = DAG.getNode(ISD::TokenFactor, dl, MVT::Other, MemOpChains);

  // Build copy-to-reg chain for register arguments.
  SDValue Glue;
  for (unsigned i = 0, e = RegsToPass.size(); i != e; ++i) {
    Chain = DAG.getCopyToReg(Chain, dl, RegsToPass[i].first,
                             RegsToPass[i].second, Glue);
    Glue = Chain.getValue(1);
  }

  // Convert callee to target form.
  bool IsDirect = true;
  if (auto *G = dyn_cast<GlobalAddressSDNode>(Callee))
    Callee = DAG.getTargetGlobalAddress(G->getGlobal(), dl, MVT::i32);
  else if (auto *E = dyn_cast<ExternalSymbolSDNode>(Callee))
    Callee = DAG.getTargetExternalSymbol(E->getSymbol(), MVT::i32);
  else
    IsDirect = false;

  // Build the call node.
  SDVTList NodeTys = DAG.getVTList(MVT::Other, MVT::Glue);
  SmallVector<SDValue, 8> Ops;
  Ops.push_back(Chain);
  Ops.push_back(Callee);

  for (unsigned i = 0, e = RegsToPass.size(); i != e; ++i)
    Ops.push_back(DAG.getRegister(RegsToPass[i].first,
                                  RegsToPass[i].second.getValueType()));

  // Add call-preserved register mask.
  const TargetRegisterInfo *TRI = Subtarget.getRegisterInfo();
  const uint32_t *Mask =
      TRI->getCallPreservedMask(DAG.getMachineFunction(), CallConv);
  assert(Mask && "Missing call preserved mask for calling convention");
  Ops.push_back(DAG.getRegisterMask(Mask));

  if (Glue.getNode())
    Ops.push_back(Glue);

  Chain = DAG.getNode(IsDirect ? ARC4ISD::BL : ARC4ISD::JL, dl, NodeTys, Ops);
  Glue = Chain.getValue(1);

  // Emit CALLSEQ_END.
  Chain = DAG.getCALLSEQ_END(Chain, NumBytes, 0, Glue, dl);
  Glue = Chain.getValue(1);

  // Copy return values from physical registers.
  return lowerCallResult(Chain, Glue, RVLocs, dl, DAG, InVals);
}
