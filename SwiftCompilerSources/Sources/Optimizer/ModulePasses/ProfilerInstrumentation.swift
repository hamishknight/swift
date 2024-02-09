//===--- ProfilerInstrumentation.swift ------------------------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2023 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

import SIL
import SILBridging

let profilerInstrumentation = ModulePass(name: "profiler-instrumentation") {
  (moduleContext: ModulePassContext) in

  for fn in moduleContext.functions where fn.isProfilable {
    let regions = fn.mapRegions()
    fn.insertCounterIncrements(regions)
    fn.createCoverageMap(regions)
  }
}

fileprivate extension BasicBlock {
  var profilerSourceRange: LineColRange? {
    // If we have a SILGen provided source range, use it.
    for case let inst as ProfilerSourceRangeInst in instructions {
      return inst.range
    }
    // TODO: Pick up source range from debug info...
    return nil
  }
}

enum Counter {
  case zero
  case concrete(Int)
  indirect case add(Counter, Counter)
  indirect case subtract(Counter, Counter)

  func bridged(_ builder: BridgedCounterExpressionBuilder) -> BridgedCounterExpr {
    switch self {
    case .zero:
      return .zero()
    case .concrete(let idx):
      return .concrete(idx)
    case .add(let lhs, let rhs):
      return builder.add(lhs.bridged(builder), rhs.bridged(builder))
    case .subtract(let lhs, let rhs):
      return builder.subtract(lhs.bridged(builder), rhs.bridged(builder))
    }
  }
}

struct Region {
  var counter: Counter
  var range: LineColRange?
}

fileprivate struct CounterMapper {
  private var nextCounterID = 0
  private var computingBlocks = Set<BasicBlock>()
  private(set) var counters: [BasicBlock: Counter] = [:]

  private mutating func newConcreteCounter() -> Counter {
    defer {
      nextCounterID += 1
    }
    return .concrete(nextCounterID)
  }

  private mutating func getOrCreateCounter(for block: BasicBlock) -> Counter {
    if let counter = counters[block] {
      return counter
    }
    let counter = newConcreteCounter()
    counters[block] = counter
    return counter
  }

  private mutating func computeSuccessorCounter(
    for successor: BasicBlock, predecessor: BasicBlock
  ) -> Counter {
    assert(predecessor.successors.contains(successor))

    // The block is the only successor, we adopt the predecessor's counter.
    if predecessor.successors.count == 1 {
      return counter(for: successor)
    }

    let predCounter = counter(for: predecessor)

    // We have multiple successors. We assign concrete counters for all but
    // the last, which can be defined as a subtraction of all the others.
    // TODO: We could potentially do better here by attempting to simplify
    // later, and picking the branch that gives us the best simplification.
    if predecessor.successors.last == successor {
      var result = predCounter
      for succ in predecessor.successors.dropLast() {
        result = .subtract(result, getOrCreateCounter(for: succ))
      }
      return result
    }
    return getOrCreateCounter(for: successor)
  }

  private mutating func computeCounter(for block: BasicBlock) -> Counter {
    // Entry block gets a fresh counter.
    if block.parentFunction.entryBlock == block {
      return newConcreteCounter()
    }
    if computingBlocks.contains(block) {
      // We're already computing a counter for this, so we have a cycle. This
      // is resolved by assigning a new concrete counter.
      // FIXME: This is naive, and could end up assiging more concrete counters
      // than needed. Could we use SILLoopInfo to only do this for loop headers?
      return newConcreteCounter()
    }
    computingBlocks.insert(block)
    defer {
      computingBlocks.remove(block)
    }

    let predCounters = block.predecessors
      .map { computeSuccessorCounter(for: block, predecessor: $0) }

    // In cases where we have a cycle, we may have created a concrete counter
    // for this block already. Otherwise, it's a sum of the predecessors.
    return counters[block] ?? predCounters.reduce(.zero, { .add($0, $1) })
  }

  mutating func counter(for block: BasicBlock) -> Counter {
    if let counter = counters[block] {
      return counter
    }
    let counter = computeCounter(for: block)
    counters[block] = counter
    return counter
  }
}

fileprivate extension Function {
  func mapRegions() -> [BasicBlock: Region] {
    var regions: [BasicBlock: Region] = [:]
    var mapper = CounterMapper()
    for block in blocks {
      let counter = mapper.counter(for: block)
      regions[block] = Region(counter: counter, range: block.profilerSourceRange)
    }
    return regions
  }

  func insertCounterIncrements(_ regions: [BasicBlock: Region]) {
    for (block, region) in regions {
//      block.instructions.first!.
    }
  }

  @discardableResult
  func createCoverageMap(
    _ regions: [BasicBlock: Region]
  ) -> BridgedSILCoverageMap {
    let counterBuilder = BridgedCounterExpressionBuilder.makeNew()
    defer {
      counterBuilder.destroy()
    }

    let bridgedRegions = regions.values.map { region in
      let range = region.range ?? .init(
        startLine: 999, startCol: 999, endLine: 999, endCol: 999
      )
      let bridgedRange = BridgedLineColRange(
        StartLine: range.startLine, StartCol: range.startCol, 
        EndLine: range.endLine, EndCol: range.endCol
      )
      let bridgedCounter = region.counter.bridged(counterBuilder)
      return BridgedMappedRegion.code(bridgedRange, bridgedCounter)
    }
    return bridgedRegions.withBridgedArrayRef { bridgedRegions in
      BridgedSILCoverageMap(bridged, bridgedRegions, counterBuilder)
    }
  }
}
