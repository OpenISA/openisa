//===-- OiRegisterInfo.h - Oi Register Information Impl -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the Oi implementation of the TargetRegisterInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef OIREGISTERINFO_H
#define OIREGISTERINFO_H

#include "Oi.h"
#include "llvm/Target/TargetRegisterInfo.h"

#define GET_REGINFO_HEADER
#include "OiGenRegisterInfo.inc"

namespace llvm {
class OiSubtarget;
class Type;

class OiRegisterInfo : public OiGenRegisterInfo {
protected:
  const OiSubtarget &Subtarget;

public:
  OiRegisterInfo(const OiSubtarget &Subtarget);

  /// getRegisterNumbering - Given the enum value for some register, e.g.
  /// Oi::RA, return the number that it corresponds to (e.g. 31).
  static unsigned getRegisterNumbering(unsigned RegEnum);

  /// Get PIC indirect call register
  static unsigned getPICCallReg();

  /// Adjust the Oi stack frame.
  void adjustOiStackFrame(MachineFunction &MF) const;

  /// Code Generation virtual methods...
  unsigned getRegPressureLimit(const TargetRegisterClass *RC,
                               MachineFunction &MF) const;
  const uint16_t *getCalleeSavedRegs(const MachineFunction *MF = 0) const;
  const uint32_t *getCallPreservedMask(CallingConv::ID) const;

  BitVector getReservedRegs(const MachineFunction &MF) const;

  virtual bool requiresRegisterScavenging(const MachineFunction &MF) const;

  virtual bool trackLivenessAfterRegAlloc(const MachineFunction &MF) const;

  /// Stack Frame Processing Methods
  virtual void eliminateFrameIndex(MachineBasicBlock::iterator II,
                                   int SPAdj, RegScavenger *RS = NULL) const;

  void processFunctionBeforeFrameFinalized(MachineFunction &MF) const;

  /// Debug information queries.
  unsigned getFrameRegister(const MachineFunction &MF) const;

  /// Exception handling queries.
  unsigned getEHExceptionRegister() const;
  unsigned getEHHandlerRegister() const;

private:
  virtual void eliminateFI(MachineBasicBlock::iterator II, unsigned OpNo,
                           int FrameIndex, uint64_t StackSize,
                           int64_t SPOffset) const = 0;
};

} // end namespace llvm

#endif
