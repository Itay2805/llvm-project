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

//===----------------------------------------------------------------------===//
// Call nodes
//===----------------------------------------------------------------------===//

def SDT_ARC4Call : SDTypeProfile<0, -1, [SDTCisVT<0, iPTR>]>;
def SDT_ARC4CallSeqStart : SDCallSeqStart<[SDTCisVT<0, i32>, SDTCisVT<1, i32>]>;
def SDT_ARC4CallSeqEnd   : SDCallSeqEnd<[SDTCisVT<0, i32>, SDTCisVT<1, i32>]>;

// Direct call (bl)
def ARC4call : SDNode<"ARC4ISD::BL", SDT_ARC4Call,
                      [SDNPHasChain, SDNPOutGlue, SDNPOptInGlue, SDNPVariadic]>;

// Indirect call (jl)
def ARC4callind : SDNode<"ARC4ISD::JL", SDT_ARC4Call,
                         [SDNPHasChain, SDNPOutGlue, SDNPOptInGlue, SDNPVariadic]>;

// Call sequence markers
def callseq_start : SDNode<"ISD::CALLSEQ_START", SDT_ARC4CallSeqStart,
                           [SDNPHasChain, SDNPOutGlue]>;
def callseq_end   : SDNode<"ISD::CALLSEQ_END",   SDT_ARC4CallSeqEnd,
                           [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue]>;

//===----------------------------------------------------------------------===//
// Comparison / conditional nodes
//===----------------------------------------------------------------------===//

def SDT_ARC4Cmp  : SDTypeProfile<0, 2, [SDTCisSameAs<0, 1>]>;
def SDT_ARC4Cmov : SDTypeProfile<1, 3, [SDTCisSameAs<0, 1>]>;
def SDT_ARC4BRcc : SDTypeProfile<0, 4, []>;

// Compare — sets flags (glue out)
def ARC4cmp : SDNode<"ARC4ISD::CMP", SDT_ARC4Cmp, [SDNPOutGlue]>;

// Conditional move — consumes flags (glue in)
def ARC4cmov : SDNode<"ARC4ISD::CMOV", SDT_ARC4Cmov, [SDNPInGlue]>;

// Conditional branch — has chain + consumes operands
def ARC4brcc : SDNode<"ARC4ISD::BRcc", SDT_ARC4BRcc,
                      [SDNPHasChain]>;
