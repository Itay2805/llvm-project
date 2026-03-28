//===-- ARC4InstrFormats.td - ARC4 Instruction Formats -----*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// ARC4 (ARCtangent-A4) instruction format definitions.
//
// Encoding reference: binutils-2.15/opcodes/arc-opc.c
//                     include/opcode/arc.h
//
// Word layout (32-bit, little-endian):
//   I[31:27] = major opcode (5 bits)
//   A[26:21] = register A / destination (6 bits)
//   B[20:15] = register B / source 1 (6 bits)
//   C[14:9]  = register C / source 2 (6 bits)
//   [8:0]    = flags/shimm (9 bits)
//
// Sentinel values in 6-bit register fields:
//   61 (0b111101) = SHIMM_UPDATE  - short immediate with flag update
//   62 (0b111110) = LIMM          - long immediate (extra 32-bit word)
//   63 (0b111111) = SHIMM         - short immediate without flag update
//
// Shimm: 9-bit signed value in bits [8:0]. Overlaps condition code bits [4:0],
//   so .q (condition code) is NOT available with shimm.
//
// Flag (.f):
//   Without shimm: bit [8] = 1
//   With shimm: sentinel 61 vs 63 in register field encodes .f.
//               Bit [8] is part of the shimm value, not the flag.
//
// Limm: Sentinel 62 in register field; 32-bit value in extra word. Size = 8.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Base class
//===----------------------------------------------------------------------===//

class ARC4Inst<dag outs, dag ins, string asmstr, list<dag> pattern>
    : Instruction {
  let Namespace = "ARC4";
  let Size = 4;

  // TSFlags — instruction property bits queried by MCCodeEmitter,
  // AsmParser, InstPrinter, and Disassembler.
  //
  // Bit 0: HasLimm    — 32-bit LIMM extra word follows instruction
  // Bit 1: IsShimm    — shimm sentinel (61/63) in B or C field
  // Bit 2: HasFQ      — has .f and .q suffix operands (non-shimm ALU/SOP)
  // Bit 3: HasFOnly   — has .f suffix only, no .q (shimm ALU/SOP)
  // Bit 4: HasQN      — has .q and .n suffix operands (branch)
  // Bit 5: HasFQN     — has .f, .q, .n suffix operands (jump)
  // Bit 6: HasLdMod3  — has 3 load modifiers (x, w/W, e/E)
  // Bit 7: HasLdMod2  — has 2 load modifiers (x/X, e/E, no writeback)
  // Bit 8: HasStMod2  — has 2 store modifiers (v, D)
  // Bit 9: HasStMod1  — has 1 store modifier (D only)
  bits<1> HasLimm = 0;
  bits<1> IsShimm = 0;
  bits<1> HasFQ = 0;
  bits<1> HasFOnly = 0;
  bits<1> HasQN = 0;
  bits<1> HasFQN = 0;
  bits<1> HasLdMod3 = 0;
  bits<1> HasLdMod2 = 0;
  bits<1> HasStMod2 = 0;
  bits<1> HasStMod1 = 0;
  let TSFlags{0} = HasLimm;
  let TSFlags{1} = IsShimm;
  let TSFlags{2} = HasFQ;
  let TSFlags{3} = HasFOnly;
  let TSFlags{4} = HasQN;
  let TSFlags{5} = HasFQN;
  let TSFlags{6} = HasLdMod3;
  let TSFlags{7} = HasLdMod2;
  let TSFlags{8} = HasStMod2;
  let TSFlags{9} = HasStMod1;

  field bits<32> Inst;
  field bits<32> SoftFail = 0;

  dag OutOperandList = outs;
  dag InOperandList = ins;
  let AsmString = asmstr;
  let Pattern = pattern;
}

// Base for 8-byte instructions (those with limm extra word).
// The limm word is handled by MCCodeEmitter; TableGen only encodes
// the first 32-bit word. HasLimm = 1 tells the emitter/disassembler
// that a second 32-bit word follows.
class ARC4Inst8<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  let HasLimm = 1;
}

