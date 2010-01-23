include_directories (
    ${${progname}_SOURCE_DIR}/src/tools/
    ${${progname}_SOURCE_DIR}/src/resource/
    ${${progname}_SOURCE_DIR}/src/network/
    ${${progname}_SOURCE_DIR}/src/render/
    ${${progname}_SOURCE_DIR}/src/ui/
    ${${progname}_SOURCE_DIR}/src/engine/
    ${${progname}_SOURCE_DIR}/src/engine/sound/

    ${${progname}_BINARY_DIR}/src/protobuf/

    ${${progname}_INCLUDE_DIRS}
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
