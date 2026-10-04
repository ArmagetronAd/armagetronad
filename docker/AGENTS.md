# docker/ Directory

## Summary
Docker container configuration for building, testing, and deploying Armagetron Advanced.

## Details

The docker directory contains all Docker-related configuration for Armagetron Advanced. This includes Dockerfiles, build contexts, and deployment scripts for creating containerized builds of the game and its various components.

The directory is organized into subdirectories:
- `build/` - Contains Dockerfiles and build contexts for creating build environments
- `deploy/` - Contains deployment scripts and configurations for building packages and distributions
- `images/` - Docker images used as base for builds or for testing
- `scripts/` - Utility scripts for Docker-based workflows

The build context directory (`build/context/`) contains specialized Docker build contexts for different target platforms and configurations:
- `debian/` - Debian-based builds
- `portable/` - Portable builds
- `steam/` - Steam platform builds
- `tests/` - Test environment builds

Each context may have its own Dockerfile, version.sh, and other configuration files specific to that build target.

## Directory Structure

```
.
├── build/                    # Build configurations
│   ├── Makefile.am         # Docker build Makefile
│   ├── setup_ci_test.sh    # CI test setup script
│   └── context/            # Build contexts
│       ├── debian/         # Debian-based build context
│       │   ├── version.sh   # Version script
│       │   └── [other files]
│       ├── portable/       # Portable build context
│       ├── steam/          # Steam build context
│       │   └── windows/    # Steam Windows build
│       └── tests/          # Test environment build
├── deploy/                  # Deployment configurations
│   └── targets.sh         # Deployment target definitions
└── images/                  # Docker base images
    ├── armabuild/         # ArmaBuild image
    ├── armalpine/         # Alpine Linux image
    ├── armaroot/          # Root image
    ├── steamcmd/          # SteamCMD image
    └── wineblocks/        # Wine build image
```

## Technologies

- **Containerization**: Docker
- **Build System**: Docker multi-stage builds
- **CI/CD**: GitLab CI, Travis CI, or similar

## Coding Conventions

- **Dockerfiles**: Standard Dockerfile syntax
- **Shell Scripts**: POSIX-compliant where possible
- **Multi-stage**: Uses multi-stage builds for smaller final images

## Key Patterns

- Containerized build pattern
- Multi-stage Docker build pattern
- CI/CD pipeline integration
- Build context separation

## Build System

- Docker builds integrated with main Makefile
- `docker/build/Makefile.am` contains Docker-specific targets
- Version scripts generate build metadata
- Build contexts support different architectures and platforms
