//===- LoongArchAnalyzeImmediate.h - Analyze Immediates --------*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_LOONGARCH_LOONGARCHANALYZEIMMEDIATE_H
#define LLVM_LIB_TARGET_LOONGARCH_LOONGARCHANALYZEIMMEDIATE_H

#include "llvm/ADT/SmallVector.h"

namespace llvm {
namespace LoongArchAnalyzeImmediate {
struct Inst {
  unsigned Opc;
  int64_t Imm;
  Inst(unsigned Opc, int64_t Imm) : Opc(Opc), Imm(Imm) {}
};
using InstSeq = SmallVector<Inst, 4>;

// Helper to generate an instruction sequence that will materialise the given
// immediate value into a register.
InstSeq generateInstSeq(int64_t Val, bool Is64Bit);
} // end namespace LoongArchAnalyzeImmediate
} // end namespace llvm

#endif // LLVM_LIB_TARGET_LOONGARCH_LOONGARCHANALYZEIMMEDIATE_H
