# language/ Directory

## Summary

Localization and language support files for Armagetron Advanced's internationalization system.

## Details

This directory contains language files that provide translations of the game's text content into various languages. The localization system allows Armagetron Advanced to support multiple languages, making it accessible to a global audience.

Language files typically contain key-value pairs mapping string identifiers to their translations in the target language.

## Directory Structure

```
. (Language translation files, typically with .txt or .po extensions)
```

## Technologies

- **Format**: Custom text-based localization format
- **System**: Custom localization system with fallback support
- **Encoding**: UTF-8 for international character support

## Integration

- **Localization System**: Managed by tLocale class in tools layer
- **String Management**: Localized strings used throughout the UI and game
- **Fallback**: Fallback to base language (English) for missing translations
- **Runtime Switching**: Support for changing languages without restarting

## Key Features

- **Multi-Language Support**: Translations for multiple languages
- **String Externalization**: All user-visible text separated from code
- **Dynamic Loading**: Language files loaded on demand
- **Placeholder Support**: Support for dynamic placeholders in translated strings
- **Context Awareness**: String context for disambiguation

## Language Files

- **Base Language**: English (typically american or british variants)
- **Additional Languages**: German (deutsch), French (french), and potentially others
- **File Naming**: Language-specific filenames (e.g., deutsch.txt, french.txt)
- **Language Index**: languages.txt file indexing available languages

## Usage Patterns

- **UI Localization**: All menu text, dialogs, and interface elements
- **Game Messages**: In-game messages and notifications
- **Documentation**: Help text and tutorial content
- **Error Messages**: Localized error and status messages

## Development Considerations

- **String IDs**: Consistent string identifiers across all languages
- **Context**: Appropriate context for ambiguous terms
- **Updates**: Coordination between code changes and translation updates
- **New Languages**: Process for adding support for new languages