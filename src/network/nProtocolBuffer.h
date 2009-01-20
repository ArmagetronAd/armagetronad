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

extern nVersionFeature sn_protocolBuffers;

// new descriptor for protocol buffer messages
class nPBDescriptorBase: public nDescriptorBase
{
public:
    nPBDescriptorBase(unsigned short identification,
                      const char * name, bool acceptEvenIfNotLoggedIn = false);

    //! dumb streaming to message
    inline void StreamTo( google::protobuf::Message const & in, nMessage & out ) const
    {
        DoStreamTo( in, out );
    }

    //! dumb streaming from message
    inline void StreamFrom( nMessage & in, google::protobuf::Message & out  ) const
    {
        DoStreamFrom( in, out );
    }
private:
    //! dumb streaming to message
    virtual void DoStreamTo( google::protobuf::Message const & in, nMessage & out ) const;

    //! dumb streaming from message
    virtual void DoStreamFrom( nMessage & in, google::protobuf::Message & out ) const;
};

// read/write operators for protocol buffers
nMessage& operator >> ( nMessage& m, google::protobuf::Message & buffer );
nMessage& operator << ( nMessage& m, google::protobuf::Message const & buffer );

// templated version of protocol buffer messages
template< class MESSAGE > 
class nPBDescriptor: public nPBDescriptorBase
{
    typedef void Handler( MESSAGE & message, nMessage & envelope );

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
        handler_( protocolBuffer, envelope );
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
                   const char * name, bool acceptEvenIfNotLoggedIn = false )
    : nPBDescriptorBase( identification, name, acceptEvenIfNotLoggedIn )
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