//===----------------------------------------------------------------------===//
// 3-operand ALU formats  (opcode in I[31:27])
//
// op a, b, c  /  op a, b, shimm  /  op a, b, limm  etc.
//===----------------------------------------------------------------------===//

// --- rrr: op a, b, c  (reg, reg, reg) ---
// I[31:27]=opcode, A[26:21], B[20:15], C[14:9], f[8], q[4:0]
class ARC4InstALU3_rrr<bits<5> opcode, dag outs, dag ins, string asmstr,
                       list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  let HasFQ = 1;
  bits<6> a;
  bits<6> b;
  bits<6> c;
  bits<1> f;   // flag bit, wired to bit [8]
  bits<5> q;   // condition code, wired to bits [4:0]

  let Inst{31-27} = opcode;
  let Inst{26-21} = a;
  let Inst{20-15} = b;
  let Inst{14-9}  = c;
  let Inst{8}     = f;
  let Inst{7-5}   = 0;
  let Inst{4-0}   = q;
}

// --- rrs: op a, b, shimm  (reg, reg, short-imm) ---
// C field = sentinel 63 (shimm, no flag update). MCCodeEmitter changes to 61
// when .f is specified.
// shimm[8:0] occupies bits [8:0].
class ARC4InstALU3_rrs<bits<5> opcode, dag outs, dag ins, string asmstr,
                       list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  let IsShimm = 1;
  let HasFOnly = 1;
  bits<6> a;
  bits<6> b;
  bits<9> shimm;
  bits<1> f;   // NOT wired to encoding — MCCodeEmitter flips sentinel 63->61

  let Inst{31-27} = opcode;
  let Inst{26-21} = a;
  let Inst{20-15} = b;
  let Inst{14-9}  = 0b111111;  // sentinel 63 = SHIMM (no .f)
  let Inst{8-0}   = shimm;
}

// --- rsr: op a, shimm, c  (reg, short-imm, reg) ---
// B field = sentinel 63.
class ARC4InstALU3_rsr<bits<5> opcode, dag outs, dag ins, string asmstr,
                       list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  let IsShimm = 1;
  let HasFOnly = 1;
  bits<6> a;
  bits<6> c;
  bits<9> shimm;
  bits<1> f;   // NOT wired to encoding — MCCodeEmitter flips sentinel 63->61

  let Inst{31-27} = opcode;
  let Inst{26-21} = a;
  let Inst{20-15} = 0b111111;  // sentinel 63 = SHIMM (no .f)
  let Inst{14-9}  = c;
  let Inst{8-0}   = shimm;
}

// --- rss: op a, shimm, shimm  (reg, short-imm, short-imm) ---
// Both B and C fields = sentinel 63. Same shimm value in both positions.
class ARC4InstALU3_rss<bits<5> opcode, dag outs, dag ins, string asmstr,
                       list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  let IsShimm = 1;
  let HasFOnly = 1;
  bits<6> a;
  bits<9> shimm;
  bits<1> f;   // NOT wired to encoding — MCCodeEmitter flips sentinel 63->61

  let Inst{31-27} = opcode;
  let Inst{26-21} = a;
  let Inst{20-15} = 0b111111;  // sentinel 63 = SHIMM
  let Inst{14-9}  = 0b111111;  // sentinel 63 = SHIMM
  let Inst{8-0}   = shimm;
}

// --- rrl: op a, b, limm  (reg, reg, long-imm) ---
// C field = sentinel 62 (LIMM). Extra 32-bit word follows. Size = 8.
class ARC4InstALU3_rrl<bits<5> opcode, dag outs, dag ins, string asmstr,
                       list<dag> pattern>
    : ARC4Inst8<outs, ins, asmstr, pattern> {
  let HasFQ = 1;
  bits<6> a;
  bits<6> b;
  bits<1> f;   // flag bit, wired to bit [8]
  bits<5> q;   // condition code, wired to bits [4:0]

  let Inst{31-27} = opcode;
  let Inst{26-21} = a;
  let Inst{20-15} = b;
  let Inst{14-9}  = 0b111110;  // sentinel 62 = LIMM
  let Inst{8}     = f;
  let Inst{7-5}   = 0;
  let Inst{4-0}   = q;
}

