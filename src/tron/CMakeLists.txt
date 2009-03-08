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

add_library (tron
    gAIBase.h                   gAIBase.cpp
    gAICharacter.h              gAICharacter.cpp
    gArena.h                    gArena.cpp
                                gArmagetron.cpp
    gCamera.h                   gCamera.cpp
    gCommandLineJumpStart.h     gCommandLineJumpStart.cpp
    gCycle.h                    gCycle.cpp
    gCycleMovement.h            gCycleMovement.cpp
    gExplosion.h                gExplosion.cpp
    gFriends.h                  gFriends.cpp
                                gFloor.cpp
    gGame.h                	    gGame.cpp
    gJoystick.h                 gJoystick.cpp
    gLanguageMenu.h             gLanguageMenu.cpp
    gLogo.h                     gLogo.cpp
    gMenus.h                    gMenus.cpp
    gParser.h                   gParser.cpp
    gParticles.h                gParticles.cpp
    gSensor.h                   gSensor.cpp
    gServerBrowser.h            gServerBrowser.cpp
    gSparks.h                   gSparks.cpp
    gSpawn.h                    gSpawn.cpp
    gStuff.h                    gStuff.cpp
    gRotation.h                 gRotation.cpp
    gServerFavorites.h          gServerFavorites.cpp
    gStatistics.h               gStatistics.cpp
    gStatList.h                 gStatList.cpp
    gTeam.h                     gTeam.cpp
    gVectorExtra.h
    gWall.h                     gWall.cpp
    gWinZone.h                  gWinZone.cpp
    cockpit/cCockpit.h          cockpit/cCockpit.cpp
    cockpit/cWidgetBase.h       cockpit/cWidgetBase.cpp
    cockpit/cMap.h              cockpit/cMap.cpp
    cockpit/cCamview.h          cockpit/cCamview.cpp
    cockpit/cLabel.h            cockpit/cLabel.cpp
    cockpit/cGauges.h           cockpit/cGauges.cpp
    cockpit/cRectangle.h        cockpit/cRectangle.cpp
    zone/zZone.h                zone/zZone.cpp
    zone/zTimedZone.h           zone/zTimedZone.cpp
    zone/zShape.h               zone/zShape.cpp
    zone/zMonitor.h             zone/zMonitor.cpp
    zone/zEffectGroup.h         zone/zEffectGroup.cpp
    zone/zMisc.h
    zone/zValidator.h           zone/zValidator.cpp
    zone/zSelector.h            zone/zSelector.cpp
    zone/zEffector.h            zone/zEffector.cpp
    zone/zZoneInfluence.h       zone/zZoneInfluence.cpp
    )

add_dependencies (tron
    protos)
