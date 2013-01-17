#  Copyright (c) 2013 Pavel Zaichenkov <zaichenkov@gmail.com>
#
#  Permission to use, copy, modify, and distribute this software for any
#  purpose with or without fee is hereby granted, provided that the above
#  copyright notice and this permission notice appear in all copies.
#
#  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
#  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
#  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
#  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
#  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
#  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
#  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

cmake_minimum_required (VERSION 2.6)

# Name of the project
project (pipo C)

# Uncomment this if you want to have a verbose
# makefile. It is useful for debugging. 
# set (CMAKE_VERBOSE_MAKEFILE ON)

option (BUILD_LEXER "compiling lexer" OFF)
option (RELEASE "build the compiler with highest optimisations" OFF)


if (RELEASE)
  set (CFLAGS "-O3 -fomit-frame-pointer -Wall -Wextra -Wstrict-prototypes "
	      "-pedantic -std=c99 -D_BSD_SOURCE"
	      " -D_GNU_SOURCE -pedantic -fPIC")
  message(STATUS "Compiling release version")
else()
  set (CFLAGS "-g -gdwarf-2 -g3 -Wall -Wextra -Wstrict-prototypes "
	      "-pedantic -std=c99 -D_BSD_SOURCE"
	      " -D_GNU_SOURCE -pedantic -fPIC")
  message(STATUS "Compiling debug version")
endif()

# Custom messages
if (BUILD_LEXER)
  message(STATUS "Only lexer is being built")
endif()

include (cmake/version.cmake)

configure_file (
  "${PROJECT_SOURCE_DIR}/src/config.h.in"
  "${PROJECT_BINARY_DIR}/config.h"
)

include_directories ("${PROJECT_BINARY_DIR}")
include_directories ("src")

add_definitions (${CFLAGS})
if (BUILD_LEXER)
  add_definitions(-DLEXER_BINARY)
  add_executable (pipo src/lex.c)
else()
  add_subdirectory (src)
  add_executable (pipo src/main.c)
  target_link_libraries (pipo pipolib)
endif()

# Installing pipo binary, libraries and include files.
set (CMAKE_INSTALL_PREFIX "$ENV{HOME}/local")
if (BUILD_LEXER)
  install (TARGETS pipo DESTINATION bin)
else()
  install (TARGETS pipo DESTINATION bin)
  file (GLOB headers "${PROJECT_SOURCE_DIR}/src/*.h")
  file (GLOB defs "${PROJECT_SOURCE_DIR}/src/*.def")
  install (FILES ${headers} ${defs} DESTINATION include/pipo)
endif()