// --- rlr: op a, limm, c  (reg, long-imm, reg) ---
// B field = sentinel 62 (LIMM). Extra 32-bit word follows. Size = 8.
class ARC4InstALU3_rlr<bits<5> opcode, dag outs, dag ins, string asmstr,
                       list<dag> pattern>
    : ARC4Inst8<outs, ins, asmstr, pattern> {
  let HasFQ = 1;
  bits<6> a;
  bits<6> c;
  bits<1> f;   // flag bit, wired to bit [8]
  bits<5> q;   // condition code, wired to bits [4:0]

  let Inst{31-27} = opcode;
  let Inst{26-21} = a;
  let Inst{20-15} = 0b111110;  // sentinel 62 = LIMM
  let Inst{14-9}  = c;
  let Inst{8}     = f;
  let Inst{7-5}   = 0;
  let Inst{4-0}   = q;
}

//===----------------------------------------------------------------------===//
// Single-operand format  (opcode 3, sub-op in C[14:9])
//
// op a, b  /  op a, shimm  /  op a, limm
//===----------------------------------------------------------------------===//

// --- rr: op a, b  (reg, reg) ---
class ARC4InstSOP_rr<bits<6> subop, dag outs, dag ins, string asmstr,
                     list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  let HasFQ = 1;
  bits<6> a;
  bits<6> b;
  bits<1> f;   // flag bit, wired to bit [8]
  bits<5> q;   // condition code, wired to bits [4:0]

  let Inst{31-27} = 0b00011;  // opcode 3
  let Inst{26-21} = a;
  let Inst{20-15} = b;
  let Inst{14-9}  = subop;
  let Inst{8}     = f;
  let Inst{7-5}   = 0;
  let Inst{4-0}   = q;
}

// --- rs: op a, shimm  (reg, short-imm) ---
// B field = sentinel 63 (SHIMM). shimm[8:0] in bits [8:0].
class ARC4InstSOP_rs<bits<6> subop, dag outs, dag ins, string asmstr,
                     list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  let IsShimm = 1;
  let HasFOnly = 1;
  bits<6> a;
  bits<9> shimm;
  bits<1> f;   // NOT wired to encoding — MCCodeEmitter flips sentinel 63->61

  let Inst{31-27} = 0b00011;  // opcode 3
  let Inst{26-21} = a;
  let Inst{20-15} = 0b111111;  // sentinel 63 = SHIMM
  let Inst{14-9}  = subop;
  let Inst{8-0}   = shimm;
}

// --- rl: op a, limm  (reg, long-imm) ---
// B field = sentinel 62 (LIMM). Size = 8.
class ARC4InstSOP_rl<bits<6> subop, dag outs, dag ins, string asmstr,
                     list<dag> pattern>
    : ARC4Inst8<outs, ins, asmstr, pattern> {
  let HasFQ = 1;
  bits<6> a;
  bits<1> f;   // flag bit, wired to bit [8]
  bits<5> q;   // condition code, wired to bits [4:0]

  let Inst{31-27} = 0b00011;  // opcode 3
  let Inst{26-21} = a;
  let Inst{20-15} = 0b111110;  // sentinel 62 = LIMM
  let Inst{14-9}  = subop;
  let Inst{8}     = f;
  let Inst{7-5}   = 0;
  let Inst{4-0}   = q;
}

//===----------------------------------------------------------------------===//
// Branch format  (b=opcode 4, bl=opcode 5, lp=opcode 6)
//
// offset[26:7] = 20-bit signed PC-relative offset
// n[6:5]       = delay slot mode (nullify)
// q[4:0]       = condition code
//===----------------------------------------------------------------------===//

