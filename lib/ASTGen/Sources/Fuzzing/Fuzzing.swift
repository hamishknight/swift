//
//  Fuzzing.swift
//  Swift
//
//  Created by Hamish on 01/06/2025.
//

@_spi(RawSyntax)
import SwiftSyntax
import SwiftParser
import Foundation

extension RawTokenKind {
  static var lastKind: Self { .wildcard }

  static let allCases = (0...lastKind.rawValue).map { Self(rawValue: $0)! }
  static let weightTable = allCases.flatMap {
    Array(repeating: $0, count: $0.weight)
  }

  var weight: Int {
    switch self {
    case .backtick, .colonColon:
      // Rare grammar
      5
    case .arrow, .backslash, .atSign, .ellipsis, .leftAngle, .rightAngle,
        .prefixAmpersand, .wildcard:
      // Infrequent grammar
      10
    case .leftSquare, .rightSquare, .binaryOperator, .equal, .prefixOperator, .infixQuestionMark,
        .postfixOperator, .exclamationMark, .postfixQuestionMark:
      20
    case .leftBrace, .rightBrace, .leftParen, .rightParen:
      // Slightly more frequent
      30
    case .colon, .comma, .period, .keyword:
      // More frequent grammer
      40
    case .floatLiteral, .integerLiteral:
      // Expressions
      20
    case .identifier:
      // Common grammar
      100
    case .dollarIdentifier, .singleQuote, .stringQuote, .stringSegment,
        .multilineStringQuote, .semicolon, .regexSlash, .regexLiteralPattern,
        .regexPoundDelimiter, .rawStringPoundDelimiter:
      2
    case .shebang, .unknown, .endOfFile:
      // Boring tokens
      0
    case .pound, .poundAvailable, .poundElse, .poundElseif, .poundEndif,
        .poundIf, .poundSourceLocation, .poundUnavailable:
      1
    }
  }

  init<R: RandomNumberGenerator>(random: inout R) {
    self = Self.weightTable.randomElement(using: &random)!
  }
}

extension Keyword {
  static var lastKind: Self { .yield }

  static let allCases = (0...lastKind.rawValue).map { Keyword(rawValue: $0)! }
  static let weightTable = allCases.flatMap {
    Array(repeating: $0, count: $0.weight)
  }

  var weight: Int {
    switch self {
    case .fallthrough, .operator, .precedencegroup, .super, .internal,
        .private, .fileprivate, .public, .defer, .default:
      1
    case .Any, .as, .is, .continue, .break, .deinit, .rethrows, .inout, .struct,
        .subscript, .class, .enum, .import, .associatedtype, .protocol, .unsafe:
      5
    case .repeat, .each, .typealias:
      10
    case .in, .where, .mutating, .try, .nil, .throws, .static, .false, .true:
      15
    case .if, .do, .switch, .throw, .while, .for, .catch, .case, .else,
        .guard, .extension, .`init`, .Self:
      20
    case .`self`:
      50
    case .func:
      75
    case .let, .var, .return:
      100
    default:
      1
    }
  }

  init<R: RandomNumberGenerator>(random: inout R) {
    self = Keyword.weightTable.randomElement(using: &random)!
  }
}

extension TokenKind {
  init<R: RandomNumberGenerator>(
    from source: inout R,
    getIdentifier: (inout R) -> String,
    getOperator: (inout R) -> String
  ) {
    func getTextFor(_ kind: RawTokenKind) -> String {
      switch kind {
      case .dollarIdentifier:
        return "$" + getIdentifier(&source)
      case .identifier:
        return getIdentifier(&source)
      case .integerLiteral:
        return "0"
      case .floatLiteral:
        return "0.0"
      case .keyword:
        return String(syntaxText: Keyword(random: &source).defaultText)
      case .prefixOperator, .binaryOperator, .postfixOperator:
        return getOperator(&source)
      case .rawStringPoundDelimiter, .regexPoundDelimiter:
        return "#"
      case .regexLiteralPattern, .stringSegment:
        return "x"
      default:
        fatalError("unexpected token \(kind)")
      }
    }
    let kind = RawTokenKind(random: &source)
    let text = kind.defaultText.map(String.init) ?? getTextFor(kind)
    self = TokenKind.fromRaw(kind: kind, text: text)
  }
}

