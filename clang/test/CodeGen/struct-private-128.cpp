// NOTE: Assertions have been autogenerated by utils/update_cc_test_checks.py
// RUN: %clang_cc1  -triple loongarch64-linux-gnu -emit-llvm %s -o - | FileCheck %s
typedef struct _B {
    int *ptr;
private:
    int index;
} B;

typedef struct _A : B {
    float dd;
} A;

// CHECK-LABEL: @_Z3foov(
// CHECK-NEXT:  entry:
// CHECK-NEXT:    [[RETVAL:%.*]] = alloca [[STRUCT__A:%.*]], align 8
// CHECK-NEXT:    [[TMP0:%.*]] = bitcast %struct._A* [[RETVAL]] to [2 x i64]*
// CHECK-NEXT:    [[TMP1:%.*]] = load [2 x i64], [2 x i64]* [[TMP0]], align 8
// CHECK-NEXT:    ret [2 x i64] [[TMP1]]
//
A foo()
{
    A a;
    return a;
}
