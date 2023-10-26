//===--- BasicBridgingImpl.h - header for the swift BasicBridging module --===//
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

#ifndef SWIFT_BASIC_BASICBRIDGINGIMPL_H
#define SWIFT_BASIC_BASICBRIDGINGIMPL_H

SWIFT_BEGIN_NULLABILITY_ANNOTATIONS

//===----------------------------------------------------------------------===//
// BridgedStringRef
//===----------------------------------------------------------------------===//

const uint8_t *_Nullable BridgedStringRef_data(BridgedStringRef str) {
  return (const uint8_t *)str.get().data();
}

SwiftInt BridgedStringRef_count(BridgedStringRef str) {
  return (SwiftInt)str.get().size();
}

bool BridgedStringRef_empty(BridgedStringRef str) { return str.get().empty(); }

//===----------------------------------------------------------------------===//
// BridgedOwnedString
//===----------------------------------------------------------------------===//

const uint8_t *_Nullable BridgedOwnedString_data(BridgedOwnedString str) {
  auto *data = str.getRef().data();
  return (const uint8_t *)(data ? data : "");
}

SwiftInt BridgedOwnedString_count(BridgedOwnedString str) {
  return (SwiftInt)str.getRef().size();
}

bool BridgedOwnedString_empty(BridgedOwnedString str) {
  return str.getRef().empty();
}

//===----------------------------------------------------------------------===//
// BridgedSourceLoc
//===----------------------------------------------------------------------===//

bool BridgedSourceLoc_isValid(BridgedSourceLoc str) {
  return str.getOpaquePointerValue() != nullptr;
}

BridgedSourceLoc BridgedSourceLoc::advancedBy(size_t n) const {
  return BridgedSourceLoc(get().getAdvancedLoc(n));
}

SWIFT_END_NULLABILITY_ANNOTATIONS

#endif // SWIFT_BASIC_BASICBRIDGINGIMPL_H
