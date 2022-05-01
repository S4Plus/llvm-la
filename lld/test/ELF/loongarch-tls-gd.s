# REQUIRES: loongarch

# RUN: llvm-mc -filetype=obj -triple=loongarch64-unknown-linux %s -o %t.o
# RUN: llvm-readobj -relocations %t.o | FileCheck -check-prefix=InputRelocs %s
# RUN: ld.lld %t.o -o %t
# RUN: llvm-objdump -s -section=.got %t | FileCheck -check-prefix=GOT %s
# RUN: ld.lld -shared %t.o -o %t.so
# RUN: llvm-readobj -relocations %t.so | FileCheck -check-prefix=OutputRelocs %s
# RUN: llvm-objdump -section-headers -t %t.so | FileCheck -check-prefix=SO_SYM %s
# RUN: llvm-objdump -s -section=.got %t.so | FileCheck -check-prefix=SO_GOT %s
# RUN: llvm-objdump -d %t.so | FileCheck --check-prefixes=DIS %s

# Test the handling of the global-dynamic TLS model. Dynamic Loader finds
# module index R_LARCH_TLS_DTPMODNN. For an executable we write the module
# index 1 and the offset into the TLS directly into the GOT. For a shared
# library we can only write the offset into the TLS directly if the symbol
# is non-preemptible

# la.tls.ld alias for la.tls.gd, So we only check la.tls.gd.

.globl _start
_start:
  la.tls.gd $r12, x
  la.tls.gd $r15, z

.section .tdata
.local x
x:
.byte 10

.global z
z:
.long 10

# InputRelocs:      Relocations [
# InputRelocs-NEXT:   Section (3) .rela.text {
# InputRelocs-NEXT:     0x0 R_LARCH_SOP_PUSH_PCREL _GLOBAL_OFFSET_TABLE_ 0x800
# InputRelocs-NEXT:     0x0 R_LARCH_SOP_PUSH_TLS_GD x 0x0
# InputRelocs-NEXT:     0x0 R_LARCH_SOP_ADD - 0x0
# InputRelocs-NEXT:     0x0 R_LARCH_SOP_PUSH_ABSOLUTE - 0xC
# InputRelocs-NEXT:     0x0 R_LARCH_SOP_SR - 0x0
# InputRelocs-NEXT:     0x0 R_LARCH_SOP_POP_32_S_5_20 - 0x0
# InputRelocs-NEXT:     0x4 R_LARCH_SOP_PUSH_PCREL _GLOBAL_OFFSET_TABLE_ 0x4
# InputRelocs-NEXT:     0x4 R_LARCH_SOP_PUSH_TLS_GD x 0x0
# InputRelocs-NEXT:     0x4 R_LARCH_SOP_ADD - 0x0
# InputRelocs-NEXT:     0x4 R_LARCH_SOP_PUSH_PCREL _GLOBAL_OFFSET_TABLE_ 0x804
# InputRelocs-NEXT:     0x4 R_LARCH_SOP_PUSH_TLS_GD x 0x0
# InputRelocs-NEXT:     0x4 R_LARCH_SOP_ADD - 0x0
# InputRelocs-NEXT:     0x4 R_LARCH_SOP_PUSH_ABSOLUTE - 0xC
# InputRelocs-NEXT:     0x4 R_LARCH_SOP_SR - 0x0
# InputRelocs-NEXT:     0x4 R_LARCH_SOP_PUSH_ABSOLUTE - 0xC
# InputRelocs-NEXT:     0x4 R_LARCH_SOP_SL - 0x0
# InputRelocs-NEXT:     0x4 R_LARCH_SOP_SUB - 0x0
# InputRelocs-NEXT:     0x4 R_LARCH_SOP_POP_32_S_10_12 - 0x0
# InputRelocs-NEXT:     0x8 R_LARCH_SOP_PUSH_PCREL _GLOBAL_OFFSET_TABLE_ 0x800
# InputRelocs-NEXT:     0x8 R_LARCH_SOP_PUSH_TLS_GD z 0x0
# InputRelocs-NEXT:     0x8 R_LARCH_SOP_ADD - 0x0
# InputRelocs-NEXT:     0x8 R_LARCH_SOP_PUSH_ABSOLUTE - 0xC
# InputRelocs-NEXT:     0x8 R_LARCH_SOP_SR - 0x0
# InputRelocs-NEXT:     0x8 R_LARCH_SOP_POP_32_S_5_20 - 0x0
# InputRelocs-NEXT:     0xC R_LARCH_SOP_PUSH_PCREL _GLOBAL_OFFSET_TABLE_ 0x4
# InputRelocs-NEXT:     0xC R_LARCH_SOP_PUSH_TLS_GD z 0x0
# InputRelocs-NEXT:     0xC R_LARCH_SOP_ADD - 0x0
# InputRelocs-NEXT:     0xC R_LARCH_SOP_PUSH_PCREL _GLOBAL_OFFSET_TABLE_ 0x804
# InputRelocs-NEXT:     0xC R_LARCH_SOP_PUSH_TLS_GD z 0x0
# InputRelocs-NEXT:     0xC R_LARCH_SOP_ADD - 0x0
# InputRelocs-NEXT:     0xC R_LARCH_SOP_PUSH_ABSOLUTE - 0xC
# InputRelocs-NEXT:     0xC R_LARCH_SOP_SR - 0x0
# InputRelocs-NEXT:     0xC R_LARCH_SOP_PUSH_ABSOLUTE - 0xC
# InputRelocs-NEXT:     0xC R_LARCH_SOP_SL - 0x0
# InputRelocs-NEXT:     0xC R_LARCH_SOP_SUB - 0x0
# InputRelocs-NEXT:     0xC R_LARCH_SOP_POP_32_S_10_12 - 0x0
# InputRelocs-NEXT:   }
# InputRelocs-NEXT: ]


