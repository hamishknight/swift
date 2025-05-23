//===--- swift-fuzzer.cpp - Swift fuzzer ----------------------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2025 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
//
//
//
//===----------------------------------------------------------------------===//

#include "SourceKit/Core/Context.h"
#include "SourceKit/Core/LangSupport.h"
#include "SourceKit/SwiftLang/Factory.h"
#include "swift/Basic/Defer.h"
#include "swift/Basic/InitializeSwiftModules.h"
#include "swift/Basic/LLVM.h"
#include "swift/Basic/LLVMInitialize.h"
#include "swift/Basic/PrettyStackTrace.h"
#include "swift/Frontend/Frontend.h"
#include "swift/Frontend/PrintingDiagnosticConsumer.h"
#include "swift/FrontendTool/FrontendTool.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

extern "C" {
void *swift_Fuzzing_createFuzzingContext(ssize_t MaxLen);

void *swift_Fuzzing_getFuzzerInput(const char *_Nonnull Data, ssize_t Size,
                                   void *_Nonnull Context);

ssize_t swift_Fuzzing_getNumBuffers(void *_Nonnull Input);

const char *_Nonnull swift_Fuzzing_getBuffer(void *_Nonnull input,
                                             const char *_Nonnull data,
                                             ssize_t idx,
                                             ssize_t *_Nonnull outLen);

void swift_Fuzzing_mutateSyntax(const char *_Nonnull Data, ssize_t Size,
                                void *_Nonnull Context, char **_Nonnull Out,
                                ssize_t *_Nonnull OutLen);

ssize_t swift_Fuzzing_getNumSourceKitRequests(void *_Nonnull Input);

void swift_Fuzzing_getSourceKitRequest(void *_Nonnull input, ssize_t idx,
                                       ssize_t *_Nonnull outKind,
                                       ssize_t *_Nonnull outOffset,
                                       ssize_t *_Nonnull outBufferIdx);

const char * const* swift_Fuzzing_getExtraArgs(void *_Nonnull input,
                                               ssize_t *_Nonnull outCount);

} // extern "C"

enum class SourceKitRequestKind : uint8_t { Complete = 0, CursorInfo = 1 };

using namespace swift;

static std::string MainExecutablePath;
static std::vector<const char *> ArgsBuffer;
static void *FuzzingContext = nullptr;
static bool DumpInputs = false;
static SourceKit::Context *SKCtx;

static const char *FlagValue(const char *Param, const char *Name) {
  size_t Len = strlen(Name);
  if (Param[0] == '-' && strstr(Param + 1, Name) == Param + 1 &&
      Param[Len + 1] == '=')
      return &Param[Len + 2];
  return nullptr;
}

static void testMutations(std::unique_ptr<llvm::MemoryBuffer> buffer) {
  StringRef input(buffer->getBuffer());
  llvm::errs() << input;

  for (int i = 0; i < 100000; i++) {
    llvm::errs() << "========================================================"
                    "=======\n";
    char *RandomSyntax = nullptr;
    ssize_t RandomSyntaxLen = 0;
    swift_Fuzzing_mutateSyntax(input.data(), input.size(), FuzzingContext,
                               &RandomSyntax, &RandomSyntaxLen);
    StringRef SyntaxRef(RandomSyntax, RandomSyntaxLen);
    llvm::errs() << SyntaxRef;
    input = SyntaxRef;
  }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *_Data, size_t Size);

