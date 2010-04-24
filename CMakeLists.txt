cmake_minimum_required (VERSION 2.6)

set(progname "armagetronad")
set(release "")
set (progtitle "Armagetron Advanced")

project (${progname})

##### MODULES #####

list(APPEND CMAKE_MODULE_PATH ${${progname}_SOURCE_DIR}/cmake/modules/)

include (UniversalVariables)
include (CheckIncludeFiles)
include (CheckFunctionExists)
include (CheckLibraryExists)
include (CheckTypeSize)
include (Deps)
include (DLLs)
include (RcMingwHack)

##### OPTIONS #####

option(DEDICATED "Build dedicated server (otherwise only a client)" OFF)
if (DEDICATED)
    list(APPEND ${progname}_DEFINES -DDEDICATED)
    message(STATUS "Building Dedicated")
endif()

set(RELEASE ${release}
    CACHE STRING "Build a release with this version number")

if(NOT RELEASE)
    set(RELEASE ${release})
endif()

set(DEBUGLEVEL 0 CACHE STRING "Debug level for developers, normal users can safely ignore it. Accepted values go from 0 to 5. See README-DEVELOPER for details.")

option(SYSINSTALL "Enable system-wide install" NO)

option(ENABLE_ZONESV1 "Enable deprecated zonesv1 code" NO)
option(ENABLE_ZONESV2 "Enable experimental zonesv2 code" YES)
option(FORTRESS "Enable fortress support" ${ENABLE_ZONESV2})
option(USEPARTICLES "Compile in support for experimental particle system" NO)
option(NOJOYSTICK "Disable joystick support" NO)
option(DESKTOP "Install menu entries" YES)
option(DONTUSEMEMMANAGER "Do not use the custom memory manager on systems that allow it." YES)

if(ENABLE_ZONESV2 AND FORTRESS)
    set(BUILD_FORTRESS 1)
endif()

##### CONFIGURE CHECKS #####

CHECK_FUNCTION_EXISTS (isblank HAVE_ISBLANK)
CHECK_FUNCTION_EXISTS (select HAVE_SELECT)
CHECK_FUNCTION_EXISTS (wmemset HAVE_WMEMSET)

CHECK_LIBRARY_EXISTS (m atan2f "" HAVE_ATAN2F)
CHECK_LIBRARY_EXISTS (m cosf "" HAVE_COSF)
CHECK_LIBRARY_EXISTS (m expf "" HAVE_EXPF)
CHECK_LIBRARY_EXISTS (m fabsf "" HAVE_FABSF)
CHECK_LIBRARY_EXISTS (m floorf "" HAVE_FLOORF)
CHECK_LIBRARY_EXISTS (m logf "" HAVE_LOGF)
CHECK_LIBRARY_EXISTS (m pow10 "" HAVE_POW10)
CHECK_LIBRARY_EXISTS (m pow10f "" HAVE_POW10F)
CHECK_LIBRARY_EXISTS (m sinf "" HAVE_SINF)
CHECK_LIBRARY_EXISTS (m sqrtf "" HAVE_SQRTF)
CHECK_LIBRARY_EXISTS (m tanf "" HAVE_TANF)

