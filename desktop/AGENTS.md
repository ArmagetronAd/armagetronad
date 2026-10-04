# desktop/ Directory

## Summary

Desktop integration files and resources for Armagetron Advanced.

## Details

This directory contains files and resources for desktop integration of Armagetron Advanced. These files enable proper integration with desktop environments, including desktop icons, menu entries, file associations, and other desktop-specific configurations.

The desktop integration files support various desktop environments and operating systems, ensuring that Armagetron Advanced integrates seamlessly with the user's desktop experience.

## Directory Structure

```
.
├── icons/               # Desktop icons in various sizes
│   ├── 16x16/          # 16x16 pixel icons
│   ├── 32x32/          # 32x32 pixel icons
│   ├── 48x48/          # 48x48 pixel icons
│   ├── 64x64/          # 64x64 pixel icons
│   └── 128x128/        # 128x128 pixel icons
└── os-x/              # macOS-specific desktop integration files
```

## Technologies

- **Icon Formats**: PNG, possibly other formats
- **Desktop Standards**: Freedesktop.org standards for Linux, Windows conventions, macOS standards
- **Integration**: Platform-specific desktop integration approaches

## Integration

- **Installation**: Files copied to appropriate system directories during installation
- **Platform Support**: Platform-specific desktop integration for each supported OS
- **User Experience**: Seamless integration with the user's desktop environment

## Key Features

- **Application Icons**: Icons for desktop shortcuts, launchers, and application menus
- **File Associations**: Desktop file type associations for game data files
- **Menu Integration**: Integration with system application menus
- **MIME Types**: Proper MIME type definitions and associations
- **Desktop Files**: .desktop files for Linux, .app bundles for macOS, etc.

## Platform Support

- **Linux**: Freedesktop.org standard compliance (.desktop files)
- **macOS**: macOS application bundle integration (handled in os-x/ subdirectory)
- **Windows**: Windows desktop integration (shortcuts, registry entries)
- **Cross-Platform**: Consistent desktop integration across all supported platforms

## Usage

- **Application Launching**: Desktop icons and launchers for easy application access
- **File Management**: Proper file type associations and icons
- **User Experience**: Professional desktop integration for better user experience
- **System Integration**: Integration with system utilities and file managers

## Icon Standards

- **Multiple Sizes**: Support for different icon sizes (16x16 to 128x128)
- **Color Depths**: Appropriate color depths and transparency
- **Platform Conventions**: Platform-specific icon conventions and standards
- **Consistency**: Consistent visual identity across all icon sizes