
// RUN: %target-swift-emit-silgen %s | %FileCheck %s

import Dispatch

// main
// CHECK-LABEL: sil [ossa] @main : $@convention(c) (Int32, UnsafeMutablePointer<Optional<UnsafeMutablePointer<Int8>>>) -> Int32
let c = C(j: 0)
let s = S(i: 1)

// CHECK:      [[FOO_FN:%[0-9]+]] = function_ref @$s5actor1CC3fooyyAA1SVF : $@convention(method) (S, @guaranteed C) -> ()
// CHECK-NEXT: %25 = apply [[FOO_FN]]
c.foo(s)

struct S {
  var i: Int
}

actor class C {
  var j: Int

  // C.foo(_:)
  // CHECK-LABEL: sil hidden [ossa] @$s5actor1CC3fooyyAA1SVF : $@convention(method) (S, @guaranteed C) -> ()
  // CHECK:    bb0(%0 : $S, %1 : @guaranteed $C)
  // CHECK:      [[COPIED_S:%[0-9]+]] = alloc_stack $S
  // CHECK-NEXT: [[ORIG_S:%[0-9]+]] = alloc_stack $S
  // CHECK-NEXT: store %0 to [trivial] [[ORIG_S]] : $*S
  // CHECK-NEXT: // function_ref _CompilerCopyable.copy()
  // CHECK-NEXT: [[COPY_FN:%[0-9]+]] = function_ref @$ss17_CompilerCopyablePsE4copyxyF : $@convention(method) <τ_0_0 where τ_0_0 : _CompilerCopyable> (@in_guaranteed τ_0_0) -> @out τ_0_0
  // CHECK-NEXT: apply [[COPY_FN]]<S>([[COPIED_S]], [[ORIG_S]])
  // CHECK-NEXT: dealloc_stack [[ORIG_S]]
  // CHECK-NEXT: [[LOADED_S:%[0-9]+]] = load [trivial] [[COPIED_S]]
  // CHECK-NEXT: debug_value
  // CHECK-NEXT: dealloc_stack [[COPIED_S]]
  //
  // CHECK-NEXT: // function_ref actor method impl of C.foo(_:)
  // CHECK-NEXT: [[ACTOR_IMPL:%[0-9]+]] = function_ref @$s5actor1CC3fooyyAA1SVFfa : $@convention(thin) (@guaranteed C, S) -> ()
  // CHECK-NEXT: [[ACTOR_IMP_W_ARG:%[0-9]+]] = partial_apply [callee_guaranteed] [[ACTOR_IMPL]]([[LOADED_S]])
  // CHECK-NEXT: [[COPIED_C:%[0-9]+]] = copy_value %1 : $C
  // CHECK-NEXT: [[STACK_C:%[0-9]+]] = alloc_stack $C
  // CHECK-NEXT: store [[COPIED_C]] to [init] [[STACK_C]] : $*C
  //
  // CHECK-NEXT: // function_ref thunk for @escaping @callee_guaranteed (@guaranteed C) -> ()
  // CHECK-NEXT: [[THUNK:%[0-9]+]] = function_ref @$s5actor1CCIegg_ACIegn_TR : $@convention(thin) (@in_guaranteed C, @guaranteed @callee_guaranteed (@guaranteed C) -> ()) -> ()
  // CHECK-NEXT: [[ABSTRACT_ACTOR_IMPL:%[0-9]+]] = partial_apply [callee_guaranteed] [[THUNK]]([[ACTOR_IMP_W_ARG]])
  //
  // CHECK-NEXT: // function_ref _actorCall<A>(_:body:)
  // CHECK-NEXT: [[ACTORCALL_FN:%[0-9]+]] = function_ref @$s8Dispatch10_actorCall_4bodyyx_yxctAA14_ActorProtocolRzlF : $@convention(thin) <τ_0_0 where τ_0_0 : _ActorProtocol> (@in_guaranteed τ_0_0, @guaranteed @callee_guaranteed (@in_guaranteed τ_0_0) -> ()) -> ()
  // CHECK-NEXT: apply [[ACTORCALL_FN]]<C>([[STACK_C]], [[ABSTRACT_ACTOR_IMPL]])
  //
  // CHECK-NEXT: destroy_value [[ABSTRACT_ACTOR_IMPL]]
  // CHECK-NEXT: destroy_addr [[STACK_C]]
  // CHECK-NEXT: dealloc_stack [[STACK_C]]

  // actor method impl of C.foo(_:)
  // CHECK-LABEL: sil hidden [ossa] @$s5actor1CC3fooyyAA1SVFfa : $@convention(thin) (@guaranteed C, S) -> ()
  actor func foo(_ s: S) {
    // CHECK: bb0(%0 : @guaranteed $C, %1 : $S)
    // CHECK:   [[BAR_REF:%[0-9]+]] = function_ref @$s5actor1CC3baryyAA1SVF : $@convention(method) (S, @guaranteed C) -> ()
    // CHECK:   apply [[BAR_REF]](%1, %0) : $@convention(method) (S, @guaranteed C) -> ()
    bar(s)
  }

  // C.init(j:)
  // CHECK-LABEL: sil hidden [ossa] @$s5actor1CC1jACSi_tcfc : $@convention(method) (Int, @owned C) -> @owned C {
  init(j: Int) {
    // CHECK:    bb0(%0 : $Int, %1 : @owned $C)
    // CHECK:      [[SELF:%[0-9]+]] = mark_uninitialized [rootself] %1 : $C
    //
    // CHECK-NEXT: // function_ref variable initialization expression of C.__queue
    // CHECK-NEXT: [[QUEUE_FN:%[0-9]+]] = function_ref @$s5actor1CC7__queueSo012OS_dispatch_B0Cvpfi : $@convention(thin) () -> @owned DispatchQueue
    // CHECK-NEXT: [[QUEUE:%[0-9]+]] = apply [[QUEUE_FN]]()
    // CHECK-NEXT: [[SELF_BORROW:%[0-9]+]] = begin_borrow [[SELF]] : $C
    // CHECK-NEXT: [[QUEUE_PROP:%[0-9]+]] = ref_element_addr [[SELF_BORROW]] : $C, #C.__queue
    // CHECK-NEXT: [[QUEUE_ACCESS:%[0-9]+]] = begin_access [modify] [dynamic] [[QUEUE_PROP]] : $*DispatchQueue
    // CHECK-NEXT: assign [[QUEUE]] to [[QUEUE_ACCESS]]
    // CHECK-NEXT: end_access [[QUEUE_ACCESS]]
    // CHECK-NEXT: end_borrow [[SELF_BORROW]]
    //
    // CHECK-NEXT: // function_ref Int.copy()
    // CHECK-NEXT: [[COPY_FN:%[0-9]+]] = function_ref @$sSi4copySiyF : $@convention(method) (Int) -> Int
    // CHECK-NEXT: [[COPIED_J:%[0-9]+]] = apply [[COPY_FN]](%0)
    // CHECK-NEXT: debug_value
    // CHECK-NEXT: [[SELF_BORROW:%[0-9]+]] = begin_borrow [[SELF]] : $C
    // CHECK-NEXT: [[J_PROP:%[0-9]+]] = ref_element_addr [[SELF_BORROW]] : $C, #C.j
    // CHECK-NEXT: [[J_PROP_ACCESS:%[0-9]+]] = begin_access [modify] [dynamic] [[J_PROP]] : $*Int
    // CHECK-NEXT: assign [[COPIED_J]] to [[J_PROP_ACCESS]] : $*Int
    // CHECK-NEXT: end_access [[J_PROP_ACCESS]]
    // CHECK-NEXT: end_borrow [[SELF_BORROW]]

    self.j = j
  }

  func bar(_ s: S) {}
}

