include_directories (
    ${armagetronad_SOURCE_DIR}/src/tools/
    ${armagetronad_SOURCE_DIR}/src/resource/
    ${armagetronad_SOURCE_DIR}/src/network/
    ${armagetronad_SOURCE_DIR}/src/render/
    ${armagetronad_SOURCE_DIR}/src/ui/
    ${armagetronad_SOURCE_DIR}/src/engine/
    ${armagetronad_SOURCE_DIR}/src/engine/sound/

    ${armagetronad_BINARY_DIR}/src/protobuf/

    ${armagetronad_INCLUDE_DIRS}
    )

add_library (engine
    eAdvWall.h                  eAdvWall.cpp
    eAuthentication.h           eAuthentication.cpp
    eAxis.h                     eAxis.cpp
    eCamera.h                   eCamera.cpp
    eChat.h                     eChat.cpp
    eCoord.h
    eDebugLine.h                eDebugLine.cpp
                                eDisplay.cpp
    eEventNotification.h        eEventNotification.cpp 
    eFloor.h                    eFloor.cpp
    eGameObject.h               eGameObject.cpp
    eGrid.h                     eGrid.cpp
    eKrawall.h                  eKrawall.cpp
    eLagCompensation.h          eLagCompensation.cpp
    eNetGameObject.h            eNetGameObject.cpp
    ePath.h                     ePath.cpp
    ePlayer.h                   ePlayer.cpp
    eRectangle.h 
    eSensor.h                   eSensor.cpp
    eSoundMixer.h               eSoundMixer.cpp 
    eTeam.h                     eTeam.cpp
    eTess2.h
    eTimer.h                    eTimer.cpp
    eVoter.h                    eVoter.cpp
    eWall.h                     eWall.cpp
    sound/sdl_mixer/eChannelSDLMixer.h 
                                sound/sdl_mixer/eChannelSDLMixer.cpp 
    sound/sdl_mixer/eMusicTrackSDLMixer.h
                                sound/sdl_mixer/eMusicTrackSDLMixer.cpp 
    )

add_dependencies (engine
    protos
    )