CHECK_INCLUDE_FILES (inttypes.h HAVE_INTTYPES_H)
CHECK_INCLUDE_FILES (stddef.h HAVE_STDDEF_H)
CHECK_INCLUDE_FILES (unistd.h HAVE_UNISTD_H)
CHECK_INCLUDE_FILES (stdint.h HAVE_STDINT_H)
CHECK_INCLUDE_FILES (stdlib.h HAVE_STDLIB_H)
CHECK_INCLUDE_FILES (strings.h HAVE_STRINGS_H)
CHECK_INCLUDE_FILES (string.h HAVE_STRING_H)
CHECK_INCLUDE_FILES (sys/stat.h HAVE_SYS_STAT_H)
CHECK_INCLUDE_FILES (sys/types.h HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILES (unistd.h HAVE_UNISTD_H)

SET(CMAKE_EXTRA_INCLUDE_FILES arpa/inet.h)
CHECK_TYPE_SIZE(socklen_t HAVE_SOCKLEN_T)
SET(CMAKE_EXTRA_INCLUDE_FILES)

SET (SDL_BUILDING_LIBRARY TRUE)

find_package (LibXml2 REQUIRED)
find_package (Boost 1.34.0 REQUIRED )
find_package (Bison REQUIRED)
find_package (PythonInterp REQUIRED)
find_package (Protobuf REQUIRED)

find_package (SDL)
find_package (SDL_mixer)
find_package (SDL_image)
find_package (FTGL)
find_package (Freetype)
find_package (OpenGL)
find_package (PNG)

if(WIN32)
    find_library (WS2_LIBRARIES ws2_32)
    if(NOT WS2_LIBRARIES)
        message(ERROR "You need winsock2 in order to build ${progtitle} for windows.")
    endif()
else()
    set(WS2_LIBRARIES "")
endif()

DEP(Boost Boost)
DEP(BISON Bison)
DEP(LIBXML2 LibXml2)
DEP(PROTOBUF "Google Protocol Buffers")
DEP(PYTHONINTERP "a Python interpreter")

CLIENTDEP(OPENGL "OpenGL")
CLIENTDEP(FTGL "FTGL")
CLIENTDEP(PNG "libpng")
CLIENTDEP(SDL "Simple DirectMedia Layer(SDL)")
CLIENTDEP(SDLMIXER "SDL_mixer")
CLIENTDEP(SDLIMAGE "SDL_image")

CLEANCLIENTDEPS()

if (FTGL_FOUND)
    set(HAVE_FTGL_H 1)
endif ()

if (PROTOBUF_FOUND)
    set(HAVE_GOOGLE_PROTOBUF_MESSAGE_H 1)
endif ()

if (SDLMIXER_FOUND)
    set(HAVE_LIBSDL_MIXER 1)
endif ()

include_directories (BEFORE 
    ${${progname}_SOURCE_DIR}/src/
    )

include_directories (
    ${${progname}_SOURCE_DIR}/src/thirdparty/utf8/
    ${${progname}_SOURCE_DIR}/src/thirdparty/mathexpr/
    ${${progname}_SOURCE_DIR}/src/thirdparty/scrap/
    )

include_directories ( SYSTEM
    ${LIBXML2_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${PROTOBUF_INCLUDE_DIR}
    )


if(NOT DEDICATED)
    include_directories( SYSTEM
        ${FTGL_INCLUDE_DIR}/FTGL/
        ${FREETYPE_INCLUDE_DIR_ft2build}
        ${FREETYPE_INCLUDE_DIR_freetype2}
        ${SDL_INCLUDE_DIR}
        ${SDLMIXER_INCLUDE_DIR}
        ${SDLIMAGE_INCLUDE_DIR}
        ${PNG_INCLUDE_DIR}
        )

    LIST (APPEND ${progname}_DEFINES
        ${PNG_DEFINITIONS}
    )
endif()

if(NOT DEDICATED)
    link_libraries(
        ${SDL_LIBRARY}
        ${SDLMIXER_LIBRARY}
        ${SDLIMAGE_LIBRARY}
        SDLmain
        ${FTGL_LIBRARIES}
        ${OPENGL_LIBRARIES}
        ${FREETYPE_LIBRARIES}
        ${PNG_LIBRARIES}
        )
    if(WIN32)
        GetDLLs(dlls
            ${SDL_LIBRARY}
            ${SDLMIXER_LIBRARY}
            ${SDLIMAGE_LIBRARY}
            ${FTGL_LIBRARY}
            ${FREETYPE_LIBRARY}
            ${PNG_LIBRARY}
        )
        install(FILES
            ${dlls}
            DESTINATION ${BINDIR}
            COMPONENT client)
    endif()
endif()

link_libraries(
    ${PROTOBUF_LIBRARY}
    ${LIBXML2_LIBRARIES}
    ${WS2_LIBRARIES}
    )
if(WIN32)
    GetDLLs(dlls
    ${PROTOBUF_LIBRARY}
    ${LIBXML2_LIBRARIES}
    )
    install(FILES
        ${dlls}
        DESTINATION ${BINDIR}
    )
endif()



include_directories (
    ${${progname}_SOURCE_DIR}/src/tools/
    ${${progname}_SOURCE_DIR}/src/resource/
    ${${progname}_SOURCE_DIR}/src/network/
    ${${progname}_SOURCE_DIR}/src/render/
    ${${progname}_SOURCE_DIR}/src/ui/
    ${${progname}_SOURCE_DIR}/src/engine/
    ${${progname}_SOURCE_DIR}/src/engine/sound/
    ${${progname}_SOURCE_DIR}/src/tron/
    ${${progname}_SOURCE_DIR}/src/tron/cockpit/

    ${${progname}_BINARY_DIR}/src/protobuf/
    )
include_directories ( BEFORE
    ${${progname}_BINARY_DIR}/src/
    )

##### DEBUGGING #####
if(DEBUGLEVEL GREATER 0)
endif()
if(DEBUGLEVEL GREATER 1)
    list(APPEND ${progname}_DEFINES -DDEBUG)
endif()
if(DEBUGLEVEL GREATER 2)
    set(DONTUSEMEMMANAGER 0)
endif()
if(DEBUGLEVEL GREATER 3)
    list(APPEND ${progname}_DEFINES -DDEBUG_EXPENSIVE)
endif()
if(DEBUGLEVEL GREATER 4)
    list(APPEND ${progname}_DEFINES -DMEM_DEB)
endif()


add_definitions(${${progname}_DEFINES})

if(UNIX AND NOT APPLE)
##### LINUX PATHS #####
    UNIVERSAL_VAR (PREFIX ${CMAKE_INSTALL_PREFIX})
    if(SYSINSTALL)
        set(PREFIX "/usr/")
    endif()
    UNIVERSAL_VAR (BINDIR "${PREFIX}bin/")
    UNIVERSAL_VAR (SBINDIR "${PREFIX}sbin/")
    UNIVERSAL_VAR (DATADIR "${PREFIX}share/")
    UNIVERSAL_VAR (GAMES_SUFFIX "")
    UNIVERSAL_VAR (DATADIR_SUFFIX "${progname}")
    UNIVERSAL_VAR (AA_DATADIR "${DATADIR}${DATADIR_SUFFIX}")
    if(SYSINSTALL)
        set(AA_SYSCONFDIR "/etc${GAMES_SUFFIX}/${progname}")
    endif()
    UNIVERSAL_VAR (LOCALSTATEDIR "${PREFIX}var/")
    UNIVERSAL_VAR (LOCALSTATEDIR_SUFFIX "${GAMES_SUFFIX}/${progname}")
    UNIVERSAL_VAR (AA_LOCALSTATEDIR "${LOCALSTATEDIR}${LOCALSTATEDIR_SUFFIX}")
    UNIVERSAL_VAR (INFODIR "")
    UNIVERSAL_VAR (INFODIR_SUFFIX "${GAMES_SUFFIX}/${progname}")
    UNIVERSAL_VAR (AA_INFODIR "${INFODIR}${INFODIR_SUFFIX}")
    UNIVERSAL_VAR (MANDIR "")
    UNIVERSAL_VAR (MANDIR_SUFFIX "${GAMES_SUFFIX}/${progname}")
    UNIVERSAL_VAR (AA_MANDIR "${MANDIR}${MANDIR_SUFFIX}")
    UNIVERSAL_VAR (SCRIPTDIR "${AA_DATADIR}/scripts/")
    UNIVERSAL_VAR (WWWROOTDIR "${AA_DATADIR}/www-root/")
    UNIVERSAL_VAR (DOCDIR "${DATADIR}doc")
    UNIVERSAL_VAR (DOCDIR_SUFFIX "${GAMES_SUFFIX}/${progname}")
    UNIVERSAL_VAR (AA_DOCDIR "${DOCDIR}${DOCDIR_SUFFIX}")
    UNIVERSAL_VAR (RUNDIR "${LOCALSTATEDIR}/run")
    UNIVERSAL_VAR (RUNDIR_SUFFIX "")
    UNIVERSAL_VAR (AA_RUNDIR "${RUNDIR}${RUNDIR_SUFFIX}")
    UNIVERSAL_VAR (LOGDIR "${LOCALSTATEDIR}/log")
    UNIVERSAL_VAR (LOGDIR_SUFFIX "")
    UNIVERSAL_VAR (AA_LOGDIR "${LOGDIR}${LOGDIR_SUFFIX}")
    UNIVERSAL_VAR (OLDVARDIR "${LOCALSTATEDIR}${GAMES_SUFFIX}/${progname}/var")
    UNIVERSAL_VAR (OLDVARDIR_SUFFIX "")
    UNIVERSAL_VAR (AA_OLDVARDIR "${OLDVARDIR}${OLDVARDIR_SUFFIX}")
elseif(WIN32)
##### WINDOWS PATHS #####
    UNIVERSAL_VAR (PREFIX "${CMAKE_INSTALL_PREFIX}")
    UNIVERSAL_VAR (BINDIR ".")
    UNIVERSAL_VAR (AA_DATADIR ".")
    UNIVERSAL_VAR (USER_DATA_DIR "\\\${APPDATA}/${progtitle}")
    UNIVERSAL_VAR (SCREENSHOT_DIR "\\\${MYPICTURES}/${progtitle}")
elseif(APPLE)
    message(FATAL_ERROR "OSX Paths not supported yet")
else()
    message(FATAL_ERROR "Unknown system")
endif()

##### UNIVERSAL PATHS #####
UNIVERSAL_VAR (AA_SYSCONFDIR "${AA_DATADIR}/config")

##### CPACK #####
if(EXISTS ${${progname}_BINARY_DIR}/src/tTrueVersion.h)
    file(STRINGS ${${progname}_BINARY_DIR}/src/tTrueVersion.h trueversionfile)
    foreach(line ${trueversionfile})
        if(${line} MATCHES "^#define TRUE_ARMAGETRONAD_VERSION \"([0-9a-zA-Z_.-]+)\"$")
            set(CPACK_PACKAGE_VERSION ${CMAKE_MATCH_1})
        endif()
    endforeach()
else()
    message("If you intend on building packages with CPack, please call cmake again after building.")
    unset(CPACK_PACKAGE_VERSION)
endif()

set(CPACK_PACKAGE_NAME "${progtitle}")
if(DEDICATED)
    set(CPACK_PACKAGE_NAME "${progtitle} Dedicated Server")
endif()
set(CPACK_PACKAGE_VENDOR "Armagetron Advanced Team")

set(CPACK_RESOURCE_FILE_LICENSE "${${progname}_SOURCE_DIR}/COPYING.txt")
#set(CPACK_TOPLEVEL_DIRECTORY "${${progname}_BINARY_DIR}/CPackFiles/")
#SET(CPACK_SET_DESTDIR "ON")

set(CPACK_PACKAGE_FILE_NAME "${progname}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}")
#set(CPACK_PACKAGE_EXECUTABLES armagetronad;"Armagetron Advanced")

set(CPACK_PACKAGE_ICON "${${progname}_SOURCE_DIR}/win32/banner.bmp")

if(DEFINED CPACK_PACKAGE_VERSION) # Don't allow making CPack packages if version can't be known
    include (CPack)
endif()

#########
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/aa_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/src/aa_config.h)

message (STATUS "Generating src/tUniversalVariables.h")
WRITE_UNIVARS()

if( NOT BUILDCLIENT AND NOT DEDICATED AND BUILD )
    message ("No client will be built!")
endif()


add_subdirectory (src)
add_subdirectory (config)
add_subdirectory (desktop)
add_subdirectory (language)
add_subdirectory (models)
add_subdirectory (music)
add_subdirectory (resource)
add_subdirectory (sound)
add_subdirectory (textures)

#add_custom_command (TARGET sortresources
#    COMMAND ${PYTHON_EXECUTABLE} ${${progname}_SOURCE_DIR}/batch/make/copyresources.py ${${progname}_SOURCE_DIR}/resource/proto/ ${${progname}_BINARY_DIR}/resource/included/ ${${progname}_SOURCE_DIR}/batch/make/sortresources.py
#    COMMAND ${CMAKE_COMMAND} -E copy_directory ${${progname}_SOURCE_DIR}/resource/binary/ ${${progname}_SOURCE_DIR}/resource/included/
#    )

add_custom_target (sortresources ALL
    ${PYTHON_EXECUTABLE} ${${progname}_SOURCE_DIR}/cmake/scripts/copyresources.py ${${progname}_SOURCE_DIR}/resource/proto/ ${${progname}_BINARY_DIR}/resource/included/ ${${progname}_SOURCE_DIR}/cmake/scripts/sortresources.py
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${${progname}_SOURCE_DIR}/resource/binary/ ${${progname}_BINARY_DIR}/resource/included/
    COMMENT "Sorting resources"
    )

add_custom_target (generateversion ALL 
    ${PYTHON_EXECUTABLE} ${${progname}_SOURCE_DIR}/cmake/scripts/version.py ${${progname}_SOURCE_DIR} --release=${RELEASE} -o ${${progname}_BINARY_DIR}/src/tTrueVersion.h
    COMMENT "Generating version tag"
    COMMAND ${PYTHON_EXECUTABLE} ${${progname}_SOURCE_DIR}/cmake/scripts/version.py ${${progname}_SOURCE_DIR} --release=${RELEASE} -o ${${progname}_BINARY_DIR}/src/tTrueVersion.h
    WORKING_DIRECTORY ${${progname}_SOURCE_DIR}
    )

#add_custom_target (generateversion ALL
#    DEPENDS ${${progname}_BINARY_DIR}/src/tTrueVersion.h
#    )

set(EXTRA_SOURCES 
    src/tron/gFloor.cpp
	src/tron/cockpit/cCockpit.cpp
    src/tron/cockpit/cMap.cpp
    src/tron/cockpit/cCamview.cpp
    )
if(WIN32 AND NOT DEDICATED)
    set(RESOURCEFILE
        src/win32/armagetron.rc
    )
    message("armagetron.rc passed")
endif()

if(BUILDCLIENT AND NOT DEDICATED)
    add_executable (${progname} WIN32 ${RESOURCEFILE} src/tools/tVersion.cpp ${EXTRA_SOURCES} src/tron/gClient.cpp)
    target_link_libraries (${progname} tron engine ui render network tools protos mathexpr particles scrap )
    add_dependencies (${progname} 
        generateversion
        )
    install (TARGETS ${progname}
        RUNTIME DESTINATION ${BINDIR}
        COMPONENT client
        )
endif()

if(DEDICATED)
    add_executable (${progname}-dedicated src/tools/tVersion.cpp ${EXTRA_SOURCES} src/tron/gDedicated.cpp)
    target_link_libraries (${progname}-dedicated tron engine ui render network tools protos mathexpr)
    add_dependencies (${progname}-dedicated 
        generateversion
        )
    install (TARGETS ${progname}-dedicated
        RUNTIME DESTINATION ${BINDIR}
        COMPONENT dedicated
        )
endif()

add_executable(${progname}-master src/tools/tVersion.cpp src/network/master.cpp)
target_link_libraries (${progname}-master network tools protos)
add_dependencies (${progname}-master 
    generateversion
    )
install (TARGETS ${progname}-master
    RUNTIME DESTINATION ${BINDIR}
    COMPONENT master
    )

