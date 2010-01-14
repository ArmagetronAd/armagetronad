include_directories (
    ${armagetronad_SOURCE_DIR}/src/
    ${armagetronad_SOURCE_DIR}/src/tools/
    ${armagetronad_SOURCE_DIR}/src/resource/
    ${armagetronad_SOURCE_DIR}/src/thirdparty/utf8/
    ${armagetronad_SOURCE_DIR}/src/thirdparty/mathexpr/
    ${armagetronad_SOURCE_DIR}/src/render/

    ${armagetronad_BINARY_DIR}/src/
    ${armagetronad_BINARY_DIR}/src/protobuf/

    ${armagetronad_INCLUDE_DIRS}
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
