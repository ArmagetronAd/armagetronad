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
#ifndef TRUE_ARMAGETRONAD_VERSION
#include "tTrueVersion.h"

#include "tVersion.h"
#include "tString.h"

tString st_programName          ("Armagetron Advanced");
tString st_programVersion       (TRUE_ARMAGETRONAD_VERSION);
tString st_programRevId         (TRUE_ARMAGETRONAD_REVID);
int     st_programRevNo         (TRUE_ARMAGETRONAD_REVNO);
tString st_programBranchNick    (TRUE_ARMAGETRONAD_BRANCHNICK);
int     st_programRevZNr        (TRUE_ARMAGETRONAD_ZNR);
tString st_programRevTag        (TRUE_ARMAGETRONAD_REVTAG);
bool    st_programChanged       (TRUE_ARMAGETRONAD_CHANGED);
tString st_programBuildDate     (TRUE_ARMAGETRONAD_BUILDDATE);
int     st_programBranchLca     (TRUE_ARMAGETRONAD_BRANCHLCA);
int     st_programBranchLcaZ    (TRUE_ARMAGETRONAD_BRANCHLCAZ);
tString st_programBranchUrl     (TRUE_ARMAGETRONAD_BRANCHURL);

#endif
