//===-- LoongArchISelDAGToDAG.cpp - A Dag to Dag Inst Selector for LoongArch --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines an instruction selector for the LoongArch target.
//
//===----------------------------------------------------------------------===//

#include "LoongArchISelDAGToDAG.h"
#include "LoongArch.h"
#include "LoongArchMachineFunction.h"
#include "LoongArchRegisterInfo.h"
#include "MCTargetDesc/LoongArchAnalyzeImmediate.h"
#include "MCTargetDesc/LoongArchBaseInfo.h"
#include "MCTargetDesc/LoongArchMCTargetDesc.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
using namespace llvm;

#define DEBUG_TYPE "loongarch-isel"

//===----------------------------------------------------------------------===//
// Instruction Selector Implementation
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// LoongArchDAGToDAGISel - LoongArch specific code to select LoongArch machine
// instructions for SelectionDAG operations.
//===----------------------------------------------------------------------===//

void LoongArchDAGToDAGISel::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<DominatorTreeWrapperPass>();
  SelectionDAGISel::getAnalysisUsage(AU);
}

bool LoongArchDAGToDAGISel::runOnMachineFunction(MachineFunction &MF) {
  Subtarget = &static_cast<const LoongArchSubtarget &>(MF.getSubtarget());
  bool Ret = SelectionDAGISel::runOnMachineFunction(MF);

  return Ret;
}

/// Match frameindex
bool LoongArchDAGToDAGISel::selectAddrFrameIndex(SDValue Addr, SDValue &Base,
                                              SDValue &Offset) const {
  if (FrameIndexSDNode *FIN = dyn_cast<FrameIndexSDNode>(Addr)) {
    EVT ValTy = Addr.getValueType();

    Base   = CurDAG->getTargetFrameIndex(FIN->getIndex(), ValTy);
    Offset = CurDAG->getTargetConstant(0, SDLoc(Addr), ValTy);
    return true;
  }
  return false;
}

/// Match frameindex+offset and frameindex|offset
bool LoongArchDAGToDAGISel::selectAddrFrameIndexOffset(
    SDValue Addr, SDValue &Base, SDValue &Offset, unsigned OffsetBits,
    unsigned ShiftAmount = 0) const {
  if (CurDAG->isBaseWithConstantOffset(Addr)) {
    ConstantSDNode *CN = dyn_cast<ConstantSDNode>(Addr.getOperand(1));
    if (isIntN(OffsetBits + ShiftAmount, CN->getSExtValue())) {
      EVT ValTy = Addr.getValueType();

      // If the first operand is a FI, get the TargetFI Node
      if (FrameIndexSDNode *FIN =
              dyn_cast<FrameIndexSDNode>(Addr.getOperand(0)))
        Base = CurDAG->getTargetFrameIndex(FIN->getIndex(), ValTy);
      else {
        Base = Addr.getOperand(0);
        // If base is a FI, additional offset calculation is done in
        // eliminateFrameIndex, otherwise we need to check the alignment
        if (OffsetToAlignment(CN->getZExtValue(), 1ull << ShiftAmount) != 0)
          return false;
      }

      Offset = CurDAG->getTargetConstant(CN->getZExtValue(), SDLoc(Addr),
                                         ValTy);
      return true;
    }
  }
  return false;
}

/// ComplexPattern used on LoongArchInstrInfo
/// Used on LoongArch Load/Store instructions
bool LoongArchDAGToDAGISel::selectAddrRegImm(SDValue Addr, SDValue &Base,
                                        SDValue &Offset) const {
  // if Address is FI, get the TargetFrameIndex.
  if (selectAddrFrameIndex(Addr, Base, Offset))
    return true;

  if (!TM.isPositionIndependent()) {
    if ((Addr.getOpcode() == ISD::TargetExternalSymbol ||
        Addr.getOpcode() == ISD::TargetGlobalAddress))
      return false;
  }

  // Addresses of the form FI+const or FI|const
  if (selectAddrFrameIndexOffset(Addr, Base, Offset, 12))
    return true;

  return false;
}

/// ComplexPattern used on LoongArchInstrInfo
/// Used on LoongArch Load/Store instructions
bool LoongArchDAGToDAGISel::selectAddrDefault(SDValue Addr, SDValue &Base,
                                         SDValue &Offset) const {
  Base = Addr;
  Offset = CurDAG->getTargetConstant(0, SDLoc(Addr), Addr.getValueType());
  return true;
}

bool LoongArchDAGToDAGISel::selectIntAddr(SDValue Addr, SDValue &Base,
                                     SDValue &Offset) const {
  return selectAddrRegImm(Addr, Base, Offset) ||
    selectAddrDefault(Addr, Base, Offset);
}

