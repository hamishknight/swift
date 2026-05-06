// {"kind":"typecheck","signature":"swift::constraints::ExpandArrayIntoVarargsFailure::tryDropArrayBracketsFixIt(swift::Expr const*) const","signatureAssert":"Assertion failed: (!empty()), function back","signatureNext":"ExpandArrayIntoVarargsFailure::diagnoseAsError"}
// RUN: not %target-swift-frontend -typecheck %s
ExpressibleByDictionaryLiteral([]
