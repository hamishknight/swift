// RUN: %target-typecheck-verify-swift -enable-experimental-string-processing

precedencegroup P {
  associativity: left
}

// Fine.
infix operator /^/ : P
func /^/ (lhs: Int, rhs: Int) -> Int { 0 }

let i = 0 /^/ 1/^/3

prefix operator /  // expected-error {{prefix slash not allowed}}
prefix operator ^/ // expected-error {{prefix slash not allowed}}

let x = /abc/
_ = /abc/
_ = /x/.self
_ = /\//

// These unfortunately become infix `=/`. We could likely improve the diagnostic
// though.
let z=/0/
// expected-error@-1 {{type annotation missing in pattern}}
// expected-error@-2 {{consecutive statements on a line must be separated by ';'}}
// expected-error@-3 {{expected expression after unary operator}}
// expected-error@-4 {{cannot find operator '=/' in scope}}
// expected-error@-5 {{'/' is not a postfix unary operator}}
_=/0/
// expected-error@-1 {{'_' can only appear in a pattern or on the left side of an assignment}}
// expected-error@-2 {{cannot find operator '=/' in scope}}
// expected-error@-3 {{'/' is not a postfix unary operator}}

_ = /x
// expected-error@-1 {{prefix slash not allowed}}
// expected-error@-2 {{'/' is not a prefix unary operator}}

// TODO: This will become prefix '!' on a regex literal.
_ = !/x/
// expected-error@-1 {{prefix slash not allowed}}
// expected-error@-2 {{cannot find operator '!/' in scope}}
// expected-error@-3 {{'/' is not a postfix unary operator}}

_ = /x/! // expected-error {{cannot force unwrap value of non-optional type 'Regex<Substring>'}}
_ = /x/ + /y/ // expected-error {{binary operator '+' cannot be applied to two 'Regex<Substring>' operands}}

_ = /x/+/y/
// expected-error@-1 {{cannot find operator '+/' in scope}}
// expected-error@-2 {{'/' is not a postfix unary operator}}
// expected-error@-3 {{cannot find 'y' in scope}}

_ = /x/?.blah
// expected-error@-1 {{cannot use optional chaining on non-optional value of type 'Regex<Substring>'}}
// expected-error@-2 {{value of type 'Regex<Substring>' has no member 'blah'}}
_ = /x/!.blah
// expected-error@-1 {{cannot force unwrap value of non-optional type 'Regex<Substring>'}}
// expected-error@-2 {{value of type 'Regex<Substring>' has no member 'blah'}}

_ = /x /? // expected-error {{cannot use optional chaining on non-optional value of type 'Regex<Substring>'}}
  .blah // expected-error {{value of type 'Regex<Substring>' has no member 'blah'}}

_ = 0; /x / // expected-warning {{regular expression literal is unused}}

_ = /x / ? 0 : 1 // expected-error {{cannot convert value of type 'Regex<Substring>' to expected condition type 'Bool'}}
_ = .random() ? /x / : .blah // expected-error {{type 'Regex<Substring>' has no member 'blah'}}

_ = /x/ ?? /x/ // expected-warning {{left side of nil coalescing operator '??' has non-optional type 'Regex<Substring>', so the right side is never used}}
_ = /x / ?? /x / // expected-warning {{left side of nil coalescing operator '??' has non-optional type 'Regex<Substring>', so the right side is never used}}

_ = /x/??/x/ // expected-error {{'/' is not a postfix unary operator}}

_ = /x/ ... /y/ // expected-error {{referencing operator function '...' on 'Comparable' requires that 'Regex<Substring>' conform to 'Comparable'}}

_ = /x/.../y/
// expected-error@-1 {{missing whitespace between '...' and '/' operators}}
// expected-error@-2 {{'/' is not a postfix unary operator}}
// expected-error@-3 {{cannot find 'y' in scope}}

_ = /x /...
// expected-error@-1 {{unary operator '...' cannot be applied to an operand of type 'Regex<Substring>'}}
// expected-note@-2 {{overloads for '...' exist with these partially matching parameter lists}}

do {
  _ = true / false /; // expected-error {{expected expression after operator}}
}

_ = "\(/x/)"

func defaulted(x: Regex<Substring> = /x/) {}

func foo<T>(_ x: T, y: T) {}
foo(/abc/, y: /abc /)

func bar<T>(_ x: inout T) {}

// TODO: This would become an inout argument (but error because rvalue)
bar(&/x/)
// expected-error@-1 {{prefix slash not allowed}}
// expected-error@-2 {{cannot find operator '&/' in scope}}
// expected-error@-3 {{'/' is not a postfix unary operator}}

struct S {
  subscript(x: Regex<Substring>) -> Void { () }
}

func testSubscript(_ x: S) {
  x[/x/]
  x[/x /]
}

func testReturn() -> Regex<Substring> {
  if .random() {
    return /x/
  }
  return /x /
}

func testThrow() throws {
  throw /x / // expected-error {{thrown expression type 'Regex<Substring>' does not conform to 'Error'}}
}

