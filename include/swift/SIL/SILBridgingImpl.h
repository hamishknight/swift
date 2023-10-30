//===--- SILBridgingImpl.h ------------------------------------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2023 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
//
// This file contains the implementation of bridging functions, which are either
// - depending on if PURE_BRIDGING_MODE is set - included in the cpp file or
// in the header file.
//
//===----------------------------------------------------------------------===//

#ifndef SWIFT_SIL_SILBRIDGING_IMPL_H
#define SWIFT_SIL_SILBRIDGING_IMPL_H

#include "swift/AST/Builtins.h"
#include "swift/AST/Decl.h"
#include "swift/AST/SubstitutionMap.h"
#include "swift/Basic/BasicBridging.h"
#include "swift/Basic/Nullability.h"
#include "swift/SIL/ApplySite.h"
#include "swift/SIL/InstWrappers.h"
#include "swift/SIL/SILBuilder.h"
#include "swift/SIL/SILDefaultWitnessTable.h"
#include "swift/SIL/SILFunctionConventions.h"
#include "swift/SIL/SILInstruction.h"
#include "swift/SIL/SILLocation.h"
#include "swift/SIL/SILModule.h"
#include "swift/SIL/SILVTable.h"
#include "swift/SIL/SILWitnessTable.h"
#include <stdbool.h>
#include <stddef.h>
#include <string>

SWIFT_BEGIN_NULLABILITY_ANNOTATIONS

//===----------------------------------------------------------------------===//
//                                BridgedType
//===----------------------------------------------------------------------===//

BridgedOwnedString BridgedType::getDebugDescription() const {
  return BridgedOwnedString(get().getDebugDescription());
}

bool BridgedType::isNull() const {
  return get().isNull();
}

bool BridgedType::isAddress() const {
  return get().isAddress();
}

BridgedType BridgedType::getAddressType() const {
  return get().getAddressType();
}

BridgedType BridgedType::getObjectType() const {
  return get().getObjectType();
}

bool BridgedType::isTrivial(BridgedFunction f) const {
  return get().isTrivial(f.getFunction());
}

bool BridgedType::isNonTrivialOrContainsRawPointer(BridgedFunction f) const {
  return get().isNonTrivialOrContainsRawPointer(f.getFunction());
}

bool BridgedType::isValueTypeWithDeinit() const {
  return get().isValueTypeWithDeinit();
}

bool BridgedType::isLoadable(BridgedFunction f) const {
  return get().isLoadable(f.getFunction());
}

bool BridgedType::isReferenceCounted(BridgedFunction f) const {
  return get().isReferenceCounted(f.getFunction());
}

bool BridgedType::isUnownedStorageType() const {
  return get().isUnownedStorageType();
}

bool BridgedType::hasArchetype() const {
  return get().hasArchetype();
}

bool BridgedType::isNominalOrBoundGenericNominal() const {
  return get().getNominalOrBoundGenericNominal() != nullptr;
}

BridgedNominalTypeDecl BridgedType::getNominalOrBoundGenericNominal() const {
  return {get().getNominalOrBoundGenericNominal()};
}

bool BridgedType::isClassOrBoundGenericClass() const {
  return get().getClassOrBoundGenericClass() != 0;
}

bool BridgedType::isStructOrBoundGenericStruct() const {
  return get().getStructOrBoundGenericStruct() != nullptr;
}

bool BridgedType::isTuple() const {
  return get().isTuple();
}

bool BridgedType::isEnumOrBoundGenericEnum() const {
  return get().getEnumOrBoundGenericEnum() != nullptr;
}

bool BridgedType::isFunction() const {
  return get().isFunction();
}

bool BridgedType::isMetatype() const {
  return get().isMetatype();
}

bool BridgedType::isNoEscapeFunction() const {
  return get().isNoEscapeFunction();
}

bool BridgedType::isAsyncFunction() const {
  return get().isAsyncFunction();
}

BridgedType::TraitResult BridgedType::canBeClass() const {
  return (TraitResult)get().canBeClass();
}

bool BridgedType::isMoveOnly() const {
  return get().isMoveOnly();
}

bool BridgedType::isEscapable() const {
  return get().isEscapable();
}

bool BridgedType::isOrContainsObjectiveCClass() const {
  return get().isOrContainsObjectiveCClass();
}

bool BridgedType::isBuiltinInteger() const {
  return get().isBuiltinInteger();
}

bool BridgedType::isBuiltinFloat() const {
  return get().isBuiltinFloat();
}

bool BridgedType::isBuiltinVector() const {
  return get().isBuiltinVector();
}

BridgedType BridgedType::getBuiltinVectorElementType() const {
  return get().getBuiltinVectorElementType();
}

bool BridgedType::isBuiltinFixedWidthInteger(SwiftInt width) const {
  return get().isBuiltinFixedWidthInteger((unsigned)width);
}

bool BridgedType::isExactSuperclassOf(BridgedType t) const {
  return get().isExactSuperclassOf(t.get());
}

BridgedType BridgedType::getInstanceTypeOfMetatype(BridgedFunction f) const {
  return get().getInstanceTypeOfMetatype(f.getFunction());
}

BridgedType::MetatypeRepresentation BridgedType::getRepresentationOfMetatype(BridgedFunction f) const {
  return BridgedType::MetatypeRepresentation(get().getRepresentationOfMetatype(f.getFunction()));
}

bool BridgedType::isCalleeConsumedFunction() const {
  return get().isCalleeConsumedFunction();
}

bool BridgedType::isMarkedAsImmortal() const {
  return get().isMarkedAsImmortal();
}

SwiftInt BridgedType::getCaseIdxOfEnumType(BridgedStringRef name) const {
  return get().getCaseIdxOfEnumType(name.get());
}

SwiftInt BridgedType::getNumNominalFields() const {
  return get().getNumNominalFields();
}


BridgedType BridgedType::getFieldType(SwiftInt idx, BridgedFunction f) const {
  return get().getFieldType(idx, f.getFunction());
}

SwiftInt BridgedType::getFieldIdxOfNominalType(BridgedStringRef name) const {
  return get().getFieldIdxOfNominalType(name.get());
}

BridgedStringRef BridgedType::getFieldName(SwiftInt idx) const {
  return get().getFieldName(idx);
}

SwiftInt BridgedType::getNumTupleElements() const {
  return get().getNumTupleElements();
}

BridgedType BridgedType::getTupleElementType(SwiftInt idx) const {
  return get().getTupleElementType(idx);
}

BridgedType BridgedType::getFunctionTypeWithNoEscape(bool withNoEscape) const {
  auto fnType = get().getAs<swift::SILFunctionType>();
  auto newTy = fnType->getWithExtInfo(fnType->getExtInfo().withNoEscape(true));
  return swift::SILType::getPrimitiveObjectType(newTy);
}

//===----------------------------------------------------------------------===//
//                                BridgedValue
//===----------------------------------------------------------------------===//