struct KnownSyntaxNames {
  private(set) var names: Set<String> = []
  private(set) var operators: Set<String> = []

  mutating func recordName(_ name: String) {
    self.names.insert(name)
  }

  mutating func recordLabel(_ label: String) {
    self.names.insert(label)
  }

  mutating func recordOperator(_ op: String) {
    self.operators.insert(op)
  }

  func getFreshName() -> String {
    var i = 0
    var result: String { "x\(i)" }
    while names.contains(result) { i += 1 }
    return result
  }
}

fileprivate final class SyntaxNameCollector: SyntaxVisitor {
  var names = KnownSyntaxNames()

  init() {
    super.init(viewMode: .sourceAccurate)
  }

  func recordIdentifier(_ token: TokenSyntax) {
    switch token.rawTokenKind {
    case .identifier:
      names.recordName(token.text)
    case .binaryOperator, .prefixOperator, .postfixOperator:
      names.recordOperator(token.text)
    default:
      break
    }
  }

  override func visit(_ token: TokenSyntax) -> SyntaxVisitorContinueKind {
    if let parent = token.parent,
       parent.is(FunctionParameterSyntax.self) ||
        parent.is(LabeledExprSyntax.self) {
      // Already visited.
      return .visitChildren
    }
    recordIdentifier(token)
    return .visitChildren
  }

  override func visit(_ node: FunctionParameterSyntax) -> SyntaxVisitorContinueKind {
    names.recordLabel(node.firstName.text)
    recordIdentifier(node.secondName ?? node.firstName)
    return .visitChildren
  }


  override func visit(_ node: LabeledExprSyntax) -> SyntaxVisitorContinueKind {
    if let label = node.label {
      names.recordLabel(label.text)
    }
    return .visitChildren
  }

  static func collect(_ trees: [SourceFileSyntax]) -> KnownSyntaxNames {
    let visitor = Self()
    for tree in trees {
      visitor.walk(tree)
    }
    return visitor.names
  }
}

final class FuzzingContext {
  private static let maxInputs = 1000
  private var cachedInputs: [FuzzerInput] = []
  private var maxLength: Int

  init(maxLength: Int) {
    self.maxLength = maxLength
  }

  func getOrCreateInput(from text: String) -> FuzzerInput {
    for idx in cachedInputs.indices where cachedInputs[idx].originalText == text {
      for i in (0 ..< idx).reversed() {
        cachedInputs.swapAt(i, i + 1)
      }
      return cachedInputs.first!
    }
    let cached = FuzzerInput(from: text)
    cachedInputs.insert(cached, at: 0)
    if cachedInputs.count > Self.maxInputs {
      cachedInputs.removeSubrange(Self.maxInputs...)
    }
    return cached
  }

  func next(from text: String) -> String {
    getOrCreateInput(from: text).randomlyMutating(maxLength: maxLength)
  }
}

struct SourceKitRequest: Codable, Hashable {
  enum Kind: String, Codable {
    case complete, cursorInfo

    init?(index: Int) {
      let result: Self? = switch index {
      case 0:
          .complete
      case 1:
          .cursorInfo
      default: nil
      }
      guard let result else { return nil }
      self = result
    }

    var index: Int {
      switch self {
      case .complete:
        0
      case .cursorInfo:
        1
      }
    }
  }
  var kind: Kind
  var offset: Int
  var fileIdx: Int
}

let defaultJSONEncoder = {
  let encoder = JSONEncoder()
  encoder.outputFormatting = .sortedKeys
  return encoder
}()

