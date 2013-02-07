/*
 
 *************************************************************************
 
 ArmageTron -- Just another Tron Lightcycle Game in 3D.
 Copyright (C) 2005  by Daniel Harple
 and the AA DevTeam (see the file AUTHORS(.txt) in the main source directory)
 
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

#ifndef GOSXURLHANDLER_H_W7Q4H24J
#define GOSXURLHANDLER_H_W7Q4H24J

//! 1) Call before we pass off to Armagetron's main()
void sg_SetupAAURLHandler();

//! 2) Call after everything is initialized.
void sg_StartAAURLHandler( bool & showSplash );

//! 3) Call before exiting
void sg_CleanupAAURLHandler();

#endif /* end of include guard: GOSXURLHANDLER_H_W7Q4H24J */