inline BridgedValue::Ownership castOwnership(swift::OwnershipKind ownership) {
  switch (ownership) {
    case swift::OwnershipKind::Any:
      llvm_unreachable("Invalid ownership for value");
    case swift::OwnershipKind::Unowned:    return BridgedValue::Ownership::Unowned;
    case swift::OwnershipKind::Owned:      return BridgedValue::Ownership::Owned;
    case swift::OwnershipKind::Guaranteed: return BridgedValue::Ownership::Guaranteed;
    case swift::OwnershipKind::None:       return BridgedValue::Ownership::None;
  }
}

swift::ValueBase * _Nonnull BridgedValue::getSILValue() const {
  return static_cast<swift::ValueBase *>(obj);
}

swift::ValueBase * _Nullable OptionalBridgedValue::getSILValue() const {
  if (obj)
    return static_cast<swift::ValueBase *>(obj);
  return nullptr;
}

OptionalBridgedOperand BridgedValue::getFirstUse() const {
  return {*getSILValue()->use_begin()};
}

BridgedType BridgedValue::getType() const {
  return getSILValue()->getType();
}

BridgedValue::Ownership BridgedValue::getOwnership() const {
  return castOwnership(getSILValue()->getOwnershipKind());
}

//===----------------------------------------------------------------------===//
//                                BridgedOperand
//===----------------------------------------------------------------------===//

bool BridgedOperand::isTypeDependent() const { return op->isTypeDependent(); }

bool BridgedOperand::isLifetimeEnding() const { return op->isLifetimeEnding(); }

OptionalBridgedOperand BridgedOperand::getNextUse() const {
  return {op->getNextUse()};
}

BridgedValue BridgedOperand::getValue() const { return {op->get()}; }

BridgedInstruction BridgedOperand::getUser() const {
  return {op->getUser()->asSILNode()};
}

BridgedOperand::OperandOwnership BridgedOperand::getOperandOwnership() const {
  switch (op->getOperandOwnership()) {
  case swift::OperandOwnership::NonUse:
    return OperandOwnership::NonUse;
  case swift::OperandOwnership::TrivialUse:
    return OperandOwnership::TrivialUse;
  case swift::OperandOwnership::InstantaneousUse:
    return OperandOwnership::InstantaneousUse;
  case swift::OperandOwnership::UnownedInstantaneousUse:
    return OperandOwnership::UnownedInstantaneousUse;
  case swift::OperandOwnership::ForwardingUnowned:
    return OperandOwnership::ForwardingUnowned;
  case swift::OperandOwnership::PointerEscape:
    return OperandOwnership::PointerEscape;
  case swift::OperandOwnership::BitwiseEscape:
    return OperandOwnership::BitwiseEscape;
  case swift::OperandOwnership::Borrow:
    return OperandOwnership::Borrow;
  case swift::OperandOwnership::DestroyingConsume:
    return OperandOwnership::DestroyingConsume;
  case swift::OperandOwnership::ForwardingConsume:
    return OperandOwnership::ForwardingConsume;
  case swift::OperandOwnership::InteriorPointer:
    return OperandOwnership::InteriorPointer;
  case swift::OperandOwnership::GuaranteedForwarding:
    return OperandOwnership::GuaranteedForwarding;
  case swift::OperandOwnership::EndBorrow:
    return OperandOwnership::EndBorrow;
  case swift::OperandOwnership::Reborrow:
    return OperandOwnership::Reborrow;
  }
}

BridgedOperand OptionalBridgedOperand::advancedBy(SwiftInt index) const { return {op + index}; }

// Assumes that `op` is not null.
SwiftInt OptionalBridgedOperand::distanceTo(BridgedOperand element) const { return element.op - op; }

//===----------------------------------------------------------------------===//
//                                BridgedArgument
//===----------------------------------------------------------------------===//

inline BridgedArgumentConvention castToArgumentConvention(swift::SILArgumentConvention convention) {
  return static_cast<BridgedArgumentConvention>(convention.Value);
}

swift::SILArgument * _Nonnull BridgedArgument::getArgument() const {
  return static_cast<swift::SILArgument *>(obj);
}

BridgedBasicBlock BridgedArgument::getParent() const {
  return {getArgument()->getParent()};
}

BridgedArgumentConvention BridgedArgument::getConvention() const {
  auto *fArg = llvm::cast<swift::SILFunctionArgument>(getArgument());
  return castToArgumentConvention(fArg->getArgumentConvention());
}

bool BridgedArgument::isSelf() const {
  auto *fArg = llvm::cast<swift::SILFunctionArgument>(getArgument());
  return fArg->isSelf();
}

//===----------------------------------------------------------------------===//
//                            BridgedSubstitutionMap
//===----------------------------------------------------------------------===//

BridgedSubstitutionMap::BridgedSubstitutionMap() : BridgedSubstitutionMap(swift::SubstitutionMap()) {
}

bool BridgedSubstitutionMap::isEmpty() const {
  return get().empty();
}

//===----------------------------------------------------------------------===//
//                                BridgedLocation
//===----------------------------------------------------------------------===//

BridgedLocation BridgedLocation::getAutogeneratedLocation() const {
  return getLoc().getAutogeneratedLocation();
}
bool BridgedLocation::hasValidLineNumber() const {
  return getLoc().hasValidLineNumber();
}
bool BridgedLocation::isAutoGenerated() const {
  return getLoc().isAutoGenerated();
}
bool BridgedLocation::isEqualTo(BridgedLocation rhs) const {
  return getLoc().isEqualTo(rhs.getLoc());
}
BridgedSourceLoc BridgedLocation::getSourceLocation() const {
  swift::SILDebugLocation debugLoc = getLoc();
  swift::SILLocation silLoc = debugLoc.getLocation();
  swift::SourceLoc sourceLoc = silLoc.getSourceLoc();
  return BridgedSourceLoc(sourceLoc.getOpaquePointerValue());
}
bool BridgedLocation::hasSameSourceLocation(BridgedLocation rhs) const {
  return getLoc().hasSameSourceLocation(rhs.getLoc());
}
BridgedLocation BridgedLocation::getArtificialUnreachableLocation() {
  return swift::SILDebugLocation::getArtificialUnreachableLocation();
}

//===----------------------------------------------------------------------===//
//                                BridgedFunction
//===----------------------------------------------------------------------===//

swift::SILFunction * _Nonnull BridgedFunction::getFunction() const {
  return static_cast<swift::SILFunction *>(obj);
}

BridgedStringRef BridgedFunction::getName() const {
  return getFunction()->getName();
}

bool BridgedFunction::hasOwnership() const { return getFunction()->hasOwnership(); }

OptionalBridgedBasicBlock BridgedFunction::getFirstBlock() const {
  return {getFunction()->empty() ? nullptr : getFunction()->getEntryBlock()};
}

OptionalBridgedBasicBlock BridgedFunction::getLastBlock() const {
  return {getFunction()->empty() ? nullptr : &*getFunction()->rbegin()};
}

SwiftInt BridgedFunction::getNumIndirectFormalResults() const {
  return (SwiftInt)getFunction()->getLoweredFunctionType()->getNumIndirectFormalResults();
}

SwiftInt BridgedFunction::getNumParameters() const {
  return (SwiftInt)getFunction()->getLoweredFunctionType()->getNumParameters();
}

