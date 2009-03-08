include_directories (
    ${ARMA_SOURCE_DIR}/src/tools/
    ${ARMA_SOURCE_DIR}/src/resource/
    ${ARMA_SOURCE_DIR}/src/render/

    ${ARMA_BINARY_DIR}/src/protobuf/

    ${ARMA_INCLUDE_DIRS}
    )

add_library(ui
    uInput.h                    uInput.cpp
    uInputQueue.h               uInputQueue.cpp
    uMenu.h                     uMenu.cpp
    )
