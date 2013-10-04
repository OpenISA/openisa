//===-- OiAsmParser.cpp - Parse Oi assembly to MCInst instructions ----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/OiMCTargetDesc.h"
#include "OiRegisterInfo.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCParser/MCAsmLexer.h"
#include "llvm/MC/MCParser/MCParsedAsmOperand.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/MCTargetAsmParser.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

namespace {
class OiAssemblerOptions {
public:
  OiAssemblerOptions():
    aTReg(1), reorder(true), macro(true) {
  }

  unsigned getATRegNum() {return aTReg;}
  bool setATReg(unsigned Reg);

  bool isReorder() {return reorder;}
  void setReorder() {reorder = true;}
  void setNoreorder() {reorder = false;}

  bool isMacro() {return macro;}
  void setMacro() {macro = true;}
  void setNomacro() {macro = false;}

private:
  unsigned aTReg;
  bool reorder;
  bool macro;
};
}

namespace {
class OiAsmParser : public MCTargetAsmParser {

  enum FpFormatTy {
    FP_FORMAT_NONE = -1,
    FP_FORMAT_S,
    FP_FORMAT_D,
    FP_FORMAT_L,
    FP_FORMAT_W
  } FpFormat;

  MCSubtargetInfo &STI;
  MCAsmParser &Parser;
  OiAssemblerOptions Options;


#define GET_ASSEMBLER_HEADER
#include "OiGenAsmMatcher.inc"

  bool MatchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                               SmallVectorImpl<MCParsedAsmOperand*> &Operands,
                               MCStreamer &Out, unsigned &ErrorInfo,
                               bool MatchingInlineAsm);

  bool ParseRegister(unsigned &RegNo, SMLoc &StartLoc, SMLoc &EndLoc);

  bool ParseInstruction(ParseInstructionInfo &Info, StringRef Name,
                        SMLoc NameLoc,
                        SmallVectorImpl<MCParsedAsmOperand*> &Operands);

  bool parseMathOperation(StringRef Name, SMLoc NameLoc,
                        SmallVectorImpl<MCParsedAsmOperand*> &Operands);

  bool ParseDirective(AsmToken DirectiveID);

  OiAsmParser::OperandMatchResultTy
  parseMemOperand(SmallVectorImpl<MCParsedAsmOperand*> &Operands);

  OiAsmParser::OperandMatchResultTy
  parseCPURegs(SmallVectorImpl<MCParsedAsmOperand*> &Operands);

  OiAsmParser::OperandMatchResultTy
  parseCPU64Regs(SmallVectorImpl<MCParsedAsmOperand*> &Operands);

  OiAsmParser::OperandMatchResultTy
  parseHWRegs(SmallVectorImpl<MCParsedAsmOperand*> &Operands);

  OiAsmParser::OperandMatchResultTy
  parseHW64Regs(SmallVectorImpl<MCParsedAsmOperand*> &Operands);

  OiAsmParser::OperandMatchResultTy
  parseCCRRegs(SmallVectorImpl<MCParsedAsmOperand*> &Operands);

  bool ParseOperand(SmallVectorImpl<MCParsedAsmOperand*> &,
                    StringRef Mnemonic);

  int tryParseRegister(bool is64BitReg);

  bool tryParseRegisterOperand(SmallVectorImpl<MCParsedAsmOperand*> &Operands,
                               bool is64BitReg);

  bool needsExpansion(MCInst &Inst);

  void expandInstruction(MCInst &Inst, SMLoc IDLoc,
                         SmallVectorImpl<MCInst> &Instructions);
  void expandLoadImm(MCInst &Inst, SMLoc IDLoc,
                     SmallVectorImpl<MCInst> &Instructions);
  void expandLoadAddressImm(MCInst &Inst, SMLoc IDLoc,
                            SmallVectorImpl<MCInst> &Instructions);
  void expandLoadAddressReg(MCInst &Inst, SMLoc IDLoc,
                            SmallVectorImpl<MCInst> &Instructions);
  bool reportParseError(StringRef ErrorMsg);

  bool parseMemOffset(const MCExpr *&Res);
  bool parseRelocOperand(const MCExpr *&Res);

  bool parseDirectiveSet();

  bool parseSetAtDirective();
  bool parseSetNoAtDirective();
  bool parseSetMacroDirective();
  bool parseSetNoMacroDirective();
  bool parseSetReorderDirective();
  bool parseSetNoReorderDirective();

  bool parseDirectiveWord(unsigned Size, SMLoc L);

  MCSymbolRefExpr::VariantKind getVariantKind(StringRef Symbol);

  bool isOi64() const {
    return (STI.getFeatureBits() & Oi::FeatureOi64) != 0;
  }

  bool isFP64() const {
    return (STI.getFeatureBits() & Oi::FeatureFP64Bit) != 0;
  }

  int matchRegisterName(StringRef Symbol, bool is64BitReg);

  int matchRegisterByNumber(unsigned RegNum, unsigned RegClass);

  void setFpFormat(FpFormatTy Format) {
    FpFormat = Format;
  }

  void setDefaultFpFormat();

  void setFpFormat(StringRef Format);

  FpFormatTy getFpFormat() {return FpFormat;}

  bool requestsDoubleOperand(StringRef Mnemonic);

  unsigned getReg(int RC,int RegNo);

  unsigned getATReg();
public:
  OiAsmParser(MCSubtargetInfo &sti, MCAsmParser &parser)
    : MCTargetAsmParser(), STI(sti), Parser(parser) {
    // Initialize the set of available features.
    setAvailableFeatures(ComputeAvailableFeatures(STI.getFeatureBits()));
  }

  MCAsmParser &getParser() const { return Parser; }
  MCAsmLexer &getLexer() const { return Parser.getLexer(); }

};
}

namespace {

/// OiOperand - Instances of this class represent a parsed Oi machine
/// instruction.
class OiOperand : public MCParsedAsmOperand {

public:
  enum RegisterKind {
    Kind_None,
    Kind_CPURegs,
    Kind_CPU64Regs,
    Kind_HWRegs,
    Kind_HW64Regs,
    Kind_FGR32Regs,
    Kind_FGR64Regs,
    Kind_AFGR32Regs,
    Kind_CCRRegs
  };

private:
  enum KindTy {
    k_CondCode,
    k_CoprocNum,
    k_Immediate,
    k_Memory,
    k_PostIndexRegister,
    k_Register,
    k_Token
  } Kind;

  OiOperand(KindTy K) : MCParsedAsmOperand(), Kind(K) {}

  union {
    struct {
      const char *Data;
      unsigned Length;
    } Tok;

    struct {
      unsigned RegNum;
      RegisterKind Kind;
    } Reg;

    struct {
      const MCExpr *Val;
    } Imm;

    struct {
      unsigned Base;
      const MCExpr *Off;
    } Mem;
  };

  SMLoc StartLoc, EndLoc;

public:
  void addRegOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    Inst.addOperand(MCOperand::CreateReg(getReg()));
  }

