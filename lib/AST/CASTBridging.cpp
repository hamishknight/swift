#include "swift/AST/CASTBridging.h"

#include "swift/AST/ASTContext.h"
#include "swift/AST/ASTNode.h"
#include "swift/AST/Decl.h"
#include "swift/AST/DiagnosticsCommon.h"
#include "swift/AST/Expr.h"
#include "swift/AST/GenericParamList.h"
#include "swift/AST/Identifier.h"
#include "swift/AST/ParameterList.h"
#include "swift/AST/ParseRequests.h"
#include "swift/AST/Pattern.h"
#include "swift/AST/PluginRegistry.h"
#include "swift/AST/SourceFile.h"
#include "swift/AST/Stmt.h"
#include "swift/AST/TypeRepr.h"

using namespace swift;

namespace {
struct BridgedDiagnosticImpl {
  typedef llvm::MallocAllocator Allocator;

  InFlightDiagnostic inFlight;
  std::vector<StringRef> textBlobs;

  BridgedDiagnosticImpl(InFlightDiagnostic inFlight,
                        std::vector<StringRef> textBlobs)
      : inFlight(std::move(inFlight)), textBlobs(std::move(textBlobs)) {}

  BridgedDiagnosticImpl(const BridgedDiagnosticImpl &) = delete;
  BridgedDiagnosticImpl(BridgedDiagnosticImpl &&) = delete;
  BridgedDiagnosticImpl &operator=(const BridgedDiagnosticImpl &) = delete;
  BridgedDiagnosticImpl &operator=(BridgedDiagnosticImpl &&) = delete;

  ~BridgedDiagnosticImpl() {
    inFlight.flush();

    Allocator allocator;
    for (auto text : textBlobs) {
      allocator.Deallocate(text.data(), text.size());
    }
  }
};
} // namespace

static inline BridgedDeclContext bridgedDeclContext(DeclContext *declContext) {
  return {declContext};
}

static inline DeclContext *unbridged(BridgedDeclContext cDeclContext) {
  return static_cast<DeclContext *>(cDeclContext.raw);
}

// Define `bridged` overloads for each AST node.
#define AST_BRIDGING_WRAPPER(Name)                                             \
  [[maybe_unused]]                                                             \
  static Bridged##Name bridged(Name *raw) {                                    \
    return {raw};                                                              \
  }                                                                            \
// Don't define `bridged` overloads for the TypeRepr subclasses, since we always
// return a BridgedTypeRepr currently, only define `bridged(TypeRepr *)`.
#define TYPEREPR(Id, Parent)
#include "swift/AST/ASTBridgingWrappers.def"

// Define `unbridged` overloads for each AST node.
#define AST_BRIDGING_WRAPPER(Name)                                             \
  [[maybe_unused]]                                                             \
  static Name *unbridged(Bridged##Name bridged) {                              \
    return static_cast<Name *>(bridged.raw);                                   \
  }
#include "swift/AST/ASTBridgingWrappers.def"

// Define `.asDecl` on each BridgedXXXDecl type.
#define DECL(Id, Parent)                                                       \
  BridgedDecl Id##Decl_asDecl(Bridged##Id##Decl decl) {                        \
    return bridged((Decl *)unbridged(decl));                                   \
  }
#define ABSTRACT_DECL(Id, Parent) DECL(Id, Parent)
#include "swift/AST/DeclNodes.def"

// Define `.asDeclContext` on each BridgedXXXDecl type that's also a
// DeclContext.
#define DECL(Id, Parent)
#define CONTEXT_DECL(Id, Parent)                                               \
  BridgedDeclContext Id##Decl_asDeclContext(Bridged##Id##Decl decl) {          \
    return bridgedDeclContext(unbridged(decl));                                 \
  }
#define ABSTRACT_CONTEXT_DECL(Id, Parent) CONTEXT_DECL(Id, Parent)
#include "swift/AST/DeclNodes.def"

// Define `.asStmt` on each BridgedXXXStmt type.
#define STMT(Id, Parent)                                                       \
  BridgedStmt Id##Stmt_asStmt(Bridged##Id##Stmt stmt) {                        \
    return bridged((Stmt *)unbridged(stmt));                                   \
  }
#define ABSTRACT_STMT(Id, Parent) STMT(Id, Parent)
#include "swift/AST/StmtNodes.def"

// Define `.asExpr` on each BridgedXXXExpr type.
#define EXPR(Id, Parent)                                                       \
  BridgedExpr Id##Expr_asExpr(Bridged##Id##Expr expr) {                        \
    return bridged((Expr *)unbridged(expr));                                   \
  }
#define ABSTRACT_EXPR(Id, Parent) EXPR(Id, Parent)
#include "swift/AST/ExprNodes.def"

// Define `.asTypeRepr` on each BridgedXXXTypeRepr type.
#define TYPEREPR(Id, Parent)                                                   \
  BridgedTypeRepr Id##TypeRepr_asTypeRepr(Bridged##Id##TypeRepr typeRepr) {    \
    return bridged((TypeRepr *)unbridged(typeRepr));                           \
  }
#define ABSTRACT_TYPEREPR(Id, Parent) TYPEREPR(Id, Parent)
#include "swift/AST/TypeReprNodes.def"

template <typename T>
static inline llvm::ArrayRef<T>
unbridgedArrayRef(const BridgedArrayRef bridged) {
  return {static_cast<const T *>(bridged.data), size_t(bridged.numElements)};
}

static inline StringRef unbridged(BridgedString cStr) {
  return StringRef{reinterpret_cast<const char *>(cStr.data),
                   size_t(cStr.length)};
}

static inline ASTContext &unbridged(BridgedASTContext cContext) {
  return *static_cast<ASTContext *>(cContext.raw);
}

static inline SourceLoc unbridged(BridgedSourceLoc cLoc) {
  auto smLoc = llvm::SMLoc::getFromPointer(static_cast<const char *>(cLoc.raw));
  return SourceLoc(smLoc);
}

static inline SourceRange unbridged(BridgedSourceRange cRange) {
  return SourceRange(unbridged(cRange.startLoc), unbridged(cRange.endLoc));
}

static inline Identifier unbridged(BridgedIdentifier cIdentifier) {
  return Identifier::getFromOpaquePointer(const_cast<void *>(cIdentifier.raw));
}

static inline BridgedDiagnosticImpl *unbridged(BridgedDiagnostic cDiag) {
  return static_cast<BridgedDiagnosticImpl *>(cDiag.raw);
}

static inline DiagnosticEngine &unbridged(BridgedDiagnosticEngine cEngine) {
  return *static_cast<DiagnosticEngine *>(cEngine.raw);
}

static inline TypeAttributes *unbridged(BridgedTypeAttributes cAttributes) {
  return static_cast<TypeAttributes *>(cAttributes.raw);
}

static TypeAttrKind unbridged(BridgedTypeAttrKind kind) {
  switch (kind) {
#define TYPE_ATTR(X)                                                           \
  case BridgedTypeAttrKind_##X:                                                \
    return TAK_##X;
#include "swift/AST/Attr.def"
  case BridgedTypeAttrKind_Count:
    return TAK_Count;
  }
}

BridgedDiagnostic Diagnostic_create(BridgedSourceLoc cLoc, BridgedString cText,
                                    BridgedDiagnosticSeverity severity,
                                    BridgedDiagnosticEngine cDiags) {
  StringRef origText = unbridged(cText);
  BridgedDiagnosticImpl::Allocator alloc;
  StringRef text = origText.copy(alloc);

  SourceLoc loc = unbridged(cLoc);

  Diag<StringRef> diagID;
  switch (severity) {
  case BridgedDiagnosticSeverity::BridgedError:
    diagID = diag::bridged_error;
    break;
  case BridgedDiagnosticSeverity::BridgedFatalError:
    diagID = diag::bridged_fatal_error;
    break;
  case BridgedDiagnosticSeverity::BridgedNote:
    diagID = diag::bridged_note;
    break;
  case BridgedDiagnosticSeverity::BridgedRemark:
    diagID = diag::bridged_remark;
    break;
  case BridgedDiagnosticSeverity::BridgedWarning:
    diagID = diag::bridged_warning;
    break;
  }

  DiagnosticEngine &diags = unbridged(cDiags);
  return {new BridgedDiagnosticImpl{diags.diagnose(loc, diagID, text), {text}}};
}

