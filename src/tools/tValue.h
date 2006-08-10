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
//! @brief Contains the class declarations for all tValue members

#ifndef ARMAGETRON_VALUE_H
#define ARMAGETRON_VALUE_H

#include "values/vCore.h"
#include "values/vRegistry.h"
#include "values/vCollection.h"
#include "values/veComparison.h"
#include "values/veLogic.h"
#include "values/veMath.h"
#include "values/vebLegacy.h"
#include "values/vebMathExpr.h"

namespace tValue {
using namespace vValue;
using namespace Expr::Core;
using namespace Expr::Collection;
using namespace Expr::Bindings;
using namespace Expr::Bindings::Legacy;
using namespace Expr::Logic;
using namespace Expr::Math;
namespace Func {
using namespace Expr::Math;
using namespace Expr::Math::Trig;
}
using namespace Expr::Comparison;
using namespace MiscWTF;
using namespace Type;
using namespace Registry;
typedef Expr::Bindings::MathExpr Expr;	// must be last, since it redefines Expr
}

#endif