extern "C" int LLVMFuzzerInitialize(int *_argc, const char ***_argv) {
  INITIALIZE_LLVM();
  initializeSwiftModules();

  auto &argc = *_argc;
  auto &argv = *_argv;

  MainExecutablePath =
      llvm::sys::fs::getMainExecutable(*argv, (void *)&LLVMFuzzerInitialize);

  llvm::SmallString<128> libPath(MainExecutablePath);
  llvm::sys::path::remove_filename(libPath); // swift-fuzzer
  llvm::sys::path::remove_filename(libPath); // bin
  llvm::sys::path::append(libPath, "lib");

  {
    using namespace SourceKit;
    SKCtx = new Context(
        MainExecutablePath, libPath, createSwiftLangSupport,
        /*plugin*/ [](Context &Ctx) { return nullptr; },
        /*dispatchOnMain=*/false);

    // Don't ever check dependencies, it can lead to non-determinism. We always
    // make sure to clear the cached compiler instance for a new input anyway.
    auto Config = std::make_shared<GlobalConfig>();
    Config->update(/*MaxASTReuse*/std::nullopt, /*DepCheckInterval*/~0U);
    SKCtx->getSwiftLangSupport().globalConfigurationUpdated(Config);
  }

  // Split out trailing arguments to pass to the frontend.
  auto separatorIdx = [&]() -> std::optional<int> {
    for (int i = 0; i < argc; i++) {
      StringRef arg(argv[i]);
      if (arg == "-ignore_remaining_args=1")
        return i;
    }
    return std::nullopt;
  }();
  if (separatorIdx) {
    ArgsBuffer = std::vector<const char *>(argv + *separatorIdx + 1, argv + argc);
    argc = *separatorIdx;
  }

  // Intercept '-max_len=<len>' and replace it with effectively unlimited
  // length, we'll instead handle the limit ourselves.
  unsigned maxLen = 0;
  for (int i = 0; i < argc; i += 1) {
    if (auto *val = FlagValue(argv[i], "max_len")) {
      maxLen = strtol(val, nullptr, 10);
      argv[i] = "-max_len=99999999";
    }
  }
  // FIXME: Clean up this mess
  if (!maxLen) {
    maxLen = 10000;
    auto *newArgs = (const char **)malloc((argc + 1) * sizeof(char *));
    memcpy(newArgs, argv, argc * sizeof(char *));
    argv = newArgs;
    argv[argc] = "-max_len=99999999";
    argc += 1;
  }

  llvm::errs() << "Fuzzing with max length: " << maxLen << "\n";
  llvm::errs() << "libfuzzer args: [";
  for (int i = 0; i < argc; i++) {
    llvm::errs() << argv[i] << " ";
  }
  llvm::errs() << "]\n";
  llvm::errs() << "compiler args: [";
  for (auto arg : ArgsBuffer) {
    llvm::errs() << arg << " ";
  }
  llvm::errs() << "]\n";

  if (separatorIdx) {
    auto numCombinedArgs = argc + ArgsBuffer.size() + 1;
    auto *newArgs = (const char **)malloc(numCombinedArgs * sizeof(char *));
    memcpy(newArgs, argv, argc * sizeof(char *));
    newArgs[argc] = "-ignore_remaining_args=1";
    memcpy(newArgs + argc + 1, ArgsBuffer.data(), ArgsBuffer.size() * sizeof(char *));
    argv = newArgs;
    argc = numCombinedArgs;
  }

  llvm::errs() << "combined args: [";
  for (int i = 0; i < argc; i++) {
    llvm::errs() << argv[i] << " ";
  }
  llvm::errs() << "]\n";

  FuzzingContext = swift_Fuzzing_createFuzzingContext(maxLen);

  if (argc > 2 && strcmp(argv[1], "test") == 0) {
    if (argc > 3 && strcmp(argv[3], "-dump-inputs") == 0) {
      DumpInputs = true;
    }
    auto buff = llvm::MemoryBuffer::getFile(argv[2]);
    StringRef input(buff.get()->getBuffer());
    exit(LLVMFuzzerTestOneInput((const uint8_t *)input.data(), input.size()));
  }
  if (argc > 2 && strcmp(argv[1], "test-mutate") == 0) {
    auto buff = llvm::MemoryBuffer::getFile(argv[2]);
    testMutations(std::move(buff.get()));
    exit(0);
  }
  return 0;
}

extern "C" size_t LLVMFuzzerCustomMutator(uint8_t *Data, size_t Size,
                                          size_t MaxSize, unsigned int Seed) {
  char *OutSyntax = nullptr;
  ssize_t OutLen = 0;
  swift_Fuzzing_mutateSyntax((const char *)Data, Size, FuzzingContext,
                             &OutSyntax, &OutLen);
  SWIFT_DEFER { free(OutSyntax); };

  // Truncate the output if needed, note this should never happen in practice
  // since we set 'max_len' to a very high value.
  size_t OutSize = (size_t)OutLen <= MaxSize ? OutLen : MaxSize;
  memcpy(Data, OutSyntax, OutSize);
  return OutSize;
}

