# ARC4 Full Base ISA Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Complete the ARC4 base instruction set: shimm/limm operand encoding for all instruction types, instruction modifiers (.f, .q, delay slots), load/store, branch/jump, and flag instruction.

**Architecture:** Bottom-up approach. First build the encoding infrastructure (operand types, format classes, MCCodeEmitter shimm/limm logic, AsmParser modifier parsing), then define instruction multiclasses that use that infrastructure, then add tests. Each instruction family (ALU, SOP, load, store, branch/jump, flag) is a separate task.

**Tech Stack:** LLVM TableGen, MC framework (MCCodeEmitter, MCAsmBackend, AsmParser, InstPrinter)

**Reference implementations (cross-reference ALL encodings against these):**
- Binutils opcode table: `/home/tomato/Downloads/Obsolete_Arc-gnu-tools-src-20060612/arc-gnu-tools-src-20060612/binutils-2.15/opcodes/arc-opc.c`
- Architecture spec: `/home/tomato/projects/arctangent-a4-llvm/docs/ARC4_Programmers_reference.html`
- Design spec: `/home/tomato/projects/llvm-project/docs/superpowers/specs/2026-03-28-arc4-full-isa-design.md`

**Key encoding rules:**
- Sentinel 61 in register field = shimm with .f (flag update)
- Sentinel 62 in register field = limm (32-bit word follows instruction)
- Sentinel 63 in register field = shimm without .f
- shimm is 9-bit signed in bits [8:0], overlaps condition code bits [4:0]
- .f without shimm uses bit [8]; with shimm uses sentinel 61 vs 63
- .q condition codes NOT available when shimm present

---

## Task 1: Operand Types and Instruction Format Classes

**Files:**
- Modify: `llvm/lib/Target/ARC4/ARC4InstrFormats.td`
- Modify: `llvm/lib/Target/ARC4/ARC4RegisterInfo.td`

This task defines the TableGen operand types and instruction format classes needed by all subsequent tasks.

- [ ] **Step 1: Add shimm and limm operand types to ARC4InstrFormats.td**

Add after the existing format classes in `ARC4InstrFormats.td`:

```tablegen
//===----------------------------------------------------------------------===//
// Operand types
//===----------------------------------------------------------------------===//

// 9-bit signed immediate (shimm) — encoded in bits [8:0]
def shimm9 : Operand<i32> {
  let PrintMethod = "printOperand";
  let ParserMatchClass = ImmAsmOperand<"S9">;
}

// Predicate: fits in 9-bit signed
def Simm9 : ImmLeaf<i32, [{return isInt<9>(Imm);}]>;

// 32-bit immediate (limm) — encoded in extra word after instruction
def limm32 : Operand<i32> {
  let PrintMethod = "printOperand";
}

// Condition code operand (5-bit, bits [4:0])
def condcode : Operand<i32> {
  let PrintMethod = "printCondCode";
}

// Memory operand for load/store: [base, offset]
def memsrc : Operand<i32> {
  let PrintMethod = "printMemOperand";
  let MIOperandInfo = (ops GPR32, GPR32);
}

// Memory operand: [base, shimm]
def memshimm : Operand<i32> {
  let PrintMethod = "printMemShimmOperand";
  let MIOperandInfo = (ops GPR32, shimm9);
}

// Memory operand: [base, limm]
def memlimm : Operand<i32> {
  let PrintMethod = "printMemLimmOperand";
  let MIOperandInfo = (ops GPR32, limm32);
}
```

- [ ] **Step 2: Add ALU3 format classes for all operand combinations**

Add to `ARC4InstrFormats.td`:

```tablegen
//===----------------------------------------------------------------------===//
// 3-operand ALU formats with modifiers
//===----------------------------------------------------------------------===//

// ALU3 reg, reg, reg — with .f (bit 8) and .q (bits [4:0]) support
class ARC4InstALU3_rrr<bits<5> opcode, dag outs, dag ins, string asmstr,
                        list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  bits<6> a;
  bits<6> b;
  bits<6> c;
  bits<1> f;   // flag bit
  bits<5> q;   // condition code

  let Inst{31-27} = opcode;
  let Inst{26-21} = a;
  let Inst{20-15} = b;
  let Inst{14-9}  = c;
  let Inst{8}     = f;
  let Inst{7-5}   = 0;
  let Inst{4-0}   = q;
}

// ALU3 reg, reg, shimm — sentinel in C field, shimm in [8:0]
// .f encoded via sentinel (61 vs 63), NO .q (bits occupied by shimm)
class ARC4InstALU3_rrs<bits<5> opcode, dag outs, dag ins, string asmstr,
                        list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  bits<6> a;
  bits<6> b;
  bits<9> imm;  // shimm value
  bits<1> f;    // flag — selects sentinel 61 vs 63

  let Inst{31-27} = opcode;
  let Inst{26-21} = a;
  let Inst{20-15} = b;
  // C field: sentinel 61 (0b111101) if f=1, else 63 (0b111111)
  let Inst{14}    = 1;
  let Inst{13}    = 1;
  let Inst{12}    = 1;
  let Inst{11}    = 1;
  let Inst{10}    = f;
  let Inst{9}     = !not(f);
  let Inst{8-0}   = imm;
}

// ALU3 reg, shimm, reg — sentinel in B field, shimm in [8:0]
class ARC4InstALU3_rsr<bits<5> opcode, dag outs, dag ins, string asmstr,
                        list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  bits<6> a;
  bits<9> imm;  // shimm value (goes in bits [8:0])
  bits<6> c;
  bits<1> f;

  let Inst{31-27} = opcode;
  let Inst{26-21} = a;
  // B field: sentinel 61/63
  let Inst{20}    = 1;
  let Inst{19}    = 1;
  let Inst{18}    = 1;
  let Inst{17}    = 1;
  let Inst{16}    = f;
  let Inst{15}    = !not(f);
  let Inst{14-9}  = c;
  let Inst{8-0}   = imm;
}

// ALU3 reg, shimm, shimm — sentinel in B and C fields, shimm in [8:0]
// Both shimms share the same bits [8:0] (must match)
class ARC4InstALU3_rss<bits<5> opcode, dag outs, dag ins, string asmstr,
                        list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  bits<6> a;
  bits<9> imm;  // shared shimm value
  bits<1> f;

  let Inst{31-27} = opcode;
  let Inst{26-21} = a;
  // B field: sentinel 61/63
  let Inst{20}    = 1;
  let Inst{19}    = 1;
  let Inst{18}    = 1;
  let Inst{17}    = 1;
  let Inst{16}    = f;
  let Inst{15}    = !not(f);
  // C field: sentinel 61/63 (same as B)
  let Inst{14}    = 1;
  let Inst{13}    = 1;
  let Inst{12}    = 1;
  let Inst{11}    = 1;
  let Inst{10}    = f;
  let Inst{9}     = !not(f);
  let Inst{8-0}   = imm;
}

// ALU3 reg, reg, limm — sentinel 62 in C field
// .f (bit 8), .q (bits [4:0]) available
class ARC4InstALU3_rrl<bits<5> opcode, dag outs, dag ins, string asmstr,
                        list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  bits<6> a;
  bits<6> b;
  bits<1> f;
  bits<5> q;

  let Inst{31-27} = opcode;
  let Inst{26-21} = a;
  let Inst{20-15} = b;
  let Inst{14-9}  = 0b111110; // sentinel 62 = LIMM
  let Inst{8}     = f;
  let Inst{7-5}   = 0;
  let Inst{4-0}   = q;
  let Size = 8; // instruction + limm word
}

// ALU3 reg, limm, reg — sentinel 62 in B field
class ARC4InstALU3_rlr<bits<5> opcode, dag outs, dag ins, string asmstr,
                        list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  bits<6> a;
  bits<6> c;
  bits<1> f;
  bits<5> q;

  let Inst{31-27} = opcode;
  let Inst{26-21} = a;
  let Inst{20-15} = 0b111110; // sentinel 62 = LIMM
  let Inst{14-9}  = c;
  let Inst{8}     = f;
  let Inst{7-5}   = 0;
  let Inst{4-0}   = q;
  let Size = 8;
}
```

