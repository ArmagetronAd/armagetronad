# src/protobuf/ Directory

## Summary

Protocol Buffer definitions for Armagetron Advanced's network communication and data serialization.

## Details

This directory contains Protocol Buffer (protobuf) schema files (.proto) that define the data structures used for network communication between clients and servers. These schemas enable efficient, language-neutral, and platform-independent data serialization for game state synchronization, player information, and various network messages.

The protobuf system is used as an alternative or complementary approach to the custom binary protocol used in the network layer. It provides structured data definitions that can be compiled into C++ classes for efficient serialization/deserialization.

## Directory Structure

```
.
├── eNetGameObject.proto      # Network game object serialization
├── nNetwork.proto           # Core network message types
├── tPolynomial.proto        # Polynomial data structures
├── eTimer.proto             # Timer and timing data
├── gCycle.proto             # Cycle/lightcycle data
├── gZone.proto              # Zone/game area data
├── nServerInfo.proto        # Server information and status
├── eEventNotification.proto  # Event notification system
├── tCoord.proto             # Coordinate system data
└── nConfig.proto            # Configuration data structures
```

## Technologies

- **Protocol Buffers**: proto2 syntax
- **Language Generation**: C++ classes from .proto files
- **Dependencies**: Google Protocol Buffers compiler (protoc)

## Key Data Structures

### Network Objects
- **NetGameObject**: Legacy network game object with extension support
- **NetGameObjectSync**: Synchronization data for networked game objects

### Core Network Types
- **Network Messages**: Structured message definitions for client-server communication
- **Server Information**: Server status, settings, and metadata
- **Configuration**: Game configuration data for network synchronization

### Game Objects
- **Cycle Data**: Lightcycle state and properties for network synchronization
- **Zone Data**: Game area and zone definitions
- **Timer Data**: Timing information for synchronized gameplay

### Utility Types
- **Coordinate System**: 2D/3D coordinate data for spatial positioning
- **Polynomial Data**: Mathematical expressions for various game calculations
- **Event Notifications**: System for notifying clients about game events

## Integration

- **Network Layer**: Used by src/network/ for message serialization
- **Engine Layer**: Integrated with src/engine/ for game object network synchronization
- **Build System**: Proto files compiled during build process to generate C++ classes

## Build Configuration

- Protocol Buffer files are processed by the build system
- Generated C++ headers and source files are used for serialization
- Integration with the existing network message system

## Key Patterns

- **Structured Data**: Well-defined message formats for reliable communication
- **Backward Compatibility**: Optional fields for extension without breaking existing code
- **Efficient Serialization**: Compact binary format for network transmission
- **Type Safety**: Strongly typed data structures prevent protocol errors