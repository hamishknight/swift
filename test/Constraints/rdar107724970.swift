// RUN: %target-typecheck-verify-swift

// rdar://107724970 – Make sure we don't crash.
enum E {
  case e(Int)
}
func foo(_ x: E) {
  let fn = { // expected-error {{unable to infer closure type in the current context}}
    switch x {
    case E.e(_, _):
      break
    }
  }
}