Note: The "discard" variants (0rr, 0rs, etc.) use the same format classes but with the A field set to the shimm sentinel. These will be defined as separate instruction records in the multiclass with `let a = sentinel_value`.

- [ ] **Step 3: Add single-operand format classes**

Add to `ARC4InstrFormats.td`:

```tablegen
//===----------------------------------------------------------------------===//
// Single-operand (2-operand) formats
//===----------------------------------------------------------------------===//

// SOP reg, reg — with .f and .q
class ARC4InstSOP_rr<bits<6> subop, dag outs, dag ins, string asmstr,
                     list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  bits<6> a;
  bits<6> b;
  bits<1> f;
  bits<5> q;

  let Inst{31-27} = 0b00011;
  let Inst{26-21} = a;
  let Inst{20-15} = b;
  let Inst{14-9}  = subop;
  let Inst{8}     = f;
  let Inst{7-5}   = 0;
  let Inst{4-0}   = q;
}

// SOP reg, shimm — sentinel in B, shimm in [8:0], no .q
class ARC4InstSOP_rs<bits<6> subop, dag outs, dag ins, string asmstr,
                     list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  bits<6> a;
  bits<9> imm;
  bits<1> f;

  let Inst{31-27} = 0b00011;
  let Inst{26-21} = a;
  // B field: sentinel 61/63
  let Inst{20}    = 1;
  let Inst{19}    = 1;
  let Inst{18}    = 1;
  let Inst{17}    = 1;
  let Inst{16}    = f;
  let Inst{15}    = !not(f);
  let Inst{14-9}  = subop;
  let Inst{8-0}   = imm;
}

// SOP reg, limm — sentinel 62 in B, .f and .q available
class ARC4InstSOP_rl<bits<6> subop, dag outs, dag ins, string asmstr,
                     list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  bits<6> a;
  bits<1> f;
  bits<5> q;

  let Inst{31-27} = 0b00011;
  let Inst{26-21} = a;
  let Inst{20-15} = 0b111110; // LIMM sentinel
  let Inst{14-9}  = subop;
  let Inst{8}     = f;
  let Inst{7-5}   = 0;
  let Inst{4-0}   = q;
  let Size = 8;
}
```

- [ ] **Step 4: Add branch/jump format classes**

Add to `ARC4InstrFormats.td`:

```tablegen
//===----------------------------------------------------------------------===//
// Branch/Jump formats
//===----------------------------------------------------------------------===//

// Branch: b/bl/lp — 20-bit PC-relative offset
class ARC4InstBranch<bits<5> opcode, dag outs, dag ins, string asmstr,
                     list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  bits<20> offset; // 20-bit signed word-aligned PC-relative
  bits<2>  n;      // delay slot mode
  bits<5>  q;      // condition code

  let Inst{31-27} = opcode;
  let Inst{26-7}  = offset;
  let Inst{6-5}   = n;
  let Inst{4-0}   = q;

  let isBranch = 1;
  let isTerminator = 1;
}

// Jump register: j/jl [reg]
class ARC4InstJumpReg<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  bits<6>  b;     // target register
  bits<1>  link;  // 0=j, 1=jl
  bits<1>  f;     // flag update
  bits<2>  n;     // delay slot mode
  bits<5>  q;     // condition code

  let Inst{31-27} = 0b00111;
  let Inst{26-21} = 0;
  let Inst{20-15} = b;
  let Inst{14-10} = 0;
  let Inst{9}     = link;
  let Inst{8}     = f;
  let Inst{7}     = 0;
  let Inst{6-5}   = n;
  let Inst{4-0}   = q;
}

// Jump limm: j/jl limm
class ARC4InstJumpLimm<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  bits<1>  link;
  bits<1>  f;
  bits<2>  n;
  bits<5>  q;

  let Inst{31-27} = 0b00111;
  let Inst{26-21} = 0;
  let Inst{20-15} = 0b111110; // LIMM sentinel
  let Inst{14-10} = 0;
  let Inst{9}     = link;
  let Inst{8}     = f;
  let Inst{7}     = 0;
  let Inst{6-5}   = n;
  let Inst{4-0}   = q;
  let Size = 8;
}
```

- [ ] **Step 5: Add load format classes**

Add to `ARC4InstrFormats.td`:

```tablegen
//===----------------------------------------------------------------------===//
// Load formats
//===----------------------------------------------------------------------===//

// Load opcode 0: ld a, [b, c] (register offset)
class ARC4InstLD0_rr<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  bits<6> a;   // dest
  bits<6> b;   // base
  bits<6> c;   // offset register
  bits<1> e;   // cache bypass
  bits<1> w;   // writeback
  bits<2> z;   // size (00=word, 01=byte, 10=half)
  bits<1> x;   // sign extend

  let Inst{31-27} = 0b00000;
  let Inst{26-21} = a;
  let Inst{20-15} = b;
  let Inst{14-9}  = c;
  let Inst{8-6}   = 0;
  let Inst{5}     = e;
  let Inst{4}     = 0;
  let Inst{3}     = w;
  let Inst{2-1}   = z;
  let Inst{0}     = x;

  let mayLoad = 1;
}

// Load opcode 0: ld a, [b, limm] — sentinel 62 in C field
class ARC4InstLD0_rl<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  bits<6> a;
  bits<6> b;
  bits<1> e;
  bits<1> w;
  bits<2> z;
  bits<1> x;

  let Inst{31-27} = 0b00000;
  let Inst{26-21} = a;
  let Inst{20-15} = b;
  let Inst{14-9}  = 0b111110; // LIMM
  let Inst{8-6}   = 0;
  let Inst{5}     = e;
  let Inst{4}     = 0;
  let Inst{3}     = w;
  let Inst{2-1}   = z;
  let Inst{0}     = x;
  let Size = 8;
  let mayLoad = 1;
}

// Load opcode 0: ld a, [limm, c] — sentinel 62 in B field
class ARC4InstLD0_lr<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  bits<6> a;
  bits<6> c;
  bits<1> e;
  bits<2> z;
  bits<1> x;

  let Inst{31-27} = 0b00000;
  let Inst{26-21} = a;
  let Inst{20-15} = 0b111110; // LIMM
  let Inst{14-9}  = c;
  let Inst{8-6}   = 0;
  let Inst{5}     = e;
  let Inst{4}     = 0;
  let Inst{3}     = 0; // no writeback (limm base)
  let Inst{2-1}   = z;
  let Inst{0}     = x;
  let Size = 8;
  let mayLoad = 1;
}

// Load opcode 1: ld a, [b, shimm]
class ARC4InstLD1_rs<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  bits<6> a;
  bits<6> b;
  bits<9> imm;  // shimm offset
  bits<1> E;    // cache bypass
  bits<1> W;    // writeback
  bits<2> Z;    // size
  bits<1> X;    // sign extend

  let Inst{31-27} = 0b00001;
  let Inst{26-21} = a;
  let Inst{20-15} = b;
  let Inst{14}    = E;
  let Inst{13}    = 0;
  let Inst{12}    = W;
  let Inst{11-10} = Z;
  let Inst{9}     = X;
  let Inst{8-0}   = imm;
  let mayLoad = 1;
}

// Load opcode 1: ld a, [shimm, shimm] — sentinels in B, shimm in [8:0]
class ARC4InstLD1_ss<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  bits<6> a;
  bits<9> imm;
  bits<1> E;
  bits<2> Z;
  bits<1> X;

  let Inst{31-27} = 0b00001;
  let Inst{26-21} = a;
  // B field: shimm sentinel 63 (no flag on loads)
  let Inst{20-15} = 0b111111;
  let Inst{14}    = E;
  let Inst{13}    = 0;
  let Inst{12}    = 0; // no writeback
  let Inst{11-10} = Z;
  let Inst{9}     = X;
  let Inst{8-0}   = imm;
  let mayLoad = 1;
}

// Load opcode 1: ld a, [limm] — sentinel 62 in B, shimm=0
class ARC4InstLD1_l<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  bits<6> a;
  bits<1> E;
  bits<2> Z;
  bits<1> X;

  let Inst{31-27} = 0b00001;
  let Inst{26-21} = a;
  let Inst{20-15} = 0b111110; // LIMM sentinel
  let Inst{14}    = E;
  let Inst{13}    = 0;
  let Inst{12}    = 0; // no writeback
  let Inst{11-10} = Z;
  let Inst{9}     = X;
  let Inst{8-0}   = 0; // shimm = 0
  let Size = 8;
  let mayLoad = 1;
}
```

- [ ] **Step 6: Add store format classes**

Add to `ARC4InstrFormats.td`:

```tablegen
//===----------------------------------------------------------------------===//
// Store formats — opcode 2, shimm always in [8:0]
//===----------------------------------------------------------------------===//

// Store: st reg, [reg, shimm]
class ARC4InstST_rrs<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  bits<6> c;    // value register
  bits<6> b;    // base register
  bits<9> imm;  // shimm offset
  bits<1> D;    // cache bypass
  bits<1> v;    // writeback
  bits<2> y;    // size

  let Inst{31-27} = 0b00010;
  let Inst{26}    = D;
  let Inst{25}    = 0;
  let Inst{24}    = v;
  let Inst{23-22} = y;
  let Inst{21}    = 0;
  let Inst{20-15} = b;
  let Inst{14-9}  = c;
  let Inst{8-0}   = imm;
  let mayStore = 1;
}

// Store: st reg, [limm] — B=62(limm), shimm in offset
class ARC4InstST_rl<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  bits<6> c;
  bits<9> imm;
  bits<1> D;
  bits<2> y;

  let Inst{31-27} = 0b00010;
  let Inst{26}    = D;
  let Inst{25}    = 0;
  let Inst{24}    = 0; // no writeback
  let Inst{23-22} = y;
  let Inst{21}    = 0;
  let Inst{20-15} = 0b111110; // LIMM
  let Inst{14-9}  = c;
  let Inst{8-0}   = imm;
  let Size = 8;
  let mayStore = 1;
}

// Store: st reg, [shimm, shimm] — B=sentinel, shimm offset must match
class ARC4InstST_rss<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  bits<6> c;
  bits<9> imm;
  bits<1> D;
  bits<2> y;

  let Inst{31-27} = 0b00010;
  let Inst{26}    = D;
  let Inst{25}    = 0;
  let Inst{24}    = 0; // no writeback
  let Inst{23-22} = y;
  let Inst{21}    = 0;
  let Inst{20-15} = 0b111111; // SHIMM sentinel (no flag on stores)
  let Inst{14-9}  = c;
  let Inst{8-0}   = imm;
  let mayStore = 1;
}

// Store: st shimm, [reg, shimm] — C=sentinel, shimm shared (must match)
class ARC4InstST_srs<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  bits<6> b;
  bits<9> imm;  // shared shimm (value AND offset)
  bits<1> D;
  bits<1> v;
  bits<2> y;

  let Inst{31-27} = 0b00010;
  let Inst{26}    = D;
  let Inst{25}    = 0;
  let Inst{24}    = v;
  let Inst{23-22} = y;
  let Inst{21}    = 0;
  let Inst{20-15} = b;
  let Inst{14-9}  = 0b111111; // SHIMM sentinel
  let Inst{8-0}   = imm;
  let mayStore = 1;
}

// Store: st shimm, [limm] — C=shimm sentinel, B=limm sentinel
// Assembler must adjust limm so limm+shimm = intended address
class ARC4InstST_sl<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  bits<9> imm;
  bits<1> D;
  bits<2> y;

  let Inst{31-27} = 0b00010;
  let Inst{26}    = D;
  let Inst{25}    = 0;
  let Inst{24}    = 0; // no writeback
  let Inst{23-22} = y;
  let Inst{21}    = 0;
  let Inst{20-15} = 0b111110; // LIMM
  let Inst{14-9}  = 0b111111; // SHIMM
  let Inst{8-0}   = imm;
  let Size = 8;
  let mayStore = 1;
}

// Store: st limm, [reg, shimm] — C=62(limm)
class ARC4InstST_lrs<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  bits<6> b;
  bits<9> imm;
  bits<1> D;
  bits<1> v;
  bits<2> y;

  let Inst{31-27} = 0b00010;
  let Inst{26}    = D;
  let Inst{25}    = 0;
  let Inst{24}    = v;
  let Inst{23-22} = y;
  let Inst{21}    = 0;
  let Inst{20-15} = b;
  let Inst{14-9}  = 0b111110; // LIMM
  let Inst{8-0}   = imm;
  let Size = 8;
  let mayStore = 1;
}

// Store: st limm, [shimm, shimm] — C=62(limm), B=shimm sentinel
class ARC4InstST_lss<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  bits<9> imm;
  bits<1> D;
  bits<2> y;

  let Inst{31-27} = 0b00010;
  let Inst{26}    = D;
  let Inst{25}    = 0;
  let Inst{24}    = 0; // no writeback
  let Inst{23-22} = y;
  let Inst{21}    = 0;
  let Inst{20-15} = 0b111111; // SHIMM
  let Inst{14-9}  = 0b111110; // LIMM
  let Inst{8-0}   = imm;
  let Size = 8;
  let mayStore = 1;
}
```