bool LoongArchDAGToDAGISel::selectAddrRegImm12(SDValue Addr, SDValue &Base,
                                            SDValue &Offset) const {
  if (selectAddrFrameIndex(Addr, Base, Offset))
    return true;

  if (selectAddrFrameIndexOffset(Addr, Base, Offset, 12))
    return true;

  return false;
}

bool LoongArchDAGToDAGISel::selectIntAddrSImm12(SDValue Addr, SDValue &Base,
                                           SDValue &Offset) const {
  if (selectAddrFrameIndex(Addr, Base, Offset))
    return true;

  if (selectAddrFrameIndexOffset(Addr, Base, Offset, 12))
    return true;

  return selectAddrDefault(Addr, Base, Offset);
}

bool LoongArchDAGToDAGISel::selectIntAddrSImm10Lsl1(SDValue Addr, SDValue &Base,
                                               SDValue &Offset) const {
  if (selectAddrFrameIndex(Addr, Base, Offset))
    return true;

  if (selectAddrFrameIndexOffset(Addr, Base, Offset, 10, 1))
    return true;

  return selectAddrDefault(Addr, Base, Offset);
}

bool LoongArchDAGToDAGISel::selectIntAddrSImm10(SDValue Addr, SDValue &Base,
                                               SDValue &Offset) const {
  if (selectAddrFrameIndex(Addr, Base, Offset))
    return true;

  if (selectAddrFrameIndexOffset(Addr, Base, Offset, 10))
    return true;

  return selectAddrDefault(Addr, Base, Offset);
}

bool LoongArchDAGToDAGISel::selectIntAddrSImm10Lsl2(SDValue Addr, SDValue &Base,
                                               SDValue &Offset) const {
  if (selectAddrFrameIndex(Addr, Base, Offset))
    return true;

  if (selectAddrFrameIndexOffset(Addr, Base, Offset, 10, 2))
    return true;

  return selectAddrDefault(Addr, Base, Offset);
}

bool LoongArchDAGToDAGISel::selectIntAddrSImm11Lsl1(SDValue Addr, SDValue &Base,
                                               SDValue &Offset) const {
  if (selectAddrFrameIndex(Addr, Base, Offset))
    return true;

  if (selectAddrFrameIndexOffset(Addr, Base, Offset, 11, 1))
    return true;

  return selectAddrDefault(Addr, Base, Offset);
}

bool LoongArchDAGToDAGISel::selectIntAddrSImm9Lsl3(SDValue Addr, SDValue &Base,
                                               SDValue &Offset) const {
  if (selectAddrFrameIndex(Addr, Base, Offset))
    return true;

  if (selectAddrFrameIndexOffset(Addr, Base, Offset, 9, 3))
    return true;

  return selectAddrDefault(Addr, Base, Offset);
}

bool LoongArchDAGToDAGISel::selectIntAddrSImm14Lsl2(SDValue Addr, SDValue &Base,
                                               SDValue &Offset) const {
  if (selectAddrFrameIndex(Addr, Base, Offset))
    return true;

  if (selectAddrFrameIndexOffset(Addr, Base, Offset, 14, 2))
    return true;

  return false;
}

bool LoongArchDAGToDAGISel::selectIntAddrSImm10Lsl3(SDValue Addr, SDValue &Base,
                                               SDValue &Offset) const {
  if (selectAddrFrameIndex(Addr, Base, Offset))
    return true;

  if (selectAddrFrameIndexOffset(Addr, Base, Offset, 10, 3))
    return true;

  return selectAddrDefault(Addr, Base, Offset);
}

// Select constant vector splats.
//
// Returns true and sets Imm if:
// * LSX is enabled
// * N is a ISD::BUILD_VECTOR representing a constant splat
bool LoongArchDAGToDAGISel::selectVSplat(SDNode *N, APInt &Imm,
                                         unsigned MinSizeInBits) const {
  if (!(Subtarget->hasLSX()|| Subtarget->hasLASX()))
    return false;

  BuildVectorSDNode *Node = dyn_cast<BuildVectorSDNode>(N);

  if (!Node)
    return false;

  APInt SplatValue, SplatUndef;
  unsigned SplatBitSize;
  bool HasAnyUndefs;

  if (!Node->isConstantSplat(SplatValue, SplatUndef, SplatBitSize, HasAnyUndefs,
                             MinSizeInBits))
    return false;

  Imm = SplatValue;

  return true;
}

