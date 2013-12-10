//=== SyscallsIface.cpp - -------------------------------------- -*- C++ -*-==//
//
// Generate code to call host syscalls or libc functions during static
// binary translation.
//
//===----------------------------------------------------------------------===//
#include "SyscallsIface.h"
#include "OiInstrInfo.h"
#include "SBTUtils.h"

using namespace llvm;

bool SyscallsIface::HandleLibcAtoi(Value *&V, Value **First) {
  SmallVector<Type*, 8> args(1, Type::getInt32Ty(getGlobalContext()));
  FunctionType *ft = FunctionType::get(Type::getInt32Ty(getGlobalContext()),
                                       args, /*isvararg*/false);
  Value *fun = TheModule->getOrInsertFunction("atoi", ft);
  SmallVector<Value*, 8> params;
  Value *f = Builder.CreateLoad(IREmitter.Regs[ConvToDirective(Oi::A0)]);
  if (First)
    *First = f;
  Value *addrbuf = IREmitter.AccessShadowMemory(f, false);
  params.push_back(Builder.CreatePtrToInt(addrbuf,
                                          Type::getInt32Ty(getGlobalContext())));
  V = Builder.CreateStore(Builder.CreateCall(fun, params), IREmitter.Regs[ConvToDirective
                                                                (Oi::V0)]);
  ReadMap[ConvToDirective(Oi::A0)] = true;
  WriteMap[ConvToDirective(Oi::V0)] = true;
  return true;
}

bool SyscallsIface::HandleLibcMalloc(Value *&V, Value **First) {
  SmallVector<Type*, 8> args(1, Type::getInt32Ty(getGlobalContext()));
  FunctionType *ft = FunctionType::get(Type::getInt32Ty(getGlobalContext()),
                                       args, /*isvararg*/false);
  Value *fun = TheModule->getOrInsertFunction("malloc", ft);
  SmallVector<Value*, 8> params;
  Value *f = Builder.CreateLoad(IREmitter.Regs[ConvToDirective(Oi::A0)]);
  if (First)
    *First = f;
  params.push_back(f);
  Value *mal = Builder.CreateCall(fun, params);
  Value *ptr = Builder.CreatePtrToInt(IREmitter.ShadowImageValue,
                                      Type::getInt32Ty(getGlobalContext()));
  Value *fixed = Builder.CreateSub(mal, ptr);
  V = Builder.CreateStore(fixed, IREmitter.Regs[ConvToDirective
                                      (Oi::V0)]);
  ReadMap[ConvToDirective(Oi::A0)] = true;
  WriteMap[ConvToDirective(Oi::V0)] = true;
  return true;
}

bool SyscallsIface::HandleLibcCalloc(Value *&V, Value **First) {
  SmallVector<Type*, 8> args(2, Type::getInt32Ty(getGlobalContext()));
  FunctionType *ft = FunctionType::get(Type::getInt32Ty(getGlobalContext()),
                                       args, /*isvararg*/false);
  Value *fun = TheModule->getOrInsertFunction("calloc", ft);
  Value *f = Builder.CreateLoad(IREmitter.Regs[ConvToDirective(Oi::A0)]);
  if (First)
    *First = f;
  SmallVector<Value*, 8> params;
  params.push_back(f);
  params.push_back(Builder.CreateLoad(IREmitter.Regs[ConvToDirective(Oi::A1)]));
  Value *mal = Builder.CreateCall(fun, params);
  Value *ptr = Builder.CreatePtrToInt(IREmitter.ShadowImageValue,
                                      Type::getInt32Ty(getGlobalContext()));
  Value *fixed = Builder.CreateSub(mal, ptr);
  V = Builder.CreateStore(fixed, IREmitter.Regs[ConvToDirective
                                      (Oi::V0)]);
  ReadMap[ConvToDirective(Oi::A0)] = true;
  ReadMap[ConvToDirective(Oi::A1)] = true;
  WriteMap[ConvToDirective(Oi::V0)] = true;
  return true;
}

