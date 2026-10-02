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

// FYI This file demonstrates the *ideal* coding style (hah!)
// and, on the other end, demonstrates some idiosyncrasies of the code base.

/*
FYI Comments starting with `FYI` in them are meant as comments just to describe
what is going on HERE, they are not meant as templates to include in actual code.
*/

/*
FYI Code should be CLEAN, SOLID and DRY if appropriate.
Do not be a zealot about it, though. Really important is that
code should be readable, maintainable, changeable and testable,
in that priority order.

FYI Everything here is functional code, it is included in our
automated tests; it is completely useless, of course.
*/

// FYI include guards
#ifndef ArmageTron_CODING_STYLE_H
#define ArmageTron_CODING_STYLE_H

#include "defs.h"

#include <tuple>

// FYI reference counted objects and their smart pointers
#include "tSafePTR.h"

// FYI for using enums as flag bit collections
#include "tFlagEnums.h"

// FYI classes get a single lowercase letter prefix indicating the library they are in: t for tools, e for enginge, n for network, g for game.
// FYI we pick 'c' here for Coding Style. Classes defined just for tests don't get a prefix; the choices t for Test or u for UnitTests.

// FYI Use Doxygen style comments to document things, keep them brief and clear.

/// class that can count up
class cCounter
{
    // FYI rule of zero: whenever possible, rely on auto-generated destructor and move/copy constructor/assignment.

    // FYI public members come first, users reading a class should first learn what they can do with it
public:
    // FYI method names are CamelCase. Default to 'constexpr const noexcept', remove as needed.

    // FYI trival functions where the function name is all the documentation one needs can stay comment-less

    constexpr int GetCount() const noexcept { return count_; }
    // FYI trivial implementations can also be single line.

    void CountUp() noexcept { count_++; }

    /* FYI slightly longer functions should be defined out-of-line later in the header;
    that gives you room to put a short documenting comment after the declaration.
    Consider putting "Try" at the start of a function name if the function can fail in
    regular operation and communicates success in the return value.
    */
    bool TryCountDown() noexcept; // try to count down, but do not go below zero

    // FYI private members come last
private:
    // FYI member variables are camelCase (lower case start letter),
    // get an underscore at the end and, whenever possible, are initialized with brace initializers

    /// the counter
    int count_{};
};

// FYI On separate implementation and declaration, use Doxygen style comments on a function's implementation.
// FYI On the declaration, still leave a short non-Doxygen comment.

/// @brief Try to decrease the counter
/// @return true if it could be decreased, false if it already was at 0 and nothing happened
inline bool cCounter::TryCountDown() noexcept
{
    // FYI if one of the branches is nontritival, use braces for both.
    if (count_ > 0)
    {
        --count_;
        return true;
    }
    else // FYI for longer branches, consider adding a comment what the condition is now (here: "// count_ <= 0")
    {
        return false;
    }
}

// FYI Reference counted objects are derived from tReferencable, which uses CRTP to cast itself to the correct leaf type

/// class of reference counted objects that counts how many of them are alive at every given time
class cReferenceCounted : public tReferencable<cReferenceCounted>
{
public:
    /// @brief returns the number of currently existing objects of this type
    /// @return the count
    static int GetNumberOfObjects() noexcept { return s_numberOfObjects_.GetCount(); }

    // FYI Rule of Three: Implement destructor, copy constructor and assignment operator together
    virtual ~cReferenceCounted() noexcept
    {
        auto success = s_numberOfObjects_.TryCountDown();
        tASSERT(success);
        std::ignore = success;
    } // FYI if this is a leaf class, mark it with 'final', then you can make the destructor non-virtual

    // FYI standard functions (destructor, default constructor, copy/move constructor/assignment) need no doxygen docu, unless they do something unusual
    cReferenceCounted(cReferenceCounted const& that) noexcept
        : tReferencable<cReferenceCounted>(that)
    {
        s_numberOfObjects_.CountUp();
    }
    cReferenceCounted& operator=(cReferenceCounted const&) noexcept = default;

    // FYI though in this case, the assignment operator needs no special implementation, the default constructor does
    cReferenceCounted() noexcept { s_numberOfObjects_.CountUp(); }

    /* FYI virtual function pattern safe under modification: Have a public wrapper function that calls the virtual function.
    The virtual function itself is private or public and starts with `Do` for actions and `On` for reactions (event handlers).
    Rationale: If we change the function signature later, we don't have to adapt all implementations and call sites together, at once.
    */

