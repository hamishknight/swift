// RUN: %target-swift-emit-sil -verify -enable-experimental-feature StatementExpressions %s -o /dev/null

func takesGenericReturningFn<R>(_ fn: () -> R) {}

func testOuterClosureReturn() {
  takesGenericReturningFn {
    if .random() {
      return
    } else {
      ()
    }
  }
}

func testNeverDecay() {
  takesGenericReturningFn {
    if .random() { // Okay, Never decays to Void, so R is Void.
      fatalError()
    } else {
      do {}
    }
  }
}
