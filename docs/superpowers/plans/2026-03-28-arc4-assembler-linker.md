# ARC4 Assembler & Linker Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add ARCtangent-A4 (ARC4) as a new LLVM target with assembler (llvm-mc) and linker (lld) support for embedded bare-metal development.

**Architecture:** MC-layer-first approach. We register the triple, build TableGen register/instruction definitions, implement the MC layer (code emitter, asm backend, asm parser, inst printer), add lld ELF support, wire up the clang driver, and create a stub TargetMachine. Each layer is independently testable.

**Tech Stack:** LLVM MC framework, TableGen, lld ELF, Clang Driver

**Scope note:** This plan covers infrastructure bring-up with ALU + NOP instructions — enough for end-to-end validation of the assembler, linker, and clang driver. Load/store instructions, branch/jump instructions, and instruction modifiers (`.f`, `.q`, `.d`, `.nd`, `.jd`) require complex multi-format encodings and will be added in follow-up plans once the core infrastructure is proven.

**Reference implementations (cross-reference all encodings against these):**
- Binutils opcode table: `/home/tomato/Downloads/Obsolete_Arc-gnu-tools-src-20060612/arc-gnu-tools-src-20060612/binutils-2.15/opcodes/arc-opc.c`
- Binutils assembler: `/home/tomato/Downloads/Obsolete_Arc-gnu-tools-src-20060612/arc-gnu-tools-src-20060612/binutils-2.15/gas/config/tc-arc.c`
- ELF relocations: `/home/tomato/Downloads/Obsolete_Arc-gnu-tools-src-20060612/arc-gnu-tools-src-20060612/binutils-2.15/include/elf/arc.h`
- BFD relocation handling: `/home/tomato/Downloads/Obsolete_Arc-gnu-tools-src-20060612/arc-gnu-tools-src-20060612/binutils-2.15/bfd/elf32-arc.c`
- GCC machine description: `/home/tomato/Downloads/Obsolete_Arc-gnu-tools-src-20060612/arc-gnu-tools-src-20060612/gcc-3.4.x/gcc/config/arc/arc.h`
- Architecture spec: `/home/tomato/projects/arctangent-a4-llvm/docs/ARC4_Programmers_reference.{pdf,html}`

---

## Task 1: Register `arc4` Triple in LLVM

**Files:**
- Modify: `llvm/include/llvm/TargetParser/Triple.h:57` (add enum)
- Modify: `llvm/lib/TargetParser/Triple.cpp` (8 switch statements)

- [ ] **Step 1: Add `arc4` to the ArchType enum**

In `llvm/include/llvm/TargetParser/Triple.h`, add `arc4` after the existing `arc` entry:

```cpp
    arc,         // ARC: Synopsys ARC
    arc4,        // ARC4: ARCtangent-A4
    avr,         // AVR: Atmel AVR microcontroller
```

- [ ] **Step 2: Add `arc4` to `getArchTypeName`**

In `llvm/lib/TargetParser/Triple.cpp` in the `getArchTypeName` function, add after the `arc` case:

```cpp
  case arc:            return "arc";
  case arc4:           return "arc4";
  case arm:            return "arm";
```

- [ ] **Step 3: Add `arc4` to `getArchTypePrefix`**

In `llvm/lib/TargetParser/Triple.cpp` in the `getArchTypePrefix` function, add after the `arc` entry:

```cpp
  case arc:         return "arc";
  case arc4:        return "arc4";
  case arm:
```

- [ ] **Step 4: Add `arc4` to `parseArch`**

In `llvm/lib/TargetParser/Triple.cpp` in the `parseArch` function, add:

```cpp
      .Case("arc", arc)
      .Case("arc4", arc4)
      .Case("avr", avr)
```

- [ ] **Step 5: Add `arc4` to `getDefaultFormat` (returns ELF)**

In `llvm/lib/TargetParser/Triple.cpp` in the `getDefaultFormat` function, add `arc4` to the ELF case list alongside `arc`:

```cpp
  case Triple::arc:
  case Triple::arc4:
  case Triple::armeb:
```

- [ ] **Step 6: Add `arc4` to pointer width function (32-bit)**

In `llvm/lib/TargetParser/Triple.cpp` in the function that returns pointer widths, add `arc4` to the 32-bit case block alongside `arc`:

```cpp
  case llvm::Triple::arc:
  case llvm::Triple::arc4:
  case llvm::Triple::arm:
```

- [ ] **Step 7: Add `arc4` to `get32BitArchVariant` (already 32-bit)**

In `llvm/lib/TargetParser/Triple.cpp` in `get32BitArchVariant`, add to the "Already 32-bit" block:

```cpp
  case Triple::arc:
  case Triple::arc4:
  case Triple::arm:
```

- [ ] **Step 8: Add `arc4` to `get64BitArchVariant` (no 64-bit variant)**

In `llvm/lib/TargetParser/Triple.cpp` in `get64BitArchVariant`, add to the "set UnknownArch" block:

```cpp
  case Triple::arc:
  case Triple::arc4:
  case Triple::avr:
```

- [ ] **Step 9: Add `arc4` to `getBigEndianArchVariant` (no big-endian variant)**

In `llvm/lib/TargetParser/Triple.cpp` in `getBigEndianArchVariant`, add `arc4` to the list of little-endian-only arches that return UnknownArch:

```cpp
  case Triple::avr:
  case Triple::arc4:
  case Triple::dxil:
```

- [ ] **Step 10: Add `arc4` to `getDefaultExceptionHandling` (DwarfCFI)**

In `llvm/lib/TargetParser/Triple.cpp` in `getDefaultExceptionHandling`, add to the DwarfCFI list:

```cpp
  case Triple::arc:
  case Triple::arc4:
  case Triple::csky:
```

- [ ] **Step 11: Build and verify Triple parses correctly**

Run:
```bash
cd /home/tomato/projects/llvm-project
cmake --build build --target LLVMTargetParser 2>&1 | tail -20
```
Expected: Clean build with no errors.

- [ ] **Step 12: Commit**

```bash
git add llvm/include/llvm/TargetParser/Triple.h llvm/lib/TargetParser/Triple.cpp
git commit -m "feat(ARC4): register arc4 triple for ARCtangent-A4"
```

---

## Task 2: Create ARC4 Target Skeleton and TargetInfo Registration

**Files:**
- Create: `llvm/lib/Target/ARC4/CMakeLists.txt`
- Create: `llvm/lib/Target/ARC4/TargetInfo/ARC4TargetInfo.h`
- Create: `llvm/lib/Target/ARC4/TargetInfo/ARC4TargetInfo.cpp`
- Create: `llvm/lib/Target/ARC4/TargetInfo/CMakeLists.txt`
- Modify: `llvm/CMakeLists.txt:541` (add to experimental targets)

- [ ] **Step 1: Add ARC4 to experimental targets list**

In `llvm/CMakeLists.txt`, add `ARC4` to `LLVM_ALL_EXPERIMENTAL_TARGETS`:

```cmake
set(LLVM_ALL_EXPERIMENTAL_TARGETS
  ARC
  ARC4
  CSKY
  DirectX
  M68k
  Xtensa
)
```

- [ ] **Step 2: Create TargetInfo header**

Create `llvm/lib/Target/ARC4/TargetInfo/ARC4TargetInfo.h`:

```cpp
//===-- ARC4TargetInfo.h - ARC4 Target Implementation -----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ARC4_TARGETINFO_ARC4TARGETINFO_H
#define LLVM_LIB_TARGET_ARC4_TARGETINFO_ARC4TARGETINFO_H

namespace llvm {

class Target;

Target &getTheARC4Target();

} // namespace llvm

#endif // LLVM_LIB_TARGET_ARC4_TARGETINFO_ARC4TARGETINFO_H
```

- [ ] **Step 3: Create TargetInfo implementation**

Create `llvm/lib/Target/ARC4/TargetInfo/ARC4TargetInfo.cpp`:

```cpp
//===-- ARC4TargetInfo.cpp - ARC4 Target Implementation -------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "TargetInfo/ARC4TargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"

using namespace llvm;

Target &llvm::getTheARC4Target() {
  static Target TheARC4Target;
  return TheARC4Target;
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeARC4TargetInfo() {
  RegisterTarget<Triple::arc4> X(getTheARC4Target(), "arc4",
                                 "ARCtangent-A4", "ARC4");
}
```

- [ ] **Step 4: Create TargetInfo CMakeLists.txt**

Create `llvm/lib/Target/ARC4/TargetInfo/CMakeLists.txt`:

```cmake
add_llvm_component_library(LLVMARC4Info
  ARC4TargetInfo.cpp

  LINK_COMPONENTS
  MC
  Support

  ADD_TO_COMPONENT
  ARC4
  )
```

- [ ] **Step 5: Create top-level ARC4 CMakeLists.txt (minimal)**

Create `llvm/lib/Target/ARC4/CMakeLists.txt`:

```cmake
add_llvm_component_group(ARC4)

add_subdirectory(TargetInfo)
```

- [ ] **Step 6: Build and verify target registration**

Run:
```bash
cd /home/tomato/projects/llvm-project
cmake -G Ninja build -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD="ARC4" -DLLVM_TARGETS_TO_BUILD="" -DLLVM_ENABLE_PROJECTS="" 2>&1 | tail -20
cmake --build build --target LLVMARC4Info 2>&1 | tail -20
```
Expected: Clean build.

- [ ] **Step 7: Commit**

```bash
git add llvm/CMakeLists.txt llvm/lib/Target/ARC4/
git commit -m "feat(ARC4): add target skeleton and TargetInfo registration"
```

---

## Task 3: TableGen Register Definitions

**Files:**
- Create: `llvm/lib/Target/ARC4/ARC4RegisterInfo.td`
- Create: `llvm/lib/Target/ARC4/ARC4.td`

- [ ] **Step 1: Create register definitions**

Create `llvm/lib/Target/ARC4/ARC4RegisterInfo.td`:

