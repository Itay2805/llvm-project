//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ARC4SelectionDAGInfo.h"

#define GET_SDNODE_DESC
#include "ARC4GenSDNodeInfo.inc"

using namespace llvm;

ARC4SelectionDAGInfo::ARC4SelectionDAGInfo()
    : SelectionDAGGenTargetInfo(ARC4GenSDNodeInfo) {}

ARC4SelectionDAGInfo::~ARC4SelectionDAGInfo() = default;
