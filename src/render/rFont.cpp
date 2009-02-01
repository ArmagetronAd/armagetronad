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

#include "rFont.h"
#include "rScreen.h"
#include "tConfiguration.h"
#include "tDirectories.h"
#include "tCoord.h"
#include "tColor.h"
#include <ctype.h>

#ifndef DEDICATED
#include "rRender.h"
#include "rTexture.h"

#ifdef HAVE_FTGL_H
// single include. practical.
#include <ftgl.h>
#else
// alternative includes go here
#include <FTGLPixmapFont.h>
#include <FTGLBitmapFont.h>
#include <FTGLTextureFont.h>
#include <FTGLPolygonFont.h>
#include <FTGLOutlineFont.h>
#include <FTGLExtrdFont.h>
#endif

//#include <GL/gl>
//#include <SDL>

#include "utf8.h"
#include <iconv.h>
#include <errno.h>

//! like strnlen, but that's nonstandard :-(
//! also replaces the equally nonstandard wcsnlen.
static size_t my_strnlen(FTGL_CHAR const * c, size_t i) {
    FTGL_CHAR const *begin = c;
    FTGL_CHAR const *end = c + i;
    for(; *c && c != end; ++c) ;
    return c - begin;
}

// more defines for interface with FTGL
#ifdef FTGL_HAS_UTF8
// string compare
#define my_strncmp strncmp
#else
#define my_strncmp wcsncmp

// conversion functions utf8->wstring
wchar_t sr_utf8216(tString::const_iterator &c, tString::const_iterator const &end) {
    unsigned char char1 = *c;
    if(char1 < 128) {
        // 0xxxxxxx
        return static_cast<wchar_t>(char1);
        //std::cerr << "1: " << std::oct << static_cast<int>(char1) << std::endl;
    } else if (char1 >> 5 == 06) {
        // 110x xxxx   10xx xxxx
        if(char1 == 0xc0 || char1 == 0xc1) return 0xfffd;
        if(++c == end || ( *c & 0xC0 ) != 0x80) return 0xfffd;
        unsigned char char2 = *c;
        return ((static_cast<wchar_t>(char1) & 0x1F) << 6) |
               (static_cast<wchar_t>(char2) & 0x3F);
    } else if (char1 >> 4 == 016) {
        // 1110 xxxx  10xx xxxx  10xx xxxx
        if(++c == end || ( *c & 0xC0 ) != 0x80 ) return 0xfffd;
        unsigned char char2 = *c;
        if(++c == end || ( *c & 0xC0 ) != 0x80) return 0xfffd;
        unsigned char char3 = *c;
        return ((static_cast<wchar_t>(char1) & 0x0F) << 12) |
               ((static_cast<wchar_t>(char2) & 0x3F) <<  6) |
               (static_cast<wchar_t>(char3) & 0x3F);
    } else if (char1 >> 3 == 036) {
        // 11110 xxx  10xx xxxx  10xx xxxx 10xx xxxx
        if(++c == end || ( *c & 0xC0 ) != 0x80) return 0xfffd;
        unsigned char char2 = *c;
        if(++c == end || ( *c & 0xC0 ) != 0x80) return 0xfffd;
        unsigned char char3 = *c;
        if(++c == end || ( *c & 0xC0 ) != 0x80) return 0xfffd;
        unsigned char char4 = *c;
        return ((static_cast<wchar_t>(char1) & 0x07) << 18) |
               ((static_cast<wchar_t>(char2) & 0x3F) << 12) |
               ((static_cast<wchar_t>(char3) & 0x3F) <<  6) |
               (static_cast<wchar_t>(char4) & 0x3F);
    } else {
        return 0xfffd;
    }
}
void sr_utf8216(tString const &in, std::wstring &out) {
    //if(!utf8::is_valid(in.begin(), in.end())) {
    //    tERR_WARN("Invalid utf-8 char");
    //}
    //out.clear();
    //utf8::utf8to16(in.begin(), in.end(), back_inserter(out));
    //std::cerr << in << std::endl;
    //for(std::wstring::const_iterator i = out.begin(); i != out.end(); ++i) {
    //	std::cerr << static_cast<unsigned short>(*i) << std::endl;
    //}
    out.clear();
    //iconv_t cd = iconv_open("UTF-32", "UTF-8");
    //const char *inbuf = in.c_str();
    //size_t inbytesleft = in.size();
    //char outbuf[4];
    //size_t outbytesleft = 4;
    //char *outbufptr = outbuf;
    //int run = 0;
    //std::cerr << in << std::endl;
    //while (true) {
    //	++run;
    //	if(run > 100) return;
    //	outbytesleft = 4;
    //	outbufptr = outbuf;
    //	std::cerr << "inbytesleft: " << inbytesleft << std::endl;
    //	size_t ret = iconv(cd, const_cast<char **>(&inbuf), &inbytesleft, &outbufptr, &outbytesleft);
    //	if(ret == -1) {
    //		std::cerr << *inbuf << std::endl;
    //		std::cerr << (errno == EILSEQ) << std::endl;
    //		std::cerr << (errno == EINVAL) << std::endl;
    //		std::cerr << (errno == E2BIG) << std::endl;
    //		out += static_cast<wchar_t>(*(reinterpret_cast<wchar_t *>(outbuf)));
    //		std::cerr << static_cast<unsigned>(*(reinterpret_cast<wchar_t *>(outbuf))) << std::endl;
    //		outbuf[0] = outbuf[1] = outbuf[2] = outbuf[3] = 0;
    //	} else {
    //		return;
    //	}
    //}
    //iconv_close(cd);
    for(tString::const_iterator c = in.begin(); c != in.end(); ++c) {
        //std::cerr << "char: " << *c << std::endl;
        out += sr_utf8216(c, in.end());
    }
}
#endif // FTGL_HAS_UTF8

