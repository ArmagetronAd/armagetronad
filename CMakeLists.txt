cmake_minimum_required (VERSION 2.6)

set(progname "armagetronad")
set(release "")

project (${progname})
set (progtitle "Armagetron Advanced")

list(APPEND CMAKE_MODULE_PATH ${${progname}_SOURCE_DIR}/batch/make/cmake/)

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

option(SYSINSTALL "Enable system-wide install" NO)

option(ENABLE_ZONESV1 "Enable deprecated zonesv1 code" NO)
option(ENABLE_ZONESV2 "Enable experimental zonesv2 code" YES)
option(FORTRESS "Enable fortress support" ${ENABLE_ZONESV2})
option(USEPARTICLES "Compile in support for experimental particle system" NO)
option(NOJOYSTICK "Disable joystick support" NO)

if(ENABLE_ZONESV2 AND FORTRESS)
    set(BUILD_FORTRESS 1)
endif()

##### CONFIGURE CHECKS #####
include (UniversalVariables)
include (CheckIncludeFiles)
include (CheckFunctionExists)
include (CheckLibraryExists)
include (CheckTypeSize)
include (protobuf)
include (Deps)

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

include (FindLibXml2)
include (FindSDL)
include (FindSDL_mixer)
include (FindSDL_image)
include (FindFTGL)
include (FindFreetype)
include (FindBoost)
include (FindOpenGL)
include (bison)
include (FindPNG)
include (FindPythonInterp)

if (FTGL_FOUND)
    set(HAVE_FTGL_H 1)
endif ()

if (PROTOBUF_FOUND)
    set(HAVE_GOOGLE_PROTOBUF_MESSAGE_H 1)
endif ()

if (SDLMIXER_FOUND)
    set(HAVE_LIBSDL_MIXER 1)
endif ()

find_package( Boost 1.36.0 )

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

set (${progname}_INCLUDE_DIRS
    ${${progname}_BINARY_DIR}/src/
    ${${progname}_SOURCE_DIR}/src/

    ${${progname}_SOURCE_DIR}/src/thirdparty/utf8/
    ${${progname}_SOURCE_DIR}/src/thirdparty/mathexpr/
    ${${progname}_SOURCE_DIR}/src/thirdparty/scrap/

    ${LIBXML2_INCLUDE_DIR}
    ${FTGL_INCLUDE_DIR}/FTGL/
    ${FREETYPE_INCLUDE_DIR_ft2build}
    ${FREETYPE_INCLUDE_DIR_freetype2}
    ${SDL_INCLUDE_DIR}
    ${SDLMIXER_INCLUDE_DIR}
    ${SDLIMAGE_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${PROTOBUF_INCLUDE_DIR}
    ${PNG_INCLUDE_DIR}
    )

set (${progname}_DEFINES
    ${${progname}_DEFINES}
    ${PNG_DEFINITIONS}
    )

add_definitions(${${progname}_DEFINES})

##### PATHS #####
UNIVERSAL_VAR (PREFIX "/usr/local/")
if(SYSINSTALL)
    set(PREFIX "/usr/")
endif()
UNIVERSAL_VAR (BINDIR "${PREFIX}bin/")
UNIVERSAL_VAR (SBINDIR "${PREFIX}sbin/")
UNIVERSAL_VAR (DATADIR "${PREFIX}share/")
UNIVERSAL_VAR (GAMES_SUFFIX "")
UNIVERSAL_VAR (DATADIR_SUFFIX "${progname}")
UNIVERSAL_VAR (AA_DATADIR "${DATADIR}${DATADIR_SUFFIX}")
UNIVERSAL_VAR (AA_SYSCONFDIR "${AA_DATADIR}/config")
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

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/aa_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/src/aa_config.h)

message (STATUS "Generating src/tUniversalVariables.h")
WRITE_UNIVARS()

if( NOT BUILDCLIENT AND NOT DEDICATEDONLY AND BUILD )
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

set (${progname}_LINK_LIBRARIES
    ${SDL_LIBRARY}
    ${SDLMIXER_LIBRARY}
    ${SDLIMAGE_LIBRARY}
    ${OPENGL_LIBRARIES}
    ${PROTOBUF_LIBRARY}
    ${FTGL_LIBRARIES}
    ${FREETYPE_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    ${PNG_LIBRARIES}
    )

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

    ${${progname}_INCLUDE_DIRS}
    )


#add_custom_command (TARGET sortresources
#    COMMAND ${PYTHON_EXECUTABLE} ${${progname}_SOURCE_DIR}/batch/make/copyresources.py ${${progname}_SOURCE_DIR}/resource/proto/ ${${progname}_BINARY_DIR}/resource/included/ ${${progname}_SOURCE_DIR}/batch/make/sortresources.py
#    COMMAND ${CMAKE_COMMAND} -E copy_directory ${${progname}_SOURCE_DIR}/resource/binary/ ${${progname}_SOURCE_DIR}/resource/included/
#    )

add_custom_target (sortresources ALL
    ${PYTHON_EXECUTABLE} ${${progname}_SOURCE_DIR}/batch/make/copyresources.py ${${progname}_SOURCE_DIR}/resource/proto/ ${${progname}_BINARY_DIR}/resource/included/ ${${progname}_SOURCE_DIR}/batch/make/sortresources.py
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${${progname}_SOURCE_DIR}/resource/binary/ ${${progname}_SOURCE_DIR}/resource/included/
    COMMENT "Sorting resources"
    )

add_custom_target (generateversion ALL 
    ${PYTHON_EXECUTABLE} ${${progname}_SOURCE_DIR}/batch/make/version.py ${${progname}_SOURCE_DIR} --release=${RELEASE} -o ${${progname}_BINARY_DIR}/src/tTrueVersion.h
    COMMENT "Generating version tag"
    COMMAND ${PYTHON_EXECUTABLE} ${${progname}_SOURCE_DIR}/batch/make/version.py ${${progname}_SOURCE_DIR} --release=${RELEASE} -o ${${progname}_BINARY_DIR}/src/tTrueVersion.h
    DEPENDS ${${progname}_SOURCE_DIR}/batch/make/version.py
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

if(BUILDCLIENT AND NOT DEDICATED)
    add_executable (${progname} WIN32 src/tools/tVersion.cpp ${EXTRA_SOURCES} src/tron/gClient.cpp)
    target_link_libraries (${progname} tron engine ui render network tools protos mathexpr particles scrap ${${progname}_LINK_LIBRARIES})
    add_dependencies (${progname} 
        generateversion
        )
    install (TARGETS ${progname}
        RUNTIME DESTINATION ${BINDIR})
endif()

if(DEDICATED)
add_executable (${progname}-dedicated src/tools/tVersion.cpp ${EXTRA_SOURCES} src/tron/gDedicated.cpp)
target_link_libraries (${progname}-dedicated tron engine ui render network tools protos mathexpr particles scrap ${${progname}_LINK_LIBRARIES})
add_dependencies (${progname}-dedicated 
    generateversion
    )
endif()

add_executable(${progname}-master src/tools/tVersion.cpp src/network/master.cpp)
target_link_libraries (${progname}-master network tools protos scrap ${${progname}_LINK_LIBRARIES})
add_dependencies (${progname}-master 
    generateversion
    )