  void addExpr(MCInst &Inst, const MCExpr *Expr) const{
    // Add as immediate when possible.  Null MCExpr = 0.
    if (Expr == 0)
      Inst.addOperand(MCOperand::CreateImm(0));
    else if (const MCConstantExpr *CE = dyn_cast<MCConstantExpr>(Expr))
      Inst.addOperand(MCOperand::CreateImm(CE->getValue()));
    else
      Inst.addOperand(MCOperand::CreateExpr(Expr));
  }

  void addImmOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    const MCExpr *Expr = getImm();
    addExpr(Inst,Expr);
  }

  void addMemOperands(MCInst &Inst, unsigned N) const {
    assert(N == 2 && "Invalid number of operands!");

    Inst.addOperand(MCOperand::CreateReg(getMemBase()));

    const MCExpr *Expr = getMemOff();
    addExpr(Inst,Expr);
  }

  bool isReg() const { return Kind == k_Register; }
  bool isImm() const { return Kind == k_Immediate; }
  bool isToken() const { return Kind == k_Token; }
  bool isMem() const { return Kind == k_Memory; }

  StringRef getToken() const {
    assert(Kind == k_Token && "Invalid access!");
    return StringRef(Tok.Data, Tok.Length);
  }

  unsigned getReg() const {
    assert((Kind == k_Register) && "Invalid access!");
    return Reg.RegNum;
  }

  void setRegKind(RegisterKind RegKind) {
    assert((Kind == k_Register) && "Invalid access!");
    Reg.Kind = RegKind;
  }

  const MCExpr *getImm() const {
    assert((Kind == k_Immediate) && "Invalid access!");
    return Imm.Val;
  }

  unsigned getMemBase() const {
    assert((Kind == k_Memory) && "Invalid access!");
    return Mem.Base;
  }

  const MCExpr *getMemOff() const {
    assert((Kind == k_Memory) && "Invalid access!");
    return Mem.Off;
  }

  static OiOperand *CreateToken(StringRef Str, SMLoc S) {
    OiOperand *Op = new OiOperand(k_Token);
    Op->Tok.Data = Str.data();
    Op->Tok.Length = Str.size();
    Op->StartLoc = S;
    Op->EndLoc = S;
    return Op;
  }

  static OiOperand *CreateReg(unsigned RegNum, SMLoc S, SMLoc E) {
    OiOperand *Op = new OiOperand(k_Register);
    Op->Reg.RegNum = RegNum;
    Op->StartLoc = S;
    Op->EndLoc = E;
    return Op;
  }

  static OiOperand *CreateImm(const MCExpr *Val, SMLoc S, SMLoc E) {
    OiOperand *Op = new OiOperand(k_Immediate);
    Op->Imm.Val = Val;
    Op->StartLoc = S;
    Op->EndLoc = E;
    return Op;
  }

  static OiOperand *CreateMem(unsigned Base, const MCExpr *Off,
                                 SMLoc S, SMLoc E) {
    OiOperand *Op = new OiOperand(k_Memory);
    Op->Mem.Base = Base;
    Op->Mem.Off = Off;
    Op->StartLoc = S;
    Op->EndLoc = E;
    return Op;
  }

  bool isCPURegsAsm() const {
    return Kind == k_Register && Reg.Kind == Kind_CPURegs;
  }
  void addCPURegsAsmOperands(MCInst &Inst, unsigned N) const {
    Inst.addOperand(MCOperand::CreateReg(Reg.RegNum));
  }

  bool isCPU64RegsAsm() const {
    return Kind == k_Register && Reg.Kind == Kind_CPU64Regs;
  }
  void addCPU64RegsAsmOperands(MCInst &Inst, unsigned N) const {
    Inst.addOperand(MCOperand::CreateReg(Reg.RegNum));
  }

  bool isHWRegsAsm() const {
    assert((Kind == k_Register) && "Invalid access!");
    return Reg.Kind == Kind_HWRegs;
  }
  void addHWRegsAsmOperands(MCInst &Inst, unsigned N) const {
    Inst.addOperand(MCOperand::CreateReg(Reg.RegNum));
  }

  bool isHW64RegsAsm() const {
    assert((Kind == k_Register) && "Invalid access!");
    return Reg.Kind == Kind_HW64Regs;
  }
  void addHW64RegsAsmOperands(MCInst &Inst, unsigned N) const {
    Inst.addOperand(MCOperand::CreateReg(Reg.RegNum));
  }

  void addCCRAsmOperands(MCInst &Inst, unsigned N) const {
    Inst.addOperand(MCOperand::CreateReg(Reg.RegNum));
  }

  bool isCCRAsm() const {
    assert((Kind == k_Register) && "Invalid access!");
    return Reg.Kind == Kind_CCRRegs;
  }

  /// getStartLoc - Get the location of the first token of this operand.
  SMLoc getStartLoc() const { return StartLoc; }
  /// getEndLoc - Get the location of the last token of this operand.
  SMLoc getEndLoc() const { return EndLoc; }

  virtual void print(raw_ostream &OS) const {
    llvm_unreachable("unimplemented!");
  }
};
}

bool OiAsmParser::needsExpansion(MCInst &Inst) {

  switch(Inst.getOpcode()) {
    case Oi::LoadImm32Reg:
    case Oi::LoadAddr32Imm:
    case Oi::LoadAddr32Reg:
      return true;
    default:
      return false;
  }
}

void OiAsmParser::expandInstruction(MCInst &Inst, SMLoc IDLoc,
                        SmallVectorImpl<MCInst> &Instructions){
  switch(Inst.getOpcode()) {
    case Oi::LoadImm32Reg:
      return expandLoadImm(Inst, IDLoc, Instructions);
    case Oi::LoadAddr32Imm:
      return expandLoadAddressImm(Inst,IDLoc,Instructions);
    case Oi::LoadAddr32Reg:
      return expandLoadAddressReg(Inst,IDLoc,Instructions);
    }
}