    /// @brief makes a copy of this
    /// @return the copy
    cReferenceCounted* Clone() const noexcept { return DoClone(); }

private:
    /// @brief makes a copy of this
    /// @return the copy
    virtual cReferenceCounted* DoClone() const noexcept { return new cReferenceCounted{*this}; }

private:
    // FYI static variables get an s_ prefix. Global variables defined in cpp files get st_, se_, etc.
    static cCounter s_numberOfObjects_;
};

/// a derived class
class cReferenceCountedDerived : public cReferenceCounted
{
public:
    // FYI always use `override` on overridden virtual functions, that way we notice when the base definition changes
    ~cReferenceCountedDerived() noexcept override = default;

private:
    cReferenceCounted* DoClone() const noexcept override { return new cReferenceCountedDerived{*this}; }
};

/// class that holds a refernce to cReferenceCounted, doing shallow copies
class cShallowCopy final
{
public:
    cReferenceCounted* GetTarget() const noexcept { return target_; }
    void SetTarget(cReferenceCounted* target) noexcept { target_ = target; }

    // FYI avoid accidentally creating implicit conversions
    // FYI prefer direct member initialization instead of using SetTarget() here
    explicit cShallowCopy(cReferenceCounted* target) noexcept : target_{target} {}
    template<typename T>
    explicit cShallowCopy(tRefPtr<T>&& target) noexcept : target_{std::move(target)} {}

    // FYI Rule of Zero: tRefPtr does shallow copies, none of the three special functions needs implementing
private:
    /*
    FYI tRefPtr is the go-to reference counting pointer to use. You will find it in the code as tJUST_CONTROLLED_PTR,
    a very poorly chosen name. In my defense, it was the last of the smart pointer to get one.
    The others are (I suggest you do not use them):
        tCheckedPTR<T> keeps track of all tCheckedPTR instances and throws an error if an object with a live pointer to it gets destroyed
        tControlledPTR<T> reference counting pointer like tRefPtr, but uses a tCheckedPTR as a base for extra safety.
    */

    tRefPtr<cReferenceCounted> target_{};
};

/// class that holds a refernce to cReferenceCounted and makes deep copies on copy
class cDeepCopy final
{
public:
    cReferenceCounted* GetTarget() const noexcept { return target_; }
    void SetTarget(cReferenceCounted* target) noexcept { target_ = target; }

    explicit cDeepCopy(cReferenceCounted* target) noexcept : target_{target} {}
    template<typename T>
    explicit cDeepCopy(tRefPtr<T>&& target) noexcept : target_{std::move(target)} {}

    // FYI Rule of Five: default would be shallow copy, avoid that
    ~cDeepCopy() noexcept = default; // FYI except the destructor, the default is fine
    cDeepCopy(cDeepCopy const& that) : target_(CloneFrom(that)) {}
    cDeepCopy& operator=(cDeepCopy const& that)
    {
        SetTarget(CloneFrom(that));
        return *this;
    }

    // FYI rule of five: moves need to explicitly move here; the default implementations do that,
    // but we need to invoke them explicitly because the copy operations above disable them.
    cDeepCopy(cDeepCopy&& that) = default;
    cDeepCopy& operator=(cDeepCopy&& that) = default;
    // FYI It is of course also allowed to declare a class move-only or non-copyable
    // by making copy (and optionally move) operations explicitly deleted.

private:
    // helper function: Clone from other
    static cReferenceCounted* CloneFrom(cDeepCopy const& that)
    {
        if (auto const target = that.GetTarget())
            return target->Clone();
        else
            return nullptr; // FYI prefer nullptr over NULL
    }

    tRefPtr<cReferenceCounted> target_{};
};

// FYI value class for the rest of the style
class cRandomStuff
{
public:
    // FYI rule of zero: No custom constructor, assignment, or destructor

    // FYI methods that accept mutliple switches should define them in a
    // custom enum, indicating their flag nature by writing values as hex
    enum LightTypes /// types of light
    {
        None = 0x0,          ///< no lights
        HeadLights = 0x1,    ///< flag to activate headlights
        TailLights = 0x2,    ///< flag to activate taillights
        Underlighting = 0x4, ///< flag to activate fancy underlighting
    };

    /// @brief activates the selected lights
    /// @param lights lights to activate
    void SetLights(LightTypes lights) noexcept { /*... */ }
    // FYI NOT: void SetLights(bool head, bool tail, bool under) noexcept { /*... */ }
private:
};

// FYI to make enums usable as flag enums with bitwise operators, use this macro at the top of a header file
MARK_FLAG_ENUM(cRandomStuff::LightTypes);

#endif // ArmageTron_CODING_STYLE_H
