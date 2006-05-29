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

#include "tDirectories.h"
#include "tPlayList.h"
#include <fstream>
#include <iostream>
#include "tString.h"
#include <algorithm>
#include "tConsole.h"

tSong::tSong() {
    // do nothing constructor
}

tSong::tSong(tString& pathToSong) {
    location = pathToSong;
}

tSong::tSong(const tSong& copySong) {
    location = copySong.location;
}

tPlayList::tPlayList() {
}

void tPlayList::Randomize() {
    std::random_shuffle(m_Playlist.begin(), m_Playlist.end());
    m_CurrentSong = m_Playlist.begin();
}

const tSong& tPlayList::GetNextSong() {
    std::cout << "Getting next song from playlist\n";
    if(m_CurrentSong == m_Playlist.end() ) {
        m_CurrentSong = m_Playlist.begin();
    } else {
        if(m_Playlist.size() > 1)
            m_CurrentSong++;
            if(m_CurrentSong == m_Playlist.end() )
                m_CurrentSong = m_Playlist.begin();
    }
    std::cout << "Got song:" << (*m_CurrentSong).location << "\n";
    return (*m_CurrentSong);
}

const tSong& tPlayList::GetPreviousSong() {
    std::cout << "Getting previous song from playlist\n";
    if(m_CurrentSong == m_Playlist.begin() ) {
        m_CurrentSong = m_Playlist.end();
        m_CurrentSong--;
    } else {
        if(m_Playlist.size() > 1)
            m_CurrentSong--;
    }
    std::cout << "Got song:" << (*m_CurrentSong).location << "\n";
    return (*m_CurrentSong);
}

bool tPlayList::LoadPlaylist(const char *playlist) {
    std::ifstream playList(playlist);
    if(playList.good() ) {
        std::cout << "Opened playlist\n";
    } else {
        std::cout << "Couldn't open playlist\n";
        return false;
    }
    while(! playList.eof() && playList.good() ) {
        tString oneLine;

        oneLine.ReadLine( playList );
        oneLine = oneLine.Trim();

        // We only care about lines that start with '#', those are music files
        // Also, we require a playlist entry to be at least 2 characters long
        if(! oneLine.StartsWith("#") && oneLine.Len() > 2) {
            const tPath& vpath = tDirectories::Data();
            tSong newSong(oneLine);
            if(usePlaylist == PLAYLIST_INTERNAL) {
                newSong.location = vpath.GetReadPath(oneLine);
                //std::cout << newSong.location << "\n";
            }
            m_Playlist.push_back( newSong );
            //std::cout << newSong.location.GetFileMimeExtension() << "\n";
            tSong last = m_Playlist.back();
        }
    }

    m_CurrentSong = m_Playlist.begin();

    /*/ add at least one song
    if ( m_Playlist.empty() )
        m_Playlist.push_back( tSong() );*/

    return true;
}

bool tPlayList::empty() {
    return m_Playlist.empty();
}






