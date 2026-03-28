# ARC4 Backend — Minimal Codegen Bootstrap

**Date:** 2026-03-28
**Scope:** Get `llc` to compile `int foo() { return 42; }` to a valid ARC4 object file. This is the first sub-project of the backend — prove the IR → MachineInstr → MCInst → binary pipeline works end-to-end.

## Overview

Build the minimum set of backend components needed to compile a trivial C function through `llc`. This establishes the codegen infrastructure that all subsequent work builds on: calling convention, register allocation, frame lowering, instruction selection, and MC emission.

**Success criterion:** `echo 'define i32 @foo() { ret i32 42 }' | llc -march=arc4 -filetype=obj -o foo.o` produces a valid ELF with correct machine code.

## Components

### ARC4Subtarget (ARC4Subtarget.h/.cpp)

Central container for all target-specific information. Holds:
- `ARC4InstrInfo` — instruction utilities
- `ARC4RegisterInfo` — register info (accessed via InstrInfo)
- `ARC4FrameLowering` — prologue/epilogue
- `ARC4TargetLowering` — type/operation legalization and lowering
- `ARC4SelectionDAGInfo` — selection DAG info (can be default)

Constructor takes Triple, CPU, FS, TargetMachine. Initialized from generated `ARC4GenSubtargetInfo`.

### ARC4RegisterInfo (ARC4RegisterInfo.h/.cpp)

- `getCalleeSavedRegs()` → returns CSR_ARC4 (r13-r25, FP)
- `getReservedRegs()` → marks SP, GP, ILINK1, ILINK2 as reserved; FP reserved when frame pointer used
- `eliminateFrameIndex()` → replaces frame index operands with SP/FP + offset
- `getFrameRegister()` → returns FP or SP depending on whether frame pointer is needed
- Uses register scavenger for large frame offsets

### ARC4InstrInfo (ARC4InstrInfo.h/.cpp)

- `copyPhysReg()` → `mov $dst, $src` (which is `and $dst, $src, $src`)
- `storeRegToStackSlot()` → `st $reg, [SP, offset]`
- `loadRegFromStackSlot()` → `ld $reg, [SP, offset]`
- `isLoadFromStackSlot()` / `isStoreToStackSlot()` — recognize stack access patterns
- Inherits from `ARC4GenInstrInfo` (generated)

### ARC4FrameLowering (ARC4FrameLowering.h/.cpp)

Stack frame following the GCC A4 ABI (16-byte backchain):

```
High addr
  [incoming args beyond r7]
  ─── SP at entry ───
  [BLINK (return addr)]        offset 0
  [saved FP]                   offset 4
  [static chain]               offset 8
  [reserved]                   offset 12
  [callee-saved regs]          offset 16+
  [local variables]
  [outgoing arg area]
  ─── SP after prologue ───
Low addr
```

- Stack grows downward, 4-byte aligned
- `emitPrologue()`:
  1. Save BLINK to [SP, 0] (if non-leaf)
  2. Save FP to [SP, 4] (if frame pointer needed)
  3. Set FP = SP (if frame pointer needed)
  4. Adjust SP for frame size (sub SP, SP, framesize)
  5. Save callee-saved registers
- `emitEpilogue()`:
  1. Restore callee-saved registers
  2. Restore SP (add SP, SP, framesize or mov SP, FP)
  3. Restore FP from [SP, 4]
  4. Restore BLINK from [SP, 0]
  5. Return via j [blink]
- `determineCalleeSaves()` — mark which registers need saving
- `hasFP()` — true if function has variable-size alloca, takes address of locals, or -fno-omit-frame-pointer

For the minimal bootstrap (leaf function returning constant): prologue and epilogue can be empty (no stack frame needed).

### ARC4ISelLowering (ARC4ISelLowering.h/.cpp)

Type legalization:
- i32 is the only legal type
- i1, i8, i16 promoted to i32
- f32, f64: soft-float (LibCall)
- i64: expand to i32 pairs

