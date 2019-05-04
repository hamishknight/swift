
// RUN: %target-swift-emit-silgen %s | %FileCheck %s

func takesActorSafeFn(_ fn: @escaping @actorSafe () -> Int) {}

// CHECK-LABEL: sil hidden [ossa] @$s9actorSafe3fooyyF : $@convention(thin) () -> ()
func foo() {
  let i = 0
  let j = 0

  // Check that we synthesize .copy() for the captured args.
  takesActorSafeFn { [i, j] in
    return i + j
  }

  // CHECK:      [[COPY_FN_1:%[0-9]+]] = function_ref @$sSi4copySiyF : $@convention(method) (Int) -> Int
  // CHECK-NEXT: [[COPIED_1:%[0-9]+]] = apply [[COPY_FN_1]]({{%[0-9]+}}) : $@convention(method) (Int) -> Int

  // CHECK:      [[COPY_FN_2:%[0-9]+]] = function_ref @$sSi4copySiyF : $@convention(method) (Int) -> Int
  // CHECK-NEXT: [[COPIED_2:%[0-9]+]] = apply [[COPY_FN_2]]({{%[0-9]+}}) : $@convention(method) (Int) -> Int

  // CHECK:      [[CLOSURE:%[0-9]+]] = function_ref @$s9actorSafe3fooyyFSiycfU_ : $@convention(thin) (Int, Int) -> Int
  // CHECK-NEXT: partial_apply [callee_guaranteed] [[CLOSURE]]([[COPIED_1]], [[COPIED_2]])
}