- [ ] **Step 7: Add flag instruction format**

Add to `ARC4InstrFormats.td`:

```tablegen
//===----------------------------------------------------------------------===//
// Flag instruction format
//===----------------------------------------------------------------------===//

// flag reg — opcode 3, A=61, C=0
class ARC4InstFlag_r<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  bits<6> b;
  bits<5> q;

  let Inst{31-27} = 0b00011;
  let Inst{26-21} = 0b111101; // A = 61 (SHIMM_UPDATE)
  let Inst{20-15} = b;
  let Inst{14-9}  = 0;
  let Inst{8-5}   = 0;
  let Inst{4-0}   = q;
}

// flag shimm — A=61, B=shimm sentinel, no .q
class ARC4InstFlag_s<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  bits<9> imm;

  let Inst{31-27} = 0b00011;
  let Inst{26-21} = 0b111101; // A = 61
  let Inst{20-15} = 0b111111; // SHIMM sentinel
  let Inst{14-9}  = 0;
  let Inst{8-0}   = imm;
}

// flag limm — A=61, B=62(LIMM), .q available
class ARC4InstFlag_l<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  bits<5> q;

  let Inst{31-27} = 0b00011;
  let Inst{26-21} = 0b111101; // A = 61
  let Inst{20-15} = 0b111110; // LIMM sentinel
  let Inst{14-9}  = 0;
  let Inst{8-5}   = 0;
  let Inst{4-0}   = q;
  let Size = 8;
}
```

- [ ] **Step 8: Build tablegen to verify format classes compile**

Run:
```bash
cd /home/tomato/projects/llvm-project
cmake --build build --target ARC4CommonTableGen 2>&1 | tail -20
```
Expected: Clean tablegen pass.

- [ ] **Step 9: Commit**

```bash
git add llvm/lib/Target/ARC4/ARC4InstrFormats.td
git commit -m "feat(ARC4): add format classes for all instruction operand combinations"
```

---

## Task 2: 3-Operand ALU Multiclass (12 variants)

**Files:**
- Modify: `llvm/lib/Target/ARC4/ARC4InstrInfo.td`

Replace the existing `ALU3` multiclass with one generating all 12 variants.

- [ ] **Step 1: Replace ALU3 multiclass and instruction definitions**

Replace the existing ALU3 section in `ARC4InstrInfo.td` with:

```tablegen
//===----------------------------------------------------------------------===//
// 3-Operand ALU Instructions — 12 variants per instruction
//===----------------------------------------------------------------------===//

multiclass ALU3<bits<5> opcode, string mnemonic> {
  // --- With result (A = register) ---

  // 1. rrr: reg, reg, reg — .f and .q available
  def _rrr : ARC4InstALU3_rrr<opcode,
    (outs GPR32:$a), (ins GPR32:$b, GPR32:$c),
    mnemonic # "\t$a, $b, $c", []>;

  // 2. rrs: reg, reg, shimm — .f via sentinel, no .q
  def _rrs : ARC4InstALU3_rrs<opcode,
    (outs GPR32:$a), (ins GPR32:$b, shimm9:$imm),
    mnemonic # "\t$a, $b, $imm", []>;

  // 3. rsr: reg, shimm, reg — .f via sentinel, no .q
  def _rsr : ARC4InstALU3_rsr<opcode,
    (outs GPR32:$a), (ins shimm9:$imm, GPR32:$c),
    mnemonic # "\t$a, $imm, $c", []>;

  // 4. rss: reg, shimm, shimm — .f via sentinel, no .q (shimms must match)
  def _rss : ARC4InstALU3_rss<opcode,
    (outs GPR32:$a), (ins shimm9:$imm),
    mnemonic # "\t$a, $imm, $imm", []>;

  // 5. rrl: reg, reg, limm — .f and .q available
  def _rrl : ARC4InstALU3_rrl<opcode,
    (outs GPR32:$a), (ins GPR32:$b, limm32:$imm),
    mnemonic # "\t$a, $b, $imm", []>;

  // 6. rlr: reg, limm, reg — .f and .q available
  def _rlr : ARC4InstALU3_rlr<opcode,
    (outs GPR32:$a), (ins limm32:$imm, GPR32:$c),
    mnemonic # "\t$a, $imm, $c", []>;

  // --- Without result (A = discard) ---

  // 7. 0rr: discard, reg, reg
  let a = 0b111101 in // sentinel 61 in A field = discard
  def _0rr : ARC4InstALU3_rrr<opcode,
    (outs), (ins GPR32:$b, GPR32:$c),
    mnemonic # "\t0, $b, $c", []>;

  // 8. 0rs: discard, reg, shimm
  let a = 0b111101 in
  def _0rs : ARC4InstALU3_rrs<opcode,
    (outs), (ins GPR32:$b, shimm9:$imm),
    mnemonic # "\t0, $b, $imm", []>;

  // 9. 0sr: discard, shimm, reg
  let a = 0b111101 in
  def _0sr : ARC4InstALU3_rsr<opcode,
    (outs), (ins shimm9:$imm, GPR32:$c),
    mnemonic # "\t0, $imm, $c", []>;

  // 10. 0ss: discard, shimm, shimm
  let a = 0b111101 in
  def _0ss : ARC4InstALU3_rss<opcode,
    (outs), (ins shimm9:$imm),
    mnemonic # "\t0, $imm, $imm", []>;

  // 11. 0rl: discard, reg, limm
  let a = 0b111101 in
  def _0rl : ARC4InstALU3_rrl<opcode,
    (outs), (ins GPR32:$b, limm32:$imm),
    mnemonic # "\t0, $b, $imm", []>;

  // 12. 0lr: discard, limm, reg
  let a = 0b111101 in
  def _0lr : ARC4InstALU3_rlr<opcode,
    (outs), (ins limm32:$imm, GPR32:$c),
    mnemonic # "\t0, $imm, $c", []>;
}

defm ADD : ALU3<0b01000, "add">;
defm ADC : ALU3<0b01001, "adc">;
defm SUB : ALU3<0b01010, "sub">;
defm SBC : ALU3<0b01011, "sbc">;
defm AND : ALU3<0b01100, "and">;
defm OR  : ALU3<0b01101, "or">;
defm BIC : ALU3<0b01110, "bic">;
defm XOR : ALU3<0b01111, "xor">;
```