// Select constant vector splats.
//
// In addition to the requirements of selectVSplat(), this function returns
// true and sets Imm if:
// * The splat value is the same width as the elements of the vector
// * The splat value fits in an integer with the specified signed-ness and
//   width.
//
// This function looks through ISD::BITCAST nodes.
// TODO: This might not be appropriate for big-endian LSX since BITCAST is
//       sometimes a shuffle in big-endian mode.
//
// It's worth noting that this function is not used as part of the selection
// of [v/xv]ldi.[bhwd] since it does not permit using the wrong-typed [v/xv]ldi.[bhwd]
// instruction to achieve the desired bit pattern. [v/xv]ldi.[bhwd] is selected in
// LoongArchDAGToDAGISel::selectNode.
bool LoongArchDAGToDAGISel::
selectVSplatCommon(SDValue N, SDValue &Imm, bool Signed,
                   unsigned ImmBitSize) const {
  APInt ImmValue;
  EVT EltTy = N->getValueType(0).getVectorElementType();

  if (N->getOpcode() == ISD::BITCAST)
    N = N->getOperand(0);

  if (selectVSplat(N.getNode(), ImmValue, EltTy.getSizeInBits()) &&
      ImmValue.getBitWidth() == EltTy.getSizeInBits()) {

    if (( Signed && ImmValue.isSignedIntN(ImmBitSize)) ||
        (!Signed && ImmValue.isIntN(ImmBitSize))) {
      Imm = CurDAG->getTargetConstant(ImmValue, SDLoc(N), EltTy);
      return true;
    }
  }

  return false;
}

// Select constant vector splats.
bool LoongArchDAGToDAGISel::selectVSplatUimm1(SDValue N, SDValue &Imm) const {
  return selectVSplatCommon(N, Imm, false, 1);
}

bool LoongArchDAGToDAGISel::selectVSplatUimm2(SDValue N, SDValue &Imm) const {
  return selectVSplatCommon(N, Imm, false, 2);
}

bool LoongArchDAGToDAGISel::selectVSplatUimm3(SDValue N, SDValue &Imm) const {
  return selectVSplatCommon(N, Imm, false, 3);
}

bool LoongArchDAGToDAGISel::selectVSplatUimm4(SDValue N, SDValue &Imm) const {
  return selectVSplatCommon(N, Imm, false, 4);
}

bool LoongArchDAGToDAGISel::selectVSplatUimm5(SDValue N, SDValue &Imm) const {
  return selectVSplatCommon(N, Imm, false, 5);
}

bool LoongArchDAGToDAGISel::selectVSplatUimm6(SDValue N, SDValue &Imm) const {
  return selectVSplatCommon(N, Imm, false, 6);
}

bool LoongArchDAGToDAGISel::selectVSplatUimm8(SDValue N, SDValue &Imm) const {
  return selectVSplatCommon(N, Imm, false, 8);
}

bool LoongArchDAGToDAGISel::selectVSplatSimm5(SDValue N, SDValue &Imm) const {
  return selectVSplatCommon(N, Imm, true, 5);
}

// Select constant vector splats whose value is a power of 2.
//
// In addition to the requirements of selectVSplat(), this function returns
// true and sets Imm if:
// * The splat value is the same width as the elements of the vector
// * The splat value is a power of two.
//
// This function looks through ISD::BITCAST nodes.
// TODO: This might not be appropriate for big-endian LSX since BITCAST is
//       sometimes a shuffle in big-endian mode.
bool LoongArchDAGToDAGISel::selectVSplatUimmPow2(SDValue N, SDValue &Imm) const {
  APInt ImmValue;
  EVT EltTy = N->getValueType(0).getVectorElementType();

  if (N->getOpcode() == ISD::BITCAST)
    N = N->getOperand(0);

  if (selectVSplat(N.getNode(), ImmValue, EltTy.getSizeInBits()) &&
      ImmValue.getBitWidth() == EltTy.getSizeInBits()) {
    int32_t Log2 = ImmValue.exactLogBase2();

    if (Log2 != -1) {
      Imm = CurDAG->getTargetConstant(Log2, SDLoc(N), EltTy);
      return true;
    }
  }

  return false;
}

bool LoongArchDAGToDAGISel::selectVSplatUimmInvPow2(SDValue N, SDValue &Imm) const {
  APInt ImmValue;
  EVT EltTy = N->getValueType(0).getVectorElementType();

  if (N->getOpcode() == ISD::BITCAST)
    N = N->getOperand(0);

  if (selectVSplat(N.getNode(), ImmValue, EltTy.getSizeInBits()) &&
      ImmValue.getBitWidth() == EltTy.getSizeInBits()) {
    int32_t Log2 = (~ImmValue).exactLogBase2();

    if (Log2 != -1) {
      Imm = CurDAG->getTargetConstant(Log2, SDLoc(N), EltTy);
      return true;
    }
  }

  return false;
}