SwiftInt BridgedFunction::getSelfArgumentIndex() const {
  swift::SILFunctionConventions conv(getFunction()->getConventionsInContext());
  swift::CanSILFunctionType fTy = getFunction()->getLoweredFunctionType();
  if (!fTy->hasSelfParam())
    return -1;
  return conv.getNumParameters() + conv.getNumIndirectSILResults() - 1;
}

SwiftInt BridgedFunction::getNumSILArguments() const {
  return swift::SILFunctionConventions(getFunction()->getConventionsInContext()).getNumSILArguments();
}

BridgedType BridgedFunction::getSILArgumentType(SwiftInt idx) const {
  swift::SILFunctionConventions conv(getFunction()->getConventionsInContext());
  return conv.getSILArgumentType(idx, getFunction()->getTypeExpansionContext());
}

BridgedArgumentConvention BridgedFunction::getSILArgumentConvention(SwiftInt idx) const {
  swift::SILFunctionConventions conv(getFunction()->getConventionsInContext());
  return castToArgumentConvention(conv.getSILArgumentConvention(idx));
}

BridgedType BridgedFunction::getSILResultType() const {
  swift::SILFunctionConventions conv(getFunction()->getConventionsInContext());
  return conv.getSILResultType(getFunction()->getTypeExpansionContext());
}

bool BridgedFunction::isSwift51RuntimeAvailable() const {
  if (getFunction()->getResilienceExpansion() != swift::ResilienceExpansion::Maximal)
    return false;

  swift::ASTContext &ctxt = getFunction()->getModule().getASTContext();
  return swift::AvailabilityContext::forDeploymentTarget(ctxt).isContainedIn(ctxt.getSwift51Availability());
}

bool BridgedFunction::isPossiblyUsedExternally() const {
  return getFunction()->isPossiblyUsedExternally();
}

bool BridgedFunction::isAvailableExternally() const {
  return getFunction()->isAvailableExternally();
}

bool BridgedFunction::isTransparent() const {
  return getFunction()->isTransparent() == swift::IsTransparent;
}

bool BridgedFunction::isAsync() const {
  return getFunction()->isAsync();
}

bool BridgedFunction::isGlobalInitFunction() const {
  return getFunction()->isGlobalInit();
}

bool BridgedFunction::isGlobalInitOnceFunction() const {
  return getFunction()->isGlobalInitOnceFunction();
}

bool BridgedFunction::isDestructor() const {
  if (auto *declCtxt = getFunction()->getDeclContext()) {
    return llvm::isa<swift::DestructorDecl>(declCtxt);
  }
  return false;
}

bool BridgedFunction::isGenericFunction() const {
  return !getFunction()->getGenericSignature().isNull();
}

bool BridgedFunction::hasSemanticsAttr(BridgedStringRef attrName) const {
  return getFunction()->hasSemanticsAttr(attrName.get());
}

bool BridgedFunction::hasUnsafeNonEscapableResult() const {
  return getFunction()->hasUnsafeNonEscapableResult();
}

BridgedFunction::EffectsKind BridgedFunction::getEffectAttribute() const {
  return (EffectsKind)getFunction()->getEffectsKind();
}

BridgedFunction::PerformanceConstraints BridgedFunction::getPerformanceConstraints() const {
  return (PerformanceConstraints)getFunction()->getPerfConstraints();
}

BridgedFunction::InlineStrategy BridgedFunction::getInlineStrategy() const {
  return (InlineStrategy)getFunction()->getInlineStrategy();
}

bool BridgedFunction::isSerialized() const {
  return getFunction()->isSerialized();
}

bool BridgedFunction::hasValidLinkageForFragileRef() const {
  return getFunction()->hasValidLinkageForFragileRef();
}

bool BridgedFunction::needsStackProtection() const {
  return getFunction()->needsStackProtection();
}

void BridgedFunction::setNeedStackProtection(bool needSP) const {
  getFunction()->setNeedStackProtection(needSP);
}


//===----------------------------------------------------------------------===//
//                                BridgedGlobalVar
//===----------------------------------------------------------------------===//

swift::SILGlobalVariable * _Nonnull BridgedGlobalVar::getGlobal() const {
  return static_cast<swift::SILGlobalVariable *>(obj);
}

BridgedStringRef BridgedGlobalVar::getName() const {
  return getGlobal()->getName();
}

bool BridgedGlobalVar::isLet() const { return getGlobal()->isLet(); }

void BridgedGlobalVar::setLet(bool value) const { getGlobal()->setLet(value); }

bool BridgedGlobalVar::isPossiblyUsedExternally() const {
  return getGlobal()->isPossiblyUsedExternally();
}

bool BridgedGlobalVar::isAvailableExternally() const {
  return swift::isAvailableExternally(getGlobal()->getLinkage());
}

OptionalBridgedInstruction BridgedGlobalVar::getFirstStaticInitInst() const {
  if (getGlobal()->begin() == getGlobal()->end()) {
    return {nullptr};
  }
  swift::SILInstruction *firstInst = &*getGlobal()->begin();
  return {firstInst->asSILNode()};
}

//===----------------------------------------------------------------------===//
//                                BridgedMultiValueResult
//===----------------------------------------------------------------------===//

BridgedInstruction BridgedMultiValueResult::getParent() const {
  return {get()->getParent()};
}

SwiftInt BridgedMultiValueResult::getIndex() const {
  return (SwiftInt)get()->getIndex();
}

//===----------------------------------------------------------------------===//
//                                BridgedTypeArray
//===----------------------------------------------------------------------===//

BridgedTypeArray BridgedTypeArray::fromReplacementTypes(BridgedSubstitutionMap substMap) {
  swift::ArrayRef<swift::Type> replTypes = substMap.get().getReplacementTypes();
  return {replTypes.data(), replTypes.size()};
}

BridgedType BridgedTypeArray::getAt(SwiftInt index) const {
  assert((size_t)index < typeArray.numElements);
  swift::Type origTy = ((const swift::Type *)typeArray.data)[index];
  auto ty = origTy->getCanonicalType();
  if (ty->isLegalSILType())
    return swift::SILType::getPrimitiveObjectType(ty);
  return swift::SILType();
}

//===----------------------------------------------------------------------===//
//                                BridgedTypeArray
//===----------------------------------------------------------------------===//

BridgedType BridgedSILTypeArray::getAt(SwiftInt index) const {
  assert((size_t)index < typeArray.numElements);
  return ((const swift::SILType *)typeArray.data)[index];
}

//===----------------------------------------------------------------------===//
//                                BridgedInstruction
//===----------------------------------------------------------------------===//

OptionalBridgedInstruction BridgedInstruction::getNext() const {
  auto iter = std::next(get()->getIterator());
  if (iter == get()->getParent()->end())
    return {nullptr};
  return {iter->asSILNode()};
}

OptionalBridgedInstruction BridgedInstruction::getPrevious() const {
  auto iter = std::next(get()->getReverseIterator());
  if (iter == get()->getParent()->rend())
    return {nullptr};
  return {iter->asSILNode()};
}

BridgedBasicBlock BridgedInstruction::getParent() const {
  assert(!get()->isStaticInitializerInst() &&
         "cannot get the parent of a static initializer instruction");
  return {get()->getParent()};
}

