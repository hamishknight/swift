// RUN: %target-typecheck-verify-swift -disable-objc-attr-requires-foundation-module

import Dispatch

var globalVar = 0
func nonActorSafeGlobal() {}

struct S {
  static var staticVar = 0
}

@actorSafe
func id<T>(_ x: T) -> T { return x }

@actorSafe
func actorSafeGlobal() {
  nonActorSafeGlobal()
  // expected-error@-1 {{expression is not legal in a '@actorSafe' context}}
  // expected-error@-2 {{reference to non-actor-safe global function 'nonActorSafeGlobal()' in actor-safe context}}

  globalVar += 1 // expected-error {{reference to non-actor-safe var 'globalVar' in actor-safe context}}
  _ = globalVar  // expected-error {{reference to non-actor-safe var 'globalVar' in actor-safe context}}

  S.staticVar += 1 // expected-error {{reference to non-actor-safe static property 'staticVar' in actor-safe context}}
  _ = S.staticVar  // expected-error {{reference to non-actor-safe static property 'staticVar' in actor-safe context}}

  // We can make a non-actor-safe closure within an actor safe context.
  let fn = {
    nonActorSafeGlobal()
    globalVar += 1
    _ = globalVar
  }

  // ... we just cant call it.
  fn() // expected-error {{expression is not legal in a '@actorSafe' context}}

  // Local vars are fine
  var i = 0
  i += 1

  var localWObservers = 0 {
    didSet {
      nonActorSafeGlobal()
      // expected-error@-1 {{expression is not legal in a '@actorSafe' context}}
      // expected-error@-2 {{reference to non-actor-safe global function 'nonActorSafeGlobal()' in actor-safe context}}
    }
  }
  localWObservers += 1

  _ = id(5)

  _ = [1, 2, 3]
  let _: [Int] = [1, 2, 3]
  let _: [Int: String] = [5 : ""]
  let _: Set<Int> = [5]
}

protocol ProtocolWActorSafeReq {
  associatedtype T

  @actorSafe func actorSafeFn() // expected-note {{requirement 'actorSafeFn()' declared here}}
  func nonActorSafeFn()

  @actorSafe var actorSafeVar: Int { get }
  var nonActorSafeVar: Int { get }

  @actorSafe subscript(actorSafeSub x: Int) -> Int { get } // expected-note {{requirement 'subscript(actorSafeSub:)' declared here}}
  subscript(nonActorSafeSub x: Int) -> Int { get }
}

// expected-error@+1 {{type 'S1' does not conform to protocol 'ProtocolWActorSafeReq'}}
struct S1 : ProtocolWActorSafeReq {
  typealias T = Float

  func actorSafeFn() {} // expected-error {{non-actor-safe instance method 'actorSafeFn()' cannot be used to satisfy a requirement of protocol 'ProtocolWActorSafeReq'}}
  func nonActorSafeFn() {}

  // Both are actor-safe, as they're within a struct.
  var actorSafeVar: Int = 0
  var nonActorSafeVar: Int = 0

  subscript(actorSafeSub x: Int) -> Int { return x } // expected-error {{non-actor-safe subscript 'subscript(actorSafeSub:)' cannot be used to satisfy a requirement of protocol 'ProtocolWActorSafeReq'}}
  subscript(nonActorSafeSub x: Int) -> Int { return x }
}

@actorSafe
protocol ProtocolWActorSafeVarReq {
  var actorSafeVar1: Int { get } // expected-note 2{{requirement 'actorSafeVar1' declared here}}
  var actorSafeVar2: Int { get } // expected-note {{requirement 'actorSafeVar2' declared here}}
}

// expected-error@+1 {{type 'S2' does not conform to protocol 'ProtocolWActorSafeVarReq'}}
struct S2 : ProtocolWActorSafeVarReq {
  // Not actor-safe by default, as has observer.
  var actorSafeVar1: Int = 0 { // expected-error {{non-actor-safe property 'actorSafeVar1' cannot be used to satisfy a requirement of protocol 'ProtocolWActorSafeVarReq'}}
    didSet {}
  }

  // Also not actor-safe by default.
  var actorSafeVar2: Int { return 0 } // expected-error {{non-actor-safe property 'actorSafeVar2' cannot be used to satisfy a requirement of protocol 'ProtocolWActorSafeVarReq'}}
}

class CBase {
  @objc dynamic var actorSafeVar1: Int = 0
}

@actorSafe
class C1 : CBase {
  override required init() {}
  func copy() -> Self { return type(of: self).init() }
}