# For an executable we write the module index 1 and the offset
# into the TLS directly into the GOT.

# GOT:      Contents of section .got:
# GOT-NEXT: 120020008 00000000 00000000 01000000 00000000
#                     ^                 ^-----x-module-id
#                      +-.dynamic address(not exist)
# GOT-NEXT: 120020018 00000000 00000000 01000000 00000000
#                     ^--x-offset       ^-----z-module-id
# GOT-NEXT: 120020028 01000000 00000000
#                     ^--z-offset

# OutputRelocs:      Relocations [
# OutputRelocs-NEXT:   Section (5) .rela.dyn {
# OutputRelocs-NEXT:     0x200B0 R_LARCH_TLS_DTPMOD64 - 0x0
# OutputRelocs-NEXT:     0x200C0 R_LARCH_TLS_DTPMOD64 z 0x0
# OutputRelocs-NEXT:     0x200C8 R_LARCH_TLS_DTPREL64 z 0x0
# OutputRelocs-NEXT:   }
# OutputRelocs-NEXT: ]

# SO_SYM: Sections:
# SO_SYM: Idx Name          Size      Address          Type
# SO_SYM:   8 .tdata        00000005 0000000000020000 DATA
# SO_SYM:   9 .dynamic      000000a0 0000000000020008
# SO_SYM:  10 .got          00000028 00000000000200a8 DATA
# SO_SYM: SYMBOL TABLE:
# SO_SYM: 0000000000000000 l     O .tdata   00000000 x
# _GLOBAL_OFFSET_TABLE_ 0x200a8
# SO_SYM: 00000000000200a8         .got 00000000 .hidden _GLOBAL_OFFSET_TABLE_
# SO_SYM: 0000000000020008         .dynamic 00000000 .hidden _DYNAMIC
# SO_SYM: 0000000000000001 g     O .tdata   00000000 z

# For a shared library we can only write the offset into the TLS directly

# SO_GOT:     Contents of section .got:
# SO_GOT-NEXT: 200a8 08000200 00000000 00000000 00000000
#                    ^                 ^-----x-module-id
#                    +---.dynamic address
# SO_GOT-NEXT: 200b8 00000000 00000000 00000000 00000000
#                    ^---x-offset      ^-----z-module-id
# SO_GOT-NEXT: 200c8 00000000 00000000
#                    ^---z-offset

# %tlsgd(x)=8 ; %tlsgd(z)=24

# la.tls.gd rd, symbol will be expanded to such instructions:
# pcaddu12i rd,(%pcrel(_GLOBAL_OFFSET_TABLE_+0x800)+%tlsgd(symbol))>>12
# addi.d rd,rd,%pcrel(_GLOBAL_OFFSET_TABLE_+4)+%tlsgd(symbol)- \
# ((%pcrel(_GLOBAL_OFFSET_TABLE_+4+0x800)+%tlsgd(symbol))>>12<<12)

# la.tls.gd $r12, x
# (0x200a8+0x800-0x10000+8)>>12 = 16
# DIS:      10000:  0c 02 00 1c     pcaddu12i $r12, 16
# (0x200a8+4-0x10004+8)-((0x200a8+4+0x800-0x10004)+8)>>12<<12 = 0xb8 = 176
# DIS-NEXT: 10004:  8c c1 c2 02     addi.d    $r12, $r12, 176

# la.tls.gd $r15, z
# (0x200a8+0x800-0x1001c+8)>>12 = 16
# DIS-NEXT: 10008:  0f 02 00 1c     pcaddu12i $r15, 16
# (0x200a8+4-0x10020+40)-((0x200a8+4+0x800-0x10020)+40)>>12<<12 = 0xbc = 184
# DIS-NEXT: 1000c: ef e1 c2 02      addi.d $r15, $r15, 184