- [ ] **Step 2: Update MOV alias**

Update the alias section:

```tablegen
//===----------------------------------------------------------------------===//
// Aliases
//===----------------------------------------------------------------------===//

// mov a, b => and a, b, b (rrr form — reg source)
def : InstAlias<"mov\t$a, $b", (AND_rrr GPR32:$a, GPR32:$b, GPR32:$b)>;

// mov a, shimm => and a, shimm, shimm (rss form — shimm source)
def : InstAlias<"mov\t$a, $imm", (AND_rss GPR32:$a, shimm9:$imm)>;
```

- [ ] **Step 3: Build and verify**

Run:
```bash
cmake --build build --target ARC4CommonTableGen 2>&1 | tail -20
cmake --build build --target llvm-mc 2>&1 | tail -20
```
Expected: Clean build.

- [ ] **Step 4: Commit**

```bash
git add llvm/lib/Target/ARC4/ARC4InstrInfo.td
git commit -m "feat(ARC4): add all 12 ALU3 variants with shimm/limm/discard operands"
```

---

## Task 3: Single-Operand Multiclass (6 variants)

**Files:**
- Modify: `llvm/lib/Target/ARC4/ARC4InstrInfo.td`

- [ ] **Step 1: Replace single-operand instruction definitions**

Replace the single-op section in `ARC4InstrInfo.td`:

```tablegen
//===----------------------------------------------------------------------===//
// Single-Operand Instructions — 6 variants per instruction
//===----------------------------------------------------------------------===//

multiclass SOP<bits<6> subop, string mnemonic> {
  // 1. rr: reg, reg — .f and .q available
  def _rr : ARC4InstSOP_rr<subop,
    (outs GPR32:$a), (ins GPR32:$b),
    mnemonic # "\t$a, $b", []>;

  // 2. rs: reg, shimm — .f via sentinel, no .q
  def _rs : ARC4InstSOP_rs<subop,
    (outs GPR32:$a), (ins shimm9:$imm),
    mnemonic # "\t$a, $imm", []>;

  // 3. rl: reg, limm — .f and .q available
  def _rl : ARC4InstSOP_rl<subop,
    (outs GPR32:$a), (ins limm32:$imm),
    mnemonic # "\t$a, $imm", []>;

  // 4. 0r: discard, reg
  let a = 0b111101 in
  def _0r : ARC4InstSOP_rr<subop,
    (outs), (ins GPR32:$b),
    mnemonic # "\t0, $b", []>;

  // 5. 0s: discard, shimm
  let a = 0b111101 in
  def _0s : ARC4InstSOP_rs<subop,
    (outs), (ins shimm9:$imm),
    mnemonic # "\t0, $imm", []>;

  // 6. 0l: discard, limm
  let a = 0b111101 in
  def _0l : ARC4InstSOP_rl<subop,
    (outs), (ins limm32:$imm),
    mnemonic # "\t0, $imm", []>;
}

defm ASL  : SOP<0b000000, "asl">;
defm ASR  : SOP<0b000001, "asr">;
defm LSR  : SOP<0b000010, "lsr">;
defm ROR  : SOP<0b000011, "ror">;
defm RRC  : SOP<0b000100, "rrc">;
defm SEXB : SOP<0b000101, "sexb">;
defm SEXW : SOP<0b000110, "sexw">;
defm EXTB : SOP<0b000111, "extb">;
defm EXTW : SOP<0b001000, "extw">;
```

- [ ] **Step 2: Build and verify**

```bash
cmake --build build --target ARC4CommonTableGen 2>&1 | tail -20
cmake --build build --target llvm-mc 2>&1 | tail -20
```

- [ ] **Step 3: Commit**

```bash
git add llvm/lib/Target/ARC4/ARC4InstrInfo.td
git commit -m "feat(ARC4): add all 6 single-operand variants with shimm/limm/discard"
```

---

## Task 4: Load, Store, Branch/Jump, Flag Instruction Definitions

**Files:**
- Modify: `llvm/lib/Target/ARC4/ARC4InstrInfo.td`

- [ ] **Step 1: Add load instruction definitions**

Add to `ARC4InstrInfo.td`:

```tablegen
//===----------------------------------------------------------------------===//
// Load Instructions — 6 variants × 3 sizes
//===----------------------------------------------------------------------===//

multiclass LD<string suffix, bits<2> sz> {
  // Opcode 0 variants (no shimm)
  def _rr#suffix : ARC4InstLD0_rr<(outs GPR32:$a), (ins GPR32:$b, GPR32:$c),
    "ld" # suffix # "\t$a, [$b, $c]", []> { let z = sz; }
  def _rl#suffix : ARC4InstLD0_rl<(outs GPR32:$a), (ins GPR32:$b, limm32:$imm),
    "ld" # suffix # "\t$a, [$b, $imm]", []> { let z = sz; }
  def _lr#suffix : ARC4InstLD0_lr<(outs GPR32:$a), (ins limm32:$imm, GPR32:$c),
    "ld" # suffix # "\t$a, [$imm, $c]", []> { let z = sz; }

  // Opcode 1 variants (shimm)
  def _rs#suffix : ARC4InstLD1_rs<(outs GPR32:$a), (ins GPR32:$b, shimm9:$imm),
    "ld" # suffix # "\t$a, [$b, $imm]", []> { let Z = sz; }
  def _ss#suffix : ARC4InstLD1_ss<(outs GPR32:$a), (ins shimm9:$imm),
    "ld" # suffix # "\t$a, [$imm]", []> { let Z = sz; }
  def _l#suffix  : ARC4InstLD1_l<(outs GPR32:$a), (ins limm32:$imm),
    "ld" # suffix # "\t$a, [$imm]", []> { let Z = sz; }
}

defm LD  : LD<"", 0b00>;    // word
defm LDB : LD<"b", 0b01>;   // byte
defm LDW : LD<"w", 0b10>;   // halfword

// Alias: ld a, [b] => ld a, [b, 0]
def : InstAlias<"ld\t$a, [$b]", (LD_rs GPR32:$a, GPR32:$b, 0)>;
def : InstAlias<"ldb\t$a, [$b]", (LDB_rsb GPR32:$a, GPR32:$b, 0)>;
def : InstAlias<"ldw\t$a, [$b]", (LDW_rsw GPR32:$a, GPR32:$b, 0)>;
```

