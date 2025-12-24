// RUN: %target-swift-frontend -typecheck %s -debug-generic-signatures 2>&1 | %FileCheck %s

// This is already cyclic if you manually write 'typealias A<T> = _A<T>'
// REQUIRES: fixme

// CHECK-LABEL: .P@
// CHECK-NEXT: Requirement signature: <Self where Self.[P]X == _A<Int>>
protocol P {
  typealias A = _A
  typealias B = A<Int>

  associatedtype X where X == B
}

struct _A<T> {}