```tablegen
//===-- ARC4RegisterInfo.td - ARC4 Register defs -----------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// General-purpose registers r0-r31
class ARC4Reg<bits<6> enc, string n, list<string> alt = []>
    : Register<n> {
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

// GPR32 - all allocatable general purpose registers
// Allocation order: r2-r12 (temporaries first), r0-r1 (args/return),
// r13-r25 (callee-saved), gp, fp, sp (fixed)
def GPR32 : RegisterClass<"ARC4", [i32], 32,
  (add R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12,
       R0, R1, R13, R14, R15, R16, R17, R18, R19, R20, R21, R22, R23, R24, R25,
       GP, FP, SP, BLINK)> {
  let RegInfos = RegInfoByHwMode<[], [RegInfo<32, 32, 32>]>;
}

// GPR32NoSP - same as GPR32 but without the stack pointer
def GPR32NoSP : RegisterClass<"ARC4", [i32], 32,
  (sub GPR32, SP)>;

// LP - loop count register (separate class, not allocatable)
def LP : RegisterClass<"ARC4", [i32], 32, (add LP_COUNT)> {
  let isAllocatable = 0;
}
```

- [ ] **Step 2: Create top-level TableGen file**

Create `llvm/lib/Target/ARC4/ARC4.td`:

```tablegen
//===- ARC4.td - Describe the ARC4 Target Machine ---------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

include "llvm/Target/Target.td"

//===----------------------------------------------------------------------===//
// Register File
//===----------------------------------------------------------------------===//

include "ARC4RegisterInfo.td"

//===----------------------------------------------------------------------===//
// Instruction Descriptions
//===----------------------------------------------------------------------===//

include "ARC4InstrInfo.td"

//===----------------------------------------------------------------------===//
// ARC4 processors supported
//===----------------------------------------------------------------------===//

def : ProcessorModel<"generic", NoSchedModel, []>;
def : ProcessorModel<"arc4", NoSchedModel, []>;

def ARC4InstrInfo : InstrInfo;

def ARC4InstPrinter : AsmWriter {
  string AsmWriterClassName = "InstPrinter";
  bit isMCAsmWriter = 1;
}

def ARC4AsmParser : AsmParser;

//===----------------------------------------------------------------------===//
// Target Declaration
//===----------------------------------------------------------------------===//

def ARC4 : Target {
  let InstructionSet = ARC4InstrInfo;
  let AssemblyWriters = [ARC4InstPrinter];
  let AssemblyParsers = [ARC4AsmParser];
}
```

- [ ] **Step 3: Create stub instruction info (needed for tablegen)**

Create `llvm/lib/Target/ARC4/ARC4InstrInfo.td`:

```tablegen
//===-- ARC4InstrInfo.td - ARC4 Instruction defs -----------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

include "ARC4InstrFormats.td"

//===----------------------------------------------------------------------===//
// NOP Instruction
//===----------------------------------------------------------------------===//

// nop is encoded as 0x7fffffff (a specific xor pattern)
let hasSideEffects = 0, mayLoad = 0, mayStore = 0, isBarrier = 0 in
def NOP : ARC4Inst<(outs), (ins), "nop", []> {
  let Inst{31-0} = 0x7fffffff;
}
```

- [ ] **Step 4: Create stub instruction formats**

Create `llvm/lib/Target/ARC4/ARC4InstrFormats.td`:

```tablegen
//===-- ARC4InstrFormats.td - ARC4 Instruction Formats -----*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// ARC4 instruction formats. All instructions are 32-bit.
//
// Encoding reference: binutils-2.15/opcodes/arc-opc.c
//   I[31:27] = major opcode (5 bits)
//   A[26:21] = register A / destination (6 bits)
//   B[20:15] = register B / source 1 (6 bits)
//   C[14:9]  = register C / source 2 (6 bits)
//   [8:0]    = flags/shimm (9 bits)
//
//===----------------------------------------------------------------------===//

// Base class for all ARC4 instructions
class ARC4Inst<dag outs, dag ins, string asmstr, list<dag> pattern>
    : Instruction {
  let Namespace = "ARC4";
  let Size = 4;

  field bits<32> Inst;
  field bits<32> SoftFail = 0;

  dag OutOperandList = outs;
  dag InOperandList = ins;
  let AsmString = asmstr;
  let Pattern = pattern;
}

// 3-operand ALU format: op.q.f a, b, c
//   I[31:27] = opcode
//   A[26:21] = destination register
//   B[20:15] = source register 1
//   C[14:9]  = source register 2
//   F[8]     = flag update bit
//   [7:5]    = reserved/condition
//   Q[4:0]   = condition code
class ARC4InstALU3<bits<5> opcode, dag outs, dag ins, string asmstr,
                   list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  bits<6> a;
  bits<6> b;
  bits<6> c;

  let Inst{31-27} = opcode;
  let Inst{26-21} = a;
  let Inst{20-15} = b;
  let Inst{14-9}  = c;
  let Inst{8-0}   = 0; // default: no flags, no condition
}

// Single operand format (opcode 3, sub-op in C field):
//   I[31:27] = 0b00011 (opcode 3)
//   A[26:21] = destination register
//   B[20:15] = source register
//   C[14:9]  = sub-opcode
class ARC4InstSOP<bits<6> subop, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  bits<6> a;
  bits<6> b;

  let Inst{31-27} = 0b00011; // opcode 3
  let Inst{26-21} = a;
  let Inst{20-15} = b;
  let Inst{14-9}  = subop;
  let Inst{8-0}   = 0;
}
```

- [ ] **Step 5: Update top-level CMakeLists.txt to run tablegen**

Update `llvm/lib/Target/ARC4/CMakeLists.txt`:

```cmake
add_llvm_component_group(ARC4)

set(LLVM_TARGET_DEFINITIONS ARC4.td)

tablegen(LLVM ARC4GenAsmMatcher.inc -gen-asm-matcher)
tablegen(LLVM ARC4GenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM ARC4GenInstrInfo.inc -gen-instr-info)
tablegen(LLVM ARC4GenMCCodeEmitter.inc -gen-emitter)
tablegen(LLVM ARC4GenRegisterInfo.inc -gen-register-info)
tablegen(LLVM ARC4GenSubtargetInfo.inc -gen-subtarget)

add_public_tablegen_target(ARC4CommonTableGen)

add_subdirectory(MCTargetDesc)
add_subdirectory(TargetInfo)
```

- [ ] **Step 6: Build and verify tablegen produces .inc files**

Run:
```bash
cd /home/tomato/projects/llvm-project
cmake -G Ninja build -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD="ARC4" -DLLVM_TARGETS_TO_BUILD="" -DLLVM_ENABLE_PROJECTS="" 2>&1 | tail -20
cmake --build build --target ARC4CommonTableGen 2>&1 | tail -20
```
Expected: TableGen produces `ARC4GenRegisterInfo.inc`, `ARC4GenInstrInfo.inc`, etc.

- [ ] **Step 7: Commit**

```bash
git add llvm/lib/Target/ARC4/ARC4.td llvm/lib/Target/ARC4/ARC4RegisterInfo.td \
        llvm/lib/Target/ARC4/ARC4InstrFormats.td llvm/lib/Target/ARC4/ARC4InstrInfo.td \
        llvm/lib/Target/ARC4/CMakeLists.txt
git commit -m "feat(ARC4): add TableGen register and instruction format definitions"
```

---

## Task 4: MC Layer — MCAsmInfo, MCTargetDesc, MCCodeEmitter

**Files:**
- Create: `llvm/lib/Target/ARC4/MCTargetDesc/ARC4MCAsmInfo.h`
- Create: `llvm/lib/Target/ARC4/MCTargetDesc/ARC4MCAsmInfo.cpp`
- Create: `llvm/lib/Target/ARC4/MCTargetDesc/ARC4MCTargetDesc.h`
- Create: `llvm/lib/Target/ARC4/MCTargetDesc/ARC4MCTargetDesc.cpp`
- Create: `llvm/lib/Target/ARC4/MCTargetDesc/ARC4MCCodeEmitter.cpp`
- Create: `llvm/lib/Target/ARC4/MCTargetDesc/ARC4FixupKinds.h`
- Create: `llvm/lib/Target/ARC4/MCTargetDesc/CMakeLists.txt`

- [ ] **Step 1: Create MCAsmInfo header**

Create `llvm/lib/Target/ARC4/MCTargetDesc/ARC4MCAsmInfo.h`:

```cpp
//===-- ARC4MCAsmInfo.h - ARC4 asm properties -----------------*- C++ -*---===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ARC4_MCTARGETDESC_ARC4MCASMINFO_H
#define LLVM_LIB_TARGET_ARC4_MCTARGETDESC_ARC4MCASMINFO_H

#include "llvm/MC/MCAsmInfoELF.h"

namespace llvm {
class Triple;

class ARC4MCAsmInfo : public MCAsmInfoELF {
  void anchor() override;

public:
  explicit ARC4MCAsmInfo(const Triple &TheTriple,
                         const MCTargetOptions &Options);
};

} // namespace llvm

#endif // LLVM_LIB_TARGET_ARC4_MCTARGETDESC_ARC4MCASMINFO_H
```

- [ ] **Step 2: Create MCAsmInfo implementation**

Create `llvm/lib/Target/ARC4/MCTargetDesc/ARC4MCAsmInfo.cpp`:

```cpp
//===-- ARC4MCAsmInfo.cpp - ARC4 asm properties ---------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ARC4MCAsmInfo.h"
#include "llvm/TargetParser/Triple.h"

using namespace llvm;

void ARC4MCAsmInfo::anchor() {}

ARC4MCAsmInfo::ARC4MCAsmInfo(const Triple & /*TheTriple*/,
                             const MCTargetOptions &Options) {
  IsLittleEndian = true;
  PrivateGlobalPrefix = ".L";
  WeakRefDirective = "\t.weak\t";
  CommentString = ";";
  UsesELFSectionDirectiveForBSS = true;
  SupportsDebugInformation = true;
  MinInstAlignment = 4;
  ExceptionsType = ExceptionHandling::DwarfCFI;
}
```

- [ ] **Step 3: Create fixup kinds**

Create `llvm/lib/Target/ARC4/MCTargetDesc/ARC4FixupKinds.h`:

```cpp
//===-- ARC4FixupKinds.h - ARC4 Fixup Entries -----------------*- C++ -*---===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ARC4_MCTARGETDESC_ARC4FIXUPKINDS_H
#define LLVM_LIB_TARGET_ARC4_MCTARGETDESC_ARC4FIXUPKINDS_H

#include "llvm/MC/MCFixup.h"

namespace llvm {
namespace ARC4 {

enum Fixups {
  // 26-bit absolute branch target (R_ARC_B26)
  // Encoded in bits [23:0] with 2-bit right shift
  fixup_arc4_b26 = FirstTargetFixupKind,

  // 22-bit PC-relative branch (R_ARC_B22_PCREL)
  // Encoded in bits [28:7] with 2-bit right shift
  fixup_arc4_b22_pcrel,

  // Marker
  fixup_arc4_invalid,
  NumTargetFixupKinds = fixup_arc4_invalid - FirstTargetFixupKind
};

} // namespace ARC4
} // namespace llvm

#endif // LLVM_LIB_TARGET_ARC4_MCTARGETDESC_ARC4FIXUPKINDS_H
```

