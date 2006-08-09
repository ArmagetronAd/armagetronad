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

//! @file
//! @brief Contains the class declarations for comparison vValue members

#ifndef ARMAGETRON_veComparison_h
#define ARMAGETRON_veComparison_h

#include "values/vCore.h"

namespace vValue {
	namespace Expr {
		namespace Comparison {

DeclStdBinaryOp(GreaterThan    )
DeclStdBinaryOp(GreaterOrEquals)
DeclStdBinaryOp(         Equals)
DeclStdBinaryOp(   LessOrEquals)
DeclStdBinaryOp(   LessThan    )
DeclStdBinaryOp(Compare)

		}
	}
}

#endif