struct FuzzHeader: Codable {
  var splits: [Int] = []
  var sourceKitRequests: [SourceKitRequest] = []
  var extraArgs: [String] = []
  var extraOpts: [String: Any] = [:]

  init() {}

  init?(from str: some StringProtocol) {
    guard let result = try? JSONDecoder().decode(
      FuzzHeader.self, from: Data(str.utf8)
    ) else {
      return nil
    }
    self = result
  }

  struct CodingKeys: CodingKey, Hashable {
    var stringValue: String
    var intValue: Int? { nil }

    init?(stringValue: String) {
      self.stringValue = stringValue
    }
    init?(intValue: Int) {
      return nil
    }

    init(_ str: String) {
      self.stringValue = str
    }

    static let splits = Self("splits")
    static let sourceKitRequests = Self("sourceKitRequests")
    static let extraArgs = Self("extraArgs")
  }

  init(from decoder: any Decoder) throws {
    let container = try decoder.container(keyedBy: CodingKeys.self)
    self.splits = try container.decodeIfPresent([Int].self, forKey: .splits) ?? []
    self.sourceKitRequests = try container.decodeIfPresent([SourceKitRequest].self, forKey: .sourceKitRequests) ?? []
    self.extraArgs = try container.decodeIfPresent([String].self, forKey: .extraArgs) ?? []
    for extraKey in container.allKeys where extraKey != .splits && extraKey != .sourceKitRequests && extraKey != .extraArgs {
      if let str = try? container.decode(String.self, forKey: extraKey) {
        extraOpts[extraKey.stringValue] = str
      } else if let dict = try? container.decode([String:String].self, forKey: extraKey) {
        extraOpts[extraKey.stringValue] = dict
      }
    }
  }

  func encode(to encoder: any Encoder) throws {
    var container = encoder.container(keyedBy: CodingKeys.self)
    try container.encode(splits, forKey: .splits)
    try container.encode(sourceKitRequests, forKey: .sourceKitRequests)
    try container.encode(extraArgs, forKey: .extraArgs)
    for (key, value) in extraOpts {
      if let str = value as? String {
        try container.encode(str, forKey: CodingKeys(key))
      } else if let dict = value as? [String: String] {
        try container.encode(dict, forKey: CodingKeys(key))
      }
    }
  }

  var asTrivia: Trivia {
    [
      .lineComment(
        "// " + String(
          decoding: try! defaultJSONEncoder.encode(self), as: UTF8.self
        )
      ),
      .newlines(1)
    ]
  }
}

final class FuzzerInput {
  let header: FuzzHeader
  let headerOffset: Int
  let trees: [SourceFileSyntax]
  let knownNames: KnownSyntaxNames
  let originalText: String
  let extraArgs: UnsafeMutableBufferPointer<UnsafeMutablePointer<CChar>>

  init(from text: String) {
    let newText: String
    (self.trees, self.header, newText) = Self.parse(text)
    self.headerOffset = text.utf8.count - newText.utf8.count
    self.knownNames = SyntaxNameCollector.collect(trees)
    self.originalText = text
    self.extraArgs = .allocate(capacity: header.extraArgs.count)
    for (idx, arg) in header.extraArgs.enumerated() {
      self.extraArgs.initializeElement(at: idx, to: arg.withCString { strdup($0) })
    }
  }
  deinit {
    for arg in extraArgs {
      free(arg)
    }
    extraArgs.deallocate()
  }
}

extension FuzzerInput {
  private static func takeFuzzHeader(_ text: String) -> (FuzzHeader, String) {
    guard text.hasPrefix("// {"), let newline = text.firstIndex(of: "\n"),
          let header = FuzzHeader(from: text.dropFirst(3)[..<newline]) else {
      return (FuzzHeader(), text)
    }
    return (header, String(text[text.index(after: newline)...]))
  }

