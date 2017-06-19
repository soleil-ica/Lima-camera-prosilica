############################################################################
# This file is part of LImA, a Library for Image Acquisition
#
# Copyright (C) : 2009-2017
# European Synchrotron Radiation Facility
# BP 220, Grenoble 38043
# FRANCE
#
# This is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <http://www.gnu.org/licenses/>.
############################################################################
# CMakeLists.txt for Prosilica camera

SET (PROSILICA_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/sdk" CACHE PATH "location of prosilica sdk")
SET(CMAKE_SHARED_LINKER_FLAGS "-Wl,--add-needed")


# library path for prosilica
SET(PROSILICA_LIB_DIR "${PROSILICA_SDK_ROOT}/bin/x64" CACHE PATH "location of prosilica lib")


LINK_DIRECTORIES(${PROSILICA_LIB_DIR})

#SOURCEs
SET(prosilica_srcs src/ProsilicaCamera.cpp  src/ProsilicaInterface.cpp
	 src/ProsilicaDetInfoCtrlObj.cpp src/ProsilicaSyncCtrlObj.cpp
	 src/ProsilicaBinCtrlObj.cpp src/ProsilicaBufferCtrlObj.cpp
	 src/ProsilicaVideoCtrlObj.cpp)

FILE(STRINGS "VERSION" prosilica_vers)
ADD_LIBRARY(limaprosilica SHARED ${prosilica_srcs})

#INCLUDEs
TARGET_INCLUDE_DIRECTORIES(limaprosilica PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
TARGET_INCLUDE_DIRECTORIES(limaprosilica PUBLIC "${PROSILICA_SDK_ROOT}/include")

#LIBRARY
FIND_LIBRARY(PROSILICA_LIB_BASE PvAPI ${PROSILICA_LIB_DIR})
IF (("${PROSILICA_LIB_BASE}" STREQUAL "PROSILICA_LIB_BASE-NOTFOUND"))
  MESSAGE(FATAL_ERROR "prosilica : libPvAPI not found, you can search it in cmake-gui or try to install it if it isn't here.")
ENDIF()

TARGET_LINK_LIBRARIES(limaprosilica limacore)
TARGET_LINK_LIBRARIES(limaprosilica ${PROSILICA_LIB_BASE})

SET_TARGET_PROPERTIES(limaprosilica PROPERTIES VERSION ${prosilica_vers} SOVERSION "1")

INSTALL(TARGETS limaprosilica LIBRARY DESTINATION lib)

IF(COMPILE_SIP)
    SET(NAME "prosilica")
	include(CameraRunSIP)
    INSTALL(FILES python/__init__.py DESTINATION "${PYTHON_SITE_PACKAGES_DIR}/Lima/Prosilica")
ENDIF()
