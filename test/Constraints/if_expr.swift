// RUN: %target-typecheck-verify-swift -enable-experimental-feature StatementExpressions

enum E {
  case e
  case f
  case g(Int)
}

func testDotSyntax1() -> E {
  if .random() { .e } else { .f }
}
func testDotSyntax2() -> E? {
  if .random() { .e } else { .f }
}
func testDotSyntax3() -> E? {
  if .random() { .e } else { .none }
}
func testDotSyntax4() -> Int {
  let i = if .random() { 0 } else { .random() }
  // expected-error@-1 {{cannot infer contextual base in reference to member 'random'}}

  return i
}

let testVar1: E = if .random() { .e } else { .f }
let testVar2: E? = if .random() { .e } else { .f }
let testVar3: E? = if .random() { .e } else { .none }
let testVar4: E? = if .random() { nil } else { .e }

let testVar5 = if .random() { 0 } else { 1.0 }
// expected-error@-1 {{branches have mismatching types 'Int' and 'Double'}}

let testVar6: Double = if .random() { 0 } else { 1.0 }

let testVar7: Double = if .random() { 0 + 1 } else if .random() { 1.0 + 3 } else { 9 + 0.0 }

let testContextualMismatch1: String = if .random() { 1 } else { "" }
// expected-error@-1 {{cannot convert value of type 'Int' to specified type 'String'}}

let testContextualMismatch2: String = if .random() { 1 } else { 2 }
// expected-error@-1 {{cannot convert value of type 'Int' to specified type 'String'}}

// TODO: Should we coalesce all of these into a single diagnostic?
let testMismatch1 = if .random() { 1 } else if .random() { "" } else { 0.0 }
// expected-error@-1 {{branches have mismatching types 'Int' and 'String'}}
// expected-error@-2 {{branches have mismatching types 'Double' and 'String'}}

func testMismatch2() -> Double {
  let x = if .random() {
    return 0
  } else if .random() {
    0  // expected-error {{branches have mismatching types 'Int' and 'Double'}}
  } else {
    1.0
  }
  return x
}
func testOptionalBinding1(_ x: Int?) -> Int {
  if let x = x { x } else { 0 }
}
func testOptionalBinding2(_ x: Int?, _ y: Int?) -> Int {
  if let x = x, let y = y { x + y } else { 0 }
}
func testPatternBinding2(_ e: E) -> Int {
  if case .g(let i) = e { i } else { 0 }
}

func testTernary() -> Int {
  if .random() { .random() ? 1 : 0 } else { .random() ? 3 : 2 }
}

func testReturn() -> Int {
  if .random() {
    return 0
  } else {
    1
  }
}

func testFatalError1() -> Int {
  if .random() {
    fatalError()
  } else {
    0
  }
}

func testReturnMismatch() {
  let _ = if .random() {
    return 1 // expected-error {{unexpected non-void return value in void function}}
    // expected-note@-1 {{did you mean to add a return type?}}
  } else {
    0
  }
}

func testOptionalGeneric() {
  func bar<T>(_ fn: () -> T?) -> T? { fn() }
  bar {
    if .random() {
      ()
    } else {
      ()
    }
  }
}

func testNestedOptional() -> Int? {
  if .random() {
    1
  } else {
    if .random() {
      1
    } else {
      nil
    }
  }
}

let neverVar = if .random() { fatalError() } else { fatalError() }
// expected-warning@-1 {{constant 'neverVar' inferred to have type 'Never'}}
// expected-note@-2 {{add an explicit type annotation to silence this warning}}

func testNonVoidToVoid() {
  if .random() { 0 } else { 1 } // expected-warning 2{{integer literal is unused}}
}

func testVoidConversion() {
  func foo(_ fn: () -> Void) {}
  foo {
    if .random() {
      0
    } else {
      0
    }
  }
  func bar<T>(_ fn: () -> T) {}
  bar {
    if .random() {
      0
    } else {
      0
    }
  }
  bar {
    if .random() {
      ()
    } else {
      0
    }
  }
}

func uninferableNil() {
  let _ = if .random() { nil } else { 2.0 } // expected-error {{'nil' requires a contextual type}}
}

func testAssignment() {
  var d: Double = if .random() { 0 } else { 1.0 }
  d = if .random() { 0 } else { 1 }
  _ = d
}

struct TestBadReturn {
  var y = if .random() { return } else { 0 } // expected-error {{return invalid outside of a func}}
}

// MARK: Opaque result types

protocol P {}
extension Int : P {}

@available(macOS 10.15, iOS 13.0, tvOS 13.0, watchOS 6.0, *)
func testOpaqueReturn1() -> some P {
  if .random() {
    0
  } else {
    1
  }
}

@available(macOS 10.15, iOS 13.0, tvOS 13.0, watchOS 6.0, *)
func testOpaqueReturn2() -> some P {
  if .random() {
    0
  } else {
    fatalError()
  }
}

// MARK: Result builders

enum Either<T, U> {
  case first(T), second(U)
}

@resultBuilder
struct Builder {
  static func buildBlock<T>(_ x: T) -> T { x }
  static func buildBlock<T, U>(_ x: T, _ y: U) -> (T, U) { (x, y) }

  static func buildEither<T, U>(first x: T) -> Either<T, U> { .first(x) }
  static func buildEither<T, U>(second x: U) -> Either<T, U> { .second(x) }

  static func buildExpression(_ x: Double) -> Double { x }
  static func buildExpression<T>(_ x: T) -> T { x }
}

@Builder
func singleExprBuilder() -> Either<String, Int> {
  if .random() {
    ""
  } else {
    1
  }
}

@Builder
func builderStaticMember() -> (Either<String, Int>, Double) {
  if .random() {
    ""
  } else {
    1
  }
  .pi // This becomes a static member ref, not a member on an if expression.
}

@Builder
func builderNotPostfix() -> (Either<String, Int>, Bool) {
  if .random() { "" } else { 1 } !.random() // expected-error {{consecutive statements on a line must be separated by ';'}}
}

@Builder
func builderWithBinding() -> Either<String, Int> {
  // Make sure the binding gets type-checked as an if expression, but the
  // other if block gets type-checked as a stmt.
  let str = if .random() { "a" } else { "b" }
  if .random() {
    str
  } else {
    1
  }
}

func builderInClosure() {
  func build(@Builder _ fn: () -> Either<String, Int>) {}
  build {
    if .random() {
      ""
    } else {
      1
    }
  }
}
