cmake_minimum_required (VERSION 2.4)

project (ARMA)

##### CONFIGURE CHECKS #####
include (CheckIncludeFiles)
include (CheckFunctionExists)
include (CheckLibraryExists)
include (CheckTypeSize)
include (protobuf.cmake)

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

CHECK_INCLUDE_FILES (google/protobuf/message.h HAVE_GOOGLE_PROTOBUF_MESSAGE_H)
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
include (FindFTGL.cmake)
include (FindFreetype)
include (FindBoost)
include (FindOpenGL)
include (bison.cmake)
include (FindPNG)

find_package( Boost 1.36.0 )

set (ARMA_INCLUDE_DIRS
    ${ARMA_BINARY_DIR}/src/
    ${ARMA_SOURCE_DIR}/src/

    ${ARMA_SOURCE_DIR}/src/thirdparty/utf8/
    ${ARMA_SOURCE_DIR}/src/thirdparty/mathexpr/
    ${ARMA_SOURCE_DIR}/src/thirdparty/scrap/

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
message( "${ARMA_INCLUDE_DIRS}" )

set (ARMA_DEFINES
    -DENABLE_ZONESV2 -DENABLE_ZONESV1 # Hacks :(
    ${PNG_DEFINITIONS}
    )
add_definitions(${ARMA_DEFINES})

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/src/aa_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/src/aa_config.h)

add_subdirectory (src)

set (ARMA_LINK_LIBRARIES
    ${SDL_LIBRARY}
    ${SDLMIXER_LIBRARY}
    ${SDLIMAGE_LIBRARY}
    ${OPENGL_LIBRARIES}
    ${PROTOBUF_LIBRARY}
    ${FTGL_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    ${PNG_LIBRARIES}
    )
message( "${ARMA_LINK_LIBRARIES}" )

include_directories (
    ${ARMA_SOURCE_DIR}/src/tools/
    ${ARMA_SOURCE_DIR}/src/resource/
    ${ARMA_SOURCE_DIR}/src/network/
    ${ARMA_SOURCE_DIR}/src/render/
    ${ARMA_SOURCE_DIR}/src/ui/
    ${ARMA_SOURCE_DIR}/src/engine/
    ${ARMA_SOURCE_DIR}/src/engine/sound/
    ${ARMA_SOURCE_DIR}/src/tron/
    ${ARMA_SOURCE_DIR}/src/tron/cockpit/

    ${ARMA_BINARY_DIR}/src/protobuf/

    ${ARMA_INCLUDE_DIRS}
    )

add_executable (armagetronad src/tron/gArmagetron.cpp)
target_link_libraries (armagetronad tron engine ui render network tools protos mathexpr particles scrap ${ARMA_LINK_LIBRARIES})


