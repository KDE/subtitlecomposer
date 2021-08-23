# - Try to find PocketSphinx
# Once done this will define
#  POCKETSPHINX_FOUND - System has PocketSphinx
#  POCKETSPHINX_VERSION - PocketSphinx version
#  POCKETSPHINX_INCLUDE_DIRS - The PocketSphinx include directories
#  POCKETSPHINX_LIBRARIES - The libraries needed to use PocketSphinx
#  POCKETSPHINX_DEFINITIONS - Compiler switches required for using PocketSphinx
#  POCKETSPHINX_MODELDIR - Directory that contains PocketSphinx models

# SPDX-FileCopyrightText: 2010-2019 Mladen Milinkovic <maxrd2@smoothware.net>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

find_package(PkgConfig REQUIRED)
pkg_check_modules(PC_POCKETSPHINX QUIET pocketsphinx)
set(POCKETSPHINX_DEFINITIONS ${PC_POCKETSPHINX_CFLAGS_OTHER})
set(POCKETSPHINX_VERSION ${PC_POCKETSPHINX_VERSION})

find_path(POCKETSPHINX_INCDIR pocketsphinx.h HINTS ${PC_POCKETSPHINX_INCLUDEDIR} ${PC_POCKETSPHINX_INCLUDE_DIRS})
find_path(SPHINXBASE_INCDIR sphinx_config.h HINTS ${PC_POCKETSPHINX_INCLUDEDIR} ${PC_POCKETSPHINX_INCLUDE_DIRS})
set(POCKETSPHINX_INCLUDE_DIR ${POCKETSPHINX_INCDIR} ${SPHINXBASE_INCDIR})
unset(POCKETSPHINX_INCDIR)
unset(SPHINXBASE_INCDIR)

foreach(_LIB ${PC_POCKETSPHINX_LIBRARIES})
	set(_LIB_PATH "_LIB_PATH-NOTFOUND")
	find_library(_LIB_PATH
		NAMES ${_LIB}
		HINTS ${PC_POCKETSPHINX_LIBDIR} ${PC_POCKETSPHINX_LIBRARY_DIRS})
	set(POCKETSPHINX_LIBRARIES ${POCKETSPHINX_LIBRARIES} ${_LIB_PATH})
endforeach(_LIB)

set(POCKETSPHINX_INCLUDE_DIRS ${POCKETSPHINX_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set POCKETSPHINX_FOUND to TRUE if all listed variables are TRUE
find_package_handle_standard_args(PocketSphinx
	REQUIRED_VARS POCKETSPHINX_LIBRARIES POCKETSPHINX_INCLUDE_DIR
	VERSION_VAR POCKETSPHINX_VERSION)

execute_process(COMMAND pkg-config --variable=modeldir pocketsphinx
				OUTPUT_VARIABLE POCKETSPHINX_MODELDIR_OUTPUT
				OUTPUT_STRIP_TRAILING_WHITESPACE)
set(POCKETSPHINX_MODELDIR ${POCKETSPHINX_MODELDIR_OUTPUT})

mark_as_advanced(POCKETSPHINX_INCLUDE_DIR POCKETSPHINX_LIBRARY)
