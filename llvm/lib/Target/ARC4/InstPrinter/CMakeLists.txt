add_llvm_component_library(LLVMARC4InstPrinter
  ARC4InstPrinter.cpp

  LINK_COMPONENTS
  MC
  Support
  TargetParser

  ADD_TO_COMPONENT
  ARC4
)

add_dependencies(LLVMARC4InstPrinter ARC4CommonTableGen)
