//===-- ARC4MCTargetDesc.h - ARC4 Target Descriptions -----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ARC4_MCTARGETDESC_ARC4MCTARGETDESC_H
#define LLVM_LIB_TARGET_ARC4_MCTARGETDESC_ARC4MCTARGETDESC_H

#include "llvm/MC/MCTargetOptions.h"
#include "llvm/Support/DataTypes.h"
#include <memory>

namespace llvm {
class MCAsmBackend;
class MCCodeEmitter;
class MCContext;
class MCInstrInfo;
class MCObjectTargetWriter;
class MCRegisterInfo;
class MCSubtargetInfo;
class Target;

MCCodeEmitter *createARC4MCCodeEmitter(const MCInstrInfo &MII, MCContext &Ctx);

MCAsmBackend *createARC4AsmBackend(const Target &T, const MCSubtargetInfo &STI,
                                   const MCRegisterInfo &MRI,
                                   const MCTargetOptions &Options);

std::unique_ptr<MCObjectTargetWriter> createARC4ELFObjectWriter(uint8_t OSABI);

} // namespace llvm

// TableGen-generated register info
#define GET_REGINFO_ENUM
#include "ARC4GenRegisterInfo.inc"

// TableGen-generated instruction info
#define GET_INSTRINFO_ENUM
#define GET_INSTRINFO_MC_HELPER_DECLS
#include "ARC4GenInstrInfo.inc"

// TableGen-generated subtarget info
#define GET_SUBTARGETINFO_ENUM
#include "ARC4GenSubtargetInfo.inc"

#endif // LLVM_LIB_TARGET_ARC4_MCTARGETDESC_ARC4MCTARGETDESC_H
