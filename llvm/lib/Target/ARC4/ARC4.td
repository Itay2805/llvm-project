//===- ARC4.td - Describe the ARC4 Target Machine ---------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

include "llvm/Target/Target.td"

include "ARC4RegisterInfo.td"
include "ARC4InstrInfo.td"

defm : RemapAllTargetPseudoPointerOperands<GPR32>;

def : ProcessorModel<"generic", NoSchedModel, []>;
def : ProcessorModel<"arc4", NoSchedModel, []>;

def ARC4InstrInfo : InstrInfo;

def ARC4InstPrinter : AsmWriter {
  string AsmWriterClassName = "InstPrinter";
  bit isMCAsmWriter = 1;
}

def ARC4AsmParser : AsmParser {
  let ShouldEmitMatchRegisterAltName = true;
}

def ARC4 : Target {
  let InstructionSet = ARC4InstrInfo;
  let AssemblyWriters = [ARC4InstPrinter];
  let AssemblyParsers = [ARC4AsmParser];
}
