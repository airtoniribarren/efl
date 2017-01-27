include(CheckSymbolExists)
include(CheckIncludeFile)
include(CheckTypeSize)
include(FindPkgConfig)
include(CTest)
find_package(Threads REQUIRED)

pkg_check_modules(CHECK check)
pkg_check_modules(LIBSYSTEMD libsystemd)
pkg_check_modules(VALGRIND valgrind)
pkg_check_modules(OPENSSL openssl>=1.0)
pkg_check_modules(GNUTLS gnutls>=2.12.16)
pkg_check_modules(LIBJPEG libjpeg REQUIRED)

set(SUGGEST_VALGRIND OFF)
if("${BUILD_PROFILE}" STREQUAL "dev")
  set(SUGGEST_VALGRIND ${VALGRIND_FOUND})
elseif("${BUILD_PROFILE}" STREQUAL "debug")
  set(SUGGEST_VALGRIND ${VALGRIND_FOUND})
endif()

EFL_OPTION(BUILD_PROFILE "How to build EFL" dev CHOICE release;dev;debug)
EFL_OPTION(EFL_NOLEGACY_API_SUPPORT "Whenever to disable non-Eo (Legacy) APIs" ON)
EFL_OPTION(EFL_EO_API_SUPPORT "Whenever to enable new Eo APIs" ON)
if(NOT EFL_EO_API_SUPPORT)
  EFL_OPTION_SET_MESSAGE(EFL_EO_API_SUPPORT "Risky!")
endif()

EFL_OPTION_BACKEND(WITH_CRYPTO "Choose crypto backend" OPENSSL GNUTLS)

EFL_OPTION(ENABLE_SYSTEMD "Enable systemd support" "${LIBSYSTEMD_FOUND}" DEPENDS "LIBSYSTEMD_FOUND" OFF)
EFL_OPTION(ENABLE_VALGRIND "Enable valgrind support" "${SUGGEST_VALGRIND}" DEPENDS "VALGRIND_FOUND" OFF)

# BEGIN: HEADER, TYPE and FUNCTION CHECKS
# KEEP EACH BLOCK SORTED

HEADER_CHECK(alloca.h)
HEADER_CHECK(asm/hwcap.h)
HEADER_CHECK(bsd/string.h)
HEADER_CHECK(dirent.h)
HEADER_CHECK(execinfo.h)
HEADER_CHECK(mcheck.h)
HEADER_CHECK(netinet/in.h)
HEADER_CHECK(stdlib.h)
HEADER_CHECK(sys/auxv.h)
HEADER_CHECK(sys/ioctl.h)
HEADER_CHECK(sys/mman.h)
HEADER_CHECK(sys/types.h)