void OiAsmParser::expandLoadImm(MCInst &Inst, SMLoc IDLoc,
                                  SmallVectorImpl<MCInst> &Instructions){
  MCInst tmpInst;
  const MCOperand &ImmOp = Inst.getOperand(1);
  assert(ImmOp.isImm() && "expected immediate operand kind");
  const MCOperand &RegOp = Inst.getOperand(0);
  assert(RegOp.isReg() && "expected register operand kind");

  int ImmValue = ImmOp.getImm();
  tmpInst.setLoc(IDLoc);
  if ( 0 <= ImmValue && ImmValue <= 65535) {
    // for 0 <= j <= 65535.
    // li d,j => ori d,$zero,j
    tmpInst.setOpcode(Oi::ORi);
    tmpInst.addOperand(MCOperand::CreateReg(RegOp.getReg()));
    tmpInst.addOperand(
              MCOperand::CreateReg(Oi::ZERO));
    tmpInst.addOperand(MCOperand::CreateImm(ImmValue));
    Instructions.push_back(tmpInst);
  } else if ( ImmValue < 0 && ImmValue >= -32768) {
    // for -32768 <= j < 0.
    // li d,j => addiu d,$zero,j
    tmpInst.setOpcode(Oi::ADDiu);
    tmpInst.addOperand(MCOperand::CreateReg(RegOp.getReg()));
    tmpInst.addOperand(
              MCOperand::CreateReg(Oi::ZERO));
    tmpInst.addOperand(MCOperand::CreateImm(ImmValue));
    Instructions.push_back(tmpInst);
  } else {
    // for any other value of j that is representable as a 32-bit integer.
    // li d,j => lui d,hi16(j)
    //           ori d,d,lo16(j)
    tmpInst.setOpcode(Oi::LUi);
    tmpInst.addOperand(MCOperand::CreateReg(RegOp.getReg()));
    tmpInst.addOperand(MCOperand::CreateImm((ImmValue & 0xffff0000) >> 16));
    Instructions.push_back(tmpInst);
    tmpInst.clear();
    tmpInst.setOpcode(Oi::ORi);
    tmpInst.addOperand(MCOperand::CreateReg(RegOp.getReg()));
    tmpInst.addOperand(MCOperand::CreateReg(RegOp.getReg()));
    tmpInst.addOperand(MCOperand::CreateImm(ImmValue & 0xffff));
    tmpInst.setLoc(IDLoc);
    Instructions.push_back(tmpInst);
  }
}

void OiAsmParser::expandLoadAddressReg(MCInst &Inst, SMLoc IDLoc,
                                         SmallVectorImpl<MCInst> &Instructions){
  MCInst tmpInst;
  const MCOperand &ImmOp = Inst.getOperand(2);
  assert(ImmOp.isImm() && "expected immediate operand kind");
  const MCOperand &SrcRegOp = Inst.getOperand(1);
  assert(SrcRegOp.isReg() && "expected register operand kind");
  const MCOperand &DstRegOp = Inst.getOperand(0);
  assert(DstRegOp.isReg() && "expected register operand kind");
  int ImmValue = ImmOp.getImm();
  if ( -32768 <= ImmValue && ImmValue <= 65535) {
    //for -32768 <= j <= 65535.
    //la d,j(s) => addiu d,s,j
    tmpInst.setOpcode(Oi::ADDiu);
    tmpInst.addOperand(MCOperand::CreateReg(DstRegOp.getReg()));
    tmpInst.addOperand(MCOperand::CreateReg(SrcRegOp.getReg()));
    tmpInst.addOperand(MCOperand::CreateImm(ImmValue));
    Instructions.push_back(tmpInst);
  } else {
    //for any other value of j that is representable as a 32-bit integer.
    //la d,j(s) => lui d,hi16(j)
    //             ori d,d,lo16(j)
    //             addu d,d,s
    tmpInst.setOpcode(Oi::LUi);
    tmpInst.addOperand(MCOperand::CreateReg(DstRegOp.getReg()));
    tmpInst.addOperand(MCOperand::CreateImm((ImmValue & 0xffff0000) >> 16));
    Instructions.push_back(tmpInst);
    tmpInst.clear();
    tmpInst.setOpcode(Oi::ORi);
    tmpInst.addOperand(MCOperand::CreateReg(DstRegOp.getReg()));
    tmpInst.addOperand(MCOperand::CreateReg(DstRegOp.getReg()));
    tmpInst.addOperand(MCOperand::CreateImm(ImmValue & 0xffff));
    Instructions.push_back(tmpInst);
    tmpInst.clear();
    tmpInst.setOpcode(Oi::ADDu);
    tmpInst.addOperand(MCOperand::CreateReg(DstRegOp.getReg()));
    tmpInst.addOperand(MCOperand::CreateReg(DstRegOp.getReg()));
    tmpInst.addOperand(MCOperand::CreateReg(SrcRegOp.getReg()));
    Instructions.push_back(tmpInst);
  }
}

void OiAsmParser::expandLoadAddressImm(MCInst &Inst, SMLoc IDLoc,
                                         SmallVectorImpl<MCInst> &Instructions){
  MCInst tmpInst;
  const MCOperand &ImmOp = Inst.getOperand(1);
  assert(ImmOp.isImm() && "expected immediate operand kind");
  const MCOperand &RegOp = Inst.getOperand(0);
  assert(RegOp.isReg() && "expected register operand kind");
  int ImmValue = ImmOp.getImm();
  if ( -32768 <= ImmValue && ImmValue <= 65535) {
    //for -32768 <= j <= 65535.
    //la d,j => addiu d,$zero,j
    tmpInst.setOpcode(Oi::ADDiu);
    tmpInst.addOperand(MCOperand::CreateReg(RegOp.getReg()));
    tmpInst.addOperand(
              MCOperand::CreateReg(Oi::ZERO));
    tmpInst.addOperand(MCOperand::CreateImm(ImmValue));
    Instructions.push_back(tmpInst);
  } else {
    //for any other value of j that is representable as a 32-bit integer.
    //la d,j => lui d,hi16(j)
    //          ori d,d,lo16(j)
    tmpInst.setOpcode(Oi::LUi);
    tmpInst.addOperand(MCOperand::CreateReg(RegOp.getReg()));
    tmpInst.addOperand(MCOperand::CreateImm((ImmValue & 0xffff0000) >> 16));
    Instructions.push_back(tmpInst);
    tmpInst.clear();
    tmpInst.setOpcode(Oi::ORi);
    tmpInst.addOperand(MCOperand::CreateReg(RegOp.getReg()));
    tmpInst.addOperand(MCOperand::CreateReg(RegOp.getReg()));
    tmpInst.addOperand(MCOperand::CreateImm(ImmValue & 0xffff));
    Instructions.push_back(tmpInst);
  }
}

bool OiAsmParser::
MatchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                        SmallVectorImpl<MCParsedAsmOperand*> &Operands,
                        MCStreamer &Out, unsigned &ErrorInfo,
                        bool MatchingInlineAsm) {
  MCInst Inst;
  unsigned MatchResult = MatchInstructionImpl(Operands, Inst, ErrorInfo,
                                              MatchingInlineAsm);

  switch (MatchResult) {
  default: break;
  case Match_Success: {
    if (needsExpansion(Inst)) {
      SmallVector<MCInst, 4> Instructions;
      expandInstruction(Inst, IDLoc, Instructions);
      for(unsigned i =0; i < Instructions.size(); i++){
        Out.EmitInstruction(Instructions[i]);
      }
    } else {
        Inst.setLoc(IDLoc);
        Out.EmitInstruction(Inst);
      }
    return false;
  }
  case Match_MissingFeature:
    Error(IDLoc, "instruction requires a CPU feature not currently enabled");
    return true;
  case Match_InvalidOperand: {
    SMLoc ErrorLoc = IDLoc;
    if (ErrorInfo != ~0U) {
      if (ErrorInfo >= Operands.size())
        return Error(IDLoc, "too few operands for instruction");

      ErrorLoc = ((OiOperand*)Operands[ErrorInfo])->getStartLoc();
      if (ErrorLoc == SMLoc()) ErrorLoc = IDLoc;
    }

    return Error(ErrorLoc, "invalid operand for instruction");
  }
  case Match_MnemonicFail:
    return Error(IDLoc, "invalid instruction");
  }
  return true;
}

