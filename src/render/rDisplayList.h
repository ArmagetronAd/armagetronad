/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2008  Manuel Moos (manuel@moosnet.de) and the AA Development Team

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

#ifndef ArmageTron_DISPLAYLIST_H
#define ArmageTron_DISPLAYLIST_H

#include "aa_config.h"

#include "rGL.h"
#include "tLinkedList.h"


// Usage example for caching display elements:

#if 0
void example()
{
    static rDisplayList list;

    // + some logic calling list.Clear() when it needs updating

    if ( !list.Call() )
    {
        rDisplayListFiller filler( list );

        // + rendering code
    }
}

// Things we have found to be problematic in certain OpenGL
// implementations:
// - Redundant color settings (glColor3f(1,1,1);glColor3f(1,1,1);)
// - Infinite points (glVertex4f(1,1,0,0);)
#endif

//! display list wrapper
class rDisplayList: public tListItem< rDisplayList >
{
    friend class rDisplayListFiller;
public:
    rDisplayList();
    ~rDisplayList();

    bool IsSet() const
    {
#ifndef DEDICATED
        return list_ && !inhibit_;
#else
        return false;
#endif
    }

    bool IsInhibited() const
    {
#ifndef DEDICATED
        return inhibit_;
#else
        return false;
#endif
    }

    //! check whether a displaylist is currently being recorded.
    static bool IsRecording();
    
    //! calls the display list, returns true if there was a list to call
    bool Call()
    {
        return OnCall();
    }

    //! clears the display list and don't regenerate it for the next few calls
    void Clear( int inhibitGeneration = 1 );

    //! clears all display lists
    static void ClearAll();

    //! cancels recording of the current display list
    static void Cancel();
protected:
    //! calls the display list, returns true if there was a list to call
    virtual bool OnCall();
private:
    rDisplayList( rDisplayList const & );
    rDisplayList & operator = ( rDisplayList const & );

#ifndef DEDICATED
    GLuint list_;   //!< the display list
    int inhibit_;   //!< inhibit display list generation for a while
    bool filling_;  //!< set if we're just filling the list
#endif
};

//! display list wrapper for display lists changing when the alpha blending setting changes
class rDisplayListAlphaSensitive: public rDisplayList
{
public:
    rDisplayListAlphaSensitive();

protected:
    //! calls the display list, returns true if there was a list to call
    virtual bool OnCall();
private:
    bool lastAlpha_; //!< the last alpha blending value
};

//! create an object of this type to fill a display list while you render
class rDisplayListFiller
{
    friend class rDisplayList;
public:
    //! constructor, automatically starting to fill teh list
    explicit rDisplayListFiller( rDisplayList & list, bool respectBlacklist = true );
    ~rDisplayListFiller();

    //! starts filling the display list (done automatically on construction)
    void Start( bool respectBlacklist = true );
    
    //! stops filling the display list (done automatically on destruction)
    void Stop();

private: 
    rDisplayListFiller( rDisplayListFiller const & );
    rDisplayListFiller & operator = ( rDisplayListFiller const & );

#ifndef DEDICATED
    //! the list
    rDisplayList & list_;
#endif
};

#endif


