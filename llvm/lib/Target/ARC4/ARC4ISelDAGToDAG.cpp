//===-- ARC4ISelDAGToDAG.cpp - ARC4 DAG->DAG Instruction Selection ---------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ARC4.h"
#include "ARC4TargetMachine.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/SelectionDAGISel.h"

using namespace llvm;

#define DEBUG_TYPE "arc4-isel"
#define PASS_NAME "ARC4 DAG->DAG Instruction Selection"

namespace {

class ARC4DAGToDAGISel : public SelectionDAGISel {
public:
  ARC4DAGToDAGISel() = delete;
  explicit ARC4DAGToDAGISel(ARC4TargetMachine &TM, CodeGenOptLevel OL = CodeGenOptLevel::Default)
      : SelectionDAGISel(TM, OL) {}

  bool runOnMachineFunction(MachineFunction &MF) override {
    return SelectionDAGISel::runOnMachineFunction(MF);
  }

  void Select(SDNode *N) override;

  /// Try to select an address as base+offset for loads/stores.
  bool SelectAddrRI(SDValue Addr, SDValue &Base, SDValue &Offset);

private:
  #include "ARC4GenDAGISel.inc"
};

class ARC4DAGToDAGISelLegacy : public SelectionDAGISelLegacy {
public:
  static char ID;
  explicit ARC4DAGToDAGISelLegacy(ARC4TargetMachine &TM)
      : SelectionDAGISelLegacy(ID, std::make_unique<ARC4DAGToDAGISel>(TM)) {}
};

char ARC4DAGToDAGISelLegacy::ID = 0;

} // namespace

void ARC4DAGToDAGISel::Select(SDNode *N) {
  if (N->isMachineOpcode()) {
    N->setNodeId(-1);
    return;
  }

  switch (N->getOpcode()) {
  case ISD::FrameIndex: {
    SDLoc DL(N);
    int FI = cast<FrameIndexSDNode>(N)->getIndex();
    SDValue TFI = CurDAG->getTargetFrameIndex(FI, MVT::i32);
    SDValue Zero = CurDAG->getTargetConstant(0, DL, MVT::i32);
    ReplaceNode(N, CurDAG->getMachineNode(ARC4::CG_ADDri, DL, MVT::i32,
                                          TFI, Zero));
    return;
  }
  case ISD::LOAD: {
    // Custom-select loads to handle FrameIndex addresses without an ADD.
    auto *LD = cast<LoadSDNode>(N);
    if (LD->getExtensionType() != ISD::NON_EXTLOAD)
      break; // Let tablegen handle extending loads
    SDLoc DL(N);
    SDValue Base, Offset;
    if (SelectAddrRI(LD->getBasePtr(), Base, Offset)) {
      SDValue Ops[] = {Base, Offset, LD->getChain()};
      auto *ResNode = CurDAG->getMachineNode(ARC4::CG_LDri, DL,
                                             MVT::i32, MVT::Other, Ops);
      CurDAG->setNodeMemRefs(ResNode, {LD->getMemOperand()});
      ReplaceNode(N, ResNode);
      return;
    }
    break;
  }
  case ISD::STORE: {
    auto *ST = cast<StoreSDNode>(N);
    if (ST->isTruncatingStore())
      break;
    SDLoc DL(N);
    SDValue Base, Offset;
    if (SelectAddrRI(ST->getBasePtr(), Base, Offset)) {
      SDValue Ops[] = {ST->getValue(), Base, Offset, ST->getChain()};
      auto *ResNode = CurDAG->getMachineNode(ARC4::CG_STri, DL,
                                             MVT::Other, Ops);
      CurDAG->setNodeMemRefs(ResNode, {ST->getMemOperand()});
      ReplaceNode(N, ResNode);
      return;
    }
    break;
  }
  case ISD::Constant: {
    // Materialize constants using MOV pseudo.
    // Per spec (page 108), MOV is the AND instruction:
    //   MOV a,shimm = AND a,shimm,shimm (shimms must match, valid per spec)
    //   MOV a,limm  = AND a,limm (32-bit immediate in following word)
    SDLoc DL(N);
    int64_t SVal = cast<ConstantSDNode>(N)->getSExtValue();
    uint64_t Val = static_cast<uint64_t>(SVal) & 0xFFFFFFFF;
    SDValue Imm = CurDAG->getTargetConstant(Val, DL, MVT::i32);
    if (SVal >= -256 && SVal <= 255) {
      // shimm MOV: AND dst, shimm, shimm
      ReplaceNode(N, CurDAG->getMachineNode(ARC4::CG_MOVri, DL, MVT::i32, Imm));
    } else {
      // limm MOV: AND dst, limm
      ReplaceNode(N, CurDAG->getMachineNode(ARC4::CG_MOVli, DL, MVT::i32, Imm));
    }
    return;
  }
  default:
    break;
  }

  SelectCode(N);
}

bool ARC4DAGToDAGISel::SelectAddrRI(SDValue Addr, SDValue &Base,
                                    SDValue &Offset) {
  SDLoc DL(Addr);

  // Match: (add base, imm)
  if (Addr.getOpcode() == ISD::ADD) {
    if (auto *CN = dyn_cast<ConstantSDNode>(Addr.getOperand(1))) {
      Base = Addr.getOperand(0);
      Offset = CurDAG->getTargetConstant(CN->getSExtValue(), DL, MVT::i32);
      return true;
    }
    // (add fi, imm) where fi was already selected
    if (auto *FIN = dyn_cast<FrameIndexSDNode>(Addr.getOperand(0))) {
      Base = CurDAG->getTargetFrameIndex(FIN->getIndex(), MVT::i32);
      if (auto *CN = dyn_cast<ConstantSDNode>(Addr.getOperand(1)))
        Offset = CurDAG->getTargetConstant(CN->getSExtValue(), DL, MVT::i32);
      else
        Offset = CurDAG->getTargetConstant(0, DL, MVT::i32);
      return true;
    }
  }

  // Match: bare FrameIndex (load/store from stack slot with no offset)
  if (auto *FIN = dyn_cast<FrameIndexSDNode>(Addr)) {
    Base = CurDAG->getTargetFrameIndex(FIN->getIndex(), MVT::i32);
    Offset = CurDAG->getTargetConstant(0, DL, MVT::i32);
    return true;
  }

  // Fallback: treat the whole address as a base with offset 0
  Base = Addr;
  Offset = CurDAG->getTargetConstant(0, DL, MVT::i32);
  return true;
}

FunctionPass *llvm::createARC4ISelDag(ARC4TargetMachine &TM) {
  return new ARC4DAGToDAGISelLegacy(TM);
}
