/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2000  Manuel Moos (manuel@moosnet.de)
Copyright (C) 2004  Armagetron Advanced Team (http://sourceforge.net/projects/armagetronad/)

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

#ifndef DIRENT_INCLUDED
#define DIRENT_INCLUDED

/*

    Declaration of POSIX directory browsing functions and types for Win32.

    Author:  Kevlin Henney (kevlin@acm.org, kevlin@curbralan.com)
    History: Created March 1997. Updated June 2003.
    Rights:  See end of file.
    
*/

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct DIR DIR;

    struct dirent
    {
        char *d_name;
    };

    DIR           *opendir(const char *);
    int           closedir(DIR *);
    struct dirent *readdir(DIR *);
    void          rewinddir(DIR *);

    /*

        Copyright Kevlin Henney, 1997, 2003. All rights reserved.

        Permission to use, copy, modify, and distribute this software and its
        documentation for any purpose is hereby granted without fee, provided
        that this copyright and permissions notice appear in all copies and
        derivatives.
        
        This software is supplied "as is" without express or implied warranty.

        But that said, if there are any problems please get in touch.

    */

#ifdef __cplusplus
}
#endif

#endif

