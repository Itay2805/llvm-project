# ARC4 (ARCtangent-A4) LLVM Target — Assembler & Linker Design

**Date:** 2026-03-28
**Approach:** MC-Layer-First (Approach A)
**Scope:** Assembler (llvm-mc), Linker (lld), Clang driver integration, stub TargetMachine

## Overview

Port the ARCtangent-A4 architecture into LLVM as a new, separate target (`arc4`). This initial phase delivers assembler and linker infrastructure for embedded bare-metal development, with a stub TargetMachine ready for future codegen backend work.

The ARC4 is a 32-bit RISC architecture with fixed 32-bit instruction words. It is distinct from the existing LLVM `arc` target (Synopsys ARCompact — ARC600/ARC700) which uses a different, incompatible ISA with mixed 16/32-bit instructions.

**Reference implementations:**
- GCC 3.4.x ARC port: `/home/tomato/Downloads/Obsolete_Arc-gnu-tools-src-20060612/arc-gnu-tools-src-20060612/gcc-3.4.x/gcc/config/arc/`
- Binutils 2.15 ARC port: `/home/tomato/Downloads/Obsolete_Arc-gnu-tools-src-20060612/arc-gnu-tools-src-20060612/binutils-2.15/`
- Architecture spec: `/home/tomato/projects/arctangent-a4-llvm/docs/ARC4_Programmers_reference.{pdf,html}`
- GNU tools user guide: `/home/tomato/projects/arctangent-a4-llvm/docs/Obsolete_ARC_upgrade_GNU_tools_user_guide-20060612.pdf`

---

## 1. Target Identity & Triple

- **Triple:** `arc4-unknown-elf`
- **Architecture enum:** `Triple::arc4` (new entry in `Triple.h`)
- **Endianness:** Little-endian only
- **ELF machine type:** `EM_ARC` (0x2D / 45) — the original ARC machine type, predating ARCompact's `EM_ARC_COMPACT` (0x5D / 93)
- **ELF flags:** `E_ARC_MACH_A4 = 0x00` in `e_flags`
- **Object format:** ELF32 little-endian
- **Data layout string:** `e-m:e-p:32:32-i1:8:32-i8:8:32-i16:16:32-i32:32:32-f32:32:32-a:0:32-n32`
  - 32-bit pointers, natural alignment, no 64-bit native operations

---

## 2. Register File

### Core Registers (r0–r31, all 32-bit)

| Registers | ABI Name | Role | Callee-saved? |
|-----------|----------|------|---------------|
| r0 | r0 | Return value / arg 0 | No |
| r1–r7 | r1–r7 | Arguments 1–7 | No |
| r8–r12 | r8–r12 | Temporaries | No |
| r13–r25 | r13–r25 | Saved registers | Yes |
| r26 | gp | Global pointer | Yes (fixed) |
| r27 | fp | Frame pointer | Yes (fixed) |
| r28 | sp | Stack pointer | Yes (fixed) |
| r29 | ilink1 | Interrupt link 1 | Reserved |
| r30 | ilink2 | Interrupt link 2 | Reserved |
| r31 | blink | Return address | No |

### Special Registers

- **r60 / LP_COUNT** — 24-bit hardware loop counter (separate register class, not in GPR)

### Register Classes

