# src/network/ Directory

## Summary
Network communication layer implementing the custom Armagetron multiplayer protocol. Built as `libnetwork.a`.

**Doxygen Group**: `@defgroup Network` - Network Library. Handles server/client connections, defines messages passed between them (defined in README.md)

## Details

The network directory provides the multiplayer networking infrastructure for Armagetron Advanced. It implements a custom protocol for game synchronization, client-server communication, and peer-to-peer features. The system supports both client and server modes, with dedicated server capability.

Key components include: low-level socket handling (`nSocket.cpp/h`) with platform abstraction; message serialization (`nMessage` class in `nNetwork.h`); network state management (`nNetwork.cpp/h`) tracking server/client mode and connection state; network object synchronization (`nNetObject.cpp/h`) providing the base class for all network-synced entities with creation, sync, and control message handling; observer pattern (`nObserver.cpp/h`) for tracking network object state on clients; server information and discovery (`nServerInfo.cpp/h`); authentication system (`nAuthentication.cpp/h`); spam protection (`nSpamProtection.cpp/h`); client management and Krawall protocol (`nKrawall.cpp/h`, `nKrawallPrivate.cpp`); configuration (`nConfig.cpp/h`); ping simulation (`nSimulatePing.h`); and prioritization system (`nPriorizing.cpp/h`) for message ordering. Also includes MD5 hashing (`md5.cpp/h`) for authentication and checksums.

The network layer uses a message descriptor system (`nDescriptor`) for registering message handlers. Network objects maintain synchronization state per-client (`knowsAbout[]` array tracks message acknowledgment status). The system supports client-side prediction and server reconciliation.

## Directory Structure

```
.
├── README.md             # Doxygen @defgroup Network definition
├── md5.cpp                # MD5 hashing implementation
├── md5.h                 # MD5 hashing header
├── nAuthentication.cpp    # Player authentication system
├── nAuthentication.h     # Player authentication system header
├── nConfig.cpp            # Network configuration
├── nConfig.h             # Network configuration header
├── nKrawall.cpp           # Krawall protocol implementation
├── nKrawall.h            # Krawall protocol header
├── nKrawallPrivate.cpp    # Krawall private implementation
├── nNetObject.cpp         # Network object base class
├── nNetObject.h          # Network object base class header
├── nNetwork.cpp           # Network state management
├── nNetwork.h            # Network state management header
├── nObserver.cpp          # Network observer pattern
├── nObserver.h           # Network observer pattern header
├── nPriorizing.cpp        # Message prioritization
├── nPriorizing.h         # Message prioritization header
├── nServerInfo.cpp        # Server information and discovery
├── nServerInfo.h         # Server information and discovery header
├── nSimulatePing.h         # Ping simulation
├── nSocket.cpp            # Low-level socket handling
├── nSocket.h             # Low-level socket handling header
├── nSpamProtection.cpp    # Flood/spam protection
└── nSpamProtection.h     # Flood/spam protection header
```

## Technologies

- **Language**: C++
- **Networking**: Custom TCP/UDP protocol over Berkeley sockets
- **Build System**: Autotools
- **Dependencies**: libtools.a, standard C++ library
- **Crypto**: MD5 hashing (custom implementation)

## Coding Conventions

- **Class Prefix**: `n` for network classes (nNetObject, nSocket, nMessage, etc.)
- **Global Prefix**: `sn_` for static network variables/functions
- **Message Types**: `nDescriptor` system for message type registration
- **Network IDs**: `nNetObjectID` (unsigned short) for object identification
- **Message Serialization**: `WriteSync()`/`ReadSync()` pattern for state synchronization
- **Ownership**: `owner` field tracks which client owns each network object
- **Access Control**: `AcceptClientSync()` determines if clients can create objects
- **Spam Protection**: Rate limiting and flood detection
- **Krawall Protocol**: Custom authentication and session management

## Key Patterns

- Observer pattern for network state tracking
- Factory pattern via CRTP for object registration
- Message descriptor pattern for protocol extensibility
- Rate limiting pattern for spam protection
- Ping simulation for network testing
- Client-side prediction and server reconciliation

## Build System

- Compiled as `libnetwork.a` static library
- Dependencies: `-iquote @srcdir@/network`
- `armagetronad_main_master` uses `network/master.cpp` as entry point
- `nNetObject` uses CRTP pattern via `nNOInitialisator<T>` for type registration