/// Highlight a source range as part of the diagnostic.
void Diagnostic_highlight(BridgedDiagnostic cDiag, BridgedSourceLoc cStartLoc,
                          BridgedSourceLoc cEndLoc) {
  SourceLoc startLoc = unbridged(cStartLoc);
  SourceLoc endLoc = unbridged(cEndLoc);

  BridgedDiagnosticImpl *diag = unbridged(cDiag);
  diag->inFlight.highlightChars(startLoc, endLoc);
}

/// Add a Fix-It to replace a source range as part of the diagnostic.
void Diagnostic_fixItReplace(BridgedDiagnostic cDiag,
                             BridgedSourceLoc cStartLoc,
                             BridgedSourceLoc cEndLoc,
                             BridgedString cReplaceText) {

  SourceLoc startLoc = unbridged(cStartLoc);
  SourceLoc endLoc = unbridged(cEndLoc);

  StringRef origReplaceText = unbridged(cReplaceText);
  BridgedDiagnosticImpl::Allocator alloc;
  StringRef replaceText = origReplaceText.copy(alloc);

  BridgedDiagnosticImpl *diag = unbridged(cDiag);
  diag->textBlobs.push_back(replaceText);
  diag->inFlight.fixItReplaceChars(startLoc, endLoc, replaceText);
}

/// Finish the given diagnostic and emit it.
void Diagnostic_finish(BridgedDiagnostic cDiag) {
  BridgedDiagnosticImpl *diag = unbridged(cDiag);
  delete diag;
}

BridgedIdentifier ASTContext_getIdentifier(BridgedASTContext cContext,
                                           BridgedString cStr) {
  StringRef str = unbridged(cStr);
  if (str.size() == 1 && str.front() == '_')
    return BridgedIdentifier();

  // If this was a back-ticked identifier, drop the back-ticks.
  if (str.size() >= 2 && str.front() == '`' && str.back() == '`') {
    str = str.drop_front().drop_back();
  }

  return {unbridged(cContext).getIdentifier(str).getAsOpaquePointer()};
}

bool ASTContext_langOptsHasFeature(BridgedASTContext cContext,
                                   BridgedFeature feature) {
  return unbridged(cContext).LangOpts.hasFeature((Feature)feature);
}

BridgedSourceLoc SourceLoc_advanced(BridgedSourceLoc cLoc, size_t len) {
  SourceLoc loc = unbridged(cLoc).getAdvancedLoc(len);
  return {loc.getOpaquePointerValue()};
}

BridgedTopLevelCodeDecl
TopLevelCodeDecl_createStmt(BridgedASTContext cContext,
                            BridgedDeclContext cDeclContext,
                            BridgedSourceLoc cStartLoc, BridgedStmt statement,
                            BridgedSourceLoc cEndLoc) {
  ASTContext &context = unbridged(cContext);
  DeclContext *declContext = unbridged(cDeclContext);

  auto *S = unbridged(statement);
  auto Brace =
      BraceStmt::create(context, unbridged(cStartLoc), {S}, unbridged(cEndLoc),
                        /*Implicit=*/true);
  return bridged(new (context) TopLevelCodeDecl(declContext, Brace));
}

BridgedTopLevelCodeDecl
TopLevelCodeDecl_createExpr(BridgedASTContext cContext,
                            BridgedDeclContext cDeclContext,
                            BridgedSourceLoc cStartLoc, BridgedExpr expression,
                            BridgedSourceLoc cEndLoc) {
  ASTContext &context = unbridged(cContext);
  DeclContext *declContext = unbridged(cDeclContext);

  auto *E = unbridged(expression);
  auto Brace =
      BraceStmt::create(context, unbridged(cStartLoc), {E}, unbridged(cEndLoc),
                        /*Implicit=*/true);
  return bridged(new (context) TopLevelCodeDecl(declContext, Brace));
}

BridgedSequenceExpr SequenceExpr_createParsed(BridgedASTContext cContext,
                                              BridgedArrayRef exprs) {
  return bridged(SequenceExpr::create(unbridged(cContext),
                                      unbridgedArrayRef<Expr *>(exprs)));
}

BridgedTupleExpr
TupleExpr_createParsed(BridgedASTContext cContext, BridgedSourceLoc cLParen,
                       BridgedArrayRef subs, BridgedArrayRef names,
                       BridgedArrayRef cNameLocs, BridgedSourceLoc cRParen) {
  ASTContext &context = unbridged(cContext);
  return bridged(TupleExpr::create(context, unbridged(cLParen),
                                   unbridgedArrayRef<Expr *>(subs),
                                   unbridgedArrayRef<Identifier>(names),
                                   unbridgedArrayRef<SourceLoc>(cNameLocs),
                                   unbridged(cRParen), /*Implicit*/ false));
}

BridgedCallExpr CallExpr_createParsed(BridgedASTContext cContext,
                                      BridgedExpr fn, BridgedTupleExpr args) {
  ASTContext &context = unbridged(cContext);
  TupleExpr *TE = unbridged(args);
  SmallVector<Argument, 8> arguments;
  for (unsigned i = 0; i < TE->getNumElements(); ++i) {
    arguments.emplace_back(TE->getElementNameLoc(i), TE->getElementName(i),
                           TE->getElement(i));
  }
  auto *argList = ArgumentList::create(context, TE->getLParenLoc(), arguments,
                                       TE->getRParenLoc(), llvm::None,
                                       /*isImplicit*/ false);
  return bridged(CallExpr::create(context, unbridged(fn), argList,
                                  /*implicit*/ false));
}

BridgedUnresolvedDeclRefExpr UnresolvedDeclRefExpr_createParsed(
    BridgedASTContext cContext, BridgedIdentifier base, BridgedSourceLoc cLoc) {
  ASTContext &context = unbridged(cContext);
  auto name = DeclNameRef{unbridged(base)};
  auto *E = new (context) UnresolvedDeclRefExpr(name, DeclRefKind::Ordinary,
                                                DeclNameLoc{unbridged(cLoc)});
  return bridged(E);
}

BridgedStringLiteralExpr
StringLiteralExpr_createParsed(BridgedASTContext cContext, BridgedString cStr,
                               BridgedSourceLoc cTokenLoc) {
  ASTContext &context = unbridged(cContext);
  auto str = context.AllocateCopy(unbridged(cStr));
  return bridged(new (context) StringLiteralExpr(str, unbridged(cTokenLoc)));
}

BridgedIntegerLiteralExpr
IntegerLiteralExpr_createParsed(BridgedASTContext cContext, BridgedString cStr,
                                BridgedSourceLoc cTokenLoc) {
  ASTContext &context = unbridged(cContext);
  auto str = context.AllocateCopy(unbridged(cStr));
  return bridged(new (context) IntegerLiteralExpr(str, unbridged(cTokenLoc)));
}

BridgedArrayExpr ArrayExpr_createParsed(BridgedASTContext cContext,
                                        BridgedSourceLoc cLLoc,
                                        BridgedArrayRef elements,
                                        BridgedArrayRef commas,
                                        BridgedSourceLoc cRLoc) {
  ASTContext &context = unbridged(cContext);
  return bridged(ArrayExpr::create(
      context, unbridged(cLLoc), unbridgedArrayRef<Expr *>(elements),
      unbridgedArrayRef<SourceLoc>(commas), unbridged(cRLoc)));
}

