cmake_minimum_required(VERSION 2.8)

# Set porject and version
project(portaudio)

# Set target name & version
set(TARGET_NAME ${PROJECT_NAME})
set(TARGET_VERSION 19.0.0)

# Portaudio root dir
get_filename_component(PORTAUDIO_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)

#######################################################################################
#                                     Common
#######################################################################################

set(PORTAUDIO_INCLUDE_DIR ${PORTAUDIO_ROOT_DIR}/include)

list(APPEND PA_INC_DIRS ${PORTAUDIO_INCLUDE_DIR})

file(GLOB PA_INC_FILES "${PORTAUDIO_INCLUDE_DIR}/*.h")
source_group("include" FILES ${PA_INC_FILES})


set(PA_SKELETON_SOURCES ${PORTAUDIO_ROOT_DIR}/src/hostapi/skeleton/pa_hostapi_skeleton.c)
source_group("hostapi\\skeleton"  ${PA_SKELETON_SOURCES})

list(APPEND PA_CMAKE_GENERATED
  ${CMAKE_CURRENT_BINARY_DIR}/portaudio_cmake.def
  ${CMAKE_CURRENT_BINARY_DIR}/options_cmake.h
 )
source_group("cmake_generated" FILES ${PA_CMAKE_GENERATED})
add_definitions(-DPORTAUDIO_CMAKE_GENERATED)

aux_source_directory(${PORTAUDIO_ROOT_DIR}/src/common/ PA_SRC_COMMON)
source_group("Common" FILES ${PA_SRC_COMMON})
list(APPEND PA_INC_DIRS ${PORTAUDIO_ROOT_DIR}/src/common)

#######################################################################################
#                                     API
#######################################################################################

if(WIN32)
  option(PORTAUDIO_UNICODE_BUILD  "Enable Portaudio Unicode build"  ON)
  option(PA_USE_ASIO              "Enable support for ASIO"         ON)
  option(PA_USE_DS                "Enable support for DirectSound"  ON)
  option(PA_USE_WMME              "Enable support for MME"          ON)
  option(PA_USE_WASAPI            "Enable support for WASAPI"       ON)
  option(PA_USE_WDMKS             "Enable support for WDMKS"        ON)
  option(PA_USE_WDMKS_DEVICE_INFO "Use WDM/KS API for device info"  ON)

  # ASIO
  if(PA_USE_ASIO)
    find_package(ASIOSDK REQUIRED)

    include_directories(${ASIOSDK_ROOT_DIR}/common)
    include_directories(${ASIOSDK_ROOT_DIR}/host)
    include_directories(${ASIOSDK_ROOT_DIR}/host/pc)

    set(PA_SRC_ASIO   ${PORTAUDIO_ROOT_DIR}/src/hostapi/asio/pa_asio.cpp)
    source_group("hostapi\\ASIO" FILES ${PA_SRC_ASIO})

    set(PA_SRC_ASIO_SDK
      ${ASIOSDK_ROOT_DIR}/common/asio.cpp
      ${ASIOSDK_ROOT_DIR}/host/pc/asiolist.cpp
      ${ASIOSDK_ROOT_DIR}/host/asiodrivers.cpp
    )
    source_group("hostapi\\ASIO\\SDK" FILES ${PA_SRC_ASIO_SDK})
  else()
    set(DEF_EXCLUDE_ASIO_SYMBOLS ";")
  endif()

  # DirectSound
  if(PA_USE_DS)
    find_package(DirectX REQUIRED)
    include_directories(${DIRECTX_INCLUDE_DIRS})
    list(APPEND PA_LIBS  ${DIRECTX_DSOUND_LIBRARY} )

    add_definitions(-DPAWIN_USE_DIRECTSOUNDFULLDUPLEXCREATE)
    aux_source_directory(${PORTAUDIO_ROOT_DIR}/src/hostapi/dsound PA_SRC_DSOUND)
    source_group("hostapi\\dsound" FILES ${PA_SRC_DSOUND} )
  endif()

  # Wmme
  if(PA_USE_WMME)
    set(PA_SRC_WMME ${PORTAUDIO_ROOT_DIR}/src/hostapi/wmme/pa_win_wmme.c)
    source_group("hostapi\\wmme" FILES ${PA_SRC_WMME} )
  endif()

  # Wasapi
  if(PA_USE_WASAPI)
    set(PA_SRC_WASAPI ${PORTAUDIO_ROOT_DIR}/src/hostapi/wasapi/pa_win_wasapi.c)
    source_group("hostapi\\wasapi" FILES ${PA_SRC_WASAPI} )
  else()
    set(DEF_EXCLUDE_WASAPI_SYMBOLS ";")
  endif()

  # Wdmks
  if(PA_USE_WDMKS)
    list(APPEND PA_LIBS setupapi)

    set(PA_SRC_WDMKS  ${PORTAUDIO_ROOT_DIR}/src/hostapi/wdmks/pa_win_wdmks.c)
    source_group("hostapi\\wdmks" files${PA_WDMKS_SOURCES})
  endif()

  if(PA_USE_WDMKS_DEVICE_INFO)
    add_definitions(-DPAWIN_USE_WDMKS_DEVICE_INFO)
  endif()
  mark_as_advanced(PA_USE_WDMKS_DEVICE_INFO)

  list(APPEND PA_SRC_API
    ${PA_SRC_WASAPI}
    ${PA_SRC_WMME}
    ${PA_SRC_WDMKS}
    ${PA_SRC_DSOUND}
    ${PA_SRC_ASIO}
  )
