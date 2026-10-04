# src/doc/ Directory

## Summary
Documentation files and Doxygen configuration for Armagetron Advanced.

## Details

The doc directory contains documentation-related files for Armagetron Advanced. These include configuration files for Doxygen, HTML generation, and other documentation processing tools.

Key files include:
- `Doxyfile.in` - Template for Doxygen configuration file. This is processed by configure to generate the actual Doxyfile with proper paths and settings for API documentation generation.
- `README.md` - Documentation overview with @defgroup Doxygen tags for code documentation groups
- `HtmlMakefile` - Makefile for generating HTML documentation. This provides targets for building HTML-based documentation from various sources.
- `Makefile.am` - Autotools makefile with doxygen build target that processes Doxyfile.in and assembles main README
- `filter_h.py` - Python filter for .h files that adds @ingroup and @brief Doxygen tags automatically
- `filter_md.sh` - Shell filter for markdown files that converts math expressions to Doxygen format
- `MAINPAGE.md` - Main page content for Doxygen documentation
- `EXPERIMENTS.md` - Experimental documentation and notes
- `net/` - Network-related documentation (likely historical or reference material)
- `Content_Creation/` - Documentation about creating game content

The documentation system generates API reference from source code comments using Doxygen. The HTML documentation can be built as part of the development process and is useful for understanding the codebase structure and APIs. Doxygen processes all .h, .cpp, and .m4 files in the source tree, with filter scripts enhancing header files with group information.

## Directory Structure

```
.
├── Content_Creation/  # Documentation about creating game content
├── Doxyfile.in        # Doxygen configuration template
├── HtmlMakefile      # HTML documentation build Makefile
├── Makefile.am       # Autotools makefile with doxygen target
├── README.md         # Documentation overview with Doxygen groups
├── MAINPAGE.md       # Main page for Doxygen
├── EXPERIMENTS.md    # Experimental documentation
├── filter_h.py        # Python filter for header files
├── filter_md.sh       # Markdown math filter
├── net/              # Network-related documentation
└── .gitignore        # Git ignore patterns for documentation
```

## Technologies

- **Documentation**: Doxygen 1.16.1+
- **Build System**: Autotools integration
- **Output**: HTML, LaTeX, or other formats
- **Filtering**: Python and shell scripts for preprocessing source files

## Coding Conventions

- **Doxygen Comments**: Standard Doxygen comment format (`///`, `/** */`, etc.)
- **Doxygen Groups**: @defgroup tags in README.md files define module groupings
- **Template Files**: .in extension for configure templates
- **Documentation Style**: Follows project documentation guidelines
- **Math Support**: Use `@f$...@f$` for inline math and `@f[...@f]` for block math in Doxygen

## Key Patterns

- Documentation generation pattern
- Template processing pattern
- API reference documentation
- Automatic group assignment via filter scripts
- Preprocessing of source files for enhanced documentation

## Build System

- `Doxyfile.in` processed by configure to create `Doxyfile`
- `HtmlMakefile` provides HTML documentation build targets
- `Makefile.am` provides `doxygen` target that builds API documentation
- Documentation can be built with `make doxygen` or similar targets
- `filter_h.py` automatically adds @ingroup tags to classes based on directory structure
- Installed to `${docdir}` (typically /usr/share/doc/armagetronad/)
