//===-- ARC4RegisterInfo.td - ARC4 Register Definitions ----*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

class ARC4Reg<bits<6> enc, string n, list<string> altNames = []>
    : Register<n, altNames> {
  field bits<6> Enc = enc;
  let Namespace = "ARC4";
  let HWEncoding{5-0} = enc;
}

// General-purpose registers r0–r28
foreach i = 0-28 in {
  def R#i : ARC4Reg<i, "r"#i>, DwarfRegNum<[i]>;
}

// r29–r31: aliased link/branch registers
def R29 : ARC4Reg<29, "r29", ["ilink1"]>, DwarfRegNum<[29]>;
def R30 : ARC4Reg<30, "r30", ["ilink2"]>, DwarfRegNum<[30]>;
def R31 : ARC4Reg<31, "r31", ["blink"]>,  DwarfRegNum<[31]>;

// r32–r59: extension registers (defined but not user-visible in asm)
foreach i = 32-59 in {
  def R#i : ARC4Reg<i, "r"#i>, DwarfRegNum<[i]>;
}

// r60: loop count
def R60 : ARC4Reg<60, "lp_count">, DwarfRegNum<[60]>;

// Encoding-only sentinel registers (not user-visible in asm)
// r61: short-immediate with flag-set
// r62: long-immediate (limm word follows)
// r63: short-immediate without flag-set
def R61 : ARC4Reg<61, "r61">;
def R62 : ARC4Reg<62, "r62">;
def R63 : ARC4Reg<63, "r63">;

// Register classes
// GR: all user-addressable general-purpose registers
def GR : RegisterClass<"ARC4", [i32], 32,
    (add (sequence "R%u", 0, 31), R60)>;

// SENTIN: sentinel registers used by the encoder only
def SENTIN : RegisterClass<"ARC4", [i32], 32, (add R61, R62, R63)> {
  let isAllocatable = 0;
}
