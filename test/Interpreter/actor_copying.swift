// RUN: %target-run-simple-swift | %FileCheck %s
// REQUIRES: executable_test

import Foundation
import Dispatch

@actorSafe
struct S {
  var i: Int
  func copy() -> S {
    print("copied –– main=\(Thread.isMainThread)")
    return self
  }
}

let sema = DispatchSemaphore(value: 0)

actor class C {
  var j: S
  init(j: S) {
    self.j = j
    print("init(j:) –– main=\(Thread.isMainThread)")
  }
  actor func foo(_ s: S) {
    print("foo(_:) –– main=\(Thread.isMainThread)")
    sema.signal()
  }
}

let s = S(i: 5)
let c = C(j: s)
c.foo(s)

sema.wait()

// CHECK:      copied –– main=true
// CHECK-NEXT: init(j:) –– main=true
// CHECK-NEXT: copied –– main=true
// CHECK-NEXT: foo(_:) –– main=false
