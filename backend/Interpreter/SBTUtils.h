//=== SBTUtils.h - General utilities --------------------*- C++ -*-==//
// 
// Convenience functions to convert register numbers when reading
// an OpenISA binary and converting it to IR.
//
//===------------------------------------------------------------===//
#ifndef SBTUTILS_H
#define SBTUTILS_H
#include "llvm/Support/system_error.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/IR/Value.h"
#include <vector>
#include <utility>

namespace llvm {

namespace object{
class ObjectFile;
}

using namespace object;

unsigned conv32(unsigned regnum);
unsigned ConvFromDirective(unsigned regnum);
unsigned ConvToDirective(unsigned regnum);
unsigned ConvToDirectiveDbl(unsigned regnum);
bool error(error_code ec);
uint64_t GetELFOffset(section_iterator &i);
std::vector<std::pair<uint64_t, StringRef> > GetSymbolsList(const ObjectFile *Obj, section_iterator &i);
Value* GetFirstInstruction(Value *o0, Value *o1);
Value* GetFirstInstruction(Value *o0, Value *o1, Value *o2);
Value* GetFirstInstruction(Value *o0, Value *o1, Value *o2, Value *o3);
Value* GetFirstInstruction(Value *o0, Value *o1, Value *o2, Value *o3, Value *o4);
uint32_t GetInstructionSize();
}
#endif