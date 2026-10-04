# src/serverquery/ Directory

## Summary

Server query functionality for Armagetron Advanced, providing external access to server information via HTTP/JSON interfaces.

## Details

This directory contains the server query system that allows external applications and web services to query Armagetron Advanced game servers for information such as server status, player lists, game settings, and other metadata. The system uses JSON for data serialization and HTTP for transport.

The serverquery system is particularly useful for:
- Game server listing services (master servers)
- Web-based server browsers
- Monitoring tools
- Integration with gaming communities and platforms

## Directory Structure

```
.
├── serverquery.cpp        # Main server query implementation
├── jsoncpp.cpp           # JSON C++ implementation
└── json/                 # JSON library headers
    ├── json-forwards.h   # JSON forward declarations
    └── json.h            # JSON library main header
```

## Technologies

- **Language**: C++ (C++98/03 with C++11 features where available)
- **Data Format**: JSON for structured data serialization
- **Transport**: HTTP protocol for server queries
- **Dependencies**: Network layer (nNetwork.h, nServerInfo.h, nSocket.h)

## Key Components

### serverquery.cpp
- **Namespace**: `sq` (server query)
- **Connection Parsing**: Parse connection strings for server addresses and ports
- **HTTP Server**: Handle incoming HTTP requests for server information
- **JSON Generation**: Create JSON responses with server data
- **Command Line**: Support for command-line server query operations

### JSON Library
- **jsoncpp.cpp**: JSON C++ implementation for parsing and generating JSON data
- **json/** : JSON library headers providing the core JSON functionality

## Key Features

- **Server Information**: Query server name, version, game mode, and settings
- **Player Data**: Retrieve lists of current players, scores, and statistics
- **Game State**: Access current game state, cycle counts, and timing information
- **Custom Queries**: Support for extended server information and custom data
- **HTTP Interface**: Standard HTTP protocol for easy integration with web services

## Integration Points

- **Network Layer**: Uses network infrastructure for socket management and data transfer
- **Server Info**: Leverages `nServerInfo` for accessing server metadata and status
- **Command Line**: Can be used as a standalone tool or integrated into the main server
- **Web Services**: Designed for integration with web-based server listing services

## Build System

- Compiled as part of the main game server or as standalone utility
- JSON library included as part of the serverquery module
- Network dependencies for server communication

## Usage Examples

- **Direct Query**: Connect to a server and retrieve JSON-formatted server information
- **Master Server**: Register servers and provide listing services
- **Monitoring**: Track server uptime, player counts, and game activity
- **Web Integration**: Provide server data for web-based game browsers

## Key Patterns

- **Request/Response**: HTTP request handling with JSON responses
- **Data Serialization**: JSON format for structured, human-readable data
- **Error Handling**: Robust error handling for network failures and invalid requests
- **Extensibility**: Designed for easy addition of new query types and data fields