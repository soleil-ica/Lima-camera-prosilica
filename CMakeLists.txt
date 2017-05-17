# CMakeLists.txt for Prosilica camera

SET (PROSILICA_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/sdk" CACHE PATH "location of prosilica sdk")
SET(CMAKE_SHARED_LINKER_FLAGS "-Wl,--add-needed")


# library path for prosilica
SET(PROSILICA_LIB_DIR "${PROSILICA_SDK_ROOT}/bin/x64" CACHE PATH "location of prosilica lib")


LINK_DIRECTORIES(${PROSILICA_LIB_DIR})

SET(prosilica_srcs src/ProsilicaCamera.cpp  src/ProsilicaInterface.cpp
	 src/ProsilicaDetInfoCtrlObj.cpp src/ProsilicaSyncCtrlObj.cpp
	 src/ProsilicaBinCtrlObj.cpp src/ProsilicaBufferCtrlObj.cpp
	 src/ProsilicaVideoCtrlObj.cpp)

FILE(STRINGS "VERSION" prosilica_vers)
ADD_LIBRARY(limaprosilica SHARED ${prosilica_srcs})

TARGET_INCLUDE_DIRECTORIES(limaprosilica PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
TARGET_INCLUDE_DIRECTORIES(limaprosilica PUBLIC "${PROSILICA_SDK_ROOT}/include")

FIND_LIBRARY(PROSILICA_LIB_BASE PvAPI ${PROSILICA_LIB_DIR})
IF (("${PROSILICA_LIB_BASE}" STREQUAL "PROSILICA_LIB_BASE-NOTFOUND"))
  MESSAGE(FATAL_ERROR "prosilica lib not found")
ENDIF()

TARGET_LINK_LIBRARIES(limaprosilica limacore)
TARGET_LINK_LIBRARIES(limaprosilica ${PROSILICA_LIB_BASE})

SET_TARGET_PROPERTIES(limaprosilica PROPERTIES VERSION ${prosilica_vers} SOVERSION "1")

INSTALL(TARGETS limaprosilica LIBRARY DESTINATION lib)

IF(COMPILE_SIP)
    SET(NAME "prosilica")
    SET(INCLUDES)
    FILE(GLOB sipfiles RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/sip" "${CMAKE_CURRENT_SOURCE_DIR}/sip/*.sip")
    FOREACH(sipfile ${sipfiles})
        SET(INCLUDES 
        "${INCLUDES} 
        %Include ${sipfile}")
    ENDFOREACH()

    CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/sip/limamodules.sip.in sip/limaprosilica.sip)
    SET(SIP_CONCAT_PARTS 1)
    SET(SIP_INCLUDES ${SIP_INCLUDES}
        "${CMAKE_SOURCE_DIR}/third-party/Processlib/sip"
        "${CMAKE_BINARY_DIR}/sip/core"
        "${CMAKE_SOURCE_DIR}/third-party/Processlib/tasks/sip"
        "${CMAKE_SOURCE_DIR}/common/sip"
        "${CMAKE_SOURCE_DIR}/hardware/sip"
        "${CMAKE_SOURCE_DIR}/control/sip"
        "${CMAKE_SOURCE_DIR}/control/software_operation/sip"
        "${CMAKE_CURRENT_SOURCE_DIR}/sip")
    set(SIP_DISABLE_FEATURES WITH_CONFIG)
    set(SIP_CHECK_EXC ON)
    ADD_SIP_PYTHON_MODULE(limaprosilica ${CMAKE_CURRENT_BINARY_DIR}/sip/limaprosilica.sip)
    TARGET_INCLUDE_DIRECTORIES(python_module_limaprosilica PRIVATE
                               ${PYTHON_INCLUDE_DIRS}
                               "${CMAKE_SOURCE_DIR}/sip"
                               "${CMAKE_SOURCE_DIR}/sip/core")
    TARGET_LINK_LIBRARIES(python_module_limaprosilica limaprosilica)

    INSTALL(FILES python/__init__.py DESTINATION "${PYTHON_SITE_PACKAGES_DIR}/Lima/Prosilica")
ENDIF()