BridgedInstruction BridgedInstruction::getLastInstOfParent() const {
  return {get()->getParent()->back().asSILNode()};
}

bool BridgedInstruction::isDeleted() const {
  return get()->isDeleted();
}

BridgedOperandArray BridgedInstruction::getOperands() const {
  auto operands = get()->getAllOperands();
  return {{operands.data()}, (SwiftInt)operands.size()};
}

BridgedOperandArray BridgedInstruction::getTypeDependentOperands() const {
  auto typeOperands = get()->getTypeDependentOperands();
  return {{typeOperands.data()}, (SwiftInt)typeOperands.size()};
}

void BridgedInstruction::setOperand(SwiftInt index, BridgedValue value) const {
  get()->setOperand((unsigned)index, value.getSILValue());
}

BridgedLocation BridgedInstruction::getLocation() const {
  return get()->getDebugLocation();
}

BridgedMemoryBehavior BridgedInstruction::getMemBehavior() const {
  return (BridgedMemoryBehavior)get()->getMemoryBehavior();
}

bool BridgedInstruction::mayRelease() const {
  return get()->mayRelease();
}

bool BridgedInstruction::mayHaveSideEffects() const {
  return get()->mayHaveSideEffects();
}

bool BridgedInstruction::maySuspend() const {
  return get()->maySuspend();
}

SwiftInt BridgedInstruction::MultipleValueInstruction_getNumResults() const {
  return getAs<swift::MultipleValueInstruction>()->getNumResults();
}

BridgedMultiValueResult BridgedInstruction::MultipleValueInstruction_getResult(SwiftInt index) const {
  return {getAs<swift::MultipleValueInstruction>()->getResult(index)};
}

BridgedSuccessorArray BridgedInstruction::TermInst_getSuccessors() const {
  auto successors = getAs<swift::TermInst>()->getSuccessors();
  return {{successors.data()}, (SwiftInt)successors.size()};
}

OptionalBridgedOperand BridgedInstruction::ForwardingInst_singleForwardedOperand() const {
  return {swift::ForwardingOperation(get()).getSingleForwardingOperand()};
}

BridgedOperandArray BridgedInstruction::ForwardingInst_forwardedOperands() const {
  auto operands =
    swift::ForwardingOperation(get()).getForwardedOperands();
  return {{operands.data()}, (SwiftInt)operands.size()};
}

BridgedValue::Ownership BridgedInstruction::ForwardingInst_forwardingOwnership() const {
  auto *forwardingInst = swift::ForwardingInstruction::get(get());
  return castOwnership(forwardingInst->getForwardingOwnershipKind());
}

bool BridgedInstruction::ForwardingInst_preservesOwnership() const {
  return swift::ForwardingInstruction::get(get())->preservesOwnership();
}

BridgedStringRef BridgedInstruction::CondFailInst_getMessage() const {
  return getAs<swift::CondFailInst>()->getMessage();
}

SwiftInt BridgedInstruction::LoadInst_getLoadOwnership() const {
  return (SwiftInt)getAs<swift::LoadInst>()->getOwnershipQualifier();
}

BridgedInstruction::BuiltinValueKind BridgedInstruction::BuiltinInst_getID() const {
  return (BuiltinValueKind)getAs<swift::BuiltinInst>()->getBuiltinInfo().ID;
}

BridgedInstruction::IntrinsicID BridgedInstruction::BuiltinInst_getIntrinsicID() const {
  switch (getAs<swift::BuiltinInst>()->getIntrinsicInfo().ID) {
    case llvm::Intrinsic::memcpy:  return IntrinsicID::memcpy;
    case llvm::Intrinsic::memmove: return IntrinsicID::memmove;
    default: return IntrinsicID::unknown;
  }
}

BridgedSubstitutionMap BridgedInstruction::BuiltinInst_getSubstitutionMap() const {
  return getAs<swift::BuiltinInst>()->getSubstitutions();
}

bool BridgedInstruction::PointerToAddressInst_isStrict() const {
  return getAs<swift::PointerToAddressInst>()->isStrict();
}

bool BridgedInstruction::AddressToPointerInst_needsStackProtection() const {
  return getAs<swift::AddressToPointerInst>()->needsStackProtection();
}

bool BridgedInstruction::IndexAddrInst_needsStackProtection() const {
  return getAs<swift::IndexAddrInst>()->needsStackProtection();
}

BridgedGlobalVar BridgedInstruction::GlobalAccessInst_getGlobal() const {
  return {getAs<swift::GlobalAccessInst>()->getReferencedGlobal()};
}

BridgedGlobalVar BridgedInstruction::AllocGlobalInst_getGlobal() const {
  return {getAs<swift::AllocGlobalInst>()->getReferencedGlobal()};
}

BridgedFunction BridgedInstruction::FunctionRefBaseInst_getReferencedFunction() const {
  return {getAs<swift::FunctionRefBaseInst>()->getInitiallyReferencedFunction()};
}

BridgedInstruction::OptionalInt BridgedInstruction::IntegerLiteralInst_getValue() const {
  llvm::APInt result = getAs<swift::IntegerLiteralInst>()->getValue();
  if (result.getSignificantBits() <= std::min(std::numeric_limits<SwiftInt>::digits, 64)) {
    return {(SwiftInt)result.getSExtValue(), true};
  }
  return {0, false};
}

BridgedStringRef BridgedInstruction::StringLiteralInst_getValue() const {
  return getAs<swift::StringLiteralInst>()->getValue();
}

int BridgedInstruction::StringLiteralInst_getEncoding() const {
  return (int)getAs<swift::StringLiteralInst>()->getEncoding();
}

SwiftInt BridgedInstruction::TupleExtractInst_fieldIndex() const {
  return getAs<swift::TupleExtractInst>()->getFieldIndex();
}

SwiftInt BridgedInstruction::TupleElementAddrInst_fieldIndex() const {
  return getAs<swift::TupleElementAddrInst>()->getFieldIndex();
}

SwiftInt BridgedInstruction::StructExtractInst_fieldIndex() const {
  return getAs<swift::StructExtractInst>()->getFieldIndex();
}

OptionalBridgedValue BridgedInstruction::StructInst_getUniqueNonTrivialFieldValue() const {
  return {getAs<swift::StructInst>()->getUniqueNonTrivialFieldValue()};
}

SwiftInt BridgedInstruction::StructElementAddrInst_fieldIndex() const {
  return getAs<swift::StructElementAddrInst>()->getFieldIndex();
}

SwiftInt BridgedInstruction::ProjectBoxInst_fieldIndex() const {
  return getAs<swift::ProjectBoxInst>()->getFieldIndex();
}

bool BridgedInstruction::EndCOWMutationInst_doKeepUnique() const {
  return getAs<swift::EndCOWMutationInst>()->doKeepUnique();
}

SwiftInt BridgedInstruction::EnumInst_caseIndex() const {
  return getAs<swift::EnumInst>()->getCaseIndex();
}

SwiftInt BridgedInstruction::UncheckedEnumDataInst_caseIndex() const {
  return getAs<swift::UncheckedEnumDataInst>()->getCaseIndex();
}

