//=== OiInstTranslate.h - Convert Oi MCInst to LLVM IR -*- C++ -*-==//
// 
// A MCInstPrinter subclass that specializes in converting OpenISA 
// MC Instructions to LLVM I.R. It is used for the static binary
// translator by reading a binary file and dumping its contents
// with this component. 
//
//===------------------------------------------------------------===//

#ifndef OIINSTTRANSLATE_H
#define OIINSTTRANSLATE_H
#include "OiIREmitter.h"
#include "RelocationReader.h"
#include "SyscallsIface.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/Object/ObjectFile.h"
#include "InstPrinter/OiInstPrinter.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"

namespace llvm {

namespace object{
class ObjectFile;
}

using namespace object;

class OiInstTranslate : public MCInstPrinter {
public:
  OiInstTranslate(const MCAsmInfo &MAI, const MCInstrInfo &MII,
                  const MCRegisterInfo &MRI, const ObjectFile *obj,
                  uint64_t Stacksz)
    : MCInstPrinter(MAI, MII, MRI), Obj(obj), IREmitter(obj, Stacksz),
      RelocReader(obj, IREmitter.CurSection, IREmitter.CurAddr),
      Syscalls(IREmitter), Builder(IREmitter.Builder),
      ReadMap(IREmitter.ReadMap), WriteMap(IREmitter.WriteMap)
  {
  }

  // Autogenerated by tblgen.
  void printInstruction(const MCInst *MI, raw_ostream &O);
  static const char *getRegisterName(unsigned RegNo);

  virtual void printRegName(raw_ostream &OS, unsigned RegNo) const;
  virtual void printInst(const MCInst *MI, raw_ostream &O, StringRef Annot);
  void printCPURegs(const MCInst *MI, unsigned OpNo, raw_ostream &O);

  bool printAliasInstr(const MCInst *MI, raw_ostream &OS);
  Module* takeModule();
  void StartFunction(Twine &N);
  void FinishFunction();
  void FinishModule();
  void UpdateCurAddr(uint64_t val) {
    IREmitter.UpdateCurAddr(val);
  }
  void SetCurSection(section_iterator *i) {
    IREmitter.SetCurSection(i);
  }

private:
  const ObjectFile *Obj;
  OiIREmitter IREmitter;
  RelocationReader RelocReader;
  SyscallsIface Syscalls;
  IRBuilder<> &Builder;
  DenseMap<int32_t, bool> &ReadMap, &WriteMap;

  bool HandleAluSrcOperand(const MCOperand &o, Value *&V, Value **First = 0);
  bool HandleAluDstOperand(const MCOperand &o, Value *&V);
  bool HandleMemExpr(const MCExpr &exp, Value *&V, bool IsLoad);
  bool HandleSpilledOperand(const MCOperand &o, const MCOperand &o2,
                            Value *&V, Value **First, bool IsLoad);
  bool HandleGetSpilledAddress(const MCOperand &o, const MCOperand &o2,
                         const MCOperand &dst, Value *&V, Value **First);
  bool HandleMemOperand(const MCOperand &o, const MCOperand &o2, Value *&V,
                        Value **First, bool IsLoad, int width = 32);
  bool HandleDoubleMemOperand(const MCOperand &o, const MCOperand &o2,
                              Value *&V1, Value **First, bool IsLoad);
  bool HandleFloatMemOperand(const MCOperand &o, const MCOperand &o2,
                             Value *&V, Value **First, bool IsLoad);
  bool HandleLUiOperand(const MCOperand &o, Value *&V, Value **First, bool IsLoad);
  bool HandleCallTarget(const MCOperand &o, Value *&V, Value **First = 0);
  bool HandleFCmpOperand(const MCOperand &o, Value *o0, Value *o1, Value *&V);
  bool HandleBranchTarget(const MCOperand &o, BasicBlock *&Addr, bool IsRelative = true);
  bool HandleSaveDouble(Value *In, Value *&Out1, Value *&Out2);
  bool HandleDoubleSrcOperand(const MCOperand &o, Value *&V, Value **First = 0);
  bool HandleDoubleDstOperand(const MCOperand &o, Value *&V);
  bool HandleSaveFloat(Value *In, Value *&V);
  bool HandleFloatSrcOperand(const MCOperand &o, Value *&V, Value **First = 0);
  bool HandleFloatDstOperand(const MCOperand &o, Value *&V);


  void printOperand(const MCInst *MI, unsigned OpNo, raw_ostream &O);
  void printUnsignedImm(const MCInst *MI, int opNum, raw_ostream &O);
  void printMemOperand(const MCInst *MI, int opNum, raw_ostream &O);
  void printMemOperandEA(const MCInst *MI, int opNum, raw_ostream &O);
  void printFCCOperand(const MCInst *MI, int opNum, raw_ostream &O);
};
} // end namespace llvm

#endif