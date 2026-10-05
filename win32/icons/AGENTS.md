# win32/icons/ Directory

## Summary

Windows icon resources for Armagetron Advanced game client.

## Details

This directory contains Windows icon files (.ico) used for the Armagetron Advanced game client on Windows platforms. Icons are used for the game executable, desktop shortcuts, taskbar representation, and other Windows-specific visual elements.

Icons in this directory typically include different sizes and resolutions to support various Windows display requirements, from small taskbar icons to large desktop icons.

## Directory Structure

```
. (Icon files of various sizes and styles)
```

## Technologies

- **Format**: Windows Icon format (.ico)
- **Design**: Multiple sizes and color depths in single .ico files
- **Tools**: Icon editing and creation tools

## Key Components

- **Application Icon**: Main game executable icon
- **Desktop Icon**: Icon for desktop shortcuts
- **Taskbar Icon**: Small icon for taskbar and window titles
- **Installer Icons**: Icons for the Windows installer
- **Document Icons**: Icons for file type associations

## Design Standards

- **Brand Consistency**: Icons reflect Armagetron Advanced branding
- **Multiple Sizes**: Support for 16x16, 32x32, 48x48, 64x64, 128x128, and potentially larger sizes
- **Color Depths**: Support for different color depths (16-color, 256-color, true color)
- **Transparency**: Support for alpha channel transparency where applicable
- **Visual Style**: Consistent visual style across all icons

## Integration

- **Resource Files**: Included in Windows resource files (.rc) for compilation
- **Executable**: Embedded in the final game executable
- **Installation**: Used by the Windows installer for desktop and start menu shortcuts
- **File Association**: Used for associating file types with the application

## Build Process

- Compiled into resource files using Windows resource compiler (rc.exe)
- Integrated into the main executable build process
- May be processed by build scripts for different configurations

## Usage

- **Brand Identity**: Visual representation of Armagetron Advanced on Windows
- **User Experience**: Professional appearance in Windows environment
- **Functionality**: Required for proper Windows application integration