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
