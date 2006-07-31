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

#ifndef ArmageTron_tMemStack_H
#define ArmageTron_tMemStack_H

// class for temporal memory allocation; use it as a safe and flexible replacement for
// stacked char[...] arrays. tMemStack Objects need to be destructed in opposite
// construction order.

class tMemStackItem;

class tMemStack
{
public:
    tMemStack	(int minSize = 10);
    ~tMemStack	();

    void* 	GetMem()		const	;	// get the memory pointer
    int	  	GetSize() 		const	;	// get the memory size
    void  	IncreaseMem()			;	// recreate the buffer a bit larger

private:
    int    	index;

    tMemStackItem& Item() const;
};

#endif