int sr_fontType = sr_fontTexture;
static tConfItem< int > sr_fontTypeConf( "FONT_TYPE", sr_fontType, &sr_ReloadFont);

float sr_lineHeight = 1.;
static tConfItem< float > sr_lineHeightconf( "LINE_HEIGHT", sr_lineHeight);

class rFontContainer : std::map<int, FTFont *> {
    FTFont &New(int size);
    FTFont *Load(tString const &path);
public:
    void clear() {
        for(iterator i = begin(); i != end(); ++i) {
            delete i->second;
        }
        std::map<int, FTFont *>::clear();
    }
    /*
    float GetWidth(tString const &str, float height) {
        if(sr_fontType == sr_fontPixmap) {
            return height*(height*sr_screenHeight < sr_bigFontThresholdHeight ? .41 : .5)*str.size();
        }
    }
    */
    float GetWidth(FTGL_STRING const &str, float height) {
        return GetFont(height).Advance(str.c_str())/sr_screenWidth*2.;
    }
    void Render(FTGL_STRING const &str, float height, tCoord const &where) {
        //std::cerr << "len: " << str.size() << std::endl;
        if(sr_fontType >= sr_fontTexture) {
            glPushMatrix();
            glTranslatef(where.x, where.y, 0.);
            glScalef(2./sr_screenWidth, 2./sr_screenHeight, 1.);
            if(sr_fontType == sr_fontTexture) {
                glEnable(GL_TEXTURE_2D);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            }
            if(sr_fontType == sr_fontExtruded) {
                glEnable( GL_DEPTH_TEST);
                glDisable( GL_BLEND);
                glEnable(GL_TEXTURE_2D);
                static rFileTexture sg_RimWallNoWrap(rTextureGroups::TEX_WALL,"textures/dir_wall.png",1,0);
                sg_RimWallNoWrap.Select();
                glRotatef(45,1.,0.,0.);
            }
        } else {
            glRasterPos2f(where.x, where.y);
        }
        GetFont(height).Render(str.c_str());
        if(sr_fontType >= sr_fontTexture) {
            glPopMatrix();
        }
    }
    FTFont &GetFont(float height) {
        static float size_factor = .8; // guess, then improve
        int size = int(height/sr_lineHeight*size_factor*sr_screenHeight/2.+.5);
        FTFont *ret;
        if(count(size)) {
            ret = (*this)[size]; //already exists
        } else {
            ret = &New(size);
        }
        // set the new size factor to what we found out about our
        // current font… this assumes the line height is linear to
        // the font size, which should be true unless the font uses
        // different glyphs for different sizes.
        size_factor = size / ret->LineHeight();
        return *ret;
    }
    void BBox(FTGL_STRING const &str, float height, tCoord where, float &l, float &b, float &r, float &t) {
        if(sr_fontType != sr_fontOld) {
            float rubbish;
            GetFont(height).BBox(str.c_str(), l, b, rubbish, r, t, rubbish);
            l/=sr_screenWidth/2.;
            r/=sr_screenWidth/2.;
            t/=sr_screenHeight/2.;
            b/=sr_screenHeight/2.;
            l+=where.x-0.005;
            r+=where.x+0.005;
            t+=where.y+0.005;
            b+=where.y-0.005;
            return;
        } else {
            l=where.x;
            r=where.x+GetWidth(str, height);
            b=where.y;
            t=where.y+height;
        }
    }
    ~rFontContainer() {
        clear();
    }
};

