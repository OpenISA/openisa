//===- OiDisassembler.cpp - Disassembler for Oi -------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is part of the Oi Disassembler.
//
//===----------------------------------------------------------------------===//

#include "Oi.h"
#include "OiRegisterInfo.h"
#include "OiSubtarget.h"
#include "llvm/MC/MCDisassembler.h"
#include "llvm/MC/MCFixedLenDisassembler.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/MemoryObject.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

typedef MCDisassembler::DecodeStatus DecodeStatus;

namespace {

/// OiDisassemblerBase - a disasembler class for Oi.
class OiDisassemblerBase : public MCDisassembler {
public:
  /// Constructor     - Initializes the disassembler.
  ///
  OiDisassemblerBase(const MCSubtargetInfo &STI, const MCRegisterInfo *Info,
                       bool bigEndian) :
    MCDisassembler(STI), RegInfo(Info), isBigEndian(bigEndian) {}

  virtual ~OiDisassemblerBase() {}

  const MCRegisterInfo *getRegInfo() const { return RegInfo; }

private:
  const MCRegisterInfo *RegInfo;
protected:
  bool isBigEndian;
};

/// OiDisassembler - a disasembler class for Oi32.
class OiDisassembler : public OiDisassemblerBase {
public:
  /// Constructor     - Initializes the disassembler.
  ///
  OiDisassembler(const MCSubtargetInfo &STI, const MCRegisterInfo *Info,
                   bool bigEndian) :
    OiDisassemblerBase(STI, Info, bigEndian) {}

  /// getInstruction - See MCDisassembler.
  virtual DecodeStatus getInstruction(MCInst &instr,
                                      uint64_t &size,
                                      const MemoryObject &region,
                                      uint64_t address,
                                      raw_ostream &vStream,
                                      raw_ostream &cStream) const;
};


/// Oi64Disassembler - a disasembler class for Oi64.
class Oi64Disassembler : public OiDisassemblerBase {
public:
  /// Constructor     - Initializes the disassembler.
  ///
  Oi64Disassembler(const MCSubtargetInfo &STI, const MCRegisterInfo *Info,
                     bool bigEndian) :
    OiDisassemblerBase(STI, Info, bigEndian) {}

  /// getInstruction - See MCDisassembler.
  virtual DecodeStatus getInstruction(MCInst &instr,
                                      uint64_t &size,
                                      const MemoryObject &region,
                                      uint64_t address,
                                      raw_ostream &vStream,
                                      raw_ostream &cStream) const;
};

} // end anonymous namespace

// Forward declare these because the autogenerated code will reference them.
// Definitions are further down.
static DecodeStatus DecodeCPU64RegsRegisterClass(MCInst &Inst,
                                                 unsigned RegNo,
                                                 uint64_t Address,
                                                 const void *Decoder);

static DecodeStatus DecodeCPURegsRegisterClass(MCInst &Inst,
                                               unsigned RegNo,
                                               uint64_t Address,
                                               const void *Decoder);

static DecodeStatus DecodeDSPRegsRegisterClass(MCInst &Inst,
                                               unsigned RegNo,
                                               uint64_t Address,
                                               const void *Decoder);

static DecodeStatus DecodeFGR64RegisterClass(MCInst &Inst,
                                             unsigned RegNo,
                                             uint64_t Address,
                                             const void *Decoder);

static DecodeStatus DecodeFGR32RegisterClass(MCInst &Inst,
                                             unsigned RegNo,
                                             uint64_t Address,
                                             const void *Decoder);

static DecodeStatus DecodeCCRRegisterClass(MCInst &Inst,
                                           unsigned RegNo,
                                           uint64_t Address,
                                           const void *Decoder);

static DecodeStatus DecodeHWRegsRegisterClass(MCInst &Inst,
                                              unsigned Insn,
                                              uint64_t Address,
                                              const void *Decoder);

static DecodeStatus DecodeAFGR64RegisterClass(MCInst &Inst,
                                              unsigned RegNo,
                                              uint64_t Address,
                                              const void *Decoder);

static DecodeStatus DecodeHWRegs64RegisterClass(MCInst &Inst,
                                                unsigned Insn,
                                                uint64_t Address,
                                                const void *Decoder);

static DecodeStatus DecodeACRegsRegisterClass(MCInst &Inst,
                                              unsigned RegNo,
                                              uint64_t Address,
                                              const void *Decoder);

static DecodeStatus DecodeBranchTarget(MCInst &Inst,
                                       unsigned Offset,
                                       uint64_t Address,
                                       const void *Decoder);

