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

#include "rFont.h"
#include "rScreen.h"
#include "tConfiguration.h"
#include <ctype.h>

#ifndef DEDICATED
#include "rRender.h"
//#include <GL/gl>
//#include <SDL>
#endif

/*
#include "nConfig.h"

tString lala_font_extra("Anonymous/original/textures/font_extra.png");
static nSettingItem<tString> lalala_font_extra("TEXTURE_FONT_EXTRA", lala_font_extra);
static rFont sr_lowerPartFont(lala_font_extra);
static rFont sr_lowerPartFont("Anonymous/original/textures/font_extra.png");

tString lala_defaultFont("Anonymous/original/textures/font.png");
static nSettingItem<tString> lalala_defaultFont("TEXTURE_DEFAULT_FONT", lala_defaultFont);
rFont rFont::s_defaultFont(lala_defaultFont, &sr_lowerPartFont);
rFont rFont::s_defaultFont("Anonymous/original/textures/font.png", &sr_lowerPartFont);

tString lala_defaultFontSmall("Anonymous/original/textures/font_s.png");
static nSettingItem<tString> lalala_defaultFontSmall("TEXTURE_DEFAULT_FONT_SMALL", lala_defaultFontSmall);
rFont rFont::s_defaultFontSmall(lala_defaultFontSmall, 32,5/128.0,9/128.0,1/128.0);
rFont rFont::s_defaultFontSmall("Anonymous/original/textures/font_s.png", 32,5/128.0,9/128.0,1/128.0);
*/

static rFont sr_lowerPartFont("textures/font_extra.png");
rFont rFont::s_defaultFont("textures/font.png", &sr_lowerPartFont);
rFont rFont::s_defaultFontSmall("textures/font_s.png",32,5/128.0,9/128.0,1/128.0);
//rFont rFont::s_defaultFontSmall("textures/Font.png",0,16/256.0,32/256.0);
//rFont rFont::s_defaultFontSmall("textures/Font.png",0,1/16.0,1/8.0);

rFont::rFont(const char *fileName,int Offset,REAL CWidth,REAL CHeight,REAL op, rFont *lower):
        rFileTexture(rTextureGroups::TEX_FONT,fileName,0,0),
        offset(Offset),cwidth(CWidth),cheight(CHeight),
        onepixel(op),lowerPart(lower)
{
    StoreAlpha();
}

rFont::rFont(const char *fileName, rFont *lower):
        rFileTexture(rTextureGroups::TEX_FONT,fileName,0,0),
        offset(0),cwidth(1/16.0),cheight(1/8.0),
        onepixel(1/256.0),lowerPart(lower)
{
    StoreAlpha();
}

rFont::~rFont(){}

// ******************************************************************************************
// *
// *	ProcessImage
// *
// ******************************************************************************************
//!
//!		@param	surface the surface to process
//!
// ******************************************************************************************

void rFont::ProcessImage( SDL_Surface * surface )
{
#ifndef DEDICATED
    if ( sr_alphaBlend )
        return;

    // pre-blend alpha values
    GLubyte *pixels =reinterpret_cast<GLubyte *>(surface->pixels);

    if (surface->format->BytesPerPixel == 4)
    {
        for (int i=surface->w*surface->h-1;i>=0;i--){
            GLubyte alpha=pixels[4*i+3];
            pixels[4*i  ] = (alpha * pixels[4*i  ]) >> 8;
            pixels[4*i+1] = (alpha * pixels[4*i+1]) >> 8;
            pixels[4*i+2] = (alpha * pixels[4*i+2]) >> 8;
        }
    }
    else if (surface->format->BytesPerPixel == 2)
    {
        for (int i=surface->w*surface->h-1;i>=0;i--){
            GLubyte alpha=pixels[2*i+1];
            pixels[2*i  ] = (alpha * pixels[2*i  ]) >> 8;
        }
    }
#endif
}