// Font config items

tString fontFile("Armagetronad.ttf");
static tConfItemLine ff("FONT_FILE", fontFile, &sr_ReloadFont);

static tString customFont("");
static tConfItemLine ffc("FONT_FILE_CUSTOM", customFont, &sr_ReloadFont);

static int useCustomFont = 0;
static tConfItem<int> ufc("USE_CUSTOM_FONT", useCustomFont, &sr_ReloadFont);

static rCallbackBeforeScreenModeChange reloadft(&sr_ReloadFont);

FTFont *rFontContainer::Load(tString const &path) {
    FTFont *font;
    switch (sr_fontType) {
    case sr_fontPixmap:
        font = new FTGLPixmapFont(path);
        break;
    case sr_fontBitmap:
        font = new FTGLBitmapFont(path);
        break;
    case sr_fontPolygon:
        font = new FTGLPolygonFont(path);
        break;
    case sr_fontOutline:
        font = new FTGLOutlineFont(path);
        break;
    case sr_fontExtruded:
        font = new FTGLExtrdFont(path);
        reinterpret_cast<FTGLExtrdFont *>(font)->Depth(10.);
        break;
    default:
        font = new FTGLTextureFont(path);
    }
    return font;
}
FTFont &rFontContainer::New(int size) {
    FTFont *font;
    tString theFontFile("");

    if(useCustomFont == 1) {
        theFontFile = customFont;
    } else {
        theFontFile = "textures/" + fontFile;
        theFontFile = tDirectories::Data().GetReadPath(theFontFile);
    }
    font = Load(theFontFile);
    //std::cout << "Use custom font: " << useCustomFont << std::endl;
    //std::cout << "The font file: " << theFontFile << std::endl;
    if(font->Error()) {
        std::cerr << "Error while loading font from path '" << theFontFile << "'. Error code: " << font->Error() << std::endl;
        //delete font;
        std::cerr << "Loading default font instead.  Sorry." << std::endl;
        font = Load(tDirectories::Data().GetReadPath("textures/Armagetronad.ttf"));

    }
    font->CharMap(ft_encoding_latin_1);
    font->FaceSize(size);
    font->CharMap(ft_encoding_unicode);
    (*this)[size] = font;
    return *font;
}
rFontContainer sr_Font;

void sr_ReloadFont(void) {
    sr_Font.clear();
}
#endif

rTextField::rTextField(REAL Left,REAL Top,
                       REAL Cheight, sr_fontClass Type)
        :parIndent(0),
left(Left),top(Top),cheight(Cheight),x(0),y(0),realx(0),nextx(Left),currentWidth(0),multiline(false),type(Type),cursor(0),cursorPos(0){
    if (cheight*sr_screenHeight<18)
        cheight=18/REAL(sr_screenHeight);

    color_ = defaultColor_;

    width = 1.-Left;

    /*
    top=(int(top*sr_screenHeight)+.5)/REAL(sr_screenHeight);
    left=(int(left*sr_screenWidth)+.5)/REAL(sr_screenWidth);
    */

    cursor_x = -100;
    cursor_y = -100;
}


rTextField::~rTextField(){
    FlushLine();

#ifndef DEDICATED
    if (cursor && sr_glOut){
        if (cursor==2)
            glColor4f(1,1,1,.5);
        else
            glColor3f(1,1,0);

        //    glDisable(GL_TEXTURE);
        glDisable(GL_TEXTURE_2D);

        BeginLines();
        glVertex2f(cursor_x,cursor_y);
        glVertex2f(cursor_x,cursor_y-cheight);
        RenderEnd();
    }
#endif
}