static DecodeStatus DecodeBC1(MCInst &Inst,
                              unsigned Insn,
                              uint64_t Address,
                              const void *Decoder);


static DecodeStatus DecodeJumpTarget(MCInst &Inst,
                                     unsigned Insn,
                                     uint64_t Address,
                                     const void *Decoder);

static DecodeStatus DecodeMem(MCInst &Inst,
                              unsigned Insn,
                              uint64_t Address,
                              const void *Decoder);

static DecodeStatus DecodeFMem(MCInst &Inst, unsigned Insn,
                               uint64_t Address,
                               const void *Decoder);

static DecodeStatus DecodeSimm16(MCInst &Inst,
                                 unsigned Insn,
                                 uint64_t Address,
                                 const void *Decoder);

static DecodeStatus DecodeCondCode(MCInst &Inst,
                                   unsigned Insn,
                                   uint64_t Address,
                                   const void *Decoder);

static DecodeStatus DecodeInsSize(MCInst &Inst,
                                  unsigned Insn,
                                  uint64_t Address,
                                  const void *Decoder);

static DecodeStatus DecodeExtSize(MCInst &Inst,
                                  unsigned Insn,
                                  uint64_t Address,
                                  const void *Decoder);

namespace llvm {
extern Target TheOielTarget, TheOiTarget, TheOi64Target,
              TheOi64elTarget;
}

static MCDisassembler *createOiDisassembler(
                       const Target &T,
                       const MCSubtargetInfo &STI) {
  return new OiDisassembler(STI, T.createMCRegInfo(""), true);
}

static MCDisassembler *createOielDisassembler(
                       const Target &T,
                       const MCSubtargetInfo &STI) {
  return new OiDisassembler(STI, T.createMCRegInfo(""), false);
}

static MCDisassembler *createOi64Disassembler(
                       const Target &T,
                       const MCSubtargetInfo &STI) {
  return new Oi64Disassembler(STI, T.createMCRegInfo(""), true);
}

static MCDisassembler *createOi64elDisassembler(
                       const Target &T,
                       const MCSubtargetInfo &STI) {
  return new Oi64Disassembler(STI, T.createMCRegInfo(""), false);
}

extern "C" void LLVMInitializeOiDisassembler() {
  // Register the disassembler.
  TargetRegistry::RegisterMCDisassembler(TheOiTarget,
                                         createOiDisassembler);
  TargetRegistry::RegisterMCDisassembler(TheOielTarget,
                                         createOielDisassembler);
  TargetRegistry::RegisterMCDisassembler(TheOi64Target,
                                         createOi64Disassembler);
  TargetRegistry::RegisterMCDisassembler(TheOi64elTarget,
                                         createOi64elDisassembler);
}


#include "OiGenDisassemblerTables.inc"

  /// readInstruction - read four bytes from the MemoryObject
  /// and return 32 bit word sorted according to the given endianess
static DecodeStatus readInstruction32(const MemoryObject &region,
                                      uint64_t address,
                                      uint64_t &size,
                                      uint32_t &insn,
                                      bool isBigEndian) {
  uint8_t Bytes[4];

  // We want to read exactly 4 Bytes of data.
  if (region.readBytes(address, 4, (uint8_t*)Bytes, NULL) == -1) {
    size = 0;
    return MCDisassembler::Fail;
  }

  if (isBigEndian) {
    // Encoded as a big-endian 32-bit word in the stream.
    insn = (Bytes[3] <<  0) |
           (Bytes[2] <<  8) |
           (Bytes[1] << 16) |
           (Bytes[0] << 24);
  }
  else {
    // Encoded as a small-endian 32-bit word in the stream.
    insn = (Bytes[0] <<  0) |
           (Bytes[1] <<  8) |
           (Bytes[2] << 16) |
           (Bytes[3] << 24);
  }

  return MCDisassembler::Success;
}

DecodeStatus
OiDisassembler::getInstruction(MCInst &instr,
                                 uint64_t &Size,
                                 const MemoryObject &Region,
                                 uint64_t Address,
                                 raw_ostream &vStream,
                                 raw_ostream &cStream) const {
  uint32_t Insn;

  DecodeStatus Result = readInstruction32(Region, Address, Size,
                                          Insn, isBigEndian);
  if (Result == MCDisassembler::Fail)
    return MCDisassembler::Fail;

  // Calling the auto-generated decoder function.
  Result = decodeInstruction(DecoderTableOi32, instr, Insn, Address,
                             this, STI);
  if (Result != MCDisassembler::Fail) {
    Size = 4;
    return Result;
  }

  return MCDisassembler::Fail;
}

