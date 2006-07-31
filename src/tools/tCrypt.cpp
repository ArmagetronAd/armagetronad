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

#include "tCrypt.h"
#include "tMemManager.h"

#include <string>

class tCryptDummy: public tCrypt
{
public:
    virtual ~tCryptDummy(){}

    virtual int KeyLength(){return 0;}   // the key length in bytes (NOT BITS)

    virtual int MinDataLen(){return 16;}  // minimum data length (bytes)
    virtual int MaxDataLen(){return 32;}  // maximum data length
    virtual int StepDataLen(){return 8;} // data length step

    virtual void CreateRandomKey(void *keydata){}
    virtual bool SetKey(const void *keydata){return true;}

    virtual bool Crypt(const void *data_in, int data_len, void *data_out)
    {
        memcpy(data_out, data_in, data_len);
        return true;
    }
};
































tCrypt *tCrypt::CreateSymmetricCrypt(int id)
{
    return tNEW(tCryptDummy);
}

tCrypt *tCrypt::CreatePublicEncrypt(int id)
{
    return tNEW(tCryptDummy);
}

tCrypt *tCrypt::CreatePrivateEncrypt(int id)
{
    return tNEW(tCryptDummy);
}