// Select constant vector splats whose value only has a consecutive sequence
// of left-most bits set (e.g. 0b11...1100...00).
//
// In addition to the requirements of selectVSplat(), this function returns
// true and sets Imm if:
// * The splat value is the same width as the elements of the vector
// * The splat value is a consecutive sequence of left-most bits.
//
// This function looks through ISD::BITCAST nodes.
// TODO: This might not be appropriate for big-endian LSX since BITCAST is
//       sometimes a shuffle in big-endian mode.
bool LoongArchDAGToDAGISel::selectVSplatMaskL(SDValue N, SDValue &Imm) const {
  APInt ImmValue;
  EVT EltTy = N->getValueType(0).getVectorElementType();

  if (N->getOpcode() == ISD::BITCAST)
    N = N->getOperand(0);

  if (selectVSplat(N.getNode(), ImmValue, EltTy.getSizeInBits()) &&
      ImmValue.getBitWidth() == EltTy.getSizeInBits()) {
    // Extract the run of set bits starting with bit zero from the bitwise
    // inverse of ImmValue, and test that the inverse of this is the same
    // as the original value.
    if (ImmValue == ~(~ImmValue & ~(~ImmValue + 1))) {

      Imm = CurDAG->getTargetConstant(ImmValue.countPopulation() - 1, SDLoc(N),
                                      EltTy);
      return true;
    }
  }

  return false;
}

// Select constant vector splats whose value only has a consecutive sequence
// of right-most bits set (e.g. 0b00...0011...11).
//
// In addition to the requirements of selectVSplat(), this function returns
// true and sets Imm if:
// * The splat value is the same width as the elements of the vector
// * The splat value is a consecutive sequence of right-most bits.
//
// This function looks through ISD::BITCAST nodes.
// TODO: This might not be appropriate for big-endian LSX since BITCAST is
//       sometimes a shuffle in big-endian mode.
bool LoongArchDAGToDAGISel::selectVSplatMaskR(SDValue N, SDValue &Imm) const {
  APInt ImmValue;
  EVT EltTy = N->getValueType(0).getVectorElementType();

  if (N->getOpcode() == ISD::BITCAST)
    N = N->getOperand(0);

  if (selectVSplat(N.getNode(), ImmValue, EltTy.getSizeInBits()) &&
      ImmValue.getBitWidth() == EltTy.getSizeInBits()) {
    // Extract the run of set bits starting with bit zero, and test that the
    // result is the same as the original value
    if (ImmValue == (ImmValue & ~(ImmValue + 1))) {
      Imm = CurDAG->getTargetConstant(ImmValue.countPopulation() - 1, SDLoc(N),
                                      EltTy);
      return true;
    }
  }

  return false;
}