endif()

if(APPLE)
  # on
  add_definitions(-DPA_USE_COREAUDIO)
  aux_source_directory(${PORTAUDIO_ROOT_DIR}/src/hostapi/coreaudio PA_SRC_API)

  find_library(COREAUDIO_LIBRARY    CoreAudio)
  find_library(AUDIOTOOLBOX_LIBRARY AudioToolbox)
  find_library(AUDIOUNIT_LIBRARY    AudioUnit)
  find_library(CORESERVICES_LIBRARY CoreServices)
  find_library(CARBON_LIBRARY       Carbon)

  mark_as_advanced(
    COREAUDIO_LIBRARY
    AUDIOTOOLBOX_LIBRARY
    AUDIOUNIT_LIBRARY
    CORESERVICES_LIBRARY
    CARBON_LIBRARY
  )

  list(APPEND PA_LIBS
    ${COREAUDIO_LIBRARY}
    ${AUDIOTOOLBOX_LIBRARY}
    ${AUDIOUNIT_LIBRARY}
    ${CORESERVICES_LIBRARY}
    ${CARBON_LIBRARY}
  )
endif()

source_group("API" FILES ${PA_SRC_API})


#######################################################################################
#                                     OS
#######################################################################################

if (WIN32)
  list(APPEND PA_INC_DIRS ${PORTAUDIO_ROOT_DIR}/src/os/win)

  list(APPEND PA_SRC_OS
    ${PORTAUDIO_ROOT_DIR}/src/os/win/pa_win_hostapis.c
    ${PORTAUDIO_ROOT_DIR}/src/os/win/pa_win_util.c
    ${PORTAUDIO_ROOT_DIR}/src/os/win/pa_win_waveformat.c
    ${PORTAUDIO_ROOT_DIR}/src/os/win/pa_win_wdmks_utils.c
    ${PORTAUDIO_ROOT_DIR}/src/os/win/pa_win_coinitialize.c
    ${PORTAUDIO_ROOT_DIR}/src/os/win/pa_x86_plain_converters.c
  )
  source_group("os\\win" FILES ${PA_SRC_OS})

  add_definitions(-DPA_WDMKS_NO_KSGUID_LIB)

  if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  endif()

endif()

if (APPLE)
  list(APPEND PA_INC_DIRS ${PORTAUDIO_ROOT_DIR}/src/os/unix)

  list(APPEND PA_SRC_OS
    ${PORTAUDIO_ROOT_DIR}/src/os/unix/pa_unix_hostapis.c
    ${PORTAUDIO_ROOT_DIR}/src/os/unix/pa_unix_util.c
    ${PORTAUDIO_ROOT_DIR}/src/os/unix/pa_unix_util.h
  )
  source_group("os\\mac" FILES ${PA_SRC_OS})

endif()

#######################################################################################
#                                     LIBRARY
#######################################################################################

# Include directories
list(APPEND PA_INC_DIRS ${CMAKE_CURRENT_BINARY_DIR})
include_directories(${PA_INC_DIRS})

# Sources (without asio SDK)
list(APPEND PA_SRC ${PA_SKELETON_SOURCES} ${PA_SRC_COMMON} ${PA_SRC_API} ${PA_SRC_OS})

if(PORTAUDIO_UNICODE_BUILD)
  set_source_files_properties(
    ${PA_SRC}
    PROPERTIES
    COMPILE_DEFINITIONS "UNICODE;_UNICODE"
  )
endif()

# Append more sources
list(APPEND PA_SRC ${PA_INC_FILES} ${PA_CMAKE_GENERATED} ${PA_SRC_ASIO_SDK})

# Create the library
add_library(portaudio ${PA_SRC})
target_link_libraries(portaudio ${PA_LIBS})
target_include_directories(portaudio PUBLIC include)

# Configure the exports file according to settings
set(GENERATED_MESSAGE "CMake generated file, do NOT edit! Use CMake-GUI to change configuration instead.")
configure_file( cmake_support/template_portaudio.def ${CMAKE_CURRENT_BINARY_DIR}/portaudio_cmake.def @ONLY )
# Configure header for options (PA_USE_xxx)
configure_file( cmake_support/options_cmake.h.in ${CMAKE_CURRENT_BINARY_DIR}/options_cmake.h @ONLY )


if (FALSE)
#######################################################################################
#                                     INSTALL
#######################################################################################

# Library install layout:

set(INCLUDE_INSTALL_DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/portaudio)
set(LIB_INSTALL_DESTINATION     ${CMAKE_INSTALL_LIBDIR}/portaudio)

# Interface
target_include_directories(portaudio
  PUBLIC
  "$<INSTALL_INTERFACE:${INCLUDE_INSTALL_DESTINATION}>"
)
install(
  TARGETS portaudio
  DESTINATION ${LIB_INSTALL_DESTINATION} EXPORT portaudioTargets
)
install(
  FILES "${PORTAUDIO_ROOT_DIR}/include/portaudio.h"
  DESTINATION ${INCLUDE_INSTALL_DESTINATION}
)

#################################

endif()