BridgedBooleanLiteralExpr
BooleanLiteralExpr_createParsed(BridgedASTContext cContext, bool value,
                                BridgedSourceLoc cTokenLoc) {
  ASTContext &context = unbridged(cContext);
  return bridged(new (context) BooleanLiteralExpr(value, unbridged(cTokenLoc)));
}

BridgedNilLiteralExpr
NilLiteralExpr_createParsed(BridgedASTContext cContext,
                            BridgedSourceLoc cNilKeywordLoc) {
  auto *e = new (unbridged(cContext)) NilLiteralExpr(unbridged(cNilKeywordLoc));
  return bridged(e);
}

BridgedPatternBindingDecl PatternBindingDecl_createParsed(
    BridgedASTContext cContext, BridgedDeclContext cDeclContext,
    BridgedSourceLoc cBindingKeywordLoc, BridgedExpr nameExpr,
    BridgedExpr initExpr, bool isStatic, bool isLet) {
  ASTContext &context = unbridged(cContext);
  DeclContext *declContext = unbridged(cDeclContext);

  auto *name = cast<UnresolvedDeclRefExpr>(unbridged(nameExpr));
  auto *varDecl = new (context) VarDecl(
      isStatic, isLet ? VarDecl::Introducer::Let : VarDecl::Introducer::Var,
      name->getLoc(), name->getName().getBaseIdentifier(), declContext);
  auto *pattern = new (context) NamedPattern(varDecl);
  return bridged(PatternBindingDecl::create(
      context,
      /*StaticLoc=*/SourceLoc(), // FIXME
      isStatic ? StaticSpellingKind::KeywordStatic : StaticSpellingKind::None,
      unbridged(cBindingKeywordLoc), pattern,
      /*EqualLoc=*/SourceLoc(), // FIXME
      unbridged(initExpr), declContext));
}

BridgedSingleValueStmtExpr SingleValueStmtExpr_createWithWrappedBranches(
    BridgedASTContext cContext, BridgedStmt S, BridgedDeclContext cDeclContext,
    bool mustBeExpr) {
  ASTContext &context = unbridged(cContext);
  DeclContext *declContext = unbridged(cDeclContext);
  return bridged(SingleValueStmtExpr::createWithWrappedBranches(
      context, unbridged(S), declContext, mustBeExpr));
}

BridgedIfStmt IfStmt_createParsed(BridgedASTContext cContext,
                                  BridgedSourceLoc cIfLoc, BridgedExpr cond,
                                  BridgedStmt then, BridgedSourceLoc cElseLoc,
                                  BridgedStmt elseStmt) {
  ASTContext &context = unbridged(cContext);
  return bridged(new (context) IfStmt(
      unbridged(cIfLoc), unbridged(cond), unbridged(then), unbridged(cElseLoc),
      unbridged(elseStmt), llvm::None, context));
}

BridgedReturnStmt ReturnStmt_createParsed(BridgedASTContext cContext,
                                          BridgedSourceLoc cLoc,
                                          BridgedExpr expr) {
  ASTContext &context = unbridged(cContext);
  return bridged(new (context) ReturnStmt(unbridged(cLoc), unbridged(expr)));
}

BridgedBraceStmt BraceStmt_createParsed(BridgedASTContext cContext,
                                        BridgedSourceLoc cLBLoc,
                                        BridgedArrayRef elements,
                                        BridgedSourceLoc cRBLoc) {
  llvm::SmallVector<ASTNode, 6> nodes;
  for (auto node : unbridgedArrayRef<BridgedASTNode>(elements)) {
    if (node.kind == ASTNodeKindExpr) {
      auto expr = (Expr *)node.ptr;
      nodes.push_back(expr);
    } else if (node.kind == ASTNodeKindStmt) {
      auto stmt = (Stmt *)node.ptr;
      nodes.push_back(stmt);
    } else {
      assert(node.kind == ASTNodeKindDecl);
      auto decl = (Decl *)node.ptr;
      nodes.push_back(decl);

      // Variable declarations are part of the list on par with pattern binding
      // declarations per the legacy parser.
      if (auto *bindingDecl = dyn_cast<PatternBindingDecl>(decl)) {
        for (auto i : range(bindingDecl->getNumPatternEntries())) {
          bindingDecl->getPattern(i)->forEachVariable(
              [&nodes](VarDecl *variable) {
            nodes.push_back(variable);
          });
        }
      }
    }
  }

  ASTContext &context = unbridged(cContext);
  return bridged(BraceStmt::create(context, unbridged(cLBLoc),
                                   context.AllocateCopy(nodes),
                                   unbridged(cRBLoc)));
}

BridgedParamDecl ParamDecl_createParsed(
    BridgedASTContext cContext, BridgedDeclContext cDeclContext,
    BridgedSourceLoc cSpecifierLoc, BridgedIdentifier cFirstName,
    BridgedSourceLoc cFirstNameLoc, BridgedIdentifier cSecondName,
    BridgedSourceLoc cSecondNameLoc, BridgedTypeRepr opaqueType,
    BridgedExpr opaqueDefaultValue) {
  assert((bool)cSecondNameLoc.raw == (bool)cSecondName.raw);
  if (!cSecondName.raw) {
    cSecondName = cFirstName;
    cSecondNameLoc = cFirstNameLoc;
  }

  auto *declContext = unbridged(cDeclContext);

  auto *defaultValue = unbridged(opaqueDefaultValue);
  DefaultArgumentKind defaultArgumentKind;

  if (declContext->getParentSourceFile()->Kind == SourceFileKind::Interface &&
      isa<SuperRefExpr>(defaultValue)) {
    defaultValue = nullptr;
    defaultArgumentKind = DefaultArgumentKind::Inherited;
  } else {
    defaultArgumentKind = getDefaultArgKind(defaultValue);
  }

  auto *paramDecl = new (unbridged(cContext)) ParamDecl(
      unbridged(cSpecifierLoc), unbridged(cFirstNameLoc), unbridged(cFirstName),
      unbridged(cSecondNameLoc), unbridged(cSecondName), declContext);
  paramDecl->setTypeRepr(unbridged(opaqueType));
  paramDecl->setDefaultExpr(defaultValue, /*isTypeChecked*/ false);
  paramDecl->setDefaultArgumentKind(defaultArgumentKind);

  return bridged(paramDecl);
}

void ConstructorDecl_setParsedBody(BridgedConstructorDecl decl,
                                   BridgedBraceStmt body) {
  unbridged(decl)->setBody(unbridged(body), FuncDecl::BodyKind::Parsed);
}

void FuncDecl_setParsedBody(BridgedFuncDecl decl, BridgedBraceStmt body) {
  unbridged(decl)->setBody(unbridged(body), FuncDecl::BodyKind::Parsed);
}

void DestructorDecl_setParsedBody(BridgedDestructorDecl decl,
                                  BridgedBraceStmt body) {
  unbridged(decl)->setBody(unbridged(body), FuncDecl::BodyKind::Parsed);
}

BridgedFuncDecl FuncDecl_createParsed(
    BridgedASTContext cContext, BridgedDeclContext cDeclContext,
    BridgedSourceLoc cStaticLoc, BridgedSourceLoc cFuncKeywordLoc,
    BridgedIdentifier cName, BridgedSourceLoc cNameLoc,
    BridgedGenericParamList genericParamList,
    BridgedParameterList parameterList, BridgedSourceLoc cAsyncLoc,
    BridgedSourceLoc cThrowsLoc, BridgedTypeRepr thrownType,
    BridgedTypeRepr returnType, BridgedTrailingWhereClause genericWhereClause) {
  ASTContext &context = unbridged(cContext);

  auto *paramList = unbridged(parameterList);
  auto declName = DeclName(context, unbridged(cName), paramList);
  auto asyncLoc = unbridged(cAsyncLoc);
  auto throwsLoc = unbridged(cThrowsLoc);
  // FIXME: rethrows

  auto *decl = FuncDecl::create(
      context, unbridged(cStaticLoc), StaticSpellingKind::None,
      unbridged(cFuncKeywordLoc), declName, unbridged(cNameLoc),
      asyncLoc.isValid(), asyncLoc, throwsLoc.isValid(), throwsLoc,
      unbridged(thrownType), unbridged(genericParamList), paramList,
      unbridged(returnType), unbridged(cDeclContext));
  decl->setTrailingWhereClause(unbridged(genericWhereClause));

  return bridged(decl);
}

