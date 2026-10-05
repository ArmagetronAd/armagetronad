# Documentation

This folder contains two documentation generation systems:
 - Doxygen over Doxyfile.in for code documentation
 - Ad-Hoc macro processing using m4 for user documentation (we should probably migrate away from that)

@defgroup Documentation Documentation
@brief Documentation generation using Doxygen and ad-hoc m4 macro shenanigans

@ingroup Documentation

## Documentation Generation

Makefile.am handles converting `.html.m4` files to raw HTML.

Doxyfile.in scoops up every .h, .cpp and .m4 file in the entire source tree and tries to mash them
together into a cohesive whole.
