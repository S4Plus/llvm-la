include(BuiltinTests)
include(CheckCSourceCompiles)

# Make all the tests only check the compiler
set(TEST_COMPILE_ONLY On)

# Check host compiler support for certain flags
builtin_check_c_compiler_flag(-fPIC                 COMPILER_RT_HAS_FPIC_FLAG)
builtin_check_c_compiler_flag(-fPIE                 COMPILER_RT_HAS_FPIE_FLAG)
builtin_check_c_compiler_flag(-fno-builtin          COMPILER_RT_HAS_FNO_BUILTIN_FLAG)
builtin_check_c_compiler_flag(-std=c11              COMPILER_RT_HAS_STD_C11_FLAG)
builtin_check_c_compiler_flag(-fvisibility=hidden   COMPILER_RT_HAS_VISIBILITY_HIDDEN_FLAG)
builtin_check_c_compiler_flag(-fomit-frame-pointer  COMPILER_RT_HAS_OMIT_FRAME_POINTER_FLAG)
builtin_check_c_compiler_flag(-ffreestanding        COMPILER_RT_HAS_FREESTANDING_FLAG)
builtin_check_c_compiler_flag(-fxray-instrument     COMPILER_RT_HAS_XRAY_COMPILER_FLAG)

builtin_check_c_compiler_source(COMPILER_RT_HAS_ATOMIC_KEYWORD
"
int foo(int x, int y) {
 _Atomic int result = x * y;
 return result;
}
")


set(ARM64 aarch64)
set(ARM32 arm armhf armv6m armv7m armv7em armv7 armv7s armv7k)
set(HEXAGON hexagon)
set(X86 i386)
set(X86_64 x86_64)
set(MIPS32 mips mipsel)
set(MIPS64 mips64 mips64el)
set(PPC64 powerpc64 powerpc64le)
set(RISCV32 riscv32)
set(RISCV64 riscv64)
set(WASM32 wasm32)
set(WASM64 wasm64)
set(LOONGARCH64 loongarch64)

if(APPLE)
  set(ARM64 arm64)
  set(ARM32 armv7 armv7k armv7s)
  set(X86_64 x86_64 x86_64h)
endif()

set(ALL_BUILTIN_SUPPORTED_ARCH ${X86} ${X86_64} ${ARM32} ${ARM64}
  ${HEXAGON} ${MIPS32} ${MIPS64} ${PPC64} ${RISCV32} ${RISCV64} ${WASM32} ${WASM64} ${LOONGARCH64})

include(CompilerRTUtils)
include(CompilerRTDarwinUtils)

if(APPLE)

  find_darwin_sdk_dir(DARWIN_osx_SYSROOT macosx)
  find_darwin_sdk_dir(DARWIN_iossim_SYSROOT iphonesimulator)
  find_darwin_sdk_dir(DARWIN_ios_SYSROOT iphoneos)
  find_darwin_sdk_dir(DARWIN_watchossim_SYSROOT watchsimulator)
  find_darwin_sdk_dir(DARWIN_watchos_SYSROOT watchos)
  find_darwin_sdk_dir(DARWIN_tvossim_SYSROOT appletvsimulator)
  find_darwin_sdk_dir(DARWIN_tvos_SYSROOT appletvos)

  set(DARWIN_EMBEDDED_PLATFORMS)
  set(DARWIN_osx_BUILTIN_MIN_VER 10.5)
  set(DARWIN_osx_BUILTIN_MIN_VER_FLAG
      -mmacosx-version-min=${DARWIN_osx_BUILTIN_MIN_VER})

  if(COMPILER_RT_ENABLE_IOS)
    list(APPEND DARWIN_EMBEDDED_PLATFORMS ios)
    set(DARWIN_ios_MIN_VER_FLAG -miphoneos-version-min)
    set(DARWIN_ios_BUILTIN_MIN_VER 6.0)
    set(DARWIN_ios_BUILTIN_MIN_VER_FLAG
      ${DARWIN_ios_MIN_VER_FLAG}=${DARWIN_ios_BUILTIN_MIN_VER})
  endif()
  if(COMPILER_RT_ENABLE_WATCHOS)
    list(APPEND DARWIN_EMBEDDED_PLATFORMS watchos)
    set(DARWIN_watchos_MIN_VER_FLAG -mwatchos-version-min)
    set(DARWIN_watchos_BUILTIN_MIN_VER 2.0)
    set(DARWIN_watchos_BUILTIN_MIN_VER_FLAG
      ${DARWIN_watchos_MIN_VER_FLAG}=${DARWIN_watchos_BUILTIN_MIN_VER})
  endif()
  if(COMPILER_RT_ENABLE_TVOS)
    list(APPEND DARWIN_EMBEDDED_PLATFORMS tvos)
    set(DARWIN_tvos_MIN_VER_FLAG -mtvos-version-min)
    set(DARWIN_tvos_BUILTIN_MIN_VER 9.0)
    set(DARWIN_tvos_BUILTIN_MIN_VER_FLAG
      ${DARWIN_tvos_MIN_VER_FLAG}=${DARWIN_tvos_BUILTIN_MIN_VER})
  endif()

  set(BUILTIN_SUPPORTED_OS osx)

  # We're setting the flag manually for each target OS
  set(CMAKE_OSX_DEPLOYMENT_TARGET "")

  if(NOT DARWIN_osx_ARCHS)
    set(DARWIN_osx_ARCHS i386 x86_64 x86_64h)
  endif()

  set(DARWIN_sim_ARCHS i386 x86_64)
  set(DARWIN_device_ARCHS armv7 armv7s armv7k arm64)

  message(STATUS "OSX supported arches: ${DARWIN_osx_ARCHS}")
  foreach(arch ${DARWIN_osx_ARCHS})
    list(APPEND COMPILER_RT_SUPPORTED_ARCH ${arch})
    set(CAN_TARGET_${arch} 1)
  endforeach()

  foreach(platform ${DARWIN_EMBEDDED_PLATFORMS})
    if(DARWIN_${platform}sim_SYSROOT)
      set(DARWIN_${platform}sim_BUILTIN_MIN_VER
        ${DARWIN_${platform}_BUILTIN_MIN_VER})
      set(DARWIN_${platform}sim_BUILTIN_MIN_VER_FLAG
        ${DARWIN_${platform}_BUILTIN_MIN_VER_FLAG})

      set(DARWIN_${platform}sim_SKIP_CC_KEXT On)

      set(test_arches ${DARWIN_sim_ARCHS})
      if(DARWIN_${platform}sim_ARCHS)
        set(test_arches DARWIN_${platform}sim_ARCHS)
      endif()

      darwin_test_archs(${platform}sim
        DARWIN_${platform}sim_ARCHS
        ${test_arches})
      message(STATUS "${platform} Simulator supported builtin arches: ${DARWIN_${platform}sim_ARCHS}")
      if(DARWIN_${platform}sim_ARCHS)
        list(APPEND BUILTIN_SUPPORTED_OS ${platform}sim)
      endif()
      foreach(arch ${DARWIN_${platform}sim_ARCHS})
        list(APPEND COMPILER_RT_SUPPORTED_ARCH ${arch})
        set(CAN_TARGET_${arch} 1)
      endforeach()
    endif()

    if(DARWIN_${platform}_SYSROOT)
      set(test_arches ${DARWIN_device_ARCHS})
      if(DARWIN_${platform}_ARCHS)
        set(test_arches DARWIN_${platform}_ARCHS)
      endif()

      darwin_test_archs(${platform}
        DARWIN_${platform}_ARCHS
        ${test_arches})
      message(STATUS "${platform} supported builtin arches: ${DARWIN_${platform}_ARCHS}")
      if(DARWIN_${platform}_ARCHS)
        list(APPEND BUILTIN_SUPPORTED_OS ${platform})
      endif()
      foreach(arch ${DARWIN_${platform}_ARCHS})
        list(APPEND COMPILER_RT_SUPPORTED_ARCH ${arch})
        set(CAN_TARGET_${arch} 1)
      endforeach()
    endif()
  endforeach()

  list_intersect(BUILTIN_SUPPORTED_ARCH ALL_BUILTIN_SUPPORTED_ARCH COMPILER_RT_SUPPORTED_ARCH)

else()
  # If we're not building the builtins standalone, just rely on the  tests in
  # config-ix.cmake to tell us what to build. Otherwise we need to do some leg
  # work here...
  if(COMPILER_RT_BUILTINS_STANDALONE_BUILD)
    test_targets()
  endif()
  # Architectures supported by compiler-rt libraries.
  filter_available_targets(BUILTIN_SUPPORTED_ARCH
    ${ALL_BUILTIN_SUPPORTED_ARCH})
endif()

message(STATUS "Builtin supported architectures: ${BUILTIN_SUPPORTED_ARCH}")
