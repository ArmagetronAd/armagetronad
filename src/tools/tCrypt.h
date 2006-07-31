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

#ifndef ArmageTron_CRYPT_H
#define ArmageTron_CRYPT_H

// basic cryptography class

class tCrypt{
public:
    virtual ~tCrypt(){}

    virtual int KeyLength()=0;   // the key length in bytes (NOT BITS)

    virtual int MinDataLen()=0;  // minimum data length (bytes)
    virtual int MaxDataLen()=0;  // maximum data length
    virtual int StepDataLen()=0; // data length step

    virtual void CreateRandomKey(void *keydata)=0;
    virtual bool SetKey(const void *keydata)=0;

    virtual bool Crypt(const void *data_in, int data_len, void *data_out)=0;

    // create crypts for symmetric and asymetric processing
    static tCrypt *CreateSymmetricCrypt(int id=0);
    static tCrypt *CreatePublicEncrypt(int id=0);
    static tCrypt *CreatePrivateEncrypt(int id=0);
};



#endif
