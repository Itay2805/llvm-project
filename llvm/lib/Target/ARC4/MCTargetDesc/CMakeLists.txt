add_llvm_component_library(LLVMARC4Desc
  ARC4MCAsmInfo.cpp
  ARC4MCCodeEmitter.cpp
  ARC4AsmBackend.cpp
  ARC4MCTargetDesc.cpp

  LINK_COMPONENTS
  ARC4Info
  ARC4InstPrinter
  MC
  Support
  TargetParser

  ADD_TO_COMPONENT
  ARC4
)
