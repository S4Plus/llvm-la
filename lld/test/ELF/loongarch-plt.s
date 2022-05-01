# REQUIRES: loongarch
# RUN: echo '.globl bar, weak; .type bar,@function; .type weak,@function; bar: weak:' > %t1.s

# RUN: llvm-mc -filetype=obj -triple=loongarch64 %t1.s -o %t1.64.o
# RUN: ld.lld -shared %t1.64.o -soname=t1.64.so -o %t1.64.so
# RUN: llvm-mc -filetype=obj -triple=loongarch64 %s -o %t.64.o
# RUN: ld.lld %t.64.o %t1.64.so -o %t.64
# RUN: llvm-readelf -S -s %t.64 | FileCheck --check-prefixes=SEC,NM %s
# RUN: llvm-readobj -r %t.64 | FileCheck --check-prefix=RELOC64 %s
# RUN: llvm-readelf -x .got.plt %t.64 | FileCheck --check-prefix=GOTPLT64 %s
# RUN: llvm-objdump -d --no-show-raw-insn %t.64 | FileCheck --check-prefixes=DIS,DIS64 %s

# SEC: .plt PROGBITS {{0*}}120010020

## A canonical PLT has a non-zero st_value. bar and weak are called but their
## addresses are not taken, so a canonical PLT is not necessary.
# NM: {{0*}}00000000 0 FUNC GLOBAL DEFAULT UND bar
# NM: {{0*}}00000000 0 FUNC WEAK   DEFAULT UND weak

## The .got.plt slots relocated by .rela.plt point to .plt
## This is required by glibc.
# RELOC64:      .rela.plt {
# RELOC64-NEXT:   0x120020010 R_LARCH_JUMP_SLOT bar 0x0
# RELOC64-NEXT:   0x120020018 R_LARCH_JUMP_SLOT weak 0x0
# RELOC64-NEXT: }
# GOTPLT64:      section '.got.plt'
# GOTPLT64-NEXT: 0x00000000 00000000 00000000 00000000 00000000
# GOTPLT64-NEXT: 0x00000010 20000120 01000000 20000120 01000000

# DIS:      _start:
## foo - . = 0x120010010-0x120010000 = 16
# DIS-NEXT:   120010000: bl 16 <foo>
## bar@plt - . = 0x120010040-0x120010004 = 60
# DIS-NEXT:   120010004: bl 60
## bar@plt - . = 0x0x120010040-0x120010008 = 56
# DIS-NEXT:   120010008: bl 56
## weak@plt - . = 0x120010050-0x12001000c = 68
# DIS-NEXT:   12001000c: bl 68
# DIS:      foo:
# DIS-NEXT:   120010010: jirl $zero, $ra, 0

## 120020000 .got.plt
# DIS:      Disassembly of section .plt:
# DIS:      .plt:
## hi20(.got.plt - .plt + 0x800) = (0x120020000 - 0x120010020 + 0x800)>>12 = 0x107e0 >> 12 = 0x10
# DIS-NEXT:              pcaddu12i $r14, 16
# DIS-NEXT:              sub.d     $r13, $r13, $r15
## lo12(.got.plt - .plt) = (0x120020000 - 0x120010020) & 0xfff = 0xffe0 & 0xfff = 0xfe0
# DIS64-NEXT:            ld.d      $r15, $r14, -32
# DIS64-NEXT:            addi.d    $r13, $r13, -40
## lo12(.got.plt - .plt) = (0x120020000 - 0x120010020) & 0xfff = 0xffe0 & 0xfff = 0xfe0
# DIS64-NEXT:            addi.d    $r12, $r14, -32
# DIS64-NEXT:            srli.d    $r13, $r13, 1
# DIS64-NEXT:            ld.d      $r12, $r12, 8
# DIS-NEXT:              jirl      $zero, $r15, 0

## hi20(&.got.plt[bar]-.) = (0x120020010 - 0x120010040 + 0x800) >> 12 = 0x107d0 >> 12 = 0x10
# DIS:        120010040: pcaddu12i $r15, 16
## lo12(&.got.plt[bar]-.) = (0x120020010 - 0x120010040) & 0xfff = 0xffd0 & 0xfff = 0xfd0
# DIS64-NEXT:            ld.d      $r15, $r15, -48
# DIS-NEXT:              pcaddu12i $r13, 0
# DIS-NEXT:              jirl      $zero, $r15, 0

## hi20(&.got.plt[weak]-.) = (0x120020018 - 0x120010050 + 0x800) >> 12 = 0x107c8 >> 12 = 0x10
# DIS:        120010050: pcaddu12i $r15, 16
## lo12(&.got.plt[weak]-.) = (0x120020018 - 0x120010050) & 0xfff = 0xffc8 & 0xfff = 0xfc8
# DIS64-NEXT:            ld.d      $r15, $r15, -56
# DIS-NEXT:              pcaddu12i $r13, 0
# DIS-NEXT:              jirl      $zero, $r15, 0

.global _start, foo, bar
.weak weak

_start:
  bl foo
  bl bar
  bl bar@plt
  bl weak

## foo is local and non-preemptale, no PLT is generated.
foo:
  jr $ra