class ARC4InstBranch<bits<5> opcode, dag outs, dag ins, string asmstr,
                     list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  let HasQN = 1;
  bits<20> offset;
  bits<2>  n;
  bits<5>  q;

  let Inst{31-27} = opcode;
  let Inst{26-7}  = offset;
  let Inst{6-5}   = n;
  let Inst{4-0}   = q;
}

//===----------------------------------------------------------------------===//
// Jump format  (opcode 7)
//
// j/jl target in register or limm.
// A[26:21] = 0 (unused for jump)
// B[20:15] = target register or 62 (LIMM)
// C[14:9]  = 0
// link[9]  = 1 for jl, 0 for j
// f[8]     = flag
// R(0,7,1) = bit 7 forced to 0
// n[6:5]   = delay slot mode
// q[4:0]   = condition code
//===----------------------------------------------------------------------===//

// --- Jump via register ---
class ARC4InstJumpReg<bit link, dag outs, dag ins, string asmstr,
                      list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  let HasFQN = 1;
  bits<6> b;
  bits<1> f;   // flag bit, wired to bit [8]
  bits<2> n;
  bits<5> q;

  let Inst{31-27} = 0b00111;  // opcode 7
  let Inst{26-21} = 0;        // A = 0
  let Inst{20-15} = b;
  let Inst{14-10} = 0;        // C[14:10] = 0
  let Inst{9}     = link;
  let Inst{8}     = f;
  let Inst{7}     = 0;        // forced 0 per R(0,7,1)
  let Inst{6-5}   = n;
  let Inst{4-0}   = q;
}

// --- Jump via limm ---
class ARC4InstJumpLimm<bit link, dag outs, dag ins, string asmstr,
                       list<dag> pattern>
    : ARC4Inst8<outs, ins, asmstr, pattern> {
  let HasFQN = 1;
  bits<1> f;   // flag bit, wired to bit [8]
  bits<2> n;
  bits<5> q;

  let Inst{31-27} = 0b00111;  // opcode 7
  let Inst{26-21} = 0;        // A = 0
  let Inst{20-15} = 0b111110; // B = sentinel 62 = LIMM
  let Inst{14-10} = 0;        // C[14:10] = 0
  let Inst{9}     = link;
  let Inst{8}     = f;
  let Inst{7}     = 0;        // forced 0
  let Inst{6-5}   = n;
  let Inst{4-0}   = q;
}

//===----------------------------------------------------------------------===//
// Load format - opcode 0  (register + register offset)
//
// ld a, [b, c]
// Fields: a[26:21], b[20:15], c[14:9]=offset reg
// Modifier bits:  e[5]=cache bypass, (bit 4 unused), w[3]=writeback,
//                 z[2:1]=size, x[0]=sign extend
//===----------------------------------------------------------------------===//

// --- LD0 reg,reg: ld a, [b, c] ---
class ARC4InstLD0_rr<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  let HasLdMod3 = 1;
  bits<6> a;
  bits<6> b;
  bits<6> c;
  bits<1> e;  // cache bypass, bit 5
  bits<1> w;  // writeback, bit 3
  bits<2> z;  // size, bits 2:1
  bits<1> x;  // sign extend, bit 0

  let Inst{31-27} = 0b00000;  // opcode 0
  let Inst{26-21} = a;
  let Inst{20-15} = b;
  let Inst{14-9}  = c;
  let Inst{8-7}   = 0;
  let Inst{6}     = 0;
  let Inst{5}     = e;
  let Inst{4}     = 0;
  let Inst{3}     = w;
  let Inst{2-1}   = z;
  let Inst{0}     = x;
}

// --- LD0 reg,limm: ld a, [b, limm] ---  C = 62
class ARC4InstLD0_rl<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ARC4Inst8<outs, ins, asmstr, pattern> {
  let HasLdMod3 = 1;
  bits<6> a;
  bits<6> b;
  bits<1> e;
  bits<1> w;
  bits<2> z;
  bits<1> x;