DecodeStatus
Oi64Disassembler::getInstruction(MCInst &instr,
                                   uint64_t &Size,
                                   const MemoryObject &Region,
                                   uint64_t Address,
                                   raw_ostream &vStream,
                                   raw_ostream &cStream) const {
  uint32_t Insn;

  DecodeStatus Result = readInstruction32(Region, Address, Size,
                                          Insn, isBigEndian);
  if (Result == MCDisassembler::Fail)
    return MCDisassembler::Fail;

  // Calling the auto-generated decoder function.
  Result = decodeInstruction(DecoderTableOi6432, instr, Insn, Address,
                             this, STI);
  if (Result != MCDisassembler::Fail) {
    Size = 4;
    return Result;
  }
  // If we fail to decode in Oi64 decoder space we can try in Oi32
  Result = decodeInstruction(DecoderTableOi32, instr, Insn, Address,
                             this, STI);
  if (Result != MCDisassembler::Fail) {
    Size = 4;
    return Result;
  }

  return MCDisassembler::Fail;
}

static unsigned getReg(const void *D, unsigned RC, unsigned RegNo) {
  const OiDisassemblerBase *Dis = static_cast<const OiDisassemblerBase*>(D);
  return *(Dis->getRegInfo()->getRegClass(RC).begin() + RegNo);
}