void rTextField::FlushLine(int len,bool newline){
#ifndef DEDICATED
    float realTop = top-y*cheight;
    FTGL_STRING str(buffer.substr(realx, len));
    realx += len;
    if (len >= cursorPos && cursorPos >= 0) {
        cursor_y=realTop;
        cursor_x=nextx+sr_Font.GetWidth(str.substr(0, cursorPos), cheight);
    }
    cursorPos -= len+1;
    float thisx = nextx+sr_Font.GetWidth(str, cheight);

    REAL r = color_.r_;
    REAL g = color_.g_;
    REAL b = color_.b_;
    REAL a = color_.a_;

    if (sr_glOut)
    {
        // render bright background
        if ( color_.IsDark() )
        {
            if ( sr_alphaBlend && !str.empty() )
            {

                glDisable(GL_TEXTURE_2D);

                glColor4f( blendColor_.r_, blendColor_.g_, blendColor_.b_, a * blendColor_.a_ );

                float l,t,r,b;

                sr_Font.BBox(str, cheight, tCoord(nextx, realTop-cheight), l, b, r, t);

                if(t > realTop) {
                    t = realTop;
                }

                //sr_ResetRenderState(true);

                BeginQuads();

                glVertex2f(l, b);

                glVertex2f(r, b);

                glVertex2f(r ,t);

                glVertex2f(l, t);
                RenderEnd();
            }
            else
            {
                if ( r < .5 ) r = .5;
                if ( g < .5 ) g = .5;
                if ( b < .5 ) b = .5;
            }
        }

        glColor4f(r * blendColor_.r_,g * blendColor_.g_,b * blendColor_.b_,a * blendColor_.a_);
    }

    //F->Render(buffer[realx],l,t,l+cwidth,t-cheight);
    glRasterPos2f(nextx, realTop-cheight);
    sr_Font.Render(str, cheight, tCoord(nextx, realTop-cheight));
    nextx = thisx;

#endif
    /*
    for(i=0;i<buffer.Len()-len;i++)
      buffer[i]=buffer[i+len];

    buffer.SetLen(buffer.Len()-len);
    */

    if (newline){
        y++;
        realx=x=0;
        nextx=left;
    }
    else
    {
        //      realx = 0;
        //      buffer.SetLen(0);
    }
    //    x+=len;
}

void rTextField::FlushLine(bool newline){
    FlushLine(buffer.size()-realx,newline);
}

inline void rTextField::WriteChar(FTGL_CHAR c)
{
    switch(c){
    case('\n'):
                    FlushLine();
        buffer.clear();
        break;
    default:
        buffer += c;
        x++;
        break;
    }
}

/*
rTextField & rTextField::operator<<(unsigned char c){
    WriteChar( c );

    if (x>=width)
    {
        // overflow! insert newline
        int i=x-1;
        while (!isspace(buffer(i)) && i>0) i--;

        bool force=false;
        if (x-i>=width-parIndent){
            i=x;
            force=true;
        }


        FlushLine(i-realx);

        if (force)
            cursorPos++;

        for(int j=0;j<parIndent;j++){
            buffer[x++]=' ';
        }
        i++;
        while (i<width)
            buffer[x++]=buffer[i++];
        buffer.SetLen(x);
        buffer[x]='\0';
        if (cursorPos>=0)
            cursorPos+=parIndent;
    }
    return *this;
}
*/