  let Inst{31-27} = 0b00000;
  let Inst{26-21} = a;
  let Inst{20-15} = b;
  let Inst{14-9}  = 0b111110;  // C = LIMM sentinel
  let Inst{8-7}   = 0;
  let Inst{6}     = 0;
  let Inst{5}     = e;
  let Inst{4}     = 0;
  let Inst{3}     = w;
  let Inst{2-1}   = z;
  let Inst{0}     = x;
}

// --- LD0 limm,reg: ld a, [limm, c] ---  B = 62, no writeback
class ARC4InstLD0_lr<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ARC4Inst8<outs, ins, asmstr, pattern> {
  let HasLdMod2 = 1;
  bits<6> a;
  bits<6> c;
  bits<1> e;
  bits<2> z;
  bits<1> x;

  let Inst{31-27} = 0b00000;
  let Inst{26-21} = a;
  let Inst{20-15} = 0b111110;  // B = LIMM sentinel
  let Inst{14-9}  = c;
  let Inst{8-7}   = 0;
  let Inst{6}     = 0;
  let Inst{5}     = e;
  let Inst{4}     = 0;
  let Inst{3}     = 0;        // no writeback with limm base
  let Inst{2-1}   = z;
  let Inst{0}     = x;
}

//===----------------------------------------------------------------------===//
// Load format - opcode 1  (register + shimm offset)
//
// ld a, [b, shimm]
// Fields: a[26:21], b[20:15], E[14]=cache, bit13=0,
//         W[12]=writeback, Z[11:10]=size, X[9]=sign extend, shimm[8:0]
//===----------------------------------------------------------------------===//

// --- LD1 reg,shimm: ld a, [b, shimm] ---
class ARC4InstLD1_rs<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  let HasLdMod3 = 1;
  bits<6> a;
  bits<6> b;
  bits<1> E;   // cache bypass, bit 14
  bits<1> W;   // writeback, bit 12
  bits<2> Z;   // size, bits 11:10
  bits<1> X;   // sign extend, bit 9
  bits<9> shimm;

  let Inst{31-27} = 0b00001;  // opcode 1
  let Inst{26-21} = a;
  let Inst{20-15} = b;
  let Inst{14}    = E;
  let Inst{13}    = 0;
  let Inst{12}    = W;
  let Inst{11-10} = Z;
  let Inst{9}     = X;
  let Inst{8-0}   = shimm;
}

// --- LD1 shimm,shimm: ld a, [shimm, shimm] ---  B = sentinel 63, no wb
class ARC4InstLD1_ss<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  let HasLdMod2 = 1;
  bits<6> a;
  bits<1> E;
  bits<2> Z;
  bits<1> X;
  bits<9> shimm;

  let Inst{31-27} = 0b00001;
  let Inst{26-21} = a;
  let Inst{20-15} = 0b111111;  // B = SHIMM sentinel
  let Inst{14}    = E;
  let Inst{13}    = 0;
  let Inst{12}    = 0;        // no writeback
  let Inst{11-10} = Z;
  let Inst{9}     = X;
  let Inst{8-0}   = shimm;
}

// --- LD1 limm: ld a, [limm] ---  B = sentinel 62, shimm = 0, no wb. Size = 8.
class ARC4InstLD1_l<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ARC4Inst8<outs, ins, asmstr, pattern> {
  let HasLdMod2 = 1;
  bits<6> a;
  bits<1> E;
  bits<2> Z;
  bits<1> X;

  let Inst{31-27} = 0b00001;
  let Inst{26-21} = a;
  let Inst{20-15} = 0b111110;  // B = LIMM sentinel
  let Inst{14}    = E;
  let Inst{13}    = 0;
  let Inst{12}    = 0;        // no writeback
  let Inst{11-10} = Z;
  let Inst{9}     = X;
  let Inst{8-0}   = 0;        // shimm = 0
}

