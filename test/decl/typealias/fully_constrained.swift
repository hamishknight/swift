// RUN: %target-typecheck-verify-swift

struct OtherGeneric<U> {}

struct Generic<T> {
  typealias NonGeneric = Int where T == Int
  typealias FakeGeneric = T where T == Int

  typealias Unbound = OtherGeneric where T == Int
  typealias Generic<U> = OtherGeneric<U> where T == Int

  typealias UnconstrainedNonGeneric = Int
  typealias UnconstrainedFake<U> = Int
  typealias UnconstrainedUnbound = OtherGeneric
  typealias UnconstrainedGeneric<U> = OtherGeneric<U>
}

extension Generic where T == Int {
  typealias NonGenericInExtension = Int
  typealias FakeGenericInExtension = T

  typealias UnboundInExtension = OtherGeneric
  typealias GenericInExtension<U> = OtherGeneric<U>
}

func use(_: Generic.NonGeneric,
         _: Generic.FakeGeneric,
         _: Generic.Unbound<String>,
         _: Generic.Generic<String>,
         _: Generic.NonGenericInExtension,
         _: Generic.UnboundInExtension<String>,
         _: Generic.GenericInExtension<String>,
         _: Generic.UnconstrainedNonGeneric,
         _: Generic.UnconstrainedFake<String>,
         _: Generic.UnconstrainedUnbound<String>,
         _: Generic.UnconstrainedGeneric<String>) {

  // FIXME: Get these working too
#if false
  let _ = Generic.UnconstrainedNonGeneric.self
  let _ = Generic.UnconstrainedFake<String>.self
  let _ = Generic.UnconstrainedUnbound<String>.self
  let _ = Generic.UnconstrainedGeneric<String>.self

  let _: Generic.UnconstrainedGeneric = OtherGeneric<String>()
#endif

  let _ = Generic.NonGeneric.self
  let _ = Generic.FakeGeneric.self

  let _ = Generic.NonGenericInExtension.self
  let _ = Generic.FakeGenericInExtension.self

  let _ = Generic.Unbound<String>.self
  let _ = Generic.UnboundInExtension<String>.self

  let _ = Generic.Generic<String>.self
  let _ = Generic.GenericInExtension<String>.self

  let _: Generic.NonGeneric = 123
  let _: Generic.FakeGeneric = 123
  let _: Generic.NonGenericInExtension = 123
  let _: Generic.FakeGenericInExtension = 123

  let _: Generic.Unbound = OtherGeneric<String>()
  let _: Generic.Generic = OtherGeneric<String>()

  let _: Generic.UnboundInExtension = OtherGeneric<String>()
  let _: Generic.GenericInExtension = OtherGeneric<String>()

  let _: Generic.UnconstrainedNonGeneric = 0
  let _: Generic.UnconstrainedFake<String> = 0
  let _: Generic.UnconstrainedUnbound = OtherGeneric<String>()
}

struct Use {
  let a1: Generic.NonGeneric
  let b1: Generic.FakeGeneric
  let c1: Generic.Unbound<String>
  let d1: Generic.Generic<String>
  let a2: Generic.NonGenericInExtension
  let b2: Generic.FakeGenericInExtension
  let c2: Generic.UnboundInExtension<String>
  let d2: Generic.GenericInExtension<String>
  let e1: Generic.UnconstrainedNonGeneric
  let e2: Generic.UnconstrainedFake<String>
  let e3: Generic.UnconstrainedUnbound<String>
  let e4: Generic.UnconstrainedGeneric<String>
}

extension Generic.NonGeneric {}
extension Generic.Unbound {}
extension Generic.Generic {}

extension Generic.NonGenericInExtension {}
extension Generic.UnboundInExtension {}
extension Generic.GenericInExtension {}

extension Generic.UnconstrainedNonGeneric {}
extension Generic.UnconstrainedFake<String> {}
extension Generic.UnconstrainedUnbound<String> {}
extension Generic.UnconstrainedGeneric<String> {}

protocol P {}

func testPlaceholderAndConformance() {
  struct S<T, U> {
    typealias X = Int where T == String, U: P
  }
  func foo<U>(_ x: U) {
    // FIXME: Misleading diagnostic
    _ = S<_, U>.X.self // expected-error {{struct 'Int' requires that 'U' conform to 'P'}}

    // FIXME
    // let _: S<_, U>.X = 0
  }
  func bar<U: P>(_ x: U) {
    _ = S<_, U>.X.self

    // FIXME
    // let _: S<_, U>.X = 0
  }
}