bool SyscallsIface::HandleLibcFree(Value *&V, Value **First) {
  SmallVector<Type*, 8> args(1, Type::getInt32Ty(getGlobalContext()));
  FunctionType *ft = FunctionType::get(Type::getVoidTy(getGlobalContext()),
                                       args, /*isvararg*/false);
  Value *fun = TheModule->getOrInsertFunction("free", ft);
  SmallVector<Value*, 8> params;
  Value *f = Builder.CreateLoad(IREmitter.Regs[ConvToDirective(Oi::A0)]);
  if (First)
    *First = f;
  Value *addrbuf = IREmitter.AccessShadowMemory
    (f, false);
  params.push_back(Builder.CreatePtrToInt(addrbuf,
                                          Type::getInt32Ty(getGlobalContext())));
  V = Builder.CreateCall(fun, params);
  ReadMap[ConvToDirective(Oi::A0)] = true;
  return true;
}

bool SyscallsIface::HandleLibcExit(Value *&V, Value **First) {
  SmallVector<Type*, 8> args(1, Type::getInt32Ty(getGlobalContext()));
  FunctionType *ft = FunctionType::get(Type::getVoidTy(getGlobalContext()),
                                       args, /*isvararg*/false);
  Value *fun = TheModule->getOrInsertFunction("exit", ft);
  SmallVector<Value*, 8> params;
  Value *f = Builder.CreateLoad(IREmitter.Regs[ConvToDirective(Oi::A0)]);
  if (First)
    *First = f;
  params.push_back(f);
  V = Builder.CreateCall(fun, params);
  ReadMap[ConvToDirective(Oi::A0)] = true;
  return true;
}

bool SyscallsIface::HandleLibcPuts(Value *&V, Value **First) {
  SmallVector<Type*, 8> args(1, Type::getInt32Ty(getGlobalContext()));
  FunctionType *ft = FunctionType::get(Type::getInt32Ty(getGlobalContext()),
                                       args, /*isvararg*/false);
  Value *fun = TheModule->getOrInsertFunction("puts", ft);
  SmallVector<Value*, 8> params;
  Value *f = Builder.CreateLoad(IREmitter.Regs[ConvToDirective(Oi::A0)]);
  if (First)
    *First = f;
  Value *addrbuf = IREmitter.AccessShadowMemory
    (f, false);
  params.push_back(Builder.CreatePtrToInt(addrbuf,
                                          Type::getInt32Ty(getGlobalContext())));
  V = Builder.CreateStore(Builder.CreateCall(fun, params), IREmitter.Regs[ConvToDirective
                                                                (Oi::V0)]);
  ReadMap[ConvToDirective(Oi::A0)] = true;
  WriteMap[ConvToDirective(Oi::V0)] = true;
  return true;
}

bool SyscallsIface::HandleLibcMemset(Value *&V, Value **First) {
  SmallVector<Type*, 8> args(3, Type::getInt32Ty(getGlobalContext()));
  FunctionType *ft = FunctionType::get(Type::getInt32Ty(getGlobalContext()),
                                       args, /*isvararg*/false);
  Value *fun = TheModule->getOrInsertFunction("memset", ft);
  SmallVector<Value*, 8> params;
  Value *f = Builder.CreateLoad(IREmitter.Regs[ConvToDirective(Oi::A0)]);
  if (First)
    *First = f;
  Value *addrbuf = IREmitter.AccessShadowMemory
    (f, false);
  params.push_back(Builder.CreatePtrToInt(addrbuf,
                                          Type::getInt32Ty(getGlobalContext())));
  params.push_back(Builder.CreateLoad(IREmitter.Regs[ConvToDirective(Oi::A1)]));
  params.push_back(Builder.CreateLoad(IREmitter.Regs[ConvToDirective(Oi::A2)]));
  V = Builder.CreateStore(Builder.CreateCall(fun, params), IREmitter.Regs[ConvToDirective
                                                                (Oi::V0)]);
  ReadMap[ConvToDirective(Oi::A0)] = true;
  ReadMap[ConvToDirective(Oi::A1)] = true;
  ReadMap[ConvToDirective(Oi::A2)] = true;
  WriteMap[ConvToDirective(Oi::V0)] = true;
  return true;
}

