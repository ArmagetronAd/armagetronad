/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2006, Armagetron Advanced Development Team

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

#include "config.h"
#include "rGL.h"
#include "tConsole.h"
#include "tError.h"
#include <sstream>
#include <iomanip>

#ifdef AA_GL_ERROR_CHECKING
void sr_CheckGLError()
{
    GLenum error;
    while ( (error = glGetError()) != GL_NO_ERROR )
    {
        // don't spam, only report every odd error
        static int count = 0;
        int check = ++count;
        while ( ( check & 1 ) == 0 && check !=0 )
        {
            check >>= 1;
        }
        if ( check > 3 )
        {
            continue;
        }

        std::stringstream s;
        s << "GL error 0X" << std::hex << error << " (" << (char const *)gluErrorString(error) << ")\n";
        con << s.str();

        // catch a breakpoint
        static bool reported = false;
        if ( !reported )
            st_Breakpoint();
        reported = true;
    }
}
#endif
