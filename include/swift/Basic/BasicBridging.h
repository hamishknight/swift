//===--- BasicBridging.h - header for the swift BasicBridging module ------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2022 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#ifndef SWIFT_BASIC_BASICBRIDGING_H
#define SWIFT_BASIC_BASICBRIDGING_H

#if !defined(COMPILED_WITH_SWIFT) || !defined(PURE_BRIDGING_MODE)
#define USED_IN_CPP_SOURCE
#endif

// Do not add other C++/llvm/swift header files here!
// Function implementations should be placed into BasicBridging.cpp and required header files should be added there.
//
#include "swift/Basic/BridgedSwiftObject.h"
#include "swift/Basic/Compiler.h"

#include <stddef.h>
#include <stdint.h>
#ifdef USED_IN_CPP_SOURCE
// Workaround to avoid a compiler error because `cas::ObjectRef` is not defined
// when including VirtualFileSystem.h
#include <cassert>
#include "llvm/CAS/CASReference.h"

#include "swift/Basic/SourceLoc.h"
#include "llvm/ADT/StringRef.h"
#include <string>
#endif

// FIXME: We ought to be importing '<swift/briging>' instead.
#if __has_attribute(swift_name)
#define SWIFT_NAME(NAME) __attribute__((swift_name(NAME)))
#else
#define SWIFT_NAME(NAME)
#endif

#if __has_attribute(availability)
#define SWIFT_UNAVAILABLE(msg) \
  __attribute__((availability(swift, unavailable, message=msg)))
#else
#define SWIFT_UNAVAILABLE(msg)
#endif

#ifdef PURE_BRIDGING_MODE
// In PURE_BRIDGING_MODE, briding functions are not inlined
#define BRIDGED_INLINE
#else
#define BRIDGED_INLINE inline
#endif

SWIFT_BEGIN_NULLABILITY_ANNOTATIONS

typedef intptr_t SwiftInt;
typedef uintptr_t SwiftUInt;

// Define a bridging wrapper that wraps an underlying C++ pointer type. When
// importing into Swift, we expose an initializer and accessor that works with
// `void *`, which is imported as UnsafeMutableRawPointer. Note we can't rely on
// Swift importing the underlying C++ pointer as an OpaquePointer since that is
// liable to change with PURE_BRIDGING_MODE, since that changes what we include,
// and Swift could import the underlying pointee type instead. We need to be
// careful that the interface we expose remains consistent regardless of
// PURE_BRIDGING_MODE.
#define BRIDGING_WRAPPER_IMPL(Node, Name, Nullability)                         \
  class Bridged##Name {                                                        \
    swift::Node * Nullability Ptr;                                             \
                                                                               \
  public:                                                                      \
    SWIFT_UNAVAILABLE("Use init(raw:) instead")                                \
    Bridged##Name(swift::Node * Nullability ptr) : Ptr(ptr) {}                 \
                                                                               \
    SWIFT_UNAVAILABLE("Use '.raw' instead")                                    \
    swift::Node * Nullability get() const { return Ptr; }                      \
  };                                                                           \
                                                                               \
  SWIFT_NAME("getter:Bridged" #Name ".raw(self:)")                             \
  inline void * Nullability Bridged##Name##_getRaw(Bridged##Name bridged) {    \
    return bridged.get();                                                      \
  }                                                                            \
                                                                               \
  SWIFT_NAME("Bridged" #Name ".init(raw:)")                                    \
  inline Bridged##Name Bridged##Name##_fromRaw(void * Nullability ptr) {       \
    return static_cast<swift::Node *>(ptr);                                    \
  }

// Bridging wrapper macros for convenience.
#define BRIDGING_WRAPPER_NONNULL(Name) \
  BRIDGING_WRAPPER_IMPL(Name, Name, _Nonnull)

