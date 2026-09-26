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

#ifndef ArmageTron_tFlagEnums_H
#define ArmageTron_tFlagEnums_H

// Turn enums into flag enums, allowing for bitwise operations on them
// Idea adapted from https://stackoverflow.com/a/64870807
template <typename T>
struct tIsFlagEnum
{
};

// Use this macro to make an enum a flag enum, allowing for comfty bitwise logical operators
#define MARK_FLAG_ENUM(T) \
    template <>           \
    struct tIsFlagEnum<T> \
    {                     \
        using t = int;    \
    }

template <typename T, typename SFINAE = typename tIsFlagEnum<T>::t>
constexpr T operator|(T lhs, T rhs) noexcept
{
    return static_cast<T>(
        static_cast<typename std::underlying_type<T>::type>(lhs) |
        static_cast<typename std::underlying_type<T>::type>(rhs));
}
template <typename T, typename SFINAE = typename tIsFlagEnum<T>::t>
constexpr T operator&(T lhs, T rhs) noexcept
{
    return static_cast<T>(
        static_cast<typename std::underlying_type<T>::type>(lhs) &
        static_cast<typename std::underlying_type<T>::type>(rhs));
}
template <typename T, typename SFINAE = typename tIsFlagEnum<T>::t>
constexpr T operator~(T rhs) noexcept
{
    return static_cast<T>(
        ~static_cast<typename std::underlying_type<T>::type>(rhs));
}

#endif // FlagEnums