//===----------------------------------------------------------------------===//
// Store format  (opcode 2)
//
// st c, [b, shimm]
// Fields: D[26]=cache bypass, bit25=0, v[24]=writeback, y[23:22]=size,
//         bit21=0, b[20:15]=base, c[14:9]=value reg, shimm[8:0]=offset
//
// Variants based on what is reg vs shimm vs limm in b and c positions.
//===----------------------------------------------------------------------===//

// --- ST reg, [reg, shimm]: st c, [b, offset] ---
class ARC4InstST_rrs<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  let HasStMod2 = 1;
  bits<1> D;   // cache bypass, bit 26
  bits<1> v;   // writeback, bit 24
  bits<2> y;   // size, bits 23:22
  bits<6> b;   // base register
  bits<6> c;   // value register
  bits<9> offset;

  let Inst{31-27} = 0b00010;  // opcode 2
  let Inst{26}    = D;
  let Inst{25}    = 0;
  let Inst{24}    = v;
  let Inst{23-22} = y;
  let Inst{21}    = 0;
  let Inst{20-15} = b;
  let Inst{14-9}  = c;
  let Inst{8-0}   = offset;
}

// --- ST shimm, [reg, shimm]: st shimm, [b, offset] --- C = sentinel 63
class ARC4InstST_srs<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  let HasStMod2 = 1;
  bits<1> D;
  bits<1> v;
  bits<2> y;
  bits<6> b;
  bits<9> offset;

  let Inst{31-27} = 0b00010;
  let Inst{26}    = D;
  let Inst{25}    = 0;
  let Inst{24}    = v;
  let Inst{23-22} = y;
  let Inst{21}    = 0;
  let Inst{20-15} = b;
  let Inst{14-9}  = 0b111111;  // C = SHIMM sentinel (value is shimm)
  let Inst{8-0}   = offset;
}

// --- ST reg, [shimm, shimm]: st c, [shimm, offset] --- B = sentinel 63
class ARC4InstST_rss<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  let HasStMod1 = 1;
  bits<1> D;
  bits<2> y;
  bits<6> c;
  bits<9> offset;

  let Inst{31-27} = 0b00010;
  let Inst{26}    = D;
  let Inst{25}    = 0;
  let Inst{24}    = 0;        // no writeback with shimm base
  let Inst{23-22} = y;
  let Inst{21}    = 0;
  let Inst{20-15} = 0b111111;  // B = SHIMM sentinel
  let Inst{14-9}  = c;
  let Inst{8-0}   = offset;
}

// --- ST shimm, [shimm, shimm]: all shimm --- B and C = sentinel 63
class ARC4InstST_sss<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  let HasStMod1 = 1;
  bits<1> D;
  bits<2> y;
  bits<9> offset;

  let Inst{31-27} = 0b00010;
  let Inst{26}    = D;
  let Inst{25}    = 0;
  let Inst{24}    = 0;
  let Inst{23-22} = y;
  let Inst{21}    = 0;
  let Inst{20-15} = 0b111111;
  let Inst{14-9}  = 0b111111;
  let Inst{8-0}   = offset;
}

// --- ST reg, [limm, shimm]: st c, [limm, offset] --- B = sentinel 62. Size=8.
class ARC4InstST_rls<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ARC4Inst8<outs, ins, asmstr, pattern> {
  let HasStMod1 = 1;
  bits<1> D;
  bits<2> y;
  bits<6> c;
  bits<9> offset;

  let Inst{31-27} = 0b00010;
  let Inst{26}    = D;
  let Inst{25}    = 0;
  let Inst{24}    = 0;        // no writeback with limm base
  let Inst{23-22} = y;
  let Inst{21}    = 0;
  let Inst{20-15} = 0b111110;  // B = LIMM sentinel
  let Inst{14-9}  = c;
  let Inst{8-0}   = offset;
}

