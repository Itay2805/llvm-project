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
  Core
  MC
  Support
  Target
  TargetParser

  ADD_TO_COMPONENT
  ARC4
  )

add_subdirectory(TargetInfo)
add_subdirectory(MCTargetDesc)