BridgedConstructorDecl ConstructorDecl_createParsed(
    BridgedASTContext cContext, BridgedDeclContext cDeclContext,
    BridgedSourceLoc cInitKeywordLoc, BridgedSourceLoc cFailabilityMarkLoc,
    bool isIUO, BridgedGenericParamList genericParams,
    BridgedParameterList bridgedParameterList, BridgedSourceLoc cAsyncLoc,
    BridgedSourceLoc cThrowsLoc, BridgedTypeRepr thrownType,
    BridgedTrailingWhereClause genericWhereClause) {
  assert((bool)cFailabilityMarkLoc.raw || !isIUO);

  ASTContext &context = unbridged(cContext);

  auto *parameterList = unbridged(bridgedParameterList);
  auto declName =
      DeclName(context, DeclBaseName::createConstructor(), parameterList);
  auto asyncLoc = unbridged(cAsyncLoc);
  auto throwsLoc = unbridged(cThrowsLoc);
  auto failabilityMarkLoc = unbridged(cFailabilityMarkLoc);
  // FIXME: rethrows

  auto *decl = new (context) ConstructorDecl(
      declName, unbridged(cInitKeywordLoc), failabilityMarkLoc.isValid(),
      failabilityMarkLoc, asyncLoc.isValid(), asyncLoc, throwsLoc.isValid(),
      throwsLoc, unbridged(thrownType), parameterList, unbridged(genericParams),
      unbridged(cDeclContext));
  decl->setTrailingWhereClause(unbridged(genericWhereClause));
  decl->setImplicitlyUnwrappedOptional(isIUO);

  return bridged(decl);
}

BridgedDestructorDecl
DestructorDecl_createParsed(BridgedASTContext cContext,
                            BridgedDeclContext cDeclContext,
                            BridgedSourceLoc cDeinitKeywordLoc) {
  ASTContext &context = unbridged(cContext);
  auto *decl = new (context)
      DestructorDecl(unbridged(cDeinitKeywordLoc), unbridged(cDeclContext));

  return bridged(decl);
}

BridgedTypeRepr SimpleIdentTypeRepr_createParsed(BridgedASTContext cContext,
                                                 BridgedSourceLoc cLoc,
                                                 BridgedIdentifier id) {
  ASTContext &context = unbridged(cContext);
  return bridged(new (context) SimpleIdentTypeRepr(DeclNameLoc(unbridged(cLoc)),
                                                   DeclNameRef(unbridged(id))));
}

BridgedTypeRepr GenericIdentTypeRepr_createParsed(BridgedASTContext cContext,
                                                  BridgedIdentifier name,
                                                  BridgedSourceLoc cNameLoc,
                                                  BridgedArrayRef genericArgs,
                                                  BridgedSourceLoc cLAngleLoc,
                                                  BridgedSourceLoc cRAngleLoc) {
  ASTContext &context = unbridged(cContext);
  auto Loc = DeclNameLoc(unbridged(cNameLoc));
  auto Name = DeclNameRef(unbridged(name));
  SourceLoc lAngleLoc = unbridged(cLAngleLoc);
  SourceLoc rAngleLoc = unbridged(cRAngleLoc);
  return bridged(GenericIdentTypeRepr::create(
      context, Loc, Name, unbridgedArrayRef<TypeRepr *>(genericArgs),
      SourceRange{lAngleLoc, rAngleLoc}));
}

BridgedUnresolvedDotExpr
UnresolvedDotExpr_createParsed(BridgedASTContext cContext, BridgedExpr base,
                               BridgedSourceLoc cDotLoc, BridgedIdentifier name,
                               BridgedSourceLoc cNameLoc) {
  ASTContext &context = unbridged(cContext);
  return bridged(new (context) UnresolvedDotExpr(
      unbridged(base), unbridged(cDotLoc), DeclNameRef(unbridged(name)),
      DeclNameLoc(unbridged(cNameLoc)), false));
}

BridgedClosureExpr ClosureExpr_createParsed(BridgedASTContext cContext,
                                            BridgedDeclContext cDeclContext,
                                            BridgedBraceStmt body) {
  DeclAttributes attributes;
  SourceRange bracketRange;
  SourceLoc asyncLoc;
  SourceLoc throwsLoc;
  SourceLoc arrowLoc;
  SourceLoc inLoc;

  ASTContext &context = unbridged(cContext);
  DeclContext *declContext = unbridged(cDeclContext);

  auto params = ParameterList::create(context, inLoc, {}, inLoc);

  auto *out = new (context) ClosureExpr(
      attributes, bracketRange, nullptr, nullptr, asyncLoc, throwsLoc,
      /*FIXME:thrownType=*/nullptr, arrowLoc, inLoc, nullptr, declContext);
  out->setBody(unbridged(body), true);
  out->setParameterList(params);
  return bridged(out);
}

BridgedTypeAliasDecl TypeAliasDecl_createParsed(
    BridgedASTContext cContext, BridgedDeclContext cDeclContext,
    BridgedSourceLoc cAliasKeywordLoc, BridgedIdentifier cName,
    BridgedSourceLoc cNameLoc, BridgedGenericParamList genericParamList,
    BridgedSourceLoc cEqualLoc, BridgedTypeRepr opaqueUnderlyingType,
    BridgedTrailingWhereClause genericWhereClause) {
  ASTContext &context = unbridged(cContext);

  auto *decl = new (context)
      TypeAliasDecl(unbridged(cAliasKeywordLoc), unbridged(cEqualLoc),
                    unbridged(cName), unbridged(cNameLoc),
                    unbridged(genericParamList), unbridged(cDeclContext));
  decl->setUnderlyingTypeRepr(unbridged(opaqueUnderlyingType));
  decl->setTrailingWhereClause(unbridged(genericWhereClause));

  return bridged(decl);
}

static void setParsedMembers(IterableDeclContext *IDC,
                             BridgedArrayRef bridgedMembers) {
  auto &ctx = IDC->getDecl()->getASTContext();

  SmallVector<Decl *> members;
  for (auto *decl : unbridgedArrayRef<Decl *>(bridgedMembers)) {
    members.push_back(decl);
    // Each enum case element is also part of the members list according to the
    // legacy parser.
    if (auto *ECD = dyn_cast<EnumCaseDecl>(decl)) {
      for (auto *EED : ECD->getElements()) {
        members.push_back(EED);
      }
    }
  }

  ctx.evaluator.cacheOutput(
      ParseMembersRequest{IDC},
      FingerprintAndMembers{llvm::None, ctx.AllocateCopy(members)});
}

void NominalTypeDecl_setParsedMembers(BridgedNominalTypeDecl bridgedDecl,
                                      BridgedArrayRef bridgedMembers) {
  setParsedMembers(unbridged(bridgedDecl), bridgedMembers);
}

void ExtensionDecl_setParsedMembers(BridgedExtensionDecl bridgedDecl,
                                    BridgedArrayRef bridgedMembers) {
  setParsedMembers(unbridged(bridgedDecl), bridgedMembers);
}

static SmallVector<InheritedEntry>
convertToInheritedEntries(BridgedArrayRef cInheritedTypes) {
  SmallVector<InheritedEntry> inheritedEntries;
  for (auto &repr : unbridgedArrayRef<BridgedTypeRepr>(cInheritedTypes)) {
    inheritedEntries.emplace_back(unbridged(repr));
  }

  return inheritedEntries;
}

