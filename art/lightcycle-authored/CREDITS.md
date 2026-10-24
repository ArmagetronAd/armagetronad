# Source credit

**Tron Lightcycle — LiNk2.0**

Source: https://blendswap.com/blend/4098

Released under **Creative Commons Attribution 3.0**. The license supplied with
the download is preserved verbatim in `source/BLENDSWAP_LICENSE.txt`.
License terms: https://creativecommons.org/licenses/by/3.0/

The source author credits **masterxeon1001** for the base human mesh on the
model's BlendSwap page.

The original download is preserved as `source/original.blend`. The working
copy converts the legacy Blender Internal materials to current node materials,
binds the existing UVs and packed textures, shares the light colour between the
cycle and rider, and adds review lighting, cameras and a static trail study.
The authored bike and rider mesh coordinates, topology and posing rig are
preserved underneath the requested single-level smoothing modifiers. Structural
crease and shallow normal-seam settings were adjusted for that pass. These
conversion and smoothing changes are not part of the original download.

The October 23 finish revision repairs light-mask seams and the helmet contour,
uses black non-emissive base materials, shortens the upper rear shell to clear
the identity disc, and replaces the raised rear fin with a flush optical stripe.
The game derivative evaluates this posed mesh, separates its rotating wheels,
and bakes a scalar suit-light mask; each player's colour is applied at runtime.
The original model, rather than these changes, is credited to LiNk2.0.

The later colour correction preserves saturated optical colours in both the
Blender review and the native renderer. It does not change the source authorship.

Retain these source credits and the supplied license with any redistributed
asset or derivative.
