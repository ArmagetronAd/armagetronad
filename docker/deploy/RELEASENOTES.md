This is from the 0.2.9 branch of development. Our current releases are derived from here.

Changes in AppImage Land: Our AppImage files now are signed and support bandwidth-saving updates via [AppImageUpdate](https://github.com/AppImage/AppImageUpdate). They no longer carry their version in the filename because after an update, that would be a lie.

We now support installation via Flatpak for the 64-bit Linux client. Get the stable versions from [Flathub](https://flathub.org/apps/details/org.armagetronad.ArmagetronAdvanced); [our own repository](https://download.armagetronad.org/docs/flatpak/) has those and also carries the usual test builds.

We started to integrate builds for [macOS](http://wiki.armagetronad.org/index.php?title=MacOS).
Consider them alpha for now, testing and patches very welcome. The current macOS specific issues
can be viewed [on our tracker](https://gitlab.com/armagetronad/armagetronad/-/issues?label_name%5B%5D=macOS).