void rFont::OnSelect( bool enforce )
{
    rISurfaceTexture::OnSelect( enforce );
    if ( !Loaded() && sr_glOut )
    {
        // abort. It makes no sense to continue without a font.
        tERR_ERROR( "Font file " << this->GetFileName() << " could not be loaded.");
    }
}

// displays c
#ifndef DEDICATED
static rFont * sr_lastSelected = 0;
void rFont::Render(unsigned char c,REAL left,REAL top,REAL right,REAL bot){
    //  if (c > 128 && this == &rFont::s_defaultFont)
    //rFont::s_defaultFontSmall.Render(c, left, top, right, bot);
    //  else
    // if(31<c && 256>c && sr_glOut)
    {
        c-=offset;

        int x=c%16;
        int y=c/16;

        REAL pix = onepixel *.1;
        if (rTextureGroups::TextureMode[rTextureGroups::TEX_FONT] != GL_NEAREST && rTextureGroups::TextureMode[rTextureGroups::TEX_FONT] != GL_NEAREST_MIPMAP_NEAREST)
            pix = onepixel * .5;


        REAL ttop=y*cheight+pix;
        REAL tbot=(y+1)*cheight-pix;
        REAL tleft=x*cwidth+pix;
        REAL tright=(x+1)*cwidth-pix;

        rFont* select = this;
        while (ttop > .999 && select->lowerPart)
        {
            tbot -= 1;
            ttop -= 1;
            select = select->lowerPart;
        }
        if ( sr_lastSelected != select )
        {
            RenderEnd(true);
            select->Select(true);
            sr_lastSelected = select;
        }

        BeginQuads();

        glTexCoord2f(tleft,ttop);
        glVertex2f(   left, top);

        glTexCoord2f(tright,ttop);
        glVertex2f(   right ,top);

        glTexCoord2f(tright,tbot);
        glVertex2f(   right, bot);

        glTexCoord2f(tleft,tbot);
        glVertex2f(   left, bot);
    }
}
#endif

// **************************************************

static REAL sr_bigFontThresholdWidth  = 12;
static REAL sr_bigFontThresholdHeight = 24;

static tSettingItem< REAL > sr_bigFontThresholdWidthConf(  "FONT_BIG_THRESHOLD_WIDTH", sr_bigFontThresholdWidth );
static tSettingItem< REAL > sr_bigFontThresholdHeightConf( "FONT_BIG_THRESHOLD_HEIGHT", sr_bigFontThresholdHeight );

static REAL sr_smallFontThresholdWidth  = 5;
static REAL sr_smallFontThresholdHeight = 8;

static tSettingItem< REAL > sr_smallFontThresholdWidthConf(  "FONT_SMALL_THRESHOLD_WIDTH", sr_smallFontThresholdWidth );
static tSettingItem< REAL > sr_smallFontThresholdHeightConf( "FONT_SMALL_THRESHOLD_HEIGHT", sr_smallFontThresholdHeight );

rTextField::rTextField(REAL Left,REAL Top,
                       REAL Cwidth,REAL Cheight,
                       rFont *f)
        :parIndent(0),
        left(Left),top(Top),cwidth(Cwidth),cheight(Cheight),
        F(f),x(0),y(0),realx(0),cursor(0),cursorPos(0){
    if ( cwidth*sr_screenWidth < sr_bigFontThresholdWidth*2 || cheight*sr_screenHeight < sr_bigFontThresholdHeight*2 )
        F=&rFont::s_defaultFontSmall;
    if (cwidth*sr_screenWidth<sr_smallFontThresholdWidth*2)
    {
        cwidth=sr_smallFontThresholdWidth*2/REAL(sr_screenWidth);

        // try to place font at exact pixels
        // left = ( int( ( 1 + left ) * sr_screenWidth -.5 ) +.5 )/REAL(sr_screenWidth) - 1;
    }
    if (cheight*sr_screenHeight<sr_smallFontThresholdHeight*2)
    {
        cheight=sr_smallFontThresholdHeight*2/REAL(sr_screenHeight);

        // try to place font at exact pixels
        // top = ( int( ( 1 + top ) * sr_screenHeight - .5 ) + .5)/REAL(sr_screenHeight) - 1;
    }

    color_ = defaultColor_;

    width = int((1-Left)/cwidth);

    buffer.SetLen(0);
    /*
    top=(int(top*sr_screenHeight)+.5)/REAL(sr_screenHeight);
    left=(int(left*sr_screenWidth)+.5)/REAL(sr_screenWidth);
    */

    cursor_x = -100;
    cursor_y = -100;
}


