//===-- OIMCAsmInfo.h - OI asm properties ----------------*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the declaration of the OIMCAsmInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef OITARGETASMINFO_H
#define OITARGETASMINFO_H

#include "llvm/MC/MCAsmInfo.h"

namespace llvm {
  class StringRef;
  class Target;

  class OIELFMCAsmInfo : public MCAsmInfo {
    virtual void anchor();
  public:
    explicit OIELFMCAsmInfo(const Target &T, StringRef TT);
  };

} // namespace llvm

#endif
