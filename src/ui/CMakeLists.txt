include_directories (
    ${${progname}_SOURCE_DIR}/src/tools/
    ${${progname}_SOURCE_DIR}/src/resource/
    ${${progname}_SOURCE_DIR}/src/render/

    ${${progname}_BINARY_DIR}/src/protobuf/

    ${${progname}_INCLUDE_DIRS}
    )

add_library(ui EXCLUDE_FROM_ALL
    uInput.h                    uInput.cpp
    uInputQueue.h               uInputQueue.cpp
    uMenu.h                     uMenu.cpp
    )
