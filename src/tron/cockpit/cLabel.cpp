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

#include "cockpit/cLabel.h"

#ifndef DEDICATED

#include "rFont.h"
#include <numeric>

namespace cWidget {

bool Label::Process(tXmlParser::node cur) {
    if (
        WithTable       ::Process(cur) ||
        WithCoordinates ::Process(cur) ||
        WithCaption     ::Process(cur))
        return true;
    else {
        DisplayError(cur);
        return false;
    }
}

void Label::Render()
{
    std::deque<std::deque<tString> > contents;
    unsigned maxlen = 0;
    {
        for(std::deque<std::deque<std::deque<tValue::Set> > >::iterator i(m_table.begin()); i!=m_table.end(); ++i) {
            if(i->size() > maxlen) maxlen = i->size();
        }
    }
    std::deque<float> coloumns(maxlen, 0.); //max widths
    {
        for(std::deque<std::deque<std::deque<tValue::Set> > >::iterator i(m_table.begin()); i!=m_table.end(); ++i) {
            contents.push_back(std::deque<tString>());
            std::deque<float>::iterator m(coloumns.begin());
            for(std::deque<std::deque<tValue::Set> >::iterator j(i->begin()); j!=i->end(); ++j, ++m) {
                tColoredString result;
                for(std::deque<tValue::Set>::iterator k(j->begin()); k!=j->end(); ++k) {
                    result += k->GetVal().GetString();
                }
                float size = rTextField::GetTextLength(result, m_size.y, true);
                if(size > *m) {
                    *m = size;
                }
                contents.back().push_back(result);
            }
        }
    }
    if(m_captionloc != off) {
        int height = m_table.size();
        float width = std::accumulate(coloumns.begin(), coloumns.end(), float()) + (maxlen - 1)*m_size.x;

        tCoord captionloc(m_position.x + (width-rTextField::GetTextLength(m_caption, m_size.y))/2., 0);
        if(m_captionloc == top) {
            captionloc.y = m_position.y + m_size.y;
        } else if(m_captionloc) {
            captionloc.y = m_position.y - height*m_size.y;
        }
        rTextField c(captionloc.x,captionloc.y,m_size.x,m_size.y);
        c<<m_caption;
    }

    tCoord pos(0.,m_position.y);
    //now displaying the results
    {
        for(std::deque<std::deque<tString> >::iterator i(contents.begin()); i != contents.end(); ++i) {
            pos.x = m_position.x;
            std::deque<float>::iterator m(coloumns.begin());
            for(std::deque<tString>::iterator j(i->begin()); j != i->end(); ++j, ++m) {
                rTextField c(pos.x,pos.y,m_size.x,m_size.y);
                c<<*j;
                pos.x += *m+m_size.x;
            }
            pos.y -= m_size.y;
        }
    }
}

}

#endif
