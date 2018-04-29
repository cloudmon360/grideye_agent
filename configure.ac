#
# Copyright (C) 2015-2016 Olof Hagsand
#
# This file is part of GRIDEYE.
#
# GRIDEYE is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# GRIDEYE is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GRIDEYE; see the file LICENSE.  If not, see
# <http://www.gnu.org/licenses/>.
#

# To rebuild the `configure' script from this, execute the command
# 	autoconf
# in the directory containing this script.
#
#
#

AC_INIT(grideye_agent.c)

# Default CFLAGS unless set by environment.
: ${CFLAGS="-O2"}

#AC_CONFIG_HEADER(grideye_config.h)
# clicon versions spread to Makefile's (.so files) and variable in build.c
AC_SUBST(GRIDEYE_VERSION)
AC_SUBST(GRIDEYE_VERSION_MAJOR)
AC_SUBST(GRIDEYE_VERSION_MINOR)
AC_SUBST(CLIGEN_VERSION) # Bind to specific CLIgen version
AC_SUBST(CLIXON_VERSION) # Bind to specific CLIgen version

AC_DEFUN(MY_EXPAND_DIR, [
                $1=$2
                $1=`(
                     test "x$prefix" = xNONE && prefix="$ac_default_prefix"
                     test "x$exec_prefix" = xNONE && exec_prefix="${prefix}"
                     eval echo \""[$]$1"\"
                    )`
              ])


GRIDEYE_VERSION_MAJOR="1"
GRIDEYE_VERSION_MINOR="4"
GRIDEYE_VERSION_PATCH="0"
GRIDEYE_VERSION="${GRIDEYE_VERSION_MAJOR}.${GRIDEYE_VERSION_MINOR}.${GRIDEYE_VERSION_PATCH}-PRE"
CLIGEN_VERSION="3" # Or newer?
CLIXON_VERSION="3" # Or newer?

AC_MSG_RESULT(GRIDEYE version ${GRIDEYE_VERSION})

AC_CANONICAL_TARGET

AC_SUBST(CC)
AC_SUBST(CFLAGS)
AC_SUBST(LDFLAGS)
AC_SUBST(INCLUDES)
AC_SUBST(CPPFLAGS)
AC_SUBST(LIBS)

# Check for cligen
AC_CHECK_HEADERS(cligen/cligen.h,, AC_MSG_ERROR(cligen missing. Try: git clone https://github.com/olofhagsand/cligen.git))
AC_CHECK_LIB(cligen, cligen_init,, AC_MSG_ERROR(cligen missing. Try: git clone https://github.com/olofhagsand/cligen.git))


# Check for clixon
AC_CHECK_HEADERS(clixon/clixon.h,, AC_MSG_ERROR(clixon missing. Try: git clone https://github.com/clicon/clixon.git), [[ #include <cligen/cligen.h> ]])
AC_SEARCH_LIBS(clicon_handle_init, clixon,, AC_MSG_ERROR(clixon missing. Try: git clone https://github.com/clicon/clixon.git), -lcligen)

AC_PROG_CC()
AC_PROG_CPP
AC_PROG_INSTALL

CPPFLAGS="-DHAVE_CONFIG_H ${CPPFLAGS}"

AC_MSG_RESULT(compiler is $CC)	

CFLAGS="${CFLAGS} -Wall" # -Werror

AC_MSG_RESULT(CPPFLAGS is $CPPFLAGS)	
AC_MSG_RESULT(CFLAGS is $CFLAGS)	

LDFLAGS=""
AC_CHECK_LIB(m, main)
EXE_SUFFIX=""
OBJ_SUFFIX=".o"
AR_SUFFIX=".a"
SH_SUFFIX=".so"
AR="ar"

dnl ----------------------------
dnl check sin_len of sockaddr_in
dnl ----------------------------
AC_MSG_CHECKING(whether struct sockaddr_in has a sin_len field)
AC_TRY_COMPILE([#include <sys/types.h>
#include <netinet/in.h>
],[static struct sockaddr_in ac_i;int ac_j = sizeof (ac_i.sin_len); ac_j++;],
[AC_MSG_RESULT(yes)
 AC_DEFINE(HAVE_SIN_LEN)],
 AC_MSG_RESULT(no))

# influxdb api uses libcurl
AC_CHECK_LIB(curl, curl_global_init,, AC_MSG_ERROR([libcurl missing]))

AC_CHECK_LIB(socket, socket)

# Programming interface to dynamic linking loader
AC_CHECK_LIB(dl, dlopen)

AC_CHECK_HEADERS(sys/sysinfo.h)

AC_CHECK_FUNCS(clock_gettime)

AC_SUBST(GRIDEYE_PLUGIN_DIR, $libdir/grideye) # Bind to specific CLIgen version
echo "LIBDIR: $libdir"
echo "GRIDEYE_PLUGIN_DIR: $GRIDEYE_PLUGIN_DIR"

# Check Python3
echo Checking for python
AC_PATH_PROGS(pathpython3, [python3 python3.6 python3.5 python3.4 python3.3 python3.2 python3.1 python3.0])
if test -z "$pathpython3" ; then
  AC_MSG_ERROR(I can't find python3);
fi

AC_PATH_PROGS(pathpython_config, [python3-config])
if test -z "$pathpython_config" ; then
  AC_MSG_NOTICE(I can't find python$PYTHON_VERSION-config. python$PYTHON_VERSION-config is required)
fi

CFLAGS+=" `$pathpython_config --includes`"
LDFLAGS+=" `$pathpython_config --libs`"

AC_OUTPUT(
	Makefile
	plugins/Makefile
	docker/Makefile
	docker/Dockerfile

)
