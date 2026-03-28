add_llvm_component_library(LLVMARC4Disassembler
  ARC4Disassembler.cpp

  LINK_COMPONENTS
  ARC4Desc
  ARC4Info
  MC
  MCDisassembler
  Support

  ADD_TO_COMPONENT
  ARC4
  )
