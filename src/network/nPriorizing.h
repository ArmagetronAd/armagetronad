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

#ifndef ArmageTron_nPRIORIZING_H
#define ArmageTron_nPRIORIZING_H

#include "defs.h"
#include "tHeap.h"

class nBandwidthSceduler;
class nBandwidthArbitrator;
class nSendBuffer;
class nBandwidthControl;
class nBandwidthTask;
class nBandwidthTaskPriorizer;

tDECLARE_REFOBJ( nBandwidthTask )
tDECLARE_REFOBJ( nBandwidthArbitrator )

// small task that will eat away some bandwidth
class nBandwidthTask: public tHeapElement, public tReferencable< nBandwidthTask >
{
    friend class tReferencable< nBandwidthTask >;
    friend class nBandwidthTaskPriorizer;

public:
    enum nType
    {
        Type_System = 0,							// extremely important task for the integrety of the connection
        Type_Vital  = 1,							// task important for someone's survival in the game
        Type_Casual = 2,							// not so important task
        Type_Count  = 3
    };

    void Execute( nSendBuffer& buffer, nBandwidthControl& control )		{	this->DoExecute( buffer, control );	}				// executes the small task
    void Priorize()														{	if ( this->priorizer_ ) this->DoPriorize(); }		// rethinks priority

    int		EstimateSize	( void		) const {	return this->DoEstimateSize(); }	// estimate bandwidth usage

    nType	Type			( void		) const	{	return type_;		}				// returns the type of task
    REAL	Priority		( void		) const {	return priority_;	}				// returns the priority ( event gets important after 1/priority_ seconds )

    void	SetType			( nType t	);												// sets the task type
    void	AddPriority		( REAL add	)		{ priority_ += add; this->Priorize(); }	// adds priority
void	Timestep		( REAL dt	)		{ waiting_ += dt;	this->Priorize(); }	// adds time

protected:
    virtual void DoExecute( nSendBuffer& buffer, nBandwidthControl& control ) = 0;				// executes whatever it has to do
    virtual int  DoEstimateSize() const = 0;													// estimate bandwidth usage
    virtual void DoPriorize();																	// rethinks priority

    virtual tHeapBase *Heap() const;															// in wich heap are we?

    virtual ~nBandwidthTask();
    nBandwidthTask( nType type );
private:
    nType type_;									// type of this task
    REAL priority_;									// something terrible happens if this task is not executed within 1/priority_ seconds
    REAL waiting_;									// time this task is already waiting ( plus small offset )
    nBandwidthTaskPriorizer* priorizer_;			// the arbitrator taking care of this task
};

// bandwidth priorizer: selects bandwidth taks
class nBandwidthTaskPriorizer
{
public:
    typedef tHeap< nBandwidthTask > nTaskHeap;
    typedef nBandwidthTask::nType nType;

    nTaskHeap& Tasks( nType t )
    {
        tASSERT( 0 <= t && t < nBandwidthTask::Type_Count );

        return tasks_[ t ];
    }

    const nTaskHeap& Tasks( nType t ) const
    {
        tASSERT( 0 <= t && t < nBandwidthTask::Type_Count );

        return tasks_[ t ];
    }

    void Insert( nBandwidthTask* task );							// inserts a task into the queue
    nBandwidthTask* PeekNext( nType type );							// returns the top priority task
    virtual ~nBandwidthTaskPriorizer(){}
protected:
    tJUST_CONTROLLED_PTR<nBandwidthTask> Next( nType type );		// removes and returns the top priority task
private:
    virtual void OnChange(){};										// called on every change of data
    nTaskHeap tasks_[ nBandwidthTask::Type_Count ];
};

// bandwidth arbitrator: executes bandwidth tasks
class nBandwidthArbitrator: public nBandwidthTaskPriorizer, public tHeapElement, public tListMember, public tReferencable< nBandwidthArbitrator >
{
    friend class nBandwidthSceduler;
    friend class tReferencable< nBandwidthArbitrator >;

public:
    bool Fill( nSendBuffer& buffer, nBandwidthControl& control );	// fills the send buffer with top priority messages
    void Timestep( REAL dt );										// advances timers of all tasks
    bool UseBandwidth( REAL dt ){ return DoUseBandwidth( dt ); }	// consumes some bandwidth
protected:
    nBandwidthArbitrator();
    ~nBandwidthArbitrator();
private:
    nType FirstType() const;										// determines the type of the message to send next
    virtual void OnChange();										// called on every change of data
    virtual bool DoUseBandwidth( REAL dt ) = 0;
    virtual tHeapBase* Heap() const;

    virtual REAL TimeScale(){ return 0.1f; }						// higher values let really urgent messages be sent even if the bandwidth control objectd
    virtual REAL PacketOverhead(){ return 60.0f; }					// overhead in bytes per sent packet. Determines average package size

    nBandwidthSceduler* sceduler_;
};

// sceduler: distributes bandwidth around all arbitrators
class nBandwidthSceduler
{
    friend class nBandwidthArbitrator;

public:
    ~nBandwidthSceduler();

    void UseBandwidth( REAL dt );										// consumes bandwidth
    void AddArbitrator		( nBandwidthArbitrator& arbitrator );		// adds an arbitrator
    void RemoveArbitrator	( nBandwidthArbitrator& arbitrator );		// removes an arbitrator
private:
    tList< nBandwidthArbitrator, false, true >		arbitratorList_;	// arbitrators managed by the sceduler
    tHeap< nBandwidthArbitrator >					arbitratorHeap_;	// arbitrators managed by the sceduler organized in priority heap
};

class nNetObject;

// object syncing or creating bandwidth task
class nBandwidthTaskObject: public nBandwidthTask
{
public:
    nBandwidthTaskObject( nType type, nNetObject& object );
    virtual ~nBandwidthTaskObject();

    nNetObject& Object() const { return *object_; }
protected:
    virtual int  DoEstimateSize() const;													// estimate bandwidth usage
private:
    tJUST_CONTROLLED_PTR< nNetObject > object_;
};

// object syncing bandwidth task
class nBandwidthTaskSync: public nBandwidthTaskObject
{
public:
    nBandwidthTaskSync( nType type, nNetObject& object )
            : nBandwidthTaskObject( type, object )
    {}

protected:
    virtual void DoExecute( nSendBuffer& buffer, nBandwidthControl& control );				// executes whatever it has to do
};

// object syncing bandwidth task
class nBandwidthTaskCreate: public nBandwidthTaskObject
{
public:
    nBandwidthTaskCreate( nType type, nNetObject& object )
            : nBandwidthTaskObject( type, object )
    {}

protected:
    virtual void DoExecute( nSendBuffer& buffer, nBandwidthControl& control );				// executes whatever it has to do
};

#endif

