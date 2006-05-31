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
#include "tDirectories.h"
#include "tCoord.h"
#include <ctype.h>

#ifndef DEDICATED
#include "rRender.h"
#include <FTGLPixmapFont.h>
#include <FTGLBitmapFont.h>
#include <FTGLTextureFont.h>
#include <FTGLPolygonFont.h>
#include <FTGLOutlineFont.h>
#include <FTGLExtrdFont.h>
//#include <GL/gl>
//#include <SDL>
#endif

static REAL sr_bigFontThresholdWidth  = 12;
static REAL sr_bigFontThresholdHeight = 24;

static tSettingItem< REAL > sr_bigFontThresholdWidthConf(  "FONT_BIG_THRESHOLD_WIDTH", sr_bigFontThresholdWidth );
static tSettingItem< REAL > sr_bigFontThresholdHeightConf( "FONT_BIG_THRESHOLD_HEIGHT", sr_bigFontThresholdHeight );

class rFont:public rFileTexture{
    int offset;
    REAL cwidth;
    REAL cheight;
    REAL onepixel;
    rFont *lowerPart;
public:
    rFont(const char *fileName,int Offset=0,REAL CWidth=(1/16.0),
          REAL CHeight=(1/8.0),REAL onepixel=1/256.0, rFont *lower=NULL);
    rFont(const char *fileName, rFont *lower);
    virtual ~rFont();

#ifndef DEDICATED
    // displays c
    void Render(unsigned char c,REAL left,REAL top,REAL right,REAL bot);
#endif
    static rFont s_defaultFont,s_defaultFontSmall;

protected:
    virtual void ProcessImage(SDL_Surface *);       //!< process the surface before uploading it to GL
    virtual void OnSelect( bool enforce );
};
rFont::rFont(const char *fileName,int Offset,REAL CWidth,REAL CHeight,REAL op, rFont *lower):
        rFileTexture(rTextureGroups::TEX_FONT,fileName,0,0),
        offset(Offset),cwidth(CWidth),cheight(CHeight),
        onepixel(op),lowerPart(lower)
{StoreAlpha();}

rFont::rFont(const char *fileName, rFont *lower):
        rFileTexture(rTextureGroups::TEX_FONT,fileName,0,0),
        offset(0),cwidth(1/16.0),cheight(1/8.0),
        onepixel(1/256.0),lowerPart(lower)
{StoreAlpha();}

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

    if(surface->format->BytesPerPixel == 4)
    {
        for(int i=surface->w*surface->h-1;i>=0;i--){
            GLubyte alpha=pixels[4*i+3];
            pixels[4*i  ] = (alpha * pixels[4*i  ]) >> 8;
            pixels[4*i+1] = (alpha * pixels[4*i+1]) >> 8;
            pixels[4*i+2] = (alpha * pixels[4*i+2]) >> 8;
        }
    }
    else if(surface->format->BytesPerPixel == 2)
    {
        for(int i=surface->w*surface->h-1;i>=0;i--){
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
        select->Select(true);

        BeginQuads();
        glTexCoord2f(tleft,ttop);
        glVertex2f(   left, top);

        glTexCoord2f(tright,ttop);
        glVertex2f(   right ,top);

        glTexCoord2f(tright,tbot);
        glVertex2f(   right, bot);

        glTexCoord2f(tleft,tbot);
        glVertex2f(   left, bot);
        RenderEnd();
    }
}
#endif

static rFont sr_lowerPartFont("textures/font_extra.png");
rFont rFont::s_defaultFont("textures/font.png", &sr_lowerPartFont);
rFont rFont::s_defaultFontSmall("textures/font_s.png",32,5/128.0,9/128.0,1/128.0);

#ifndef DEDICATED

static int sr_fontType = 3;
static tConfItem< int > sr_fontTypeConf( "FONT_TYPE", sr_fontType, &sr_ReloadFont );

