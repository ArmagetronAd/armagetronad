/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2000  Manuel Moos (manuel@moosnet.de)

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

#ifndef tVersion_h
#define tVersion_h 1

static tString  st_programName      ("Armagetron Advanced");    //!< The name of this game                                  (\g)
static tString  st_programVersion;      //!< The build label, or version                                                    (\v)
static tString  st_programRevId;        //!< The revision ID this build is based from, if any.                              (\i)
static int      st_programRevNo;        //!< The revision number this build is based from                                   (\o)
static tString  st_programBranchNick;   //!< The branch's nick                                                              (\p)
static int      st_programRevZNr;       //!< The revisions's z-number, fool's number or whatever shows up right after the z (\z)
static tString  st_programRevTag;       //!< This revision's tag, if any                                                    (\t)
static bool     st_programChanged;      //!< True if working tree contained changes, or if the branch diverged              (\c)
static tString  st_programBuildDate;    //!< The date this build was compiled on                                            (\d)
static int      st_programBranchLca;    //!< The branch's last common ancestor with it's parent branch                      (\l)
static int      st_programBranchLcaZ;   //!< The branch's last common ancestor with it's parent branch, z-number edition    (\m)
static tString  st_programBranchUrl;    //!< The branch's URL, or whatever leads to it                                      (\b)

#endif

