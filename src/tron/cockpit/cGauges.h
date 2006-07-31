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
//! @brief Contains the classes rendering bar and needle gauges
//!
//! This file is a candidate for being moved to src/ui.

#ifndef ARMAGETRON_CGAUGES_H
#define ARMAGETRON_CGAUGES_H

#include "cockpit/cWidgetBase.h"

#ifndef DEDICATED

namespace cWidget {

//! Processes and renders a bar gauge
class BarGauge : public WithSingleData, public WithBackground, public WithForeground, public WithCoordinates, public WithShowSettings, public WithCaption, public WithReverse {
public:
    virtual ~BarGauge() { }; //!< Do- nothing destructor
    void Render(); //!< Renders the gauge
    virtual bool Process(tXmlParser::node cur); //!< Passes on to all Process() functions of the base classes and calls Base::DisplayError() on failure
protected:
    virtual void RenderGraph(float min, float max, float val, float factor, tValue::Base const &val_s); //!< Renders the Background, bar and current value (if enabled)
    virtual void RenderMinMax(tValue::Base const &min_s, tValue::Base const &max_s); //!< Renders the minimum and maximum values (if enabled)
    virtual void RenderCaption(void); //!< Renders the caption (if enabled)
};

//! Renders a vertical bar gauge
class VerticalBarGauge : public BarGauge {
public:
    virtual ~VerticalBarGauge() { }; //!< Do- nothing destructor
protected:
    virtual void RenderGraph(float min, float max, float val, float factor, tValue::Base const &val_s); //!< Renders the Background, bar and current value (if enabled)
    void RenderCaption(void);
};

//! Renders a needle gauge
class NeedleGauge : public BarGauge {
public:
    virtual ~NeedleGauge() { }; //!< Do- nothing destructor
protected:
    virtual void RenderGraph(float min, float max, float val, float factor, tValue::Base const &val_s); //!< Renders the needle and current value (if enabled)
};

}

#endif
#endif
