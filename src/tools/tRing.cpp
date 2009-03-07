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

#include "aa_config.h"
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <cstddef>
#include "tRing.h"
//#include "gStuff.h"

/* ***************************************************
   tRing:
   ************************************+************** */


tRing::tRing(){ringNext=this;ringPrev=this;}
tRing::tRing(tRing *insert):ringNext(insert->ringNext),ringPrev(insert){
    insert->ringNext=this;
    ringNext->ringPrev=this;
}

tRing::~tRing(){
    ringNext->ringPrev=ringPrev;
    ringPrev->ringNext=ringNext;
#ifndef WIN32
    ringNext=NULL;
    ringPrev=NULL;
#endif
}