int OiAsmParser::matchRegisterName(StringRef Name, bool is64BitReg) {

   int CC;
   if (!is64BitReg)
    CC = StringSwitch<unsigned>(Name)
      .Case("zero",  Oi::ZERO)
      .Case("a0",  Oi::A0)
      .Case("a1",  Oi::A1)
      .Case("a2",  Oi::A2)
      .Case("a3",  Oi::A3)
      .Case("v0",  Oi::V0)
      .Case("v1",  Oi::V1)
      .Case("s0",  Oi::S0)
      .Case("s1",  Oi::S1)
      .Case("s2",  Oi::S2)
      .Case("s3",  Oi::S3)
      .Case("s4",  Oi::S4)
      .Case("s5",  Oi::S5)
      .Case("s6",  Oi::S6)
      .Case("s7",  Oi::S7)
      .Case("k0",  Oi::K0)
      .Case("k1",  Oi::K1)
      .Case("sp",  Oi::SP)
      .Case("fp",  Oi::FP)
      .Case("gp",  Oi::GP)
      .Case("ra",  Oi::RA)
      .Case("t0",  Oi::T0)
      .Case("t1",  Oi::T1)
      .Case("t2",  Oi::T2)
      .Case("t3",  Oi::T3)
      .Case("t4",  Oi::T4)
      .Case("t5",  Oi::T5)
      .Case("t6",  Oi::T6)
      .Case("t7",  Oi::T7)
      .Case("t8",  Oi::T8)
      .Case("t9",  Oi::T9)
      .Case("at",  Oi::AT)
      .Case("fcc0",  Oi::FCC0)
      .Default(-1);
   else
    CC = StringSwitch<unsigned>(Name)
      .Case("zero", Oi::ZERO_64)
      .Case("at", Oi::AT_64)
      .Case("v0", Oi::V0_64)
      .Case("v1", Oi::V1_64)
      .Case("a0", Oi::A0_64)
      .Case("a1", Oi::A1_64)
      .Case("a2", Oi::A2_64)
      .Case("a3", Oi::A3_64)
      .Case("a4", Oi::T0_64)
      .Case("a5", Oi::T1_64)
      .Case("a6", Oi::T2_64)
      .Case("a7", Oi::T3_64)
      .Case("t4", Oi::T4_64)
      .Case("t5", Oi::T5_64)
      .Case("t6", Oi::T6_64)
      .Case("t7", Oi::T7_64)
      .Case("s0", Oi::S0_64)
      .Case("s1", Oi::S1_64)
      .Case("s2", Oi::S2_64)
      .Case("s3", Oi::S3_64)
      .Case("s4", Oi::S4_64)
      .Case("s5", Oi::S5_64)
      .Case("s6", Oi::S6_64)
      .Case("s7", Oi::S7_64)
      .Case("t8", Oi::T8_64)
      .Case("t9", Oi::T9_64)
      .Case("kt0", Oi::K0_64)
      .Case("kt1", Oi::K1_64)
      .Case("gp", Oi::GP_64)
      .Case("sp", Oi::SP_64)
      .Case("fp", Oi::FP_64)
      .Case("s8", Oi::FP_64)
      .Case("ra", Oi::RA_64)
      .Default(-1);

  if (CC != -1)
    return CC;

  if (Name[0] == 'f') {
    StringRef NumString = Name.substr(1);
    unsigned IntVal;
    if( NumString.getAsInteger(10, IntVal))
      return -1; // not integer
    if (IntVal > 31)
      return -1;

    FpFormatTy Format = getFpFormat();

    if (Format == FP_FORMAT_S || Format == FP_FORMAT_W)
      return getReg(Oi::FGR32RegClassID, IntVal);
    if (Format == FP_FORMAT_D) {
      if(isFP64()) {
        return getReg(Oi::FGR64RegClassID, IntVal);
      }
      // only even numbers available as register pairs
      if (( IntVal > 31) || (IntVal%2 !=  0))
        return -1;
      return getReg(Oi::AFGR64RegClassID, IntVal/2);
    }
  }

  return -1;
}
void OiAsmParser::setDefaultFpFormat() {

  if (isOi64() || isFP64())
    FpFormat = FP_FORMAT_D;
  else
    FpFormat = FP_FORMAT_S;
}

bool OiAsmParser::requestsDoubleOperand(StringRef Mnemonic){

  bool IsDouble = StringSwitch<bool>(Mnemonic.lower())
    .Case("ldxc1", true)
    .Case("ldc1",  true)
    .Case("sdxc1", true)
    .Case("sdc1",  true)
    .Default(false);

  return IsDouble;
}
void OiAsmParser::setFpFormat(StringRef Format) {

  FpFormat = StringSwitch<FpFormatTy>(Format.lower())
    .Case(".s",  FP_FORMAT_S)
    .Case(".d",  FP_FORMAT_D)
    .Case(".l",  FP_FORMAT_L)
    .Case(".w",  FP_FORMAT_W)
    .Default(FP_FORMAT_NONE);
}

bool OiAssemblerOptions::setATReg(unsigned Reg) {
  if (Reg > 31)
    return false;

  aTReg = Reg;
  return true;
}

unsigned OiAsmParser::getATReg() {
  unsigned Reg = Options.getATRegNum();
  if (isOi64())
    return getReg(Oi::CPU64RegsRegClassID,Reg);

  return getReg(Oi::CPURegsRegClassID,Reg);
}

unsigned OiAsmParser::getReg(int RC,int RegNo) {
  return *(getContext().getRegisterInfo().getRegClass(RC).begin() + RegNo);
}

int OiAsmParser::matchRegisterByNumber(unsigned RegNum, unsigned RegClass) {

  if (RegNum > 31)
    return -1;

  return getReg(RegClass, RegNum);
}

int OiAsmParser::tryParseRegister(bool is64BitReg) {
  const AsmToken &Tok = Parser.getTok();
  int RegNum = -1;

  if (Tok.is(AsmToken::Identifier)) {
    std::string lowerCase = Tok.getString().lower();
    RegNum = matchRegisterName(lowerCase, is64BitReg);
  } else if (Tok.is(AsmToken::Integer))
    RegNum = matchRegisterByNumber(static_cast<unsigned>(Tok.getIntVal()),
                                   is64BitReg ? Oi::CPU64RegsRegClassID
                                              : Oi::CPURegsRegClassID);
  return RegNum;
}

