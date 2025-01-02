//===--- ProjectTree.swift ------------------------------------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2024 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

/// A tree of elements to generate at a given path for a project.
final class ProjectTree {
  private let buildDir: RepoBuildDir
  private let root: Node
  private var nodesByPath: [RelativePath: Node] = [:]

  init(buildDir: RepoBuildDir) {
    self.buildDir = buildDir
    self.root = Node(at: "", parent: nil)
    self.nodesByPath[""] = root
  }
}

extension ProjectTree {
  struct Element {
    var path: RelativePath
    var clangSources: [RelativePath]
    var swiftSources: [RelativePath]
    var nonSources: [RelativePath]

    init(
      at path: RelativePath,
      clangSources: [RelativePath],
      swiftSources: [RelativePath],
      nonSources: [RelativePath]
    ) {
      self.path = path
      self.clangSources = clangSources
      self.swiftSources = swiftSources
      self.nonSources = nonSources
    }
  }

  final class Node {
    fileprivate(set) var element: Element
    fileprivate(set) weak var parent: Node?
    fileprivate(set) var children: [Node] = []

    init(_ element: Element, parent: Node?, children: [Node]) {
      self.element = element
      self.parent = parent
      self.children = children
    }

    init(at path: RelativePath, parent: Node?) {
      self.element = Element(
        at: path, clangSources: [], swiftSources: [], nonSources: []
      )
      self.parent = parent
    }
  }
}

extension ProjectTree {
  private func getOrCreate(at path: RelativePath) -> Node {
    if let node = nodesByPath[path] {
      return node
    }
    // 'parentDir' should only be nil for the root, in which case we should
    // have returned the root.
    let node = Node(at: path, parent: getOrCreate(at: path.parentDir!))
    nodesByPath[path] = node
    return node
  }

  private func addFiles(
    _ files: [RelativePath],
    for getNode: (RelativePath) -> Node,
    to destKP: WritableKeyPath<Element, [RelativePath]>,
    where pred: (RelativePath) -> Bool
  ) {
    for source in files where pred(source) {
      getNode(source).element[keyPath: destKP].append(source)
    }
  }

  private func addLeafFiles(
    _ files: [RelativePath],
    to destKP: WritableKeyPath<Element, [RelativePath]>,
    where pred: (RelativePath) -> Bool
  ) {
    addFiles(
      files, for: { getOrCreate(at: $0.parentDir!) }, to: destKP, where: pred
    )
  }

  func addTargetSource(_ targetSource: ClangTargetSource) throws {
    let files = try buildDir.getAllRepoSubpaths(of: targetSource.path)
    // If we want to split the sources, add the files to leaf nodes. Otherwise
    // add them for the target path.
    let targetNode = getOrCreate(at: targetSource.path)
    func getNode(for source: RelativePath) -> Node {
      if targetSource.shouldSplitSources {
        getOrCreate(at: source.parentDir!)
      } else {
        targetNode
      }
    }
    addFiles(files, for: getNode, to: \.clangSources, where: \.isCSourceLike)
    addFiles(files, for: getNode, to: \.nonSources, where: \.isHeaderLike)
  }

  func addTargetSource(_ targetSource: SwiftTargetSource) throws {
    let files = try buildDir.getAllRepoSubpaths(of: targetSource.path)
    addLeafFiles(files, to: \.swiftSources, where: \.isSwift)
    addLeafFiles(files, to: \.nonSources, where: \.isSwiftGyb)
  }
}