  private static func getBufferSegments(
    _ splits: [Int], range: Range<Int>
  ) -> [Range<Int>] {
    let splitOffsets = [range.lowerBound] + splits.map { $0 + range.lowerBound } + [range.upperBound]
    return zip(splitOffsets, splitOffsets.dropFirst()).map { $0 ..< $1 }
  }

  var bufferSegments: [Range<Int>] {
    Self.getBufferSegments(
      header.splits, range: headerOffset ..< originalText.utf8.count
    )
  }

  private static func parse(
    _ text: String
  ) -> ([SourceFileSyntax], FuzzHeader, String) {
    let (header, text) = takeFuzzHeader(text)
    var subtrees: [Substring] = []
    let textUTF8 = text.utf8
    for range in getBufferSegments(header.splits, range: 0 ..< textUTF8.count) {
      // Would be quadratic if splits were proportional to length, but that's
      // currently not the case.
      let start = textUTF8.index(textUTF8.startIndex, offsetBy: range.lowerBound)
      let end = textUTF8.index(textUTF8.startIndex, offsetBy: range.upperBound)
      subtrees.append(text[start ..< end])
    }
    let trees = subtrees.map {
      var parser = Parser(String($0))
      return SourceFileSyntax.parse(from: &parser)
    }
    return (trees, header, text)
  }

  func getRandomName<R: RandomNumberGenerator>(_ random: inout R) -> String {
    // 10% unique, 90% existing.
    if random.next(upperBound: UInt8(10)) == 0 {
      return knownNames.getFreshName()
    } else {
      let names = Array(knownNames.names)
      return names.randomElement(using: &random) ?? knownNames.getFreshName()
    }
  }

  func getRandomOperator<R: RandomNumberGenerator>(_ random: inout R) -> String {
    let operators = Array(knownNames.operators)
    return operators.randomElement(using: &random) ?? ["+", "-", "*", "??"].randomElement()!
  }

  func getRandomTrivia<R: RandomNumberGenerator>(
    _ random: inout R
  ) -> (leading: Trivia, trailing: Trivia) {
    let choices: [(Trivia, Trivia)] = [
      ([], []),
      (.space, .space),
      (.space, .space),
      (.space, .space),
      (.space, .space),
      (.space, .space),
      (.space, []),
      (.space, []),
      (.space, []),
      (.space, []),
      (.space, []),
      ([], .space),
      ([], .space),
      ([], .space),
      ([], .space),
      ([], .space),
      (.newline, []),
      (.newline, []),
      (.newline, []),
    ]
    return choices.randomElement(using: &random)!
  }

  func getRandomToken<R: RandomNumberGenerator>(_ random: inout R) -> TokenSyntax {
    let kind = TokenKind(
      from: &random,
      getIdentifier: getRandomName,
      getOperator: getRandomOperator
    )
    let (leading, trailing) = getRandomTrivia(&random)
    return TokenSyntax(
      kind, leadingTrivia: leading, trailingTrivia: trailing, presence: .present
    )
  }

  private func getTokenOffsets(_ tokens: [TokenSyntax]) -> [Int] {
    var seenOffsets: Set<Int> = []
    var offsets: [Int] = []
    var preOffset = 0
    for token in tokens {
      guard token.tokenKind != .endOfFile else { break }
      let totalLen = token.totalLength.utf8Length
      let start = token.leadingTriviaLength.utf8Length
      let end = totalLen - token.trailingTriviaLength.utf8Length
      for offset in [preOffset + start, preOffset + end] {
        guard seenOffsets.insert(offset).inserted else { continue }
        offsets.append(offset)
      }
      preOffset += totalLen
    }
    return offsets
  }

