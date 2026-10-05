/*
Armagetron Advanced -- a Tron clone in 3D
Copyright (C) 2000  Manuel Moos (manuel@moosnet.de)

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
*/

#ifndef ArmageTron_MOCK_CONSOLE_H
#define ArmageTron_MOCK_CONSOLE_H

#include "defs.h"
#include "tConsole.h"

/// Console implementation for tests. Swallows real console log.
class MockConsole final : public tConsole
{
public:
    /// Sets up a mock console, swallowing console output, storing the last one for evaluation by tests.
    MockConsole() noexcept : tConsole{}
    {
        RegisterBetterConsole(this);
    }

    /// Returns the last printed log line.
    tString const& GetLastPrinted() const noexcept
    {
        return lastPrinted_;
    }

private:
    /// Swallows a line meant to be printed.
    tConsole& DoPrint(const tString& s) noexcept override
    {
        lastPrinted_ = s;
#ifdef DEDICATED
        // remove origin decorator
        if (lastPrinted_.StartsWith("[0] "))
            lastPrinted_ = lastPrinted_.SubStr(4);
#endif
        return *this;
    }

    /// the captured most recent log line
    tString lastPrinted_;
};

#endif
