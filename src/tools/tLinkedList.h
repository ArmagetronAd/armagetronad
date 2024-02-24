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

#ifndef ArmageTron_tLinkedList_H
#define ArmageTron_tLinkedList_H

#include "tError.h"

#include <stdlib.h>

template <class T>
class tListItem
{
public:
    tListItem() : next(nullptr), anchor(nullptr)
    {
    }

    //! directly insert in list
    tListItem(T*& a) : next(a), anchor(&a)
    {
        a = Self();
        if (next)
            next->anchor = &next;
    }

    ~tListItem()
    {
        Remove();
    }

    //! returns the next list element
    T* Next() { return next; }
    //! returns the next list element
    T const* Next() const { return next; }

    //! returns the previous list element
    T* Prev(T const* a) { return PrevInternal(a); }
    //! returns the previous list element
    T const* Prev(T const* a) const { return PrevInternal(a); }

    //! remove element from the list it is currently in
    void Remove()
    {
        if (anchor)
        {
            *anchor = next;
        }
        if (next)
        {
            next->anchor = anchor;
        }

        anchor = nullptr;
        next = nullptr;
    }

    //! insert this into the list given by the anchor
    void Insert(T*& a)
    {
        if (anchor)
        {
            Remove();
        }

        anchor = &a;
        next = a;
        a = Self();
        if (next)
            next->anchor = &next;
    }

    //! insert this into the list after the given element
    void InsertAfter(T& t)
    {
        Insert(t.next);
    }

    //! swap the lists given by the two anchors
    static void SwapLists(T*& a, T*& b)
    {
        using std::swap;
        swap(a, b);
        if (a)
            a->anchor = &a;
        if (b)
            b->anchor = &b;
    }

    //! returns true if this object is in a list
    bool IsInList() const { return anchor; }

    //! counts the length of a list
    static int Len(T* anchor)
    {
        if (!anchor)
            return 0;
        return anchor->LenFromHere();
    }

    //! counts the length starting at this
    int LenFromHere()
    {
        int ret = 0;
        tListItem* x = this;
        while (x)
        {
            ret++;
            x = x->next;
        }
        return ret;
    }

    // backward compatible
    int Len()
    {
        return LenFromHere();
    }

    //! sorts the list starting at anchor by the templated comparator function
    template <typename comparator>
    static void Sort(T* anchor)
    {
        if (anchor)
            static_cast<tListItem*>(anchor)->SortHere<comparator>();
    }

    template <typename comparator>
    void SortHere()
    {
        tASSERT(*anchor == this);

        // early return statements: empty list or single element in list
        if (!next)
        {
            return;
        }

        T* middle = Self();

        {
            // locate the middle of the list
            T* run = middle;
            while (run)
            {
                middle = middle->next;
                run = run->next;
                if (run)
                {
                    run = run->next;
                }
            }
        }

        // split the list in the middle
        *middle->anchor = nullptr;
        middle->anchor = &middle;

        // retrieve the anchor of the first half list
        T*& first = *anchor;

        // sort the two half lists
        Sort<comparator>(first);
        Sort<comparator>(middle);

        // merge the lists
        {
            T** run = &first;
            while (middle)
            {
                // find the correct place for middle
                while (*run && comparator::Compare(*run, middle) > 0)
                    run = &(*run)->next;

                // remove middle from the second list; care needs to be taken because middle->remove() would modify middle.
                T* insert = middle;
                insert->Remove();

                // insert it into the first list
                insert->Insert(*run);
                run = &insert->next;
            }
        }

        // done!
    }

    // backward compatible
    template <typename comparator>
    void Sort()
    {
        SortHere<comparator>();
    }

protected:
    T* next;
    T** anchor;

    T* Self() { return static_cast<T*>(this); }
    T const* Self() const { return static_cast<T const*>(this); }

private:
    T* PrevInternal(T const* a) const
    {
        if (a == this || !anchor)
            return nullptr;

        // assert that the code below is correct
        tASSERT(reinterpret_cast<tListItem const*>(&anchor) == this);

        // apparently, this is legal; since anchor is our
        // first data element and we have no virtual functions,
        // its offset is guaranteed to be zero.
        // Usually (if the assert above ever fails), one needs
        // to subtract offsetof(tListItem, next), which is noti
        // yet available on our oldest platforms.

        // auto* const rawAnchor = reinterpret_cast<char*>(anchor);
        // char* const rawPrev = rawAnchor - offsetof(tListItem, next);
        auto* const listItemPrev = reinterpret_cast<tListItem*>(anchor); // (rawPrev);
        return static_cast<T*>(listItemPrev);
    }
};

#endif
