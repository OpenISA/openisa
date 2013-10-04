//===-- Oi16ISelDAGToDAG.cpp - A Dag to Dag Inst Selector for Oi16 ----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Subclass of OiDAGToDAGISel specialized for oi16.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "oi-isel"
#include "Oi16ISelDAGToDAG.h"
#include "Oi.h"
#include "MCTargetDesc/OiBaseInfo.h"
#include "OiAnalyzeImmediate.h"
#include "OiMachineFunction.h"
#include "OiRegisterInfo.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
using namespace llvm;

bool Oi16DAGToDAGISel::runOnMachineFunction(MachineFunction &MF) {
  if (!Subtarget.inOi16Mode())
    return false;
  return OiDAGToDAGISel::runOnMachineFunction(MF);
}
/// Select multiply instructions.
std::pair<SDNode*, SDNode*>
Oi16DAGToDAGISel::selectMULT(SDNode *N, unsigned Opc, DebugLoc DL, EVT Ty,
                               bool HasLo, bool HasHi) {
  SDNode *Lo = 0, *Hi = 0;
  SDNode *Mul = CurDAG->getMachineNode(Opc, DL, MVT::Glue, N->getOperand(0),
                                       N->getOperand(1));
  SDValue InFlag = SDValue(Mul, 0);

  if (HasLo) {
    unsigned Opcode = Oi::Mflo16;
    Lo = CurDAG->getMachineNode(Opcode, DL, Ty, MVT::Glue, InFlag);
    InFlag = SDValue(Lo, 1);
  }
  if (HasHi) {
    unsigned Opcode = Oi::Mfhi16;
    Hi = CurDAG->getMachineNode(Opcode, DL, Ty, InFlag);
  }
  return std::make_pair(Lo, Hi);
}

void Oi16DAGToDAGISel::initGlobalBaseReg(MachineFunction &MF) {
  OiFunctionInfo *OiFI = MF.getInfo<OiFunctionInfo>();

  if (!OiFI->globalBaseRegSet())
    return;

  MachineBasicBlock &MBB = MF.front();
  MachineBasicBlock::iterator I = MBB.begin();
  MachineRegisterInfo &RegInfo = MF.getRegInfo();
  const TargetInstrInfo &TII = *MF.getTarget().getInstrInfo();
  DebugLoc DL = I != MBB.end() ? I->getDebugLoc() : DebugLoc();
  unsigned V0, V1, V2, GlobalBaseReg = OiFI->getGlobalBaseReg();
  const TargetRegisterClass *RC =
    (const TargetRegisterClass*)&Oi::CPU16RegsRegClass;

  V0 = RegInfo.createVirtualRegister(RC);
  V1 = RegInfo.createVirtualRegister(RC);
  V2 = RegInfo.createVirtualRegister(RC);

  BuildMI(MBB, I, DL, TII.get(Oi::LiRxImmX16), V0)
    .addExternalSymbol("_gp_disp", OiII::MO_ABS_HI);
  BuildMI(MBB, I, DL, TII.get(Oi::AddiuRxPcImmX16), V1)
    .addExternalSymbol("_gp_disp", OiII::MO_ABS_LO);
  BuildMI(MBB, I, DL, TII.get(Oi::SllX16), V2).addReg(V0).addImm(16);
  BuildMI(MBB, I, DL, TII.get(Oi::AdduRxRyRz16), GlobalBaseReg)
    .addReg(V1).addReg(V2);
}

// Insert instructions to initialize the Oi16 SP Alias register in the
// first MBB of the function.
//
void Oi16DAGToDAGISel::initOi16SPAliasReg(MachineFunction &MF) {
  OiFunctionInfo *OiFI = MF.getInfo<OiFunctionInfo>();

  if (!OiFI->oi16SPAliasRegSet())
    return;

  MachineBasicBlock &MBB = MF.front();
  MachineBasicBlock::iterator I = MBB.begin();
  const TargetInstrInfo &TII = *MF.getTarget().getInstrInfo();
  DebugLoc DL = I != MBB.end() ? I->getDebugLoc() : DebugLoc();
  unsigned Oi16SPAliasReg = OiFI->getOi16SPAliasReg();

  BuildMI(MBB, I, DL, TII.get(Oi::MoveR3216), Oi16SPAliasReg)
    .addReg(Oi::SP);
}