// expected-error@+1 {{type 'C1' does not conform to protocol 'ProtocolWActorSafeVarReq'}}
extension C1 : ProtocolWActorSafeVarReq {
  // Not actor-safe, as implicitly dynamic.
  override var actorSafeVar1: Int { // expected-error {{non-actor-safe property 'actorSafeVar1' cannot be used to satisfy a requirement of protocol 'ProtocolWActorSafeVarReq'}}
    get { return 0 }
    set {}
  }

  var actorSafeVar2: Int { return 0 }
}

// Obj-C protocols are a no-go for actor safety.
@objc protocol ObjCProtocol {
  var i: Int { get }
  func foo()
  subscript(x: Int) -> Int { get }
}

@actorSafe func actorSafeGlobal(_ x: ObjCProtocol, _ y: AnyObject) {
  _ = x.i // expected-error {{reference to non-actor-safe property 'i' in actor-safe context}}
  _ = x[5] // expected-error {{reference to non-actor-safe subscript 'subscript(_:)' in actor-safe context}}
  x.foo()
  // expected-error@-1 {{expression is not legal in a '@actorSafe' context}}
  // expected-error@-2 {{reference to non-actor-safe instance method 'foo()' in actor-safe context}}

  _ = y.i // expected-error {{reference to non-actor-safe property 'i' in actor-safe context}}
  _ = y[5] // expected-error {{reference to non-actor-safe subscript 'subscript(_:)' in actor-safe context}}
  y.foo()
  // expected-error@-1 {{expression is not legal in a '@actorSafe' context}}
  // expected-error@-2 {{reference to non-actor-safe instance method 'foo()' in actor-safe context}}
}

// expected-error@+1 {{protocol 'NotActorSafeObjCProtocol' cannot be marked @actorSafe}}
@actorSafe @objc protocol NotActorSafeObjCProtocol {}

// Capture diagnostics
func takesActorSafeFn<R>(_ fn: @actorSafe () throws -> R) rethrows -> R {
  return try fn()
}

func takesEscapingActorSafeFn<R>(_ fn: @actorSafe @escaping () throws -> R) rethrows -> R {
  return try fn()
}

actor class SomeActor {
  var i = 0
  actor func foo() {}
  actor func doSomething(callback: () -> Void) {}
}

struct NotCopyable {
  var ptr: UnsafePointer<Int>?
}

func foo() {
  let act = SomeActor()
  let nc = NotCopyable()

  // Non-escaping @actorSafe closures are allowed to capture anything.
  takesActorSafeFn {
    act.foo()
  }
  takesActorSafeFn { [act] in
    act.foo()
  }

  takesActorSafeFn {
    _ = nc
  }
  takesActorSafeFn { [nc] in
    _ = nc
  }

  // Escaping @actorSafe closures can only capture Copyable things in capture lists.
  takesEscapingActorSafeFn {
    act.foo() // expected-error {{escaping actor-safe closure must capture 'act' using a capture list}}
  }
  takesEscapingActorSafeFn { [act] in
    act.foo()
  }
  act.doSomething {
    act.foo() // expected-error {{escaping actor-safe closure must capture 'act' using a capture list}}
  }
  act.doSomething { [act] in
    act.foo()
  }

  takesEscapingActorSafeFn {
    _ = nc // expected-error {{escaping actor-safe closure must capture 'nc' using a capture list}}
  }
  takesEscapingActorSafeFn { [nc] in // expected-error {{captured variable 'nc' by escaping actor-safe closure must be Copyable}}
    _ = nc
  }
  act.doSomething {
    _ = nc // expected-error {{escaping actor-safe closure must capture 'nc' using a capture list}}
  }
  act.doSomething { [nc] in // expected-error {{captured variable 'nc' by escaping actor-safe closure must be Copyable}}
    _ = nc
  }
}

class OverridingActorSafe {
  @actorSafe func foo() {}
  @actorSafe func bar() {}
  func baz() {}
  @actorSafe func qux() {}
}

class OverridenActorSafe : OverridingActorSafe {
  @actorSafe override func foo() {
    nonActorSafeGlobal()
    // expected-error@-1 {{expression is not legal in a '@actorSafe' context}}
    // expected-error@-2 {{reference to non-actor-safe global function 'nonActorSafeGlobal()' in actor-safe context}}
  }
  override func bar() {
    nonActorSafeGlobal()
    // expected-error@-1 {{expression is not legal in a '@actorSafe' context}}
    // expected-error@-2 {{reference to non-actor-safe global function 'nonActorSafeGlobal()' in actor-safe context}}
  }
  @actorSafe override func baz() {
    nonActorSafeGlobal()
    // expected-error@-1 {{expression is not legal in a '@actorSafe' context}}
    // expected-error@-2 {{reference to non-actor-safe global function 'nonActorSafeGlobal()' in actor-safe context}}
  }
  @objc override dynamic func qux() {} // expected-error {{override of '@actorSafe' instance method must also be '@actorSafe'}}
}

