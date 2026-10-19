#include "file_util.h"

#include <gtest/gtest.h>
#include <stdio.h>

#ifdef _WIN32
#include <io.h>
#include <Windows.h>
#include <fcntl.h>
#elif defined(__wasi__)
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <random>
#endif

namespace onnxruntime {
namespace test {

#ifndef _WIN32
// Opens a new file named by replacing the trailing "XXXXXX" in name_template, like mkstemp().
static int MakeTempFile(char* name_template) {
#if defined(__wasi__)
  // wasi-libc omits mkstemp() because WASI has no temporary directory, so create the file relative to the
  // working directory instead.
  constexpr char kChars[] = "abcdefghijklmnopqrstuvwxyz0123456789";
  const size_t length = strlen(name_template);
  if (length < 6 || strcmp(name_template + length - 6, "XXXXXX") != 0) {
    errno = EINVAL;
    return -1;
  }
  static std::minstd_rand rng{std::random_device{}()};
  for (int attempt = 0; attempt < 100; ++attempt) {
    for (size_t i = length - 6; i < length; ++i) {
      name_template[i] = kChars[rng() % (sizeof(kChars) - 1)];
    }
    int fd = open(name_template, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (fd >= 0 || errno != EEXIST) {
      return fd;
    }
  }
  errno = EEXIST;
  return -1;
#else
  return mkstemp(name_template);
#endif
}
#endif

PathString GetSharedLibraryFileName(const PathString& base_library_name) {
#if defined(_WIN32)
  constexpr auto kPrefix{ORT_TSTR("")}, kSuffix{ORT_TSTR(".dll")};
#elif defined(__APPLE__)
  constexpr auto kPrefix{ORT_TSTR("lib")}, kSuffix{ORT_TSTR(".dylib")};
#else
  constexpr auto kPrefix{ORT_TSTR("lib")}, kSuffix{ORT_TSTR(".so")};
#endif

  return PathString{kPrefix} + base_library_name + kSuffix;
}

void DeleteFileFromDisk(const ORTCHAR_T* path) {
#ifdef _WIN32
  ASSERT_EQ(TRUE, DeleteFileW(path));
#else
  ASSERT_EQ(0, unlink(path));
#endif
}
void CreateTestFile(int& out, std::basic_string<ORTCHAR_T>& filename_template) {
  if (filename_template.empty())
    ORT_THROW_EX(std::runtime_error, "file name template can't be empty");

  ORTCHAR_T* filename = const_cast<ORTCHAR_T*>(filename_template.c_str());
#ifdef _WIN32
  ASSERT_EQ(0, _wmktemp_s(filename, filename_template.length() + 1));
  int fd;
  int err = _wsopen_s(&fd, filename, _O_CREAT | _O_EXCL | _O_SEQUENTIAL | _O_BINARY | _O_WRONLY, _SH_DENYRW, _S_IREAD | _S_IWRITE);
  if (err != 0)
    ORT_THROW_EX(std::runtime_error, "open temp file failed");
#else
  int fd = MakeTempFile(filename);
  if (fd < 0) {
    ORT_THROW_EX(std::runtime_error, "open temp file failed");
  }
#endif
  out = fd;
}
void CreateTestFile(FILE*& out, std::basic_string<ORTCHAR_T>& filename_template) {
  if (filename_template.empty())
    ORT_THROW_EX(std::runtime_error, "file name template can't be empty");

  ORTCHAR_T* filename = const_cast<ORTCHAR_T*>(filename_template.c_str());
#ifdef _WIN32
  ASSERT_EQ(0, _wmktemp_s(filename, filename_template.length() + 1));
  FILE* fp = nullptr;
  ASSERT_EQ(0, _wfopen_s(&fp, filename, ORT_TSTR("wb")));
#else
  int fd = MakeTempFile(filename);
  if (fd < 0) {
    ORT_THROW_EX(std::runtime_error, "open temp file failed");
  }
  FILE* fp = fdopen(fd, "w");
#endif
  out = fp;
}
}  // namespace test
}  // namespace onnxruntime
