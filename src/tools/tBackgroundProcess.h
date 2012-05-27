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

#ifndef     TBACKGROUNDPROCESS_H_INCLUDED
#define     TBACKGROUNDPROCESS_H_INCLUDED

#include "tToDo.h"
#include "tThread.h"
#include "tLockedQueue.h"

//! template that runs void member functions of reference countable objects
template< class T > class tMemberFunctionRunnerTemplate
{
private:
public:
    tMemberFunctionRunnerTemplate( T & object, void (T::*function)() )
    : object_( &object ), function_( function )
    {
    }

    //! runs the function
    void run()
    {
        (object_->*function_)();
    }

    //! runs the function, too. 
    void operator () ()
    {
        run();
    }

    //! schedule a task for execution in a background thread
    static void ScheduleBackground( T & object, void (T::*function)()  )
    {
        // schedule the task into a background thread
        boost::thread(tMemberFunctionRunnerTemplate<T>( object, function ) );
    }

    //! schedule a task for execution in the next tToDo call
    static void ScheduleForeground( T & object, void (T::*function)()  )
    {
        Pending().add( tMemberFunctionRunnerTemplate( object, function ) );
        st_ToDoOnce( FinishAll );
    }
private:
    // queue of foreground tasks
    typedef tLockedQueue< tMemberFunctionRunnerTemplate, boost::mutex > Queue;
    static Queue & Pending()
    {
        static Queue pending;
        return pending;
    }

    // function that calls them
    static void FinishAll()
    {
        // finish all pending tasks
        while( Pending().size() > 0 )
        {
            tMemberFunctionRunnerTemplate next = Pending().next();
            next.run();
        }
    }

    //! pointer to the object we should so something with
    tJUST_CONTROLLED_PTR< T > object_;
    
    //! the function to call
    void (T::*function_)();
};

//! convenience wrapper
class tMemberFunctionRunner
{
public:
    //! runs a member function in a background thread
    template< class T > static void ScheduleBackground( T & object, void (T::*function)() )
    {
        tMemberFunctionRunnerTemplate<T>::ScheduleBackground( object, function );
    }

    //! runs a member function on the next call of st_DoToDo()
    template< class T > static void ScheduleForeground( T & object, void (T::*function)() )
    {
        tMemberFunctionRunnerTemplate<T>::ScheduleForeground( object, function );
    }
};

#endif
