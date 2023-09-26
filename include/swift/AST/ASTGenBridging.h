//===--- ASTBridging.h - header for the swift SILBridging module ----------===//
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

#ifndef SWIFT_AST_ASTGENBRIDGING_H
#define SWIFT_AST_ASTGENBRIDGING_H

// Workaround to avoid a compiler error because `cas::ObjectRef` is not defined
// when including VirtualFileSystem.h
#include <cassert>
#include "llvm/CAS/CASReference.h"

// Not yet sure if these are really needed.
#include "swift/AST/ForeignAsyncConvention.h"
#include "swift/AST/ForeignErrorConvention.h"
#include "swift/AST/PropertyWrappers.h"
//#include "swift/AST/ModuleDependencies.h"

#include "swift/AST/ASTContext.h"
#include "swift/AST/AnyFunctionRef.h"
#include "swift/Basic/Compiler.h"
#include "swift/AST/Decl.h"
#include "swift/AST/DeclContext.h"
#include "swift/AST/DiagnosticEngine.h"
#include "swift/AST/DiagnosticConsumer.h"

#endif // SWIFT_AST_ASTGENBRIDGING_H