  private func getSourceKitRequests(
    offsets: [[Int]], using random: inout some RandomNumberGenerator
  ) -> [SourceKitRequest] {
    let joinedOffsets = offsets.flatMap {$0}
    if joinedOffsets.isEmpty {
      return []
    }
    // Aim to do an average of 1 request per 100 offsets, 10 reqs max.
    let tokenMul = 100
    let maxReqs = 10
    let precisionMul = 1_000_000
    let averageMul = max(tokenMul, joinedOffsets.count)
    let probMul = (precisionMul * (averageMul - tokenMul)) / averageMul

    var selected: Set<SourceKitRequest> = []
    repeat {
      while selected.count < joinedOffsets.count {
        let fileIdx = offsets.indices.randomElement(using: &random)!
        let newOffset = offsets[fileIdx].randomElement(using: &random)!
        let req = SourceKitRequest(
          kind: .complete, offset: newOffset, fileIdx: fileIdx
        )
        if selected.insert(req).inserted { break }
      }
    } while selected.count < maxReqs &&
              Int.random(in: 0 ..< precisionMul, using: &random) < probMul

    return selected.sorted(by: { ($0.fileIdx, $0.offset) < ($1.fileIdx, $1.offset) })
  }

  private func getRandomExtraArgs(
    using random: inout some RandomNumberGenerator
  ) -> [String] {
    // 70% Swift 6, 25% Swift 5, 5% Swift 4
    let langMode = switch Int.random(in: 0 ..< 100, using: &random) {
    case 0 ..< 70: 6
    case 70 ..< 95: 5
    default: 4
    }
    var result: [String] = []
    result += ["-language-mode", "\(langMode)"]
    result.append("-experimental-allow-module-with-compiler-errors")
    // 50% change to emit each supplementary output.
    if .random(using: &random) {
      result += ["-emit-module-path", "/dev/null"]
    }
    if .random(using: &random) {
      result += [
        "-enable-library-evolution", "-emit-module-interface-path", "/dev/null"
      ]
      if .random(using: &random) {
        result += ["-emit-private-module-interface-path", "/dev/null"]
      }
    }
//    if .random(using: &random) {
//      result += ["-index-store-path", "%t"]
//    }
    if .random(using: &random) {
      result += [
        "-cxx-interoperability-mode=default",
        "-emit-clang-header-min-access", "internal",
        "-emit-clang-header-path", "/dev/null"
      ]
    }
    return result
  }

  private func getNewHeader(
    tokens: [[TokenSyntax]],
    using random: inout some RandomNumberGenerator
  ) -> FuzzHeader {
    var header = self.header
    let allOffsets = tokens.map(getTokenOffsets)

    // 1% chance of splitting or joining file.
    if Int.random(in: 0 ..< 100, using: &random) == 0 {
      if tokens.count == 1, let offset = allOffsets.first!.randomElement(using: &random) {
        header.splits = [offset]
      } else {
        header.splits = []
      }
      // For now just clear the SourceKit requests.
      header.sourceKitRequests = []
      return header
    } else {
      // Re-adjust the split offsets after mutation.
      var lastOffset = 0
      header.splits = []
      for offsets in allOffsets.dropLast() {
        let last = offsets.last ?? 0
        header.splits.append(lastOffset + last)
        lastOffset = last
      }
    }
    header.sourceKitRequests = getSourceKitRequests(
      offsets: allOffsets, using: &random
    )
    header.extraArgs = getRandomExtraArgs(using: &random)
    return header
  }

  private func requireSpaceBetween(_ lhs: TokenSyntax, _ rhs: TokenSyntax) -> Bool {
    guard lhs.trailingTrivia.isEmpty && rhs.leadingTrivia.isEmpty else {
      return false
    }
    func needsSpace(_ kind: RawTokenKind) -> Bool {
      switch kind {
      case .identifier, .keyword, .stringSegment, .regexLiteralPattern,
          .integerLiteral, .floatLiteral:
        return true
      default:
        return false
      }
    }
    return needsSpace(lhs.rawTokenKind) && needsSpace(rhs.rawTokenKind)
  }

