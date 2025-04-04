# check if git is installed in system
find_program(git_executable git)
# check if ${CMAKE_SOURCE_DIR} is git repository if git is installed
# and set git_branch
if(git_executable)
  execute_process(
    COMMAND git rev-parse --is-inside-work-tree
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    RESULT_VARIABLE git_repo)
  if(NOT git_repo EQUAL 0)
    set(git_executable "")
  else()
    # git_branch
    execute_process(
      COMMAND git rev-parse --abbrev-ref HEAD
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      OUTPUT_VARIABLE git_branch
      OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif()
endif()
# set build_release
if ("${git_branch}" STREQUAL "master")
  # git_commit
  execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE git_commit
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  set(build_release OFF)
else()
  set(build_release ON)
endif()
set(original_filename "rime.dll")
if(MSVC)
  message(STATUS "MSVC is detected in AddRCINfo.cmake")
  set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -DLOCALIZE_VERSION_INFO")

  # Search for rc.exe in common Windows SDK locations
  find_program(RC_COMPILER
      NAMES rc.exe
      PATHS
      "C:/Program Files (x86)/Windows Kits/10/bin/10.0.22621.0/x64"
      "C:/Program Files (x86)/Windows Kits/10/bin/10.0.22000.0/x64"
      "C:/Program Files (x86)/Windows Kits/10/bin/10.0.20348.0/x64"
      "C:/Program Files (x86)/Windows Kits/10/bin/10.0.19041.0/x64"
      "C:/Program Files (x86)/Windows Kits/10/bin/10.0.18362.0/x64"
      "C:/Program Files (x86)/Windows Kits/10/bin/x64"
      "C:/Program Files (x86)/Microsoft SDKs/Windows/v10.0A/bin/NETFX 4.8 Tools/x64"
      "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.29.30133/bin/Hostx64/x64"
      "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.38.33130/bin/Hostx64/x64"
      NO_DEFAULT_PATH
  )

  if(RC_COMPILER)
      message(STATUS "Found RC_COMPILER: ${RC_COMPILER}")
      set(CMAKE_RC_COMPILER ${RC_COMPILER})
      set(CMAKE_RC_COMPILER_INIT ${RC_COMPILER})
      set(CMAKE_RC_FLAGS "/nologo")
  else()
      message(FATAL_ERROR "Could not find Microsoft RC compiler. Please install Windows SDK or Visual Studio with Windows SDK components.")
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU") # mingw
  set(original_filename "librime.dll")
endif()

# generate tag_suffix for nightly and release
if(build_release)
  set(tag_suffix ".0")
else(build_release)
  # arch_suffix
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(arch_suffix "x64")
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(arch_suffix "Win32")
  endif()

  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang") # clang
    set(arch_suffix "${arch_suffix} clang")
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU") # mingw
    set(arch_suffix "${arch_suffix} mingw")
  endif()
  # set tag_suffix
  set(tag_suffix "-${git_commit} Nightly build ${arch_suffix}")
endif(build_release)

# if mingw env add -c 65001 to CMAKE_RC_FLAGS
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -c 65001")
endif()
# if clang build, use llvm-rc to compile resource file
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(CMAKE_RC_COMPILER "llvm-rc")
  set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -finput-charset=UTF-8")
endif()

# set resource file
set(rime_resource_file "${CMAKE_CURRENT_SOURCE_DIR}/rime.rc")
# convert rime_version to comma separated format
string(REPLACE "." "," rime_version_comma_separated ${rime_version})
# configure resource file, make version info to actually value
configure_file(${rime_resource_file} ${CMAKE_CURRENT_BINARY_DIR}/rime.rc @ONLY)
# append resource file to source file list
list(APPEND rime_core_module_src ${CMAKE_CURRENT_BINARY_DIR}/rime.rc)