class rFontContainer : std::map<int, FTFont *> {
    FTFont &New(int size);
public:
    void clear() {
        for(iterator i = begin(); i != end(); ++i) {
            delete i->second;
        }
        std::map<int, FTFont *>::clear();
    }
    float GetWidth(tString const &str, float height) {
        switch(sr_fontType) {
        case 1:
            return height*(height*sr_screenHeight < sr_bigFontThresholdHeight ? .41 : .5)*str.size();
	    break;
	default:
            return GetFont(height).Advance(str.c_str())/sr_screenWidth*2.;
        }
    }
    void Render(tString const &str, float height, tCoord const &where) {
        if (sr_fontType != 0) {
            if(sr_fontType >= 3) {
                glPushMatrix();
                glTranslatef(where.x, where.y, .5);
                glScalef(2./sr_screenWidth, 2./sr_screenHeight, 1.);
		if(sr_fontType == 3) {
		    glEnable(GL_TEXTURE_2D);
		    glEnable(GL_BLEND);
		    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
		if(sr_fontType == 6) {
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
            if(sr_fontType >= 3) {
                glPopMatrix();
            }
        } else {
            rFont *font;
            float widthFactor;
            if (height*sr_screenHeight < sr_bigFontThresholdHeight) {
                font = &rFont::s_defaultFontSmall;
                widthFactor = .41;
            } else {
                font = &rFont::s_defaultFont;
                widthFactor = .5;
            }
            float left = where.x;
            for(tString::const_iterator i = str.begin(); i < str.end(); ++i, left += height*widthFactor) {
                //void rFont::Render(unsigned char c,REAL left,REAL top,REAL right,REAL bot){
                font->Render(*i, left, where.y+height, left+widthFactor*height, where.y);
                //F->Render(buffer[realx],l,t,l+cwidth,t-cheight);
            }
        }
    }
    FTFont &GetFont(float height) {
        int size = int(height*0.9*sr_screenHeight/2.+.5);
        if(count(size)) {
            return *((*this)[size]); //already exists
        } else {
            return New(size);
        }
    }
    void BBox(tString const &str, float height, tCoord where, float &l, float &b, float &r, float &t) {
        if(sr_fontType != 0) {
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

FTFont &rFontContainer::New(int size) {
    FTFont *font;
    tString theFontFile("");

    if(useCustomFont == 1) {
        theFontFile = customFont;
    } else {
        theFontFile = "textures/" + fontFile;
        theFontFile = tDirectories::Data().GetReadPath(theFontFile);
    }
    std::cout << "Use custom font: " << useCustomFont << std::endl;
    std::cout << "The font file: " << theFontFile << std::endl;
    switch (sr_fontType) {
    case 1:
        font = new FTGLPixmapFont(theFontFile);
        break;
    case 2:
        font = new FTGLBitmapFont(theFontFile);
        break;
    case 4:
        font = new FTGLPolygonFont(theFontFile);
        break;
    case 5:
        font = new FTGLOutlineFont(theFontFile);
        break;
    case 6:
        font = new FTGLExtrdFont(theFontFile);
	reinterpret_cast<FTGLExtrdFont *>(font)->Depth(10.);
        break;
    default:
        font = new FTGLTextureFont(theFontFile);
    }
    if(font->Error()) {
        std::cerr << "Error while loading font from path '" << theFontFile << "'. Error code: " << font->Error() << std::endl;
        //delete font;
        std::cerr << "Loading default font instead.  Sorry." << std::endl;
        switch (sr_fontType) {
        case 1:
            font = new FTGLPixmapFont(tDirectories::Data().GetReadPath("textures/Armagetronad.ttf"));
            break;
        case 2:
            font = new FTGLBitmapFont(tDirectories::Data().GetReadPath("textures/Armagetronad.ttf"));
            break;
        case 4:
            font = new FTGLPolygonFont(tDirectories::Data().GetReadPath("textures/Armagetronad.ttf"));
            break;
        case 5:
            font = new FTGLOutlineFont(tDirectories::Data().GetReadPath("textures/Armagetronad.ttf"));
            break;
        case 6:
            font = new FTGLExtrdFont(tDirectories::Data().GetReadPath("textures/Armagetronad.ttf"));
            break;
        default:
            font = new FTGLTextureFont(tDirectories::Data().GetReadPath("textures/Armagetronad.ttf"));
        }

    }
    font->FaceSize(size);
    (*this)[size] = font;
    return *font;
}
rFontContainer sr_Font;

void sr_ReloadFont(void) {
    sr_Font.clear();
}
#endif

rTextField::rTextField(REAL Left,REAL Top,
                       REAL Cwidth,REAL Cheight)
        :parIndent(0),
left(Left),top(Top),cwidth(Cwidth),cheight(Cheight),x(0),y(0),realx(0),nextx(Left),multiline(false),cursor(0),cursorPos(0){
    if (cwidth*sr_screenWidth<10)
        cwidth=10/REAL(sr_screenWidth);
    if (cheight*sr_screenHeight<18)
        cheight=18/REAL(sr_screenHeight);

    color_ = defaultColor_;

    width = int((1-Left)/cwidth);

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

// minimal tolerated values of font color before a white background is rendered
static REAL sr_minR = .5, sr_minG = .5, sr_minB =.5, sr_minTotal = .7;
tSettingItem< REAL > sr_minRConf( "FONT_MIN_R", sr_minR );
tSettingItem< REAL > sr_minGConf( "FONT_MIN_G", sr_minG );
tSettingItem< REAL > sr_minBConf( "FONT_MIN_B", sr_minB );
tSettingItem< REAL > sr_minTotalConf( "FONT_MIN_TOTAL", sr_minTotal );

void rTextField::FlushLine(int len,bool newline){
#ifndef DEDICATED
    float realTop = top-y*cheight;
    tString str(buffer.SubStr(realx, len));
    realx += len;
    if (len >= cursorPos && cursorPos >= 0) {
        cursor_y=realTop;
        cursor_x=nextx+sr_Font.GetWidth(str.SubStr(0, cursorPos), cheight);
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
        if ( r < sr_minR && g < sr_minG && b < sr_minG || r+g+b < sr_minTotal )
        {
            if ( sr_alphaBlend && !str.empty() )
            {

                glDisable(GL_TEXTURE_2D);

                glColor4f( blendColor_.r_, blendColor_.g_, blendColor_.b_, a * blendColor_.a_ );

                float l,t,r,b;

                sr_Font.BBox(str, cheight, tCoord(nextx, realTop-cheight), l, b, r, t);

                //sr_ResetRenderState(true);

                BeginQuads();
                glVertex2f(   l, t);

                glVertex2f(   r ,t);

                glVertex2f(   r, b);

                glVertex2f(   l, b);
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
    FlushLine(buffer.Size()-realx,newline);
}

inline void rTextField::WriteChar(unsigned char c)
{
    switch(c){
    case('\n'):
                    FlushLine();
        buffer.Clear();
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
    for(int i=15;i>=0;i--)
        if (hex_array[i]==c)
            ret=i;
    return ret;
}

rTextField & rTextField::StringOutput(const char * c, ColorMode colorMode)
{
#ifndef DEDICATED
    float currentWidth = nextx - left;
    float maxWidth = width * cwidth;
    bool lastIsNewline = true;
    bool trouble = false; // Do we have a word that won't fit on a line?
    // run through string
    while (*c!='\0')
    {
        if (trouble && !(*c=='0' && strlen(c)>=8 && c[1]=='x' && colorMode != COLOR_IGNORE)) {
            tString str;
            str << *c;
            currentWidth += sr_Font.GetWidth(str, cheight);
            if(isspace(*c)) {
                trouble = false;
            } else if ( currentWidth >= maxWidth) {
                WriteChar('\n');
                c++;
                currentWidth = 0.;
            }
        }
        // break line if next space character is too far away
        if ( !trouble && multiline && (isspace(*c) || lastIsNewline) )
        {
            lastIsNewline = false;
            // count number of nonblank characters following
            char const * nextSpace = c+1;
            int wordLen = 0;
            while ( *nextSpace != '\0' && !isspace(*nextSpace) )
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
            tString str(c, nextSpace);
            str = tColoredString::RemoveColors(str.c_str());
            float wordWidth = sr_Font.GetWidth(str, cheight);

            currentWidth += wordWidth;
            if ( currentWidth >= maxWidth)
            {
                WriteChar('\n');
                c++;
                if (wordWidth >= maxWidth) {
                    trouble = true;
                    currentWidth = 0.;
                } else {
                    currentWidth = wordWidth;
                }
                continue;
            }
        }
        if ( *c == '\n' ) {
            lastIsNewline = true;
            currentWidth = 0.;
        }

        //// linebreak if line has gotten too long anyway
        //if ( x >= width )
        //{
        //    WriteChar('\n');
        //    currentWidth = 0.;
        //}

        // detect presence of color code
        if (*c=='0' && strlen(c)>=8 && c[1]=='x' && colorMode != COLOR_IGNORE )
        {
            if ( 0 ==strncmp(c,"0xRESETT",8) )
            {
                // color reset to default requested
                ResetColor();
            }
            else
            {
                // found! extract colors
                FlushLine(false);

                cursorPos-=7;

                color_.r_=CTR(hex_to_int(c[2])*16+hex_to_int(c[3]));
                color_.g_=CTR(hex_to_int(c[4])*16+hex_to_int(c[5]));
                color_.b_=CTR(hex_to_int(c[6])*16+hex_to_int(c[7]));
            }

            // advance
            if ( colorMode == COLOR_USE )
            {
                c+=8;
            }
            else
            {
                c++;
            }
        }
        else
            // normal operation: add char
            WriteChar(*(c++));
    }
#endif
    return *this;
}

void DisplayText(REAL x,REAL y,REAL w,REAL h,const char *text,int center,int cursor,int cursorPos){
#ifndef DEDICATED
    float height;
    float width = rTextField::GetTextLength(tString(text), h, true, true, &height);

    // shrink fields that don't fit the screen
    REAL maxw = 1.95 - x;
    if ( width > maxw )
    {
        h *= maxw/(width);
        width=maxw;
    }

    rTextField c(x-(center+1)*width*.5,y+h*.5,w,h);
    if (center==-1)
        c.SetWidth(int((1-x)/w));
    else
        c.SetWidth(10000);

    c.SetIndent(5);
    if (cursor)
        c.SetCursor(cursor,cursorPos);
    c << text;
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

#ifndef DEDICATED
//! @param str the string to be used
//! @param height the height of one character
//! @param stripColors should colors be recognized?
//! @param useNewline should newlines be recognized (and the longest line be found)?
//! @param resultingHeight address to store the number of lines (height times the number of newlines+1)
//! @returns the width of the string if it was printed
float rTextField::GetTextLength (tString const &str, float height, bool stripColors, bool useNewline, float *resultingHeight) {
    if(stripColors) {
        return sr_Font.GetWidth(tColoredString::RemoveColors(str.c_str()), height); //TODO: Implement all the rest!
    }
    return sr_Font.GetWidth(str, height); //TODO: Implement all the rest!
}
#endif
