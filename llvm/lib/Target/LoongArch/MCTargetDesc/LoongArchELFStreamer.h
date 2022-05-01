//===- LoongArchELFStreamer.h - ELF Object Output --------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This is a custom MCELFStreamer which allows us to insert some hooks before
// emitting data into an actual object file.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_LOONGARCH_MCTARGETDESC_LOONGARCHELFSTREAMER_H
#define LLVM_LIB_TARGET_LOONGARCH_MCTARGETDESC_LOONGARCHELFSTREAMER_H

#include "llvm/ADT/SmallVector.h"
#include "llvm/MC/MCELFStreamer.h"
#include <memory>

namespace llvm {

class MCAsmBackend;
class MCCodeEmitter;
class MCContext;
class MCSubtargetInfo;
struct MCDwarfFrameInfo;

class LoongArchELFStreamer : public MCELFStreamer {

public:
  LoongArchELFStreamer(MCContext &Context, std::unique_ptr<MCAsmBackend> MAB,
                  std::unique_ptr<MCObjectWriter> OW,
                  std::unique_ptr<MCCodeEmitter> Emitter);

  /// Overriding this function allows us to add arbitrary behaviour before the
  /// \p Inst is actually emitted. For example, we can inspect the operands and
  /// gather sufficient information that allows us to reason about the register
  /// usage for the translation unit.
  void EmitInstruction(const MCInst &Inst, const MCSubtargetInfo &STI,
                       bool = false) override;

  /// Overriding these functions allows us to dismiss all labels.
  void EmitValueImpl(const MCExpr *Value, unsigned Size, SMLoc Loc) override;
  void EmitIntValue(uint64_t Value, unsigned Size) override;

  // Overriding these functions allows us to avoid recording of these labels
  // in EmitLabel.
  void EmitCFIStartProcImpl(MCDwarfFrameInfo &Frame) override;
  void EmitCFIEndProcImpl(MCDwarfFrameInfo &Frame) override;
  MCSymbol *EmitCFILabel() override;

};

MCELFStreamer *createLoongArchELFStreamer(MCContext &Context,
                                     std::unique_ptr<MCAsmBackend> MAB,
                                     std::unique_ptr<MCObjectWriter> OW,
                                     std::unique_ptr<MCCodeEmitter> Emitter,
                                     bool RelaxAll);
} // end namespace llvm

#endif // LLVM_LIB_TARGET_LOONGARCH_MCTARGETDESC_LOONGARCHELFSTREAMER_H