// --- ST limm, [reg, shimm]: st limm, [b, offset] --- C = sentinel 62. Size=8.
class ARC4InstST_lrs<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ARC4Inst8<outs, ins, asmstr, pattern> {
  let HasStMod2 = 1;
  bits<1> D;
  bits<1> v;
  bits<2> y;
  bits<6> b;
  bits<9> offset;

  let Inst{31-27} = 0b00010;
  let Inst{26}    = D;
  let Inst{25}    = 0;
  let Inst{24}    = v;
  let Inst{23-22} = y;
  let Inst{21}    = 0;
  let Inst{20-15} = b;
  let Inst{14-9}  = 0b111110;  // C = LIMM sentinel (value is limm)
  let Inst{8-0}   = offset;
}

// --- ST limm, [limm, shimm]: both limm --- B and C = sentinel 62. Size=8.
class ARC4InstST_lls<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ARC4Inst8<outs, ins, asmstr, pattern> {
  let HasStMod1 = 1;
  bits<1> D;
  bits<2> y;
  bits<9> offset;

  let Inst{31-27} = 0b00010;
  let Inst{26}    = D;
  let Inst{25}    = 0;
  let Inst{24}    = 0;
  let Inst{23-22} = y;
  let Inst{21}    = 0;
  let Inst{20-15} = 0b111110;
  let Inst{14-9}  = 0b111110;
  let Inst{8-0}   = offset;
}

// --- ST shimm, [limm, shimm]: shimm value, limm base ---
// C = sentinel 63 (SHIMM, value is shimm), B = sentinel 62 (LIMM). Size=8.
// The shimm value serves as both the stored value and the offset.
// The assembler adjusts the limm: limm_encoded = address - shimm.
class ARC4InstST_sls<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ARC4Inst8<outs, ins, asmstr, pattern> {
  let HasStMod1 = 1;
  bits<1> D;
  bits<2> y;
  bits<9> offset;  // shimm value (= stored value = offset)

  let Inst{31-27} = 0b00010;
  let Inst{26}    = D;
  let Inst{25}    = 0;
  let Inst{24}    = 0;        // no writeback with limm base
  let Inst{23-22} = y;
  let Inst{21}    = 0;
  let Inst{20-15} = 0b111110;  // B = LIMM sentinel
  let Inst{14-9}  = 0b111111;  // C = SHIMM sentinel (value is shimm)
  let Inst{8-0}   = offset;
}

//===----------------------------------------------------------------------===//
// Flag instruction  (opcode 3, A = sentinel 61, C = 0)
//
// flag b  /  flag shimm  /  flag limm
//===----------------------------------------------------------------------===//

// --- flag reg ---
class ARC4InstFlag_r<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  bits<6> b;
  bits<5> q;   // condition code, wired to bits [4:0]

  let Inst{31-27} = 0b00011;  // opcode 3
  let Inst{26-21} = 0b111101; // A = sentinel 61 = SHIMM_UPDATE (discard)
  let Inst{20-15} = b;
  let Inst{14-9}  = 0;        // C = 0
  let Inst{8-5}   = 0;
  let Inst{4-0}   = q;
}

// --- flag shimm ---
class ARC4InstFlag_s<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  bits<9> shimm;

  let Inst{31-27} = 0b00011;
  let Inst{26-21} = 0b111101; // A = sentinel 61
  let Inst{20-15} = 0b111111; // B = SHIMM sentinel
  let Inst{14-9}  = 0;        // C = 0
  let Inst{8-0}   = shimm;
}

// --- flag limm --- Size = 8.
class ARC4InstFlag_l<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ARC4Inst8<outs, ins, asmstr, pattern> {
  bits<5> q;   // condition code, wired to bits [4:0]

  let Inst{31-27} = 0b00011;
  let Inst{26-21} = 0b111101; // A = sentinel 61
  let Inst{20-15} = 0b111110; // B = LIMM sentinel
  let Inst{14-9}  = 0;        // C = 0
  let Inst{8-5}   = 0;
  let Inst{4-0}   = q;
}