static void
doSourceKitRequest(SourceKitRequestKind Kind, size_t Offset,
                   llvm::IntrusiveRefCntPtr<llvm::vfs::FileSystem> FS,
                   llvm::MemoryBuffer *buffer,
                   llvm::ArrayRef<const char *> Args) {
  using namespace SourceKit;
  auto &Lang = SKCtx->getSwiftLangSupport();
  switch (Kind) {
  case SourceKitRequestKind::Complete:
    llvm::errs() << "completing at " << Offset << "\n";
    Lang.codeCompleteFrontend(Args, FS, buffer, Offset);
    break;
  default:
    break;
  }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *_Data, size_t Size) {
  const char *Data = (const char *)_Data;

  void *FuzzInput = swift_Fuzzing_getFuzzerInput(Data, Size, FuzzingContext);
  ssize_t NumSourceKitReqs = swift_Fuzzing_getNumSourceKitRequests(FuzzInput);

  auto FS = llvm::makeIntrusiveRefCnt<llvm::vfs::OverlayFileSystem>(
      llvm::vfs::createPhysicalFileSystem());
  auto overlayFS = llvm::makeIntrusiveRefCnt<llvm::vfs::InMemoryFileSystem>();
  FS->pushOverlay(overlayFS);

  SmallVector<std::pair<std::string, std::unique_ptr<llvm::MemoryBuffer>>, 4> InputBuffers;
  auto NumBuffers = swift_Fuzzing_getNumBuffers(FuzzInput);
  for (int i = 0; i < NumBuffers; i++) {
    SmallString<32> InputPath("/tmp/swift-fuzzer");
    if (i == 0) {
      llvm::sys::path::append(InputPath, "main.swift");
    } else {
      llvm::sys::path::append(InputPath,
                              llvm::Twine("x") + llvm::Twine(i) + ".swift");
    }
    ssize_t BufferLen = 0;
    auto *BufferStart = swift_Fuzzing_getBuffer(FuzzInput, Data, i, &BufferLen);
    StringRef BufferStr(BufferStart, BufferLen);
    if (DumpInputs) {
      SmallString<64> InputDump(BufferStr);
      size_t PreOffset = 0;
      for (int ReqIdx = 0; ReqIdx < NumSourceKitReqs; ReqIdx++) {
        ssize_t Kind, Offset, BufferIdx;
        swift_Fuzzing_getSourceKitRequest(FuzzInput, ReqIdx, &Kind, &Offset,
                                          &BufferIdx);
        if (i != BufferIdx)
          continue;

        StringRef ReqStr = "#^REQ^#";
        InputDump.insert(InputDump.begin() + Offset + PreOffset,
                         ReqStr.begin(), ReqStr.end());
        PreOffset += ReqStr.size();
      }
      llvm::errs() << "============== " << llvm::sys::path::filename(InputPath);
      llvm::errs() << " ==============" << "\n";
      llvm::errs() << InputDump;
      llvm::errs() << "\n==========================================\n";
    }
    overlayFS->addFile(InputPath, 0,
                       llvm::MemoryBuffer::getMemBufferCopy(BufferStr, InputPath));
    InputBuffers.emplace_back(InputPath.str().str(),
                              std::move(overlayFS->getBufferForFile(InputPath).get()));
  }

  CompilerInstance Instance;
  Instance.getSourceMgr().setFileSystem(FS);

  PrintingDiagnosticConsumer PDC;
  PDC.setFormattingStyle(DiagnosticOptions::FormattingStyle::Swift);
  Instance.addDiagnosticConsumer(&PDC);

  CompilerInvocation Invocation;

  std::vector<const char *> InvokArgs(ArgsBuffer.begin(), ArgsBuffer.end());
  if (ArgsBuffer.empty())
    InvokArgs.push_back("-typecheck");

  for (auto &[Path, Buffer] : InputBuffers)
    InvokArgs.push_back(Path.c_str());

  // Append any extra args if needed.
  ssize_t numExtraArgs = 0;
  const char *const *extraArgs = swift_Fuzzing_getExtraArgs(FuzzInput,
                                                            &numExtraArgs);
  for (auto i = 0; i < numExtraArgs; i++)
    InvokArgs.push_back(extraArgs[i]);

  Invocation.parseArgs(InvokArgs, Instance.getDiags(), nullptr, {},
                       MainExecutablePath);

  std::string Err;
  if (Instance.setup(Invocation, Err))
    return -1;
  {
    int CompileRet = 0;
    performCompile(Instance, CompileRet, /*observer*/ nullptr, InvokArgs);
  }

  // Make sure we don't re-use any cached ASTs across inputs.
  if (NumSourceKitReqs > 0)
    SKCtx->getSwiftLangSupport().dependencyUpdated();

  for (auto i = 0; i < NumSourceKitReqs; i++) {
    ssize_t Kind, Offset, BufferIdx;
    swift_Fuzzing_getSourceKitRequest(FuzzInput, i, &Kind, &Offset, &BufferIdx);
    doSourceKitRequest((SourceKitRequestKind)Kind, Offset, FS,
                       InputBuffers[BufferIdx].second.get(), InvokArgs);
  }

  return 0;
}
