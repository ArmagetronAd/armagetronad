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

#ifndef TRUE_ARMAGETRONAD_VERION
#include "tTrueVersion.h"


static tString  st_programName      ("Armagetron Advanced");        //!< The name of this game                                                          (\g)
static tString  st_programVersion   (TRUE_ARMAGETRONAD_VERSION);    //!< The build label, or version                                                    (\v)
static tString  st_programRevId     (TRUE_ARMAGETRONAD_REVID);      //!< The revision ID this build is based from, if any.                              (\i)
static int      st_programRevNo     (TRUE_ARMAGETRONAD_REVNO);      //!< The revision number this build is based from                                   (\o)
static tString  st_programBranchNick(TRUE_ARMAGETRONAD_BRANCHNICK); //!< The branch's nick                                                              (\p)
static int      st_programRevZNr    (TRUE_ARMAGETRONAD_ZNR);        //!< The revisions's z-number, fool's number or whatever shows up right after the z (\z)
static tString  st_programRevTag    (TRUE_ARMAGETRONAD_REVTAG);     //!< This revision's tag, if any                                                    (\t)
static bool     st_programChanged   (TRUE_ARMAGETRONAD_CHANGED);    //!< True if working tree contained changes, or if the branch diverged              (\c)
static tString  st_programBuildDate (TRUE_ARMAGETRONAD_BUILDDATE);  //!< The date this build was compiled on                                            (\d)
static int      st_programBranchLca (TRUE_ARMAGETRONAD_BRANCHLCA);  //!< The branch's last common ancestor with it's parent branch                      (\l)
static int      st_programBranchLcaZ(TRUE_ARMAGETRONAD_BRANCHLCAZ); //!< The branch's last common ancestor with it's parent branch, z-number edition    (\m)
static tString  st_programBranchUrl (TRUE_ARMAGETRONAD_BRANCHURL);  //!< The branch's URL, or whatever leads to it                                      (\b)


#endif