- [ ] **Step 4: Create MCTargetDesc header**

Create `llvm/lib/Target/ARC4/MCTargetDesc/ARC4MCTargetDesc.h`:

```cpp
//===-- ARC4MCTargetDesc.h - ARC4 Target Descriptions ---------*- C++ -*---===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ARC4_MCTARGETDESC_ARC4MCTARGETDESC_H
#define LLVM_LIB_TARGET_ARC4_MCTARGETDESC_ARC4MCTARGETDESC_H

#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCTargetOptions.h"
#include "llvm/Support/DataTypes.h"

namespace llvm {
class MCAsmBackend;
class MCCodeEmitter;
class MCContext;
class MCInstrInfo;
class MCObjectTargetWriter;
class MCSubtargetInfo;
class Target;

MCCodeEmitter *createARC4MCCodeEmitter(const MCInstrInfo &MCII,
                                       MCContext &Ctx);

MCAsmBackend *createARC4AsmBackend(const Target &T,
                                   const MCSubtargetInfo &STI,
                                   const MCRegisterInfo &MRI,
                                   const MCTargetOptions &Options);

std::unique_ptr<MCObjectTargetWriter> createARC4ELFObjectWriter(uint8_t OSABI);
} // namespace llvm

// Defines symbolic names for ARC4 registers.
#define GET_REGINFO_ENUM
#include "ARC4GenRegisterInfo.inc"

// Defines symbolic names for the ARC4 instructions.
#define GET_INSTRINFO_ENUM
#define GET_INSTRINFO_MC_HELPER_DECLS
#include "ARC4GenInstrInfo.inc"

#define GET_SUBTARGETINFO_ENUM
#include "ARC4GenSubtargetInfo.inc"

#endif // LLVM_LIB_TARGET_ARC4_MCTARGETDESC_ARC4MCTARGETDESC_H
```

- [ ] **Step 5: Create MCTargetDesc implementation**

Create `llvm/lib/Target/ARC4/MCTargetDesc/ARC4MCTargetDesc.cpp`:

```cpp
//===-- ARC4MCTargetDesc.cpp - ARC4 Target Descriptions -------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ARC4MCTargetDesc.h"
#include "ARC4InstPrinter.h"
#include "ARC4MCAsmInfo.h"
#include "TargetInfo/ARC4TargetInfo.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"
#include "llvm/TargetParser/Triple.h"

#define GET_INSTRINFO_MC_DESC
#define ENABLE_INSTR_PREDICATE_VERIFIER
#include "ARC4GenInstrInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "ARC4GenSubtargetInfo.inc"

#define GET_REGINFO_MC_DESC
#include "ARC4GenRegisterInfo.inc"

using namespace llvm;

static MCInstrInfo *createARC4MCInstrInfo() {
  MCInstrInfo *X = new MCInstrInfo();
  InitARC4MCInstrInfo(X);
  return X;
}

static MCRegisterInfo *createARC4MCRegisterInfo(const Triple & /*TT*/) {
  MCRegisterInfo *X = new MCRegisterInfo();
  InitARC4MCRegisterInfo(X, ARC4::BLINK);
  return X;
}

static MCSubtargetInfo *
createARC4MCSubtargetInfo(const Triple &TT, StringRef CPU, StringRef FS) {
  std::string CPUName = std::string(CPU);
  if (CPUName.empty())
    CPUName = "generic";
  return createARC4MCSubtargetInfoImpl(TT, CPUName, /*TuneCPU*/ CPUName, FS);
}

static MCStreamer *createARC4MCStreamer(const Triple &T, MCContext &Context,
                                       std::unique_ptr<MCAsmBackend> &&MAB,
                                       std::unique_ptr<MCObjectWriter> &&OW,
                                       std::unique_ptr<MCCodeEmitter> &&Emitter) {
  if (!T.isOSBinFormatELF())
    llvm_unreachable("OS not supported");
  return createELFStreamer(Context, std::move(MAB), std::move(OW),
                           std::move(Emitter));
}

static MCInstPrinter *createARC4MCInstPrinter(const Triple & /*T*/,
                                              unsigned SyntaxVariant,
                                              const MCAsmInfo &MAI,
                                              const MCInstrInfo &MII,
                                              const MCRegisterInfo &MRI) {
  if (SyntaxVariant == 0)
    return new ARC4InstPrinter(MAI, MII, MRI);
  return nullptr;
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeARC4TargetMC() {
  RegisterMCAsmInfo<ARC4MCAsmInfo> X(getTheARC4Target());

  TargetRegistry::RegisterMCInstrInfo(getTheARC4Target(),
                                      createARC4MCInstrInfo);
  TargetRegistry::RegisterMCRegInfo(getTheARC4Target(),
                                    createARC4MCRegisterInfo);
  TargetRegistry::RegisterMCSubtargetInfo(getTheARC4Target(),
                                          createARC4MCSubtargetInfo);
  TargetRegistry::RegisterMCCodeEmitter(getTheARC4Target(),
                                        createARC4MCCodeEmitter);
  TargetRegistry::RegisterMCAsmBackend(getTheARC4Target(),
                                       createARC4AsmBackend);
  TargetRegistry::RegisterMCInstPrinter(getTheARC4Target(),
                                        createARC4MCInstPrinter);
  TargetRegistry::RegisterELFStreamer(getTheARC4Target(),
                                     createARC4MCStreamer);
}
```

- [ ] **Step 6: Create MCCodeEmitter**

Create `llvm/lib/Target/ARC4/MCTargetDesc/ARC4MCCodeEmitter.cpp`:

```cpp
//===-- ARC4MCCodeEmitter.cpp - Convert ARC4 code to machine code ---------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ARC4FixupKinds.h"
#include "ARC4MCTargetDesc.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/EndianStream.h"

#define DEBUG_TYPE "mccodeemitter"

STATISTIC(MCNumEmitted, "Number of MC instructions emitted");

namespace llvm {

namespace {

class ARC4MCCodeEmitter : public MCCodeEmitter {
  const MCInstrInfo &MCII;
  MCContext &Ctx;

public:
  ARC4MCCodeEmitter(const MCInstrInfo &MCII, MCContext &Ctx)
      : MCII(MCII), Ctx(Ctx) {}

  // TableGen'erated function
  uint64_t getBinaryCodeForInstr(const MCInst &Inst,
                                 SmallVectorImpl<MCFixup> &Fixups,
                                 const MCSubtargetInfo &STI) const;

  unsigned getMachineOpValue(const MCInst &Inst, const MCOperand &MO,
                             SmallVectorImpl<MCFixup> &Fixups,
                             const MCSubtargetInfo &STI) const;

  void encodeInstruction(const MCInst &Inst, SmallVectorImpl<char> &CB,
                         SmallVectorImpl<MCFixup> &Fixups,
                         const MCSubtargetInfo &STI) const override;
};

} // end anonymous namespace

unsigned ARC4MCCodeEmitter::getMachineOpValue(const MCInst &Inst,
                                              const MCOperand &MO,
                                              SmallVectorImpl<MCFixup> &Fixups,
                                              const MCSubtargetInfo &STI) const {
  if (MO.isReg())
    return Ctx.getRegisterInfo()->getEncodingValue(MO.getReg());
  if (MO.isImm())
    return static_cast<unsigned>(MO.getImm());

  assert(MO.isExpr());
  Fixups.push_back(
      MCFixup::create(0, MO.getExpr(), MCFixupKind(FK_Data_4)));
  return 0;
}

void ARC4MCCodeEmitter::encodeInstruction(const MCInst &Inst,
                                           SmallVectorImpl<char> &CB,
                                           SmallVectorImpl<MCFixup> &Fixups,
                                           const MCSubtargetInfo &STI) const {
  uint64_t Value = getBinaryCodeForInstr(Inst, Fixups, STI);
  ++MCNumEmitted;

  // ARC4 is little-endian, 32-bit fixed-width instructions
  support::endian::write<uint32_t>(CB, Value, llvm::endianness::little);
}

MCCodeEmitter *createARC4MCCodeEmitter(const MCInstrInfo &MCII,
                                       MCContext &Ctx) {
  return new ARC4MCCodeEmitter(MCII, Ctx);
}

} // namespace llvm

#include "ARC4GenMCCodeEmitter.inc"
```

- [ ] **Step 7: Create MCTargetDesc CMakeLists.txt**

Create `llvm/lib/Target/ARC4/MCTargetDesc/CMakeLists.txt`:

```cmake
add_llvm_component_library(LLVMARC4Desc
  ARC4MCAsmInfo.cpp
  ARC4MCTargetDesc.cpp
  ARC4MCCodeEmitter.cpp
  ARC4AsmBackend.cpp
  ARC4ELFObjectWriter.cpp
  ARC4InstPrinter.cpp

  LINK_COMPONENTS
  ARC4Info
  MC
  MCDisassembler
  Support
  TargetParser

  ADD_TO_COMPONENT
  ARC4
  )
```