rTextField & rTextField::StringOutput(const FTGL_CHAR * c, ColorMode colorMode)
{
#ifndef DEDICATED
    //float currentWidth = nextx - left;
    float const &maxWidth = width;
    bool lastIsNewline = true;
    bool trouble = false; // Do we have a word that won't fit on a line?
    static FTGL_STRING spaces;
    // run through string
    while (*c!='\0')
    {
        if (trouble && !(*c=='0' && my_strnlen(c, 8)>=8 && c[1]=='x' && colorMode != COLOR_IGNORE)) {
            FTGL_STRING str;
            str += *c;
#ifdef FTGL_HAS_UTF8
            // be sure to add full utf8 character sequences
            if ( (*c & 0x80) == 0x80 )
            {
                c++;
                while ((*c & 0xc0) == 0x80 )
                {
                    str += *(c);
                    c++;
                }

                // gone one step too far
                c--;
            }
            // ( we don't need to worry about the isspace() tests, they'll fail)
#endif
            currentWidth += sr_Font.GetWidth(str, cheight);
            if(isspace(*c)) {
                trouble = false;
            } else if ( currentWidth >= maxWidth) {
                WriteChar('\n');
                spaces.clear();
                for ( int i = parIndent-1; i >= 0; --i )
                {
                    WriteChar(' ');
                    spaces += ' ';
                    cursorPos++;
                }
                currentWidth = sr_Font.GetWidth(spaces, cheight);
            }
        }
        // break line if next space character is too far away
        if ( !trouble && multiline && (isblank(*c) || lastIsNewline) )
        {
            lastIsNewline = false;
            // count number of nonblank characters following
            FTGL_CHAR const * nextSpace = c+1;
            int wordLen = 0;
            while ( *nextSpace != '\0' && *nextSpace != '\n' && !isblank(*nextSpace) )
            {
                if (*nextSpace=='0' && my_strnlen(nextSpace, 8)>=8 && nextSpace[1]=='x' && colorMode != COLOR_IGNORE )
                {
                    // skip color code
                    nextSpace += 8;
                }
                else
                {
                    // count letter
                    nextSpace++;
                    wordLen++;
                }
            }
            FTGL_STRING str(c, nextSpace);
            //TODO: fix for non-utf8 rendering, too
#ifdef FTGL_HAS_UTF8
            str = tColoredString::RemoveColors(str.c_str());
#endif
            float wordWidth = sr_Font.GetWidth(str, cheight);

            currentWidth += wordWidth;
            if ( currentWidth >= maxWidth)
            {
                WriteChar('\n');
                c++;

                spaces.clear();
                for ( int i = parIndent-1; i >= 0; --i )
                {
                    WriteChar(' ');
                    spaces += ' ';
                    cursorPos++;
                }
                float spaceWidth = sr_Font.GetWidth(spaces, cheight);

                if (wordWidth >= maxWidth) {
                    trouble = true;
                    currentWidth = spaceWidth;
                } else {
                    currentWidth = wordWidth + spaceWidth;
                }
                continue;
            }
        }
        if ( *c == '\n' ) {
            lastIsNewline = true;
            currentWidth = 0.;
            cursorPos += 1;
        }

        //// linebreak if line has gotten too long anyway
        //if ( x >= width )
        //{
        //    WriteChar('\n');
        //    currentWidth = 0.
        //}

        // detect presence of color code
        if (*c=='0' && my_strnlen(c, 8)>=8 && c[1]=='x' && colorMode != COLOR_IGNORE )
        {
            tColor color;
            bool use = false;

            FTGL_CHAR const resett[] = {
                                         static_cast<FTGL_CHAR>('0'),
                                         static_cast<FTGL_CHAR>('x'),
                                         static_cast<FTGL_CHAR>('R'),
                                         static_cast<FTGL_CHAR>('E'),
                                         static_cast<FTGL_CHAR>('S'),
                                         static_cast<FTGL_CHAR>('E'),
                                         static_cast<FTGL_CHAR>('T'),
                                         static_cast<FTGL_CHAR>('T'),
                                         0};
            if ( 0 == my_strncmp(c,resett,8) )
            {
                // color reset to default requested
                 color = defaultColor_;
                 use = true;
            }
            else
            {
                // found! extract colors
                cursorPos-=8;
                color = tColor( c );
                use = true;
            }

            // advance
            if ( colorMode == COLOR_USE )
            {
                c+=8;
            }
            else
            {
                // write color code out
                cursorPos+=8;
                for(int i=7; i>=0;--i)
                    WriteChar(*(c++));
            }

            // apply color
            if ( use )
            {
                FlushLine(false);
                cursorPos++;
                color_ = color;
            }
        }
        else
            // normal operation: add char
            WriteChar(*(c++));
    }
#endif
    return *this;
}

rTextField & operator<<(rTextField &c,const FTGL_STRING &x){
    return c.StringOutput(x.c_str());
}

void DisplayText(REAL x,REAL y,REAL h,const char *t,sr_fontClass type,int center,int cursor,int cursorPos, rTextField::ColorMode colorMode){
#ifndef DEDICATED
    tString text( t );
    // transform string for FTGL
    STRING_TO_FTGL( text, str );
    // do so again, with colors removed
    STRING_TO_FTGL( tColoredString::RemoveColors(t), str_colorless );
    float height;
    float width = rTextField::GetTextLengthRaw(str_colorless, h, true, &height);

    // shrink fields that don't fit the screen
    REAL maxw = 1.95 - x;
    if ( width > maxw )
    {
        h *= maxw/(width);
        width=maxw;
    }

    rTextField c(x-(center+1)*width*.5,y+h*.5,h,type);
    if (center==-1)
        c.SetWidth(1.-x);
    else
        c.SetWidth(10000);

    c.SetIndent(5);
    if (cursor)
    {
#ifndef FTGL_HAS_UTF8
        // translate cursor position from byte index to character index
        cursorPos = text.LenUtf8(0, cursorPos);
#endif
        c.SetCursor(cursor,cursorPos);
    }
    c.StringOutput(str.c_str(), colorMode );
#endif
}
// *******************************************************************************************
// *
// *	GetDefaultColor
// *
// *******************************************************************************************
//!
//!		@return		default color
//!
// *******************************************************************************************

