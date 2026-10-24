/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2026, Romil Bijarnia

**************************************************************************

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

***************************************************************************

*/

#ifndef ArmageTron_BLOOM_H
#define ArmageTron_BLOOM_H

//! Lets the bright parts of the finished 3D scene glow (BLOOM, BLOOM_INTENSITY,
//! BLOOM_THRESHOLD, BLOOM_RADIUS, BLOOM_HALO). Call once per frame after every
//! viewport's scene has been drawn and before any 2D overlay; the current framebuffer
//! is processed in place. Does nothing when the setting is off, without GLEW,
//! or after an OpenGL error.
void sr_Bloom();

#endif
