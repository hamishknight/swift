// RUN: %target-typecheck-verify-swift

actor class X {} // expected-error {{'actor' attribute used without importing module 'Dispatch'}}
