# REQUIRES: loongarch
# Check la.got relocation calculation. In this case, la.global will be expanded to la.got.

# RUN: llvm-mc -filetype=obj -triple=loongarch64 %s -o %t.o
# RUN: llvm-readobj -relocations %t.o | FileCheck -check-prefix=RELOC %s
# RUN: ld.lld %t.o -o %t.exe
# RUN: llvm-objdump -section-headers -t %t.exe | FileCheck -check-prefix=EXE_SYM %s
# RUN: llvm-objdump -s -section=.got %t.exe | FileCheck -check-prefix=EXE_GOT %s
# RUN: llvm-objdump -d %t.exe | FileCheck -check-prefix=EXE_DIS %s
# RUN: llvm-readobj -relocations %t.exe | FileCheck -check-prefix=NORELOC %s

.text
.globl  _start
_start:
  la.global $r12, value

.data
value:
  .word 1

# RELOC:      Relocations [
# RELOC-NEXT:   Section (3) .rela.text {
# RELOC-NEXT:     0x0 R_LARCH_SOP_PUSH_PCREL _GLOBAL_OFFSET_TABLE_ 0x800
# RELOC-NEXT:     0x0 R_LARCH_SOP_PUSH_GPREL value 0x0
# RELOC-NEXT:     0x0 R_LARCH_SOP_ADD - 0x0
# RELOC-NEXT:     0x0 R_LARCH_SOP_PUSH_ABSOLUTE - 0xC
# RELOC-NEXT:     0x0 R_LARCH_SOP_SR - 0x0
# RELOC-NEXT:     0x0 R_LARCH_SOP_POP_32_S_5_20 - 0x0
# RELOC-NEXT:     0x4 R_LARCH_SOP_PUSH_PCREL _GLOBAL_OFFSET_TABLE_ 0x4
# RELOC-NEXT:     0x4 R_LARCH_SOP_PUSH_GPREL value 0x0
# RELOC-NEXT:     0x4 R_LARCH_SOP_ADD - 0x0
# RELOC-NEXT:     0x4 R_LARCH_SOP_PUSH_PCREL _GLOBAL_OFFSET_TABLE_ 0x804
# RELOC-NEXT:     0x4 R_LARCH_SOP_PUSH_GPREL value 0x0
# RELOC-NEXT:     0x4 R_LARCH_SOP_ADD - 0x0
# RELOC-NEXT:     0x4 R_LARCH_SOP_PUSH_ABSOLUTE - 0xC
# RELOC-NEXT:     0x4 R_LARCH_SOP_SR - 0x0
# RELOC-NEXT:     0x4 R_LARCH_SOP_PUSH_ABSOLUTE - 0xC
# RELOC-NEXT:     0x4 R_LARCH_SOP_SL - 0x0
# RELOC-NEXT:     0x4 R_LARCH_SOP_SUB - 0x0
# RELOC-NEXT:     0x4 R_LARCH_SOP_POP_32_S_10_12 - 0x0
# RELOC-NEXT:   }
# RELOC-NEXT: ]

# EXE_SYM: Sections:
# EXE_SYM: Idx Name          Size      Address          Type
# EXE_SYM:   3 .got          00000010 0000000120030000 DATA
# EXE_SYM: SYMBOL TABLE:
# EXE_SYM: 0000000120020000         .data       00000000 value
# EXE_SYM: 0000000120030000         .got        00000000 .hidden _GLOBAL_OFFSET_TABLE_
#          ^---- .got

# EXE_GOT:      Contents of section .got:
# EXE_GOT-NEXT: 120030000 00000000 00000000 00000220 01000000
#                         ^                 ^---------value
#                         +-- .dynamic address (if exist)

# pcaddu12i  rd,(%pcrel(_GLOBAL_OFFSET_TABLE_+0x800)+%gprel(symbol))>>12
# value_GotAddr=%gprel(synbol)=0x120030008-0x120030000 = 8
# (0x120030000+0x800-0x120010000+8)>>12 = 32

# ld.d  rd,rd,%pcrel(_GLOBAL_OFFSET_TABLE_+4)+%gprel(symbol)-((%pcrel(
# _GLOBAL_OFFSET_TABLE_+4+0x800)+%gprel(symbol))>>12<<12)
# (0x120030000+4-0x120010004)+8-((0x120030000+4+0x800-0x120010004)+8)>>12<<12=8

# EXE_DIS:      120010000:      0c 04 00 1c     pcaddu12i       $r12, 32
# EXE_DIS-NEXT: 120010004:      8c 21 c0 28     ld.d    $r12, $r12, 8

# NORELOC:      Relocations [
# NORELOC-NEXT: ]
