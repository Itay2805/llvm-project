# ARC4 Minimal Codegen Backend Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Get `llc -march=arc4` to compile `define i32 @foo() { ret i32 42 }` to a valid ARC4 ELF object file.

**Architecture:** Build the minimum LLVM codegen pipeline: IR → SelectionDAG → MachineInstr → MCInst → binary. Create Subtarget, RegisterInfo, InstrInfo, FrameLowering, ISelLowering, ISelDAGToDAG, MCInstLower, AsmPrinter, and calling convention. Follow the existing ARC (ARCompact) backend as a reference pattern — it's the closest architecture and uses similar conventions.

**Tech Stack:** LLVM CodeGen framework, TableGen, SelectionDAG

**Key reference:** `/home/tomato/projects/llvm-project/llvm/lib/Target/ARC/` — the ARCompact backend. Use it as a template for file structure, class hierarchy, and LLVM API usage. Adapt for ARC4's specific calling convention (16-byte backchain), register set, and instruction encodings.

**Design spec:** `/home/tomato/projects/llvm-project/docs/superpowers/specs/2026-03-28-arc4-backend-minimal-design.md`

---

## Task 1: Calling Convention and TableGen Rules

**Files:**
- Create: `llvm/lib/Target/ARC4/ARC4CallingConv.td`
- Modify: `llvm/lib/Target/ARC4/ARC4.td` (include CallingConv)
- Modify: `llvm/lib/Target/ARC4/CMakeLists.txt` (add tablegen rules)

- [ ] **Step 1: Create ARC4CallingConv.td**

Model after `ARC/ARCCallingConv.td`. Define:
- `CC_ARC4`: i8/i16 promoted to i32, first 8 args in R0-R7, rest on stack 4-byte aligned
- `RetCC_ARC4`: i32 returned in R0
- `CSR_ARC4`: callee-saved = R13-R25, FP

```tablegen
def CC_ARC4 : CallingConv<[
  CCIfType<[i1, i8, i16], CCPromoteToType<i32>>,
  CCIfType<[i32], CCAssignToReg<[R0, R1, R2, R3, R4, R5, R6, R7]>>,
  CCIfType<[i32], CCAssignToStack<4, 4>>
]>;

def RetCC_ARC4 : CallingConv<[
  CCIfType<[i32], CCAssignToReg<[R0]>>
]>;

def CSR_ARC4 : CalleeSavedRegs<(add (sequence "R%u", 13, 25), FP)>;
```

- [ ] **Step 2: Include CallingConv in ARC4.td**

Add `include "ARC4CallingConv.td"` to ARC4.td.

- [ ] **Step 3: Add tablegen rules to CMakeLists.txt**

Add these tablegen lines:
```cmake
tablegen(LLVM ARC4GenCallingConv.inc -gen-callingconv)
tablegen(LLVM ARC4GenDAGISel.inc -gen-dag-isel)
tablegen(LLVM ARC4GenSDNodeInfo.inc -gen-sd-node-info)
```

- [ ] **Step 4: Build tablegen**

```bash
cmake --build build --target ARC4CommonTableGen
```

- [ ] **Step 5: Commit**

```bash
git add llvm/lib/Target/ARC4/
git commit -m "feat(ARC4): add calling convention and backend tablegen rules"
```

---

## Task 2: MachineFunctionInfo and SDNode Definitions

**Files:**
- Create: `llvm/lib/Target/ARC4/ARC4MachineFunctionInfo.h`
- Create: `llvm/lib/Target/ARC4/ARC4MachineFunctionInfo.cpp`
- Create: `llvm/lib/Target/ARC4/ARC4ISD.h` (custom ISD node enum)

- [ ] **Step 1: Create ARC4MachineFunctionInfo**

Model after `ARC/ARCMachineFunctionInfo.h`. Minimal — just VarArgsFrameIndex and ReturnStackOffset fields. Include the `clone()` override.

- [ ] **Step 2: Create ARC4ISD.h**

Define custom SelectionDAG node types:
```cpp
namespace ARC4ISD {
enum NodeType : unsigned {
  FIRST_NUMBER = ISD::BUILTIN_OP_END,
  RET,          // Return from function (j [blink])
  // Future: CMP, CMOV, BR_CC, CALL, WRAPPER, etc.
};
} // namespace ARC4ISD
```

- [ ] **Step 3: Commit**

---

## Task 3: RegisterInfo

**Files:**
- Create: `llvm/lib/Target/ARC4/ARC4RegisterInfo.h`
- Create: `llvm/lib/Target/ARC4/ARC4RegisterInfo.cpp`

- [ ] **Step 1: Create ARC4RegisterInfo**

Model after `ARC/ARCRegisterInfo.h/.cpp`. Inherits from `ARC4GenRegisterInfo`.

