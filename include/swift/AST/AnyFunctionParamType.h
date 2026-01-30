//===--- AnyFunctionParamType.h - Function parameter type -------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2020 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#ifndef SWIFT_ANYFUNCTIONPARAMTYPE_H
#define SWIFT_ANYFUNCTIONPARAMTYPE_H

#include "swift/AST/Identifier.h"
#include "swift/AST/Ownership.h"
#include "swift/AST/Type.h"
#include "swift/Basic/LLVM.h"
#include "swift/Basic/OptionSet.h"

namespace swift {
class CanGenericSignature;
class Identifier;
class ValueDecl;

/// The various spellings of ownership modifier that can be used in source.
enum class ParamSpecifier : uint8_t {
  /// No explicit ownership specifier was provided. The parameter will use the
  /// default ownership convention for the declaration.
  Default = 0,

  /// `inout`, indicating exclusive mutable access to the argument for the
  /// duration of a call.
  InOut = 1,

  /// `borrowing`, indicating nonexclusive access to the argument for the
  /// duration of a call.
  Borrowing = 2,
  /// `consuming`, indicating ownership transfer of the argument from caller
  /// to callee.
  Consuming = 3,

  /// `__shared`, a legacy spelling of `borrowing`.
  LegacyShared = 4,
  /// `__owned`, a legacy spelling of `consuming`.
  LegacyOwned = 5,

  /// A convention that is similar to consuming a parameter that is mutable and
  /// var like, but for which no implicit copy semantics are not implemented.
  ImplicitlyCopyableConsuming = 6,
};

StringRef getNameForParamSpecifier(ParamSpecifier name);

/// What does \c ParamSpecifier::Default mean for a parameter that's directly
/// attached to \p VD ? Pass \c nullptr for the value for a closure.
ParamSpecifier getDefaultParamSpecifier(const ValueDecl *VD);

/// Provide parameter type relevant flags, i.e. variadic, autoclosure, and
/// escaping.
class ParameterTypeFlags {
  enum ParameterFlags : uint16_t {
    None = 0,
    Variadic = 1 << 0,
    AutoClosure = 1 << 1,
    NonEphemeral = 1 << 2,
    SpecifierShift = 3,
    Specifier = 7 << SpecifierShift,
    NoDerivative = 1 << 6,
    Isolated = 1 << 7,
    CompileTimeLiteral = 1 << 8,
    Sending = 1 << 9,
    Addressable = 1 << 10,
    ConstValue = 1 << 11,
    NumBits = 12
  };
  OptionSet<ParameterFlags> value;
  static_assert(NumBits <= 8 * sizeof(OptionSet<ParameterFlags>), "overflowed");

  ParameterTypeFlags(OptionSet<ParameterFlags, uint16_t> val) : value(val) {}

public:
  ParameterTypeFlags() = default;
  static ParameterTypeFlags fromRaw(uint16_t raw) {
    return ParameterTypeFlags(OptionSet<ParameterFlags>(raw));
  }

  ParameterTypeFlags(bool variadic, bool autoclosure, bool nonEphemeral,
                     ParamSpecifier specifier, bool isolated, bool noDerivative,
                     bool compileTimeLiteral, bool isSending,
                     bool isAddressable, bool isConstValue)
      : value((variadic ? Variadic : 0) | (autoclosure ? AutoClosure : 0) |
              (nonEphemeral ? NonEphemeral : 0) |
              uint8_t(specifier) << SpecifierShift | (isolated ? Isolated : 0) |
              (noDerivative ? NoDerivative : 0) |
              (compileTimeLiteral ? CompileTimeLiteral : 0) |
              (isSending ? Sending : 0) | (isAddressable ? Addressable : 0) |
              (isConstValue ? ConstValue : 0)) {}

  /// Create one from what's present in the parameter type
  inline static ParameterTypeFlags
  fromParameterType(Type paramTy, bool isVariadic, bool isAutoClosure,
                    bool isNonEphemeral, ParamSpecifier ownership,
                    bool isolated, bool isNoDerivative, bool compileTimeLiteral,
                    bool isSending, bool isAddressable, bool isConstVal);

  bool isNone() const { return !value; }
  bool isVariadic() const { return value.contains(Variadic); }
  bool isAutoClosure() const { return value.contains(AutoClosure); }
  bool isNonEphemeral() const { return value.contains(NonEphemeral); }
  bool isInOut() const { return getValueOwnership() == ValueOwnership::InOut; }
  bool isShared() const {
    return getValueOwnership() == ValueOwnership::Shared;
  }
  bool isOwned() const { return getValueOwnership() == ValueOwnership::Owned; }
  bool isIsolated() const { return value.contains(Isolated); }
  bool isCompileTimeLiteral() const {
    return value.contains(CompileTimeLiteral);
  }
  bool isNoDerivative() const { return value.contains(NoDerivative); }
  bool isSending() const { return value.contains(Sending); }
  bool isAddressable() const { return value.contains(Addressable); }
  bool isConstValue() const { return value.contains(ConstValue); }