bool OiAsmParser::
  tryParseRegisterOperand(SmallVectorImpl<MCParsedAsmOperand*> &Operands,
                          bool is64BitReg){

  SMLoc S = Parser.getTok().getLoc();
  int RegNo = -1;

  RegNo = tryParseRegister(is64BitReg);
  if (RegNo == -1)
    return true;

  Operands.push_back(OiOperand::CreateReg(RegNo, S,
      Parser.getTok().getLoc()));
  Parser.Lex(); // Eat register token.
  return false;
}

bool OiAsmParser::ParseOperand(SmallVectorImpl<MCParsedAsmOperand*>&Operands,
                                 StringRef Mnemonic) {
  // Check if the current operand has a custom associated parser, if so, try to
  // custom parse the operand, or fallback to the general approach.
  OperandMatchResultTy ResTy = MatchOperandParserImpl(Operands, Mnemonic);
  if (ResTy == MatchOperand_Success)
    return false;
  // If there wasn't a custom match, try the generic matcher below. Otherwise,
  // there was a match, but an error occurred, in which case, just return that
  // the operand parsing failed.
  if (ResTy == MatchOperand_ParseFail)
    return true;

  switch (getLexer().getKind()) {
  default:
    Error(Parser.getTok().getLoc(), "unexpected token in operand");
    return true;
  case AsmToken::Dollar: {
    // parse register
    SMLoc S = Parser.getTok().getLoc();
    Parser.Lex(); // Eat dollar token.
    // parse register operand
    if (!tryParseRegisterOperand(Operands, isOi64())) {
      if (getLexer().is(AsmToken::LParen)) {
        // check if it is indexed addressing operand
        Operands.push_back(OiOperand::CreateToken("(", S));
        Parser.Lex(); // eat parenthesis
        if (getLexer().isNot(AsmToken::Dollar))
          return true;

        Parser.Lex(); // eat dollar
        if (tryParseRegisterOperand(Operands, isOi64()))
          return true;

        if (!getLexer().is(AsmToken::RParen))
          return true;

        S = Parser.getTok().getLoc();
        Operands.push_back(OiOperand::CreateToken(")", S));
        Parser.Lex();
      }
      return false;
    }
    // maybe it is a symbol reference
    StringRef Identifier;
    if (Parser.ParseIdentifier(Identifier))
      return true;

    SMLoc E = SMLoc::getFromPointer(Parser.getTok().getLoc().getPointer() - 1);

    MCSymbol *Sym = getContext().GetOrCreateSymbol("$" + Identifier);

    // Otherwise create a symbol ref.
    const MCExpr *Res = MCSymbolRefExpr::Create(Sym, MCSymbolRefExpr::VK_None,
                                                getContext());

    Operands.push_back(OiOperand::CreateImm(Res, S, E));
    return false;
  }
  case AsmToken::Identifier:
  case AsmToken::LParen:
  case AsmToken::Minus:
  case AsmToken::Plus:
  case AsmToken::Integer:
  case AsmToken::String: {
     // quoted label names
    const MCExpr *IdVal;
    SMLoc S = Parser.getTok().getLoc();
    if (getParser().ParseExpression(IdVal))
      return true;
    SMLoc E = SMLoc::getFromPointer(Parser.getTok().getLoc().getPointer() - 1);
    Operands.push_back(OiOperand::CreateImm(IdVal, S, E));
    return false;
  }
  case AsmToken::Percent: {
    // it is a symbol reference or constant expression
    const MCExpr *IdVal;
    SMLoc S = Parser.getTok().getLoc(); // start location of the operand
    if (parseRelocOperand(IdVal))
      return true;

    SMLoc E = SMLoc::getFromPointer(Parser.getTok().getLoc().getPointer() - 1);

    Operands.push_back(OiOperand::CreateImm(IdVal, S, E));
    return false;
  } // case AsmToken::Percent
  } // switch(getLexer().getKind())
  return true;
}

bool OiAsmParser::parseRelocOperand(const MCExpr *&Res) {

  Parser.Lex(); // eat % token
  const AsmToken &Tok = Parser.getTok(); // get next token, operation
  if (Tok.isNot(AsmToken::Identifier))
    return true;

  std::string Str = Tok.getIdentifier().str();

  Parser.Lex(); // eat identifier
  // now make expression from the rest of the operand
  const MCExpr *IdVal;
  SMLoc EndLoc;

  if (getLexer().getKind() == AsmToken::LParen) {
    while (1) {
      Parser.Lex(); // eat '(' token
      if (getLexer().getKind() == AsmToken::Percent) {
        Parser.Lex(); // eat % token
        const AsmToken &nextTok = Parser.getTok();
        if (nextTok.isNot(AsmToken::Identifier))
          return true;
        Str += "(%";
        Str += nextTok.getIdentifier();
        Parser.Lex(); // eat identifier
        if (getLexer().getKind() != AsmToken::LParen)
          return true;
      } else
        break;
    }
    if (getParser().ParseParenExpression(IdVal,EndLoc))
      return true;

    while (getLexer().getKind() == AsmToken::RParen)
      Parser.Lex(); // eat ')' token

  } else
    return true; // parenthesis must follow reloc operand

  // Check the type of the expression
  if (const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(IdVal)) {
    // it's a constant, evaluate lo or hi value
    int Val = MCE->getValue();
    if (Str == "lo") {
      Val = Val & 0xffff;
    } else if (Str == "hi") {
      Val = (Val & 0xffff0000) >> 16;
    }
    Res = MCConstantExpr::Create(Val, getContext());
    return false;
  }

  if (const MCSymbolRefExpr *MSRE = dyn_cast<MCSymbolRefExpr>(IdVal)) {
    // it's a symbol, create symbolic expression from symbol
    StringRef Symbol = MSRE->getSymbol().getName();
    MCSymbolRefExpr::VariantKind VK = getVariantKind(Str);
    Res = MCSymbolRefExpr::Create(Symbol,VK,getContext());
    return false;
  }
  return true;
}

bool OiAsmParser::ParseRegister(unsigned &RegNo, SMLoc &StartLoc,
                                  SMLoc &EndLoc) {

  StartLoc = Parser.getTok().getLoc();
  RegNo = tryParseRegister(isOi64());
  EndLoc = Parser.getTok().getLoc();
  return (RegNo == (unsigned)-1);
}

bool OiAsmParser::parseMemOffset(const MCExpr *&Res) {

  SMLoc S;

  switch(getLexer().getKind()) {
  default:
    return true;
  case AsmToken::Integer:
  case AsmToken::Minus:
  case AsmToken::Plus:
    return (getParser().ParseExpression(Res));
  case AsmToken::Percent:
    return parseRelocOperand(Res);
  case AsmToken::LParen:
    return false;  // it's probably assuming 0
  }
  return true;
}