- `GPR32` — r0–r28, r31 (allocatable general purpose)
- `GPR32NoSP` — same minus r28 (for ops that can't use SP)
- `LP` — LP_COUNT only

### Not Modeled Initially

Auxiliary registers (STATUS, SEMAPHORE, LP_START, LP_END, IDENTITY, DEBUG, etc.) are accessed via `lr`/`sr` instructions, not part of the register file for allocation. Added when the backend needs them.

---

## 3. Instruction Set (Initial Subset)

All instructions are 32-bit fixed width. Major opcode in bits [31:27] (I field), register fields A[26:21], B[20:15], C[14:9].

All encodings will be cross-referenced against the binutils `arc-opc.c` opcode table as the authoritative source.

### 3.1 ALU Instructions (3-operand: `op.q.f a, b, c/shimm/limm`)

| Mnemonic | Opcode (I) | Notes |
|----------|-----------|-------|
| add | 8 | |
| adc | 9 | Add with carry |
| sub | 10 | |
| sbc | 11 | Sub with carry |
| and | 12 | |
| or | 13 | |
| bic | 14 | Bit clear |
| xor | 15 | |

### 3.2 Single-Operand Instructions (opcode 3, sub-opcode in C field)

| Mnemonic | Sub-op | Notes |
|----------|--------|-------|
| asl | 0 | Arithmetic shift left |
| asr | 1 | Arithmetic shift right |
| lsr | 2 | Logical shift right |
| ror | 3 | Rotate right |
| rrc | 4 | Rotate right through carry |
| sexb | 5 | Sign-extend byte |
| sexw | 6 | Sign-extend halfword |
| extb | 7 | Zero-extend byte |
| extw | 8 | Zero-extend halfword |

### 3.3 Pseudo/Alias Instructions

| Mnemonic | Expansion | Notes |
|----------|-----------|-------|
| mov | `and a, b, b` | Move register |
| nop | encoding `0x7fffffff` | No-op (special xor pattern) |

### 3.4 Load/Store

| Mnemonic | Opcode | Notes |
|----------|--------|-------|
| ld | 0, 1 | Word load (0 = reg+reg/shimm, 1 = reg+limm) |
| ldb | variant | Byte load (.B suffix) |
| ldw | variant | Halfword load (.W suffix) |
| st | 2 | Word store |
| stb | variant | Byte store |
| stw | variant | Halfword store |

Addressing modes: `[b]`, `[b, c]`, `[b, shimm]`, `[b, limm]`, with `.A` writeback.

### 3.5 Branch/Jump (opcode 4 sub-opcodes)

| Mnemonic | Notes |
|----------|-------|
| b | Branch (20-bit PC-relative offset) |
| bl | Branch and link |
| j | Jump (absolute or register) |
| jl | Jump and link |
| lp | Loop setup (zero-overhead loop) |

All branch/jump support condition codes and delay slot modifiers.

### 3.6 Flag Instruction

| Mnemonic | Notes |
|----------|-------|
| flag | Load STATUS register from operand |

### 3.7 Instruction Modifiers

- `.f` — set flags (condition codes)
- `.q` — conditional execution (16 conditions: al, eq, ne, pl, mi, cs/hs, cc/lo, vs, vc, gt, ge, lt, le, hi, ls, pnz)
- `.d` — delay slot (always execute delay slot instruction)
- `.nd` — no delay slot (nullify delay slot)
- `.jd` — jump delay (needed for `jl limm` and similar)

### 3.8 Deferred to Later

Extension instructions: barrel shifter, norm, swap, mul64, min/max, extended arithmetic. These will be added as optional features gated on subtarget flags.

---

## 4. MC Layer Components

### 4.1 MCAsmInfo (`ARC4MCAsmInfo`)

- Comment string: `;`
- Assembler directive prefix: `.`
- Label suffix: `:`
- PC symbol: `.`
- RELA relocations (not REL) — matching binutils reference
- Supports `.option` / `.cpu` directives for future extensibility

### 4.2 MCCodeEmitter (`ARC4MCCodeEmitter`)

- Encodes instructions to 32-bit little-endian words
- Handles SHIMM (9-bit signed) and LIMM (32-bit, extra word after instruction) encoding
- Emits fixups for relocations unresolvable at assembly time (branch targets, absolute addresses)

### 4.3 AsmBackend (`ARC4AsmBackend`)

- Applies fixups: resolves branch offsets, immediate values
- No relaxation (all instructions are fixed 32-bit)
- Object writer: ELF32 little-endian, `EM_ARC`, `E_ARC_MACH_A4` flags
- Fixup kinds → ELF relocations:
  - `FK_Data_4` → `R_ARC_32` (32-bit absolute)
  - `fixup_arc4_b26` → `R_ARC_B26` (26-bit absolute branch)
  - `fixup_arc4_b22_pcrel` → `R_ARC_B22_PCREL` (22-bit PC-relative branch)

### 4.4 InstPrinter (`ARC4InstPrinter`)

- Register names: `r0`–`r31`, plus aliases `gp`, `fp`, `sp`, `blink`, `ilink1`, `ilink2`, `lp_count`
- Instruction mnemonics with modifiers (`.f`, `.q`, `.d`, `.nd`, `.jd`)
- Immediate formatting: shimm as decimal, limm as hex

### 4.5 AsmParser (`ARC4AsmParser`)

- Parses register names (numbered `r0`–`r31` and aliases `sp`, `fp`, etc.)
- Parses instruction mnemonics with optional chained modifiers (e.g., `add.eq.f`)
- Parses operands: registers, immediates, memory operands (`[b, offset]`)
- Parses condition codes and delay slot suffixes
- Handles assembler directives

---

## 5. lld/ELF Linker Support

### 5.1 Target Info (`ARC4` in `lld/ELF/Arch/ARC4.cpp`)

- ELF machine: `EM_ARC`
- Default entry point: `__start`
- Page size: 4096 bytes
- Default image base: 0x0 (embedded, no virtual memory)

### 5.2 Relocations (Initial Set)

| Relocation | Type | Description |
|-----------|------|-------------|
| `R_ARC_NONE` | — | No relocation |
| `R_ARC_32` | Absolute | 32-bit absolute address |
| `R_ARC_B26` | Absolute | 26-bit absolute branch target |
| `R_ARC_B22_PCREL` | PC-relative | 22-bit PC-relative branch |

SDA relocations (`R_ARC_SDA*`) and section-relative (`R_ARC_SECTOFF*`) deferred — optimization features not required for basic linking.

### 5.3 Linker Script

No custom built-in linker script. Users provide their own (standard for embedded targets).

---

## 6. Clang Driver & Target Info

### 6.1 Clang TargetInfo (`ARC4TargetInfo` in `clang/lib/Basic/Targets/`)

- Data layout string matching LLVM target
- Pointer width/align: 32/32
- Int/Long: 32/32, LongLong: 64
- Float/Double: IEEE 754 32/64 (soft-float only, no FPU)
- Predefined macros: `__arc4__`, `__ARC4__`, `__arc__`
- No TLS support (bare-metal)
- Atomic width: 32-bit max

### 6.2 Clang ToolChain (`ARC4ToolChain` in `clang/lib/Driver/ToolChains/`)

- Inherits `Generic_ELF`
- Defaults: `-fno-exceptions`, `-fno-rtti`
- Links with `ld.lld` by default
- No default sysroot or standard library paths (bare-metal; user supplies everything)
- Passes `-march=arc4` through to the assembler

### 6.3 Stub TargetMachine (`ARC4TargetMachine`)

- Registered in target registry so `llc` recognizes `arc4`
- Data layout and triple wired up
- No passes, no codegen — shell for future backend work
- Reports error if someone tries to compile IR to machine code (assembler-only for now)

---

## 7. File Structure

```
llvm/include/llvm/TargetParser/Triple.h          — add arc4 enum
llvm/lib/TargetParser/Triple.cpp                  — arc4 triple parsing

llvm/lib/Target/ARC4/
  ARC4.td                                         — top-level TableGen
  ARC4RegisterInfo.td                             — register definitions
  ARC4InstrFormats.td                             — instruction format classes
  ARC4InstrInfo.td                                — instruction definitions
  ARC4CallingConv.td                              — calling convention (stub)
  ARC4TargetMachine.cpp/.h                        — stub target machine
  ARC4Subtarget.cpp/.h                            — subtarget (A4 features)
  CMakeLists.txt
  TargetInfo/
    ARC4TargetInfo.cpp/.h                         — target registration
    CMakeLists.txt
  MCTargetDesc/
    ARC4MCAsmInfo.cpp/.h                          — assembly syntax info
    ARC4MCTargetDesc.cpp/.h                       — MC registration
    ARC4MCCodeEmitter.cpp                         — instruction encoding
    ARC4AsmBackend.cpp                            — fixups + ELF writer
    ARC4InstPrinter.cpp/.h                        — instruction printing
    ARC4FixupKinds.h                              — fixup kind enum
    ARC4ELFObjectWriter.cpp                       — ELF object writer
    CMakeLists.txt
  AsmParser/
    ARC4AsmParser.cpp                             — assembly parser
    CMakeLists.txt

llvm/include/llvm/BinaryFormat/ELFRelocs/ARC4.def — relocation type definitions

lld/ELF/Arch/ARC4.cpp                            — linker target
lld/ELF/Target.h                                 — declare setARC4TargetInfo
lld/ELF/Target.cpp                               — wire into switch

clang/lib/Basic/Targets/ARC4.cpp/.h              — clang target info
clang/lib/Basic/Targets.cpp                       — register in factory
clang/lib/Driver/ToolChains/ARC4.cpp/.h           — toolchain
clang/lib/Driver/Driver.cpp                       — register toolchain
clang/lib/Driver/CMakeLists.txt                   — add source file
```

---

## 8. Build Order (Implementation Phases)

1. Triple + target registration (make LLVM aware of `arc4`)
2. TableGen foundations (registers, instruction formats, initial instructions)
3. MC layer (MCAsmInfo, CodeEmitter, AsmBackend, InstPrinter, AsmParser)
4. lld/ELF (relocations, basic linking)
5. Clang driver (TargetInfo, ToolChain)
6. Stub TargetMachine (future codegen hook)