  /// Get the spelling of the parameter specifier used on the parameter.
  ParamSpecifier getOwnershipSpecifier() const {
    return ParamSpecifier((value.toRaw() & Specifier) >> SpecifierShift);
  }

  ValueOwnership getValueOwnership() const;

  ParameterTypeFlags withVariadic(bool variadic) const {
    return ParameterTypeFlags(variadic ? value | ParameterTypeFlags::Variadic
                                       : value - ParameterTypeFlags::Variadic);
  }

  ParameterTypeFlags withInOut(bool isInout) const {
    return withOwnershipSpecifier(isInout ? ParamSpecifier::InOut
                                          : ParamSpecifier::Default);
  }

  ParameterTypeFlags withCompileTimeLiteral(bool isLiteral) const {
    return ParameterTypeFlags(
        isLiteral ? value | ParameterTypeFlags::CompileTimeLiteral
                  : value - ParameterTypeFlags::CompileTimeLiteral);
  }

  ParameterTypeFlags withConst(bool isConst) const {
    return ParameterTypeFlags(isConst ? value | ParameterTypeFlags::ConstValue
                                      : value - ParameterTypeFlags::ConstValue);
  }

  ParameterTypeFlags withShared(bool isShared) const {
    return withOwnershipSpecifier(isShared ? ParamSpecifier::LegacyShared
                                           : ParamSpecifier::Default);
  }

  ParameterTypeFlags withOwned(bool isOwned) const {
    return withOwnershipSpecifier(isOwned ? ParamSpecifier::LegacyOwned
                                          : ParamSpecifier::Default);
  }

  ParameterTypeFlags withOwnershipSpecifier(ParamSpecifier specifier) const {
    return (value - ParameterTypeFlags::Specifier) |
           ParameterFlags(uint8_t(specifier) << SpecifierShift);
  }

  ParameterTypeFlags withAutoClosure(bool isAutoClosure) const {
    return ParameterTypeFlags(isAutoClosure
                                  ? value | ParameterTypeFlags::AutoClosure
                                  : value - ParameterTypeFlags::AutoClosure);
  }

  ParameterTypeFlags withNonEphemeral(bool isNonEphemeral) const {
    return ParameterTypeFlags(isNonEphemeral
                                  ? value | ParameterTypeFlags::NonEphemeral
                                  : value - ParameterTypeFlags::NonEphemeral);
  }

  ParameterTypeFlags withIsolated(bool isolated) const {
    return ParameterTypeFlags(isolated ? value | ParameterTypeFlags::Isolated
                                       : value - ParameterTypeFlags::Isolated);
  }

  ParameterTypeFlags withNoDerivative(bool noDerivative) const {
    return ParameterTypeFlags(noDerivative
                                  ? value | ParameterTypeFlags::NoDerivative
                                  : value - ParameterTypeFlags::NoDerivative);
  }

  ParameterTypeFlags withSending(bool withSending) const {
    return ParameterTypeFlags(withSending
                                  ? value | ParameterTypeFlags::Sending
                                  : value - ParameterTypeFlags::Sending);
  }

  ParameterTypeFlags withAddressable(bool withAddressable) const {
    return ParameterTypeFlags(withAddressable
                                  ? value | ParameterTypeFlags::Addressable
                                  : value - ParameterTypeFlags::Addressable);
  }

  bool operator==(const ParameterTypeFlags &other) const {
    return value.toRaw() == other.value.toRaw();
  }

  bool operator!=(const ParameterTypeFlags &other) const {
    return value.toRaw() != other.value.toRaw();
  }

  uint16_t toRaw() const { return value.toRaw(); }
};

class AnyFunctionParamType {
public:
  explicit AnyFunctionParamType(Type t, Identifier l = Identifier(),
                                ParameterTypeFlags f = ParameterTypeFlags(),
                                Identifier internalLabel = Identifier());

private:
  /// The type of the parameter. For a variadic parameter, this is the
  /// element type.
  Type Ty;

  /// The label associated with the parameter, if any.
  Identifier Label;

  /// The internal label of the parameter, if explicitly specified, otherwise
  /// empty. The internal label is considered syntactic sugar. It is not
  /// considered part of the canonical type and is thus also ignored in \c
  /// operator==.
  /// E.g.
  ///  - `name name2: Int` has internal label `name2`
  ///  - `_ name2: Int` has internal label `name2`
  ///  - `name: Int` has no internal label
  Identifier InternalLabel;

  /// Parameter specific flags.
  ParameterTypeFlags Flags = {};

public:
  /// FIXME: Remove this. Return the formal type of the parameter in the
  /// function type, including the InOutType if there is one.
  ///
  /// For example, 'inout Int' => 'inout Int', 'Int...' => 'Int'.
  Type getOldType() const;

