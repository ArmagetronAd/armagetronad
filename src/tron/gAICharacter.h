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

#ifndef ArmageTron_AICHARACTER_H
#define ArmageTron_AICHARACTER_H

#include "tString.h"
#include "tArray.h"
#include "tInitExit.h"

// definition of the different AI characters that can be activated

extern tString aiPlayersConfig;

#define AI_PROPERTIES 13

class gAICharacter{
public:
    tString name, description;  // name and detailed description

    int properties[AI_PROPERTIES]; // the abilities and qualifications of this AI

    REAL iq;                    // estimated battle strength

    bool Load(std::istream& file); // load this description from a stream. Return value: success or not?

    static tArray<gAICharacter> s_Characters;  // all loaded AI types
    static void LoadAll(const tString& filename);
};


#endif
