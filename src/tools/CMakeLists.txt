include_directories (
    ${ARMA_SOURCE_DIR}/src/
    ${ARMA_SOURCE_DIR}/src/tools/
    ${ARMA_SOURCE_DIR}/src/resource/
    ${ARMA_SOURCE_DIR}/src/thirdparty/utf8/

    ${ARMA_BINARY_DIR}/src/protobuf/    
    )

add_library (libtools
    tArray.h                    tArray.cpp
    tCallback.h                 tCallback.cpp
    tCallbackString.h
    tColor.h                    tColor.cpp
    tCoord.h                    tCoord.cpp
    tConfiguration.h            tConfiguration.cpp
    tConsole.h                  tConsole.cpp
    tCrypt.h                    tCrypt.cpp
    tDirectories.h              tDirectories.cpp
    tDict.h
    tError.h                    tError.cpp
    tEventQueue.h               tEventQueue.cpp
    tHeap.h                     tHeap.cpp
    tInitExit.h
    tLinkedList.h               tLinkedList.cpp
    tList.h
    tLocale.h                   tLocale.cpp
    tMath.h
    tMemStack.h                 tMemStack.cpp
    tReferenceHolder.h
    tRing.h                     tRing.cpp
    tSafePTR.h                  tSafePTR.cpp
    tString.h                   tString.cpp
    tSysTime.h                  tSysTime.cpp
    tToDo.h                     tToDo.cpp
    tException.h                tException.cpp
    tRecorder.h                 tRecorder.cpp
    tRecorderInternal.h         tRecorderInternal.cpp
    tCommandLine.h              tCommandLine.cpp
    tRandom.h                   tRandom.cpp
    tIniFile.h                  tIniFile.cpp
    tXmlParser.h                tXmlParser.cpp
    ../thirdparty/binreloc/prefix.h ../thirdparty/binreloc/prefix.c
    values/vCore.h              values/vCore.cpp
    values/vParser.h            values/vParser.ypp
    values/vRegistry.h          values/vRegistry.cpp
    values/vCollection.h        values/vCollection.cpp
    values/veComparison.h       values/veComparison.cpp
    values/veLogic.h            values/veLogic.cpp
    values/veMath.h             values/veMath.cpp
    values/vebLegacy.h          values/vebLegacy.cpp
    values/vebMathExpr.h        values/vebMathExpr.cpp
    values/vebCFunction.h
    tStatFile.h                 tStatFile.cpp
    tStatEntry.h                tStatEntry.cpp
    tPlayList.h                 tPlayList.cpp
    tDecorator.h                tDecorator.cpp
    tValue.h
    tValueParser.h
    tVersion.h                  tVersion.cpp
    tFunction.h                 tFunction.cpp
    tPolynomial.h               tPolynomial.cpp
    tPolynomialMarshaler.h      tPolynomialMarshaler.cpp
    tPolynomialWithBase.h       tPolynomialWithBase.cpp
    tRectangle.h                tRectangle.cpp
    ../resource/tResource.h     ../resource/tResource.cpp
    ../resource/tResourceType.h ../resource/tResourceType.cpp
    tRuby.h                     tRuby.cpp
    tMemManager.h               tMemManager.cpp
)

add_dependencies( libtools
    libprotobuf
    )

## Set program to define version
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/build/info/tTrueVersion.h
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/batch/make/version ${CMAKE_CURRENT_SOURCE_DIR} >${CMAKE_CURRENT_BINARY_DIR}/build/info/tTrueVersion.h
    )