Note: The exact alias names depend on how the multiclass names expand. The subagent implementing this should verify the generated names and adjust aliases accordingly. The `.x`, `.a`, `.di` modifiers will be handled by the AsmParser (Task 5) which maps them to the appropriate format class fields.

- [ ] **Step 2: Add store instruction definitions**

```tablegen
//===----------------------------------------------------------------------===//
// Store Instructions — 7 variants × 3 sizes
//===----------------------------------------------------------------------===//

multiclass ST<string suffix, bits<2> sz> {
  // 1. reg, [reg, shimm]
  def _rrs#suffix : ARC4InstST_rrs<(outs),
    (ins GPR32:$c, GPR32:$b, shimm9:$imm),
    "st" # suffix # "\t$c, [$b, $imm]", []> { let y = sz; }

  // 2. reg, [limm]
  def _rl#suffix : ARC4InstST_rl<(outs),
    (ins GPR32:$c, limm32:$addr, shimm9:$imm),
    "st" # suffix # "\t$c, [$addr]", []> { let y = sz; }

  // 3. reg, [shimm, shimm]
  def _rss#suffix : ARC4InstST_rss<(outs),
    (ins GPR32:$c, shimm9:$imm),
    "st" # suffix # "\t$c, [$imm]", []> { let y = sz; }

  // 4. shimm, [reg, shimm] (must match)
  def _srs#suffix : ARC4InstST_srs<(outs),
    (ins shimm9:$imm, GPR32:$b),
    "st" # suffix # "\t$imm, [$b, $imm]", []> { let y = sz; }

  // 5. shimm, [limm] (encoded as [limm, shimm])
  def _sl#suffix : ARC4InstST_sl<(outs),
    (ins shimm9:$imm, limm32:$addr),
    "st" # suffix # "\t$imm, [$addr]", []> { let y = sz; }

  // 6. limm, [reg, shimm]
  def _lrs#suffix : ARC4InstST_lrs<(outs),
    (ins limm32:$val, GPR32:$b, shimm9:$imm),
    "st" # suffix # "\t$val, [$b, $imm]", []> { let y = sz; }

  // 7. limm, [shimm, shimm]
  def _lss#suffix : ARC4InstST_lss<(outs),
    (ins limm32:$val, shimm9:$imm),
    "st" # suffix # "\t$val, [$imm]", []> { let y = sz; }
}

defm ST  : ST<"", 0b00>;
defm STB : ST<"b", 0b01>;
defm STW : ST<"w", 0b10>;

// Alias: st c, [b] => st c, [b, 0]
def : InstAlias<"st\t$c, [$b]", (ST_rrs GPR32:$c, GPR32:$b, 0)>;
```

- [ ] **Step 3: Add branch and jump instruction definitions**

```tablegen
//===----------------------------------------------------------------------===//
// Branch Instructions — b (opcode 4), bl (opcode 5), lp (opcode 6)
//===----------------------------------------------------------------------===//

let isBranch = 1, isTerminator = 1 in {
def B  : ARC4InstBranch<0b00100, (outs), (ins i32imm:$offset),
  "b\t$offset", []>;

def BL : ARC4InstBranch<0b00101, (outs), (ins i32imm:$offset),
  "bl\t$offset", []> {
  let isCall = 1;
}

def LP : ARC4InstBranch<0b00110, (outs), (ins i32imm:$offset),
  "lp\t$offset", []>;
}

//===----------------------------------------------------------------------===//
// Jump Instructions — j/jl (opcode 7)
//===----------------------------------------------------------------------===//

// j [reg]
def J_r : ARC4InstJumpReg<(outs), (ins GPR32:$b),
  "j\t[$b]", []> {
  let link = 0;
  let isBranch = 1;
  let isTerminator = 1;
  let isIndirectBranch = 1;
}

// jl [reg]
def JL_r : ARC4InstJumpReg<(outs), (ins GPR32:$b),
  "jl\t[$b]", []> {
  let link = 1;
  let isCall = 1;
}

// j limm
def J_l : ARC4InstJumpLimm<(outs), (ins limm32:$target),
  "j\t$target", []> {
  let link = 0;
  let isBranch = 1;
  let isTerminator = 1;
}

// jl limm
def JL_l : ARC4InstJumpLimm<(outs), (ins limm32:$target),
  "jl\t$target", []> {
  let link = 1;
  let isCall = 1;
}
```

- [ ] **Step 4: Add flag instruction definitions**

```tablegen
//===----------------------------------------------------------------------===//
// Flag Instruction — 3 variants
//===----------------------------------------------------------------------===//

def FLAG_r : ARC4InstFlag_r<(outs), (ins GPR32:$b),
  "flag\t$b", []>;

def FLAG_s : ARC4InstFlag_s<(outs), (ins shimm9:$imm),
  "flag\t$imm", []>;

def FLAG_l : ARC4InstFlag_l<(outs), (ins limm32:$imm),
  "flag\t$imm", []>;
```

- [ ] **Step 5: Build and verify tablegen + full MC layer**

```bash
cmake --build build --target ARC4CommonTableGen 2>&1 | tail -20
cmake --build build --target llvm-mc 2>&1 | tail -20
```

- [ ] **Step 6: Commit**

```bash
git add llvm/lib/Target/ARC4/ARC4InstrInfo.td
git commit -m "feat(ARC4): add load, store, branch/jump, and flag instruction definitions"
```

---

## Task 5: MCCodeEmitter — Shimm/Limm Encoding Support

**Files:**
- Modify: `llvm/lib/Target/ARC4/MCTargetDesc/ARC4MCCodeEmitter.cpp`

The MCCodeEmitter needs to handle limm operands by emitting an extra 32-bit word after the instruction.

- [ ] **Step 1: Add limm emission to encodeInstruction**

Update `encodeInstruction` in `ARC4MCCodeEmitter.cpp` to detect limm operands and emit the extra word:

```cpp
void ARC4MCCodeEmitter::encodeInstruction(const MCInst &Inst,
                                           SmallVectorImpl<char> &CB,
                                           SmallVectorImpl<MCFixup> &Fixups,
                                           const MCSubtargetInfo &STI) const {
  uint64_t Value = getBinaryCodeForInstr(Inst, Fixups, STI);
  ++MCNumEmitted;

  // Emit the 32-bit instruction word (little-endian)
  support::endian::write<uint32_t>(CB, Value, llvm::endianness::little);

  // Check for limm operands — if any operand is a 32-bit immediate that
  // doesn't fit in shimm, emit the extra limm word.
  // The instruction size is set to 8 in TableGen for limm variants,
  // and the limm value is stored as an operand.
  const MCInstrDesc &Desc = MCII.get(Inst.getOpcode());
  if (Desc.getSize() == 8) {
    // Find the limm operand value
    for (unsigned i = 0; i < Inst.getNumOperands(); ++i) {
      const MCOperand &MO = Inst.getOperand(i);
      if (MO.isImm() && !isInt<9>(MO.getImm())) {
        support::endian::write<uint32_t>(CB, MO.getImm(),
                                          llvm::endianness::little);
        break;
      }
      if (MO.isExpr()) {
        // Symbol reference — emit as fixup
        Fixups.push_back(MCFixup::create(4, MO.getExpr(),
                                          MCFixupKind(FK_Data_4)));
        support::endian::write<uint32_t>(CB, 0, llvm::endianness::little);
        break;
      }
    }
  }
}
```

