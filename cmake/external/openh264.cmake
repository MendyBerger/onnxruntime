# OpenH264 integration for WASI builds
# OpenH264 uses Makefiles, so we need to build it with ExternalProject

if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  set(OPENH264_URL https://github.com/cisco/openh264.git)
  set(OPENH264_TAG v2.3.1)

  set(OPENH264_SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/openh264/src/openh264)
  set(OPENH264_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/openh264/install)
  set(OPENH264_LIB_DIR ${OPENH264_INSTALL_DIR}/lib)
  set(OPENH264_INCLUDE_DIR ${OPENH264_INSTALL_DIR}/include)

  # Determine WASI SDK path
  if(NOT DEFINED ENV{WASI_SDK_PATH})
    message(FATAL_ERROR "WASI_SDK_PATH environment variable is not set")
  endif()
  set(WASI_SDK_PATH $ENV{WASI_SDK_PATH})

  # Set up WASI toolchain variables
  set(WASI_CC "${WASI_SDK_PATH}/bin/wasm32-wasip2-clang")
  set(WASI_CXX "${WASI_SDK_PATH}/bin/wasm32-wasip2-clang++")
  set(WASI_AR "${WASI_SDK_PATH}/bin/llvm-ar")
  set(WASI_RANLIB "${WASI_SDK_PATH}/bin/llvm-ranlib")

  # OpenH264 build flags for WASI
  set(OPENH264_CFLAGS "-target wasm32-wasip2 -fno-exceptions -fno-rtti")
  set(OPENH264_CXXFLAGS "-target wasm32-wasip2 -fno-exceptions -fno-rtti")
  set(OPENH264_LDFLAGS "-target wasm32-wasip2 -Wl,--no-entry")

  # Build OpenH264 using ExternalProject
  # Note: OpenH264's Makefile may need patches for WASI support
  ExternalProject_Add(project_openh264
    PREFIX openh264
    GIT_REPOSITORY ${OPENH264_URL}
    GIT_TAG ${OPENH264_TAG}
    SOURCE_DIR ${OPENH264_SOURCE_DIR}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ${CMAKE_COMMAND} -E env
      "CC=${WASI_CC}"
      "CXX=${WASI_CXX}"
      "AR=${WASI_AR}"
      "RANLIB=${WASI_RANLIB}"
      "CFLAGS=${OPENH264_CFLAGS}"
      "CXXFLAGS=${OPENH264_CXXFLAGS}"
      "LDFLAGS=${OPENH264_LDFLAGS}"
      ${CMAKE_MAKE_PROGRAM}
      OS=linux
      ARCH=x86_64
      PREFIX=${OPENH264_INSTALL_DIR}
      install
    BUILD_IN_SOURCE 1
    INSTALL_COMMAND ""
    BUILD_ALWAYS OFF
  )

  # Create imported library target
  # Note: We don't set INTERFACE_INCLUDE_DIRECTORIES here because the directory
  # doesn't exist at configure time. Instead, we'll add it directly to consuming targets.
  add_library(openh264 STATIC IMPORTED)
  set_target_properties(openh264 PROPERTIES
    IMPORTED_LOCATION ${OPENH264_LIB_DIR}/libopenh264.a
  )

  add_dependencies(openh264 project_openh264)

  # Store paths in cache variables for use by consuming targets
  set(OPENH264_INCLUDE_DIR ${OPENH264_INCLUDE_DIR} CACHE INTERNAL "OpenH264 include directory")
  set(OPENH264_INSTALL_DIR ${OPENH264_INSTALL_DIR} CACHE INTERNAL "OpenH264 install directory")

  message(STATUS "OpenH264 will be built for WASI")
  message(STATUS "  Source: ${OPENH264_SOURCE_DIR}")
  message(STATUS "  Install: ${OPENH264_INSTALL_DIR}")
  message(STATUS "  Library: ${OPENH264_LIB_DIR}/libopenh264.a")
  message(STATUS "  Headers: ${OPENH264_INCLUDE_DIR}")
else()
  # For non-WASI builds, try to find OpenH264 via pkg-config or standard paths
  find_path(OPENH264_INCLUDE_DIR
    NAMES codec_api.h
    PATHS
      /usr/include
      /usr/local/include
      /opt/local/include
  )

  find_library(OPENH264_LIBRARY
    NAMES openh264
    PATHS
      /usr/lib
      /usr/local/lib
      /opt/local/lib
  )

  if(OPENH264_INCLUDE_DIR AND OPENH264_LIBRARY)
    add_library(openh264 UNKNOWN IMPORTED)
    set_target_properties(openh264 PROPERTIES
      IMPORTED_LOCATION ${OPENH264_LIBRARY}
      INTERFACE_INCLUDE_DIRECTORIES ${OPENH264_INCLUDE_DIR}
    )
    message(STATUS "Found OpenH264: ${OPENH264_LIBRARY}")
  else()
    message(WARNING "OpenH264 not found. Please install OpenH264 or build for WASI.")
  endif()
endif()
