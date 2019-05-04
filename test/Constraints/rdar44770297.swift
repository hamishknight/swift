// RUN: %target-typecheck-verify-swift

protocol P {
  associatedtype A
}

func foo<T: P>(_: () throws -> T) -> T.A? { 
  fatalError()
}

let _ = foo() {fatalError()} & nil // expected-error {{invalid conversion from throwing function of type '() throws -> _' to non-throwing function type '() -> _'}}
