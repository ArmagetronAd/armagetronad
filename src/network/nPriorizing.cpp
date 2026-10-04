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

#include "nPriorizing.h"
#include "nNetwork.h"
#include "tMemManager.h"

tDEFINE_REFOBJ( nBandwidthTask )
tDEFINE_REFOBJ( nBandwidthArbitrator )

//*************************************************************************
// nBandwidthTask: small task that will eat away some bandwidth
//*************************************************************************

// sets the task type
void nBandwidthTask::SetType ( nType t )
{
    tASSERT( priorizer_ );

    this->RemoveFromHeap();
    type_ = t;
    priorizer_->Tasks( type_ ).Insert( this );
}

// rethinks priority
void nBandwidthTask::DoPriorize()
{
    SetVal( priority_ * waiting_, *this->Heap() );
}

// in wich heap are we?
tHeapBase *nBandwidthTask::Heap() const
{
    tASSERT( priorizer_ );

    return &priorizer_->Tasks( type_ );
}

nBandwidthTask::~nBandwidthTask()
{
    this->RemoveFromHeap();
}

nBandwidthTask::nBandwidthTask( nType type )
        : type_( type )
        , priorizer_( NULL )
{
    waiting_ = .01f;
    priority_ = 0.1f;
}

//*************************************************************************
// nBandwidthTaskPriorizer: bandwidth priorizer: selects bandwidth taks
//*************************************************************************

// inserts a task into the queue
void nBandwidthTaskPriorizer::Insert( nBandwidthTask* task )
{
    nTaskHeap& heap = this->Tasks( task->Type() );

    heap.Insert( task );

    tReferencer< nBandwidthTask >::AddReference( task );

    task->priorizer_ = this;

    task->Priorize();

    this->OnChange();
}

// returns the top priority task
nBandwidthTask* nBandwidthTaskPriorizer::PeekNext( nType type )
{
    nTaskHeap& heap = this->Tasks( type );

    if ( heap.Len() <= 0 )
    {
        return NULL;
    }

    return heap(0);
}

// removes and returns the top priority task
tJUST_CONTROLLED_PTR<nBandwidthTask> nBandwidthTaskPriorizer::Next( nType type )
{
    nTaskHeap& heap = this->Tasks( type );

    if ( heap.Len() <= 0 )
    {
        return NULL;
    }

    tJUST_CONTROLLED_PTR<nBandwidthTask> ret = heap.Remove(0);

    tReferencer< nBandwidthTask >::ReleaseReference( ret );

    ret->priorizer_ = NULL;

    this->OnChange();

    return ret;
}

//*************************************************************************
// nBandwidthArbitrator: bandwidth arbitrator: executes bandwidth tasks
//*************************************************************************

nBandwidthArbitrator::nBandwidthArbitrator()
{
    sceduler_ = NULL;
}

nBandwidthArbitrator::~nBandwidthArbitrator()
{
    if ( sceduler_ )
    {
        sceduler_->RemoveArbitrator( *this );
    }

    tASSERT( NULL == sceduler_ );

    this->RemoveFromHeap();
}

// fills the send buffer with top priority messages
bool nBandwidthArbitrator::Fill( nSendBuffer& buffer, nBandwidthControl& control )
{
    // return whether a message was sent
    bool ret = false;

    // find heap to use
    nType type = this->FirstType();
    if ( type < nBandwidthTask::Type_Count )
    {
        REAL totalPriority = 0.0f;				// total prioirty of already added messages

        bool first = true;
        bool goon = true;
        while ( goon )
        {
            goon = false;

            tJUST_CONTROLLED_PTR< nBandwidthTask > next = this->PeekNext( type );
            if ( next )
            {
                REAL priority = next->Priority();
                REAL value = next->Val();

                // see if we have enough bandwidth reservers to send message
                if ( !first || value  * this->TimeScale() > -control.Score() )
                {
                    // see if the priority of the next sent message justifies the delay caused for the messages already in the buffer
                    if ( priority * this->PacketOverhead() > totalPriority * next->EstimateSize() )
                    {
                        // extract message
                        next = this->Next( type );

                        // send it
                        next->Execute( buffer, control );

                        // sum up priority
                        totalPriority += priority;

                        // try another one!
                        goon = true;
                        ret = true;
                    }
                }
            }

            first = false;
        }
    }

    // reduce priority so it is not picked until the next call to Timestep
    this->SetVal( this->Val() - 100.0f, *this->Heap() );

    return ret;
}