Key methods:
- Constructor: `ARC4GenRegisterInfo(ARC4::BLINK)` — BLINK is the return address register
- `getCalleeSavedRegs()` → return `CSR_ARC4_SaveList` (generated from CallingConv.td)
- `getCallPreservedMask()` → return `CSR_ARC4_RegMask`
- `getReservedRegs()` → mark SP, GP, ILINK1, ILINK2 as reserved. Mark FP if `hasFP()`
- `eliminateFrameIndex()` → for bootstrap, simply replace frame index with SP + offset. Handle shimm range (-256..255) directly, use limm for larger offsets.
- `getFrameRegister()` → return FP if `hasFP()`, else SP
- `requiresRegisterScavenging()` → return true

- [ ] **Step 2: Commit**

---

## Task 4: InstrInfo

**Files:**
- Create: `llvm/lib/Target/ARC4/ARC4InstrInfo.h`
- Create: `llvm/lib/Target/ARC4/ARC4InstrInfo.cpp`

- [ ] **Step 1: Create ARC4InstrInfo**

Model after `ARC/ARCInstrInfo.h/.cpp`. Inherits from `ARC4GenInstrInfo`.

Key methods for bootstrap:
- `copyPhysReg()` — emit `AND_rrr dst, src, src, 0, 0` (which is `mov dst, src`)
- `storeRegToStackSlot()` — emit `ST_rrs reg, SP, offset, 0, 0` (st reg, [sp, offset])
- `loadRegFromStackSlot()` — emit `LD_rs reg, SP, offset, 0, 0, 0` (ld reg, [sp, offset])
- `getRegisterInfo()` — return the ARC4RegisterInfo instance

The store/load methods need to handle offsets that fit in shimm9 vs those that need limm. For bootstrap, assert shimm range and handle limm later.

- [ ] **Step 2: Commit**

---

## Task 5: FrameLowering

**Files:**
- Create: `llvm/lib/Target/ARC4/ARC4FrameLowering.h`
- Create: `llvm/lib/Target/ARC4/ARC4FrameLowering.cpp`

- [ ] **Step 1: Create ARC4FrameLowering**

Model after `ARC/ARCFrameLowering.h/.cpp`. Constructor: `TargetFrameLowering(StackGrowsDown, Align(4), 0)`.

For the minimal bootstrap (leaf function, no locals, no args):
- `emitPrologue()` — if frame size > 0: save BLINK, save FP, set FP=SP, sub SP by frame size. For leaf functions with no stack, do nothing.
- `emitEpilogue()` — reverse of prologue
- `hasFP()` — return true if function has variable alloca, frame address taken, or non-zero frame size with non-leaf
- `determineCalleeSaves()` — call base class, then mark BLINK if function makes calls

For the `return 42` bootstrap, prologue/epilogue can be empty (leaf, no stack).

- [ ] **Step 2: Commit**

---

## Task 6: ISelLowering

**Files:**
- Create: `llvm/lib/Target/ARC4/ARC4ISelLowering.h`
- Create: `llvm/lib/Target/ARC4/ARC4ISelLowering.cpp`

- [ ] **Step 1: Create ARC4ISelLowering**

Model after `ARC/ARCISelLowering.h/.cpp`. This is the largest file.

**Constructor:**
- `addRegisterClass(MVT::i32, &ARC4::GPR32RegClass)`
- `computeRegisterProperties(Subtarget.getRegisterInfo())`
- `setStackPointerRegisterToSaveRestore(ARC4::SP)`
- Mark ALL operations as Expand initially: `for (unsigned Opc = 0; Opc < ISD::BUILTIN_OP_END; ++Opc) setOperationAction(Opc, MVT::i32, Expand);`
- Then enable the ones we support: ADD, SUB, AND, OR, XOR as Legal
- `ISD::Constant` as Legal, `ISD::UNDEF` as Legal
- `ISD::LOAD` and `ISD::STORE` as Legal
- `ISD::GlobalAddress` as Custom (for future use)

**LowerReturn():**
- Use `RetCC_ARC4` to determine where return values go
- Copy return value to physical register (R0) via CopyToReg
- Emit `ARC4ISD::RET` node with chain and glue

**LowerFormalArguments():**
- Use `CC_ARC4` to determine where args come from
- For bootstrap: handle register args (copy from physical to virtual)
- Handle stack args (load from SP + offset + 16 for backchain)

**LowerCall():**
- Stub for bootstrap — `llvm_unreachable("Calls not yet supported")`

**getTargetNodeName():**
- Return names for ARC4ISD::RET etc.

- [ ] **Step 2: Commit**

---

## Task 7: ISelDAGToDAG

**Files:**
- Create: `llvm/lib/Target/ARC4/ARC4ISelDAGToDAG.cpp`