FUNC_CHECK(alloca INCLUDE_FILES alloca.h)
FUNC_CHECK(backtrace INCLUDE_FILES execinfo.h)
FUNC_CHECK(backtrace_symbols INCLUDE_FILES execinfo.h)
FUNC_CHECK(clock_gettime INCLUDE_FILES time.h)
FUNC_CHECK(dirfd INCLUDE_FILES dirent.h sys/types.h)
FUNC_CHECK(dladdr INCLUDE_FILES dlfcn.h LIBRARIES dl DEFINITIONS "-D_GNU_SOURCE=1")
FUNC_CHECK(dlopen INCLUDE_FILES dlfcn.h LIBRARIES dl)
FUNC_CHECK(dlsym INCLUDE_FILES dlfcn.h LIBRARIES dl)
FUNC_CHECK(fchmod INCLUDE_FILES sys/stat.h)
FUNC_CHECK(fcntl INCLUDE_FILES fcntl.h)
FUNC_CHECK(fork INCLUDE_FILES unistd.h)
FUNC_CHECK(fpathconf INCLUDE_FILES unistd.h)
FUNC_CHECK(fstatat NAME HAVE_ATFILE_SOURCE INCLUDE_FILES sys/stat.h)
FUNC_CHECK(geteuid INCLUDE_FILES unistd.h)
FUNC_CHECK(getpagesize INCLUDE_FILES unistd.h)
FUNC_CHECK(getpwent INCLUDE_FILES sys/types.h pwd.h)
FUNC_CHECK(getuid INCLUDE_FILES unistd.h)
FUNC_CHECK(getxattr INCLUDE_FILES sys/types.h sys/xattr.h)
FUNC_CHECK(iconv INCLUDE_FILES iconv.h)
FUNC_CHECK(listxattr INCLUDE_FILES sys/types.h sys/xattr.h)
FUNC_CHECK(lround INCLUDE_FILES math.h LIBRARIES m)
FUNC_CHECK(mallinfo INCLUDE_FILES malloc.h)
FUNC_CHECK(malloc_info INCLUDE_FILES malloc.h)
FUNC_CHECK(malloc_usable_size INCLUDE_FILES malloc.h)
FUNC_CHECK(mkdirat INCLUDE_FILES sys/stat.h)
FUNC_CHECK(mmap INCLUDE_FILES sys/mman.h)
FUNC_CHECK(mtrace INCLUDE_FILES mcheck.h)
FUNC_CHECK(prctl INCLUDE_FILES sys/prctl.h)
FUNC_CHECK(realpath INCLUDE_FILES stdlib.h)
FUNC_CHECK(sched_getcpu INCLUDE_FILES sched.h DEFINITIONS "-D_GNU_SOURCE=1")
FUNC_CHECK(setxattr INCLUDE_FILES sys/types.h sys/xattr.h)
FUNC_CHECK(shm_open INCLUDE_FILES sys/mman.h sys/stat.h fcntl.h LIBRARIES rt)
FUNC_CHECK(siglongjmp INCLUDE_FILES setjmp.h)
FUNC_CHECK(splice INCLUDE_FILES fcntl.h DEFINITIONS "-D_GNU_SOURCE=1")
FUNC_CHECK(strerror_r INCLUDE_FILES string.h)

if(HAVE_BSD_STRING_H)
  FUNC_CHECK(strlcpy INCLUDE_FILES bsd/string.h LIBRARIES bsd)
else()
  FUNC_CHECK(strlcpy INCLUDE_FILES string.h)
endif()

TYPE_CHECK(siginfo_t INCLUDE_FILES signal.h)

# END: HEADER, TYPE and FUNCTION CHECKS

# TODO: move to a FindUnwind.cmake?
# or is pkg-config enough these days?
pkg_check_modules(UNWIND libunwind libunwind-generic)

CHECK_APPEND_DEFINE(EFL_BETA_API_SUPPORT 1)
if(CMAKE_THREAD_LIBS_INIT)
  set(EFL_HAVE_THREADS 1)
endif()
CHECK_APPEND_DEFINE(EFL_HAVE_THREADS "${EFL_HAVE_THREADS}")

CHECK_APPEND_DEFINE(HAVE_CIPHER ${WITH_CRYPTO_ENABLED})
CHECK_APPEND_DEFINE(HAVE_SIGNATURE ${WITH_CRYPTO_ENABLED})
CHECK_APPEND_DEFINE(HAVE_SYSTEMD ${ENABLE_SYSTEMD})

CHECK_APPEND_DEFINE(MODULE_ARCH "\"v-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}\"")
CHECK_APPEND_DEFINE(SHARED_LIB_SUFFIX "\"${CMAKE_SHARED_LIBRARY_SUFFIX}\"")
CHECK_APPEND_DEFINE(EXEEXT "\"${CMAKE_EXECUTABLE_SUFFIX}\"")

# TODO: change code and remove:
CHECK_APPEND_DEFINE(VMAJ ${PROJECT_VERSION_MAJOR})
CHECK_APPEND_DEFINE(VMIN ${PROJECT_VERSION_MINOR})
CHECK_APPEND_DEFINE(VMIC ${PROJECT_VERSION_PATCH})
CHECK_APPEND_DEFINE(VREV ${PROJECT_VERSION_TWEAK})