tColor const & rTextField::GetDefaultColor( void )
{
    return defaultColor_;
}

// *******************************************************************************************
// *
// *	GetDefaultColor
// *
// *******************************************************************************************
//!
//!		@param	defaultColor	default color to fill
//!
// *******************************************************************************************

void rTextField::GetDefaultColor( tColor & defaultColor )
{
    defaultColor = defaultColor_;
}

// *******************************************************************************************
// *
// *	SetDefaultColor
// *
// *******************************************************************************************
//!
//!		@param	defaultColor	default color to set
//!
// *******************************************************************************************

void rTextField::SetDefaultColor( tColor const & defaultColor )
{
    defaultColor_ = defaultColor;
    if ( !sr_alphaBlend )
    {
        defaultColor_.r_ *= defaultColor_.a_;
        defaultColor_.g_ *= defaultColor_.a_;
        defaultColor_.b_ *= defaultColor_.a_;
        defaultColor_.a_ = 1;
    }
    blendColor_ = tColor();
}

// *******************************************************************************************
// *
// *	GetBlendColor
// *
// *******************************************************************************************
//!
//!		@return		color all other colors are multiplied with
//!
// *******************************************************************************************

tColor const & rTextField::GetBlendColor( void )
{
    return blendColor_;
}

// *******************************************************************************************
// *
// *	GetBlendColor
// *
// *******************************************************************************************
//!
//!		@param	blendColor	color all other colors are multiplied with to fill
//!
// *******************************************************************************************

void rTextField::GetBlendColor( tColor & blendColor )
{
    blendColor = blendColor_;
}

// *******************************************************************************************
// *
// *	SetBlendColor
// *
// *******************************************************************************************
//!
//!		@param	blendColor	color all other colors are multiplied with to set
//!
// *******************************************************************************************

void rTextField::SetBlendColor( tColor const & blendColor )
{
    blendColor_ = blendColor;
    if ( !sr_alphaBlend )
    {
        blendColor_.r_ *= blendColor_.a_;
        blendColor_.g_ *= blendColor_.a_;
        blendColor_.b_ *= blendColor_.a_;
        blendColor_.a_ = 1;
    }
}

tColor rTextField::defaultColor_;
tColor rTextField::blendColor_;

#ifndef DEDICATED
//! @param str the string to be used
//! @param height the height of one character
//! @param stripColors should colors be recognized?
//! @param useNewline should newlines be recognized (and the longest line be found)?
//! @param resultingHeight address to store the number of lines (height times the number of newlines+1)
//! @returns the width of the string if it was printed
float rTextField::GetTextLength (std::string const &utf8str, float height, bool stripColors, bool useNewline, float *resultingHeight) {
    if(stripColors) {
        tString colorlessstr = tColoredString::RemoveColors(utf8str.c_str());
        STRING_TO_FTGL(colorlessstr, str);
        return GetTextLengthRaw( str, height, useNewline, resultingHeight );
    } else {
        STRING_TO_FTGL( utf8str, str );
        return GetTextLengthRaw( str, height, useNewline, resultingHeight );
    }
}

//! @param str the string to be used
//! @param height the height of one character
//! @param useNewline should newlines be recognized (and the longest line be found)?
//! @param resultingHeight address to store the number of lines (height times the number of newlines+1)
//! @returns the width of the string if it was printed
float rTextField::GetTextLengthRaw (FTGL_STRING const &str, float height, bool useNewline, float *resultingHeight) {
    return sr_Font.GetWidth(str, height); //TODO: Implement all the rest!
}

//! @param m_size the size of the font to be used
//! @param m_pos  the top-left corner of the font
//! @param width  the width to be wrapped at
rTextBox::rTextBox(float size, tCoord const &pos, float width) :
        m_size  (size ),
        m_pos   (pos  ),
        m_width (width)
{}

