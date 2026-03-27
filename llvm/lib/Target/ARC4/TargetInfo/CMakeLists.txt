add_llvm_component_library(LLVMARC4Info
  ARC4TargetInfo.cpp

  LINK_COMPONENTS
  MC
  Support
  TargetParser

  ADD_TO_COMPONENT
  ARC4
)