- [ ] **Step 8: Build MC layer (will fail until AsmBackend, ELFObjectWriter, and InstPrinter exist — that's expected)**

Note: This step is a checkpoint. The actual build will succeed after Task 5 completes.

- [ ] **Step 9: Commit**

```bash
git add llvm/lib/Target/ARC4/MCTargetDesc/
git commit -m "feat(ARC4): add MCAsmInfo, MCTargetDesc, MCCodeEmitter, and fixup kinds"
```

---

## Task 5: MC Layer — AsmBackend, ELF Object Writer, InstPrinter

**Files:**
- Create: `llvm/lib/Target/ARC4/MCTargetDesc/ARC4AsmBackend.cpp`
- Create: `llvm/lib/Target/ARC4/MCTargetDesc/ARC4ELFObjectWriter.cpp`
- Create: `llvm/lib/Target/ARC4/MCTargetDesc/ARC4InstPrinter.h`
- Create: `llvm/lib/Target/ARC4/MCTargetDesc/ARC4InstPrinter.cpp`
- Create: `llvm/include/llvm/BinaryFormat/ELFRelocs/ARC4.def`

- [ ] **Step 1: Create ARC4 ELF relocation definitions**

Create `llvm/include/llvm/BinaryFormat/ELFRelocs/ARC4.def`:

```cpp

#ifndef ELF_RELOC
#error "ELF_RELOC must be defined"
#endif

// Relocation types from binutils-2.15/include/elf/arc.h
// Only the A4-relevant subset
ELF_RELOC(R_ARC4_NONE,         0x0)
ELF_RELOC(R_ARC4_8,            0x1)
ELF_RELOC(R_ARC4_16,           0x2)
ELF_RELOC(R_ARC4_24,           0x3)
ELF_RELOC(R_ARC4_32,           0x4)
ELF_RELOC(R_ARC4_B26,          0x5)
ELF_RELOC(R_ARC4_B22_PCREL,    0x6)
ELF_RELOC(R_ARC4_H30,          0x7)
ELF_RELOC(R_ARC4_N8,           0x8)
ELF_RELOC(R_ARC4_N16,          0x9)
ELF_RELOC(R_ARC4_N24,          0xA)
ELF_RELOC(R_ARC4_N32,          0xB)
ELF_RELOC(R_ARC4_SDA,          0xC)
ELF_RELOC(R_ARC4_SECTOFF,      0xD)
```

- [ ] **Step 2: Create AsmBackend**

Create `llvm/lib/Target/ARC4/MCTargetDesc/ARC4AsmBackend.cpp`:

```cpp
//===-- ARC4AsmBackend.cpp - ARC4 Asm Backend ----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ARC4FixupKinds.h"
#include "ARC4MCTargetDesc.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCFixupKindInfo.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/Compiler.h"

using namespace llvm;

namespace {

class ARC4AsmBackend : public MCAsmBackend {
  uint8_t OSABI;

public:
  ARC4AsmBackend(uint8_t OSABI)
      : MCAsmBackend(llvm::endianness::little), OSABI(OSABI) {}

  unsigned getNumFixupKinds() const override {
    return ARC4::NumTargetFixupKinds;
  }

  MCFixupKindInfo getFixupKindInfo(MCFixupKind Kind) const override {
    const static MCFixupKindInfo Infos[ARC4::NumTargetFixupKinds] = {
        // name, offset, bits, flags
        {"fixup_arc4_b26", 0, 24, 0},
        {"fixup_arc4_b22_pcrel", 7, 22, MCFixupKindInfo::FKF_IsPCRel},
    };
    if (Kind < FirstTargetFixupKind)
      return MCAsmBackend::getFixupKindInfo(Kind);
    assert(unsigned(Kind - FirstTargetFixupKind) < getNumFixupKinds() &&
           "Invalid kind!");
    return Infos[Kind - FirstTargetFixupKind];
  }

  void applyFixup(const MCAssembler &Asm, const MCFixup &Fixup,
                  const MCValue &Target, MutableArrayRef<char> Data,
                  uint64_t Value, bool IsResolved,
                  const MCSubtargetInfo *STI) const override {
    MCFixupKind Kind = Fixup.getKind();
    if (Kind >= FirstTargetFixupKind) {
      Kind = static_cast<MCFixupKind>(static_cast<unsigned>(Kind));
    }

    unsigned Offset = Fixup.getOffset();
    unsigned NumBytes = 4; // All ARC4 instructions are 4 bytes

    assert(Offset + NumBytes <= Data.size() && "Invalid fixup offset!");

    uint32_t CurVal = 0;
    for (unsigned i = 0; i < NumBytes; ++i)
      CurVal |= static_cast<uint32_t>(static_cast<uint8_t>(Data[Offset + i]))
                << (i * 8);

    uint32_t Mask = 0;
    switch (Fixup.getTargetKind()) {
    default:
      break;
    case FK_Data_1:
      Data[Offset] = Value;
      return;
    case FK_Data_2:
      support::endian::write<uint16_t>(&Data[Offset], Value,
                                        llvm::endianness::little);
      return;
    case FK_Data_4:
      support::endian::write<uint32_t>(&Data[Offset], Value,
                                        llvm::endianness::little);
      return;
    case ARC4::fixup_arc4_b26:
      // 26-bit branch: value >> 2, placed in bits [23:0]
      Value >>= 2;
      Mask = 0x00ffffff;
      break;
    case ARC4::fixup_arc4_b22_pcrel:
      // 22-bit PC-relative: value >> 2, placed in bits [28:7]
      Value >>= 2;
      Value <<= 7;
      Mask = 0x1fffff80;
      break;
    }

    CurVal = (CurVal & ~Mask) | (Value & Mask);
    support::endian::write<uint32_t>(&Data[Offset], CurVal,
                                      llvm::endianness::little);
  }

  bool fixupNeedsRelaxation(const MCFixup &Fixup,
                            uint64_t Value) const override {
    return false; // No relaxation for fixed-width ISA
  }

  bool writeNopData(raw_ostream &OS, uint64_t Count,
                    const MCSubtargetInfo *STI) const override {
    // NOP is 0x7fffffff (little-endian: ff ff ff 7f)
    uint64_t NumNops = Count / 4;
    for (uint64_t i = 0; i < NumNops; ++i)
      support::endian::write<uint32_t>(OS, 0x7fffffff,
                                        llvm::endianness::little);
    // Fill remaining bytes with zeros
    for (uint64_t i = 0; i < Count % 4; ++i)
      OS.write('\0');
    return true;
  }

  std::unique_ptr<MCObjectTargetWriter>
  createObjectTargetWriter() const override {
    return createARC4ELFObjectWriter(OSABI);
  }
};

} // end anonymous namespace

MCAsmBackend *llvm::createARC4AsmBackend(const Target &T,
                                         const MCSubtargetInfo &STI,
                                         const MCRegisterInfo &MRI,
                                         const MCTargetOptions &Options) {
  uint8_t OSABI =
      MCELFObjectTargetWriter::getOSABI(STI.getTargetTriple().getOS());
  return new ARC4AsmBackend(OSABI);
}
```

- [ ] **Step 3: Create ELF Object Writer**

Create `llvm/lib/Target/ARC4/MCTargetDesc/ARC4ELFObjectWriter.cpp`:

```cpp
//===-- ARC4ELFObjectWriter.cpp - ARC4 ELF Writer -------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ARC4FixupKinds.h"
#include "ARC4MCTargetDesc.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

namespace {

class ARC4ELFObjectWriter : public MCELFObjectTargetWriter {
public:
  explicit ARC4ELFObjectWriter(uint8_t OSABI)
      : MCELFObjectTargetWriter(/*Is64Bit=*/false, OSABI, ELF::EM_ARC,
                                /*HasRelocationAddend=*/true) {}

  unsigned getRelocType(MCContext &Ctx, const MCValue &Target,
                        const MCFixup &Fixup, bool IsPCRel) const override {
    unsigned Kind = Fixup.getTargetKind();
    switch (Kind) {
    default:
      llvm_unreachable("Invalid fixup kind!");
    case FK_Data_4:
      return ELF::R_ARC_32;
    case ARC4::fixup_arc4_b26:
      return ELF::R_ARC_B26;
    case ARC4::fixup_arc4_b22_pcrel:
      return ELF::R_ARC_B22_PCREL;
    }
  }
};

} // end anonymous namespace

std::unique_ptr<MCObjectTargetWriter>
llvm::createARC4ELFObjectWriter(uint8_t OSABI) {
  return std::make_unique<ARC4ELFObjectWriter>(OSABI);
}
```

- [ ] **Step 4: Create InstPrinter header**

Create `llvm/lib/Target/ARC4/MCTargetDesc/ARC4InstPrinter.h`:

```cpp
//===-- ARC4InstPrinter.h - Convert ARC4 MCInst to asm --------*- C++ -*---===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ARC4_MCTARGETDESC_ARC4INSTPRINTER_H
#define LLVM_LIB_TARGET_ARC4_MCTARGETDESC_ARC4INSTPRINTER_H

#include "llvm/MC/MCInstPrinter.h"

namespace llvm {

class ARC4InstPrinter : public MCInstPrinter {
public:
  ARC4InstPrinter(const MCAsmInfo &MAI, const MCInstrInfo &MII,
                  const MCRegisterInfo &MRI)
      : MCInstPrinter(MAI, MII, MRI) {}

  void printInst(const MCInst *MI, uint64_t Address, StringRef Annot,
                 const MCSubtargetInfo &STI, raw_ostream &O) override;
  void printRegName(raw_ostream &O, MCRegister Reg) override;

  // Auto-generated by TableGen
  std::pair<const char *, uint64_t> getMnemonic(const MCInst &MI) const override;
  bool printAliasInstr(const MCInst *MI, uint64_t Address,
                       const MCSubtargetInfo &STI, raw_ostream &O);
  void printCustomAliasOperand(const MCInst *MI, uint64_t Address,
                               unsigned OpIdx, unsigned PrintMethodIdx,
                               const MCSubtargetInfo &STI, raw_ostream &O);
  void printInstruction(const MCInst *MI, uint64_t Address,
                        const MCSubtargetInfo &STI, raw_ostream &O);
  static const char *getRegisterName(MCRegister Reg);

  void printOperand(const MCInst *MI, unsigned OpNo,
                    const MCSubtargetInfo &STI, raw_ostream &O);
};

} // namespace llvm

#endif // LLVM_LIB_TARGET_ARC4_MCTARGETDESC_ARC4INSTPRINTER_H
```

- [ ] **Step 5: Create InstPrinter implementation**

Create `llvm/lib/Target/ARC4/MCTargetDesc/ARC4InstPrinter.cpp`:

```cpp
//===-- ARC4InstPrinter.cpp - Convert ARC4 MCInst to asm ------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ARC4InstPrinter.h"
#include "ARC4MCTargetDesc.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

#define DEBUG_TYPE "asm-printer"

// Include the auto-generated portion of the assembly writer.
#define PRINT_ALIAS_INSTR
#include "ARC4GenAsmWriter.inc"

void ARC4InstPrinter::printRegName(raw_ostream &O, MCRegister Reg) {
  O << getRegisterName(Reg);
}

void ARC4InstPrinter::printInst(const MCInst *MI, uint64_t Address,
                                StringRef Annot, const MCSubtargetInfo &STI,
                                raw_ostream &O) {
  if (!printAliasInstr(MI, Address, STI, O))
    printInstruction(MI, Address, STI, O);
  printAnnotation(O, Annot);
}

void ARC4InstPrinter::printOperand(const MCInst *MI, unsigned OpNo,
                                   const MCSubtargetInfo &STI,
                                   raw_ostream &O) {
  const MCOperand &Op = MI->getOperand(OpNo);
  if (Op.isReg())
    printRegName(O, Op.getReg());
  else if (Op.isImm())
    O << Op.getImm();
  else if (Op.isExpr())
    Op.getExpr()->print(O, &MAI);
}
```

- [ ] **Step 6: Add ELF relocation types to LLVM's ELF.h (if needed)**

Check if `R_ARC_B26` and `R_ARC_B22_PCREL` are already defined in the existing `ARC.def`. If not, we need to add them. The existing `ARC.def` file does NOT include `R_ARC_B26` or `R_ARC_B22_PCREL` — these are ARC4-specific relocs not in the ARCompact set. We will use our own `ARC4.def` file and reference the numeric values directly in the ELF object writer for now. The `ELF::R_ARC_32` (value 4) IS already defined since it's shared.

Update `ARC4ELFObjectWriter.cpp` to use numeric constants for the ARC4-specific relocs:

Replace the switch in the `getRelocType` function:

```cpp
    case FK_Data_4:
      return 4; // R_ARC_32
    case ARC4::fixup_arc4_b26:
      return 5; // R_ARC_B26
    case ARC4::fixup_arc4_b22_pcrel:
      return 6; // R_ARC_B22_PCREL
```

- [ ] **Step 7: Build and verify MC layer compiles**

Run:
```bash
cd /home/tomato/projects/llvm-project
cmake --build build --target LLVMARC4Desc 2>&1 | tail -30
```
Expected: Clean build.

- [ ] **Step 8: Commit**

```bash
git add llvm/lib/Target/ARC4/MCTargetDesc/ llvm/include/llvm/BinaryFormat/ELFRelocs/ARC4.def
git commit -m "feat(ARC4): add AsmBackend, ELF object writer, and InstPrinter"
```

---

## Task 6: Assembly Parser

**Files:**
- Create: `llvm/lib/Target/ARC4/AsmParser/ARC4AsmParser.cpp`
- Create: `llvm/lib/Target/ARC4/AsmParser/CMakeLists.txt`
- Modify: `llvm/lib/Target/ARC4/CMakeLists.txt` (add AsmParser subdirectory)

- [ ] **Step 1: Create AsmParser CMakeLists.txt**

Create `llvm/lib/Target/ARC4/AsmParser/CMakeLists.txt`:

```cmake
add_llvm_component_library(LLVMARC4AsmParser
  ARC4AsmParser.cpp

  LINK_COMPONENTS
  ARC4Desc
  ARC4Info
  MC
  MCParser
  Support
  TargetParser

  ADD_TO_COMPONENT
  ARC4
  )
```

- [ ] **Step 2: Create AsmParser implementation**

Create `llvm/lib/Target/ARC4/AsmParser/ARC4AsmParser.cpp`:

```cpp
//===-- ARC4AsmParser.cpp - Parse ARC4 assembly to MCInst -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ARC4MCTargetDesc.h"
#include "TargetInfo/ARC4TargetInfo.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCParser/MCAsmLexer.h"
#include "llvm/MC/MCParser/MCAsmParser.h"
#include "llvm/MC/MCParser/MCParsedAsmOperand.h"
#include "llvm/MC/MCParser/MCTargetAsmParser.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/SMLoc.h"
#include "llvm/Support/raw_ostream.h"
#include <memory>

using namespace llvm;

// Auto-generated by TableGen
static MCRegister MatchRegisterName(StringRef Name);
static MCRegister MatchRegisterAltName(StringRef Name);

namespace {

struct ARC4Operand : public MCParsedAsmOperand {
  enum KindTy { Token, Register, Immediate, Memory } Kind;

  SMLoc StartLoc, EndLoc;

  union {
    StringRef Tok;
    struct {
      MCRegister RegNum;
    } Reg;
    struct {
      const MCExpr *Val;
    } Imm;
    struct {
      MCRegister Base;
      const MCExpr *Off;
    } Mem;
  };

  ARC4Operand(KindTy K) : Kind(K) {}

public:
  static std::unique_ptr<ARC4Operand> createToken(StringRef Str, SMLoc S) {
    auto Op = std::make_unique<ARC4Operand>(Token);
    Op->Tok = Str;
    Op->StartLoc = S;
    Op->EndLoc = S;
    return Op;
  }

  static std::unique_ptr<ARC4Operand> createReg(MCRegister RegNo, SMLoc S,
                                                 SMLoc E) {
    auto Op = std::make_unique<ARC4Operand>(Register);
    Op->Reg.RegNum = RegNo;
    Op->StartLoc = S;
    Op->EndLoc = E;
    return Op;
  }

  static std::unique_ptr<ARC4Operand> createImm(const MCExpr *Val, SMLoc S,
                                                 SMLoc E) {
    auto Op = std::make_unique<ARC4Operand>(Immediate);
    Op->Imm.Val = Val;
    Op->StartLoc = S;
    Op->EndLoc = E;
    return Op;
  }

  static std::unique_ptr<ARC4Operand> createMem(MCRegister Base,
                                                 const MCExpr *Off, SMLoc S,
                                                 SMLoc E) {
    auto Op = std::make_unique<ARC4Operand>(Memory);
    Op->Mem.Base = Base;
    Op->Mem.Off = Off;
    Op->StartLoc = S;
    Op->EndLoc = E;
    return Op;
  }

  bool isToken() const override { return Kind == Token; }
  bool isReg() const override { return Kind == Register; }
  bool isImm() const override { return Kind == Immediate; }
  bool isMem() const override { return Kind == Memory; }

  StringRef getToken() const {
    assert(Kind == Token);
    return Tok;
  }

  MCRegister getReg() const override {
    assert(Kind == Register);
    return Reg.RegNum;
  }

  const MCExpr *getImm() const {
    assert(Kind == Immediate);
    return Imm.Val;
  }

  SMLoc getStartLoc() const override { return StartLoc; }
  SMLoc getEndLoc() const override { return EndLoc; }

  void addRegOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands");
    Inst.addOperand(MCOperand::createReg(getReg()));
  }

  void addImmOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands");
    if (const auto *CE = dyn_cast<MCConstantExpr>(getImm()))
      Inst.addOperand(MCOperand::createImm(CE->getValue()));
    else
      Inst.addOperand(MCOperand::createExpr(getImm()));
  }

  void print(raw_ostream &OS) const override {
    switch (Kind) {
    case Token:
      OS << "Token: " << Tok;
      break;
    case Register:
      OS << "Register: " << Reg.RegNum;
      break;
    case Immediate:
      OS << "Immediate: ";
      Imm.Val->print(OS, nullptr);
      break;
    case Memory:
      OS << "Memory: [" << Mem.Base << "]";
      break;
    }
  }
};

class ARC4AsmParser : public MCTargetAsmParser {
  MCRegister parseRegister();
  bool parseOperand(OperandVector &Operands);

#define GET_ASSEMBLER_HEADER
#include "ARC4GenAsmMatcher.inc"

public:
  ARC4AsmParser(const MCSubtargetInfo &STI, MCAsmParser &Parser,
                const MCInstrInfo &MII, const MCTargetOptions &Options)
      : MCTargetAsmParser(Options, STI, MII) {
    setAvailableFeatures(ComputeAvailableFeatures(STI.getFeatureBits()));
  }

  bool parseRegister(MCRegister &Reg, SMLoc &StartLoc,
                     SMLoc &EndLoc) override;
  ParseStatus tryParseRegister(MCRegister &Reg, SMLoc &StartLoc,
                               SMLoc &EndLoc) override;
  bool ParseInstruction(ParseInstructionInfo &Info, StringRef Name,
                        SMLoc NameLoc, OperandVector &Operands) override;
  bool MatchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                               OperandVector &Operands, MCStreamer &Out,
                               uint64_t &ErrorInfo,
                               bool MatchingInlineAsm) override;
};

} // end anonymous namespace

MCRegister ARC4AsmParser::parseRegister() {
  const AsmToken &Tok = getParser().getTok();
  if (Tok.isNot(AsmToken::Identifier))
    return MCRegister();

  StringRef Name = Tok.getIdentifier();
  MCRegister Reg = MatchRegisterName(Name);
  if (!Reg)
    Reg = MatchRegisterAltName(Name);
  if (Reg)
    getParser().Lex(); // Eat register token
  return Reg;
}

bool ARC4AsmParser::parseRegister(MCRegister &Reg, SMLoc &StartLoc,
                                  SMLoc &EndLoc) {
  StartLoc = getLexer().getLoc();
  Reg = parseRegister();
  EndLoc = getLexer().getLoc();
  return !Reg;
}

ParseStatus ARC4AsmParser::tryParseRegister(MCRegister &Reg, SMLoc &StartLoc,
                                            SMLoc &EndLoc) {
  StartLoc = getLexer().getLoc();
  Reg = parseRegister();
  EndLoc = getLexer().getLoc();
  if (!Reg)
    return ParseStatus::NoMatch;
  return ParseStatus::Success;
}

bool ARC4AsmParser::parseOperand(OperandVector &Operands) {
  SMLoc Start = getLexer().getLoc();

  // Try register
  MCRegister Reg = parseRegister();
  if (Reg) {
    Operands.push_back(ARC4Operand::createReg(Reg, Start, getLexer().getLoc()));
    return false;
  }

  // Try immediate / expression
  const MCExpr *Expr;
  if (!getParser().parseExpression(Expr)) {
    Operands.push_back(
        ARC4Operand::createImm(Expr, Start, getLexer().getLoc()));
    return false;
  }

  return true;
}

bool ARC4AsmParser::ParseInstruction(ParseInstructionInfo &Info, StringRef Name,
                                     SMLoc NameLoc,
                                     OperandVector &Operands) {
  // Add the mnemonic as a token operand
  Operands.push_back(ARC4Operand::createToken(Name, NameLoc));

  // Parse operands
  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    if (parseOperand(Operands))
      return true;

    while (getLexer().is(AsmToken::Comma)) {
      getParser().Lex(); // Eat comma
      if (parseOperand(Operands))
        return true;
    }
  }

  if (getLexer().isNot(AsmToken::EndOfStatement))
    return Error(getLexer().getLoc(), "unexpected token in operand list");

  getParser().Lex(); // Eat EndOfStatement
  return false;
}

bool ARC4AsmParser::MatchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                                            OperandVector &Operands,
                                            MCStreamer &Out,
                                            uint64_t &ErrorInfo,
                                            bool MatchingInlineAsm) {
  MCInst Inst;
  switch (MatchInstructionImpl(Operands, Inst, ErrorInfo, MatchingInlineAsm)) {
  case Match_Success:
    Out.emitInstruction(Inst, getSTI());
    return false;
  case Match_MissingFeature:
    return Error(IDLoc, "instruction requires a CPU feature not currently enabled");
  case Match_MnemonicFail:
    return Error(IDLoc, "unrecognized instruction mnemonic");
  case Match_InvalidOperand: {
    SMLoc ErrorLoc = IDLoc;
    if (ErrorInfo != ~0ULL && ErrorInfo < Operands.size())
      ErrorLoc = Operands[ErrorInfo]->getStartLoc();
    return Error(ErrorLoc, "invalid operand for instruction");
  }
  }
  llvm_unreachable("Unexpected match result");
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeARC4AsmParser() {
  RegisterMCAsmParser<ARC4AsmParser> X(getTheARC4Target());
}

#define GET_REGISTER_MATCHER
#define GET_MATCHER_IMPLEMENTATION
#include "ARC4GenAsmMatcher.inc"
```

- [ ] **Step 3: Update top-level CMakeLists.txt to add AsmParser subdirectory**

Update `llvm/lib/Target/ARC4/CMakeLists.txt`:

```cmake
add_llvm_component_group(ARC4)

set(LLVM_TARGET_DEFINITIONS ARC4.td)

tablegen(LLVM ARC4GenAsmMatcher.inc -gen-asm-matcher)
tablegen(LLVM ARC4GenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM ARC4GenInstrInfo.inc -gen-instr-info)
tablegen(LLVM ARC4GenMCCodeEmitter.inc -gen-emitter)
tablegen(LLVM ARC4GenRegisterInfo.inc -gen-register-info)
tablegen(LLVM ARC4GenSubtargetInfo.inc -gen-subtarget)

add_public_tablegen_target(ARC4CommonTableGen)

add_subdirectory(AsmParser)
add_subdirectory(MCTargetDesc)
add_subdirectory(TargetInfo)
```

- [ ] **Step 4: Build and test that llvm-mc recognizes the target**

Run:
```bash
cd /home/tomato/projects/llvm-project
cmake --build build --target llvm-mc 2>&1 | tail -30
echo "nop" | ./build/bin/llvm-mc -triple=arc4-unknown-elf -show-encoding 2>&1
```
Expected: `llvm-mc` recognizes `arc4` triple and can encode `nop` instruction.

- [ ] **Step 5: Commit**

```bash
git add llvm/lib/Target/ARC4/AsmParser/ llvm/lib/Target/ARC4/CMakeLists.txt
git commit -m "feat(ARC4): add assembly parser for llvm-mc"
```

---

## Task 7: Add ALU and Single-Operand Instructions to TableGen

**Files:**
- Modify: `llvm/lib/Target/ARC4/ARC4InstrFormats.td`
- Modify: `llvm/lib/Target/ARC4/ARC4InstrInfo.td`

This task adds the core ALU instructions. Reference: `binutils-2.15/opcodes/arc-opc.c` opcode table.

- [ ] **Step 1: Add ALU instruction definitions**

Update `llvm/lib/Target/ARC4/ARC4InstrInfo.td` to add the 3-operand ALU instructions and single-operand instructions. Replace the entire file:

```tablegen
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
// Operand Types
//===----------------------------------------------------------------------===//

def ARC4_GPR : RegisterOperand<GPR32>;

//===----------------------------------------------------------------------===//
// NOP Instruction
//===----------------------------------------------------------------------===//

// nop is encoded as 0x7fffffff (a specific xor pattern)
let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
def NOP : ARC4Inst<(outs), (ins), "nop", []> {
  let Inst{31-0} = 0x7fffffff;
}

//===----------------------------------------------------------------------===//
// 3-Operand ALU Instructions (register-register)
// Format: op a, b, c
// Encoding: I[31:27]=opcode, A[26:21], B[20:15], C[14:9], flags[8:0]=0
//===----------------------------------------------------------------------===//

// Multiclass for 3-operand ALU instructions
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

def ASL  : ARC4InstSOP<0b000000, (outs GPR32:$a), (ins GPR32:$b),
                        "asl\t$a, $b", []>;
def ASR  : ARC4InstSOP<0b000001, (outs GPR32:$a), (ins GPR32:$b),
                        "asr\t$a, $b", []>;
def LSR  : ARC4InstSOP<0b000010, (outs GPR32:$a), (ins GPR32:$b),
                        "lsr\t$a, $b", []>;
def ROR  : ARC4InstSOP<0b000011, (outs GPR32:$a), (ins GPR32:$b),
                        "ror\t$a, $b", []>;
def RRC  : ARC4InstSOP<0b000100, (outs GPR32:$a), (ins GPR32:$b),
                        "rrc\t$a, $b", []>;
def SEXB : ARC4InstSOP<0b000101, (outs GPR32:$a), (ins GPR32:$b),
                        "sexb\t$a, $b", []>;
def SEXW : ARC4InstSOP<0b000110, (outs GPR32:$a), (ins GPR32:$b),
                        "sexw\t$a, $b", []>;
def EXTB : ARC4InstSOP<0b000111, (outs GPR32:$a), (ins GPR32:$b),
                        "extb\t$a, $b", []>;
def EXTW : ARC4InstSOP<0b001000, (outs GPR32:$a), (ins GPR32:$b),
                        "extw\t$a, $b", []>;

//===----------------------------------------------------------------------===//
// Pseudo/alias instructions
//===----------------------------------------------------------------------===//

// mov a, b  =>  and a, b, b
// This is an assembly alias, not a separate instruction
def : InstAlias<"mov\t$a, $b", (AND_rrr GPR32:$a, GPR32:$b, GPR32:$b)>;
```

- [ ] **Step 2: Build and test ALU instructions**

Run:
```bash
cd /home/tomato/projects/llvm-project
cmake --build build --target llvm-mc 2>&1 | tail -20
echo "add r0, r1, r2" | ./build/bin/llvm-mc -triple=arc4-unknown-elf -show-encoding 2>&1
echo "sub r3, r4, r5" | ./build/bin/llvm-mc -triple=arc4-unknown-elf -show-encoding 2>&1
echo "asr r1, r2" | ./build/bin/llvm-mc -triple=arc4-unknown-elf -show-encoding 2>&1
echo "mov r0, r1" | ./build/bin/llvm-mc -triple=arc4-unknown-elf -show-encoding 2>&1
echo "nop" | ./build/bin/llvm-mc -triple=arc4-unknown-elf -show-encoding 2>&1
```

Expected for `add r0, r1, r2`:
- Encoding: I(8)=0b01000 at [31:27], A(0)=0 at [26:21], B(1)=1 at [20:15], C(2)=2 at [14:9], rest=0
- Binary: 01000_000000_000001_000010_000000000 = 0x40008400
- LE bytes: 0x00 0x84 0x00 0x40

Verify against binutils reference: the I() macro shifts opcode left by 27. I(8) = 8 << 27 = 0x40000000. A(0) = 0. B(1) = 1 << 15 = 0x8000. C(2) = 2 << 9 = 0x400. Total = 0x40008400.

- [ ] **Step 3: Commit**

```bash
git add llvm/lib/Target/ARC4/ARC4InstrInfo.td
git commit -m "feat(ARC4): add ALU and single-operand instruction definitions"
```

---

## Task 8: lld/ELF Linker Support

**Files:**
- Create: `lld/ELF/Arch/ARC4.cpp`
- Modify: `lld/ELF/Target.h` (add declaration)
- Modify: `lld/ELF/Target.cpp` (add switch case)
- Modify: `lld/ELF/CMakeLists.txt` (add source file)

- [ ] **Step 1: Create ARC4 linker target**

Create `lld/ELF/Arch/ARC4.cpp`:

```cpp
//===-- ARC4.cpp ----------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// ARC4 (ARCtangent-A4) target support for lld ELF linker.
//
// Relocation reference: binutils-2.15/bfd/elf32-arc.c
// ELF types: binutils-2.15/include/elf/arc.h
//
//===----------------------------------------------------------------------===//

#include "InputFiles.h"
#include "Symbols.h"
#include "Target.h"
#include "lld/Common/ErrorHandler.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/Support/Endian.h"

using namespace llvm;
using namespace llvm::object;
using namespace llvm::support::endian;
using namespace llvm::ELF;
using namespace lld;
using namespace lld::elf;

namespace {

// ARC4-specific relocation type values (from binutils elf/arc.h)
// These predate the ARCompact relocs in LLVM's ELF.h
enum {
  R_ARC4_NONE = 0,
  R_ARC4_32 = 4,
  R_ARC4_B26 = 5,
  R_ARC4_B22_PCREL = 6,
};

class ARC4 final : public TargetInfo {
public:
  ARC4(Ctx &ctx);
  RelExpr getRelExpr(RelType type, const Symbol &s,
                     const uint8_t *loc) const override;
  void relocate(uint8_t *loc, const Relocation &rel,
                uint64_t val) const override;
};

} // end anonymous namespace

ARC4::ARC4(Ctx &ctx) : TargetInfo(ctx) {
  defaultImageBase = 0;
  // ARC4 NOP = 0x7fffffff (little-endian)
  trapInstr = {0xff, 0xff, 0xff, 0x7f};
}

RelExpr ARC4::getRelExpr(RelType type, const Symbol &s,
                         const uint8_t *loc) const {
  switch (type) {
  case R_ARC4_NONE:
    return R_NONE;
  case R_ARC4_32:
    return R_ABS;
  case R_ARC4_B26:
    return R_ABS;
  case R_ARC4_B22_PCREL:
    return R_PC;
  default:
    Err(ctx) << getErrorLoc(ctx, loc) << "unknown relocation (" << type
             << ") against symbol " << &s;
    return R_NONE;
  }
}

void ARC4::relocate(uint8_t *loc, const Relocation &rel,
                    uint64_t val) const {
  switch (rel.type) {
  case R_ARC4_NONE:
    break;
  case R_ARC4_32:
    write32le(loc, val);
    break;
  case R_ARC4_B26: {
    // 26-bit branch: val >> 2, placed in bits [23:0] of instruction word
    uint32_t insn = read32le(loc);
    uint32_t offset = (val >> 2) & 0x00ffffff;
    write32le(loc, (insn & 0xff000000) | offset);
    break;
  }
  case R_ARC4_B22_PCREL: {
    // 22-bit PC-relative: val >> 2, placed in bits [28:7]
    uint32_t insn = read32le(loc);
    uint32_t offset = ((val >> 2) << 7) & 0x1fffff80;
    write32le(loc, (insn & ~0x1fffff80u) | offset);
    break;
  }
  default:
    llvm_unreachable("unknown relocation");
  }
}

void elf::setARC4TargetInfo(Ctx &ctx) {
  ctx.target.reset(new class ARC4(ctx));
}
```

- [ ] **Step 2: Add declaration to Target.h**

In `lld/ELF/Target.h`, add the declaration alongside other target setters:

```cpp
void setARC4TargetInfo(Ctx &);
```

- [ ] **Step 3: Add switch case to Target.cpp**

In `lld/ELF/Target.cpp` in the `setTarget` function, add a case for `EM_ARC`:

```cpp
  case EM_ARC:
    return setARC4TargetInfo(ctx);
```

Note: This will handle `EM_ARC` (45) which is the original ARC machine type used by ARC4. The existing ARCompact target uses `EM_ARC_COMPACT` (93), so there is no conflict.

- [ ] **Step 4: Add source file to CMakeLists.txt**

In `lld/ELF/CMakeLists.txt`, add `Arch/ARC4.cpp` to the source list:

```cmake
  Arch/AArch64.cpp
  Arch/AMDGPU.cpp
  Arch/ARC4.cpp
  Arch/ARM.cpp
```

- [ ] **Step 5: Build lld**

Run:
```bash
cd /home/tomato/projects/llvm-project
cmake --build build --target lld 2>&1 | tail -30
```
Expected: Clean build.

- [ ] **Step 6: Test linking a simple ARC4 object**

Run:
```bash
# Create a minimal assembly file
cat > /tmp/test_arc4.s << 'EOF'
  .text
  .globl __start
__start:
  nop
  nop
EOF

# Assemble it
./build/bin/llvm-mc -filetype=obj -triple=arc4-unknown-elf /tmp/test_arc4.s -o /tmp/test_arc4.o

# Verify the ELF header
./build/bin/llvm-readelf -h /tmp/test_arc4.o 2>&1 | head -10

# Link it
./build/bin/ld.lld /tmp/test_arc4.o -o /tmp/test_arc4.elf --entry=__start 2>&1

# Verify linked binary
./build/bin/llvm-readelf -h /tmp/test_arc4.elf 2>&1 | head -10
```

Expected: ELF header shows Machine: EM_ARC. Linking succeeds.

- [ ] **Step 7: Commit**

```bash
git add lld/ELF/Arch/ARC4.cpp lld/ELF/Target.h lld/ELF/Target.cpp lld/ELF/CMakeLists.txt
git commit -m "feat(ARC4): add lld/ELF linker support with basic relocations"
```

---

## Task 9: Clang Target Info and Driver

**Files:**
- Create: `clang/lib/Basic/Targets/ARC4.h`
- Create: `clang/lib/Basic/Targets/ARC4.cpp`
- Modify: `clang/lib/Basic/Targets.cpp` (register target)
- Create: `clang/lib/Driver/ToolChains/ARC4.h`
- Create: `clang/lib/Driver/ToolChains/ARC4.cpp`
- Modify: `clang/lib/Driver/Driver.cpp` (register toolchain)
- Modify: `clang/lib/Driver/CMakeLists.txt` (add source file)

- [ ] **Step 1: Create Clang TargetInfo header**

Create `clang/lib/Basic/Targets/ARC4.h`:

```cpp
//===--- ARC4.h - Declare ARC4 target feature support -----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_BASIC_TARGETS_ARC4_H
#define LLVM_CLANG_LIB_BASIC_TARGETS_ARC4_H

#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetOptions.h"
#include "llvm/Support/Compiler.h"
#include "llvm/TargetParser/Triple.h"

namespace clang {
namespace targets {

class LLVM_LIBRARY_VISIBILITY ARC4TargetInfo : public TargetInfo {
public:
  ARC4TargetInfo(const llvm::Triple &Triple, const TargetOptions &)
      : TargetInfo(Triple) {
    // ARC4: 32-bit, little-endian, no FPU
    NoAsmVariants = true;
    LongLongAlign = 32;
    SuitableAlign = 32;
    DoubleAlign = LongDoubleAlign = 32;
    LongDoubleWidth = 64;
    LongDoubleFormat = &llvm::APFloat::IEEEdouble();
    SizeType = UnsignedInt;
    PtrDiffType = SignedInt;
    IntPtrType = SignedInt;
    UseZeroLengthBitfieldAlignment = true;
    resetDataLayout("e-m:e-p:32:32-i1:8:32-i8:8:32-i16:16:32-i32:32:32"
                    "-f32:32:32-a:0:32-n32");
  }

  void getTargetDefines(const LangOptions &Opts,
                        MacroBuilder &Builder) const override;

  ArrayRef<Builtin::Info> getTargetBuiltins() const override { return {}; }

  BuiltinVaListKind getBuiltinVaListKind() const override {
    return TargetInfo::VoidPtrBuiltinVaList;
  }

  std::string_view getClobbers() const override { return ""; }

  ArrayRef<const char *> getGCCRegNames() const override;

  ArrayRef<TargetInfo::GCCRegAlias> getGCCRegAliases() const override;

  bool validateAsmConstraint(const char *&Name,
                             TargetInfo::ConstraintInfo &Info) const override {
    return false;
  }

  bool hasProtectedVisibility() const override { return false; }

  unsigned getRegisterWidth() const override { return 32; }

  unsigned getMaxAtomicInlineWidth() const override { return 32; }
};

} // namespace targets
} // namespace clang

#endif // LLVM_CLANG_LIB_BASIC_TARGETS_ARC4_H
```

- [ ] **Step 2: Create Clang TargetInfo implementation**

Create `clang/lib/Basic/Targets/ARC4.cpp`:

```cpp
//===--- ARC4.cpp - Implement ARC4 target feature support -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ARC4.h"
#include "clang/Basic/MacroBuilder.h"

using namespace clang;
using namespace clang::targets;

void ARC4TargetInfo::getTargetDefines(const LangOptions &Opts,
                                      MacroBuilder &Builder) const {
  Builder.defineMacro("__arc4__");
  Builder.defineMacro("__ARC4__");
  Builder.defineMacro("__arc__");
}

static const char *const GCCRegNames[] = {
    "r0",  "r1",  "r2",  "r3",  "r4",  "r5",  "r6",  "r7",
    "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15",
    "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
    "r24", "r25", "gp",  "fp",  "sp",  "ilink1", "ilink2", "blink",
};

ArrayRef<const char *> ARC4TargetInfo::getGCCRegNames() const {
  return llvm::ArrayRef(GCCRegNames);
}

static const TargetInfo::GCCRegAlias GCCRegAliases[] = {
    {{"r26"}, "gp"},
    {{"r27"}, "fp"},
    {{"r28"}, "sp"},
    {{"r29"}, "ilink1"},
    {{"r30"}, "ilink2"},
    {{"r31"}, "blink"},
};

ArrayRef<TargetInfo::GCCRegAlias> ARC4TargetInfo::getGCCRegAliases() const {
  return llvm::ArrayRef(GCCRegAliases);
}
```

- [ ] **Step 3: Register ARC4 target in Targets.cpp**

In `clang/lib/Basic/Targets.cpp`, add a case for `arc4` after the existing `arc` case:

```cpp
  case llvm::Triple::arc:
    return std::make_unique<ARCTargetInfo>(Triple, Opts);
  case llvm::Triple::arc4:
    return std::make_unique<targets::ARC4TargetInfo>(Triple, Opts);
```

Also add the include at the top of the file:
```cpp
#include "Targets/ARC4.h"
```

- [ ] **Step 4: Create Clang ToolChain header**

Create `clang/lib/Driver/ToolChains/ARC4.h`:

```cpp
//===--- ARC4.h - ARC4 ToolChain Implementations ---------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_ARC4_H
#define LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_ARC4_H

#include "Gnu.h"
#include "clang/Driver/ToolChain.h"

namespace clang {
namespace driver {
namespace toolchains {

class LLVM_LIBRARY_VISIBILITY ARC4ToolChain : public Generic_ELF {
public:
  ARC4ToolChain(const Driver &D, const llvm::Triple &Triple,
                const llvm::opt::ArgList &Args)
      : Generic_ELF(D, Triple, Args) {}

  bool IsIntegratedAssemblerDefault() const override { return true; }

  bool isPICDefault() const override { return false; }
  bool isPIEDefault(const llvm::opt::ArgList &Args) const override {
    return false;
  }
  bool isPICDefaultForced() const override { return false; }
  bool HasNativeLLVMSupport() const override { return true; }

  UnwindTableLevel
  getDefaultUnwindTableLevel(const llvm::opt::ArgList &Args) const override {
    return UnwindTableLevel::None;
  }

protected:
  void addClangTargetOptions(const llvm::opt::ArgList &DriverArgs,
                             llvm::opt::ArgStringList &CC1Args,
                             Action::OffloadKind DeviceOffloadKind) const override;
};

} // namespace toolchains
} // namespace driver
} // namespace clang

#endif // LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_ARC4_H
```

- [ ] **Step 5: Create Clang ToolChain implementation**

Create `clang/lib/Driver/ToolChains/ARC4.cpp`:

```cpp
//===--- ARC4.cpp - ARC4 ToolChain Implementations ------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ARC4.h"
#include "CommonArgs.h"

using namespace clang::driver;
using namespace clang::driver::toolchains;
using namespace llvm::opt;

void ARC4ToolChain::addClangTargetOptions(
    const ArgList &DriverArgs, ArgStringList &CC1Args,
    Action::OffloadKind DeviceOffloadKind) const {
  // Embedded: no exceptions, no RTTI by default
  if (!DriverArgs.hasFlag(clang::driver::options::OPT_fexceptions,
                          clang::driver::options::OPT_fno_exceptions, false))
    CC1Args.push_back("-fno-exceptions");
  if (!DriverArgs.hasFlag(clang::driver::options::OPT_frtti,
                          clang::driver::options::OPT_fno_rtti, false))
    CC1Args.push_back("-fno-rtti");
}
```

- [ ] **Step 6: Register toolchain in Driver.cpp**

In `clang/lib/Driver/Driver.cpp`, add the ARC4 case in the `getToolChainForArch` switch (near the `lanai` case):

```cpp
  case llvm::Triple::arc4:
    TC = std::make_unique<toolchains::ARC4ToolChain>(*this, Target, Args);
    break;
```

Also add the include at the top:
```cpp
#include "ToolChains/ARC4.h"
```

- [ ] **Step 7: Add ARC4.cpp to clang Driver CMakeLists.txt**

In `clang/lib/Driver/CMakeLists.txt`, add `ToolChains/ARC4.cpp` to the source list alphabetically:

```cmake
  ToolChains/AMDGPU.cpp
  ToolChains/ARC4.cpp
  ToolChains/ARM.cpp
```

- [ ] **Step 8: Build clang**

Run:
```bash
cd /home/tomato/projects/llvm-project
cmake --build build --target clang 2>&1 | tail -30
```
Expected: Clean build.

- [ ] **Step 9: Test clang recognizes ARC4**

Run:
```bash
echo "__arc4__" | ./build/bin/clang -E -dM --target=arc4-unknown-elf -x c - 2>&1 | grep arc
```
Expected: Shows `#define __arc4__ 1`, `#define __ARC4__ 1`, `#define __arc__ 1`.

- [ ] **Step 10: Test end-to-end assemble and link**

Run:
```bash
cat > /tmp/test_arc4_e2e.s << 'EOF'
  .text
  .globl __start
__start:
  nop
  add r0, r1, r2
  sub r3, r4, r5
  nop
EOF

./build/bin/clang --target=arc4-unknown-elf -c /tmp/test_arc4_e2e.s -o /tmp/test_arc4_e2e.o
./build/bin/ld.lld /tmp/test_arc4_e2e.o -o /tmp/test_arc4_e2e.elf --entry=__start
./build/bin/llvm-readelf -h /tmp/test_arc4_e2e.elf
./build/bin/llvm-objdump -d /tmp/test_arc4_e2e.elf
```

Expected: Full end-to-end path works. Objdump shows disassembled instructions.

- [ ] **Step 11: Commit**

```bash
git add clang/lib/Basic/Targets/ARC4.h clang/lib/Basic/Targets/ARC4.cpp \
        clang/lib/Basic/Targets.cpp \
        clang/lib/Driver/ToolChains/ARC4.h clang/lib/Driver/ToolChains/ARC4.cpp \
        clang/lib/Driver/Driver.cpp clang/lib/Driver/CMakeLists.txt
git commit -m "feat(ARC4): add clang target info and driver toolchain"
```

---

## Task 10: Stub TargetMachine

**Files:**
- Create: `llvm/lib/Target/ARC4/ARC4TargetMachine.h`
- Create: `llvm/lib/Target/ARC4/ARC4TargetMachine.cpp`
- Modify: `llvm/lib/Target/ARC4/CMakeLists.txt`

- [ ] **Step 1: Create TargetMachine header**

Create `llvm/lib/Target/ARC4/ARC4TargetMachine.h`:

```cpp
//===-- ARC4TargetMachine.h - Define TargetMachine for ARC4 -----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ARC4_ARC4TARGETMACHINE_H
#define LLVM_LIB_TARGET_ARC4_ARC4TARGETMACHINE_H

#include "llvm/Target/TargetMachine.h"
#include <optional>

namespace llvm {

class ARC4TargetMachine : public LLVMTargetMachine {
public:
  ARC4TargetMachine(const Target &T, const Triple &TT, StringRef CPU,
                    StringRef FS, const TargetOptions &Options,
                    std::optional<Reloc::Model> RM,
                    std::optional<CodeModel::Model> CM,
                    CodeGenOptLevel OL, bool JIT);

  ~ARC4TargetMachine() override;

  const TargetSubtargetInfo *getSubtargetImpl(const Function &) const override {
    return nullptr; // No codegen support yet
  }

  TargetPassConfig *createPassConfig(PassManagerBase &PM) override;
};

} // namespace llvm

#endif // LLVM_LIB_TARGET_ARC4_ARC4TARGETMACHINE_H
```

- [ ] **Step 2: Create TargetMachine implementation**

Create `llvm/lib/Target/ARC4/ARC4TargetMachine.cpp`:

```cpp
//===-- ARC4TargetMachine.cpp - Define TargetMachine for ARC4 -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ARC4TargetMachine.h"
#include "TargetInfo/ARC4TargetInfo.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"

using namespace llvm;

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void LLVMInitializeARC4Target() {
  RegisterTargetMachine<ARC4TargetMachine> X(getTheARC4Target());
}

static const char *ARC4DataLayout =
    "e-m:e-p:32:32-i1:8:32-i8:8:32-i16:16:32-i32:32:32"
    "-f32:32:32-a:0:32-n32";

ARC4TargetMachine::ARC4TargetMachine(const Target &T, const Triple &TT,
                                     StringRef CPU, StringRef FS,
                                     const TargetOptions &Options,
                                     std::optional<Reloc::Model> RM,
                                     std::optional<CodeModel::Model> CM,
                                     CodeGenOptLevel OL, bool JIT)
    : LLVMTargetMachine(T, ARC4DataLayout, TT,
                        CPU.empty() ? "generic" : CPU, FS, Options,
                        RM.value_or(Reloc::Static),
                        getEffectiveCodeModel(CM, CodeModel::Small), OL) {
  initAsmInfo();
}

ARC4TargetMachine::~ARC4TargetMachine() = default;

namespace {
class ARC4PassConfig : public TargetPassConfig {
public:
  ARC4PassConfig(ARC4TargetMachine &TM, PassManagerBase &PM)
      : TargetPassConfig(TM, PM) {}

  ARC4TargetMachine &getARC4TargetMachine() const {
    return getTM<ARC4TargetMachine>();
  }
};
} // end anonymous namespace

TargetPassConfig *ARC4TargetMachine::createPassConfig(PassManagerBase &PM) {
  return new ARC4PassConfig(*this, PM);
}
```

- [ ] **Step 3: Update CMakeLists.txt to build TargetMachine**

Update `llvm/lib/Target/ARC4/CMakeLists.txt`:

```cmake
add_llvm_component_group(ARC4)

set(LLVM_TARGET_DEFINITIONS ARC4.td)

tablegen(LLVM ARC4GenAsmMatcher.inc -gen-asm-matcher)
tablegen(LLVM ARC4GenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM ARC4GenInstrInfo.inc -gen-instr-info)
tablegen(LLVM ARC4GenMCCodeEmitter.inc -gen-emitter)
tablegen(LLVM ARC4GenRegisterInfo.inc -gen-register-info)
tablegen(LLVM ARC4GenSubtargetInfo.inc -gen-subtarget)

add_public_tablegen_target(ARC4CommonTableGen)

add_llvm_target(ARC4CodeGen
  ARC4TargetMachine.cpp

  LINK_COMPONENTS
  ARC4Desc
  ARC4Info
  CodeGen
  CodeGenTypes
  Core
  MC
  Support
  Target
  TargetParser

  ADD_TO_COMPONENT
  ARC4
  )

add_subdirectory(AsmParser)
add_subdirectory(MCTargetDesc)
add_subdirectory(TargetInfo)
```

- [ ] **Step 4: Build and verify**

Run:
```bash
cd /home/tomato/projects/llvm-project
cmake --build build --target LLVMARC4CodeGen 2>&1 | tail -20
```
Expected: Clean build.

- [ ] **Step 5: Test llc recognizes arc4**

Run:
```bash
./build/bin/llc --version 2>&1 | grep -i arc4
```
Expected: Shows "arc4" in the registered targets list.

- [ ] **Step 6: Commit**

```bash
git add llvm/lib/Target/ARC4/ARC4TargetMachine.h llvm/lib/Target/ARC4/ARC4TargetMachine.cpp \
        llvm/lib/Target/ARC4/CMakeLists.txt
git commit -m "feat(ARC4): add stub TargetMachine for future codegen backend"
```

---

## Task 11: Integration Tests

**Files:**
- Create: `llvm/test/MC/ARC4/basic-asm.s`
- Create: `llvm/test/MC/ARC4/nop-encoding.s`
- Create: `lld/test/ELF/arc4-basic.s`

- [ ] **Step 1: Create assembly encoding test**

Create `llvm/test/MC/ARC4/nop-encoding.s`:

```asm
; RUN: llvm-mc -triple=arc4-unknown-elf -show-encoding %s | FileCheck %s

; CHECK: nop ; encoding: [0xff,0xff,0xff,0x7f]
nop
```

- [ ] **Step 2: Create ALU instruction test**

Create `llvm/test/MC/ARC4/basic-asm.s`:

```asm
; RUN: llvm-mc -triple=arc4-unknown-elf -show-encoding %s | FileCheck %s

; ALU register-register instructions
; add r0, r1, r2: I(8)|A(0)|B(1)|C(2) = 0x40008400
; CHECK: add r0, r1, r2 ; encoding: [0x00,0x84,0x00,0x40]
add r0, r1, r2

; sub r3, r4, r5: I(10)|A(3)|B(4)|C(5) = 0x50620a00
; CHECK: sub r3, r4, r5
sub r3, r4, r5

; Single-operand instructions
; CHECK: asr
asr r1, r2

; Aliases
; CHECK: and r0, r1, r1
mov r0, r1
```

- [ ] **Step 3: Create lld basic link test**

Create `lld/test/ELF/arc4-basic.s`:

```asm
# REQUIRES: arc4
# RUN: llvm-mc -filetype=obj -triple=arc4-unknown-elf %s -o %t.o
# RUN: ld.lld %t.o -o %t --entry=__start
# RUN: llvm-readelf -h %t | FileCheck %s

# CHECK: Machine: Argonaut RISC Core

  .text
  .globl __start
__start:
  nop
```

- [ ] **Step 4: Run the tests**

Run:
```bash
cd /home/tomato/projects/llvm-project
cmake --build build --target llvm-mc llvm-readelf ld.lld FileCheck 2>&1 | tail -10
./build/bin/llvm-lit llvm/test/MC/ARC4/ -v 2>&1
./build/bin/llvm-lit lld/test/ELF/arc4-basic.s -v 2>&1
```
Expected: All tests pass.

- [ ] **Step 5: Commit**

```bash
git add llvm/test/MC/ARC4/ lld/test/ELF/arc4-basic.s
git commit -m "test(ARC4): add MC assembly and lld integration tests"
```
