// RUN: %target-swift-frontend -emit-silgen -enable-sil-ownership %s | %FileCheck %s

struct Foo {
  static func staticMethod(b: Bool) {}
  func method(a: Int32) { }
  var isProperty: Bool = false
  init(anInitializer: Bool) {}
  init() {}
  subscript(isSubscript subscript: Int) -> Bool {
    get { return false }
  }
}

var isGlobal: String = ""

func declNameLiteralStatic() {
  // CHECK: string_literal utf16 "staticMethod(b:)"
  _ = #name(Foo.staticMethod)

  // CHECK: string_literal utf16 "method(a:)"
  _ = #name(Foo.method)

  // CHECK: string_literal utf16 "isProperty"
  _ = #name(Foo.isProperty)

  // CHECK: string_literal utf16 "init(anInitializer:)"
  _ = #name(Foo.init(anInitializer:))

  // CHECK: string_literal utf16 "isGlobal"
  _ = #name(isGlobal)
}

func declNameLiteralInstance() {
  let foo = Foo()

  // CHECK: string_literal utf16 "method(a:)"
  _ = #name(foo.method)

  // CHECK: string_literal utf16 "isProperty"
  _ = #name(foo.isProperty)

  // CHECK: string_literal utf16 "subscript(isSubscript:)"
  _ = #name(foo[isSubscript: 4])
}

