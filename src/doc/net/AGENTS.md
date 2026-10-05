# src/doc/net/ Directory

## Summary
Network protocol documentation in M4 macro template format, describing Armagetron Advanced's three-layer networking architecture.

## Details
This directory contains legacy HTML documentation source files (M4 templates) that describe the network subsystem architecture. The documentation is organized into three conceptual layers:

- **Layer 1 (Protocol Abstraction)**: Low-level raw data packet handling via pure C code (`net_anet.h`, `net_sysdep.C`, `net_udp.C`, `net_wins.C`)
- **Layer 2 (Network Messages)**: Client/server communication framework with message queues, bandwidth control, and acknowledgment-based reliability (`network.h`, `network.C`)
- **Layer 3 (Network Aware Objects)**: C++ base class system for objects that automatically synchronize across the network (NAOs)

The documentation is primarily intended for developers integrating Armagetron's networking into other GPL projects. Note: The documentation notes that Layer 1 docs are outdated due to refactoring, but Layers 2 and 3 remain largely valid.

The files are M4 macro templates that generate HTML documentation. They are processed by the build system's HTML generation framework (via `../HtmlMakefile`).

## Directory Structure

```
.
├── head.html.m4      # Common HTML header template
├── index.html.m4     # Main documentation entry point
├── lower.html.m4     # Layer 1: Protocol abstraction documentation
├── middle.html.m4    # Layer 2: Network messages documentation
├── upper.html.m4     # Layer 3: Network aware objects documentation
├── navbar.html.m4    # Navigation bar template
├── sig.m4            # Signature footer template
├── Makefile.am       # Autotools makefile for documentation build
└── Makefile.in       # Generated makefile
```
