// RUN: %target-typecheck-verify-swift

import Dispatch

actor class X {
  actor func foo() {
    bar()
    self.bar()
    makeX().bar() // expected-error {{actor-internal instance method 'bar()' can only be called directly by its own actor}}

    _ = self.bar
    // expected-error@-1 {{partial application of actor-internal method is not allowed}}
    // expected-note@-2 {{actor-internal instance method 'bar()' can only be called directly by its own actor}}

    _ = bar
    // expected-error@-1 {{partial application of actor-internal method is not allowed}}
    // expected-note@-2 {{actor-internal instance method 'bar()' can only be called directly by its own actor}}

    _ = X.bar     // expected-error {{actor-internal instance method 'bar()' can only be called directly by its own actor}}
    _ = self[x: 0]
    self[x: 0] += ""
  }
  func bar() {
    foo()
    self.foo()
    
    _ = self.bar
    // expected-error@-1 {{partial application of actor-internal method is not allowed}}
    // expected-note@-2 {{actor-internal instance method 'bar()' can only be called directly by its own actor}}

    _ = bar
    // expected-error@-1 {{partial application of actor-internal method is not allowed}}
    // expected-note@-2 {{actor-internal instance method 'bar()' can only be called directly by its own actor}}

    _ = X.bar    // expected-error {{actor-internal instance method 'bar()' can only be called directly by its own actor}}
    _ = self[x: 0]
    property = 1
    self.property = 1
  }
  func makeX() -> X {
    return X()
  }

  var property = 0

  subscript(x x: Int) -> String {
    get { return "" } set {}
  }
}

actor struct X1 {} // expected-error {{'actor' modifier cannot be applied to this declaration}}

struct X2 {
  actor func foo() {} // expected-error {{'actor' method can only be defined within an 'actor' type}}
}

class Y {}

extension X {
  actor func baz(_ s: String) {
    qux(s)
    makeX().bar() // expected-error {{actor-internal instance method 'bar()' can only be called directly by its own actor}}
  }
  func qux(_ s: String) {
    baz(s)
  }

  actor func quux(_ x: Y) { // expected-error {{'actor' instance method parameter #1 does not conform to Copyable}}
  }
  actor func quinx() -> String { // expected-error {{'actor' method cannot return a value}}
    return ""
  }
}

let x = X()
x.foo()
x.baz("")

x.bar() // expected-error {{actor-internal instance method 'bar()' can only be called directly by its own actor}}
x.qux("") // expected-error {{actor-internal instance method 'qux' can only be called directly by its own actor}}
_ = x[x: 0] // expected-error {{subscript 'subscript(x:)' can only be accessed directly by its own actor}}
x[x: 0] += "" // expected-error {{subscript 'subscript(x:)' can only be accessed directly by its own actor}}
x.property += 1 // expected-error {{property 'property' can only be accessed directly by its own actor}}

let y = X.init()
_ = x.qux // expected-error {{actor-internal instance method 'qux' can only be called directly by its own actor}}
_ = X.qux // expected-error {{actor-internal instance method 'qux' can only be called directly by its own actor}}

actor class X3 {
  init(_ y: Y) {} // expected-error {{'actor' initializer parameter #1 does not conform to Copyable}}
}

actor class X4 {
  @objc actor func foo() {}
  @objc func bar() {} // expected-error{{actor-internal method cannot be @objc}}
  actor func qux() throws {} // expected-error{{'actor' method cannot throw}}
}

// Actor function parameters are always escaping.
actor class X5 {
  actor func foo(_ fn: () -> Void) {}
}

func callX5Foo(_ fn: @actorSafe () -> Void) { // expected-note {{parameter 'fn' is implicitly non-escaping}}
  X5().foo(fn) // expected-error {{passing non-escaping parameter 'fn' to function expecting an @escaping closure}}
}

actor class X6 {
  let completion: @actorSafe (Int) -> Void
  init(completion: @escaping (Int) -> Void) {
    self.completion = completion
  }
}

actor class X7 {
  static func foo() {}

  func foo(_ x: Int) {
    X7.foo()
  }
  func bar() {
    _ = X6(completion: foo)
    // expected-error@-1 {{partial application of actor-internal method is not allowed}}
    // expected-note@-2 {{actor-internal instance method 'foo' can only be called directly by its own actor}}
  }
}

protocol P1 {
  static func foo()
}

actor class X8 : P1 {
  static func foo() {} // Okay.
}

protocol P2 {
  func foo() // expected-note {{requirement 'foo()' declared here}}
}

actor class X9 : P2 { // expected-error {{type 'X9' does not conform to protocol 'P2'}}
  func foo() {} // expected-error {{actor-internal instance method 'foo()' cannot be used to satisfy a requirement of protocol 'P2'}}
}
