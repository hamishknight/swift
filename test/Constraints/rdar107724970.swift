// RUN: %target-typecheck-verify-swift

// rdar://107724970 – Make sure we don't crash.
enum E {
  case e(Int)
}
func foo(_ x: E) {
  let fn = {
    switch x {
    case E.e(_, _): // expected-error {{extra argument in call}}
      break
    }
  }
}