void Oi16DAGToDAGISel::processFunctionAfterISel(MachineFunction &MF) {
  initGlobalBaseReg(MF);
  initOi16SPAliasReg(MF);
}

/// getOi16SPAliasReg - Output the instructions required to put the
/// SP into a Oi16 accessible aliased register.
SDValue Oi16DAGToDAGISel::getOi16SPAliasReg() {
  unsigned Oi16SPAliasReg =
    MF->getInfo<OiFunctionInfo>()->getOi16SPAliasReg();
  return CurDAG->getRegister(Oi16SPAliasReg, TLI.getPointerTy());
}

void Oi16DAGToDAGISel::getOi16SPRefReg(SDNode *Parent, SDValue &AliasReg) {
  SDValue AliasFPReg = CurDAG->getRegister(Oi::S0, TLI.getPointerTy());
  if (Parent) {
    switch (Parent->getOpcode()) {
      case ISD::LOAD: {
        LoadSDNode *SD = dyn_cast<LoadSDNode>(Parent);
        switch (SD->getMemoryVT().getSizeInBits()) {
        case 8:
        case 16:
          AliasReg = TM.getFrameLowering()->hasFP(*MF)?
            AliasFPReg: getOi16SPAliasReg();
          return;
        }
        break;
      }
      case ISD::STORE: {
        StoreSDNode *SD = dyn_cast<StoreSDNode>(Parent);
        switch (SD->getMemoryVT().getSizeInBits()) {
        case 8:
        case 16:
          AliasReg = TM.getFrameLowering()->hasFP(*MF)?
            AliasFPReg: getOi16SPAliasReg();
          return;
        }
        break;
      }
    }
  }
  AliasReg = CurDAG->getRegister(Oi::SP, TLI.getPointerTy());
  return;

}

bool Oi16DAGToDAGISel::selectAddr16(
  SDNode *Parent, SDValue Addr, SDValue &Base, SDValue &Offset,
  SDValue &Alias) {
  EVT ValTy = Addr.getValueType();

  Alias = CurDAG->getTargetConstant(0, ValTy);

  // if Address is FI, get the TargetFrameIndex.
  if (FrameIndexSDNode *FIN = dyn_cast<FrameIndexSDNode>(Addr)) {
    Base   = CurDAG->getTargetFrameIndex(FIN->getIndex(), ValTy);
    Offset = CurDAG->getTargetConstant(0, ValTy);
    getOi16SPRefReg(Parent, Alias);
    return true;
  }
  // on PIC code Load GA
  if (Addr.getOpcode() == OiISD::Wrapper) {
    Base   = Addr.getOperand(0);
    Offset = Addr.getOperand(1);
    return true;
  }
  if (TM.getRelocationModel() != Reloc::PIC_) {
    if ((Addr.getOpcode() == ISD::TargetExternalSymbol ||
        Addr.getOpcode() == ISD::TargetGlobalAddress))
      return false;
  }
  // Addresses of the form FI+const or FI|const
  if (CurDAG->isBaseWithConstantOffset(Addr)) {
    ConstantSDNode *CN = dyn_cast<ConstantSDNode>(Addr.getOperand(1));
    if (isInt<16>(CN->getSExtValue())) {

      // If the first operand is a FI, get the TargetFI Node
      if (FrameIndexSDNode *FIN = dyn_cast<FrameIndexSDNode>
                                  (Addr.getOperand(0))) {
        Base = CurDAG->getTargetFrameIndex(FIN->getIndex(), ValTy);
        getOi16SPRefReg(Parent, Alias);
      }
      else
        Base = Addr.getOperand(0);

      Offset = CurDAG->getTargetConstant(CN->getZExtValue(), ValTy);
      return true;
    }
  }
  // Operand is a result from an ADD.
  if (Addr.getOpcode() == ISD::ADD) {
    // When loading from constant pools, load the lower address part in
    // the instruction itself. Example, instead of:
    //  lui $2, %hi($CPI1_0)
    //  addiu $2, $2, %lo($CPI1_0)
    //  lwc1 $f0, 0($2)
    // Generate:
    //  lui $2, %hi($CPI1_0)
    //  lwc1 $f0, %lo($CPI1_0)($2)
    if (Addr.getOperand(1).getOpcode() == OiISD::Lo ||
        Addr.getOperand(1).getOpcode() == OiISD::GPRel) {
      SDValue Opnd0 = Addr.getOperand(1).getOperand(0);
      if (isa<ConstantPoolSDNode>(Opnd0) || isa<GlobalAddressSDNode>(Opnd0) ||
          isa<JumpTableSDNode>(Opnd0)) {
        Base = Addr.getOperand(0);
        Offset = Opnd0;
        return true;
      }
    }

    // If an indexed floating point load/store can be emitted, return false.
    const LSBaseSDNode *LS = dyn_cast<LSBaseSDNode>(Parent);

    if (LS &&
        (LS->getMemoryVT() == MVT::f32 || LS->getMemoryVT() == MVT::f64) &&
        Subtarget.hasFPIdx())
      return false;
  }
  Base   = Addr;
  Offset = CurDAG->getTargetConstant(0, ValTy);
  return true;
}

