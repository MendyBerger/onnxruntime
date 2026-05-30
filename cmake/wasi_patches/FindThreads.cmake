# WASI stub for FindThreads.cmake
# WASI has no real pthreads. Return success with an empty no-op target so
# dependencies that call find_package(Threads REQUIRED) don't fail.
set(CMAKE_THREAD_LIBS_INIT "")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(Threads_FOUND TRUE)
if(NOT TARGET Threads::Threads)
  add_library(Threads::Threads INTERFACE IMPORTED GLOBAL)
endif()
