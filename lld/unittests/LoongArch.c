// The cleanInstrImm function is copied from lld/ELF/Arch/LoongArch.cpp.
// It is intented to clear the immediate number in the instruction.
// The TESTcleanInstrImm test is added to verify whether the immediate
// number in the instruction can be cleared.
// Test command: clang ./LoongArch.c -o la && ./la

#include <stdio.h>

typedef unsigned int uint32_t;
typedef unsigned long uint64_t;

static uint32_t cleanInstrImm(uint32_t v, uint32_t hi, uint32_t lo) {
  return v & ~((((1ULL << (hi + 1)) - 1) >> lo) << lo);
}

struct InstrImmRange {
  uint32_t ins;
  uint32_t hi;
  uint32_t lo;
};
typedef struct InstrImmRange InstrImm;

#define IMM_TYPE_SIZE 3

int TESTcleanInstrImm() {
  printf("uint32_t size = %d\n", sizeof(uint32_t));
  printf("uint64_t size = %d\n", sizeof(uint64_t));
  InstrImm ins[IMM_TYPE_SIZE] = {{0x54008000, 25, 0},   // bl 128
                                 {0x58001085, 25, 10},  // beq $r4,$r5,16
                                 {0x40001080, 25, 10}}; // beqz $r4,16

  uint32_t ins0[IMM_TYPE_SIZE] = {0x54000000,  // bl 0
                                  0x58000085,  // beq $r4,$r5,0
                                  0x40000080}; // beqz $r4,0

  for (int i = 0; i < IMM_TYPE_SIZE; i++) {
    if (cleanInstrImm(ins[i].ins, ins[i].hi, ins[i].lo) == ins0[i]) {
      printf("clean instruction %x OK, index : %d\n", ins[i].ins, i);
    } else {
      printf("clean instruction %x FAIL, index : %d\n", ins[i].ins, i);
      return -1;
    }
  }

  return 0;
}

int main() {
  int ret = 0;
  ret += TESTcleanInstrImm();
  return ret;
}
