include_directories (
    ${armagetronad_SOURCE_DIR}/src/tools/
    ${armagetronad_SOURCE_DIR}/src/resource/
    ${armagetronad_SOURCE_DIR}/src/render/

    ${armagetronad_BINARY_DIR}/src/protobuf/

    ${armagetronad_INCLUDE_DIRS}
    )

add_library(ui
    uInput.h                    uInput.cpp
    uInputQueue.h               uInputQueue.cpp
    uMenu.h                     uMenu.cpp
    )
