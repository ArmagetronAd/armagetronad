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

#ifndef ArmageTron_TODO_H
#define ArmageTron_TODO_H

// defines a way to do things at the next possible time

typedef void tTODO_FUNC();

void st_ToDo_Signal(tTODO_FUNC *td); // postpone something, callable from a signal handler
void st_ToDo(tTODO_FUNC *td); // postpone something
void st_ToDoOnce(tTODO_FUNC *td); // postpone something, avoid double entries
void st_DoToDo(); // do the things that have been postponed

#endif
