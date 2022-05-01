# REQUIRES: loongarch

# RUN: clang -c -g -shared --target=loongarch64-unknown-linux-gnu %S/Inputs/loongarch.s -o %t.o
# RUN: llvm-readelf -r %t.o | FileCheck %s
# RUN: ld.lld %t.o  -o %t.so

# CHECK: .rela.debug_aranges
# CHECK: R_LARCH_ADD64
