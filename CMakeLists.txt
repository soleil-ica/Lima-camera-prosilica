###########################################################################
# This file is part of LImA, a Library for Image Acquisition
#
#  Copyright (C) : 2009-2017
#  European Synchrotron Radiation Facility
#  CS40220 38043 Grenoble Cedex 9 
#  FRANCE
# 
#  Contact: lima@esrf.fr
# 
#  This is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 3 of the License, or
#  (at your option) any later version.
# 
#  This software is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
# 
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, see <http://www.gnu.org/licenses/>.
############################################################################

set (PROSILICA_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/sdk" CACHE PATH "location of prosilica sdk")
set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--add-needed")


# library path for prosilica
set(PROSILICA_LIB_DIR "${PROSILICA_SDK_ROOT}/bin/x64" CACHE PATH "location of prosilica lib")


link_directories(${PROSILICA_LIB_DIR})

#SOURCEs
set(prosilica_srcs src/ProsilicaCamera.cpp  src/ProsilicaInterface.cpp
	 src/ProsilicaDetInfoCtrlObj.cpp src/ProsilicaSyncCtrlObj.cpp
	 src/ProsilicaBinCtrlObj.cpp src/ProsilicaBufferCtrlObj.cpp
	 src/ProsilicaVideoCtrlObj.cpp)

file(STRINGS "VERSION" prosilica_vers)
add_library(limaprosilica SHARED ${prosilica_srcs})

#INCLUDEs
target_include_directories(limaprosilica PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(limaprosilica PUBLIC "${PROSILICA_SDK_ROOT}/include")

#LIBRARY
find_library(PROSILICA_LIB_BASE PvAPI ${PROSILICA_LIB_DIR})
if (("${PROSILICA_LIB_BASE}" STREQUAL "PROSILICA_LIB_BASE-NOTFOUND"))
  message(FATAL_ERROR "prosilica : libPvAPI not found, you can search it in cmake-gui or try to install it if it isn't here.")
endif()

target_link_libraries(limaprosilica limacore)
target_link_libraries(limaprosilica ${PROSILICA_LIB_BASE})

set_target_properties(limaprosilica PROPERTIES VERSION ${prosilica_vers} SOVERSION "1")

install(TARGETS limaprosilica LIBRARY DESTINATION lib)

if(COMPILE_SIP)
    set(NAME "prosilica")
	include(CameraRunSIP)
    install(FILES python/__init__.py DESTINATION "${PYTHON_SITE_PACKAGES_DIR}/Lima/Prosilica")
endif()
