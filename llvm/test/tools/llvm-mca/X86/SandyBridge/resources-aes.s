# NOTE: Assertions have been autogenerated by utils/update_mca_test_checks.py
# RUN: llvm-mca -mtriple=x86_64-unknown-unknown -mcpu=sandybridge -instruction-tables < %s | FileCheck %s

aesdec          %xmm0, %xmm2
aesdec          (%rax), %xmm2

aesdeclast      %xmm0, %xmm2
aesdeclast      (%rax), %xmm2

aesenc          %xmm0, %xmm2
aesenc          (%rax), %xmm2

aesenclast      %xmm0, %xmm2
aesenclast      (%rax), %xmm2

aesimc          %xmm0, %xmm2
aesimc          (%rax), %xmm2

aeskeygenassist $22, %xmm0, %xmm2
aeskeygenassist $22, (%rax), %xmm2

# CHECK:      Instruction Info:
# CHECK-NEXT: [1]: #uOps
# CHECK-NEXT: [2]: Latency
# CHECK-NEXT: [3]: RThroughput
# CHECK-NEXT: [4]: MayLoad
# CHECK-NEXT: [5]: MayStore
# CHECK-NEXT: [6]: HasSideEffects (U)

# CHECK:      [1]    [2]    [3]    [4]    [5]    [6]    Instructions:
# CHECK-NEXT:  2      7     1.00                        aesdec	%xmm0, %xmm2
# CHECK-NEXT:  3      13    1.00    *                   aesdec	(%rax), %xmm2
# CHECK-NEXT:  2      7     1.00                        aesdeclast	%xmm0, %xmm2
# CHECK-NEXT:  3      13    1.00    *                   aesdeclast	(%rax), %xmm2
# CHECK-NEXT:  2      7     1.00                        aesenc	%xmm0, %xmm2
# CHECK-NEXT:  3      13    1.00    *                   aesenc	(%rax), %xmm2
# CHECK-NEXT:  2      7     1.00                        aesenclast	%xmm0, %xmm2
# CHECK-NEXT:  3      13    1.00    *                   aesenclast	(%rax), %xmm2
# CHECK-NEXT:  2      12    2.00                        aesimc	%xmm0, %xmm2
# CHECK-NEXT:  3      18    2.00    *                   aesimc	(%rax), %xmm2
# CHECK-NEXT:  1      8     3.67                        aeskeygenassist	$22, %xmm0, %xmm2
# CHECK-NEXT:  1      8     3.33    *                   aeskeygenassist	$22, (%rax), %xmm2

# CHECK:      Resources:
# CHECK-NEXT: [0]   - SBDivider
# CHECK-NEXT: [1]   - SBFPDivider
# CHECK-NEXT: [2]   - SBPort0
# CHECK-NEXT: [3]   - SBPort1
# CHECK-NEXT: [4]   - SBPort4
# CHECK-NEXT: [5]   - SBPort5
# CHECK-NEXT: [6.0] - SBPort23
# CHECK-NEXT: [6.1] - SBPort23

# CHECK:      Resource pressure per iteration:
# CHECK-NEXT: [0]    [1]    [2]    [3]    [4]    [5]    [6.0]  [6.1]
# CHECK-NEXT:  -      -     9.67   9.67    -     21.67  3.00   3.00

# CHECK:      Resource pressure by instruction:
# CHECK-NEXT: [0]    [1]    [2]    [3]    [4]    [5]    [6.0]  [6.1]  Instructions:
# CHECK-NEXT:  -      -     0.33   0.33    -     1.33    -      -     aesdec	%xmm0, %xmm2
# CHECK-NEXT:  -      -     0.33   0.33    -     1.33   0.50   0.50   aesdec	(%rax), %xmm2
# CHECK-NEXT:  -      -     0.33   0.33    -     1.33    -      -     aesdeclast	%xmm0, %xmm2
# CHECK-NEXT:  -      -     0.33   0.33    -     1.33   0.50   0.50   aesdeclast	(%rax), %xmm2
# CHECK-NEXT:  -      -     0.33   0.33    -     1.33    -      -     aesenc	%xmm0, %xmm2
# CHECK-NEXT:  -      -     0.33   0.33    -     1.33   0.50   0.50   aesenc	(%rax), %xmm2
# CHECK-NEXT:  -      -     0.33   0.33    -     1.33    -      -     aesenclast	%xmm0, %xmm2
# CHECK-NEXT:  -      -     0.33   0.33    -     1.33   0.50   0.50   aesenclast	(%rax), %xmm2
# CHECK-NEXT:  -      -      -      -      -     2.00    -      -     aesimc	%xmm0, %xmm2
# CHECK-NEXT:  -      -      -      -      -     2.00   0.50   0.50   aesimc	(%rax), %xmm2
# CHECK-NEXT:  -      -     3.67   3.67    -     3.67    -      -     aeskeygenassist	$22, %xmm0, %xmm2
# CHECK-NEXT:  -      -     3.33   3.33    -     3.33   0.50   0.50   aeskeygenassist	$22, (%rax), %xmm2
