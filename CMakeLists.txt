cmake_minimum_required (VERSION 2.6)

project (armagetronad)
set (progtitle "Armagetron Advanced")

list(APPEND CMAKE_MODULE_PATH ${armagetronad_SOURCE_DIR}/batch/make/cmake/)

option(DEDICATED "Build dedicated server (otherwise only a client)" OFF)
if (DEDICATED)
    list(APPEND armagetronad_DEFINES -DDEDICATED)
    message(STATUS "Building Dedicated")
endif()

option(RELEASE "Build a release with this version number" "")
if(NOT RELEASE)
    set(RELEASE "")
endif()

option(PREFIX "Prefix where to install armagetronad" "")
if(NOT PREFIX)
    set(PREFIX "usr/local/")
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
endif (FTGL_FOUND)

if (PROTOBUF_FOUND)
    set(HAVE_GOOGLE_PROTOBUF_MESSAGE_H 1)
endif (PROTOBUF_FOUND)

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

set (armagetronad_INCLUDE_DIRS
    ${armagetronad_BINARY_DIR}/src/
    ${armagetronad_SOURCE_DIR}/src/

    ${armagetronad_SOURCE_DIR}/src/thirdparty/utf8/
    ${armagetronad_SOURCE_DIR}/src/thirdparty/mathexpr/
    ${armagetronad_SOURCE_DIR}/src/thirdparty/scrap/

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

set (armagetronad_DEFINES
    -DENABLE_ZONESV2 -DENABLE_ZONESV1 # Hacks :(
    ${PNG_DEFINITIONS}
    )

add_definitions(${armagetronad_DEFINES})

##### PATHS #####
UNIVERSAL_VAR (PREFIX ${PREFIX})
UNIVERSAL_VAR (BINDIR "${PREFIX}bin")
UNIVERSAL_VAR (SBINDIR "${PREFIX}sbin")
UNIVERSAL_VAR (DATADIR "${PREFIX}share")
UNIVERSAL_VAR (DATADIR_SUFFIX "/games/armagetronad")
UNIVERSAL_VAR (AA_DATADIR ${DATADIR}${DATADIR_SUFFIX})
UNIVERSAL_VAR (SYSCONFDIR "etc")
UNIVERSAL_VAR (SYSCONFDIR_SUFFIX "/games/armagetronad")
UNIVERSAL_VAR (AA_SYSCONFDIR ${SYSCONFDIR}${SYSCONFDIR_SUFFIX})
UNIVERSAL_VAR (LOCALSTATEDIR "var")
UNIVERSAL_VAR (LOCALSTATEDIR_SUFFIX "/games/armagetronad")
UNIVERSAL_VAR (AA_LOCALSTATEDIR "${LOCALSTATEDIR}${LOCALSTATEDIR_SUFFIX}")
UNIVERSAL_VAR (INFODIR "")
UNIVERSAL_VAR (INFODIR_SUFFIX "/games/armagetronad")
UNIVERSAL_VAR (AA_INFODIR "${INFODIR}${INFODIR_SUFFIX}")
UNIVERSAL_VAR (MANDIR "")
UNIVERSAL_VAR (MANDIR_SUFFIX "/games/armagetronad")
UNIVERSAL_VAR (AA_MANDIR "${MANDIR}${MANDIR_SUFFIX}")
UNIVERSAL_VAR (SCRIPTDIR "${AA_DATADIR}/scripts/")
UNIVERSAL_VAR (WWWROOTDIR "${AA_DATADIR}/www-root/")
UNIVERSAL_VAR (DOCDIR "${DATADIR}/doc")
UNIVERSAL_VAR (DOCDIR_SUFFIX "/games/armagetronad")
UNIVERSAL_VAR (AA_DOCDIR "${DOCDIR}${DOCDIR_SUFFIX}")
UNIVERSAL_VAR (RUNDIR "${LOCALSTATEDIR}/run")
UNIVERSAL_VAR (RUNDIR_SUFFIX "")
UNIVERSAL_VAR (AA_RUNDIR "${RUNDIR}${RUNDIR_SUFFIX}")
UNIVERSAL_VAR (LOGDIR "${LOCALSTATEDIR}/log")
UNIVERSAL_VAR (LOGDIR_SUFFIX "")
UNIVERSAL_VAR (AA_LOGDIR "${LOGDIR}${LOGDIR_SUFFIX}")
UNIVERSAL_VAR (OLDVARDIR "${LOCALSTATEDIR}/games/armagetronad/var")
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

set (armagetronad_LINK_LIBRARIES
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
    ${armagetronad_SOURCE_DIR}/src/tools/
    ${armagetronad_SOURCE_DIR}/src/resource/
    ${armagetronad_SOURCE_DIR}/src/network/
    ${armagetronad_SOURCE_DIR}/src/render/
    ${armagetronad_SOURCE_DIR}/src/ui/
    ${armagetronad_SOURCE_DIR}/src/engine/
    ${armagetronad_SOURCE_DIR}/src/engine/sound/
    ${armagetronad_SOURCE_DIR}/src/tron/
    ${armagetronad_SOURCE_DIR}/src/tron/cockpit/

    ${armagetronad_BINARY_DIR}/src/protobuf/

    ${armagetronad_INCLUDE_DIRS}
    )


add_custom_command (TARGET sortresources
    COMMAND ${PYTHON_EXECUTABLE} ${armagetronad_SOURCE_DIR}/batch/make/copyresources.py ${armagetronad_SOURCE_DIR}/resource/proto/ ${armagetronad_BINARY_DIR}/resource/included/ ${armagetronad_SOURCE_DIR}/batch/make/sortresources.py
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${armagetronad_SOURCE_DIR}/resource/binary/ ${armagetronad_SOURCE_DIR}/resource/included/
    )

add_custom_target (sortresources ALL
    COMMENT "Sorting resources"
    )

add_custom_target (generateversion ALL 
    ${PYTHON_EXECUTABLE} ${armagetronad_SOURCE_DIR}/batch/make/version.py ${armagetronad_SOURCE_DIR} --release=${RELEASE} -o ${armagetronad_BINARY_DIR}/src/tTrueVersion.h
    COMMENT "Generating version tag"
    COMMAND ${PYTHON_EXECUTABLE} ${armagetronad_SOURCE_DIR}/batch/make/version.py ${armagetronad_SOURCE_DIR} --release=${RELEASE} -o ${armagetronad_BINARY_DIR}/src/tTrueVersion.h
    DEPENDS ${armagetronad_SOURCE_DIR}/batch/make/version.py
    WORKING_DIRECTORY ${armagetronad_SOURCE_DIR}
    )

#add_custom_target (generateversion ALL
#    DEPENDS ${armagetronad_BINARY_DIR}/src/tTrueVersion.h
#    )

set(EXTRA_SOURCES 
    src/tron/gFloor.cpp
	src/tron/cockpit/cCockpit.cpp
    src/tron/cockpit/cMap.cpp
    src/tron/cockpit/cCamview.cpp
    )

if(BUILDCLIENT AND NOT DEDICATED)
    add_executable (armagetronad WIN32 src/tools/tVersion.cpp ${EXTRA_SOURCES} src/tron/gClient.cpp)
    target_link_libraries (armagetronad tron engine ui render network tools protos mathexpr particles scrap ${armagetronad_LINK_LIBRARIES})
    add_dependencies (armagetronad 
        generateversion
        )
    install (TARGETS armagetronad
        RUNTIME DESTINATION ${BINDIR})
endif()

if(DEDICATED)
add_executable (armagetronad-dedicated src/tools/tVersion.cpp ${EXTRA_SOURCES} src/tron/gDedicated.cpp)
target_link_libraries (armagetronad-dedicated tron engine ui render network tools protos mathexpr particles scrap ${armagetronad_LINK_LIBRARIES})
add_dependencies (armagetronad-dedicated 
    generateversion
    )
endif()