BridgedNominalTypeDecl EnumDecl_createParsed(
    BridgedASTContext cContext, BridgedDeclContext cDeclContext,
    BridgedSourceLoc cEnumKeywordLoc, BridgedIdentifier cName,
    BridgedSourceLoc cNameLoc, BridgedGenericParamList genericParamList,
    BridgedArrayRef cInheritedTypes,
    BridgedTrailingWhereClause genericWhereClause,
    BridgedSourceRange cBraceRange) {
  ASTContext &context = unbridged(cContext);

  NominalTypeDecl *decl = new (context) EnumDecl(
      unbridged(cEnumKeywordLoc), unbridged(cName), unbridged(cNameLoc),
      context.AllocateCopy(convertToInheritedEntries(cInheritedTypes)),
      unbridged(genericParamList), unbridged(cDeclContext));
  decl->setTrailingWhereClause(unbridged(genericWhereClause));
  decl->setBraces(unbridged(cBraceRange));

  return bridged(decl);
}

BridgedEnumCaseDecl EnumCaseDecl_createParsed(BridgedDeclContext cDeclContext,
                                              BridgedSourceLoc cCaseKeywordLoc,
                                              BridgedArrayRef cElements) {
  auto *decl = EnumCaseDecl::create(
      unbridged(cCaseKeywordLoc),
      unbridgedArrayRef<EnumElementDecl *>(cElements), unbridged(cDeclContext));

  return bridged(decl);
}

BridgedEnumElementDecl EnumElementDecl_createParsed(
    BridgedASTContext cContext, BridgedDeclContext cDeclContext,
    BridgedIdentifier cName, BridgedSourceLoc cNameLoc,
    BridgedParameterList bridgedParameterList, BridgedSourceLoc cEqualsLoc,
    BridgedExpr rawValue) {
  ASTContext &context = unbridged(cContext);

  auto *parameterList = unbridged(bridgedParameterList);
  DeclName declName;
  {
    auto identifier = unbridged(cName);
    if (parameterList) {
      declName = DeclName(context, identifier, parameterList);
    } else {
      declName = identifier;
    }
  }

  return bridged(new (context) EnumElementDecl(
      unbridged(cNameLoc), declName, parameterList, unbridged(cEqualsLoc),
      cast_or_null<LiteralExpr>(unbridged(rawValue)), unbridged(cDeclContext)));
}

BridgedNominalTypeDecl StructDecl_createParsed(
    BridgedASTContext cContext, BridgedDeclContext cDeclContext,
    BridgedSourceLoc cStructKeywordLoc, BridgedIdentifier cName,
    BridgedSourceLoc cNameLoc, BridgedGenericParamList genericParamList,
    BridgedArrayRef cInheritedTypes,
    BridgedTrailingWhereClause genericWhereClause,
    BridgedSourceRange cBraceRange) {
  ASTContext &context = unbridged(cContext);

  NominalTypeDecl *decl = new (context) StructDecl(
      unbridged(cStructKeywordLoc), unbridged(cName), unbridged(cNameLoc),
      context.AllocateCopy(convertToInheritedEntries(cInheritedTypes)),
      unbridged(genericParamList), unbridged(cDeclContext));
  decl->setTrailingWhereClause(unbridged(genericWhereClause));
  decl->setBraces(unbridged(cBraceRange));

  return bridged(decl);
}

BridgedNominalTypeDecl ClassDecl_createParsed(
    BridgedASTContext cContext, BridgedDeclContext cDeclContext,
    BridgedSourceLoc cClassKeywordLoc, BridgedIdentifier cName,
    BridgedSourceLoc cNameLoc, BridgedGenericParamList genericParamList,
    BridgedArrayRef cInheritedTypes,
    BridgedTrailingWhereClause genericWhereClause,
    BridgedSourceRange cBraceRange, bool isActor) {
  ASTContext &context = unbridged(cContext);

  NominalTypeDecl *decl = new (context) ClassDecl(
      unbridged(cClassKeywordLoc), unbridged(cName), unbridged(cNameLoc),
      context.AllocateCopy(convertToInheritedEntries(cInheritedTypes)),
      unbridged(genericParamList), unbridged(cDeclContext), isActor);
  decl->setTrailingWhereClause(unbridged(genericWhereClause));
  decl->setBraces(unbridged(cBraceRange));

  return bridged(decl);
}

BridgedNominalTypeDecl ProtocolDecl_createParsed(
    BridgedASTContext cContext, BridgedDeclContext cDeclContext,
    BridgedSourceLoc cProtocolKeywordLoc, BridgedIdentifier cName,
    BridgedSourceLoc cNameLoc, BridgedArrayRef cPrimaryAssociatedTypeNames,
    BridgedArrayRef cInheritedTypes,
    BridgedTrailingWhereClause genericWhereClause,
    BridgedSourceRange cBraceRange) {
  SmallVector<PrimaryAssociatedTypeName, 2> primaryAssociatedTypeNames;
  for (auto &pair : unbridgedArrayRef<BridgedIdentifierAndSourceLoc>(
           cPrimaryAssociatedTypeNames)) {
    primaryAssociatedTypeNames.emplace_back(unbridged(pair.name),
                                            unbridged(pair.nameLoc));
  }

  ASTContext &context = unbridged(cContext);
  NominalTypeDecl *decl = new (context) ProtocolDecl(
      unbridged(cDeclContext), unbridged(cProtocolKeywordLoc),
      unbridged(cNameLoc), unbridged(cName),
      context.AllocateCopy(primaryAssociatedTypeNames),
      context.AllocateCopy(convertToInheritedEntries(cInheritedTypes)),
      unbridged(genericWhereClause));
  decl->setBraces(unbridged(cBraceRange));

  return bridged(decl);
}

BridgedAssociatedTypeDecl AssociatedTypeDecl_createParsed(
    BridgedASTContext cContext, BridgedDeclContext cDeclContext,
    BridgedSourceLoc cAssociatedtypeKeywordLoc, BridgedIdentifier cName,
    BridgedSourceLoc cNameLoc, BridgedArrayRef cInheritedTypes,
    BridgedTypeRepr defaultType,
    BridgedTrailingWhereClause genericWhereClause) {
  ASTContext &context = unbridged(cContext);

  auto *decl = AssociatedTypeDecl::createParsed(
      context, unbridged(cDeclContext), unbridged(cAssociatedtypeKeywordLoc),
      unbridged(cName), unbridged(cNameLoc), unbridged(defaultType),
      unbridged(genericWhereClause));
  decl->setInherited(
      context.AllocateCopy(convertToInheritedEntries(cInheritedTypes)));

  return bridged(decl);
}

BridgedExtensionDecl ExtensionDecl_createParsed(
    BridgedASTContext cContext, BridgedDeclContext cDeclContext,
    BridgedSourceLoc cExtensionKeywordLoc, BridgedTypeRepr extendedType,
    BridgedArrayRef cInheritedTypes,
    BridgedTrailingWhereClause genericWhereClause,
    BridgedSourceRange cBraceRange) {
  ASTContext &context = unbridged(cContext);

  auto *decl = ExtensionDecl::create(
      context, unbridged(cExtensionKeywordLoc), unbridged(extendedType),
      context.AllocateCopy(convertToInheritedEntries(cInheritedTypes)),
      unbridged(cDeclContext), unbridged(genericWhereClause));
  decl->setBraces(unbridged(cBraceRange));
  return bridged(decl);
}

