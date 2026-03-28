# ARC4 Full Base ISA — Shimm/Limm, Modifiers, Load/Store, Branch/Jump

**Date:** 2026-03-28
**Scope:** Complete the ARC4 base instruction set: shimm/limm operand support for all instructions, instruction modifiers (.f, .q, delay slots), load/store, branch/jump, flag instruction. No extension instructions.

## Overview

Phase 1 delivered ALU instructions in register-register-register form only. This phase completes the base ISA by adding:

1. **Shimm/limm operand encoding** — 9-bit short immediates and 32-bit long immediates as operands to all instruction types, with all valid combinations per the spec
2. **Instruction modifiers** — `.f` (flag update), `.q` (condition codes), `.d`/`.nd`/`.jd` (delay slot control)
3. **Load instructions** — 6 variants across opcodes 0 and 1
4. **Store instructions** — 7 variants on opcode 2
5. **Branch/jump instructions** — b, bl, lp (PC-relative), j, jl (absolute/register)
6. **Flag instruction** — 3 variants

**Approach:** Enumerate all valid operand combinations as separate TableGen instruction records, generated via multiclasses. Custom MCCodeEmitter logic handles sentinel values (61/62/63) in register fields and shimm/limm encoding. AsmParser extended to parse modifiers and memory operands.

**Reference implementations:**
- Binutils opcode table: `binutils-2.15/opcodes/arc-opc.c`
- Architecture spec: `ARC4_Programmers_reference.html`

---

## 1. Operand Encoding Infrastructure

### Sentinel Values in Register Fields (6-bit)

| Value | Name | Meaning |
|-------|------|---------|
| 0–31 | — | Physical register r0–r31 |
| 60 | LP_COUNT | Loop counter register |
| 61 | SHIMM_UPDATE | Short immediate, flag update enabled |
| 62 | LIMM | Long immediate marker (32-bit word follows) |
| 63 | SHIMM | Short immediate, no flag update |

### Immediate Operand Types

| Type | Width | Location | Constraint |
|------|-------|----------|------------|
| shimm | 9-bit signed (-256..255) | Bits [8:0] of instruction word | Only one shimm field per instruction; overlaps condition code bits [4:0] |
| limm | 32-bit | Separate word following instruction | Sentinel 62 in register field; instruction becomes 8 bytes total |

### Flag (.f) Encoding

- **Without shimm:** Bit [8] = 1 when `.f` is specified
- **With shimm:** Sentinel 61 (SHIMM_UPDATE) in register field means `.f`; sentinel 63 (SHIMM) means no `.f`. Bit [8] is part of the shimm value in this case.
- **Result:** `.f` is ALWAYS available on all instruction forms

### Condition Code (.q) Encoding

5-bit field in bits [4:0]:

| Code | Value | Code | Value |
|------|-------|------|-------|
| al/ra | 0x00 | gt | 0x09 |
| eq/z | 0x01 | ge | 0x0A |
| ne/nz | 0x02 | lt | 0x0B |
| pl/p | 0x03 | le | 0x0C |
| mi/n | 0x04 | hi | 0x0D |
| cs/c/lo | 0x05 | ls | 0x0E |
| cc/nc/hs | 0x06 | pnz | 0x0F |
| vs/v | 0x07 | | |
| vc/nv | 0x08 | | |

**Constraint:** Condition codes occupy bits [4:0], which overlap shimm bits [4:0]. Therefore `.q` is NOT available when shimm operands are present.

### Delay Slot Encoding

2-bit field in bits [6:5] (branch/jump instructions only):

| Mode | Value | Meaning |
|------|-------|---------|
| nd | 0b00 | No delay slot (nullify) |
| d | 0b01 | Always execute delay slot |
| jd | 0b10 | Execute delay slot only on jump taken |

---

## 2. 3-Operand ALU Instructions (12 variants each)

**Instructions:** ADD (8), ADC (9), SUB (10), SBC (11), AND (12), OR (13), BIC (14), XOR (15)

**Encoding:** I[31:27]=opcode, A[26:21]=dest, B[20:15]=src1, C[14:9]=src2, bits [8:0]=flags/shimm

### All 12 Variants

| # | Suffix | Dest (A) | Src1 (B) | Src2 (C) | .f | .q |
|---|--------|----------|----------|----------|----|----|
| 1 | rrr | reg | reg | reg | yes (bit 8) | yes |
| 2 | rrs | reg | reg | shimm | yes (sentinel) | **no** |
| 3 | rsr | reg | shimm | reg | yes (sentinel) | **no** |
| 4 | rss | reg | shimm | shimm | yes (sentinel) | **no** |
| 5 | rrl | reg | reg | limm | yes (bit 8) | yes |
| 6 | rlr | reg | limm | reg | yes (bit 8) | yes |
| 7 | 0rr | discard | reg | reg | yes (bit 8) | yes |
| 8 | 0rs | discard | reg | shimm | yes (sentinel) | **no** |
| 9 | 0sr | discard | shimm | reg | yes (sentinel) | **no** |
| 10 | 0ss | discard | shimm | shimm | yes (sentinel) | **no** |
| 11 | 0rl | discard | reg | limm | yes (bit 8) | yes |
| 12 | 0lr | discard | limm | reg | yes (bit 8) | yes |