SwiftInt BridgedInstruction::InitEnumDataAddrInst_caseIndex() const {
  return getAs<swift::InitEnumDataAddrInst>()->getCaseIndex();
}

SwiftInt BridgedInstruction::UncheckedTakeEnumDataAddrInst_caseIndex() const {
  return getAs<swift::UncheckedTakeEnumDataAddrInst>()->getCaseIndex();
}

SwiftInt BridgedInstruction::InjectEnumAddrInst_caseIndex() const {
  return getAs<swift::InjectEnumAddrInst>()->getCaseIndex();
}

SwiftInt BridgedInstruction::RefElementAddrInst_fieldIndex() const {
  return getAs<swift::RefElementAddrInst>()->getFieldIndex();
}

bool BridgedInstruction::RefElementAddrInst_fieldIsLet() const {
  return getAs<swift::RefElementAddrInst>()->getField()->isLet();
}

bool BridgedInstruction::RefElementAddrInst_isImmutable() const {
  return getAs<swift::RefElementAddrInst>()->isImmutable();
}

void BridgedInstruction::RefElementAddrInst_setImmutable(bool isImmutable) const {
  getAs<swift::RefElementAddrInst>()->setImmutable(isImmutable);
}

SwiftInt BridgedInstruction::PartialApplyInst_numArguments() const {
  return getAs<swift::PartialApplyInst>()->getNumArguments();
}

SwiftInt BridgedInstruction::ApplyInst_numArguments() const {
  return getAs<swift::ApplyInst>()->getNumArguments();
}

bool BridgedInstruction::ApplyInst_getNonThrowing() const {
  return getAs<swift::ApplyInst>()->isNonThrowing();
}

bool BridgedInstruction::ApplyInst_getNonAsync() const {
  return getAs<swift::ApplyInst>()->isNonAsync();
}

BridgedGenericSpecializationInformation BridgedInstruction::ApplyInst_getSpecializationInfo() const {
  return {getAs<swift::ApplyInst>()->getSpecializationInfo()};
}

SwiftInt BridgedInstruction::ObjectInst_getNumBaseElements() const {
  return getAs<swift::ObjectInst>()->getNumBaseElements();
}

SwiftInt BridgedInstruction::PartialApply_getCalleeArgIndexOfFirstAppliedArg() const {
  return swift::ApplySite(get()).getCalleeArgIndexOfFirstAppliedArg();
}

bool BridgedInstruction::PartialApplyInst_isOnStack() const {
  return getAs<swift::PartialApplyInst>()->isOnStack();
}

bool BridgedInstruction::AllocStackInst_hasDynamicLifetime() const {
  return getAs<swift::AllocStackInst>()->hasDynamicLifetime();
}

bool BridgedInstruction::AllocRefInstBase_isObjc() const {
  return getAs<swift::AllocRefInstBase>()->isObjC();
}

bool BridgedInstruction::AllocRefInstBase_canAllocOnStack() const {
  return getAs<swift::AllocRefInstBase>()->canAllocOnStack();
}

SwiftInt BridgedInstruction::AllocRefInstBase_getNumTailTypes() const {
  return getAs<swift::AllocRefInstBase>()->getNumTailTypes();
}

BridgedSILTypeArray BridgedInstruction::AllocRefInstBase_getTailAllocatedTypes() const {
  llvm::ArrayRef<swift::SILType> types = getAs<const swift::AllocRefInstBase>()->getTailAllocatedTypes();
  return {types.data(), types.size()};
}

bool BridgedInstruction::AllocRefDynamicInst_isDynamicTypeDeinitAndSizeKnownEquivalentToBaseType() const {
  return getAs<swift::AllocRefDynamicInst>()->isDynamicTypeDeinitAndSizeKnownEquivalentToBaseType();
}

SwiftInt BridgedInstruction::BeginApplyInst_numArguments() const {
  return getAs<swift::BeginApplyInst>()->getNumArguments();
}

SwiftInt BridgedInstruction::TryApplyInst_numArguments() const {
  return getAs<swift::TryApplyInst>()->getNumArguments();
}

BridgedBasicBlock BridgedInstruction::BranchInst_getTargetBlock() const {
  return {getAs<swift::BranchInst>()->getDestBB()};
}

SwiftInt BridgedInstruction::SwitchEnumInst_getNumCases() const {
  return getAs<swift::SwitchEnumInst>()->getNumCases();
}

SwiftInt BridgedInstruction::SwitchEnumInst_getCaseIndex(SwiftInt idx) const {
  auto *seInst = getAs<swift::SwitchEnumInst>();
  return seInst->getModule().getCaseIndex(seInst->getCase(idx).first);
}

SwiftInt BridgedInstruction::StoreInst_getStoreOwnership() const {
  return (SwiftInt)getAs<swift::StoreInst>()->getOwnershipQualifier();
}

SwiftInt BridgedInstruction::AssignInst_getAssignOwnership() const {
  return (SwiftInt)getAs<swift::AssignInst>()->getOwnershipQualifier();
}

BridgedInstruction::AccessKind BridgedInstruction::BeginAccessInst_getAccessKind() const {
  return (AccessKind)getAs<swift::BeginAccessInst>()->getAccessKind();
}

bool BridgedInstruction::BeginAccessInst_isStatic() const {
  return getAs<swift::BeginAccessInst>()->getEnforcement() == swift::SILAccessEnforcement::Static;
}

bool BridgedInstruction::CopyAddrInst_isTakeOfSrc() const {
  return getAs<swift::CopyAddrInst>()->isTakeOfSrc();
}

bool BridgedInstruction::CopyAddrInst_isInitializationOfDest() const {
  return getAs<swift::CopyAddrInst>()->isInitializationOfDest();
}

SwiftInt BridgedInstruction::MarkUninitializedInst_getKind() const {
  return (SwiftInt)getAs<swift::MarkUninitializedInst>()->getMarkUninitializedKind();
}

void BridgedInstruction::RefCountingInst_setIsAtomic(bool isAtomic) const {
  getAs<swift::RefCountingInst>()->setAtomicity(
      isAtomic ? swift::RefCountingInst::Atomicity::Atomic
               : swift::RefCountingInst::Atomicity::NonAtomic);
}

bool BridgedInstruction::RefCountingInst_getIsAtomic() const {
  return getAs<swift::RefCountingInst>()->getAtomicity() == swift::RefCountingInst::Atomicity::Atomic;
}

SwiftInt BridgedInstruction::CondBranchInst_getNumTrueArgs() const {
  return getAs<swift::CondBranchInst>()->getNumTrueArgs();
}

void BridgedInstruction::AllocRefInstBase_setIsStackAllocatable() const {
  getAs<swift::AllocRefInstBase>()->setStackAllocatable();
}

bool BridgedInstruction::AllocRefInst_isBare() const {
  return getAs<swift::AllocRefInst>()->isBare();
}

void BridgedInstruction::AllocRefInst_setIsBare() const {
  getAs<swift::AllocRefInst>()->setBare(true);
}

