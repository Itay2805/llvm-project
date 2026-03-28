add_llvm_component_group(ARC4)

add_llvm_target(ARC4CodeGen
  ARC4TargetMachine.cpp

  LINK_COMPONENTS
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
