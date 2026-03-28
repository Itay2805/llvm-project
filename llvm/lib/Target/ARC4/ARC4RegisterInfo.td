//===-- ARC4RegisterInfo.td - ARC4 Register defs -----------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

class ARC4Reg<bits<6> enc, string n, list<string> alt = []>
    : Register<n>, DwarfRegNum<[enc]> {
  let HWEncoding{5-0} = enc;
  let AltNames = alt;
  let Namespace = "ARC4";
}

// Core GPRs r0-r31
def R0  : ARC4Reg< 0, "r0">;
def R1  : ARC4Reg< 1, "r1">;
def R2  : ARC4Reg< 2, "r2">;
def R3  : ARC4Reg< 3, "r3">;
def R4  : ARC4Reg< 4, "r4">;
def R5  : ARC4Reg< 5, "r5">;
def R6  : ARC4Reg< 6, "r6">;
def R7  : ARC4Reg< 7, "r7">;
def R8  : ARC4Reg< 8, "r8">;
def R9  : ARC4Reg< 9, "r9">;
def R10 : ARC4Reg<10, "r10">;
def R11 : ARC4Reg<11, "r11">;
def R12 : ARC4Reg<12, "r12">;
def R13 : ARC4Reg<13, "r13">;
def R14 : ARC4Reg<14, "r14">;
def R15 : ARC4Reg<15, "r15">;
def R16 : ARC4Reg<16, "r16">;
def R17 : ARC4Reg<17, "r17">;
def R18 : ARC4Reg<18, "r18">;
def R19 : ARC4Reg<19, "r19">;
def R20 : ARC4Reg<20, "r20">;
def R21 : ARC4Reg<21, "r21">;
def R22 : ARC4Reg<22, "r22">;
def R23 : ARC4Reg<23, "r23">;
def R24 : ARC4Reg<24, "r24">;
def R25 : ARC4Reg<25, "r25">;
def GP  : ARC4Reg<26, "gp",  ["r26"]>;
def FP  : ARC4Reg<27, "fp",  ["r27"]>;
def SP  : ARC4Reg<28, "sp",  ["r28"]>;
def ILINK1 : ARC4Reg<29, "ilink1", ["r29"]>;
def ILINK2 : ARC4Reg<30, "ilink2", ["r30"]>;
def BLINK  : ARC4Reg<31, "blink",  ["r31"]>;

// Special registers
def LP_COUNT : ARC4Reg<60, "lp_count">;

// Status register (condition flags: Z, N, C, V). Not directly accessible
// by general instructions, but needed to model flag dependencies.
def STATUS32 : ARC4Reg<63, "status32">;

// GPR32 - allocatable general purpose registers
def GPR32 : RegisterClass<"ARC4", [i32], 32,
  (add R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12,
       R0, R1, R13, R14, R15, R16, R17, R18, R19, R20, R21, R22, R23, R24, R25,
       GP, FP, SP, BLINK)> {
  let RegInfos = RegInfoByHwMode<[DefaultMode], [RegInfo<32, 32, 32>]>;
}

def GPR32NoSP : RegisterClass<"ARC4", [i32], 32,
  (sub GPR32, SP)>;

def LP : RegisterClass<"ARC4", [i32], 32, (add LP_COUNT)> {
  let isAllocatable = 0;
}
