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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
  
***************************************************************************

*/

#include "gAICharacter.h"
#include "tDirectories.h"
#include "tRecorder.h"
#include <fstream>
#include <sstream>

tArray<gAICharacter> gAICharacter::s_Characters(0);

static REAL weight[AI_PROPERTIES]=
    {
        10,
        10,
        10,
        10,
        10,
        10,
        10,
        10,
        10,
        10,
        0,
        0,
        0
    };

// load this description from a stream. Return value: success or not?
bool gAICharacter::Load(std::istream& file)
{
    file >> name;

    iq  = 1;
    REAL iqt = 1;

    for (int i=0; i< AI_PROPERTIES; i++)
    {
        if (file.eof())
            return false;

        file >> properties[i];
        iq  += properties[i] * weight[i];
        iqt += weight[i];
    }
    iq *= 10/iqt;

    description.ReadLine(file);
    return true;
}

// loads and adds a single AI from a string to the array
static bool LoadSingleAI( tString const & line )
{
    // build stream from
    std::stringstream str(static_cast< char const * >( line ) );

    // gets new character and lets it load the stream
    int i = gAICharacter::s_Characters.Len();
    gAICharacter& c = gAICharacter::s_Characters[i];
    bool ret = c.Load(str);

    // remove character on failure
    if ( !ret )
        gAICharacter::s_Characters.SetLen(i);

    return ret;
}

void gAICharacter::LoadAll(const tString& filename)
{
    std::ifstream f;

    tTextFileRecorder stream( tDirectories::Config(), filename );
    while( !stream.EndOfFile() )
    {
        tString line( stream.GetLine().c_str() );
        if ( line[0] != '#' )
            LoadSingleAI( line );
    }
}

