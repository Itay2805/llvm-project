add_llvm_component_group(ARC4)

set(LLVM_TARGET_DEFINITIONS ARC4.td)

tablegen(LLVM ARC4GenAsmMatcher.inc -gen-asm-matcher)
tablegen(LLVM ARC4GenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM ARC4GenCallingConv.inc -gen-callingconv)
tablegen(LLVM ARC4GenDAGISel.inc -gen-dag-isel)
tablegen(LLVM ARC4GenInstrInfo.inc -gen-instr-info)
tablegen(LLVM ARC4GenRegisterInfo.inc -gen-register-info)
tablegen(LLVM ARC4GenSDNodeInfo.inc -gen-sd-node-info)
tablegen(LLVM ARC4GenSubtargetInfo.inc -gen-subtarget)

add_public_tablegen_target(ARC4CommonTableGen)

add_llvm_target(ARC4CodeGen
  ARC4AsmPrinter.cpp
  ARC4FrameLowering.cpp
  ARC4InstrInfo.cpp
  ARC4ISelDAGToDAG.cpp
  ARC4ISelLowering.cpp
  ARC4MachineFunctionInfo.cpp
  ARC4MCInstLower.cpp
  ARC4RegisterInfo.cpp
  ARC4SelectionDAGInfo.cpp
  ARC4Subtarget.cpp
  ARC4TargetMachine.cpp

  LINK_COMPONENTS
  Analysis
  AsmPrinter
  CodeGen
  CodeGenTypes
  Core
  ARC4Desc
  ARC4Info
  MC
  SelectionDAG
  Support
  Target
  TargetParser
  TransformUtils

  ADD_TO_COMPONENT
  ARC4
)

add_subdirectory(AsmParser)
add_subdirectory(InstPrinter)
add_subdirectory(MCTargetDesc)
add_subdirectory(TargetInfo)
