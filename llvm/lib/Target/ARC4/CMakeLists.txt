add_llvm_component_group(ARC4)

set(LLVM_TARGET_DEFINITIONS ARC4.td)

tablegen(LLVM ARC4GenAsmMatcher.inc -gen-asm-matcher)
tablegen(LLVM ARC4GenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM ARC4GenInstrInfo.inc -gen-instr-info)
tablegen(LLVM ARC4GenMCCodeEmitter.inc -gen-emitter)
tablegen(LLVM ARC4GenRegisterInfo.inc -gen-register-info)
tablegen(LLVM ARC4GenSubtargetInfo.inc -gen-subtarget)
# Note: -gen-disassembler fails with "Decoding conflict" because rrr forms
# (variable register fields) overlap with shimm/limm forms (fixed sentinel
# values 61/62/63 in the same fields). The manual disassembler in
# Disassembler/ARC4Disassembler.cpp handles this by checking sentinels at
# runtime. A future improvement would be to use DecoderMethod on GPR32 to
# reject sentinel values, allowing the TableGen decoder to fall through.

add_public_tablegen_target(ARC4CommonTableGen)

add_llvm_target(ARC4CodeGen
  ARC4TargetMachine.cpp

  LINK_COMPONENTS
  ARC4AsmParser
  ARC4Desc
  ARC4Disassembler
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
add_subdirectory(Disassembler)
add_subdirectory(TargetInfo)
add_subdirectory(MCTargetDesc)