// XXX: Handling a fixed number of 4 arguments, since we cannot infer how many
// arguments the program is using with fprintf
bool SyscallsIface::HandleLibcFwrite(Value *&V, Value **First) {
  SmallVector<Type*, 8> args(4, Type::getInt32Ty(getGlobalContext()));
  FunctionType *ft = FunctionType::get(Type::getInt32Ty(getGlobalContext()),
                                       args, /*isvararg*/false);
  Value *fun = TheModule->getOrInsertFunction("fwrite", ft);
  SmallVector<Value*, 8> params;
  Value *f = Builder.CreateLoad(IREmitter.Regs[ConvToDirective(Oi::A0)]);
  if (First)
    *First = f;
  Value *addrbuf = IREmitter.AccessShadowMemory(f, false);
  params.push_back(Builder.CreatePtrToInt(addrbuf,
                                          Type::getInt32Ty(getGlobalContext())));
  params.push_back(Builder.CreateLoad(IREmitter.Regs[ConvToDirective(Oi::A1)]));
  params.push_back(Builder.CreateLoad(IREmitter.Regs[ConvToDirective(Oi::A2)]));
  addrbuf = IREmitter.AccessShadowMemory
    (Builder.CreateLoad(IREmitter.Regs[ConvToDirective(Oi::A3)]), false);
  params.push_back(Builder.CreatePtrToInt(addrbuf,
                                          Type::getInt32Ty(getGlobalContext())));
  V = Builder.CreateStore(Builder.CreateCall(fun, params), IREmitter.Regs[ConvToDirective
                                                                (Oi::V0)]);
  ReadMap[ConvToDirective(Oi::A0)] = true;
  ReadMap[ConvToDirective(Oi::A1)] = true;
  ReadMap[ConvToDirective(Oi::A2)] = true;
  ReadMap[ConvToDirective(Oi::A3)] = true;
  WriteMap[ConvToDirective(Oi::V0)] = true;
  return true;
}

// XXX: Handling a fixed number of 4 arguments, since we cannot infer how many
// arguments the program is using with fprintf
bool SyscallsIface::HandleLibcFprintf(Value *&V, Value **First) {
  SmallVector<Type*, 8> args(2, Type::getInt32Ty(getGlobalContext()));
  FunctionType *ft = FunctionType::get(Type::getInt32Ty(getGlobalContext()),
                                       args, /*isvararg*/true);
  Value *fun = TheModule->getOrInsertFunction("fprintf", ft);
  SmallVector<Value*, 8> params;
  Value *f = Builder.CreateLoad(IREmitter.Regs[ConvToDirective(Oi::A0)]);
  if (First)
    *First = f;
  params.push_back(f);
  Value *addrbuf = IREmitter.AccessShadowMemory
    (Builder.CreateLoad(IREmitter.Regs[ConvToDirective(Oi::A1)]), false);
  params.push_back(Builder.CreatePtrToInt(addrbuf,
                                          Type::getInt32Ty(getGlobalContext())));
  params.push_back(Builder.CreateLoad(IREmitter.Regs[ConvToDirective(Oi::A2)]));
  params.push_back(Builder.CreateLoad(IREmitter.Regs[ConvToDirective(Oi::A3)]));
  V = Builder.CreateStore(Builder.CreateCall(fun, params), IREmitter.Regs[ConvToDirective
                                                                (Oi::V0)]);
  ReadMap[ConvToDirective(Oi::A0)] = true;
  ReadMap[ConvToDirective(Oi::A1)] = true;
  ReadMap[ConvToDirective(Oi::A2)] = true;
  ReadMap[ConvToDirective(Oi::A3)] = true;
  WriteMap[ConvToDirective(Oi::V0)] = true;
  return true;
}

// XXX: Handling a fixed number of 4 arguments, since we cannot infer how many
// arguments the program is using with printf
bool SyscallsIface::HandleLibcPrintf(Value *&V, Value **First) {
  SmallVector<Type*, 8> args(1, Type::getInt32Ty(getGlobalContext()));
  FunctionType *ft = FunctionType::get(Type::getInt32Ty(getGlobalContext()),
                                       args, /*isvararg*/true);
  Value *fun = TheModule->getOrInsertFunction("printf", ft);
  SmallVector<Value*, 8> params;
  Value *f = Builder.CreateLoad(IREmitter.Regs[ConvToDirective(Oi::A0)]);
  if (First)
    *First = f;
  Value *addrbuf = IREmitter.AccessShadowMemory(f, false);
  params.push_back(Builder.CreatePtrToInt(addrbuf,
                                          Type::getInt32Ty(getGlobalContext())));
  params.push_back(Builder.CreateLoad(IREmitter.Regs[ConvToDirective(Oi::A1)]));
  params.push_back(Builder.CreateLoad(IREmitter.Regs[ConvToDirective(Oi::A2)]));
  params.push_back(Builder.CreateLoad(IREmitter.Regs[ConvToDirective(Oi::A3)]));
  V = Builder.CreateStore(Builder.CreateCall(fun, params), IREmitter.Regs[ConvToDirective
                                                                (Oi::V0)]);
  ReadMap[ConvToDirective(Oi::A0)] = true;
  ReadMap[ConvToDirective(Oi::A1)] = true;
  ReadMap[ConvToDirective(Oi::A2)] = true;
  ReadMap[ConvToDirective(Oi::A3)] = true;
  WriteMap[ConvToDirective(Oi::V0)] = true;
  return true;
}