// advances timers of all tasks
void nBandwidthArbitrator::Timestep( REAL dt )
{
    for ( int i = 0; i < nBandwidthTask::Type_Count; ++i )
    {
        nType type = nType(i);

        nTaskHeap& heap = this->Tasks( type );
        int j;

        static tArray< nBandwidthTask* > tasks;
        tasks.SetLen( 0 );

        // copy heap; otherwise, we would risk updating elements twice or not al all
        for ( j = heap.Len()-1; j>=0; --j )
        {
            tasks[j] = heap(j);
        }

        for ( j = tasks.Len()-1; j>=0; --j )
        {
            tasks(j)->Timestep( dt );
        }
    }

    this->OnChange();
}

// determines the type of the message to send next
nBandwidthArbitrator::nType nBandwidthArbitrator::FirstType() const
{
    for ( int i = 0; i < nBandwidthTask::Type_Count; ++i )
    {
        nType type = nType(i);
        if ( this->Tasks(type).Len() > 0 )
        {
            return type;
        }
    }

    return nBandwidthTask::Type_Count;
}

// called on every change of data
void nBandwidthArbitrator::OnChange()
{
    REAL value = 0.0f;

    for ( int i = 0; i < nBandwidthTask::Type_Count; ++i )
    {
        nType type = nType(i);
        const nTaskHeap& heap = this->Tasks(type);
        if ( heap.Len() > 0 )
        {
            value += heap(0)->Val();
        }
    }

    this->SetVal( value, *this->Heap() );
}

tHeapBase* nBandwidthArbitrator::Heap() const
{
    if ( !sceduler_ )
    {
        return NULL;
    }

    return &sceduler_->arbitratorHeap_;
}

//*************************************************************************
// nBandwidthSceduler: distributes bandwidth around all arbitrators
//*************************************************************************
nBandwidthSceduler::~nBandwidthSceduler()
{
    while ( this->arbitratorList_.Len() > 0 )
    {
        this->RemoveArbitrator( *this->arbitratorList_(0) );
    }
}

void nBandwidthSceduler::UseBandwidth( REAL dt )
{
    int i;

    // andvance all timers
    for ( i = this->arbitratorList_.Len()-1; i>=0; --i )
    {
        this->arbitratorList_(i)->Timestep( dt );
    }

    // let the first arbitrator do its job
    if ( this->arbitratorHeap_.Len() <= 0 )
    {
        return;
    }

    bool goon = true;
    while( goon )
    {
        goon = false;

        nBandwidthArbitrator* arbitrator = this->arbitratorHeap_(0);
        tASSERT( arbitrator );

        goon = arbitrator->UseBandwidth( dt );
    }
}

// adds an arbitrator
void nBandwidthSceduler::AddArbitrator		( nBandwidthArbitrator& arbitrator )
{
    tASSERT( NULL == arbitrator.sceduler_ );

    tJUST_CONTROLLED_PTR< nBandwidthArbitrator > keepalive( &arbitrator );

    this->arbitratorHeap_.Insert( &arbitrator );
    this->arbitratorList_.Add( &arbitrator );

    arbitrator.sceduler_ = this;
}

// removes an arbitrator
void nBandwidthSceduler::RemoveArbitrator	( nBandwidthArbitrator& arbitrator )
{
    tASSERT( this == arbitrator.sceduler_ );

    tJUST_CONTROLLED_PTR< nBandwidthArbitrator > keepalive( &arbitrator );

    this->arbitratorHeap_.Remove( &arbitrator );
    this->arbitratorList_.Remove( &arbitrator );

    arbitrator.sceduler_ = NULL;
}























#ifdef DEBUG


//static PriorizingTester tester;

#endif

