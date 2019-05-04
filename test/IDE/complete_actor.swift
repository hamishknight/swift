
// RUN: %target-swift-ide-test -code-completion -source-filename %s -code-completion-token=CC1 | %FileCheck %s -check-prefix=CC1
// RUN: %target-swift-ide-test -code-completion -source-filename %s -code-completion-token=CC2 | %FileCheck %s -check-prefix=CC2

import Dispatch

actor class X {
  func foo(_ x: Int) {}
  actor func bar(_ y: String) {}
  static func baz(_ z: Double) {}
}

let x = X()
x.#^CC1^#
// CC1:      Begin completions, 3 items
// CC1-NEXT: Keyword[self]/CurrNominal:          self[#X#]; name=self
// CC1-NEXT: Decl[InstanceMethod]/CurrNominal:   bar({#(y): String#})[#Void#]; name=bar(y: String)
// CC1-NEXT: Decl[InstanceMethod]/Super:         copy()[#X#]; name=copy()
// CC1-NEXT: End completions

X.#^CC2^#
// CC2:      Begin completions, 6 items
// CC2-NEXT: Keyword[self]/CurrNominal:          self[#X.Type#]; name=self
// CC2-NEXT: Keyword/CurrNominal:                Type[#X.Type#]; name=Type
// CC2-NEXT: Decl[InstanceMethod]/CurrNominal:   bar({#(self): X#})[#(String) -> Void#]; name=bar(self: X)
// CC2-NEXT: Decl[StaticMethod]/CurrNominal:     baz({#(z): Double#})[#Void#]; name=baz(z: Double)
// CC2-NEXT: Decl[Constructor]/CurrNominal:      init()[#X#]; name=init()
// CC2-NEXT: Decl[InstanceMethod]/Super:         copy({#(self): X#})[#() -> X#]; name=copy(self: X)
// CC2-NEXT: End completions