// XXX: Handling a fixed number of 4 arguments, since we cannot infer how many
// arguments the program is using with scanf
bool SyscallsIface::HandleLibcScanf(Value *&V, Value **First) {
  SmallVector<Type*, 8> args(1, Type::getInt32Ty(getGlobalContext()));
  FunctionType *ft = FunctionType::get(Type::getInt32Ty(getGlobalContext()),
                                       args, /*isvararg*/true);
  Value *fun = TheModule->getOrInsertFunction("__isoc99_scanf", ft);
  SmallVector<Value*, 8> params;
  Value *f = Builder.CreateLoad(IREmitter.Regs[ConvToDirective(Oi::A0)]);
  if (First)
    *First = f;
  Value *addrbuf0 = IREmitter.AccessShadowMemory(f, false);
  Value *addrbuf1 = IREmitter.AccessShadowMemory
    (Builder.CreateLoad(IREmitter.Regs[ConvToDirective(Oi::A1)]), false);
  Value *addrbuf2 = IREmitter.AccessShadowMemory
    (Builder.CreateLoad(IREmitter.Regs[ConvToDirective(Oi::A2)]), false);
  Value *addrbuf3 = IREmitter.AccessShadowMemory
    (Builder.CreateLoad(IREmitter.Regs[ConvToDirective(Oi::A3)]), false);
  params.push_back(Builder.CreatePtrToInt(addrbuf0,
                                          Type::getInt32Ty(getGlobalContext())));
  params.push_back(Builder.CreatePtrToInt(addrbuf1,
                                          Type::getInt32Ty(getGlobalContext())));
  params.push_back(Builder.CreatePtrToInt(addrbuf2,
                                          Type::getInt32Ty(getGlobalContext())));
  params.push_back(Builder.CreatePtrToInt(addrbuf3,
                                          Type::getInt32Ty(getGlobalContext())));
  V = Builder.CreateStore(Builder.CreateCall(fun, params), IREmitter.Regs[ConvToDirective
                                                                (Oi::V0)]);
  ReadMap[ConvToDirective(Oi::A0)] = true;
  ReadMap[ConvToDirective(Oi::A1)] = true;
  ReadMap[ConvToDirective(Oi::A2)] = true;
  ReadMap[ConvToDirective(Oi::A3)] = true;
  WriteMap[ConvToDirective(Oi::V0)] = true;
  return true;
}

bool SyscallsIface::HandleSyscallWrite(Value *&V, Value **First) {
  SmallVector<Type*, 8> args(3, Type::getInt32Ty(getGlobalContext()));
  FunctionType *ft = FunctionType::get(Type::getInt32Ty(getGlobalContext()),
                                       args, /*isvararg*/false);
  Value *fun = TheModule->getOrInsertFunction("write", ft);
  SmallVector<Value*, 8> params;
  Value *f = Builder.CreateLoad(IREmitter.Regs[ConvToDirective(Oi::A0)]);
  if (First)
    *First = f;
  params.push_back(f);
  Value *addrbuf = IREmitter.AccessShadowMemory
    (Builder.CreateLoad(IREmitter.Regs[ConvToDirective(Oi::A1)]), false);
  params.push_back(Builder.CreatePtrToInt(addrbuf,
                                          Type::getInt32Ty(getGlobalContext())));
  params.push_back(Builder.CreateLoad(IREmitter.Regs[ConvToDirective(Oi::A2)]));

  V = Builder.CreateStore(Builder.CreateCall(fun, params), IREmitter.Regs[ConvToDirective
                                                                (Oi::V0)]);
  ReadMap[ConvToDirective(Oi::A0)] = true;
  ReadMap[ConvToDirective(Oi::A1)] = true;
  ReadMap[ConvToDirective(Oi::A2)] = true;
  WriteMap[ConvToDirective(Oi::V0)] = true;
  return true;
}