@actorSafe
final class X3 {
  var i = 0
  func foo() {
    i += 1

    nonActorSafeGlobal()
    // expected-error@-1 {{expression is not legal in a '@actorSafe' context}}
    // expected-error@-2 {{reference to non-actor-safe global function 'nonActorSafeGlobal()' in actor-safe context}}
  }

  // Downgrade actor safety to unchecked.
  @actorSafe(unchecked)
  func bar() {
    nonActorSafeGlobal()
  }
}

struct S3 {
  func nonActorSafeFn() {}
}

@actorSafe
extension S3 {
  func foo() {
    globalVar += 1  // expected-error {{reference to non-actor-safe var 'globalVar' in actor-safe context}}
  }
}

@actorSafe
struct S4 {}
extension S4 {
  func foo() {
    globalVar += 1  // expected-error {{reference to non-actor-safe var 'globalVar' in actor-safe context}}
  }
}

do {
  let actorSafeFn = S4().foo
  takesActorSafeFn(actorSafeFn)
  takesEscapingActorSafeFn(actorSafeFn)

  let nonActorSafeFn = S3().nonActorSafeFn
  takesActorSafeFn(nonActorSafeFn) // expected-error {{non-@actorSafe function of type '() -> ()' cannot be treated as @actorSafe}}
  takesEscapingActorSafeFn(nonActorSafeFn) // expected-error {{non-@actorSafe function of type '() -> ()' cannot be treated as @actorSafe}}
}

// Test @actorSafe closure inference
do {
  let actorSafeFn: @actorSafe () -> Void = {
    globalVar += 1
    // expected-error@-1 {{actor-safe closure must capture 'globalVar' using a capture list}}
    // expected-error@-2 {{reference to non-actor-safe var 'globalVar' in actor-safe context}}
  }

  let nonActorSafeFn1: () -> Void = {
    globalVar += 1
  }

  let nonActorSafeFn2 = {
    globalVar += 1
  }

  _ = actorSafeFn; _ = nonActorSafeFn1; _ = nonActorSafeFn2
}

@actorSafe
func actorSafeTakesNonEscapingFn(_ fn: () -> Void) {}

@actorSafe
func actorSafeTakesEscapingNonActorSafeFn(_ fn: @escaping () -> Void) {}

@actorSafe
func actorSafeTakesEscapingActorSafeFn(_ fn: @escaping @actorSafe () -> Void) {}

// Test that closures can't close over actor-internal state.
actor class A1 {
  var i = 0
  func foo() {
    // Okay.
    do {
      self.i += 1
      i += 1
    }
    actorSafeTakesNonEscapingFn {
      self.i += 1
      self.foo()
      self.bar()
    }

    // Not okay.
    actorSafeTakesEscapingNonActorSafeFn {
      self.i += 1 // expected-error {{property 'i' can only be accessed directly by its own actor}}
      self.foo()  // expected-error {{actor-internal instance method 'foo()' can only be called directly by its own actor}}
      self.bar()
    }
    actorSafeTakesEscapingActorSafeFn { [self] in
      self.i += 1 // expected-error {{property 'i' can only be accessed directly by its own actor}}
      self.foo()  // expected-error {{actor-internal instance method 'foo()' can only be called directly by its own actor}}
      self.bar()
    }
  }

  actor func bar() {
    // Okay.
    do {
      self.i += 1
      i += 1
    }
    actorSafeTakesNonEscapingFn {
      self.i += 1
      self.foo()
      self.bar()
    }

    // Not okay.
    actorSafeTakesEscapingNonActorSafeFn {
      self.i += 1 // expected-error {{property 'i' can only be accessed directly by its own actor}}
      self.foo()  // expected-error {{actor-internal instance method 'foo()' can only be called directly by its own actor}}
      self.bar()
    }
    actorSafeTakesEscapingActorSafeFn { [self] in
      self.i += 1 // expected-error {{property 'i' can only be accessed directly by its own actor}}
      self.foo()  // expected-error {{actor-internal instance method 'foo()' can only be called directly by its own actor}}
      self.bar()
    }
  }
}