- [ ] **Step 1: Create ARC4ISelDAGToDAG**

Model after `ARC/ARCISelDAGToDAG.cpp`. Minimal implementation:

```cpp
class ARC4DAGToDAGISel : public SelectionDAGISel {
public:
  ARC4DAGToDAGISel(ARC4TargetMachine &TM, CodeGenOptLevel OL)
      : SelectionDAGISel(TM, OL) {}
  void Select(SDNode *N) override;
#include "ARC4GenDAGISel.inc"
};
```

`Select()` method:
- For `ISD::FrameIndex`: materialize as register + offset
- Default: `SelectCode(N)` — delegate to TableGen-generated selector

Also create the legacy pass wrapper (`ARC4DAGToDAGISelLegacy`) and the `createARC4ISelDag()` factory function.

- [ ] **Step 2: Commit**

---

## Task 8: MCInstLower and AsmPrinter

**Files:**
- Create: `llvm/lib/Target/ARC4/ARC4MCInstLower.h`
- Create: `llvm/lib/Target/ARC4/ARC4MCInstLower.cpp`
- Create: `llvm/lib/Target/ARC4/ARC4AsmPrinter.cpp`

- [ ] **Step 1: Create ARC4MCInstLower**

Copy the ARC pattern almost directly — it's a simple pass-through:
- `Lower()` iterates MI operands, calls `LowerOperand()` for each
- `LowerOperand()` handles Register (skip implicit), Immediate, and symbol types
- `LowerSymbolOperand()` handles MBB, GlobalAddress, ExternalSymbol, JumpTable, ConstantPool

**IMPORTANT for ARC4:** After lowering the visible operands, append default suffix operands (f=0, q=0, n=0, x=0, w=0, e=0, v=0, D=0) based on TSFlags. The MCInst needs all operands including suffixes for the MCCodeEmitter to work correctly. Read the instruction's TSFlags to determine which suffix operands to add and how many.

- [ ] **Step 2: Create ARC4AsmPrinter**

Model after `ARC/ARCAsmPrinter.cpp`:
- `emitInstruction()` — lower MI via MCInstLower, emit via MCStreamer
- `runOnMachineFunction()` — ensure 4-byte alignment
- Registration: `LLVMInitializeARC4AsmPrinter()` with `RegisterAsmPrinter`

- [ ] **Step 3: Commit**

---

## Task 9: Subtarget

**Files:**
- Create: `llvm/lib/Target/ARC4/ARC4Subtarget.h`
- Create: `llvm/lib/Target/ARC4/ARC4Subtarget.cpp`

- [ ] **Step 1: Create ARC4Subtarget**

Model after `ARC/ARCSubtarget.h/.cpp`. Inherits from `ARC4GenSubtargetInfo`.

Contains:
- `ARC4InstrInfo InstrInfo`
- `ARC4FrameLowering FrameLowering`
- `ARC4TargetLowering TLInfo`
- `ARC4RegisterInfo` (accessed via InstrInfo)

Constructor initializes all members. Getter methods return const pointers.

- [ ] **Step 2: Commit**

---

## Task 10: DAG Patterns and RET Pseudo

**Files:**
- Modify: `llvm/lib/Target/ARC4/ARC4InstrInfo.td` (add patterns and RET)
- Create: `llvm/lib/Target/ARC4/ARC4Nodes.td` (SDNode definitions for TableGen)

- [ ] **Step 1: Create ARC4Nodes.td with custom SDNode definitions**

```tablegen
def ARC4ret : SDNode<"ARC4ISD::RET", SDTNone,
                     [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;
```

Include this from ARC4.td.

- [ ] **Step 2: Add RET instruction to ARC4InstrInfo.td**

Define a RET pseudo (or use the existing J_r with b=BLINK):

```tablegen
let isReturn = 1, isTerminator = 1, isBarrier = 1,
    hasCtrlDep = 1, Uses = [BLINK] in
def RET : ARC4InstJumpReg<0, (outs), (ins),
  "j\t[blink]", [(ARC4ret)]> {
  let b = 31; // BLINK encoding
  let f = 0;
  let q = 0;
  let n = 0;
}
```

- [ ] **Step 3: Add DAG patterns for ALU instructions**

Add patterns to existing instruction definitions:

```tablegen
// In the ALU3 multiclass, add patterns to the rrr variant:
def _rrr : ARC4InstALU3_rrr<opcode,
  (outs GPR32:$a), (ins GPR32:$b, GPR32:$c, i32imm:$f, i32imm:$q),
  mnemonic # "\t$a, $b, $c",
  [(set GPR32:$a, (opnode GPR32:$b, GPR32:$c))]>;
```

Where `opnode` is `add`, `sub`, `and`, `or`, `xor` respectively. The pattern needs f=0, q=0 defaults.