OiAsmParser::OperandMatchResultTy OiAsmParser::parseMemOperand(
               SmallVectorImpl<MCParsedAsmOperand*>&Operands) {

  const MCExpr *IdVal = 0;
  SMLoc S;
  // first operand is the offset
  S = Parser.getTok().getLoc();

  if (parseMemOffset(IdVal))
    return MatchOperand_ParseFail;

  const AsmToken &Tok = Parser.getTok(); // get next token
  if (Tok.isNot(AsmToken::LParen)) {
    OiOperand *Mnemonic = static_cast<OiOperand*>(Operands[0]);
    if (Mnemonic->getToken() == "la") {
      SMLoc E = SMLoc::getFromPointer(Parser.getTok().getLoc().getPointer() -1);
      Operands.push_back(OiOperand::CreateImm(IdVal, S, E));
      return MatchOperand_Success;
    }
    Error(Parser.getTok().getLoc(), "'(' expected");
    return MatchOperand_ParseFail;
  }

  Parser.Lex(); // Eat '(' token.

  const AsmToken &Tok1 = Parser.getTok(); // get next token
  if (Tok1.is(AsmToken::Dollar)) {
    Parser.Lex(); // Eat '$' token.
    if (tryParseRegisterOperand(Operands, isOi64())) {
      Error(Parser.getTok().getLoc(), "unexpected token in operand");
      return MatchOperand_ParseFail;
    }

  } else {
    Error(Parser.getTok().getLoc(), "unexpected token in operand");
    return MatchOperand_ParseFail;
  }

  const AsmToken &Tok2 = Parser.getTok(); // get next token
  if (Tok2.isNot(AsmToken::RParen)) {
    Error(Parser.getTok().getLoc(), "')' expected");
    return MatchOperand_ParseFail;
  }

  SMLoc E = SMLoc::getFromPointer(Parser.getTok().getLoc().getPointer() - 1);

  Parser.Lex(); // Eat ')' token.

  if (IdVal == 0)
    IdVal = MCConstantExpr::Create(0, getContext());

  // now replace register operand with the mem operand
  OiOperand* op = static_cast<OiOperand*>(Operands.back());
  int RegNo = op->getReg();
  // remove register from operands
  Operands.pop_back();
  // and add memory operand
  Operands.push_back(OiOperand::CreateMem(RegNo, IdVal, S, E));
  delete op;
  return MatchOperand_Success;
}

OiAsmParser::OperandMatchResultTy
OiAsmParser::parseCPU64Regs(SmallVectorImpl<MCParsedAsmOperand*> &Operands) {

  if (!isOi64())
    return MatchOperand_NoMatch;
  // if the first token is not '$' we have an error
  if (Parser.getTok().isNot(AsmToken::Dollar))
    return MatchOperand_NoMatch;

  Parser.Lex(); // Eat $
  if(!tryParseRegisterOperand(Operands, true)) {
    // set the proper register kind
    OiOperand* op = static_cast<OiOperand*>(Operands.back());
    op->setRegKind(OiOperand::Kind_CPU64Regs);
    return MatchOperand_Success;
  }
  return MatchOperand_NoMatch;
}

OiAsmParser::OperandMatchResultTy
OiAsmParser::parseCPURegs(SmallVectorImpl<MCParsedAsmOperand*> &Operands) {

  // if the first token is not '$' we have an error
  if (Parser.getTok().isNot(AsmToken::Dollar))
    return MatchOperand_NoMatch;

  Parser.Lex(); // Eat $
  if(!tryParseRegisterOperand(Operands, false)) {
    // set the propper register kind
    OiOperand* op = static_cast<OiOperand*>(Operands.back());
    op->setRegKind(OiOperand::Kind_CPURegs);
    return MatchOperand_Success;
  }
  return MatchOperand_NoMatch;
}

OiAsmParser::OperandMatchResultTy
OiAsmParser::parseHWRegs(SmallVectorImpl<MCParsedAsmOperand*> &Operands) {

  if (isOi64())
    return MatchOperand_NoMatch;

  // if the first token is not '$' we have error
  if (Parser.getTok().isNot(AsmToken::Dollar))
    return MatchOperand_NoMatch;
  SMLoc S = Parser.getTok().getLoc();
  Parser.Lex(); // Eat $

  const AsmToken &Tok = Parser.getTok(); // get next token
  if (Tok.isNot(AsmToken::Integer))
    return MatchOperand_NoMatch;

  unsigned RegNum = Tok.getIntVal();
  // at the moment only hwreg29 is supported
  if (RegNum != 29)
    return MatchOperand_ParseFail;

  OiOperand *op = OiOperand::CreateReg(Oi::HWR29, S,
        Parser.getTok().getLoc());
  op->setRegKind(OiOperand::Kind_HWRegs);
  Operands.push_back(op);

  Parser.Lex(); // Eat reg number
  return MatchOperand_Success;
}

OiAsmParser::OperandMatchResultTy
OiAsmParser::parseHW64Regs(SmallVectorImpl<MCParsedAsmOperand*> &Operands) {

  if (!isOi64())
    return MatchOperand_NoMatch;
    //if the first token is not '$' we have error
  if (Parser.getTok().isNot(AsmToken::Dollar))
    return MatchOperand_NoMatch;
  SMLoc S = Parser.getTok().getLoc();
  Parser.Lex(); // Eat $

  const AsmToken &Tok = Parser.getTok(); // get next token
  if (Tok.isNot(AsmToken::Integer))
    return MatchOperand_NoMatch;

  unsigned RegNum = Tok.getIntVal();
  // at the moment only hwreg29 is supported
  if (RegNum != 29)
    return MatchOperand_ParseFail;

  OiOperand *op = OiOperand::CreateReg(Oi::HWR29_64, S,
        Parser.getTok().getLoc());
  op->setRegKind(OiOperand::Kind_HW64Regs);
  Operands.push_back(op);

  Parser.Lex(); // Eat reg number
  return MatchOperand_Success;
}

OiAsmParser::OperandMatchResultTy
OiAsmParser::parseCCRRegs(SmallVectorImpl<MCParsedAsmOperand*> &Operands) {
  unsigned RegNum;
  //if the first token is not '$' we have error
  if (Parser.getTok().isNot(AsmToken::Dollar))
    return MatchOperand_NoMatch;
  SMLoc S = Parser.getTok().getLoc();
  Parser.Lex(); // Eat $

  const AsmToken &Tok = Parser.getTok(); // get next token
  if (Tok.is(AsmToken::Integer)) {
    RegNum = Tok.getIntVal();
    // at the moment only fcc0 is supported
    if (RegNum != 0)
      return MatchOperand_ParseFail;
  } else if (Tok.is(AsmToken::Identifier)) {
    // at the moment only fcc0 is supported
    if (Tok.getIdentifier() != "fcc0")
      return MatchOperand_ParseFail;
  } else
    return MatchOperand_NoMatch;

  OiOperand *op = OiOperand::CreateReg(Oi::FCC0, S,
        Parser.getTok().getLoc());
  op->setRegKind(OiOperand::Kind_CCRRegs);
  Operands.push_back(op);

  Parser.Lex(); // Eat reg number
  return MatchOperand_Success;
}