bool LoongArchDAGToDAGISel::trySelect(SDNode *Node) {
  unsigned Opcode = Node->getOpcode();
  SDLoc DL(Node);

  ///
  // Instruction Selection not handled by the auto-generated
  // tablegen selection should be handled here.
  ///
  switch(Opcode) {
  default: break;
  case ISD::ConstantFP: {
    ConstantFPSDNode *CN = dyn_cast<ConstantFPSDNode>(Node);
    if (Node->getValueType(0) == MVT::f64 && CN->isExactlyValue(+0.0)) {
      if (Subtarget->is64Bit()) {
        SDValue Zero = CurDAG->getCopyFromReg(CurDAG->getEntryNode(), DL,
                                              LoongArch::ZERO_64, MVT::i64);
        ReplaceNode(Node,
                    CurDAG->getMachineNode(LoongArch::MOVGR2FR_D, DL, MVT::f64, Zero));
      }
      return true;
    }
    break;
  }

  case ISD::Constant: {
    const ConstantSDNode *CN = dyn_cast<ConstantSDNode>(Node);
    MVT VT = CN->getSimpleValueType(0);
    int64_t Imm = CN->getSExtValue();
    LoongArchAnalyzeImmediate::InstSeq Seq =
        LoongArchAnalyzeImmediate::generateInstSeq(Imm, VT == MVT::i64);
    SDLoc DL(CN);
    SDNode *Result = nullptr;
    SDValue SrcReg = CurDAG->getRegister(
        VT == MVT::i64 ? LoongArch::ZERO_64 : LoongArch::ZERO, VT);

    // The instructions in the sequence are handled here.
    for (LoongArchAnalyzeImmediate::Inst &Inst : Seq) {
      SDValue SDImm = CurDAG->getTargetConstant(Inst.Imm, DL, VT);
      if (Inst.Opc == LoongArch::LU12I_W || Inst.Opc == LoongArch::LU12I_W32)
        Result = CurDAG->getMachineNode(Inst.Opc, DL, VT, SDImm);
      else
        Result = CurDAG->getMachineNode(Inst.Opc, DL, VT, SrcReg, SDImm);
      SrcReg = SDValue(Result, 0);
    }
    ReplaceNode(Node, Result);
    return true;
  }

  case ISD::BUILD_VECTOR: {
    // Select appropriate vldi.[bhwd] instructions for constant splats of
    // 128-bit when LSX is enabled. Select appropriate xvldi.[bhwd] instructions for constant
    // splats of 256-bit when LASX is enabled. Fixup any register class mismatches that
    // occur as a result.
    //
    // This allows the compiler to use a wider range of immediates than would
    // otherwise be allowed. If, for example, v4i32 could only use [v/xv]ldi.h then
    // it would not be possible to load { 0x01010101, 0x01010101, 0x01010101,
    // 0x01010101 } without using a constant pool. This would be sub-optimal
    // when // '[v/xv]ldi.b vd, 1' is capable of producing that bit-pattern in the
    // same set/ of registers. Similarly, [v/xv]ldi.h isn't capable of producing {
    // 0x00000000, 0x00000001, 0x00000000, 0x00000001 } but '[v/xv]ldi.d vd, 1' can.

    const LoongArchABIInfo &ABI =
        static_cast<const LoongArchTargetMachine &>(TM).getABI();

    BuildVectorSDNode *BVN = cast<BuildVectorSDNode>(Node);
    APInt SplatValue, SplatUndef;
    unsigned SplatBitSize;
    bool HasAnyUndefs;
    unsigned LdiOp;
    EVT ResVecTy = BVN->getValueType(0);
    EVT ViaVecTy;

    if ((!Subtarget->hasLSX() || !BVN->getValueType(0).is128BitVector()) && 
        (!Subtarget->hasLASX() || !BVN->getValueType(0).is256BitVector()))
      return false;

    if (!BVN->isConstantSplat(SplatValue, SplatUndef, SplatBitSize,
                              HasAnyUndefs, 8))
      return false;

    bool IsLASX256 = BVN->getValueType(0).is256BitVector();

    switch (SplatBitSize) {
    default:
      return false;
    case 8:
      LdiOp = IsLASX256 ? LoongArch::XVLDI_B : LoongArch::VLDI_B;
      ViaVecTy = IsLASX256 ? MVT::v32i8 : MVT::v16i8;
      break;
    case 16:
      LdiOp = IsLASX256 ? LoongArch::XVLDI_H : LoongArch::VLDI_H;
      ViaVecTy = IsLASX256 ? MVT::v16i16 : MVT::v8i16;
      break;
    case 32:
      LdiOp = IsLASX256 ? LoongArch::XVLDI_W : LoongArch::VLDI_W;
      ViaVecTy = IsLASX256 ? MVT::v8i32 : MVT::v4i32;
      break;
    case 64:
      LdiOp = IsLASX256 ? LoongArch::XVLDI_D : LoongArch::VLDI_D;
      ViaVecTy = IsLASX256 ? MVT::v4i64 : MVT::v2i64;
      break;
    }

    SDNode *Res;

    // If we have a signed 13 bit integer, we can splat it directly.
    //
    // If we have something bigger we can synthesize the value into a GPR and
    // splat from there.
    if (SplatValue.isSignedIntN(10)) {
      SDValue Imm = CurDAG->getTargetConstant(SplatValue, DL,
                                              ViaVecTy.getVectorElementType());

      Res = CurDAG->getMachineNode(LdiOp, DL, ViaVecTy, Imm);
    } else if (SplatValue.isSignedIntN(12)){
        bool Is32BitSplat = SplatBitSize < 64 ? true : false ;
        const unsigned ADDIOp = Is32BitSplat ? LoongArch::ADDI_W : LoongArch::ADDI_D;
        const MVT SplatMVT = Is32BitSplat ? MVT::i32 : MVT::i64;
        SDValue ZeroVal = CurDAG->getRegister(Is32BitSplat ? LoongArch::ZERO : LoongArch::ZERO_64, SplatMVT);

        const unsigned FILLOp = (SplatBitSize == 16) ? (IsLASX256 ? LoongArch::XVREPLGR2VR_H : LoongArch::VREPLGR2VR_H)
                                : (SplatBitSize == 32 ? (IsLASX256 ? LoongArch::XVREPLGR2VR_W : LoongArch::VREPLGR2VR_W)
                                : (SplatBitSize == 64 ? (IsLASX256 ? LoongArch::XVREPLGR2VR_D : LoongArch::VREPLGR2VR_D) : 0));

          assert(FILLOp != 0 && "Unknown FILL Op for splat synthesis!");

          short Lo = SplatValue.getLoBits(12).getSExtValue();
          SDValue LoVal = CurDAG->getTargetConstant(Lo, DL, SplatMVT);

          Res = CurDAG->getMachineNode(ADDIOp, DL, SplatMVT, ZeroVal, LoVal);
          Res = CurDAG->getMachineNode(FILLOp, DL, ViaVecTy, SDValue(Res, 0));
    }
    else if (SplatValue.isSignedIntN(16) && SplatBitSize == 16){
      const unsigned Lo = SplatValue.getLoBits(12).getZExtValue();
      const unsigned Hi = SplatValue.lshr(12).getLoBits(4).getZExtValue();
      SDValue ZeroVal = CurDAG->getRegister(LoongArch::ZERO, MVT::i32);

      SDValue LoVal = CurDAG->getTargetConstant(Lo, DL, MVT::i32);
      SDValue HiVal = CurDAG->getTargetConstant(Hi, DL, MVT::i32);
      if (Hi)
        Res = CurDAG->getMachineNode(LoongArch::LU12I_W32, DL, MVT::i32, HiVal);

      if (Lo)
        Res = CurDAG->getMachineNode(LoongArch::ORI32, DL, MVT::i32,
                                     Hi ? SDValue(Res, 0) : ZeroVal, LoVal);

      assert((Hi || Lo) && "Zero case reached 32 bit case splat synthesis!");
      const unsigned FILLOp = IsLASX256 ? LoongArch::XVREPLGR2VR_H : LoongArch::VREPLGR2VR_H;
      EVT FILLTy = IsLASX256 ? MVT::v16i16 : MVT::v8i16;
      Res = CurDAG->getMachineNode(FILLOp, DL, FILLTy, SDValue(Res, 0));
    }
    else if (SplatValue.isSignedIntN(32) && SplatBitSize == 32) {
      // Only handle the cases where the splat size agrees with the size
      // of the SplatValue here.
      const unsigned Lo = SplatValue.getLoBits(12).getZExtValue();
      const unsigned Hi = SplatValue.lshr(12).getLoBits(20).getZExtValue();
      SDValue ZeroVal = CurDAG->getRegister(LoongArch::ZERO, MVT::i32);

      SDValue LoVal = CurDAG->getTargetConstant(Lo, DL, MVT::i32);
      SDValue HiVal = CurDAG->getTargetConstant(Hi, DL, MVT::i32);
      if (Hi)
        Res = CurDAG->getMachineNode(LoongArch::LU12I_W32, DL, MVT::i32, HiVal);

      if (Lo)
        Res = CurDAG->getMachineNode(LoongArch::ORI32, DL, MVT::i32,
                                     Hi ? SDValue(Res, 0) : ZeroVal, LoVal);

      assert((Hi || Lo) && "Zero case reached 32 bit case splat synthesis!");
      const unsigned FILLOp = IsLASX256 ? LoongArch::XVREPLGR2VR_W : LoongArch::VREPLGR2VR_W;
      EVT FILLTy = IsLASX256 ? MVT::v8i32 : MVT::v4i32;
      Res = CurDAG->getMachineNode(FILLOp, DL, FILLTy, SDValue(Res, 0));

    } else if (SplatValue.isSignedIntN(32) && SplatBitSize == 64 &&
               ABI.IsLP64()) {
      // LPX32 and LP64 can perform some tricks that LP32 can't for signed 32 bit
      // integers due to having 64bit registers. lui will cause the necessary
      // zero/sign extension.
      const unsigned Lo = SplatValue.getLoBits(12).getZExtValue();
      const unsigned Hi = SplatValue.lshr(12).getLoBits(20).getZExtValue();
      SDValue ZeroVal = CurDAG->getRegister(LoongArch::ZERO, MVT::i32);

      SDValue LoVal = CurDAG->getTargetConstant(Lo, DL, MVT::i32);
      SDValue HiVal = CurDAG->getTargetConstant(Hi, DL, MVT::i32);
      if (Hi)
        Res = CurDAG->getMachineNode(LoongArch::LU12I_W32, DL, MVT::i32, HiVal);

      if (Lo)
        Res = CurDAG->getMachineNode(LoongArch::ORI32, DL, MVT::i32,
                                     Hi ? SDValue(Res, 0) : ZeroVal, LoVal);

      Res = CurDAG->getMachineNode(
              LoongArch::SUBREG_TO_REG, DL, MVT::i64,
              CurDAG->getTargetConstant(((Hi >> 19) & 0x1), DL, MVT::i64),
              SDValue(Res, 0),
              CurDAG->getTargetConstant(LoongArch::sub_32, DL, MVT::i64));

      const unsigned FILLOp = IsLASX256 ? LoongArch::XVREPLGR2VR_D : LoongArch::VREPLGR2VR_D;
      const EVT FILLTy = IsLASX256 ? MVT::v4i64 : MVT::v2i64;
      Res = CurDAG->getMachineNode(FILLOp, DL, FILLTy, SDValue(Res, 0));

    } else if (SplatValue.isSignedIntN(64)) {
      // If we have a 64 bit Splat value, we perform a similar sequence to the
      // above:
      //
      // LoongArch32:                            LoongArch64:
      //   lui $res, %highest(val)            lui $res, %highest(val)
      //   ori $res, $res, %higher(val)       ori $res, $res, %higher(val)
      //   lui $res2, %hi(val)                lui $res2, %hi(val)
      //   ori $res2, %res2, %lo(val)         ori $res2, %res2, %lo(val)
      //   $res3 = fill $res2                 dinsu $res, $res2, 0, 32
      //   $res4 = insert.w $res3[1], $res    fill.d $res
      //   splat.d $res4, 0
      //

      const unsigned Lo = SplatValue.getLoBits(12).getZExtValue();
      const unsigned Hi = SplatValue.lshr(12).getLoBits(20).getZExtValue();
      const unsigned Higher = SplatValue.lshr(32).getLoBits(12).getZExtValue();
      const unsigned Highest = SplatValue.lshr(44).getLoBits(20).getZExtValue();

      SDValue LoVal = CurDAG->getTargetConstant(Lo, DL, MVT::i32);
      SDValue HiVal = CurDAG->getTargetConstant(Hi, DL, MVT::i32);
      SDValue HigherVal = CurDAG->getTargetConstant(Higher, DL, MVT::i32);
      SDValue HighestVal = CurDAG->getTargetConstant(Highest, DL, MVT::i32);
      SDValue ZeroVal = CurDAG->getRegister(LoongArch::ZERO, MVT::i32);

      // Independent of whether we're targeting LoongArch64 or not, the basic
      // operations are the same. Also, directly use the $zero register if
      // the 16 bit chunk is zero.
      //
      // For optimization purposes we always synthesize the splat value as
      // an i32 value, then if we're targetting LoongArch64, use SUBREG_TO_REG
      // just before combining the values with dinsu to produce an i64. This
      // enables SelectionDAG to aggressively share components of splat values
      // where possible.
      //
      // FIXME: This is the general constant synthesis problem. This code
      //        should be factored out into a class shared between all the
      //        classes that need it. Specifically, for a splat size of 64
      //        bits that's a negative number we can do better than LUi/ORi
      //        for the upper 32bits.
      if (Hi)
        Res = CurDAG->getMachineNode(LoongArch::LU12I_W32, DL, MVT::i32, HiVal);

      if (Lo)
        Res = CurDAG->getMachineNode(LoongArch::ORI32, DL, MVT::i32,
                                     Hi ? SDValue(Res, 0) : ZeroVal, LoVal);

      SDNode *HiRes;
      if (Highest)
        HiRes = CurDAG->getMachineNode(LoongArch::LU12I_W32, DL, MVT::i32, HighestVal);

      if (Higher)
        HiRes = CurDAG->getMachineNode(LoongArch::ORI32, DL, MVT::i32,
                                       Highest ? SDValue(HiRes, 0) : ZeroVal,
                                       HigherVal);


      if (ABI.IsLP64()) {

        unsigned FillOp = IsLASX256 ? LoongArch::XVREPLGR2VR_W : LoongArch::VREPLGR2VR_W;
        EVT OpTy = IsLASX256 ? MVT::v8i32 : MVT::v4i32;
        Res = CurDAG->getMachineNode(FillOp, DL, OpTy, (Hi || Lo) ? SDValue(Res, 0) : ZeroVal);

        unsigned InsertOp = IsLASX256 ? LoongArch::XVINSGR2VR_W : LoongArch::VINSGR2VR_W;
        Res = CurDAG->getMachineNode(InsertOp, DL, OpTy, SDValue(Res, 0),
                                     (Highest || Higher) ? SDValue(HiRes, 0) : ZeroVal,
                                     CurDAG->getTargetConstant(1, DL, MVT::i32));

        const TargetRegisterClass *RC = TLI->getRegClassFor(ViaVecTy.getSimpleVT());
        Res = CurDAG->getMachineNode(
              LoongArch::COPY_TO_REGCLASS, DL, ViaVecTy, SDValue(Res, 0),
              CurDAG->getTargetConstant(RC->getID(), DL, MVT::i32));

        OpTy = IsLASX256 ? MVT::v4i64 : MVT::v2i64;
        if(IsLASX256){
          Res = CurDAG->getMachineNode(LoongArch::XVREPLVE0_D, DL, OpTy, SDValue(Res, 0));
        } else {
          Res = CurDAG->getMachineNode(LoongArch::VREPLVEI_D, DL, OpTy, SDValue(Res, 0),
                                       CurDAG->getTargetConstant(0, DL, MVT::i32));
        }
      } else
        llvm_unreachable("Unknown ABI in LoongArchISelDAGToDAG!");

    } else
      return false;

    if (ResVecTy != ViaVecTy) {
      // If LdiOp is writing to a different register class to ResVecTy, then
      // fix it up here. This COPY_TO_REGCLASS should never cause a move.v
      // since the source and destination register sets contain the same
      // registers.
      const TargetLowering *TLI = getTargetLowering();
      MVT ResVecTySimple = ResVecTy.getSimpleVT();
      const TargetRegisterClass *RC = TLI->getRegClassFor(ResVecTySimple);
      Res = CurDAG->getMachineNode(LoongArch::COPY_TO_REGCLASS, DL,
                                   ResVecTy, SDValue(Res, 0),
                                   CurDAG->getTargetConstant(RC->getID(), DL,
                                                             MVT::i32));
    }

    ReplaceNode(Node, Res);
    return true;
  }
  }

  return false;
}

