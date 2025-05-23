include_directories(BEFORE
  ${SWIFT_SOURCE_DIR}/tools/SourceKit/include
)
add_swift_fuzzer_host_tool(swift-fuzzer
  swift-fuzzer.cpp
  LLVM_LINK_COMPONENTS support
  SWIFT_COMPONENT compiler
)
target_link_libraries(
  swift-fuzzer
  PRIVATE
    swiftFrontendTool
    swiftFuzzing
    SourceKitSwiftLang
    swiftCompilerModules
)