Note: This is a simplified approach. The subagent implementing this should read the MCInstrInfo to determine instruction size and find the limm operand correctly. The MCII field needs to be stored in the class (add `const MCInstrInfo &MCII` back as a member).

- [ ] **Step 2: Build and test basic limm emission**

```bash
cmake --build build --target llvm-mc 2>&1 | tail -20
```

- [ ] **Step 3: Commit**

```bash
git add llvm/lib/Target/ARC4/MCTargetDesc/ARC4MCCodeEmitter.cpp
git commit -m "feat(ARC4): add limm word emission to MCCodeEmitter"
```

---

## Task 6: AsmParser — Modifier Parsing and Memory Operands

**Files:**
- Modify: `llvm/lib/Target/ARC4/AsmParser/ARC4AsmParser.cpp`

The AsmParser needs to:
1. Parse mnemonic suffixes (.f, .eq, .d, .nd, .jd, .b, .w, .x, .a, .di)
2. Parse memory operands: `[base, offset]` and `[base]`
3. Map modifier suffixes to appropriate instruction variants

- [ ] **Step 1: Add memory operand type to ARC4Operand**

Add a `Memory` kind to the operand type and factory method in `ARC4AsmParser.cpp`:

```cpp
struct ARC4Operand : public MCParsedAsmOperand {
  enum KindTy { Token, Register, Immediate, Memory } Kind;
  // ... existing fields ...
  MCRegister MemBase;
  const MCExpr *MemOff;

  // ... add factory:
  static std::unique_ptr<ARC4Operand> createMem(MCRegister Base,
                                                  const MCExpr *Off,
                                                  SMLoc S, SMLoc E) {
    auto Op = std::make_unique<ARC4Operand>(Memory, S, E);
    Op->MemBase = Base;
    Op->MemOff = Off;
    return Op;
  }

  bool isMem() const override { return Kind == Memory; }

  void addMemOperands(MCInst &Inst, unsigned N) const {
    assert(N == 2);
    Inst.addOperand(MCOperand::createReg(MemBase));
    if (const auto *CE = dyn_cast<MCConstantExpr>(MemOff))
      Inst.addOperand(MCOperand::createImm(CE->getValue()));
    else
      Inst.addOperand(MCOperand::createExpr(MemOff));
  }
};
```

- [ ] **Step 2: Add mnemonic suffix parsing to parseInstruction**

Update `parseInstruction` to split the mnemonic on `.` and handle suffixes:

```cpp
bool ARC4AsmParser::parseInstruction(ParseInstructionInfo &Info, StringRef Name,
                                     SMLoc NameLoc, OperandVector &Operands) {
  // Split mnemonic: "add.eq.f" => base="add", suffixes=[".eq", ".f"]
  StringRef Base = Name;
  SmallVector<StringRef, 4> Suffixes;

  // Extract base mnemonic and suffixes
  size_t DotPos = Name.find('.');
  if (DotPos != StringRef::npos) {
    Base = Name.slice(0, DotPos);
    StringRef Rest = Name.slice(DotPos, StringRef::npos);
    while (!Rest.empty()) {
      Rest = Rest.drop_front(); // skip '.'
      size_t NextDot = Rest.find('.');
      if (NextDot == StringRef::npos) {
        Suffixes.push_back(Rest);
        Rest = StringRef();
      } else {
        Suffixes.push_back(Rest.slice(0, NextDot));
        Rest = Rest.slice(NextDot, StringRef::npos);
      }
    }
  }

  // For now, add the base mnemonic as token
  // TODO: Process suffixes into operands for condition codes, flags, etc.
  Operands.push_back(ARC4Operand::createToken(Base, NameLoc));

  // Parse operands...
  // (keep existing operand parsing, add memory operand support)
  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    if (parseOperand(Operands))
      return true;
    while (getLexer().is(AsmToken::Comma)) {
      getParser().Lex();
      if (parseOperand(Operands))
        return true;
    }
  }

  if (getLexer().isNot(AsmToken::EndOfStatement))
    return Error(getLexer().getLoc(), "unexpected token in operand list");
  getParser().Lex();
  return false;
}
```

- [ ] **Step 3: Add memory operand parsing**

Add to `parseOperand` in `ARC4AsmParser.cpp`:

```cpp
bool ARC4AsmParser::parseOperand(OperandVector &Operands) {
  SMLoc Start = getLexer().getLoc();

  // Memory operand: [base, offset] or [base]
  if (getLexer().is(AsmToken::LBrac)) {
    getParser().Lex(); // eat '['
    SMLoc BaseLoc = getLexer().getLoc();

    // Parse base (register or immediate)
    MCRegister BaseReg;
    const MCExpr *BaseExpr = nullptr;
    if (getLexer().is(AsmToken::Identifier)) {
      BaseReg = tryParseRegisterName(getLexer().getTok().getIdentifier());
      if (BaseReg)
        getParser().Lex();
    }
    if (!BaseReg) {
      // Try immediate base (for limm/shimm addressing)
      if (getParser().parseExpression(BaseExpr))
        return Error(BaseLoc, "expected register or immediate in memory operand");
    }

    const MCExpr *Offset = MCConstantExpr::create(0, getContext());
    if (getLexer().is(AsmToken::Comma)) {
      getParser().Lex(); // eat ','
      if (getParser().parseExpression(Offset))
        return true;
    }

    if (getLexer().isNot(AsmToken::RBrac))
      return Error(getLexer().getLoc(), "expected ']'");
    getParser().Lex(); // eat ']'

    if (BaseReg) {
      Operands.push_back(ARC4Operand::createMem(BaseReg, Offset, Start,
                                                  getLexer().getLoc()));
    } else {
      // Immediate base — add as separate operands
      Operands.push_back(ARC4Operand::createImm(BaseExpr, Start, getLexer().getLoc()));
    }
    return false;
  }

  // ... existing register and immediate parsing ...
}
```

- [ ] **Step 4: Build and test**

