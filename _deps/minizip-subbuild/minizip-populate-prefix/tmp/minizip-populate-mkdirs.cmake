# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "L:/programming/C++/DuckShell/_deps/minizip-src")
  file(MAKE_DIRECTORY "L:/programming/C++/DuckShell/_deps/minizip-src")
endif()
file(MAKE_DIRECTORY
  "L:/programming/C++/DuckShell/_deps/minizip-build"
  "L:/programming/C++/DuckShell/_deps/minizip-subbuild/minizip-populate-prefix"
  "L:/programming/C++/DuckShell/_deps/minizip-subbuild/minizip-populate-prefix/tmp"
  "L:/programming/C++/DuckShell/_deps/minizip-subbuild/minizip-populate-prefix/src/minizip-populate-stamp"
  "L:/programming/C++/DuckShell/_deps/minizip-subbuild/minizip-populate-prefix/src"
  "L:/programming/C++/DuckShell/_deps/minizip-subbuild/minizip-populate-prefix/src/minizip-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "L:/programming/C++/DuckShell/_deps/minizip-subbuild/minizip-populate-prefix/src/minizip-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "L:/programming/C++/DuckShell/_deps/minizip-subbuild/minizip-populate-prefix/src/minizip-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