MCSymbolRefExpr::VariantKind OiAsmParser::getVariantKind(StringRef Symbol) {

  MCSymbolRefExpr::VariantKind VK
                   = StringSwitch<MCSymbolRefExpr::VariantKind>(Symbol)
    .Case("hi",          MCSymbolRefExpr::VK_Mips_ABS_HI)
    .Case("lo",          MCSymbolRefExpr::VK_Mips_ABS_LO)
    .Case("gp_rel",      MCSymbolRefExpr::VK_Mips_GPREL)
    .Case("call16",      MCSymbolRefExpr::VK_Mips_GOT_CALL)
    .Case("got",         MCSymbolRefExpr::VK_Mips_GOT)
    .Case("tlsgd",       MCSymbolRefExpr::VK_Mips_TLSGD)
    .Case("tlsldm",      MCSymbolRefExpr::VK_Mips_TLSLDM)
    .Case("dtprel_hi",   MCSymbolRefExpr::VK_Mips_DTPREL_HI)
    .Case("dtprel_lo",   MCSymbolRefExpr::VK_Mips_DTPREL_LO)
    .Case("gottprel",    MCSymbolRefExpr::VK_Mips_GOTTPREL)
    .Case("tprel_hi",    MCSymbolRefExpr::VK_Mips_TPREL_HI)
    .Case("tprel_lo",    MCSymbolRefExpr::VK_Mips_TPREL_LO)
    .Case("got_disp",    MCSymbolRefExpr::VK_Mips_GOT_DISP)
    .Case("got_page",    MCSymbolRefExpr::VK_Mips_GOT_PAGE)
    .Case("got_ofst",    MCSymbolRefExpr::VK_Mips_GOT_OFST)
    .Case("hi(%neg(%gp_rel",    MCSymbolRefExpr::VK_Mips_GPOFF_HI)
    .Case("lo(%neg(%gp_rel",    MCSymbolRefExpr::VK_Mips_GPOFF_LO)
    .Default(MCSymbolRefExpr::VK_None);

  return VK;
}

static int ConvertCcString(StringRef CondString) {
  int CC = StringSwitch<unsigned>(CondString)
      .Case(".f",    0)
      .Case(".un",   1)
      .Case(".eq",   2)
      .Case(".ueq",  3)
      .Case(".olt",  4)
      .Case(".ult",  5)
      .Case(".ole",  6)
      .Case(".ule",  7)
      .Case(".sf",   8)
      .Case(".ngle", 9)
      .Case(".seq",  10)
      .Case(".ngl",  11)
      .Case(".lt",   12)
      .Case(".nge",  13)
      .Case(".le",   14)
      .Case(".ngt",  15)
      .Default(-1);

  return CC;
}

bool OiAsmParser::
parseMathOperation(StringRef Name, SMLoc NameLoc,
                   SmallVectorImpl<MCParsedAsmOperand*> &Operands) {
  // split the format
  size_t Start = Name.find('.'), Next = Name.rfind('.');
  StringRef Format1 = Name.slice(Start, Next);
  // and add the first format to the operands
  Operands.push_back(OiOperand::CreateToken(Format1, NameLoc));
  // now for the second format
  StringRef Format2 = Name.slice(Next, StringRef::npos);
  Operands.push_back(OiOperand::CreateToken(Format2, NameLoc));

  // set the format for the first register
  setFpFormat(Format1);

  // Read the remaining operands.
  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    // Read the first operand.
    if (ParseOperand(Operands, Name)) {
      SMLoc Loc = getLexer().getLoc();
      Parser.EatToEndOfStatement();
      return Error(Loc, "unexpected token in argument list");
    }

    if (getLexer().isNot(AsmToken::Comma)) {
      SMLoc Loc = getLexer().getLoc();
      Parser.EatToEndOfStatement();
      return Error(Loc, "unexpected token in argument list");

    }
    Parser.Lex();  // Eat the comma.

    //set the format for the first register
    setFpFormat(Format2);

    // Parse and remember the operand.
    if (ParseOperand(Operands, Name)) {
      SMLoc Loc = getLexer().getLoc();
      Parser.EatToEndOfStatement();
      return Error(Loc, "unexpected token in argument list");
    }
  }

  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    SMLoc Loc = getLexer().getLoc();
    Parser.EatToEndOfStatement();
    return Error(Loc, "unexpected token in argument list");
  }

  Parser.Lex(); // Consume the EndOfStatement
  return false;
}

bool OiAsmParser::
ParseInstruction(ParseInstructionInfo &Info, StringRef Name, SMLoc NameLoc,
                 SmallVectorImpl<MCParsedAsmOperand*> &Operands) {
  // floating point instructions: should register be treated as double?
  if (requestsDoubleOperand(Name)) {
    setFpFormat(FP_FORMAT_D);
  Operands.push_back(OiOperand::CreateToken(Name, NameLoc));
  }
  else {
    setDefaultFpFormat();
    // Create the leading tokens for the mnemonic, split by '.' characters.
    size_t Start = 0, Next = Name.find('.');
    StringRef Mnemonic = Name.slice(Start, Next);

    Operands.push_back(OiOperand::CreateToken(Mnemonic, NameLoc));

    if (Next != StringRef::npos) {
      // there is a format token in mnemonic
      // StringRef Rest = Name.slice(Next, StringRef::npos);
      size_t Dot = Name.find('.', Next+1);
      StringRef Format = Name.slice(Next, Dot);
      if (Dot == StringRef::npos) //only one '.' in a string, it's a format
        Operands.push_back(OiOperand::CreateToken(Format, NameLoc));
      else {
        if (Name.startswith("c.")){
          // floating point compare, add '.' and immediate represent for cc
          Operands.push_back(OiOperand::CreateToken(".", NameLoc));
          int Cc = ConvertCcString(Format);
          if (Cc == -1) {
            return Error(NameLoc, "Invalid conditional code");
          }
          SMLoc E = SMLoc::getFromPointer(
              Parser.getTok().getLoc().getPointer() -1 );
          Operands.push_back(OiOperand::CreateImm(
              MCConstantExpr::Create(Cc, getContext()), NameLoc, E));
        } else {
          // trunc, ceil, floor ...
          return parseMathOperation(Name, NameLoc, Operands);
        }

        // the rest is a format
        Format = Name.slice(Dot, StringRef::npos);
        Operands.push_back(OiOperand::CreateToken(Format, NameLoc));
      }

      setFpFormat(Format);
    }
  }

  // Read the remaining operands.
  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    // Read the first operand.
    if (ParseOperand(Operands, Name)) {
      SMLoc Loc = getLexer().getLoc();
      Parser.EatToEndOfStatement();
      return Error(Loc, "unexpected token in argument list");
    }

    while (getLexer().is(AsmToken::Comma) ) {
      Parser.Lex();  // Eat the comma.

      // Parse and remember the operand.
      if (ParseOperand(Operands, Name)) {
        SMLoc Loc = getLexer().getLoc();
        Parser.EatToEndOfStatement();
        return Error(Loc, "unexpected token in argument list");
      }
    }
  }

  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    SMLoc Loc = getLexer().getLoc();
    Parser.EatToEndOfStatement();
    return Error(Loc, "unexpected token in argument list");
  }

  Parser.Lex(); // Consume the EndOfStatement
  return false;
}

