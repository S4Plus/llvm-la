; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc -march=loongarch64 -relocation-model=pic < %s | FileCheck %s

define void @f() {
; CHECK-LABEL: f:
; CHECK:       # %bb.0: # %entry
; CHECK-NEXT:    b foo
entry:
  tail call void bitcast (void (...)* @foo to void ()*)()
  ret void
}

declare void @foo(...)
