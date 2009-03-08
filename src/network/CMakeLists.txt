include_directories (
    ${ARMA_SOURCE_DIR}/src/tools/
    ${ARMA_SOURCE_DIR}/src/resource/
    ${ARMA_SOURCE_DIR}/src/network/

    ${ARMA_BINARY_DIR}/src/protobuf/

    ${ARMA_INCLUDE_DIRS}
    )

add_library (network
    md5.cpp                     md5.h
    nAuthentication.cpp         nAuthentication.h
    nConfig.cpp                 nConfig.h
    nKrawall.cpp                nKrawall.h
    nKrawallPrivate.cpp
    nNetObject.cpp              nNetObject.h
    nNetwork.cpp                nNetwork.h
    nBinary.cpp                 nBinary.h
    nStreamMessage.cpp          nStreamMessage.h
    nProtoBuf.cpp               nProtoBuf.h
                                nProtoBufForward.h
    nObserver.cpp               nObserver.h
    nPriorizing.cpp             nPriorizing.h
    nServerInfo.cpp             nServerInfo.h
                                nSimulatePing.h
    nSocket.cpp                 nSocket.h
    nSpamProtection.cpp         nSpamProtection.h
    )

add_dependencies(network
    protos
    )
