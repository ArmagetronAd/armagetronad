include_directories (
    ${${progname}_SOURCE_DIR}/src/tools/
    ${${progname}_SOURCE_DIR}/src/resource/
    ${${progname}_SOURCE_DIR}/src/network/

    ${${progname}_BINARY_DIR}/src/protobuf/

    ${${progname}_INCLUDE_DIRS}
    )

add_library (network EXCLUDE_FROM_ALL
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