rTextField::~rTextField(){
#ifndef DEDICATED
    FlushLine();
    RenderEnd();

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

// minimal tolerated values of font color before a white background is rendered
static REAL sr_minR = .5, sr_minG = .5, sr_minB =.5, sr_minTotal = .7;
tSettingItem< REAL > sr_minRConf( "FONT_MIN_R", sr_minR );
tSettingItem< REAL > sr_minGConf( "FONT_MIN_G", sr_minG );
tSettingItem< REAL > sr_minBConf( "FONT_MIN_B", sr_minB );
tSettingItem< REAL > sr_minTotalConf( "FONT_MIN_TOTAL", sr_minTotal );

void rTextField::FlushLine(int len,bool newline){
#ifndef DEDICATED
    // reload textures if alpha blending changed
    {
        static bool alphaBlendBefore = sr_alphaBlend;
        if ( alphaBlendBefore != sr_alphaBlend )
        {
            alphaBlendBefore = sr_alphaBlend;
            sr_lowerPartFont.Unload();
            rFont::s_defaultFont.Unload();
            rFont::s_defaultFontSmall.Unload();
        }
    }

    int i;

    REAL r = color_.r_;
    REAL g = color_.g_;
    REAL b = color_.b_;
    REAL a = color_.a_;

    if (sr_glOut)
    {
               
        // render bright background
        if ( r < sr_minR && g < sr_minG && b < sr_minG || r+g+b < sr_minTotal )
        {
            RenderEnd(true);
            glDisable(GL_TEXTURE_2D);
            if ( sr_alphaBlend )
            {
                glColor4f( blendColor_.r_, blendColor_.g_, blendColor_.b_, a * blendColor_.a_ );

                REAL l=left+realx*cwidth;
                REAL t=top-y*cheight;
                REAL r=l + cwidth * len;
                REAL b=t - cheight;

                BeginQuads();
                glVertex2f(   l, t);

                glVertex2f(   r ,t);

                glVertex2f(   r, b);

                glVertex2f(   l, b);
            }
            else
            {
                if ( r < .5 ) r = .5;
                if ( g < .5 ) g = .5;
                if ( b < .5 ) b = .5;
            }
            RenderEnd(true);
            glEnable(GL_TEXTURE_2D);
        }


        if ( len > 0 )
        {
            glColor4f(r * blendColor_.r_,g * blendColor_.g_,b * blendColor_.b_,a * blendColor_.a_);
            sr_lastSelected = 0;
        }
        for (i=0;i<=len;i++){
            REAL l=left+realx*cwidth;
            REAL t=top-y*cheight;
            
            if (0 <= cursorPos--){
                cursor_x=l;
                cursor_y=t;
            }
            if (i<len){
                F->Render(buffer[realx],l,t,l+cwidth,t-cheight);
                realx++;
            }
        }
    }

#endif
    /*
    for(i=0;i<buffer.Len()-len;i++)
      buffer[i]=buffer[i+len];

    buffer.SetLen(buffer.Len()-len);
    */

    if (newline){
        y++;
        realx=x=0;
    }
    else
    {
        //      realx = 0;
        //      buffer.SetLen(0);
    }
    //    x+=len;
}

void rTextField::FlushLine(bool newline){
    FlushLine(buffer.Len()-realx,newline);
}

inline void rTextField::WriteChar(unsigned char c)
{
    switch (c){
    case('\n'):
                    FlushLine();
        buffer.SetLen(0);
        break;
    default:
        buffer[x++]=c;
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

#ifndef DEDICATED
static REAL CTR(int x){
    return x/255.0;
}
#endif

static char hex_array[]="0123456789abcdef";

char int_to_hex(int i){
    if (i<0 || i >15)
        return 'Q';
    else
        return hex_array[i];
}

int hex_to_int(char c){
    int ret=0;
    for (int i=15;i>=0;i--)
        if (hex_array[i]==c)
            ret=i;
    return ret;
}

rTextField & rTextField::StringOutput(const char * c, ColorMode colorMode )
{
#ifndef DEDICATED
    // run through string
    while (*c!='\0')
    {
        // break line if next space character is too far away
        if ( isblank(*c) )
        {
            // count number of nonblank characters following
            char const * nextSpace = c+1;
            int wordLen = 0;
            while ( *nextSpace != '\0' && *nextSpace != '\n' && !isblank(*nextSpace) )
            {
                if (*nextSpace=='0' && strlen(nextSpace)>=8 && nextSpace[1]=='x' && colorMode != COLOR_IGNORE )
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

            // see if the word plus the space fit into the current line
            if ( wordLen + x + 1 >= width )
            {
                // no. Skip to the next line
                WriteChar('\n');
                c++;
                for ( int i = parIndent-1; i >= 0; --i )
                {
                    WriteChar(' ');
                    cursorPos++;
                }
                continue;
            }
        }

        // linebreak if line has gotten too long anyway
        if ( x >= width )
        {
            WriteChar('\n');
            cursorPos += 1;
        }

        // detect presence of color code
        if (*c=='0' && strlen(c)>=8 && c[1]=='x' && colorMode != COLOR_IGNORE )
        {
            tColor color;
            bool use = false;

            if ( 0 ==strncmp(c,"0xRESETT",8) )
            {
                // color reset to default requested
                color = defaultColor_;
                use = true;
            }
            else
            {
                // found! extract colors
                cursorPos-=8;
                color.r_=CTR(hex_to_int(c[2])*16+hex_to_int(c[3]));
                color.g_=CTR(hex_to_int(c[4])*16+hex_to_int(c[5]));
                color.b_=CTR(hex_to_int(c[6])*16+hex_to_int(c[7]));
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

    RenderEnd( true );
#endif

    return *this;
}

void DisplayText(REAL x,REAL y,REAL w,REAL h,const char *text,int center,int cursor,int cursorPos, rTextField::ColorMode colorMode ){
    int colorlen = strlen(text);

    if ( colorMode == rTextField::COLOR_USE )
    {
        for (unsigned int i=0; i<strlen(text); i++)
        {
            if (text[i] == '0' && text[i+1] == 'x')
                colorlen=colorlen-8;
        }
    }

    // calculate top position so that does not move when we shrink the font
    REAL top = y + h*.5;

    // shrink fields that don't fit the screen
    REAL availw = 1.9f;
    if (center < 0) availw = (.9f-x);
    if (center > 0) availw = (x + .9f);
    if ( colorlen * w > availw )
    {
        h *= availw/(colorlen * w);
        w = availw/REAL(colorlen);
    }

    rTextField c(x-(center+1)*colorlen*.5*w,y+h*.5,w,h);
    if (center==-1)
        c.SetWidth(int((.95-x)/c.GetCWidth()));
    else
        c.SetWidth(10000);

    // did the text field enlarge the font? If yes, there will be wrapping; better make some more room
    if ( c.GetWidth() < colorlen )
    {
        c.SetTop( top );
    }

    c.SetIndent(5);
    if (cursor)
        c.SetCursor(cursor,cursorPos);
    c.StringOutput(text, colorMode );
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
}

tColor rTextField::defaultColor_;
tColor rTextField::blendColor_;



