# language/ Directory

## Summary
Localization and translation files for multi-language support.

## Details

The language directory contains all localization files for Armagetron Advanced. These files provide translations of all user-facing strings in the game, allowing players from different regions to use the software in their preferred language.

Each language file is a text file containing key-value pairs where the key is an internal string identifier and the value is the translated text. The base English strings are in `english_base.txt`, and variant English versions exist in `american.txt` and `british.txt`. Other supported languages include `deutsch.txt` (German), `french.txt` (French), and potentially others.

The special file `english_base_notranslate.txt` contains strings that are intentionally not translated. These include:
- Deprecated settings that should not be used
- Internal hacks and debugging aids
- User-should-not-touch configuration options
- Undocumented features
- Technical strings that don't benefit from translation

The `languages.txt` file serves as an index, listing all available language files and their corresponding language names and codes.

## Directory Structure

```
.
├── american.txt              # American English strings
├── british.txt              # British English strings
├── deutsch.txt              # German strings
├── english_base.txt         # Base English strings
├── english_base_notranslate.txt  # Non-translatable English strings
├── french.txt               # French strings
└── languages.txt            # Language index/registry
```

## Technologies

- **Format**: Custom key-value text format
- **Parsing**: `tLocale` class in src/tools/
- **Usage**: `tOutput` streaming with language support

## Coding Conventions

- **String IDs**: Descriptive identifiers for each translatable string
- **Placeholders**: `%s`, `%d`, etc. for dynamic content insertion
- **Context**: Comments may provide context for translators
- **Escaping**: Special characters handled appropriately

## Key Patterns

- Localization pattern with fallback to base language
- String catalog pattern
- Language index pattern

## Build System

- Language files are installed to `${datadir}/language/`
- `language_DATA` in Makefile.am handles installation
- `languages.txt` is generated/updated as needed
- Supports runtime language switching
