/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2005  by 
and the AA DevTeam (see the file AUTHORS(.txt) in the main source directory)

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

#ifndef ArmageTron_ProtocolBuffer_H
#define ArmageTron_ProtocolBuffer_H

// protocol buffers forward declaration
namespace google { namespace protobuf { class Message; } }

#include "nNetwork.h"

#include <map>

extern nVersionFeature sn_protocolBuffers;

// new descriptor for protocol buffer messages
class nPBDescriptorBase: public nDescriptorBase
{
public:
    typedef google::protobuf::Message Message;
    typedef std::map< std::string, nPBDescriptorBase * > DescriptorMap;

    nPBDescriptorBase(unsigned short identification,
                      Message const & prototype, bool acceptEvenIfNotLoggedIn = false);

    static std::string const & DetermineName( Message const & prototype );

    //! dumb streaming to message
    inline void StreamTo( Message const & in, nMessage & out ) const
    {
        DoStreamTo( in, out );
    }

    //! dumb streaming from message
    inline void StreamFrom( nMessage & in, Message & out  ) const
    {
        DoStreamFrom( in, out );
    }

    //! creates a protocol buffer of the managed tpye. Needs to be deleted later.
    inline Message * Create() const
    {
        return DoCreate();
    }

    // flag that marks protocol buffer messages
    static const unsigned short protoBufFlag = 0x8000;
protected:
    static DescriptorMap const & GetDescriptorsByName();

private:
    static DescriptorMap descriptorsByName;

    //! dumb streaming to message
    virtual void DoStreamTo( Message const & in, nMessage & out ) const;

    //! dumb streaming from message
    virtual void DoStreamFrom( nMessage & in, Message & out ) const;

    //! creates a protocol buffer of the managed tpye. Needs to be deleted later.
    virtual Message * DoCreate() const = 0;
};

// read/write operators for protocol buffers
nMessage& operator >> ( nMessage& m, google::protobuf::Message & buffer );
nMessage& operator << ( nMessage& m, google::protobuf::Message const & buffer );

//! argument passed to protocol buffer handlers
template< class MESSAGE >
struct nReceivedProtocolBuffer
{
    //! the buffer itself
    MESSAGE & content;

    //! the network message it came in
    nMessage & envelope;

    // filling constructor
    nReceivedProtocolBuffer( MESSAGE & c, nMessage & e )
    : content(c), envelope(e)
    {}
};

// templated version of protocol buffer messages
template< class MESSAGE > 
class nPBDescriptor: public nPBDescriptorBase
{
    typedef nReceivedProtocolBuffer< MESSAGE > HandlerArgument;
    typedef void Handler( HandlerArgument & message );

    //! instance of this descriptor
    static nPBDescriptor * instance_;

#ifdef DEBUG
    static bool multipleInstances_;
#endif

    //! function actually handling the incoming message
    Handler * handler_;

    //! delegates message handling
    virtual void DoHandleMessage( nMessage & envelope )
    {
        // read into protocol buffer
        MESSAGE protocolBuffer;
        envelope >> protocolBuffer;

        // and delegate
        HandlerArgument argument( protocolBuffer, envelope );
        handler_( argument );
    }

    //! creates a protocol buffer of the managed tpye. Needs to be deleted later.
    virtual Message * DoCreate() const
    {
        return new MESSAGE;
    }
public:
    //! puts a puffer into a message
    nMessage * Transform( MESSAGE const & message ) const
    {
        nMessage * ret = new nMessage( *this );

        if ( sn_protocolBuffers.Supported() )
        {
            // write the message in its native format
            *ret << message;
        }
        else
        {
            // transform the message to our legacy format
            StreamTo( message, *ret );
        }

        return ret;
    }

    //! puts a puffer into a message
    static nMessage * TransformStatic( MESSAGE const & message )
    {
        // if there are multiple instances of descriptors of the same message class,
        // you can't pick one reliably.
        tASSERT( !multipleInstances_ );

        tASSERT( instance_ );
        return instance_->Transform( message );
    }

    nPBDescriptor( unsigned short identification, Handler * handler,
                   bool acceptEvenIfNotLoggedIn = false )
    : nPBDescriptorBase( identification, MESSAGE(), acceptEvenIfNotLoggedIn )
    , handler_( handler )
    {
#ifdef DEBUG
        if ( instance_ )
        {
            multipleInstances_ = true;
        }
#endif
        instance_ = this;
    }
};

//! instance of this descriptor
template< class MESSAGE > 
nPBDescriptor< MESSAGE > * nPBDescriptor< MESSAGE >::instance_ = 0;

#ifdef DEBUG
template< class MESSAGE > 
bool nPBDescriptor< MESSAGE >::multipleInstances_ = false;
#endif

// create a message from a pattern buffer
template< class MESSAGE >
nMessage * nMessage::Transform( MESSAGE const & message )
{
    return nPBDescriptor< MESSAGE >::TransformStatic( message );
}

#endif
