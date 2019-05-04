// RUN: %target-run-simple-swift | %FileCheck %s
// REQUIRES: executable_test

import Foundation
import Dispatch

let sema = DispatchSemaphore(value: 0)
let N = 100_000

// Make sure we don't deadlock.
actor class Actor1 {
  var i = 0 {
    didSet {
      if i == N {
        print("Actor1 finished recieving")
        sema.signal()
      }
    }
  }
  actor func run(_ actor2: Actor2) {
    for _ in 0 ..< N {
      actor2.bar()
    }
    print("Actor1 finished sending")
    sema.signal()
  }
  actor func bar() {
    i += 1
  }
}

actor class Actor2 {
  var i = 0 {
    didSet {
      if i == N {
        print("Actor2 finished recieving")
        sema.signal()
      }
    }
  }
  actor func run(_ actor1: Actor1) {
    for _ in 0 ..< N {
      actor1.bar()
    }
    print("Actor2 finished sending")
    sema.signal()
  }
  actor func bar() {
    i += 1
  }
}

let a1 = Actor1()
let a2 = Actor2()
a1.run(a2)
a2.run(a1)

sema.wait()
sema.wait()
sema.wait()
sema.wait()

// CHECK-DAG: Actor1 finished sending
// CHECK-DAG: Actor2 finished sending
// CHECK-DAG: Actor1 finished recieving
// CHECK-DAG: Actor2 finished recieving