/// Select instructions not customized! Used for
/// expanded, promoted and normal instructions
void LoongArchDAGToDAGISel::Select(SDNode *Node) {
  // If we have a custom node, we already have selected!
  if (Node->isMachineOpcode()) {
    LLVM_DEBUG(errs() << "== "; Node->dump(CurDAG); errs() << "\n");
    Node->setNodeId(-1);
    return;
  }

  // See if subclasses can handle this node.
  if (trySelect(Node))
    return;

  // Select the default instruction
  SelectCode(Node);
}

bool LoongArchDAGToDAGISel::
SelectInlineAsmMemoryOperand(const SDValue &Op, unsigned ConstraintID,
                             std::vector<SDValue> &OutOps) {
  SDValue Base, Offset;

  switch(ConstraintID) {
  default:
    llvm_unreachable("Unexpected asm memory constraint");
  // All memory constraints can at least accept raw pointers.
  case InlineAsm::Constraint_i:
    OutOps.push_back(Op);
    OutOps.push_back(CurDAG->getTargetConstant(0, SDLoc(Op), MVT::i32));
    return false;
  case InlineAsm::Constraint_m:
    if (selectAddrRegImm12(Op, Base, Offset)) {
      OutOps.push_back(Base);
      OutOps.push_back(Offset);
      return false;
    }
    OutOps.push_back(Op);
    OutOps.push_back(CurDAG->getTargetConstant(0, SDLoc(Op), MVT::i32));
    return false;
  case InlineAsm::Constraint_R:
    if (selectAddrRegImm12(Op, Base, Offset)) {
      OutOps.push_back(Base);
      OutOps.push_back(Offset);
      return false;
    }
    OutOps.push_back(Op);
    OutOps.push_back(CurDAG->getTargetConstant(0, SDLoc(Op), MVT::i32));
    return false;
  case InlineAsm::Constraint_ZC:
    if (selectIntAddrSImm14Lsl2(Op, Base, Offset)) {
      OutOps.push_back(Base);
      OutOps.push_back(Offset);
      return false;
    }
    OutOps.push_back(Op);
    OutOps.push_back(CurDAG->getTargetConstant(0, SDLoc(Op), MVT::i32));
    return false;
  case InlineAsm::Constraint_ZB:
    OutOps.push_back(Op);
    OutOps.push_back(CurDAG->getTargetConstant(0, SDLoc(Op), MVT::i32));
    return false;
  }
  return true;
}

FunctionPass *llvm::createLoongArchISelDag(LoongArchTargetMachine &TM,
                                           CodeGenOpt::Level OptLevel) {
  return new LoongArchDAGToDAGISel(TM, OptLevel);
}