void rTextBox::Render(void) const {
    for(std::vector<item>::const_iterator i = m_items.begin(); i != m_items.end(); ++i) {
        i->Render();
    }
}

void rTextBox::item::Render(void) const {
    m_color.Apply();
    sr_Font.Render(m_text, m_size, m_pos);
}

void rTextBox::SetText(tString const &utf8str) {
    STRING_TO_FTGL( utf8str, str );
    typedef std::pair<tColor, FTGL_STRING> colorText;
    typedef std::vector<colorText> line;
    typedef std::vector<line> lineVector;
    lineVector lines;
    lines.push_back(line());
    lines.back().push_back(colorText(rColor(1.,1.,1.,1.), FTGL_STRING()));
    for(FTGL_STRING::const_iterator c = str.begin(); c != str.end(); ++c) {
        if(*c == '\n') {
            lines.push_back(line());
            lines.back().push_back(colorText(rColor(1.,1.,1.,1.), FTGL_STRING()));
            continue;
        }
        if(*c == '0' && (c+1) != str.end() && c[1] == 'x') {
            bool end=false;
            for(int i = 2; i < 8; ++i) {
                if((c+i) == str.end()) {
                    end=true;
                    break;
                }
                if(!end) {
                    tColor color = tColor( &(*c) );
                    lines.back().push_back(colorText(color, FTGL_STRING()));
                    c+=8;
                    break;
                }
            }
        }
        lines.back().back().second += *c;
    }
    float y=m_pos.y;
    for(lineVector::iterator l=lines.begin(); l != lines.end(); ++l) {
        float currentX=m_pos.x;
        for(line::iterator i=l->begin(); i != l->end(); ++i) {
            float const targetWidth = m_width - currentX + m_pos.x;
            {
                float width;
                //see if it fits
                if((width = sr_Font.GetWidth(i->second, m_size)) < targetWidth) {
                    m_items.push_back(item(tCoord(currentX, y), m_size, i->second, i->first));
                    currentX += width;
                    continue;
                }
            }
            //ok, it doesn't, see where we have to break it
            while (!i->second.empty()) {
                size_t length = i->second.size();
                size_t biggest = 0; // biggest found length that would fit, if it had to
                size_t smallest = i->second.size(); // smallest tested length that doesn't fit
                float smallstep = false; // do 1-char steps now?
                while (biggest+1 < smallest) {
                    //std::cerr << "Checking: " << i->second.substr(0, length) << std::endl;
                    FTGL_STRING checkedString = i->second.substr(0, length);
                    float width = sr_Font.GetWidth(checkedString, m_size);
                    if(smallstep) {
                        if(width > targetWidth) {
                            length--;
                            smallest = length;
                        } else {
                            length++;
                            biggest = length;
                        }
                    } else {
                        float ratio = width / targetWidth;
                        //std::cerr << "ratio: " << ratio << std::endl;
                        size_t newpos = static_cast<int>(length / ratio);
                        if (newpos > i->second.size()) newpos = i->second.size();
                        if(newpos >= smallest || newpos <= biggest) {
                            smallstep = true;
                        }
                        if(width > targetWidth) {
                            smallest = newpos;
                        } else {
                            biggest = newpos;
                        }
                        length = newpos;
                    }
                }
                size_t pos;
                for(pos = biggest; !isspace(i->second[pos]); --pos) ;
                if(pos == 0) pos = biggest; //the word won't fit
                //TODO: word break
                m_items.push_back(item(tCoord(currentX, y), m_size, i->second.substr(0, pos), i->first));
                currentX = m_pos.x;
                y -= m_size;
                i->second = i->second.substr(pos + 1);
            }
        }
        y -= m_size;
    }
}

//static rTextBox test(0.05, tCoord(-.95, 0.), 1.9);
//class cls {
//public:
//    cls();
//    int i;
//};
//cls::cls() {
//    //test.SetText(tString("Hi there! 0xff0000green!\nNewline! Word1, Word2, Word3, Word4, Word5, Word6, Word7, Word8, Word9, Word10, Word11, Word12, Word13, Word14, Word15, Word16, Word17, Word18, ... €öäüßa"));
//}
//void asdf(){
//    //static cls obj;
//    //obj.i = 2;
//    //sr_ResetRenderState(false);
//    //test.Render();
//	DisplayText(0,0,.1,"…€äöü§xyz",sr_fontConsole);
//}
//static rPerFrameTask asdfgh(&asdf);

#endif
