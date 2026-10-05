# src/thirdparty/utf8/ Directory

## Summary

UTF-8 encoding and decoding library for C++ (utf8.h by Nemanja Trifunovic).

## Details

This directory contains a comprehensive UTF-8 handling library for C++ applications. The utf8.h header provides a complete set of functions for working with UTF-8 encoded strings, including iteration, conversion, and validation.

The library is particularly useful for internationalization support, handling Unicode text, and ensuring proper character encoding throughout the application.

## Directory Structure

```
.
└── utf8.h              # Complete UTF-8 library header
```

## Technologies

- **Language**: C++ (header-only library)
- **Author**: Nemanja Trifunovic
- **License**: Public domain / permissive license
- **Dependencies**: None (header-only)

## Key Components

### Core Functionality
- **UTF-8 Iteration**: Safe iteration over UTF-8 strings character by character
- **String Conversion**: Conversion between UTF-8, UTF-16, UTF-32, and wchar_t
- **Validation**: Check if a string contains valid UTF-8 data
- **Character Properties**: Test if a character is a specific type (letter, digit, etc.)
- **Case Conversion**: UTF-8 aware case conversion (uppercase, lowercase)
- **Normalization**: Unicode normalization forms (NFC, NFD, NFKC, NFKD)

### Classes and Functions
- **utf8::string**: UTF-8 string wrapper with extensive functionality
- **utf8::iterator**: Iterator for UTF-8 strings
- **utf8::next()**: Move to next UTF-8 character
- **utf8::prior()**: Move to previous UTF-8 character
- **utf8::distance()**: Distance between UTF-8 characters
- **utf8::find()**: Find UTF-8 characters or sequences
- **utf8::replace()**: Replace UTF-8 characters or sequences

## Integration

- **Used by**: Text processing, localization, and internationalization systems
- **Purpose**: Enable proper Unicode and UTF-8 support throughout the game
- **Inclusion**: Simple `#include "utf8.h"` for full functionality

## Key Features

- **Header-only**: No compilation or linking required
- **Complete**: Full UTF-8 and Unicode support
- **Portable**: Works across different platforms and compilers
- **Efficient**: Optimized for performance-critical applications
- **Safe**: Proper bounds checking and error handling

## Usage Examples

- **Text Processing**: Iterate over UTF-8 strings without breaking multi-byte characters
- **Internationalization**: Handle non-ASCII text in game interfaces and chat
- **Validation**: Ensure user input contains valid UTF-8
- **Conversion**: Convert between different Unicode encodings
- **String Operations**: Case-insensitive comparisons, substring operations

## Standards Compliance

- **UTF-8**: Full UTF-8 encoding/decoding support
- **Unicode**: Comprehensive Unicode character property support
- **Normalization**: Implements all Unicode normalization forms