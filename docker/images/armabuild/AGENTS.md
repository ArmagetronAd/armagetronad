# docker/images/armabuild/ Directory

## Summary
ArmaBuild Docker base image for building Armagetron Advanced.

## Details

The armabuild directory contains the Dockerfile and build scripts for creating the ArmaBuild base image. This image includes all the necessary dependencies and tools for building Armagetron Advanced from source.

## Directory Structure

```
.
├── Dockerfile.proto         # Base Dockerfile template
├── build_libcurl.sh         # libcurl build script
├── build_libsdl.sh          # SDL library build script
├── build_libsdl_image.sh    # SDL_image build script
├── build_libsdl_mixer.sh    # SDL_mixer build script
├── build_libxml2.sh         # libxml2 build script
└── libsdl-1.2.15-const-xdata32.patch  # SDL patch file
```

## Technologies

- **Containerization**: Docker
- **Dependencies**: libcurl, SDL, SDL_image, SDL_mixer, libxml2
- **Scripting**: Bash

## Key Patterns

- Dependency build automation
- Patch application for source modifications
- Multi-library build environment
