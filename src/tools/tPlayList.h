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

tPlayList by Dave Fancella

*/

#include "tString.h"
#include <deque>
#include <map>

#ifndef TPLAYLIST_H
#define TPLAYLIST_H
// Playlist options

enum { PLAYLIST_INTERNAL, PLAYLIST_CUSTOM };

class tSong {
public:
    tSong();
    tSong(tString& pathToSong);
    tSong(const tSong& newSong);

    tString location;

    tString author;
    tString year;
    tString record;
    tString genre;
    tString title;

    std::map<tString, tString> tracklist;
    std::map<tString, tString> sequence;
};

class tPlayList {

public:
    tPlayList();
    bool LoadPlaylist(const char *playlist);
    void Randomize();
    const tSong& GetNextSong();
    const tSong& GetPreviousSong();
    bool empty();
    void SetPlaylist(int i) { usePlaylist = i; };

private:
    std::deque<tSong> m_Playlist;
    std::deque<tSong>::iterator m_CurrentSong;

    int usePlaylist;
};


#endif // TPLAYLIST_H