void BridgedInstruction::TermInst_replaceBranchTarget(BridgedBasicBlock from, BridgedBasicBlock to) const {
  getAs<swift::TermInst>()->replaceBranchTarget(from.get(), to.get());
}

SwiftInt BridgedInstruction::KeyPathInst_getNumComponents() const {
  if (swift::KeyPathPattern *pattern = getAs<swift::KeyPathInst>()->getPattern()) {
    return (SwiftInt)pattern->getComponents().size();
  }
  return 0;
}

void BridgedInstruction::KeyPathInst_getReferencedFunctions(SwiftInt componentIdx,
                                                            KeyPathFunctionResults * _Nonnull results) const {
  swift::KeyPathPattern *pattern = getAs<swift::KeyPathInst>()->getPattern();
  const swift::KeyPathPatternComponent &comp = pattern->getComponents()[componentIdx];
  results->numFunctions = 0;

  comp.visitReferencedFunctionsAndMethods([results](swift::SILFunction *func) {
      assert(results->numFunctions < KeyPathFunctionResults::maxFunctions);
      results->functions[results->numFunctions++] = {func};
    }, [](swift::SILDeclRef) {});
}

bool BridgedInstruction::GlobalValueInst_isBare() const {
  return getAs<swift::GlobalValueInst>()->isBare();
}

void BridgedInstruction::GlobalValueInst_setIsBare() const {
  getAs<swift::GlobalValueInst>()->setBare(true);
}

void BridgedInstruction::LoadInst_setOwnership(SwiftInt ownership) const {
  getAs<swift::LoadInst>()->setOwnershipQualifier((swift::LoadOwnershipQualifier)ownership);
}

BridgedBasicBlock BridgedInstruction::CheckedCastBranch_getSuccessBlock() const {
  return {getAs<swift::CheckedCastBranchInst>()->getSuccessBB()};
}

BridgedBasicBlock BridgedInstruction::CheckedCastBranch_getFailureBlock() const {
  return {getAs<swift::CheckedCastBranchInst>()->getFailureBB()};
}

BridgedSubstitutionMap BridgedInstruction::ApplySite_getSubstitutionMap() const {
  auto as = swift::ApplySite(get());
  return as.getSubstitutionMap();
}

BridgedArgumentConvention BridgedInstruction::ApplySite_getArgumentConvention(SwiftInt calleeArgIdx) const {
  auto as = swift::ApplySite(get());
  auto conv = as.getSubstCalleeConv().getSILArgumentConvention(calleeArgIdx);
  return castToArgumentConvention(conv.Value);
}

SwiftInt BridgedInstruction::ApplySite_getNumArguments() const {
  return swift::ApplySite(get()).getNumArguments();
}

SwiftInt BridgedInstruction::FullApplySite_numIndirectResultArguments() const {
  auto fas = swift::FullApplySite(get());
  return fas.getNumIndirectSILResults();
}

//===----------------------------------------------------------------------===//
//                     VarDeclInst and DebugVariableInst
//===----------------------------------------------------------------------===//

BridgedNullableVarDecl BridgedInstruction::DebugValue_getDecl() const {
  return {getAs<swift::DebugValueInst>()->getDecl()};
}

BridgedNullableVarDecl BridgedInstruction::AllocStack_getDecl() const {
  return {getAs<swift::AllocStackInst>()->getDecl()};
}

BridgedNullableVarDecl BridgedInstruction::AllocBox_getDecl() const {
  return {getAs<swift::AllocBoxInst>()->getDecl()};
}

BridgedNullableVarDecl BridgedInstruction::GlobalAddr_getDecl() const {
  return {getAs<swift::DebugValueInst>()->getDecl()};
}

BridgedNullableVarDecl BridgedInstruction::RefElementAddr_getDecl() const {
  return {getAs<swift::DebugValueInst>()->getDecl()};
}

OptionalBridgedSILDebugVariable
BridgedInstruction::DebugValue_getVarInfo() const {
  return getAs<swift::DebugValueInst>()->getVarInfo();
}

OptionalBridgedSILDebugVariable
BridgedInstruction::AllocStack_getVarInfo() const {
  return getAs<swift::AllocStackInst>()->getVarInfo();
}

OptionalBridgedSILDebugVariable
BridgedInstruction::AllocBox_getVarInfo() const {
  return getAs<swift::AllocBoxInst>()->getVarInfo();
}

//===----------------------------------------------------------------------===//
//                                BridgedBasicBlock
//===----------------------------------------------------------------------===//

OptionalBridgedBasicBlock BridgedBasicBlock::getNext() const {
  auto iter = std::next(get()->getIterator());
  if (iter == get()->getParent()->end())
    return {nullptr};
  return {&*iter};
}

OptionalBridgedBasicBlock BridgedBasicBlock::getPrevious() const {
  auto iter = std::next(get()->getReverseIterator());
  if (iter == get()->getParent()->rend())
    return {nullptr};
  return {&*iter};
}

BridgedFunction BridgedBasicBlock::getFunction() const {
  return {get()->getParent()};
}

OptionalBridgedInstruction BridgedBasicBlock::getFirstInst() const {
  if (get()->empty())
    return {nullptr};
  return {get()->front().asSILNode()};
}

OptionalBridgedInstruction BridgedBasicBlock::getLastInst() const {
  if (get()->empty())
    return {nullptr};
  return {get()->back().asSILNode()};
}

SwiftInt BridgedBasicBlock::getNumArguments() const {
  return get()->getNumArguments();
}

BridgedArgument BridgedBasicBlock::getArgument(SwiftInt index) const {
  return {get()->getArgument(index)};
}

BridgedArgument BridgedBasicBlock::addBlockArgument(BridgedType type, BridgedValue::Ownership ownership) const {
  return {get()->createPhiArgument(type.get(), BridgedValue::castToOwnership(ownership))};
}

void BridgedBasicBlock::eraseArgument(SwiftInt index) const {
  get()->eraseArgument(index);
}

void BridgedBasicBlock::moveAllInstructionsToBegin(BridgedBasicBlock dest) const {
  dest.get()->spliceAtBegin(get());
}

void BridgedBasicBlock::moveAllInstructionsToEnd(BridgedBasicBlock dest) const {
  dest.get()->spliceAtEnd(get());
}

void BridgedBasicBlock::moveArgumentsTo(BridgedBasicBlock dest) const {
  dest.get()->moveArgumentList(get());
}

OptionalBridgedSuccessor BridgedBasicBlock::getFirstPred() const {
  return {get()->pred_begin().getSuccessorRef()};
}

//===----------------------------------------------------------------------===//
//                                BridgedSuccessor
//===----------------------------------------------------------------------===//

OptionalBridgedSuccessor BridgedSuccessor::getNext() const {
  return {succ->getNext()};
}

BridgedBasicBlock BridgedSuccessor::getTargetBlock() const {
  return succ->getBB();
}

BridgedInstruction BridgedSuccessor::getContainingInst() const {
  return {succ->getContainingInst()};
}

BridgedSuccessor OptionalBridgedSuccessor::advancedBy(SwiftInt index) const {
  return {succ + index};
}

//===----------------------------------------------------------------------===//
//                                BridgedVTable
//===----------------------------------------------------------------------===//