#define BRIDGING_WRAPPER_NULLABLE(Name) \
  BRIDGING_WRAPPER_IMPL(Name, Nullable##Name, _Nullable)

//===----------------------------------------------------------------------===//
// ArrayRef
//===----------------------------------------------------------------------===//

struct BridgedArrayRef {
  const void *_Nullable data;
  size_t numElements;
};

//===----------------------------------------------------------------------===//
// Data
//===----------------------------------------------------------------------===//

typedef struct BridgedData {
  const char *_Nullable baseAddress;
  size_t size;
} BridgedData;

void BridgedData_free(BridgedData data);

//===----------------------------------------------------------------------===//
// Feature
//===----------------------------------------------------------------------===//

typedef enum ENUM_EXTENSIBILITY_ATTR(open) BridgedFeature {
#define LANGUAGE_FEATURE(FeatureName, SENumber, Description, Option)           \
  FeatureName,
#include "swift/Basic/Features.def"
} BridgedFeature;

//===----------------------------------------------------------------------===//
// OStream
//===----------------------------------------------------------------------===//

struct BridgedOStream {
  void * _Nonnull streamAddr;
};

//===----------------------------------------------------------------------===//
// StringRef
//===----------------------------------------------------------------------===//

class BridgedStringRef {
  const char *_Nullable Data;
  size_t Length;

public:
#ifdef USED_IN_CPP_SOURCE
  BridgedStringRef(llvm::StringRef sref)
      : Data(sref.data()), Length(sref.size()) {}

  llvm::StringRef get() const { return llvm::StringRef(Data, Length); }
#endif

  SWIFT_NAME("init(data:count:)")
  BridgedStringRef(const char *_Nullable data, size_t length)
      : Data(data), Length(length) {}

  void write(BridgedOStream os) const;
};

SWIFT_NAME("getter:BridgedStringRef.data(self:)")
BRIDGED_INLINE 
const uint8_t *_Nullable BridgedStringRef_data(BridgedStringRef str);

SWIFT_NAME("getter:BridgedStringRef.count(self:)")
BRIDGED_INLINE SwiftInt BridgedStringRef_count(BridgedStringRef str);

SWIFT_NAME("getter:BridgedStringRef.isEmpty(self:)")
BRIDGED_INLINE bool BridgedStringRef_empty(BridgedStringRef str);

class BridgedOwnedString {
  char *_Nonnull Data;
  size_t Length;

public:
#ifdef USED_IN_CPP_SOURCE
  BridgedOwnedString(const std::string &stringToCopy);

  llvm::StringRef getRef() const { return llvm::StringRef(Data, Length); }
#endif

  void destroy() const;
};

SWIFT_NAME("getter:BridgedOwnedString.data(self:)")
BRIDGED_INLINE 
const uint8_t *_Nullable BridgedOwnedString_data(BridgedOwnedString str);

SWIFT_NAME("getter:BridgedOwnedString.count(self:)")
BRIDGED_INLINE SwiftInt BridgedOwnedString_count(BridgedOwnedString str);

SWIFT_NAME("getter:BridgedOwnedString.isEmpty(self:)")
BRIDGED_INLINE bool BridgedOwnedString_empty(BridgedOwnedString str);

//===----------------------------------------------------------------------===//
// SourceLoc
//===----------------------------------------------------------------------===//

class BridgedSourceLoc {
  const void *_Nullable Raw;

public:
  BridgedSourceLoc() : Raw(nullptr) {}

  SWIFT_NAME("init(raw:)")
  BridgedSourceLoc(const void *_Nullable raw) : Raw(raw) {}

#ifdef USED_IN_CPP_SOURCE
  BridgedSourceLoc(swift::SourceLoc loc) : Raw(loc.getOpaquePointerValue()) {}

  swift::SourceLoc get() const {
    return swift::SourceLoc(
        llvm::SMLoc::getFromPointer(static_cast<const char *>(Raw)));
  }
#endif

  SWIFT_IMPORT_UNSAFE
  const void *_Nullable getOpaquePointerValue() const { return Raw; }

  SWIFT_NAME("advanced(by:)")
  BRIDGED_INLINE
  BridgedSourceLoc advancedBy(size_t n) const;
};

SWIFT_NAME("getter:BridgedSourceLoc.isValid(self:)")
BRIDGED_INLINE bool BridgedSourceLoc_isValid(BridgedSourceLoc str);

//===----------------------------------------------------------------------===//
// SourceRange
//===----------------------------------------------------------------------===//

typedef struct {
  BridgedSourceLoc startLoc;
  BridgedSourceLoc endLoc;
} BridgedSourceRange;

typedef struct {
  void *_Nonnull start;
  size_t byteLength;
} BridgedCharSourceRange;

//===----------------------------------------------------------------------===//
// Plugins
//===----------------------------------------------------------------------===//

SWIFT_BEGIN_ASSUME_NONNULL

/// Create a new root 'null' JSON value. Clients must call \c JSON_value_delete
/// after using it.
void *JSON_newValue();

/// Parse \p data as a JSON data and return the top-level value. Clients must
/// call \c JSON_value_delete after using it.
void *JSON_deserializedValue(BridgedData data);

/// Serialize a value and populate \p result with the result data. Clients
/// must call \c BridgedData_free after using the \p result.
void JSON_value_serialize(void *valuePtr, BridgedData *result);

/// Destroy and release the memory for \p valuePtr that is a result from
/// \c JSON_newValue() or \c JSON_deserializedValue() .
void JSON_value_delete(void *valuePtr);

bool JSON_value_getAsNull(void *valuePtr);
bool JSON_value_getAsBoolean(void *valuePtr, bool *result);
bool JSON_value_getAsString(void *valuePtr, BridgedData *result);
bool JSON_value_getAsDouble(void *valuePtr, double *result);
bool JSON_value_getAsInteger(void *valuePtr, int64_t *result);
bool JSON_value_getAsObject(void *valuePtr, void *_Nullable *_Nonnull result);
bool JSON_value_getAsArray(void *valuePtr, void *_Nullable *_Nonnull result);

size_t JSON_object_getSize(void *objectPtr);
BridgedData JSON_object_getKey(void *objectPtr, size_t i);
bool JSON_object_hasKey(void *objectPtr, const char *key);
void *JSON_object_getValue(void *objectPtr, const char *key);

size_t JSON_array_getSize(void *arrayPtr);
void *JSON_array_getValue(void *arrayPtr, size_t index);

void JSON_value_emplaceNull(void *valuePtr);
void JSON_value_emplaceBoolean(void *valuePtr, bool value);
void JSON_value_emplaceString(void *valuePtr, const char *value);
void JSON_value_emplaceDouble(void *valuePtr, double value);
void JSON_value_emplaceInteger(void *valuePtr, int64_t value);
void *JSON_value_emplaceNewObject(void *valuePtr);
void *JSON_value_emplaceNewArray(void *valuePtr);

void JSON_object_setNull(void *objectPtr, const char *key);
void JSON_object_setBoolean(void *objectPtr, const char *key, bool value);
void JSON_object_setString(void *objectPtr, const char *key, const char *value);
void JSON_object_setDouble(void *objectPtr, const char *key, double value);
void JSON_object_setInteger(void *objectPtr, const char *key, int64_t value);
void *JSON_object_setNewObject(void *objectPtr, const char *key);
void *JSON_object_setNewArray(void *objectPtr, const char *key);
void *JSON_object_setNewValue(void *objectPtr, const char *key);

void JSON_array_pushNull(void *arrayPtr);
void JSON_array_pushBoolean(void *arrayPtr, bool value);
void JSON_array_pushString(void *arrayPtr, const char *value);
void JSON_array_pushDouble(void *arrayPtr, double value);
void JSON_array_pushInteger(void *arrayPtr, int64_t value);
void *JSON_array_pushNewObject(void *arrayPtr);
void *JSON_array_pushNewArray(void *arrayPtr);
void *JSON_array_pushNewValue(void *arrayPtr);

SWIFT_END_ASSUME_NONNULL

SWIFT_END_NULLABILITY_ANNOTATIONS

#ifndef PURE_BRIDGING_MODE
// In _not_ PURE_BRIDGING_MODE, bridging functions are inlined and therefore
// included in the header file.
#include "BasicBridgingImpl.h"
#endif

#endif
