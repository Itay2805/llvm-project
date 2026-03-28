//===- ARC4Nodes.td - ARC4 SelectionDAG Nodes --------------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Target-specific DAG node definitions for ARC4.
//
//===----------------------------------------------------------------------===//

// Call return — maps to j [blink].
def ARC4ret : SDNode<"ARC4ISD::RET", SDTNone,
                     [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;