BridgedFunction BridgedVTableEntry::getImplementation() const {
  return {entry->getImplementation()};
}

BridgedVTableEntry BridgedVTableEntry::advanceBy(SwiftInt index) const {
  return {entry + index};
}

BridgedVTableEntryArray BridgedVTable::getEntries() const {
  auto entries = vTable->getEntries();
  return {{entries.data()}, (SwiftInt)entries.size()};
}

//===----------------------------------------------------------------------===//
//               BridgedWitnessTable, BridgedDefaultWitnessTable
//===----------------------------------------------------------------------===//

BridgedWitnessTableEntry::Kind BridgedWitnessTableEntry::getKind() const {
  return (Kind)getEntry()->getKind();
}

OptionalBridgedFunction BridgedWitnessTableEntry::getMethodFunction() const {
  return {getEntry()->getMethodWitness().Witness};
}

BridgedWitnessTableEntry BridgedWitnessTableEntry::advanceBy(SwiftInt index) const {
  return {getEntry() + index};
}

BridgedWitnessTableEntryArray BridgedWitnessTable::getEntries() const {
  auto entries = table->getEntries();
  return {{entries.data()}, (SwiftInt)entries.size()};
}

BridgedWitnessTableEntryArray BridgedDefaultWitnessTable::getEntries() const {
  auto entries = table->getEntries();
  return {{entries.data()}, (SwiftInt)entries.size()};
}

//===----------------------------------------------------------------------===//
//                                BridgedBuilder
//===----------------------------------------------------------------------===//

BridgedInstruction BridgedBuilder::createBuiltinBinaryFunction(BridgedStringRef name,
                                               BridgedType operandType, BridgedType resultType,
                                               BridgedValueArray arguments) const {
  llvm::SmallVector<swift::SILValue, 16> argValues;
  return {get().createBuiltinBinaryFunction(regularLoc(),
                                                name.get(),
                                                operandType.get(), resultType.get(),
                                                arguments.getValues(argValues))};
}

BridgedInstruction BridgedBuilder::createCondFail(BridgedValue condition, BridgedStringRef message) const {
  return {get().createCondFail(regularLoc(), condition.getSILValue(), message.get())};
}

BridgedInstruction BridgedBuilder::createIntegerLiteral(BridgedType type, SwiftInt value) const {
  return {get().createIntegerLiteral(regularLoc(), type.get(), value)};
}

BridgedInstruction BridgedBuilder::createAllocStack(BridgedType type,
                                    bool hasDynamicLifetime, bool isLexical, bool wasMoved) const {
  return {get().createAllocStack(regularLoc(), type.get(), llvm::None,
                                         hasDynamicLifetime, isLexical, wasMoved)};
}

BridgedInstruction BridgedBuilder::createDeallocStack(BridgedValue operand) const {
  return {get().createDeallocStack(regularLoc(), operand.getSILValue())};
}

BridgedInstruction BridgedBuilder::createDeallocStackRef(BridgedValue operand) const {
  return {get().createDeallocStackRef(regularLoc(), operand.getSILValue())};
}

BridgedInstruction BridgedBuilder::createUncheckedRefCast(BridgedValue op, BridgedType type) const {
  return {get().createUncheckedRefCast(regularLoc(), op.getSILValue(), type.get())};
}

BridgedInstruction BridgedBuilder::createUpcast(BridgedValue op, BridgedType type) const {
  return {get().createUpcast(regularLoc(), op.getSILValue(), type.get())};
}

BridgedInstruction BridgedBuilder::createLoad(BridgedValue op, SwiftInt ownership) const {
  return {get().createLoad(regularLoc(), op.getSILValue(), (swift::LoadOwnershipQualifier)ownership)};
}

BridgedInstruction BridgedBuilder::createBeginDeallocRef(BridgedValue reference, BridgedValue allocation) const {
  return {get().createBeginDeallocRef(regularLoc(), reference.getSILValue(), allocation.getSILValue())};
}

BridgedInstruction BridgedBuilder::createEndInitLetRef(BridgedValue op) const {
  return {get().createEndInitLetRef(regularLoc(), op.getSILValue())};
}

BridgedInstruction BridgedBuilder::createStrongRetain(BridgedValue op) const {
  auto b = get();
  return {b.createStrongRetain(regularLoc(), op.getSILValue(), b.getDefaultAtomicity())};
}

BridgedInstruction BridgedBuilder::createStrongRelease(BridgedValue op) const {
  auto b = get();
  return {b.createStrongRelease(regularLoc(), op.getSILValue(), b.getDefaultAtomicity())};
}

BridgedInstruction BridgedBuilder::createUnownedRetain(BridgedValue op) const {
  auto b = get();
  return {b.createUnownedRetain(regularLoc(), op.getSILValue(), b.getDefaultAtomicity())};
}

BridgedInstruction BridgedBuilder::createUnownedRelease(BridgedValue op) const {
  auto b = get();
  return {b.createUnownedRelease(regularLoc(), op.getSILValue(), b.getDefaultAtomicity())};
}

BridgedInstruction BridgedBuilder::createFunctionRef(BridgedFunction function) const {
  return {get().createFunctionRef(regularLoc(), function.getFunction())};
}

BridgedInstruction BridgedBuilder::createCopyValue(BridgedValue op) const {
  return {get().createCopyValue(regularLoc(), op.getSILValue())};
}

BridgedInstruction BridgedBuilder::createBeginBorrow(BridgedValue op) const {
  return {get().createBeginBorrow(regularLoc(), op.getSILValue())};
}

BridgedInstruction BridgedBuilder::createEndBorrow(BridgedValue op) const {
  return {get().createEndBorrow(regularLoc(), op.getSILValue())};
}

BridgedInstruction BridgedBuilder::createCopyAddr(BridgedValue from, BridgedValue to,
                                  bool takeSource, bool initializeDest) const {
  return {get().createCopyAddr(regularLoc(),
                                   from.getSILValue(), to.getSILValue(),
                                   swift::IsTake_t(takeSource),
                                   swift::IsInitialization_t(initializeDest))};
}

BridgedInstruction BridgedBuilder::createDestroyValue(BridgedValue op) const {
  return {get().createDestroyValue(regularLoc(), op.getSILValue())};
}

BridgedInstruction BridgedBuilder::createDestroyAddr(BridgedValue op) const {
  return {get().createDestroyAddr(regularLoc(), op.getSILValue())};
}

BridgedInstruction BridgedBuilder::createDebugStep() const {
  return {get().createDebugStep(regularLoc())};
}

BridgedInstruction BridgedBuilder::createApply(BridgedValue function, BridgedSubstitutionMap subMap,
                               BridgedValueArray arguments, bool isNonThrowing, bool isNonAsync,
                               BridgedGenericSpecializationInformation specInfo) const {
  llvm::SmallVector<swift::SILValue, 16> argValues;
  swift::ApplyOptions applyOpts;
  if (isNonThrowing) { applyOpts |= swift::ApplyFlags::DoesNotThrow; }
  if (isNonAsync) { applyOpts |= swift::ApplyFlags::DoesNotAwait; }

  return {get().createApply(regularLoc(),
                                function.getSILValue(), subMap.get(),
                                arguments.getValues(argValues),
                                applyOpts, specInfo.data)};
}

