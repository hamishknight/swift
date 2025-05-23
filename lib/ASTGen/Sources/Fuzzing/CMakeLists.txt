add_pure_swift_host_library(swiftFuzzing STATIC
  Fuzzing.swift

  SWIFT_DEPENDENCIES
    _CompilerRegexParser
    _CompilerSwiftSyntax
    _CompilerSwiftIfConfig
    _CompilerSwiftLexicalLookup
    _CompilerSwiftOperators
    _CompilerSwiftSyntaxBuilder
    _CompilerSwiftParser
    _CompilerSwiftParserDiagnostics
    _CompilerSwiftDiagnostics
)
