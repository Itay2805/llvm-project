//===-- ARC4InstrInfo.td - ARC4 Instruction defs -----------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Instruction encoding reference: binutils-2.15/opcodes/arc-opc.c
//
//===----------------------------------------------------------------------===//

include "ARC4InstrFormats.td"

//===----------------------------------------------------------------------===//
// NOP
//===----------------------------------------------------------------------===//

let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
def NOP : ARC4Inst<(outs), (ins), "nop", []> {
  let Inst{31-0} = 0x7fffffff;
}

//===----------------------------------------------------------------------===//
// 3-Operand ALU Instructions (register-register)
// Format: op a, b, c
// Encoding: I[31:27]=opcode, A[26:21], B[20:15], C[14:9], flags[8:0]=0
//===----------------------------------------------------------------------===//

multiclass ALU3<bits<5> opcode, string mnemonic> {
  def _rrr : ARC4InstALU3<opcode, (outs GPR32:$a), (ins GPR32:$b, GPR32:$c),
                           mnemonic # "\t$a, $b, $c", []>;
}

defm ADD : ALU3<0b01000, "add">;  // opcode 8
defm ADC : ALU3<0b01001, "adc">;  // opcode 9
defm SUB : ALU3<0b01010, "sub">;  // opcode 10
defm SBC : ALU3<0b01011, "sbc">;  // opcode 11
defm AND : ALU3<0b01100, "and">;  // opcode 12
defm OR  : ALU3<0b01101, "or">;   // opcode 13
defm BIC : ALU3<0b01110, "bic">;  // opcode 14
defm XOR : ALU3<0b01111, "xor">;  // opcode 15

//===----------------------------------------------------------------------===//
// Single-Operand Instructions (opcode 3, sub-opcode in C field)
// Format: op a, b
// Encoding: I[31:27]=0b00011, A[26:21], B[20:15], C[14:9]=subop
//===----------------------------------------------------------------------===//

def ASL  : ARC4InstSOP<0b000000, (outs GPR32:$a), (ins GPR32:$b), "asl\t$a, $b", []>;
def ASR  : ARC4InstSOP<0b000001, (outs GPR32:$a), (ins GPR32:$b), "asr\t$a, $b", []>;
def LSR  : ARC4InstSOP<0b000010, (outs GPR32:$a), (ins GPR32:$b), "lsr\t$a, $b", []>;
def ROR  : ARC4InstSOP<0b000011, (outs GPR32:$a), (ins GPR32:$b), "ror\t$a, $b", []>;
def RRC  : ARC4InstSOP<0b000100, (outs GPR32:$a), (ins GPR32:$b), "rrc\t$a, $b", []>;
def SEXB : ARC4InstSOP<0b000101, (outs GPR32:$a), (ins GPR32:$b), "sexb\t$a, $b", []>;
def SEXW : ARC4InstSOP<0b000110, (outs GPR32:$a), (ins GPR32:$b), "sexw\t$a, $b", []>;
def EXTB : ARC4InstSOP<0b000111, (outs GPR32:$a), (ins GPR32:$b), "extb\t$a, $b", []>;
def EXTW : ARC4InstSOP<0b001000, (outs GPR32:$a), (ins GPR32:$b), "extw\t$a, $b", []>;

//===----------------------------------------------------------------------===//
// Aliases
//===----------------------------------------------------------------------===//

// mov a, b => and a, b, b
def : InstAlias<"mov\t$a, $b", (AND_rrr GPR32:$a, GPR32:$b, GPR32:$b)>;