  /// Return the formal type of the parameter.
  ///
  /// For example, 'inout Int' => 'Int', 'Int...' => 'Int'.
  Type getPlainType() const { return Ty; }

  /// The type of the parameter when referenced inside the function body
  /// as an rvalue.
  ///
  /// For example, 'inout Int' => 'Int', 'Int...' => '[Int]'.
  Type getParameterType(bool forCanonical = false,
                        ASTContext *ctx = nullptr) const;

  bool hasLabel() const { return !Label.empty(); }
  Identifier getLabel() const { return Label; }

  bool hasInternalLabel() const { return !InternalLabel.empty(); }
  Identifier getInternalLabel() const { return InternalLabel; }

  /// Return true if argument name is valid and matches \c paramName.
  ///
  /// The three tests to check if argument name is valid are:
  /// 1. allow argument if it matches \c paramName,
  /// 2. allow argument if  $_  for omitted projected value label,
  /// 3. allow argument if it matches \c paramName without its \c $ prefix.
  bool matchParameterLabel(Identifier const &paramName) const {
    auto argLabel = getLabel();
    if (argLabel == paramName)
      return true;
    if ((argLabel.str() == "$_") && paramName.empty())
      return true;
    if (argLabel.hasDollarPrefix() &&
        argLabel.str().drop_front() == paramName.str())
      return true;
    return false;
  }

  AnyFunctionParamType getCanonical(CanGenericSignature genericSig) const;

  ParameterTypeFlags getParameterFlags() const { return Flags; }

  /// Whether the parameter is varargs
  bool isVariadic() const { return Flags.isVariadic(); }

  /// Whether the parameter is marked '@autoclosure'
  bool isAutoClosure() const { return Flags.isAutoClosure(); }

  /// Whether the parameter is marked 'inout'
  bool isInOut() const { return Flags.isInOut(); }

  /// Whether the parameter is marked 'shared'
  bool isShared() const { return Flags.isShared(); }

  /// Whether the parameter is marked 'owned'
  bool isOwned() const { return Flags.isOwned(); }

  /// Whether the parameter is marked '@_nonEphemeral'
  bool isNonEphemeral() const { return Flags.isNonEphemeral(); }

  /// Whether the parameter is 'isolated'.
  bool isIsolated() const { return Flags.isIsolated(); }

  /// Whether or not the parameter is 'sending'.
  bool isSending() const { return Flags.isSending(); }

  /// Whether the parameter is 'isCompileTimeLiteral'.
  bool isCompileTimeLiteral() const { return Flags.isCompileTimeLiteral(); }

  /// Whether the parameter is 'isConstValue'.
  bool isConstVal() const { return Flags.isConstValue(); }

  /// Whether the parameter is marked '@noDerivative'.
  bool isNoDerivative() const { return Flags.isNoDerivative(); }

  bool isAddressable() const { return Flags.isAddressable(); }

  /// Whether the parameter might be a semantic result for autodiff purposes.
  /// This includes inout parameters.
  bool isAutoDiffSemanticResult() const { return isInOut(); }

  ValueOwnership getValueOwnership() const { return Flags.getValueOwnership(); }

  /// Returns \c true if the two \c Params are equal in their canonicalized
  /// form.
  /// Two \c Params are equal if their external label, flags and
  /// *canonicalized* types match. The internal label and sugar types are
  /// *not* considered for type equality.
  bool operator==(AnyFunctionParamType const &b) const;
  bool operator!=(AnyFunctionParamType const &b) const { return !(*this == b); }

  /// Return the parameter without external and internal labels.
  AnyFunctionParamType getWithoutLabels() const {
    return AnyFunctionParamType(Ty, /*Label=*/Identifier(), Flags,
                                /*InternalLabel=*/Identifier());
  }

  AnyFunctionParamType withLabel(Identifier newLabel) const {
    return AnyFunctionParamType(Ty, newLabel, Flags, InternalLabel);
  }

  AnyFunctionParamType withType(Type newType) const {
    return AnyFunctionParamType(newType, Label, Flags, InternalLabel);
  }

  AnyFunctionParamType withFlags(ParameterTypeFlags flags) const {
    return AnyFunctionParamType(Ty, Label, flags, InternalLabel);
  }
};

class AnyFunctionParamCanType : public AnyFunctionParamType {
  explicit AnyFunctionParamCanType(const AnyFunctionParamType &param)
      : AnyFunctionParamType(param) {}

public:
  static AnyFunctionParamCanType
  getFromParam(const AnyFunctionParamType &param) {
    return AnyFunctionParamCanType(param);
  }

  CanType getOldType() const {
    return CanType(AnyFunctionParamType::getOldType());
  }
  CanType getPlainType() const {
    return CanType(AnyFunctionParamType::getPlainType());
  }
  CanType getParameterType() const {
    return CanType(
        AnyFunctionParamType::getParameterType(/*forCanonical*/ true));
  }
};

} // end namespace swift

#endif // SWIFT_ANYFUNCTIONPARAMTYPE_H