NOTE: TableGen patterns with extra operands (f, q) that aren't in the pattern may require `let f = 0; let q = 0;` defaults or use of `ComplexPattern`. Study how this works and adapt. It may be simplest to define pattern-only instruction variants without the suffix operands, or use a pseudo instruction.

- [ ] **Step 4: Add constant materialization pattern**

For constants that fit in shimm9 (-256..255), use AND_rss (mov alias):
```tablegen
def : Pat<(i32 simm9:$val), (AND_rss simm9:$val, 0)>;
```

This materializes a shimm constant by using `mov rd, val` = `and rd, val, val`.

- [ ] **Step 5: Build and verify tablegen passes**

```bash
cmake --build build --target ARC4CommonTableGen
```

- [ ] **Step 6: Commit**

---

## Task 11: Update TargetMachine and Build

**Files:**
- Modify: `llvm/lib/Target/ARC4/ARC4TargetMachine.h`
- Modify: `llvm/lib/Target/ARC4/ARC4TargetMachine.cpp`
- Modify: `llvm/lib/Target/ARC4/CMakeLists.txt`
- Create: `llvm/lib/Target/ARC4/ARC4.h` (forward declarations and factory functions)

- [ ] **Step 1: Create ARC4.h with forward declarations**

```cpp
#ifndef LLVM_LIB_TARGET_ARC4_ARC4_H
#define LLVM_LIB_TARGET_ARC4_ARC4_H

#include "llvm/Target/TargetMachine.h"

namespace llvm {
class ARC4TargetMachine;
class FunctionPass;

FunctionPass *createARC4ISelDag(ARC4TargetMachine &TM, CodeGenOptLevel OL);
} // namespace llvm

#endif
```

- [ ] **Step 2: Update ARC4TargetMachine**

- Add `ARC4Subtarget Subtarget` member
- `getSubtargetImpl()` returns `&Subtarget` (no longer nullptr)
- Constructor creates Subtarget with proper arguments
- Create `TargetLoweringObjectFileELF` as TLOF
- `createMachineFunctionInfo()` returns `ARC4FunctionInfo`
- PassConfig: `addInstSelector()` adds the DAG instruction selector
- Initialize AsmPrinter pass

- [ ] **Step 3: Update CMakeLists.txt**

Add all new source files:
```cmake
add_llvm_target(ARC4CodeGen
  ARC4AsmPrinter.cpp
  ARC4FrameLowering.cpp
  ARC4InstrInfo.cpp
  ARC4ISelDAGToDAG.cpp
  ARC4ISelLowering.cpp
  ARC4MachineFunctionInfo.cpp
  ARC4MCInstLower.cpp
  ARC4RegisterInfo.cpp
  ARC4Subtarget.cpp
  ARC4TargetMachine.cpp

  LINK_COMPONENTS
  ARC4AsmParser
  ARC4Desc
  ARC4Disassembler
  ARC4Info
  Analysis
  AsmPrinter
  CodeGen
  CodeGenTypes
  Core
  MC
  SelectionDAG
  Support
  Target
  TargetParser
  TransformUtils
  ...
)
```

- [ ] **Step 4: Build**

```bash
cmake --build build --target LLVMARC4CodeGen llc
```

This will likely have many compile errors on the first try. Fix them iteratively until the build succeeds.

- [ ] **Step 5: Commit**

---

## Task 12: End-to-End Test

**Files:**
- Create: `llvm/test/CodeGen/ARC4/ret-const.ll`

- [ ] **Step 1: Test compilation**

```bash
echo 'define i32 @foo() { ret i32 42 }' | ./build/bin/llc -march=arc4 -filetype=obj -o /tmp/ret42.o
```

If this crashes, debug the issue. Common problems:
- Missing instruction patterns (add more patterns or pseudos)
- MCInstLower not adding enough suffix operands
- Register class mismatch
- Frame index elimination issues

- [ ] **Step 2: Verify the output**

```bash
./build/bin/llvm-objdump -d --triple=arc4-unknown-elf /tmp/ret42.o
```

Expected: function `foo` that materializes 42 into r0 and returns via j [blink].

- [ ] **Step 3: Create lit test**

```llvm
; RUN: llc -march=arc4 < %s | FileCheck %s

define i32 @foo() {
; CHECK-LABEL: foo:
; CHECK: mov r0, 42
; CHECK: j [blink]
  ret i32 42
}
```

- [ ] **Step 4: Run all tests**

```bash
./build/bin/llvm-lit llvm/test/CodeGen/ARC4/ llvm/test/MC/ARC4/ -v
```

- [ ] **Step 5: Commit**

```bash
git commit -m "feat(ARC4): minimal codegen backend — compile 'return 42' to ELF"
```
