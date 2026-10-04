# docker/images/ Directory

## Summary
Docker base images for Armagetron Advanced build environments.

## Details

The images directory contains Dockerfiles and configuration for various base Docker images used in the Armagetron Advanced build and deployment pipeline. Each subdirectory represents a different base image optimized for specific build or runtime requirements.

## Directory Structure

```
.
├── armabuild/          # ArmaBuild base image
├── armalpine/         # Alpine Linux base image
├── armaroot/          # Root filesystem base image
├── steamcmd/          # SteamCMD base image
└── wineblocks/        # Wine build base image
```

## Technologies

- **Containerization**: Docker
- **Base Images**: Various Linux distributions and build environments

## Key Patterns

- Specialized base images for different build requirements
- Multi-stage build optimization
- Platform-specific build environments