  private func randomlyMutatingTokens<R: RandomNumberGenerator>(
    _ tokens: inout [TokenSyntax], using random: inout R, maxLength: Int
  ) {
    tokens.removeAll(where: { $0.tokenKind == .endOfFile })

    // Aim to do an average of 1 mutation per 100 tokens, 20 mutations max.
    let tokenMul = 100
    let maxMutations = 20
    let precisionMul = 1_000_000
    let averageMul = max(tokenMul, tokens.count)
    let probMul = (precisionMul * (averageMul - tokenMul)) / averageMul

    // Track the number of bytes used to ensure we don't go past the maximum.
    var numBytes = originalText.utf8.count

    var numMutations = 0
    repeat {
      // 70% insert, 30% remove
      if numBytes < maxLength,
         tokens.count <= 1 || Int.random(in: 0 ..< 100, using: &random) < 70 {
        let idx = Int.random(in: 0 ... tokens.count, using: &random)
        var tok = getRandomToken(&random)
        if idx > 0, requireSpaceBetween(tokens[idx - 1], tok) {
          tok = tok.with(\.leadingTrivia, .space)
        }
        if idx < tokens.count, requireSpaceBetween(tok, tokens[idx]) {
          tok = tok.with(\.trailingTrivia, .space)
        }
//        print("=== \(numBytes) =============== insert '\(tok.text)' ====================")
        numBytes += tok.totalLength.utf8Length
        tokens.insert(tok, at: idx)
      } else if tokens.count > 1 {
        let idx = Int.random(in: tokens.indices, using: &random)
        if idx > 0 && idx + 1 < tokens.count,
           requireSpaceBetween(tokens[idx - 1], tokens[idx + 1]) {
          tokens[idx - 1] = tokens[idx - 1].with(\.trailingTrivia, .space)
        }
//        print("=== \(numBytes) =============== remove at \(idx) =======================")
        numBytes -= tokens[idx].totalLength.utf8Length
        tokens.remove(at: idx)
      } else {
        break
      }
      numMutations += 1
    } while numBytes > maxLength || (numMutations < maxMutations &&
              Int.random(in: 0 ..< precisionMul, using: &random) < probMul)
  }

  func randomlyMutating(maxLength: Int) -> String {
    var allTokens = trees.map { Array($0.tokens(viewMode: .sourceAccurate)) }
    var random = SystemRandomNumberGenerator()

    // Strip leading trivia from the first token in the first tree. This ensures
    // we get rid of an old fuzz header and other uninteresting trivia that
    // is usually written at the top of fuzz inputs, e.g RUN lines.
    if !allTokens[0].isEmpty {
      allTokens[0][0].leadingTrivia = []
    }

    for idx in allTokens.indices {
      randomlyMutatingTokens(
        &allTokens[idx], using: &random, maxLength: maxLength
      )
    }

    // Strip leading trivia again. `randomlyMutatingTokens` is guaranteed to
    // produce a non-empty output.
    allTokens[0][0].leadingTrivia = []

    var result = ""

    let header = getNewHeader(tokens: allTokens, using: &random)
    header.asTrivia.write(to: &result)

    for tokens in allTokens {
      for token in tokens {
        token.write(to: &result)
      }
    }
    if result.last != "\n" {
      result.append("\n")
    }
    return result
  }
}

@_cdecl("swift_Fuzzing_getFuzzerInput")
public func getFuzzerInput(
  _ inputPtr: UnsafePointer<CChar>, _ inputCount: Int,
  _ rawContext: UnsafeMutableRawPointer
) -> UnsafeMutableRawPointer {
  let context = Unmanaged<FuzzingContext>.fromOpaque(rawContext).takeUnretainedValue()
  let inputBuffer = UnsafeRawBufferPointer(start: inputPtr, count: inputCount)
  let input = context.getOrCreateInput(from: String(decoding: inputBuffer, as: UTF8.self))
  // We can pass unretained since it's cached in the context, in principle it
  // could be evicted but that only ought to happen across inputs, which it
  // shouldn't be persisted across.
  return Unmanaged.passUnretained(input).toOpaque()
}