static DecodeStatus DecodeCPU64RegsRegisterClass(MCInst &Inst,
                                                 unsigned RegNo,
                                                 uint64_t Address,
                                                 const void *Decoder) {

  if (RegNo > 31)
    return MCDisassembler::Fail;

  unsigned Reg = getReg(Decoder, Oi::CPU64RegsRegClassID, RegNo);
  Inst.addOperand(MCOperand::CreateReg(Reg));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeCPURegsRegisterClass(MCInst &Inst,
                                               unsigned RegNo,
                                               uint64_t Address,
                                               const void *Decoder) {
  if (RegNo > 31)
    return MCDisassembler::Fail;
  unsigned Reg = getReg(Decoder, Oi::CPURegsRegClassID, RegNo);
  Inst.addOperand(MCOperand::CreateReg(Reg));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeDSPRegsRegisterClass(MCInst &Inst,
                                               unsigned RegNo,
                                               uint64_t Address,
                                               const void *Decoder) {
  return DecodeCPURegsRegisterClass(Inst, RegNo, Address, Decoder);
}

static DecodeStatus DecodeFGR64RegisterClass(MCInst &Inst,
                                             unsigned RegNo,
                                             uint64_t Address,
                                             const void *Decoder) {
  if (RegNo > 31)
    return MCDisassembler::Fail;

  unsigned Reg = getReg(Decoder, Oi::FGR64RegClassID, RegNo);
  Inst.addOperand(MCOperand::CreateReg(Reg));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeFGR32RegisterClass(MCInst &Inst,
                                             unsigned RegNo,
                                             uint64_t Address,
                                             const void *Decoder) {
  if (RegNo > 31)
    return MCDisassembler::Fail;

  unsigned Reg = getReg(Decoder, Oi::FGR32RegClassID, RegNo);
  Inst.addOperand(MCOperand::CreateReg(Reg));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeCCRRegisterClass(MCInst &Inst,
                                           unsigned RegNo,
                                           uint64_t Address,
                                           const void *Decoder) {
  Inst.addOperand(MCOperand::CreateReg(RegNo));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeMem(MCInst &Inst,
                              unsigned Insn,
                              uint64_t Address,
                              const void *Decoder) {
  int Offset = SignExtend32<16>(Insn & 0xffff);
  unsigned Reg = fieldFromInstruction(Insn, 16, 5);
  unsigned Base = fieldFromInstruction(Insn, 21, 5);

  Reg = getReg(Decoder, Oi::CPURegsRegClassID, Reg);
  Base = getReg(Decoder, Oi::CPURegsRegClassID, Base);

  if(Inst.getOpcode() == Oi::SC){
    Inst.addOperand(MCOperand::CreateReg(Reg));
  }

  Inst.addOperand(MCOperand::CreateReg(Reg));
  Inst.addOperand(MCOperand::CreateReg(Base));
  Inst.addOperand(MCOperand::CreateImm(Offset));

  return MCDisassembler::Success;
}

static DecodeStatus DecodeFMem(MCInst &Inst,
                               unsigned Insn,
                               uint64_t Address,
                               const void *Decoder) {
  int Offset = SignExtend32<16>(Insn & 0xffff);
  unsigned Reg = fieldFromInstruction(Insn, 16, 5);
  unsigned Base = fieldFromInstruction(Insn, 21, 5);

  Reg = getReg(Decoder, Oi::FGR64RegClassID, Reg);
  Base = getReg(Decoder, Oi::CPURegsRegClassID, Base);

  Inst.addOperand(MCOperand::CreateReg(Reg));
  Inst.addOperand(MCOperand::CreateReg(Base));
  Inst.addOperand(MCOperand::CreateImm(Offset));

  return MCDisassembler::Success;
}


static DecodeStatus DecodeHWRegsRegisterClass(MCInst &Inst,
                                              unsigned RegNo,
                                              uint64_t Address,
                                              const void *Decoder) {
  // Currently only hardware register 29 is supported.
  if (RegNo != 29)
    return  MCDisassembler::Fail;
  Inst.addOperand(MCOperand::CreateReg(Oi::HWR29));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeCondCode(MCInst &Inst,
                                   unsigned Insn,
                                   uint64_t Address,
                                   const void *Decoder) {
  int CondCode = Insn & 0xf;
  Inst.addOperand(MCOperand::CreateImm(CondCode));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeAFGR64RegisterClass(MCInst &Inst,
                                              unsigned RegNo,
                                              uint64_t Address,
                                              const void *Decoder) {
  if (RegNo > 30 || RegNo %2)
    return MCDisassembler::Fail;

  ;
  unsigned Reg = getReg(Decoder, Oi::AFGR64RegClassID, RegNo /2);
  Inst.addOperand(MCOperand::CreateReg(Reg));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeHWRegs64RegisterClass(MCInst &Inst,
                                                unsigned RegNo,
                                                uint64_t Address,
                                                const void *Decoder) {
  //Currently only hardware register 29 is supported
  if (RegNo != 29)
    return  MCDisassembler::Fail;
  Inst.addOperand(MCOperand::CreateReg(Oi::HWR29_64));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeACRegsRegisterClass(MCInst &Inst,
                                              unsigned RegNo,
                                              uint64_t Address,
                                              const void *Decoder) {
  if (RegNo >= 4)
    return MCDisassembler::Fail;

  unsigned Reg = getReg(Decoder, Oi::ACRegsRegClassID, RegNo);
  Inst.addOperand(MCOperand::CreateReg(Reg));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeBranchTarget(MCInst &Inst,
                                       unsigned Offset,
                                       uint64_t Address,
                                       const void *Decoder) {
  unsigned BranchOffset = Offset & 0xffff;
  BranchOffset = SignExtend32<18>(BranchOffset << 2) + 4;
  Inst.addOperand(MCOperand::CreateImm(BranchOffset));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeBC1(MCInst &Inst,
                              unsigned Insn,
                              uint64_t Address,
                              const void *Decoder) {
  unsigned BranchOffset = Insn & 0xffff;
  BranchOffset = SignExtend32<18>(BranchOffset << 2) + 4;
  Inst.addOperand(MCOperand::CreateImm(BranchOffset));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeJumpTarget(MCInst &Inst,
                                     unsigned Insn,
                                     uint64_t Address,
                                     const void *Decoder) {

  unsigned JumpOffset = fieldFromInstruction(Insn, 0, 26) << 2;
  Inst.addOperand(MCOperand::CreateImm(JumpOffset));
  return MCDisassembler::Success;
}


static DecodeStatus DecodeSimm16(MCInst &Inst,
                                 unsigned Insn,
                                 uint64_t Address,
                                 const void *Decoder) {
  Inst.addOperand(MCOperand::CreateImm(SignExtend32<16>(Insn)));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeInsSize(MCInst &Inst,
                                  unsigned Insn,
                                  uint64_t Address,
                                  const void *Decoder) {
  // First we need to grab the pos(lsb) from MCInst.
  int Pos = Inst.getOperand(2).getImm();
  int Size = (int) Insn - Pos + 1;
  Inst.addOperand(MCOperand::CreateImm(SignExtend32<16>(Size)));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeExtSize(MCInst &Inst,
                                  unsigned Insn,
                                  uint64_t Address,
                                  const void *Decoder) {
  int Size = (int) Insn  + 1;
  Inst.addOperand(MCOperand::CreateImm(SignExtend32<16>(Size)));
  return MCDisassembler::Success;
}
