// RUN: %target-typecheck-verify-swift -enable-experimental-string-processing

prefix operator /  // expected-error {{prefix operator may not contain '/'}}
prefix operator ^/ // expected-error {{prefix operator may not contain '/'}}
prefix operator /^/ // expected-error {{prefix operator may not contain '/'}}