@_cdecl("swift_Fuzzing_getNumBuffers")
public func swift_Fuzzing_getNumBuffers(
  _ rawInput: UnsafeMutableRawPointer
) -> Int {
  let input = Unmanaged<FuzzerInput>.fromOpaque(rawInput).takeUnretainedValue()
  return input.trees.count
}

@_cdecl("swift_Fuzzing_getBuffer")
public func swift_Fuzzing_getBuffer(
  _ rawInput: UnsafeMutableRawPointer, _ inputPtr: UnsafePointer<CChar>,
  _ index: Int, _ outLen: UnsafeMutablePointer<Int>
) -> UnsafePointer<CChar> {
  let input = Unmanaged<FuzzerInput>.fromOpaque(rawInput).takeUnretainedValue()
  let r = input.bufferSegments[index]
  outLen.pointee = r.upperBound - r.lowerBound
  return inputPtr + r.lowerBound
}

@_cdecl("swift_Fuzzing_getNumSourceKitRequests")
public func swift_Fuzzing_getNumSourceKitRequests(
  _ rawInput: UnsafeMutableRawPointer
) -> Int {
  let input = Unmanaged<FuzzerInput>.fromOpaque(rawInput).takeUnretainedValue()
  return input.header.sourceKitRequests.count
}

@_cdecl("swift_Fuzzing_getSourceKitRequest")
public func swift_Fuzzing_getSourceKitRequest(
  _ rawInput: UnsafeMutableRawPointer, _ index: Int,
  _ outKind: UnsafeMutablePointer<Int>, _ outOffset: UnsafeMutablePointer<Int>,
  _ outBufferIdx: UnsafeMutablePointer<Int>
) {
  let input = Unmanaged<FuzzerInput>.fromOpaque(rawInput).takeUnretainedValue()
  let req = input.header.sourceKitRequests[index]
  outKind.pointee = req.kind.index
  outOffset.pointee = req.offset
  outBufferIdx.pointee = req.fileIdx
}

@_cdecl("swift_Fuzzing_getExtraArgs")
public func swift_Fuzzing_getExtraArgs(
  _ rawInput: UnsafeMutableRawPointer, _ outCount: UnsafeMutablePointer<Int>
) -> UnsafePointer<UnsafeMutablePointer<CChar>> {
  let input = Unmanaged<FuzzerInput>.fromOpaque(rawInput).takeUnretainedValue()
  outCount.pointee = input.extraArgs.count
  return .init(input.extraArgs.baseAddress!)
}

@_cdecl("swift_Fuzzing_createFuzzingContext")
public func swift_Fuzzing_createFuzzingContext(
  _ maxLength: Int
) -> UnsafeMutableRawPointer {
  Unmanaged.passRetained(FuzzingContext(maxLength: maxLength)).toOpaque()
}

@_cdecl("swift_Fuzzing_mutateSyntax")
public func swift_Fuzzing_mutateSyntax(
  _ inputPtr: UnsafePointer<CChar>, _ inputCount: Int,
  _ rawContext: UnsafeMutableRawPointer,
  _ out: UnsafeMutablePointer<UnsafeMutablePointer<CChar>?>,
  _ outLen: UnsafeMutablePointer<Int>
) {
  let context = Unmanaged<FuzzingContext>.fromOpaque(rawContext).takeUnretainedValue()
  let buffer = UnsafeRawBufferPointer(start: inputPtr, count: inputCount)
  var new = context.next(from: String(decoding: buffer, as: UTF8.self))
  new.withUTF8 { buffer in
    let ptr = malloc(buffer.count).assumingMemoryBound(to: CChar.self)
    memcpy(ptr, buffer.baseAddress!, buffer.count)
    out.pointee = ptr
    outLen.pointee = buffer.count
  }
}
