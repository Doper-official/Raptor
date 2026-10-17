# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "F:/coding projects/C++ projects/Engine/out/build/x64-Release/_deps/tinyobjloader-src")
  file(MAKE_DIRECTORY "F:/coding projects/C++ projects/Engine/out/build/x64-Release/_deps/tinyobjloader-src")
endif()
file(MAKE_DIRECTORY
  "F:/coding projects/C++ projects/Engine/out/build/x64-Release/_deps/tinyobjloader-build"
  "F:/coding projects/C++ projects/Engine/out/build/x64-Release/_deps/tinyobjloader-subbuild/tinyobjloader-populate-prefix"
  "F:/coding projects/C++ projects/Engine/out/build/x64-Release/_deps/tinyobjloader-subbuild/tinyobjloader-populate-prefix/tmp"
  "F:/coding projects/C++ projects/Engine/out/build/x64-Release/_deps/tinyobjloader-subbuild/tinyobjloader-populate-prefix/src/tinyobjloader-populate-stamp"
  "F:/coding projects/C++ projects/Engine/out/build/x64-Release/_deps/tinyobjloader-subbuild/tinyobjloader-populate-prefix/src"
  "F:/coding projects/C++ projects/Engine/out/build/x64-Release/_deps/tinyobjloader-subbuild/tinyobjloader-populate-prefix/src/tinyobjloader-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "F:/coding projects/C++ projects/Engine/out/build/x64-Release/_deps/tinyobjloader-subbuild/tinyobjloader-populate-prefix/src/tinyobjloader-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "F:/coding projects/C++ projects/Engine/out/build/x64-Release/_deps/tinyobjloader-subbuild/tinyobjloader-populate-prefix/src/tinyobjloader-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