BridgedOperatorDecl OperatorDecl_createParsed(
    BridgedASTContext cContext, BridgedDeclContext cDeclContext,
    BridgedOperatorFixity cFixity, BridgedSourceLoc cOperatorKeywordLoc,
    BridgedIdentifier cName, BridgedSourceLoc cNameLoc,
    BridgedSourceLoc cColonLoc, BridgedIdentifier cPrecedenceGroupName,
    BridgedSourceLoc cPrecedenceGroupLoc) {
  assert(bool(cColonLoc.raw) == (bool)cPrecedenceGroupName.raw);
  assert(bool(cColonLoc.raw) == (bool)cPrecedenceGroupLoc.raw);

  ASTContext &context = unbridged(cContext);
  auto operatorKeywordLoc = unbridged(cOperatorKeywordLoc);
  auto name = unbridged(cName);
  auto nameLoc = unbridged(cNameLoc);
  auto *declContext = unbridged(cDeclContext);

  OperatorDecl *decl = nullptr;
  switch (cFixity) {
  case BridgedOperatorFixityInfix:
    decl = new (context) InfixOperatorDecl(
        declContext, operatorKeywordLoc, name, nameLoc, unbridged(cColonLoc),
        unbridged(cPrecedenceGroupName), unbridged(cPrecedenceGroupLoc));
    break;
  case BridgedOperatorFixityPrefix:
    assert(!cColonLoc.raw);
    decl = new (context)
        PrefixOperatorDecl(declContext, operatorKeywordLoc, name, nameLoc);
    break;
  case BridgedOperatorFixityPostfix:
    assert(!cColonLoc.raw);
    decl = new (context)
        PostfixOperatorDecl(declContext, operatorKeywordLoc, name, nameLoc);
    break;
  }

  return bridged(decl);
}

BridgedPrecedenceGroupDecl PrecedenceGroupDecl_createParsed(
    BridgedDeclContext cDeclContext,
    BridgedSourceLoc cPrecedencegroupKeywordLoc, BridgedIdentifier cName,
    BridgedSourceLoc cNameLoc, BridgedSourceLoc cLeftBraceLoc,
    BridgedSourceLoc cAssociativityKeywordLoc,
    BridgedSourceLoc cAssociativityValueLoc,
    BridgedAssociativity cAssociativity, BridgedSourceLoc cAssignmentKeywordLoc,
    BridgedSourceLoc cAssignmentValueLoc, bool isAssignment,
    BridgedSourceLoc cHigherThanKeywordLoc, BridgedArrayRef cHigherThanNames,
    BridgedSourceLoc cLowerThanKeywordLoc, BridgedArrayRef cLowerThanNames,
    BridgedSourceLoc cRightBraceLoc) {

  SmallVector<PrecedenceGroupDecl::Relation, 2> higherThanNames;
  for (auto &pair :
       unbridgedArrayRef<BridgedIdentifierAndSourceLoc>(cHigherThanNames)) {
    higherThanNames.push_back(
        {unbridged(pair.nameLoc), unbridged(pair.name), nullptr});
  }

  SmallVector<PrecedenceGroupDecl::Relation, 2> lowerThanNames;
  for (auto &pair :
       unbridgedArrayRef<BridgedIdentifierAndSourceLoc>(cLowerThanNames)) {
    lowerThanNames.push_back(
        {unbridged(pair.nameLoc), unbridged(pair.name), nullptr});
  }

  auto *decl = PrecedenceGroupDecl::create(
      unbridged(cDeclContext), unbridged(cPrecedencegroupKeywordLoc),
      unbridged(cNameLoc), unbridged(cName), unbridged(cLeftBraceLoc),
      unbridged(cAssociativityKeywordLoc), unbridged(cAssociativityValueLoc),
      static_cast<Associativity>(cAssociativity),
      unbridged(cAssignmentKeywordLoc), unbridged(cAssignmentValueLoc),
      isAssignment, unbridged(cHigherThanKeywordLoc), higherThanNames,
      unbridged(cLowerThanKeywordLoc), lowerThanNames,
      unbridged(cRightBraceLoc));

  return bridged(decl);
}

BridgedImportDecl ImportDecl_createParsed(BridgedASTContext cContext,
                                          BridgedDeclContext cDeclContext,
                                          BridgedSourceLoc cImportKeywordLoc,
                                          BridgedImportKind cImportKind,
                                          BridgedSourceLoc cImportKindLoc,
                                          BridgedArrayRef cImportPathElements) {
  ImportPath::Builder builder;
  for (auto &element :
       unbridgedArrayRef<BridgedIdentifierAndSourceLoc>(cImportPathElements)) {
    builder.push_back(unbridged(element.name), unbridged(element.nameLoc));
  }

  ASTContext &context = unbridged(cContext);
  auto *decl = ImportDecl::create(
      context, unbridged(cDeclContext), unbridged(cImportKeywordLoc),
      static_cast<ImportKind>(cImportKind), unbridged(cImportKindLoc),
      std::move(builder).get());

  return bridged(decl);
}

BridgedTypeRepr OptionalTypeRepr_createParsed(BridgedASTContext cContext,
                                              BridgedTypeRepr base,
                                              BridgedSourceLoc cQuestionLoc) {
  ASTContext &context = unbridged(cContext);
  return bridged(
      new (context) OptionalTypeRepr(unbridged(base), unbridged(cQuestionLoc)));
}

BridgedTypeRepr ImplicitlyUnwrappedOptionalTypeRepr_createParsed(
    BridgedASTContext cContext, BridgedTypeRepr base,
    BridgedSourceLoc cExclamationLoc) {
  ASTContext &context = unbridged(cContext);
  return bridged(new (context) ImplicitlyUnwrappedOptionalTypeRepr(
      unbridged(base), unbridged(cExclamationLoc)));
}

BridgedTypeRepr ArrayTypeRepr_createParsed(BridgedASTContext cContext,
                                           BridgedTypeRepr base,
                                           BridgedSourceLoc cLSquareLoc,
                                           BridgedSourceLoc cRSquareLoc) {
  ASTContext &context = unbridged(cContext);
  SourceLoc lSquareLoc = unbridged(cLSquareLoc);
  SourceLoc rSquareLoc = unbridged(cRSquareLoc);
  return bridged(new (context) ArrayTypeRepr(
      unbridged(base), SourceRange{lSquareLoc, rSquareLoc}));
}

BridgedTypeRepr DictionaryTypeRepr_createParsed(BridgedASTContext cContext,
                                                BridgedSourceLoc cLSquareLoc,
                                                BridgedTypeRepr keyType,
                                                BridgedSourceLoc cColonloc,
                                                BridgedTypeRepr valueType,
                                                BridgedSourceLoc cRSquareLoc) {
  ASTContext &context = unbridged(cContext);
  SourceLoc lSquareLoc = unbridged(cLSquareLoc);
  SourceLoc colonLoc = unbridged(cColonloc);
  SourceLoc rSquareLoc = unbridged(cRSquareLoc);
  return bridged(new (context) DictionaryTypeRepr(
      unbridged(keyType), unbridged(valueType), colonLoc,
      SourceRange{lSquareLoc, rSquareLoc}));
}

BridgedTypeRepr MetatypeTypeRepr_createParsed(BridgedASTContext cContext,
                                              BridgedTypeRepr baseType,
                                              BridgedSourceLoc cTypeLoc) {
  ASTContext &context = unbridged(cContext);
  SourceLoc tyLoc = unbridged(cTypeLoc);
  return bridged(new (context) MetatypeTypeRepr(unbridged(baseType), tyLoc));
}

BridgedTypeRepr ProtocolTypeRepr_createParsed(BridgedASTContext cContext,
                                              BridgedTypeRepr baseType,
                                              BridgedSourceLoc cProtoLoc) {
  ASTContext &context = unbridged(cContext);
  SourceLoc protoLoc = unbridged(cProtoLoc);
  return bridged(new (context) ProtocolTypeRepr(unbridged(baseType), protoLoc));
}

