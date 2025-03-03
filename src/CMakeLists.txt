# Copyright © 2007-2016 Evgeny Ratnikov
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

SET(TARGET "termit")

INCLUDE_DIRECTORIES(${TERMIT_SOURCE_DIR})

INCLUDE (FindPkgConfig)

pkg_search_module(GTK REQUIRED gtk+-3.0>=3.22)
pkg_search_module(VTE REQUIRED libvte>=0.46 vte>=0.46 vte-2.91>=0.46)
IF(NOT VTE_FOUND)
  message(FATAL_ERROR "Vte library was not found.")
ENDIF(NOT VTE_FOUND)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LUA REQUIRED lua>=5.3)

IF(NOT LUA_FOUND)
  message(FATAL_ERROR "Lua library version 5.3 or higher was not found.")
ENDIF(NOT LUA_FOUND)

INCLUDE(Sources.cmake)

SET(TERMIT_CFLAGS "")
IF(${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD")
  SET(TERMIT_CFLAGS "${TERMIT_CFLAGS} -I/usr/local/include")
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD")
FOREACH(cflag ${VTE_CFLAGS} ${GTK_CFLAGS} ${LUA_CFLAGS})
  SET(TERMIT_CFLAGS "${TERMIT_CFLAGS} ${cflag}")
ENDFOREACH(cflag)

IF(DEBUG)
  SET(EXTRA_CFLAGS "-std=c99 -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter")
  SET(TERMIT_CFLAGS "${TERMIT_CFLAGS} ${EXTRA_CFLAGS} -g -DDEBUG -DGTK_DISABLE_DEPRECATED")
ELSE(DEBUG)
  SET(TERMIT_CFLAGS "${TERMIT_CFLAGS} -O2")
ENDIF(DEBUG)

SET(TERMIT_LDFLAGS "${VTE_LDFLAGS};${GTK_LDFLAGS};${LUA_LIBRARIES};-lX11")

SET_SOURCE_FILES_PROPERTIES(${TERMIT_SRCS} PROPERTIES 
  COMPILE_FLAGS "${TERMIT_CFLAGS}")

ADD_EXECUTABLE(${TARGET} ${TERMIT_SRCS})

target_link_libraries(${TARGET} ${TERMIT_LDFLAGS})

INSTALL(TARGETS ${TARGET} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