```bash
cmake --build build --target llvm-mc 2>&1 | tail -20
echo "add r0, r1, 5" | ./build/bin/llvm-mc -triple=arc4-unknown-elf -show-encoding
echo "ld r0, [r1, 4]" | ./build/bin/llvm-mc -triple=arc4-unknown-elf -show-encoding
echo "st r0, [r1, 8]" | ./build/bin/llvm-mc -triple=arc4-unknown-elf -show-encoding
echo "b 100" | ./build/bin/llvm-mc -triple=arc4-unknown-elf -show-encoding
echo "j [r5]" | ./build/bin/llvm-mc -triple=arc4-unknown-elf -show-encoding
echo "flag r1" | ./build/bin/llvm-mc -triple=arc4-unknown-elf -show-encoding
```

- [ ] **Step 5: Commit**

```bash
git add llvm/lib/Target/ARC4/AsmParser/ARC4AsmParser.cpp
git commit -m "feat(ARC4): add mnemonic suffix parsing and memory operand support"
```

---

## Task 7: InstPrinter — Modifier and Memory Operand Printing

**Files:**
- Modify: `llvm/lib/Target/ARC4/MCTargetDesc/ARC4InstPrinter.h`
- Modify: `llvm/lib/Target/ARC4/MCTargetDesc/ARC4InstPrinter.cpp`

- [ ] **Step 1: Add print methods for new operand types**

Add to `ARC4InstPrinter.h`:

```cpp
void printCondCode(const MCInst *MI, unsigned OpNo, raw_ostream &O);
void printMemOperand(const MCInst *MI, unsigned OpNo, raw_ostream &O);
void printMemShimmOperand(const MCInst *MI, unsigned OpNo, raw_ostream &O);
void printMemLimmOperand(const MCInst *MI, unsigned OpNo, raw_ostream &O);
```

Implement in `ARC4InstPrinter.cpp`:

```cpp
void ARC4InstPrinter::printCondCode(const MCInst *MI, unsigned OpNo,
                                     raw_ostream &O) {
  static const char *CondNames[] = {
    "al", "eq", "ne", "pl", "mi", "cs", "cc", "vs",
    "vc", "gt", "ge", "lt", "le", "hi", "ls", "pnz"
  };
  unsigned CC = MI->getOperand(OpNo).getImm();
  if (CC < 16 && CC != 0)  // don't print "al" (always)
    O << '.' << CondNames[CC];
}

void ARC4InstPrinter::printMemOperand(const MCInst *MI, unsigned OpNo,
                                       raw_ostream &O) {
  O << '[';
  printRegName(O, MI->getOperand(OpNo).getReg());
  O << ", ";
  printRegName(O, MI->getOperand(OpNo + 1).getReg());
  O << ']';
}

void ARC4InstPrinter::printMemShimmOperand(const MCInst *MI, unsigned OpNo,
                                            raw_ostream &O) {
  O << '[';
  printRegName(O, MI->getOperand(OpNo).getReg());
  int64_t Offset = MI->getOperand(OpNo + 1).getImm();
  if (Offset != 0)
    O << ", " << Offset;
  O << ']';
}

void ARC4InstPrinter::printMemLimmOperand(const MCInst *MI, unsigned OpNo,
                                           raw_ostream &O) {
  O << '[';
  printRegName(O, MI->getOperand(OpNo).getReg());
  O << ", ";
  printOperand(MI, OpNo + 1, O);
  O << ']';
}
```

- [ ] **Step 2: Build and verify**

```bash
cmake --build build --target llvm-mc 2>&1 | tail -20
```

- [ ] **Step 3: Commit**

```bash
git add llvm/lib/Target/ARC4/MCTargetDesc/ARC4InstPrinter.h \
        llvm/lib/Target/ARC4/MCTargetDesc/ARC4InstPrinter.cpp
git commit -m "feat(ARC4): add condition code and memory operand printing"
```

---

## Task 8: Comprehensive Tests

**Files:**
- Create: `llvm/test/MC/ARC4/alu-shimm.s`
- Create: `llvm/test/MC/ARC4/alu-limm.s`
- Create: `llvm/test/MC/ARC4/single-op-variants.s`
- Create: `llvm/test/MC/ARC4/load-store.s`
- Create: `llvm/test/MC/ARC4/branch-jump.s`
- Create: `llvm/test/MC/ARC4/flag.s`

- [ ] **Step 1: Create ALU shimm/limm encoding tests**

Create `llvm/test/MC/ARC4/alu-shimm.s`:

```asm
; RUN: llvm-mc -triple=arc4-unknown-elf -show-encoding %s | FileCheck %s

; shimm in C position (rrs variant)
; CHECK: add r0, r1, 5
add r0, r1, 5

; shimm in B position (rsr variant)
; CHECK: add r0, 5, r1
add r0, 5, r1

; shimm in both positions (rss variant, must match)
; CHECK: add r0, 5, 5
add r0, 5, 5

; mov alias with shimm
; CHECK: and
mov r0, 5
```

Create `llvm/test/MC/ARC4/alu-limm.s`:

```asm
; RUN: llvm-mc -triple=arc4-unknown-elf -show-encoding %s | FileCheck %s

; limm in C position (rrl variant)
; CHECK: add r0, r1, 1000
add r0, r1, 1000

; limm in B position (rlr variant)
; CHECK: add r0, 1000, r1
add r0, 1000, r1
```

Note: Exact CHECK lines with encoding bytes should be computed by running through llvm-mc first. The subagent should run each instruction to get actual encodings and write precise CHECK lines.

- [ ] **Step 2: Create load/store tests**

Create `llvm/test/MC/ARC4/load-store.s`:

```asm
; RUN: llvm-mc -triple=arc4-unknown-elf -show-encoding %s | FileCheck %s

; Load variants
; CHECK: ld
ld r0, [r1, r2]

; CHECK: ld
ld r0, [r1, 4]

; CHECK: ld
ld r0, [r1]

; Store variants
; CHECK: st
st r0, [r1, 4]

; CHECK: st
st r0, [r1]
```

- [ ] **Step 3: Create branch/jump tests**

Create `llvm/test/MC/ARC4/branch-jump.s`:

```asm
; RUN: llvm-mc -triple=arc4-unknown-elf -show-encoding %s | FileCheck %s

; CHECK: b
b 100

; CHECK: j
j [r5]

; CHECK: jl
jl [r31]
```

- [ ] **Step 4: Create flag test**

Create `llvm/test/MC/ARC4/flag.s`:

```asm
; RUN: llvm-mc -triple=arc4-unknown-elf -show-encoding %s | FileCheck %s

; CHECK: flag
flag r1

; CHECK: flag
flag 5
```

- [ ] **Step 5: Run all tests**

```bash
./build/bin/llvm-lit llvm/test/MC/ARC4/ -v 2>&1
```

Expected: All tests pass.

- [ ] **Step 6: Commit**

```bash
git add llvm/test/MC/ARC4/
git commit -m "test(ARC4): add encoding tests for shimm/limm, load/store, branch/jump, flag"
```