BridgedTypeRepr
PackExpansionTypeRepr_createParsed(BridgedASTContext cContext,
                                   BridgedTypeRepr base,
                                   BridgedSourceLoc cRepeatLoc) {
  ASTContext &context = unbridged(cContext);
  return bridged(new (context) PackExpansionTypeRepr(unbridged(cRepeatLoc),
                                                     unbridged(base)));
}

BridgedTypeAttrKind TypeAttrKind_fromString(BridgedString cStr) {
  TypeAttrKind kind = TypeAttributes::getAttrKindFromString(unbridged(cStr));
  switch (kind) {
#define TYPE_ATTR(X) case TAK_##X: return BridgedTypeAttrKind_##X;
#include "swift/AST/Attr.def"
    case TAK_Count: return BridgedTypeAttrKind_Count;
  }
}

BridgedTypeAttributes TypeAttributes_create() { return {new TypeAttributes()}; }

void TypeAttributes_addSimpleAttr(BridgedTypeAttributes cAttributes,
                                  BridgedTypeAttrKind cKind,
                                  BridgedSourceLoc cAtLoc,
                                  BridgedSourceLoc cAttrLoc) {
  TypeAttributes *typeAttributes = unbridged(cAttributes);
  typeAttributes->setAttr(unbridged(cKind), unbridged(cAttrLoc));
  if (typeAttributes->AtLoc.isInvalid())
    typeAttributes->AtLoc = unbridged(cAtLoc);
}

BridgedTypeRepr
AttributedTypeRepr_createParsed(BridgedASTContext cContext,
                                BridgedTypeRepr base,
                                BridgedTypeAttributes cAttributes) {
  TypeAttributes *typeAttributes = unbridged(cAttributes);
  if (typeAttributes->empty())
    return base;

  ASTContext &context = unbridged(cContext);
  auto attributedType =
      new (context) AttributedTypeRepr(*typeAttributes, unbridged(base));
  delete typeAttributes;
  return bridged(attributedType);
}

BridgedTypeRepr
SpecifierTypeRepr_createParsed(BridgedASTContext cContext, BridgedTypeRepr base,
                               BridgedAttributedTypeSpecifier specifier,
                               BridgedSourceLoc cSpecifierLoc) {
  ASTContext &context = unbridged(cContext);
  SourceLoc loc = unbridged(cSpecifierLoc);
  TypeRepr *baseType = unbridged(base);
  switch (specifier) {
  case BridgedAttributedTypeSpecifierInOut:
    return bridged(new (context)
                       OwnershipTypeRepr(baseType, ParamSpecifier::InOut, loc));
  case BridgedAttributedTypeSpecifierBorrowing:
    return bridged(new (context) OwnershipTypeRepr(
        baseType, ParamSpecifier::Borrowing, loc));
  case BridgedAttributedTypeSpecifierConsuming:
    return bridged(new (context) OwnershipTypeRepr(
        baseType, ParamSpecifier::Consuming, loc));
  case BridgedAttributedTypeSpecifierLegacyShared:
    return bridged(new (context) OwnershipTypeRepr(
        baseType, ParamSpecifier::LegacyShared, loc));
  case BridgedAttributedTypeSpecifierLegacyOwned:
    return bridged(new (context) OwnershipTypeRepr(
        baseType, ParamSpecifier::LegacyOwned, loc));
  case BridgedAttributedTypeSpecifierConst:
    return bridged(new (context) CompileTimeConstTypeRepr(baseType, loc));
  case BridgedAttributedTypeSpecifierIsolated:
    return bridged(new (context) IsolatedTypeRepr(baseType, loc));
  }
}

BridgedTypeRepr VarargTypeRepr_createParsed(BridgedASTContext cContext,
                                            BridgedTypeRepr base,
                                            BridgedSourceLoc cEllipsisLoc) {
  ASTContext &context = unbridged(cContext);
  SourceLoc ellipsisLoc = unbridged(cEllipsisLoc);
  TypeRepr *baseType = unbridged(base);
  return bridged(new (context) VarargTypeRepr(baseType, ellipsisLoc));
}

BridgedTypeRepr TupleTypeRepr_createParsed(BridgedASTContext cContext,
                                           BridgedArrayRef elements,
                                           BridgedSourceLoc cLParenLoc,
                                           BridgedSourceLoc cRParenLoc) {
  ASTContext &context = unbridged(cContext);
  SourceLoc lParen = unbridged(cLParenLoc);
  SourceLoc rParen = unbridged(cRParenLoc);

  SmallVector<TupleTypeReprElement, 8> tupleElements;
  for (auto element : unbridgedArrayRef<BridgedTupleTypeElement>(elements)) {
    TupleTypeReprElement elementRepr;
    elementRepr.Name = unbridged(element.Name);
    elementRepr.NameLoc = unbridged(element.NameLoc);
    elementRepr.SecondName = unbridged(element.SecondName);
    elementRepr.SecondNameLoc = unbridged(element.SecondNameLoc);
    elementRepr.UnderscoreLoc = unbridged(element.UnderscoreLoc);
    elementRepr.ColonLoc = unbridged(element.ColonLoc);
    elementRepr.Type = unbridged(element.Type);
    elementRepr.TrailingCommaLoc = unbridged(element.TrailingCommaLoc);
    tupleElements.emplace_back(elementRepr);
  }

  return bridged(TupleTypeRepr::create(context, tupleElements,
                                       SourceRange{lParen, rParen}));
}

BridgedTypeRepr
MemberTypeRepr_createParsed(BridgedASTContext cContext,
                            BridgedTypeRepr baseComponent,
                            BridgedArrayRef bridgedMemberComponents) {
  ASTContext &context = unbridged(cContext);
  auto memberComponents =
      unbridgedArrayRef<IdentTypeRepr *>(bridgedMemberComponents);

  return bridged(MemberTypeRepr::create(context, unbridged(baseComponent),
                                        memberComponents));
}

BridgedTypeRepr CompositionTypeRepr_createEmpty(BridgedASTContext cContext,
                                                BridgedSourceLoc cAnyLoc) {
  ASTContext &context = unbridged(cContext);
  SourceLoc anyLoc = unbridged(cAnyLoc);
  return bridged(CompositionTypeRepr::createEmptyComposition(context, anyLoc));
}

BridgedTypeRepr
CompositionTypeRepr_createParsed(BridgedASTContext cContext,
                                 BridgedArrayRef cTypes,
                                 BridgedSourceLoc cFirstAmpLoc) {
  ASTContext &context = unbridged(cContext);
  SourceLoc firstAmpLoc = unbridged(cFirstAmpLoc);
  auto types = unbridgedArrayRef<TypeRepr *>(cTypes);
  return bridged(CompositionTypeRepr::create(
      context, types, types.front()->getStartLoc(),
      SourceRange{firstAmpLoc, types.back()->getEndLoc()}));
}

BridgedTypeRepr FunctionTypeRepr_createParsed(BridgedASTContext cContext,
                                              BridgedTypeRepr argsTy,
                                              BridgedSourceLoc cAsyncLoc,
                                              BridgedSourceLoc cThrowsLoc,
                                              BridgedTypeRepr thrownType,
                                              BridgedSourceLoc cArrowLoc,
                                              BridgedTypeRepr resultType) {
  ASTContext &context = unbridged(cContext);
  return bridged(new (context) FunctionTypeRepr(
      nullptr, cast<TupleTypeRepr>(unbridged(argsTy)), unbridged(cAsyncLoc),
      unbridged(cThrowsLoc), unbridged(thrownType), unbridged(cArrowLoc),
      unbridged(resultType)));
}

BridgedTypeRepr
NamedOpaqueReturnTypeRepr_createParsed(BridgedASTContext cContext,
                                       BridgedTypeRepr baseTy) {
  ASTContext &context = unbridged(cContext);
  return bridged(new (context)
                     NamedOpaqueReturnTypeRepr(unbridged(baseTy), nullptr));
}

