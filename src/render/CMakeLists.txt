include_directories (
    ${${progname}_SOURCE_DIR}/src/
    ${${progname}_SOURCE_DIR}/src/tools/
    ${${progname}_SOURCE_DIR}/src/resource/
    ${${progname}_SOURCE_DIR}/src/thirdparty/utf8/
    ${${progname}_SOURCE_DIR}/src/thirdparty/mathexpr/
    ${${progname}_SOURCE_DIR}/src/render/

    ${${progname}_BINARY_DIR}/src/
    ${${progname}_BINARY_DIR}/src/protobuf/

    ${${progname}_INCLUDE_DIRS}
    )

add_library (render EXCLUDE_FROM_ALL
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
