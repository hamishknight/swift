//===--- DerivedConformanceActor.cpp - Derived ActorProtocol --------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
//
// This file implements derivation of the ActorProtocol and _CompilerCopyable
// protocols.
//
//===----------------------------------------------------------------------===//

#include "TypeChecker.h"
#include "swift/AST/Decl.h"
#include "swift/AST/Expr.h"
#include "swift/AST/Module.h"
#include "swift/AST/ParameterList.h"
#include "swift/AST/Pattern.h"
#include "swift/AST/Stmt.h"
#include "swift/AST/Types.h"
#include "DerivedConformances.h"

using namespace swift;

/// Synthesizes the body for `init(_copying other: Self)`.
///
/// \param initDecl The function decl whose body to synthesize.
static void deriveBodyCopyable_init(AbstractFunctionDecl *initDecl, void *) {

  // The enclosing type decl.
  auto conformanceDC = initDecl->getDeclContext();
  auto *targetDecl = conformanceDC->getSelfNominalTypeDecl();

  auto *funcDC = cast<DeclContext>(initDecl);
  auto &C = funcDC->getASTContext();

  auto *otherParam = initDecl->getParameters()->get(0);

  SmallVector<ASTNode, 4> statements;

  // `self.x = other.x.copy()`
  for (auto *property : targetDecl->getStoredProperties()) {
    // Don't output anything for an already-initialised let property.
    if ((property->isImmutable() && property->hasInitialValue()) ||
        !property->isSettable(funcDC, nullptr, true))
      continue;

    auto propertyTy =
        conformanceDC->mapTypeIntoContext(property->getInterfaceType());

    if (auto referenceType = propertyTy->getAs<ReferenceStorageType>())
      propertyTy = referenceType->getReferentType();

    // `other`
    auto *otherRef = new (C) DeclRefExpr(otherParam, DeclNameLoc(),
                                         /*Implicit*/ true);
    // `other.x`
    auto *otherDotX = new (C) MemberRefExpr(otherRef, SourceLoc(), property,
                                            DeclNameLoc(), /*Implicit*/ true);
    Expr *rhs = otherDotX;
    if (!propertyTy->is<FunctionType>()) {
      // `other.x.copy`
      auto *memberDotCopy = new (C) UnresolvedDotExpr(
          otherDotX, SourceLoc(), DeclName(C.Id_copy), DeclNameLoc(),
          /*Implicit*/ true, /*OuterAlts*/ {}, /*MustBeActorSafe*/ true);

      rhs = CallExpr::createImplicit(C, memberDotCopy, {}, {});
    }

    // `self.x`
    auto *selfRef = DerivedConformance::createSelfDeclRef(initDecl);
    auto *selfDotX = new (C) MemberRefExpr(selfRef, SourceLoc(), property,
                                           DeclNameLoc(), /*Implicit*/ true);
    auto *assignExpr = new (C) AssignExpr(selfDotX, SourceLoc(), rhs,
                                          /*Implicit*/ true);
    statements.push_back(assignExpr);
  }

  // Classes which have a superclass must call super.init(from:) if the
  // superclass is Decodable, or super.init() if it is not.
  if (auto *classDecl = dyn_cast<ClassDecl>(targetDecl)) {
    assert(!classDecl->getSuperclassDecl());
    (void)classDecl;
  }

  auto *body = BraceStmt::create(C, SourceLoc(), statements, SourceLoc(),
                                 /*implicit=*/true);
  initDecl->setBody(body);
}

static ValueDecl *deriveCopyable_init(DerivedConformance &derived) {
  auto &C = derived.TC.Context;

  auto conformanceDC = derived.getConformanceContext();

  // Expected type: (Self) -> (Self) -> (Self)
  // Constructed as: func type
  //                 input: Self
  //                 output: function type
  //                         input: Self
  //                         output: Self
  // Compute from the inside out:

  // Params: (Self)
  auto *innerSelfParamDecl =
      new (C) ParamDecl(VarDecl::Specifier::Default, SourceLoc(), SourceLoc(),
                        C.Id_copying, SourceLoc(), C.Id_copying, conformanceDC);
  innerSelfParamDecl->setImplicit();
  innerSelfParamDecl->setInterfaceType(derived.Nominal->getSelfInterfaceType());

  auto *paramList = ParameterList::createWithoutLoc(innerSelfParamDecl);

  DeclName name(C, DeclBaseName::createConstructor(), paramList);

  auto *initDecl =
      new (C) ConstructorDecl(name, SourceLoc(), OTK_None, SourceLoc(),
                              /*Throws=*/false, SourceLoc(), paramList,
                              /*GenericParams=*/nullptr, conformanceDC);
  initDecl->setImplicit();
  initDecl->setSynthesized();
  initDecl->setBodySynthesizer(&deriveBodyCopyable_init);
  initDecl->setUserAccessible(false);
  initDecl->getAttrs().add(new (C) ActorSafetyAttr(
      SourceRange(), ActorSafetyKind::Unchecked, /*Implicit*/ true));

  if (auto env = conformanceDC->getGenericEnvironmentOfContext())
    initDecl->setGenericEnvironment(env);
  initDecl->computeType();

  initDecl->setValidationToChecked();
  initDecl->copyFormalAccessFrom(derived.Nominal,
                                 /*sourceIsParentContext*/ true);

  C.addSynthesizedDecl(initDecl);

  derived.addMembersToConformanceContext({initDecl});
  return initDecl;
}

ValueDecl *DerivedConformance::deriveCopyable(ValueDecl *requirement) {

  if (checkAndDiagnoseDisallowedContext(requirement))
    return nullptr;

  return deriveCopyable_init(*this);
}
