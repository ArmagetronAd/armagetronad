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

#ifndef ArmageTron_CONF_ITEM_TEMP_SETTER_H
#define ArmageTron_CONF_ITEM_TEMP_SETTER_H

#include "tConfiguration.h"

/// @brief Class to temporarily set a configuration item to a specific value (stringly typed base)
class ConfItemTempValueBase
{
public:
    explicit ConfItemTempValueBase(char const* name); // set up modification of config item 'name', store old value
    ~ConfItemTempValueBase();                         // restore old value

    /// @brief returns the old value
    /// @return the old value as string
    std::string const& GetOldStringValue() const noexcept { return oldValue_; }
    std::string GetCurrentStringValue() const noexcept;
    void SetCurrentStringValue(std::string const& value) noexcept;

private:
    tConfItemBase* item_;  ///< pointer to confitem to modify
    std::string oldValue_; ///< old value as string

    static tConfItemBase* Find(tString const& name);

    ConfItemTempValueBase(ConfItemTempValueBase const&) = delete;
    ConfItemTempValueBase& operator=(ConfItemTempValueBase const&) = delete;
};

/// @brief Class to temporarily set a configuration item to a specific value
template <typename T>
class ConfItemTempValue : ConfItemTempValueBase
{
public:
    using ConfItemTempValueBase::ConfItemTempValueBase;

    /// @brief temporarily sets a new value
    /// @tparam T type of value
    /// @param value value to set
    void SetValue(T const& value)
    {
        std::ostringstream os;
        tConfItem<T>::DoWrite(os, value);
        SetCurrentStringValue(os.str());
    }

    /// @brief returns the old value
    /// @return the old value
    T GetOldValue() const noexcept
    {
        return DoRead(GetOldStringValue());
    }

    /// @brief returns the current value
    /// @return the current value
    T GetCurrentValue() const noexcept
    {
        return DoRead(GetCurrentStringValue());
    }

private:
    /// @brief convert string to T
    /// @param value value as string
    /// @return value as T
    static T DoRead(std::string const& value)
    {
        std::istringstream is(value);
        T ret{};
        tConfItem<T>::DoRead(is, ret);
        return ret;
    }
};

#endif // ArmageTron_CONF_ITEM_TEMP_SETTER_H