/// Select instructions not customized! Used for
/// expanded, promoted and normal instructions
std::pair<bool, SDNode*> Oi16DAGToDAGISel::selectNode(SDNode *Node) {
  unsigned Opcode = Node->getOpcode();
  DebugLoc DL = Node->getDebugLoc();

  ///
  // Instruction Selection not handled by the auto-generated
  // tablegen selection should be handled here.
  ///
  EVT NodeTy = Node->getValueType(0);
  unsigned MultOpc;

  switch(Opcode) {
  default: break;

  case ISD::SUBE:
  case ISD::ADDE: {
    SDValue InFlag = Node->getOperand(2), CmpLHS;
    unsigned Opc = InFlag.getOpcode(); (void)Opc;
    assert(((Opc == ISD::ADDC || Opc == ISD::ADDE) ||
            (Opc == ISD::SUBC || Opc == ISD::SUBE)) &&
           "(ADD|SUB)E flag operand must come from (ADD|SUB)C/E insn");

    unsigned MOp;
    if (Opcode == ISD::ADDE) {
      CmpLHS = InFlag.getValue(0);
      MOp = Oi::AdduRxRyRz16;
    } else {
      CmpLHS = InFlag.getOperand(0);
      MOp = Oi::SubuRxRyRz16;
    }

    SDValue Ops[] = { CmpLHS, InFlag.getOperand(1) };

    SDValue LHS = Node->getOperand(0);
    SDValue RHS = Node->getOperand(1);

    EVT VT = LHS.getValueType();

    unsigned Sltu_op = Oi::SltuRxRyRz16;
    SDNode *Carry = CurDAG->getMachineNode(Sltu_op, DL, VT, Ops);
    unsigned Addu_op = Oi::AdduRxRyRz16;
    SDNode *AddCarry = CurDAG->getMachineNode(Addu_op, DL, VT,
                                              SDValue(Carry,0), RHS);

    SDNode *Result = CurDAG->SelectNodeTo(Node, MOp, VT, MVT::Glue, LHS,
                                          SDValue(AddCarry,0));
    return std::make_pair(true, Result);
  }

  /// Mul with two results
  case ISD::SMUL_LOHI:
  case ISD::UMUL_LOHI: {
    MultOpc = (Opcode == ISD::UMUL_LOHI ? Oi::MultuRxRy16 : Oi::MultRxRy16);
    std::pair<SDNode*, SDNode*> LoHi = selectMULT(Node, MultOpc, DL, NodeTy,
                                                  true, true);
    if (!SDValue(Node, 0).use_empty())
      ReplaceUses(SDValue(Node, 0), SDValue(LoHi.first, 0));

    if (!SDValue(Node, 1).use_empty())
      ReplaceUses(SDValue(Node, 1), SDValue(LoHi.second, 0));

    return std::make_pair(true, (SDNode*)NULL);
  }

  case ISD::MULHS:
  case ISD::MULHU: {
    MultOpc = (Opcode == ISD::MULHU ? Oi::MultuRxRy16 : Oi::MultRxRy16);
    SDNode *Result = selectMULT(Node, MultOpc, DL, NodeTy, false, true).second;
    return std::make_pair(true, Result);
  }
  }

  return std::make_pair(false, (SDNode*)NULL);
}

FunctionPass *llvm::createOi16ISelDag(OiTargetMachine &TM) {
  return new Oi16DAGToDAGISel(TM);
}