actor class D<T : Copyable> {
  var x: T

  // CHECK-LABEL: sil hidden [ossa] @$s5actor1DCyACyxGxcfc : $@convention(method) <T where T : Copyable> (@in T, @owned D<T>) -> @owned D<T>
  init(_ x: T) {
    // CHECK:    bb0(%0 : $*T, %1 : @owned $D<T>)
    // CHECK:      [[SELF:%[0-9]+]] = mark_uninitialized [rootself] %1 : $D<T>

    // CHECK:      [[ARG_COPY:%[0-9]+]] = alloc_stack $T, let, name "argCopy"
    // CHECK-NEXT: [[COPY_FN:%[0-9]+]] = witness_method $T, #Copyable.copy!1 : <Self where Self : Copyable> (Self) -> () -> @dynamic_self Self : $@convention(witness_method: Copyable) <τ_0_0 where τ_0_0 : Copyable> (@in_guaranteed τ_0_0) -> @out τ_0_0
    // CHECK-NEXT: apply %14<T>([[ARG_COPY]], %0)

    // CHECK-NEXT: [[SELF_BORROW:%[0-9]+]] = begin_borrow [[SELF]] : $D<T>
    // CHECK-NEXT: [[ARG_TMP:%[0-9]+]] = alloc_stack $T
    // CHECK-NEXT: copy_addr [[ARG_COPY]] to [initialization] [[ARG_TMP]] : $*T
    // CHECK-NEXT: [[X_PROP:%[0-9]+]] = ref_element_addr [[SELF_BORROW]] : $D<T>, #D.x
    // CHECK-NEXT: [[X_PROP_ACCESS:%[0-9]+]] = begin_access [modify] [dynamic] [[X_PROP]] : $*T
    // CHECK-NEXT: copy_addr [take] [[ARG_TMP]] to [[X_PROP_ACCESS]] : $*T
    // CHECK-NEXT: end_access [[X_PROP_ACCESS]]
    // CHECK-NEXT: dealloc_stack [[ARG_TMP]]
    // CHECK-NEXT: end_borrow [[SELF_BORROW]]
    // CHECK-NEXT: destroy_addr [[ARG_COPY]]
    // CHECK-NEXT: dealloc_stack [[ARG_COPY]]

    self.x = x
  }

  // D.foo<A>(_:_:)
  // CHECK-LABEL: sil hidden [ossa] @$s5actor1DC3fooyyx_qd__ts8CopyableRd__lF : $@convention(method) <T where T : Copyable><U where U : Copyable> (@in_guaranteed T, @in_guaranteed U, @guaranteed D<T>) -> ()
  actor func foo<U : Copyable>(_ x: T, _ y: U) {
    // CHECK:    bb0(%0 : $*T, %1 : $*U, %2 : @guaranteed $D<T>)
    // CHECK:      [[T_COPY:%[0-9]+]] = alloc_stack $T, let, name "argCopy"
    // CHECK-NEXT: [[COPY_FN:%[0-9]+]] = witness_method $T, #Copyable.copy!1 : <Self where Self : Copyable> (Self) -> () -> @dynamic_self Self : $@convention(witness_method: Copyable) <τ_0_0 where τ_0_0 : Copyable> (@in_guaranteed τ_0_0) -> @out τ_0_0
    // CHECK-NEXT: apply [[COPY_FN]]<T>([[T_COPY]], %0)
    //
    // CHECK-NEXT: [[U_COPY:%[0-9]+]] = alloc_stack $U, let, name "argCopy"
    // CHECK-NEXT: [[COPY_FN:%[0-9]+]] = witness_method $U, #Copyable.copy!1 : <Self where Self : Copyable> (Self) -> () -> @dynamic_self Self : $@convention(witness_method: Copyable) <τ_0_0 where τ_0_0 : Copyable> (@in_guaranteed τ_0_0) -> @out τ_0_0
    // CHECK-NEXT: apply [[COPY_FN]]<U>([[U_COPY]], %1)
    //
    // CHECK-NEXT: // function_ref actor method impl of D.foo<A>(_:_:)
    // CHECK-NEXT: [[ACTOR_IMPL:%[0-9]+]] = function_ref @$s5actor1DC3fooyyx_qd__ts8CopyableRd__lFfa : $@convention(thin) <τ_0_0 where τ_0_0 : Copyable><τ_1_0 where τ_1_0 : Copyable> (@guaranteed D<τ_0_0>, @guaranteed <τ_0_0 where τ_0_0 : Copyable><τ_1_0 where τ_1_0 : Copyable> { var τ_0_0 } <τ_0_0, τ_1_0>, @guaranteed <τ_0_0 where τ_0_0 : Copyable><τ_1_0 where τ_1_0 : Copyable> { var τ_1_0 } <τ_0_0, τ_1_0>) -> ()
    //
    // CHECK-NEXT: [[BOXED_T:%[0-9]+]] = alloc_box $<τ_0_0 where τ_0_0 : Copyable><τ_1_0 where τ_1_0 : Copyable> { var τ_0_0 } <T, U>
    // CHECK-NEXT: [[BOXED_T_ADDR:%[0-9]+]] = project_box [[BOXED_T]]
    // CHECK-NEXT: copy_addr [[T_COPY]] to [initialization] [[BOXED_T_ADDR]] : $*T
    //
    // CHECK-NEXT: [[BOXED_U:%[0-9]+]] = alloc_box $<τ_0_0 where τ_0_0 : Copyable><τ_1_0 where τ_1_0 : Copyable> { var τ_1_0 } <T, U>
    // CHECK-NEXT: [[BOXED_U_ADDR:%[0-9]+]] = project_box [[BOXED_U]]
    // CHECK-NEXT: copy_addr [[U_COPY]] to [initialization] [[BOXED_U_ADDR]] : $*U
    //
    // CHECK-NEXT: [[ACTOR_IMPL_W_ARGS:%[0-9]+]] = partial_apply [callee_guaranteed] [[ACTOR_IMPL]]<T, U>([[BOXED_T]], [[BOXED_U]]) : $@convention(thin) <τ_0_0 where τ_0_0 : Copyable><τ_1_0 where τ_1_0 : Copyable> (@guaranteed D<τ_0_0>, @guaranteed <τ_0_0 where τ_0_0 : Copyable><τ_1_0 where τ_1_0 : Copyable> { var τ_0_0 } <τ_0_0, τ_1_0>, @guaranteed <τ_0_0 where τ_0_0 : Copyable><τ_1_0 where τ_1_0 : Copyable> { var τ_1_0 } <τ_0_0, τ_1_0>) -> ()
    //
    // CHECK-NEXT: [[SELF_TMP:%[0-9]+]] = copy_value %2 : $D<T>
    // CHECK-NEXT: [[SELF_COPY:%[0-9]+]] = alloc_stack $D<T>
    // CHECK-NEXT: store [[SELF_TMP]] to [init] [[SELF_COPY]] : $*D<T>
    //
    // CHECK-NEXT: // function_ref thunk for @escaping @callee_guaranteed (@guaranteed D<A>) -> ()
    // CHECK-NEXT: [[THUNK:%[0-9]+]] = function_ref @$s5actor1DCyxGIegg_ADIegn_s8CopyableRzsAERd__r__lTR : $@convention(thin) <τ_0_0 where τ_0_0 : Copyable><τ_1_0 where τ_1_0 : Copyable> (@in_guaranteed D<τ_0_0>, @guaranteed @callee_guaranteed (@guaranteed D<τ_0_0>) -> ()) -> ()
    // CHECK-NEXT: [[ABSTRACT_IMPL:%[0-9]+]] = partial_apply [callee_guaranteed] [[THUNK]]<T, U>([[ACTOR_IMPL_W_ARGS]]) : $@convention(thin) <τ_0_0 where τ_0_0 : Copyable><τ_1_0 where τ_1_0 : Copyable> (@in_guaranteed D<τ_0_0>, @guaranteed @callee_guaranteed (@guaranteed D<τ_0_0>) -> ()) -> ()
    //
    // CHECK-NEXT: // function_ref _actorCall<A>(_:body:)
    // CHECK-NEXT: [[ACTOR_CALL:%[0-9]+]] = function_ref @$s8Dispatch10_actorCall_4bodyyx_yxctAA14_ActorProtocolRzlF : $@convention(thin) <τ_0_0 where τ_0_0 : _ActorProtocol> (@in_guaranteed τ_0_0, @guaranteed @callee_guaranteed (@in_guaranteed τ_0_0) -> ()) -> ()
    // CHECK-NEXT: apply [[ACTOR_CALL]]<D<T>>([[SELF_COPY]], [[ABSTRACT_IMPL]]) : $@convention(thin) <τ_0_0 where τ_0_0 : _ActorProtocol> (@in_guaranteed τ_0_0, @guaranteed @callee_guaranteed (@in_guaranteed τ_0_0) -> ()) -> ()
    // CHECK-NEXT: destroy_value [[ABSTRACT_IMPL]]
    // CHECK-NEXT: destroy_addr [[SELF_COPY]]
    // CHECK-NEXT: dealloc_stack [[SELF_COPY]]
    // CHECK-NEXT: destroy_addr [[U_COPY]]
    // CHECK-NEXT: dealloc_stack [[U_COPY]]
    // CHECK-NEXT: destroy_addr [[T_COPY]]
    // CHECK-NEXT: dealloc_stack [[T_COPY]]
  }
}
