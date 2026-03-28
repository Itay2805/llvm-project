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
