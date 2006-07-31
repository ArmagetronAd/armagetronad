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




// Implementation tests

// bandwidth distributor: implementation of arbitrator.
class nBandwitdhDistributor: public nBandwidthArbitrator
{
public:
    nSendBuffer&		SendBuffer()				{ return buffer_; }
    const	nSendBuffer&		SendBuffer()		const	{ return buffer_; }
    nBandwidthControl&	BandwidthControl()			{ return control_; }
    const	nBandwidthControl&	BandwidthControl()	const	{ return control_; }
protected:
private:
    virtual bool DoUseBandwidth( REAL dt );

    virtual REAL TimeScale(){ return 0.1f; }						// higher values let really urgent messages be sent even if the bandwidth control objectd
    virtual REAL PacketOverhead(){ return 60.0f; }					// overhead in bytes per sent packet. Determines average package size

    nSendBuffer buffer_;											// buffer taking the messages
    nBandwidthControl control_;										// bandwidth control
};


// *********************************************************
// bandwidth distributor: implementation of arbitrator.
// *********************************************************

// consumes some bandwidth
bool nBandwitdhDistributor::DoUseBandwidth( REAL dt )
{
    return this->Fill( this->buffer_, this->control_ );
}



// message sending bandwidth task
class nBandwidthTaskMessage: public nBandwidthTask
{
public:
    nBandwidthTaskMessage( nType type, nMessage& message );

    nMessage& Message() const { return *message_; }
protected:
    virtual void DoExecute( nSendBuffer& buffer, nBandwidthControl& control );				// executes whatever it has to do
    virtual int  DoEstimateSize() const;													// estimate bandwidth usage
    //	virtual void DoPriorize();																// rethinks priority
private:
    tJUST_CONTROLLED_PTR< nMessage > message_;
};




// *********************************************************
// nBandwidthTaskMessage: message sending bandwidth task
// *********************************************************

nBandwidthTaskMessage::nBandwidthTaskMessage( nType type, nMessage& message )
        :nBandwidthTask( type ), message_( &message )
{
}

// executes whatever it has to do
void nBandwidthTaskMessage::DoExecute( nSendBuffer& buffer, nBandwidthControl& control )
{
    buffer.AddMessage( *message_, &control );
}

// estimate bandwidth usage
int  nBandwidthTaskMessage::DoEstimateSize() const
{
    return message_->DataLen();
}


















#ifdef DEBUG

static nDescriptor testDescriptor( 399, NULL, NULL, "test" );
//static nDescriptor testDescriptor( 399, NULL, NULL, "test" );

#include "nNetObject.h"

class nTestObject: public nNetObject
{
public:
    nTestObject( nMessage& m ): nNetObject( m ){}
    nTestObject(){};
    virtual nDescriptor& CreatorDescriptor() const;
    virtual bool AcceptClientSync() const{return true;}
};

nDescriptor& nTestObject::CreatorDescriptor() const
{
    static nNOInitialisator< nTestObject > cd( 398, "nTestObject" );
    return cd;
}

// Unit test
class PriorizingTester
{
public:
    PriorizingTester()
    {
        nBandwidthSceduler sceduler;

        {
            tJUST_CONTROLLED_PTR< nBandwidthArbitrator > distributor = tNEW( nBandwitdhDistributor );
            sceduler.AddArbitrator( *distributor );

            {
                nMessage* mess = tNEW( nMessage( testDescriptor) );
                tJUST_CONTROLLED_PTR< nBandwidthTask > messageTask = tNEW( nBandwidthTaskMessage ) ( nBandwidthTask::Type_Vital, *mess );
                distributor->Insert( messageTask );
                sceduler.UseBandwidth( 1.0f );
                distributor->Insert( messageTask );
                sceduler.UseBandwidth( 1.0f );
                distributor->Insert( messageTask );
            }

            {
                tJUST_CONTROLLED_PTR< nNetObject > object = tNEW( nTestObject );
                tJUST_CONTROLLED_PTR< nBandwidthTask > objectTask = tNEW( nBandwidthTaskCreate ) ( nBandwidthTask::Type_Vital, *object );
                objectTask->AddPriority( 1.0f );
                distributor->Insert( objectTask );
            }
        }

        sceduler.UseBandwidth( 1.0f );
        sceduler.UseBandwidth( 1.0f );
        sceduler.UseBandwidth( 1.0f );
    }
};

//static PriorizingTester tester;

#endif