BridgedInstruction BridgedBuilder::createSwitchEnumInst(BridgedValue enumVal, OptionalBridgedBasicBlock defaultBlock,
                                        const void * _Nullable enumCases, SwiftInt numEnumCases) const {
  using BridgedCase = const std::pair<SwiftInt, BridgedBasicBlock>;
  llvm::ArrayRef<BridgedCase> cases(static_cast<BridgedCase *>(enumCases),
                                    (unsigned)numEnumCases);
  llvm::SmallDenseMap<SwiftInt, swift::EnumElementDecl *> mappedElements;
  swift::SILValue en = enumVal.getSILValue();
  swift::EnumDecl *enumDecl = en->getType().getEnumOrBoundGenericEnum();
  for (auto elemWithIndex : llvm::enumerate(enumDecl->getAllElements())) {
    mappedElements[elemWithIndex.index()] = elemWithIndex.value();
  }
  llvm::SmallVector<std::pair<swift::EnumElementDecl *, swift::SILBasicBlock *>, 16> convertedCases;
  for (auto c : cases) {
    assert(mappedElements.count(c.first) && "wrong enum element index");
    convertedCases.push_back({mappedElements[c.first], c.second.get()});
  }
  return {get().createSwitchEnum(regularLoc(),
                                       enumVal.getSILValue(),
                                       defaultBlock.get(), convertedCases)};
}

BridgedInstruction BridgedBuilder::createUncheckedEnumData(BridgedValue enumVal, SwiftInt caseIdx,
                                           BridgedType resultType) const {
  swift::SILValue en = enumVal.getSILValue();
  return {get().createUncheckedEnumData(regularLoc(), enumVal.getSILValue(),
                                            en->getType().getEnumElement(caseIdx), resultType.get())};
}

BridgedInstruction BridgedBuilder::createEnum(SwiftInt caseIdx, OptionalBridgedValue payload,
                              BridgedType resultType) const {
  swift::EnumElementDecl *caseDecl = resultType.get().getEnumElement(caseIdx);
  swift::SILValue pl = payload.getSILValue();
  return {get().createEnum(regularLoc(), pl, caseDecl, resultType.get())};
}

BridgedInstruction BridgedBuilder::createThinToThickFunction(BridgedValue fn, BridgedType resultType) const {
  return {get().createThinToThickFunction(regularLoc(), fn.getSILValue(), resultType.get())};
}

BridgedInstruction BridgedBuilder::createBranch(BridgedBasicBlock destBlock, BridgedValueArray arguments) const {
  llvm::SmallVector<swift::SILValue, 16> argValues;
  return {get().createBranch(regularLoc(), destBlock.get(), arguments.getValues(argValues))};
}

BridgedInstruction BridgedBuilder::createUnreachable() const {
  return {get().createUnreachable(regularLoc())};
}

BridgedInstruction BridgedBuilder::createObject(BridgedType type,
                                                BridgedValueArray arguments,
                                                SwiftInt numBaseElements) const {
  llvm::SmallVector<swift::SILValue, 16> argValues;
  return {get().createObject(swift::ArtificialUnreachableLocation(),
                                 type.get(), arguments.getValues(argValues), numBaseElements)};
}

BridgedInstruction BridgedBuilder::createGlobalAddr(BridgedGlobalVar global) const {
  return {get().createGlobalAddr(regularLoc(), global.getGlobal())};
}

BridgedInstruction BridgedBuilder::createGlobalValue(BridgedGlobalVar global, bool isBare) const {
  return {get().createGlobalValue(regularLoc(), global.getGlobal(), isBare)};
}

BridgedInstruction BridgedBuilder::createStruct(BridgedType type, BridgedValueArray elements) const {
  llvm::SmallVector<swift::SILValue, 16> elementValues;
  return {get().createStruct(regularLoc(), type.get(), elements.getValues(elementValues))};
}

BridgedInstruction BridgedBuilder::createStructExtract(BridgedValue str, SwiftInt fieldIndex) const {
  swift::SILValue v = str.getSILValue();
  return {get().createStructExtract(regularLoc(), v, v->getType().getFieldDecl(fieldIndex))};
}

BridgedInstruction BridgedBuilder::createStructElementAddr(BridgedValue addr, SwiftInt fieldIndex) const {
  swift::SILValue v = addr.getSILValue();
  return {get().createStructElementAddr(regularLoc(), v, v->getType().getFieldDecl(fieldIndex))};
}

BridgedInstruction BridgedBuilder::createDestructureStruct(BridgedValue str) const {
  return {get().createDestructureStruct(regularLoc(), str.getSILValue())};
}

BridgedInstruction BridgedBuilder::createTuple(BridgedType type, BridgedValueArray elements) const {
  llvm::SmallVector<swift::SILValue, 16> elementValues;
  return {get().createTuple(regularLoc(), type.get(), elements.getValues(elementValues))};
}

BridgedInstruction BridgedBuilder::createTupleExtract(BridgedValue str, SwiftInt elementIndex) const {
  swift::SILValue v = str.getSILValue();
  return {get().createTupleExtract(regularLoc(), v, elementIndex)};
}

BridgedInstruction BridgedBuilder::createTupleElementAddr(BridgedValue addr, SwiftInt elementIndex) const {
  swift::SILValue v = addr.getSILValue();
  return {get().createTupleElementAddr(regularLoc(), v, elementIndex)};
}

BridgedInstruction BridgedBuilder::createDestructureTuple(BridgedValue str) const {
  return {get().createDestructureTuple(regularLoc(), str.getSILValue())};
}

BridgedInstruction BridgedBuilder::createStore(BridgedValue src, BridgedValue dst,
                               SwiftInt ownership) const {
  return {get().createStore(regularLoc(), src.getSILValue(), dst.getSILValue(),
                                (swift::StoreOwnershipQualifier)ownership)};
}

BridgedInstruction BridgedBuilder::createInitExistentialRef(BridgedValue instance,
                                            BridgedType type,
                                            BridgedInstruction useConformancesOf) const {
  auto *src = useConformancesOf.getAs<swift::InitExistentialRefInst>();
  return {get().createInitExistentialRef(regularLoc(), type.get(),
                                             src->getFormalConcreteType(),
                                             instance.getSILValue(),
                                             src->getConformances())};
}

BridgedInstruction BridgedBuilder::createMetatype(BridgedType type,
                                                  BridgedType::MetatypeRepresentation representation) const {
  auto *mt = swift::MetatypeType::get(type.get().getASTType(), (swift::MetatypeRepresentation)representation);
  auto t = swift::SILType::getPrimitiveObjectType(swift::CanType(mt));
  return {get().createMetatype(regularLoc(), t)};
}

BridgedInstruction BridgedBuilder::createEndCOWMutation(BridgedValue instance, bool keepUnique) const {
  return {get().createEndCOWMutation(regularLoc(), instance.getSILValue(), keepUnique)};
}

SWIFT_END_NULLABILITY_ANNOTATIONS

#endif