Operation legalization for bootstrap:
- Most operations: Expand (let LLVM decompose them)
- `ISD::RETURN` → custom lower to `ARC4ISD::RET`
- `ISD::GlobalAddress` → custom lower (for future use)

Custom ISD nodes:
- `ARC4ISD::RET` — return sequence (j [blink] with optional glue for return value copy)

Lowering functions:
- `LowerFormalArguments()` — copy r0-r7 to virtual registers based on CC_ARC4
- `LowerReturn()` — copy result to r0 via RetCC_ARC4, emit ARC4ISD::RET
- `LowerCall()` — stub for bootstrap (not needed for `return 42`)

### ARC4CallingConv.td

```
CC_ARC4: i1/i8/i16 promoted to i32, first 8 i32 in r0-r7, rest on stack (4-byte aligned)
RetCC_ARC4: i32 returned in r0
CSR_ARC4: r13-r25, FP callee-saved
```

### ARC4ISelDAGToDAG (ARC4ISelDAGToDAG.cpp)

Minimal DAG-to-DAG instruction selector. For the bootstrap, the TableGen-generated selector (from DAG patterns on instruction definitions) handles most work. The manual `Select()` method handles:
- `ISD::FrameIndex` → materialize as SP/FP + offset
- Everything else: delegate to generated `SelectCode()`

### DAG Patterns on Instructions

Add patterns to existing instruction definitions in ARC4InstrInfo.td:

```tablegen
// Return: j [blink]
let isReturn = 1, isTerminator = 1, isBarrier = 1 in
def RET : ARC4InstJumpReg<0, (outs), (ins), "j\t[blink]",
                           [(ARC4ret)]> {
  let b = 31; // BLINK
}

// Materialize 9-bit constant: add rd, 0, shimm → rd = shimm
// Materialize 32-bit constant: add rd, 0, limm → rd = limm

// Basic ALU patterns:
// (add i32:$b, i32:$c) → ADD_rrr
// (sub i32:$b, i32:$c) → SUB_rrr
// (and i32:$b, i32:$c) → AND_rrr
// (or  i32:$b, i32:$c) → OR_rrr
// (xor i32:$b, i32:$c) → XOR_rrr
```

### ARC4MCInstLower (ARC4MCInstLower.h/.cpp)

Converts MachineInstr → MCInst for the MC emission layer:
- Maps physical registers to MCOperand registers
- Maps immediates to MCOperand immediates
- Maps MachineBasicBlock, GlobalAddress, ExternalSymbol to MCExpr symbol refs
- Fills suffix operands (f, q, n, x, w, e, v, D) with default 0 values (or from MI flags)

### ARC4AsmPrinter (ARC4AsmPrinter.cpp)

Standard AsmPrinter implementation:
- `emitInstruction()` — calls MCInstLower, emits via MCStreamer
- `emitFunctionEntryLabel()` — emit function symbol
- Handles `.globl`, `.type` directives

### ARC4TargetMachine Updates

- Create real `ARC4Subtarget` instead of returning nullptr
- PassConfig adds `addInstSelector()` for DAG instruction selection
- Register `ARC4AsmPrinter` pass

### ARC4MachineFunctionInfo (ARC4MachineFunctionInfo.h)

Per-function state:
- `VarArgsFrameIndex` — for variadic functions (not needed for bootstrap)
- `ReturnAddrFrameIndex` — stack slot for saved BLINK
- Frame size tracking

## Testing

```bash
# Compile trivial function
echo 'define i32 @foo() { ret i32 42 }' | llc -march=arc4 -filetype=obj -o /tmp/test.o

# Verify it's a valid ELF
llvm-readelf -h /tmp/test.o

# Disassemble
llvm-objdump -d --triple=arc4-unknown-elf /tmp/test.o

# Expected output: foo materializes 42 into r0, then j [blink]
```

## Not In Scope (future sub-projects)

- Function calls (LowerCall)
- Control flow (branches, comparisons, select)
- Memory operations beyond stack spill/reload
- Global variable access
- 64-bit operations
- Floating point (soft-float library calls)
- Variadic functions
- Inline assembly constraints
- Exception handling