**Shimm constraint:** When both B and C are shimm (#4, #10), values must match (shared bits [8:0]).

**Discard:** A field set to shimm sentinel; result not written.

**MOV alias:** `mov a, b` → `and a, b, b`. Extends to shimm/limm: `mov r0, 5` → `and r0, 5, 5` (rss form).

**Multiclass:** Single `ALU3` multiclass generates all 12 from `defm ADD : ALU3<0b01000, "add">`.

---

## 3. Single-Operand Instructions (6 variants each)

**Instructions:** ASL (0), ASR (1), LSR (2), ROR (3), RRC (4), SEXB (5), SEXW (6), EXTB (7), EXTW (8)

**Encoding:** I[31:27]=0b00011 (opcode 3), A[26:21]=dest, B[20:15]=src, C[14:9]=sub-opcode (fixed)

### All 6 Variants

| # | Suffix | Dest (A) | Src (B) | .f | .q |
|---|--------|----------|---------|----|----|
| 1 | rr | reg | reg | yes (bit 8) | yes |
| 2 | rs | reg | shimm | yes (sentinel) | **no** |
| 3 | rl | reg | limm | yes (bit 8) | yes |
| 4 | 0r | discard | reg | yes (bit 8) | yes |
| 5 | 0s | discard | shimm | yes (sentinel) | **no** |
| 6 | 0l | discard | limm | yes (bit 8) | yes |

**Multiclass:** Single `SOP` multiclass generates all 6 from `defm ASR : SOP<0b000001, "asr">`.

---

## 4. Load Instructions (6 variants)

Two opcodes:
- **Opcode 0** (I=0b00000): C field is register or limm (no shimm)
- **Opcode 1** (I=0b00001): shimm always present in bits [8:0]

### Modifiers

| Suffix | Meaning | Encoding |
|--------|---------|----------|
| `.b` | Byte load | size field |
| `.w` | Halfword load | size field |
| (none) | Word load | size field = 0 |
| `.x` | Sign-extend | sign-extend bit |
| `.a` | Writeback base register | writeback bit |
| `.di` | Cache bypass | cache bypass bit |

No condition codes on loads.

### Opcode 0 Encoding (reg/limm offset)

- I[31:27] = 0b00000
- A[26:21] = dest register
- B[20:15] = base register (or 62 for limm)
- C[14:9] = offset register (or 62 for limm)
- e[5] = cache bypass
- w[3] = writeback
- z[2:1] = size (00=word, 01=byte, 10=halfword)
- x[0] = sign extend

### Opcode 1 Encoding (shimm offset)

- I[31:27] = 0b00001
- A[26:21] = dest register
- B[20:15] = base register (or shimm/limm sentinel)
- E[14] = cache bypass
- W[12] = writeback
- Z[11:10] = size
- X[9] = sign extend
- shimm[8:0] = 9-bit signed offset

### All 6 Variants

| # | Opcode | Base (B) | Offset (C/shimm) | .a | Notes |
|---|--------|----------|-------------------|-----|-------|
| 1 | 0 | reg | reg | yes | `ld a, [b, c]` |
| 2 | 0 | reg | limm | yes | `ld a, [b, limm]` |
| 3 | 0 | limm | reg | **no** | `ld a, [limm, c]` |
| 4 | 1 | reg | shimm | yes | `ld a, [b, shimm]` |
| 5 | 1 | shimm | shimm | **no** | must match |
| 6 | 1 | limm | shimm=0 | **no** | `ld a, [limm]` absolute |

`ld a, [b]` is syntactic sugar for `ld a, [b, 0]` (variant #4 with shimm=0).

---

## 5. Store Instructions (7 variants)

**Opcode 2** (I=0b00010) only. Shimm offset field D[8:0] always present.

### Encoding

- I[31:27] = 0b00010
- D[26] = cache bypass
- v[24] = writeback
- y[23:22] = size (word/byte/halfword)
- B[20:15] = base register (or shimm/limm sentinel)
- C[14:9] = value register (or shimm/limm sentinel)
- D[8:0] = shimm offset

### Modifiers

| Suffix | Meaning |
|--------|---------|
| `.b` | Byte store |
| `.w` | Halfword store |
| `.a` | Writeback base register |
| `.di` | Cache bypass |

No condition codes on stores.

### All 7 Variants

| # | Value (C) | Address | .a | Notes |
|---|-----------|---------|-----|-------|
| 1 | reg | [reg, shimm] | yes | includes `st c, [b]` as shimm=0 |
| 2 | reg | [limm] | **no** | absolute address |
| 3 | reg | [shimm, shimm] | **no** | must match |
| 4 | shimm | [reg, shimm] | yes | value = offset (must match) |
| 5 | shimm | [limm] | **no** | encoded as [limm, shimm], assembler adjusts limm |
| 6 | limm | [reg, shimm] | yes | limm value, shimm offset |
| 7 | limm | [shimm, shimm] | **no** | limm value, shimm address (must match) |

**Important:** Store always encodes a shimm offset. When assembly syntax shows `st c, [limm]`, the assembler encodes it as `st c, [limm_adjusted, shimm]` where `limm_adjusted + shimm = intended_address`. When shimm appears as both value and offset, they share bits [8:0] and must have the same value.

---

## 6. Branch and Jump Instructions

### Branch (b — opcode 4), Branch-and-Link (bl — opcode 5), Loop (lp — opcode 6)

**Encoding:**
- I[31:27] = opcode (4/5/6)
- L[26:7] = 20-bit signed word-aligned PC-relative offset
- N[6:5] = delay slot mode
- Q[4:0] = condition code

Single format — 20-bit PC-relative offset. Condition codes and delay slot modifiers always available.

`bl` additionally saves return address to BLINK register. `lp` sets up zero-overhead loop (LP_START/LP_END).

### Jump (j/jl — opcode 7)

**Encoding:**
- I[31:27] = 0b00111
- A[26:21] = 0 (unused)
- B[20:15] = target register (or 62 for limm)
- C[14:9] = 0
- bit 9 = link flag (0=j, 1=jl)
- F[8] = flag update
- N[6:5] = delay slot mode
- Q[4:0] = condition code

### Jump Variants

| # | Target | .f | .q | .d/.nd/.jd | Notes |
|---|--------|----|----|------------|-------|
| 1 | reg | yes | yes | yes | `j [b]` / `jl [b]` |
| 2 | limm | yes | yes | yes (.jd for jl limm) | `j limm` / `jl limm` |

---

## 7. Flag Instruction

**Encoding:** Opcode 3, A[26:21] = 61 (SHIMM_UPDATE sentinel), C[14:9] = 0.

| # | Source (B) | .q |
|---|------------|-----|
| 1 | reg | yes |
| 2 | shimm | **no** |
| 3 | limm | yes |

---

## 8. Implementation Components

### AsmParser Changes

**Mnemonic suffix parsing:** Split mnemonic on `.` to extract base instruction and modifiers. Examples:
- `add.eq.f` → base=`add`, condcode=`eq`, flag=true
- `st.b.a.di` → base=`st`, size=`b`, writeback=true, cache_bypass=true
- `b.ne.d` → base=`b`, condcode=`ne`, delay=`d`

**Memory operand parsing:** Parse `[base, offset]` and `[base]` syntax for load/store.

**Operand type selection:** For immediates, determine shimm vs limm:
- Fits in 9-bit signed (-256..255) → shimm
- Otherwise → limm

### MCCodeEmitter Changes

**Sentinel insertion:** When encoding shimm/limm operands, set the appropriate sentinel value (61/62/63) in the register field instead of the register number.

**Flag/shimm interaction:** When `.f` is specified with a shimm operand, use sentinel 61 (SHIMM_UPDATE); without `.f`, use sentinel 63 (SHIMM).

**Limm word emission:** When a limm operand is present, emit an additional 32-bit word after the instruction. Instruction size becomes 8 bytes.

**Store limm address adjustment:** When store has shimm value and limm address, adjust the limm so that limm + shimm = intended address.

### InstPrinter Changes

Print modifiers (`.f`, condition code, delay slot, size, `.x`, `.a`, `.di`) and memory operands (`[r1, 5]` syntax).

---

## 9. Variant Count Summary

| Instruction Type | Instructions | Variants Each | Total Defs |
|-----------------|-------------|---------------|------------|
| 3-op ALU | 8 (add..xor) | 12 | 96 |
| Single-op | 9 (asl..extw) | 6 | 54 |
| Load | 1 (ld) × 3 sizes | 6 | 18 |
| Store | 1 (st) × 3 sizes | 7 | 21 |
| Branch | 3 (b, bl, lp) | 1 | 3 |
| Jump | 2 (j, jl) | 2 | 4 |
| Flag | 1 | 3 | 3 |
| **Total** | | | **~199** |

All generated from multiclasses — each instruction is a single `defm` line.
