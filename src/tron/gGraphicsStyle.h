// Optional appearance presets. GPL version 2 or later.
#ifndef ArmageTron_GRAPHICS_STYLE_H
#define ArmageTron_GRAPHICS_STYLE_H

extern bool sg_blacklightVisuals;

//! Reconcile the selected renderer without resetting appearance preferences.
void sg_ApplyGraphicsStyle();
//! Apply defaults only when changing style: 0 = Classic, 1 = Legacy.
void sg_SelectGraphicsStyle(int const& style);

#endif