_ = [/abc/, /abc /]
_ = [/abc/:/abc/] // expected-error {{generic struct 'Dictionary' requires that 'Regex<Substring>' conform to 'Hashable'}}
_ = [/abc/ : /abc/] // expected-error {{generic struct 'Dictionary' requires that 'Regex<Substring>' conform to 'Hashable'}}
_ = [/abc /:/abc /] // expected-error {{generic struct 'Dictionary' requires that 'Regex<Substring>' conform to 'Hashable'}}
_ = [/abc /: /abc /] // expected-error {{generic struct 'Dictionary' requires that 'Regex<Substring>' conform to 'Hashable'}}
_ = (/abc/, /abc /)
_ = ((/abc /))

_ = { /abc/ }
_ = {
  /abc/
}

let _: () -> Int = {
  0
  / 1 /
  2
}

_ = {
  0 // expected-warning {{integer literal is unused}}
  /1 / // expected-warning {{regular expression literal is unused}}
  2 // expected-warning {{integer literal is unused}}
}

// Operator chain, as a regex literal may not start with space.
_ = 2
/ 1 / .bitWidth

_ = 2
/1/ .bitWidth // expected-error {{value of type 'Regex<Substring>' has no member 'bitWidth'}}

_ = 2
/ 1 /
  .bitWidth

_ = 2
/1 /
  .bitWidth // expected-error {{value of type 'Regex<Substring>' has no member 'bitWidth'}}

let z =
/y/

// While '.' is technically an operator character, it seems more likely that
// the user hasn't written the member name yet.
_ = 0. / 1 / 2 // expected-error {{expected member name following '.'}}
_ = 0 . / 1 / 2 // expected-error {{expected member name following '.'}}

switch "" {
case /x/:
  // expected-error@-1 {{expression pattern of type 'Regex<Substring>' cannot match values of type 'String'}}
  // expected-note@-2 {{overloads for '~=' exist with these partially matching parameter lists: (Substring, String)}}
  break
case _ where /x /:
  // expected-error@-1 {{cannot convert value of type 'Regex<Substring>' to expected condition type 'Bool'}}
  break
default:
  break
}

do {} catch /x / {}
// expected-error@-1 {{expression pattern of type 'Regex<Substring>' cannot match values of type 'any Error'}}
// expected-error@-2 {{binary operator '~=' cannot be applied to two 'any Error' operands}}
// expected-warning@-3 {{'catch' block is unreachable because no errors are thrown in 'do' block}}

switch /x / {
default:
  break
}

if /x / {} // expected-error {{cannot convert value of type 'Regex<Substring>' to expected condition type 'Bool'}}
if /x /.smth {} // expected-error {{value of type 'Regex<Substring>' has no member 'smth'}}

func testGuard() {
  guard /x/ else { return } // expected-error {{cannot convert value of type 'Regex<Substring>' to expected condition type 'Bool'}}
}

for x in [0] where /x/ {} // expected-error {{cannot convert value of type 'Regex<Substring>' to expected condition type 'Bool'}}

typealias Magic<T> = T
_ = /x/ as Magic
_ = /x/ as! String // expected-warning {{cast from 'Regex<Substring>' to unrelated type 'String' always fails}}

_ = type(of: /x /)

do {
  let /x / // expected-error {{expected pattern}}
}

_ = try /x/; _ = try /x /
// expected-warning@-1 2{{no calls to throwing functions occur within 'try' expression}}

// TODO: `try?` and `try!` are currently broken.
// _ = try? /x/; _ = try? / x /
// _ = try! /x/; _ = try! / x /

_ = await /x / // expected-error {{'await' in a function that does not support concurrency}}

/x/ = 0 // expected-error {{cannot assign to value: literals are not mutable}}
/x/() // expected-error {{cannot call value of non-function type 'Regex<Substring>'}}

// TODO: We could allow this (and treat the last '/' as postfix), though it
// seems more likely the user has written a comment and is still in the middle
// of writing the characters before it.
/x//
// expected-error@-1 {{prefix slash not allowed}}
// expected-error@-2 {{'/' is not a prefix unary operator}}

/x //
// expected-error@-1 {{prefix slash not allowed}}
// expected-error@-2 {{'/' is not a prefix unary operator}}

/x/**/
// expected-error@-1 {{prefix slash not allowed}}
// expected-error@-2 {{'/' is not a prefix unary operator}}

// Make sure we continue parsing these as operators, as they are not right
// bound.
func foo(_ x: (Int, Int) -> Int, _ y: (Int, Int) -> Int) {}
foo(/, /)
foo(/,/)
foo((/), (/))

func bar<T>(_ x: (Int, Int) -> Int, _ y: T) -> Int { 0 }
_ = bar(/, 1) / 2
_ = bar(/, "(") / 2
_ = bar(/, 1) // comment

let arr: [Double] = [2, 3, 4]
_ = arr.reduce(1, /) / 3
_ = arr.reduce(1, /) + arr.reduce(1, /)

// Fine.
_ = /./

// You need to escape if you want a regex literal to start with these characters.
// TODO: Better recovery
_ = /\ /
do { _ = / / }
// expected-error@-1 2{{unary operator cannot be separated from its operand}}
// expected-error@-2 {{expected expression in assignment}}

_ = /\)/
do { _ = /)/ }
// expected-error@-1 {{expected expression after unary operator}}
// expected-error@-2 {{expected expression in assignment}}

_ = /\,/
do { _ = /,/ }
// expected-error@-1 {{expected expression after unary operator}}
// expected-error@-2 {{expected expression in assignment}}

_ = /}/
_ = /]/
_ = /:/
_ = /;/