BridgedTypeRepr OpaqueReturnTypeRepr_createParsed(BridgedASTContext cContext,
                                                  BridgedSourceLoc cOpaqueLoc,
                                                  BridgedTypeRepr baseTy) {
  ASTContext &context = unbridged(cContext);
  return bridged(new (context) OpaqueReturnTypeRepr(unbridged(cOpaqueLoc),
                                                    unbridged(baseTy)));
}
BridgedTypeRepr ExistentialTypeRepr_createParsed(BridgedASTContext cContext,
                                                 BridgedSourceLoc cAnyLoc,
                                                 BridgedTypeRepr baseTy) {
  ASTContext &context = unbridged(cContext);
  return bridged(
      new (context) ExistentialTypeRepr(unbridged(cAnyLoc), unbridged(baseTy)));
}

BridgedGenericParamList GenericParamList_createParsed(
    BridgedASTContext cContext, BridgedSourceLoc cLeftAngleLoc,
    BridgedArrayRef cParameters,
    BridgedTrailingWhereClause bridgedGenericWhereClause,
    BridgedSourceLoc cRightAngleLoc) {
  SourceLoc whereLoc;
  ArrayRef<RequirementRepr> requirements;
  if (auto *genericWhereClause = unbridged(bridgedGenericWhereClause)) {
    whereLoc = genericWhereClause->getWhereLoc();
    requirements = genericWhereClause->getRequirements();
  }

  return bridged(GenericParamList::create(
      unbridged(cContext), unbridged(cLeftAngleLoc),
      unbridgedArrayRef<GenericTypeParamDecl *>(cParameters), whereLoc,
      requirements, unbridged(cRightAngleLoc)));
}

BridgedGenericTypeParamDecl GenericTypeParamDecl_createParsed(
    BridgedASTContext cContext, BridgedDeclContext cDeclContext,
    BridgedSourceLoc cEachLoc, BridgedIdentifier cName,
    BridgedSourceLoc cNameLoc, BridgedTypeRepr bridgedInheritedType,
    size_t index) {
  auto eachLoc = unbridged(cEachLoc);
  auto *decl = GenericTypeParamDecl::createParsed(
      unbridged(cDeclContext), unbridged(cName), unbridged(cNameLoc), eachLoc,
      index,
      /*isParameterPack*/ eachLoc.isValid());

  if (auto *inheritedType = unbridged(bridgedInheritedType)) {
    auto entry = InheritedEntry(inheritedType);
    ASTContext &context = unbridged(cContext);
    decl->setInherited(context.AllocateCopy(llvm::makeArrayRef(entry)));
  }

  return bridged(decl);
}

BridgedTrailingWhereClause
TrailingWhereClause_createParsed(BridgedASTContext cContext,
                                 BridgedSourceLoc cWhereKeywordLoc,
                                 BridgedArrayRef cRequirements) {
  SmallVector<RequirementRepr> requirements;
  for (auto &cReq : unbridgedArrayRef<BridgedRequirementRepr>(cRequirements)) {
    switch (cReq.Kind) {
    case BridgedRequirementReprKindTypeConstraint:
      requirements.push_back(RequirementRepr::getTypeConstraint(
          unbridged(cReq.FirstType), unbridged(cReq.SeparatorLoc),
          unbridged(cReq.SecondType),
          /*isExpansionPattern*/ false));
      break;
    case BridgedRequirementReprKindSameType:
      requirements.push_back(RequirementRepr::getSameType(
          unbridged(cReq.FirstType), unbridged(cReq.SeparatorLoc),
          unbridged(cReq.SecondType),
          /*isExpansionPattern*/ false));
      break;
    case BridgedRequirementReprKindLayoutConstraint:
      llvm_unreachable("cannot handle layout constraints!");
    }
  }

  SourceLoc whereKeywordLoc = unbridged(cWhereKeywordLoc);
  SourceLoc endLoc;
  if (requirements.empty()) {
    endLoc = whereKeywordLoc;
  } else {
    endLoc = requirements.back().getSourceRange().End;
  }

  return bridged(TrailingWhereClause::create(
      unbridged(cContext), whereKeywordLoc, endLoc, requirements));
}

BridgedParameterList ParameterList_createParsed(
    BridgedASTContext cContext, BridgedSourceLoc cLeftParenLoc,
    BridgedArrayRef cParameters, BridgedSourceLoc cRightParenLoc) {
  ASTContext &context = unbridged(cContext);
  return bridged(ParameterList::create(
      context, unbridged(cLeftParenLoc),
      unbridgedArrayRef<ParamDecl *>(cParameters), unbridged(cRightParenLoc)));
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

void TopLevelCodeDecl_dump(void *decl) {
  static_cast<TopLevelCodeDecl *>(decl)->dump(llvm::errs());
}
void Expr_dump(void *expr) { static_cast<Expr *>(expr)->dump(llvm::errs()); }
void Decl_dump(void *decl) { static_cast<Decl *>(decl)->dump(llvm::errs()); }
void Stmt_dump(void *statement) {
  static_cast<Stmt *>(statement)->dump(llvm::errs());
}
void TypeRepr_dump(void *type) { static_cast<TypeRepr *>(type)->dump(); }

#pragma clang diagnostic pop

//===----------------------------------------------------------------------===//
// Plugins
//===----------------------------------------------------------------------===//

PluginCapabilityPtr Plugin_getCapability(PluginHandle handle) {
  auto *plugin = static_cast<LoadedExecutablePlugin *>(handle);
  return plugin->getCapability();
}

void Plugin_setCapability(PluginHandle handle, PluginCapabilityPtr data) {
  auto *plugin = static_cast<LoadedExecutablePlugin *>(handle);
  plugin->setCapability(data);
}

const char *Plugin_getExecutableFilePath(PluginHandle handle) {
  auto *plugin = static_cast<LoadedExecutablePlugin *>(handle);
  return plugin->getExecutablePath().data();
}

void Plugin_lock(PluginHandle handle) {
  auto *plugin = static_cast<LoadedExecutablePlugin *>(handle);
  plugin->lock();
}

void Plugin_unlock(PluginHandle handle) {
  auto *plugin = static_cast<LoadedExecutablePlugin *>(handle);
  plugin->unlock();
}

bool Plugin_spawnIfNeeded(PluginHandle handle) {
  auto *plugin = static_cast<LoadedExecutablePlugin *>(handle);
  auto error = plugin->spawnIfNeeded();
  bool hadError(error);
  llvm::consumeError(std::move(error));
  return hadError;
}

bool Plugin_sendMessage(PluginHandle handle, const BridgedData data) {
  auto *plugin = static_cast<LoadedExecutablePlugin *>(handle);
  StringRef message(data.baseAddress, data.size);
  auto error = plugin->sendMessage(message);
  if (error) {
    // FIXME: Pass the error message back to the caller.
    llvm::consumeError(std::move(error));
//    llvm::handleAllErrors(std::move(error), [](const llvm::ErrorInfoBase &err) {
//      llvm::errs() << err.message() << "\n";
//    });
    return true;
  }
  return false;
}

bool Plugin_waitForNextMessage(PluginHandle handle, BridgedData *out) {
  auto *plugin = static_cast<LoadedExecutablePlugin *>(handle);
  auto result = plugin->waitForNextMessage();
  if (!result) {
    // FIXME: Pass the error message back to the caller.
    llvm::consumeError(result.takeError());
//    llvm::handleAllErrors(result.takeError(), [](const llvm::ErrorInfoBase &err) {
//      llvm::errs() << err.message() << "\n";
//    });
    return true;
  }
  auto &message = result.get();
  auto size = message.size();
  auto outPtr = malloc(size);
  memcpy(outPtr, message.data(), size);
  *out = BridgedData{(const char *)outPtr, size};
  return false;
}
