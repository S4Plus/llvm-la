// NOTE: Assertions have been autogenerated by utils/update_cc_test_checks.py
// RUN: %clang_cc1  -triple loongarch64-linux-gnu -emit-llvm %s -o - | FileCheck %s

//Check that a struct return value with a certain condition on loongarch64 is returned by address

typedef struct _B {
    int *ptr;
    int index;
} B;

typedef struct _A : B {
    float dd;
} A;

// CHECK-LABEL: void @_Z3foov(%struct._A*
// CHECK-NEXT:  entry:
// CHECK-NEXT:    ret void
//
A foo()
{
    A a;
    return a;
}
