This is from the 0.2.9 branch of development. Our current releases are derived from here.

## Return of the Mac

[macOS](https://wiki.armagetronad.org/index.php?title=MacOS) builds are now 
considered stable. Any still open macOS specific issues can be viewed 
[on our tracker](https://gitlab.com/armagetronad/armagetronad/-/issues?label_name%5B%5D=macOS).

There is not much else to this release. Mostly bugfixes, adaptions to
new environments, fixes to the build system that bring it more in line with
standards, code quality improvements from fixing all the warnings the
macOS compiler raised.

There is now a VSYNC enable/disable/default option in the display system setup
backported from trunk! That probably was a Z-Man specific itch. Changes in Linux
land switched the default from On to Off, and it is annoying to override with
the command line.

Changes in AppImage Land: Our AppImage files now are signed and support bandwidth-saving updates via [AppImageUpdate](https://github.com/AppImage/AppImageUpdate). They no longer carry their version in the filename because after an update, that would be a lie.

We now support installation via Flatpak for the 64-bit Linux client. Get the stable versions from [Flathub](https://flathub.org/apps/details/org.armagetronad.ArmagetronAdvanced); [our own repository](https://download.armagetronad.org/docs/flatpak/) has those and also carries the usual test builds.

