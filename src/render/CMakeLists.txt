include_directories (
    ${ARMA_SOURCE_DIR}/src/
    ${ARMA_SOURCE_DIR}/src/tools/
    ${ARMA_SOURCE_DIR}/src/resource/
    ${ARMA_SOURCE_DIR}/src/thirdparty/utf8/
    ${ARMA_SOURCE_DIR}/src/thirdparty/mathexpr/
    ${ARMA_SOURCE_DIR}/src/render/

    ${ARMA_BINARY_DIR}/src/
    ${ARMA_BINARY_DIR}/src/protobuf/

    ${ARMA_INCLUDE_DIRS}
    )

add_library (render
    rConsole.h                  rConsole.cpp
                                rConsoleGraph.cpp
    rFont.h                     rFont.cpp
    rGL.h                       rGL.cpp
    rGLEW.h
                                rGLRender.cpp
    rModel.h                    rModel.cpp
    rRender.h                   rRender.cpp
    rDisplayList.h              rDisplayList.cpp
    rScreen.h                   rScreen.cpp
    rSDL.h
    rSysdep.h                   rSysdep.cpp
    rTexture.h                  rTexture.cpp
    rViewport.h                 rViewport.cpp
    rColor.h
    rGradient.h                 rGradient.cpp
    rTextureRenderTarget.h      rTextureRenderTarget.cpp
    rGLuintObject.h             rGLuintObject.cpp
    )

add_dependencies (render
    protos
    )