bool OiAsmParser::reportParseError(StringRef ErrorMsg) {
   SMLoc Loc = getLexer().getLoc();
   Parser.EatToEndOfStatement();
   return Error(Loc, ErrorMsg);
}

bool OiAsmParser::parseSetNoAtDirective() {
  // line should look like:
  //  .set noat
  // set at reg to 0
  Options.setATReg(0);
  // eat noat
  Parser.Lex();
  // if this is not the end of the statement, report error
  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    reportParseError("unexpected token in statement");
    return false;
  }
  Parser.Lex(); // Consume the EndOfStatement
  return false;
}
bool OiAsmParser::parseSetAtDirective() {
  // line can be
  //  .set at - defaults to $1
  // or .set at=$reg
  getParser().Lex();
  if (getLexer().is(AsmToken::EndOfStatement)) {
    Options.setATReg(1);
    Parser.Lex(); // Consume the EndOfStatement
    return false;
  } else if (getLexer().is(AsmToken::Equal)) {
    getParser().Lex(); //eat '='
    if (getLexer().isNot(AsmToken::Dollar)) {
      reportParseError("unexpected token in statement");
      return false;
    }
    Parser.Lex(); // eat '$'
    if (getLexer().isNot(AsmToken::Integer)) {
      reportParseError("unexpected token in statement");
      return false;
    }
    const AsmToken &Reg = Parser.getTok();
    if (!Options.setATReg(Reg.getIntVal())) {
      reportParseError("unexpected token in statement");
      return false;
    }
    getParser().Lex(); //eat reg

    if (getLexer().isNot(AsmToken::EndOfStatement)) {
      reportParseError("unexpected token in statement");
      return false;
     }
    Parser.Lex(); // Consume the EndOfStatement
    return false;
  } else {
    reportParseError("unexpected token in statement");
    return false;
  }
}

bool OiAsmParser::parseSetReorderDirective() {
  Parser.Lex();
  // if this is not the end of the statement, report error
  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    reportParseError("unexpected token in statement");
    return false;
  }
  Options.setReorder();
  Parser.Lex(); // Consume the EndOfStatement
  return false;
}

bool OiAsmParser::parseSetNoReorderDirective() {
    Parser.Lex();
    // if this is not the end of the statement, report error
    if (getLexer().isNot(AsmToken::EndOfStatement)) {
      reportParseError("unexpected token in statement");
      return false;
    }
    Options.setNoreorder();
    Parser.Lex(); // Consume the EndOfStatement
    return false;
}

bool OiAsmParser::parseSetMacroDirective() {
  Parser.Lex();
  // if this is not the end of the statement, report error
  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    reportParseError("unexpected token in statement");
    return false;
  }
  Options.setMacro();
  Parser.Lex(); // Consume the EndOfStatement
  return false;
}

bool OiAsmParser::parseSetNoMacroDirective() {
  Parser.Lex();
  // if this is not the end of the statement, report error
  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    reportParseError("`noreorder' must be set before `nomacro'");
    return false;
  }
  if (Options.isReorder()) {
    reportParseError("`noreorder' must be set before `nomacro'");
    return false;
  }
  Options.setNomacro();
  Parser.Lex(); // Consume the EndOfStatement
  return false;
}
bool OiAsmParser::parseDirectiveSet() {

  // get next token
  const AsmToken &Tok = Parser.getTok();

  if (Tok.getString() == "noat") {
    return parseSetNoAtDirective();
  } else if (Tok.getString() == "at") {
    return parseSetAtDirective();
  } else if (Tok.getString() == "reorder") {
    return parseSetReorderDirective();
  } else if (Tok.getString() == "noreorder") {
    return parseSetNoReorderDirective();
  } else if (Tok.getString() == "macro") {
    return parseSetMacroDirective();
  } else if (Tok.getString() == "nomacro") {
    return parseSetNoMacroDirective();
  } else if (Tok.getString() == "nooi16") {
    // ignore this directive for now
    Parser.EatToEndOfStatement();
    return false;
  } else if (Tok.getString() == "nomicrooi") {
    // ignore this directive for now
    Parser.EatToEndOfStatement();
    return false;
  }

  return true;
}

/// parseDirectiveWord
///  ::= .word [ expression (, expression)* ]
bool OiAsmParser::parseDirectiveWord(unsigned Size, SMLoc L) {
  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    for (;;) {
      const MCExpr *Value;
      if (getParser().ParseExpression(Value))
        return true;

      getParser().getStreamer().EmitValue(Value, Size);

      if (getLexer().is(AsmToken::EndOfStatement))
        break;

      // FIXME: Improve diagnostic.
      if (getLexer().isNot(AsmToken::Comma))
        return Error(L, "unexpected token in directive");
      Parser.Lex();
    }
  }

  Parser.Lex();
  return false;
}

bool OiAsmParser::ParseDirective(AsmToken DirectiveID) {

  StringRef IDVal = DirectiveID.getString();

  if ( IDVal == ".ent") {
    // ignore this directive for now
    Parser.Lex();
    return false;
  }

  if (IDVal == ".end") {
    // ignore this directive for now
    Parser.Lex();
    return false;
  }

  if (IDVal == ".frame") {
    // ignore this directive for now
    Parser.EatToEndOfStatement();
    return false;
  }

  if (IDVal == ".set") {
    return parseDirectiveSet();
  }

  if (IDVal == ".fmask") {
    // ignore this directive for now
    Parser.EatToEndOfStatement();
    return false;
  }

  if (IDVal == ".mask") {
    // ignore this directive for now
    Parser.EatToEndOfStatement();
    return false;
  }

  if (IDVal == ".gpword") {
    // ignore this directive for now
    Parser.EatToEndOfStatement();
    return false;
  }

  if (IDVal == ".word") {
    parseDirectiveWord(4, DirectiveID.getLoc());
    return false;
  }

  return true;
}

extern "C" void LLVMInitializeOiAsmParser() {
  RegisterMCAsmParser<OiAsmParser> X(TheOiTarget);
  RegisterMCAsmParser<OiAsmParser> Y(TheOielTarget);
  RegisterMCAsmParser<OiAsmParser> A(TheOi64Target);
  RegisterMCAsmParser<OiAsmParser> B(TheOi64elTarget);
}

#define GET_REGISTER_MATCHER
#define GET_MATCHER_IMPLEMENTATION
#include "OiGenAsmMatcher.inc"
