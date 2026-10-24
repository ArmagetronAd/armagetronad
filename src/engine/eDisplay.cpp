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

//void se_FetchAndStoreSDLInput();

#include "rSDL.h"

#include "tConfiguration.h"
#include <algorithm>

// floor mirror
#ifndef DEDICATED
static REAL sr_floorMirror_strength=.1;
static tSettingItem<REAL> f_m("FLOOR_MIRROR_INT",sr_floorMirror_strength);
static REAL sr_floorReflectionSoftness = 0;
static tSettingItem<REAL> f_soft("FLOOR_REFLECTION_SOFTNESS", sr_floorReflectionSoftness);

#include "tEventQueue.h"
#include "uInputQueue.h"
#include "eTess2.h"
#include "rTexture.h"
#include "eGameObject.h"
#include "rFont.h"
#include "eTimer.h"
#include "eCamera.h"
#include "eSensor.h"
#include "rScreen.h"
#include "rRender.h"
#include "eWall.h"
#include "eAdvWall.h"
#include "eFloor.h"
#include "ePath.h"
#include "eGrid.h"
#include "eDebugLine.h"
#include "tDirectories.h"
#include "eRectangle.h"
#include "tConsole.h"

#define eWall_h 4
#define view_h 2.7

#ifdef DEBUG
bool debug_grid=0;
#endif

REAL se_upperSkyHeight=100;
REAL se_lowerSkyHeight=50;

static tSettingItem<REAL> sec_upperSkyHeight("UPPER_SKY_HEIGHT",se_upperSkyHeight);
static tSettingItem<REAL> sec_lowerSkyHeight("LOWER_SKY_HEIGHT",se_lowerSkyHeight);

#ifndef DEDICATED

#ifdef HAVE_GLEW
namespace
{
GLuint se_floorReflectionProgram = 0;
bool se_floorReflectionFailed = false;
GLuint se_floorReflectionTexture = 0;
int se_floorReflectionWidth = 0, se_floorReflectionHeight = 0;
bool se_floorReflectionCopyFailed = false;
bool se_floorReflectionPriorErrorReported = false;

void se_UnloadFloorReflection()
{
    if (se_floorReflectionProgram)
        glDeleteProgram(se_floorReflectionProgram);
    se_floorReflectionProgram = 0;
    se_floorReflectionFailed = false;
    if (se_floorReflectionTexture)
        glDeleteTextures(1, &se_floorReflectionTexture);
    se_floorReflectionTexture = 0;
    se_floorReflectionWidth = se_floorReflectionHeight = 0;
    se_floorReflectionCopyFailed = false;
    se_floorReflectionPriorErrorReported = false;
}

static rCallbackBeforeScreenModeChange se_floorReflectionUnload(&se_UnloadFloorReflection);

GLuint se_CompileFloorShader(GLenum type, char const* source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled)
    {
        char message[1024] = "";
        glGetShaderInfoLog(shader, sizeof(message) - 1, NULL, message);
        con << "Floor material: " << message << "\n";
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool se_PrepareFloorReflection()
{
    if (se_floorReflectionFailed)
        return false;
    if (se_floorReflectionProgram)
        return true;
    char const* vertexSource =
        "#version 120\n"
        "varying vec4 eyePosition;\n"
        "varying vec3 floorNormal;\n"
        "void main() {\n"
        "    eyePosition = gl_ModelViewMatrix * gl_Vertex;\n"
        "    floorNormal = gl_NormalMatrix * vec3(0.0, 0.0, 1.0);\n"
        "    gl_Position = ftransform();\n"
        "    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;\n"
        "    gl_FrontColor = gl_Color;\n"
        "    gl_BackColor = gl_Color;\n"
        "}\n";
    char const* fragmentSource =
        "#version 120\n"
        "uniform sampler2D floorTexture;\n"
        "uniform sampler2D reflectedScene;\n"
        "uniform vec2 inverseFrame;\n"
        "uniform vec4 reflectionViewport;\n"
        "uniform float reflectionSoftness;\n"
        "uniform float capturedReflection;\n"
        "uniform float reflectance;\n"
        "uniform float gridWorld;\n"
        "uniform vec4 arenaBounds;\n"
        "varying vec4 eyePosition;\n"
        "varying vec3 floorNormal;\n"
        "float inlay(float d, float radius) {\n"
        "    float aa = max(fwidth(d), 0.008);\n"
        "    float lo = max(d - aa * 0.5, -radius);\n"
        "    float hi = min(d + aa * 0.5, radius);\n"
        "    return max(hi - lo, 0.0) / aa;\n"
        "}\n"
        "float gridLines(vec2 distance, vec2 footprint, float radius) {\n"
        // Differentiate the world coordinate before wrapping it. At a grid
        // seam, differentiating abs(mod(...)) can cancel and flash a thin line.
        "    vec2 lo = max(distance - footprint * 0.5, vec2(-radius));\n"
        "    vec2 hi = min(distance + footprint * 0.5, vec2(radius));\n"
        "    vec2 coverage = max(hi - lo, vec2(0.0)) / footprint;\n"
        "    return 1.0 - (1.0 - coverage.x) * (1.0 - coverage.y);\n"
        "}\n"
        "float roundedPanel(vec2 p, vec2 size, float radius) {\n"
        "    vec2 q = abs(p) - size + radius;\n"
        "    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - radius;\n"
        "}\n"
        "vec3 reflectionAt(vec2 uv) {\n"
        "    return texture2D(reflectedScene, clamp(uv, reflectionViewport.xy, reflectionViewport.zw)).rgb;\n"
        "}\n"
        "void main() {\n"
        // Homogeneous horizon vertices have W=0. Divide the interpolated value
        // before distance attenuation; normalizing XYZ alone is only a direction.
        "    vec3 eye = eyePosition.xyz / max(abs(eyePosition.w), 0.000001);\n"
        "    float distance = length(eye);\n"
        "    float cosine = clamp(abs(dot(normalize(eye), normalize(floorNormal))), 0.0, 1.0);\n"
        "    float fresnel = pow(1.0 - cosine, 5.0);\n"
        "    float reflected = reflectance + (1.0 - reflectance) * fresnel;\n"
        "    vec3 surface = texture2DProj(floorTexture, gl_TexCoord[0]).rgb * gl_Color.rgb;\n"
        // Projective coordinates remain valid on the homogeneous infinity plane.
        // All inlays are fixed to the floor; camera motion never drags the pattern.
        "    vec2 world = gl_TexCoord[0].xy / gl_TexCoord[0].w * gridWorld;\n"
        "    vec2 center = (arenaBounds.xy + arenaBounds.zw) * 0.5;\n"
        "    vec2 halfArena = max((arenaBounds.zw - arenaBounds.xy) * 0.5, vec2(2.0));\n"
        "    float rail = 0.0;\n"
        "    for (int i = 0; i < 3; ++i) {\n"
        "        vec2 size = max(halfArena - vec2(4.5 + float(i) * 2.8), vec2(1.0));\n"
        "        float d = roundedPanel(world - center, size, min(8.0, min(size.x, size.y) * 0.25));\n"
        "        rail += inlay(d, i == 0 ? 0.11 : 0.040) * (i == 0 ? 1.0 : 0.38);\n"
        "    }\n"
        // The glass between circuits stays black. Only the inlays emit;
        // reflected objects retain their own colours in the separate pass.
        "    vec2 layout = world - center;\n"
        "    float spacing = max(gridWorld, 4.0);\n"
        "    vec2 major = abs(mod(layout + spacing * 0.5, spacing) - spacing * 0.5);\n"
        "    vec2 minor = abs(mod(layout + spacing * 0.125, spacing * 0.25) - spacing * 0.125);\n"
        "    vec2 footprint = max(fwidth(world), vec2(0.008));\n"
        "    float panels = gridLines(major, footprint, 0.045);\n"
        "    float subdivisions = gridLines(minor, footprint, 0.018);\n"
        "    float junctions = inlay(major.x, 0.025) * (1.0 - smoothstep(0.40, 0.70, major.y));\n"
        "    junctions += inlay(major.y, 0.025) * (1.0 - smoothstep(0.40, 0.70, major.x));\n"
        "    float distanceFade = 1.0 - smoothstep(90.0, 280.0, distance);\n"
        "    float grid = panels * 0.42 + subdivisions * 0.11 + junctions * 0.35;\n"
        "    vec2 panelIndex = floor((layout + spacing * 0.5) / spacing);\n"
        "    float panelShade = fract(sin(dot(panelIndex, vec2(127.1, 311.7))) * 43758.5453);\n"
        // Broad panel variation is stable in world space. Fade it before a tile
        // becomes subpixel, preserving a quiet surface through camera movement.
        "    float panelResolved = 1.0 - smoothstep(spacing * 0.20, spacing * 0.65, max(footprint.x, footprint.y));\n"
        "    panelShade = mix(0.5, panelShade, panelResolved);\n"
        "    float roughness = 0.72 + panelShade * 0.28;\n"
        "    float roomLight = 1.0 - smoothstep(0.0, 1.4, length(layout / halfArena));\n"
        "    surface = surface * 0.04 + vec3(0.001, 0.002, 0.003) * (0.70 + panelShade * 0.30);\n"
        "    surface += vec3(0.0004, 0.0007, 0.001) * roomLight;\n"
        "    surface *= 1.0 - subdivisions * 0.24;\n"
        "    float bevel = gridLines(abs(minor - vec2(0.075)), footprint, 0.035);\n"
        "    surface += vec3(0.001, 0.002, 0.003) * bevel;\n"
        "    vec2 circuitLayout = layout;\n"
        "    circuitLayout.x += mod(floor((layout.y + 36.0) / 72.0), 2.0) * 40.0;\n"
        "    vec2 cell = mod(circuitLayout + vec2(40.0, 36.0), vec2(80.0, 72.0)) - vec2(40.0, 36.0);\n"
        "    float circuit = roundedPanel(cell, vec2(30.0, 25.0), 8.0);\n"
        "    float inner = roundedPanel(cell, vec2(22.0, 17.0), 5.0);\n"
        "    float routes = inlay(circuit, 0.11) + 0.30 * inlay(circuit + 0.75, 0.035);\n"
        "    routes += 0.38 * inlay(inner, 0.045);\n"
        "    vec3 emission = vec3(0.22, 0.65, 0.76) * (rail * 0.82 + grid * 0.46 + routes * 0.85) * distanceFade;\n"
        "    float air = 1.0 - exp(-distance * 0.003);\n"
        "    surface = mix(surface, vec3(0.0005, 0.001, 0.0015), air * 0.55);\n"
        "    reflected *= 1.0 - min(reflectionSoftness, 1.0) * roughness * 0.32 * (1.0 - fresnel);\n"
        "    float opacity = max(1.0 - reflected, air * 0.12);\n"
        "    if (capturedReflection > 0.5) {\n"
        // A compact, normalized optical filter softens only the mirrored scene.
        // The real bikes, walls and HUD are drawn later at full resolution.
        "        vec2 uv = gl_FragCoord.xy * inverseFrame;\n"
        "        float radius = reflectionSoftness * roughness * (2.0 + min(distance * 0.10, 5.0));\n"
        "        vec2 step = inverseFrame * radius / (inverseFrame.y * 1080.0);\n"
        "        vec3 reflection = reflectionAt(uv) * 0.20;\n"
        "        reflection += (reflectionAt(uv + vec2(step.x, 0.0)) + reflectionAt(uv - vec2(step.x, 0.0))) * 0.12;\n"
        "        reflection += (reflectionAt(uv + vec2(0.0, step.y)) + reflectionAt(uv - vec2(0.0, step.y))) * 0.12;\n"
        "        reflection += (reflectionAt(uv + step) + reflectionAt(uv - step)) * 0.08;\n"
        "        reflection += (reflectionAt(uv + vec2(step.x, -step.y)) + reflectionAt(uv + vec2(-step.x, step.y))) * 0.08;\n"
        "        gl_FragColor = vec4(surface * opacity + reflection * (1.0 - opacity) + emission, 1.0);\n"
        "    } else {\n"
        "        gl_FragColor = vec4(surface * opacity + emission, opacity);\n"
        "    }\n"
        "}\n";
    GLuint vertex = se_CompileFloorShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragment = se_CompileFloorShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (vertex && fragment)
    {
        se_floorReflectionProgram = glCreateProgram();
        glAttachShader(se_floorReflectionProgram, vertex);
        glAttachShader(se_floorReflectionProgram, fragment);
        glLinkProgram(se_floorReflectionProgram);
        GLint linked = GL_FALSE;
        glGetProgramiv(se_floorReflectionProgram, GL_LINK_STATUS, &linked);
        if (!linked)
        {
            char message[1024] = "";
            glGetProgramInfoLog(se_floorReflectionProgram, sizeof(message) - 1, NULL, message);
            con << "Floor material link: " << message << "\n";
            glDeleteProgram(se_floorReflectionProgram);
            se_floorReflectionProgram = 0;
        }
    }
    if (vertex)
        glDeleteShader(vertex);
    if (fragment)
        glDeleteShader(fragment);
    if (!se_floorReflectionProgram)
    {
        se_floorReflectionFailed = true;
        con << "Floor reflection: shaders unavailable; using the classic reflection.\n";
    }
    return se_floorReflectionProgram != 0;
}

// Copy the mirrored scene once for this viewport. One full-size texture is
// reused across views; sampling is clamped so split-screen players never bleed
// into each other's reflections. The driver's copy resolves window MSAA.
bool se_CopyFloorReflection(GLint const* viewport)
{
    if (se_floorReflectionCopyFailed || sr_screenWidth <= 0 || sr_screenHeight <= 0)
        return false;
    // Attribute failure only to this optional copy, not to an earlier draw.
    // Preserve an actionable diagnostic rather than silently swallowing it.
    for (int count = 0; count < 16; ++count)
    {
        GLenum prior = glGetError();
        if (prior == GL_NO_ERROR)
            break;
        if (!se_floorReflectionPriorErrorReported)
        {
            con << "Floor material: pre-existing OpenGL error " << int(prior) << " before reflection capture.\n";
            se_floorReflectionPriorErrorReported = true;
        }
    }
    if (!se_floorReflectionTexture)
        glGenTextures(1, &se_floorReflectionTexture);
    glBindTexture(GL_TEXTURE_2D, se_floorReflectionTexture);
    if (se_floorReflectionWidth != sr_screenWidth || se_floorReflectionHeight != sr_screenHeight)
    {
        GLint maximum = 0;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maximum);
        if (sr_screenWidth > maximum || sr_screenHeight > maximum)
            return false;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, sr_screenWidth, sr_screenHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        if (glGetError() != GL_NO_ERROR)
        {
            se_floorReflectionCopyFailed = true;
            con << "Floor material: reflection texture allocation failed; using the unfiltered reflection.\n";
            return false;
        }
        se_floorReflectionWidth = sr_screenWidth;
        se_floorReflectionHeight = sr_screenHeight;
    }
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, viewport[0], viewport[1], viewport[0], viewport[1], viewport[2], viewport[3]);
    if (glGetError() != GL_NO_ERROR)
    {
        se_floorReflectionCopyFailed = true;
        con << "Floor material: using the unfiltered reflection.\n";
        return false;
    }
    return true;
}
} // namespace
#endif

// The mirrored scene is already below the floor. Schlick's Fresnel response
// changes only its visibility through the polished surface: the configured
// strength is normal-incidence reflectance, increasing at grazing angles.
// Eye-space directions also support the existing homogeneous infinity plane.
class se_FloorReflection
{
public:
    explicit se_FloorReflection(REAL alpha)
    {
#ifdef HAVE_GLEW
        if (sr_hiddenWindow && se_LegacyGraphics())
        {
            static bool reported = false;
            if (!reported)
            {
                reported = true;
                std::cerr << "Floor material: mirror=" << sr_floorMirror << " alpha=" << alpha
                          << " grid=" << se_GridSize() << " fog=" << int(glIsEnabled(GL_FOG)) << '\n';
            }
        }
        if (!se_LegacyGraphics() || !sr_alphaBlend ||
            !sr_floorMirror || alpha >= 1 || !GLEW_VERSION_2_0 || glIsEnabled(GL_FOG))
            return;
        RenderEnd();
        if (!se_PrepareFloorReflection())
            return;
        glGetIntegerv(GL_CURRENT_PROGRAM, &previous_);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &previousTexture_);
        glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glActiveTexture(GL_TEXTURE1);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousReflection_);
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        bool captured = sr_floorReflectionSoftness > 0 && se_CopyFloorReflection(viewport);
        glUseProgram(se_floorReflectionProgram);
        glUniform1i(glGetUniformLocation(se_floorReflectionProgram, "floorTexture"), 0);
        glUniform1i(glGetUniformLocation(se_floorReflectionProgram, "reflectedScene"), 1);
        glUniform1f(glGetUniformLocation(se_floorReflectionProgram, "capturedReflection"), captured ? 1.f : 0.f);
        glUniform1f(glGetUniformLocation(se_floorReflectionProgram, "reflectionSoftness"),
                    std::max(REAL(0), std::min(REAL(2), sr_floorReflectionSoftness)));
        glUniform2f(glGetUniformLocation(se_floorReflectionProgram, "inverseFrame"), 1.f / sr_screenWidth, 1.f / sr_screenHeight);
        glUniform4f(glGetUniformLocation(se_floorReflectionProgram, "reflectionViewport"),
                    (viewport[0] + .5f) / sr_screenWidth, (viewport[1] + .5f) / sr_screenHeight,
                    (viewport[0] + viewport[2] - .5f) / sr_screenWidth, (viewport[1] + viewport[3] - .5f) / sr_screenHeight);
        glUniform1f(glGetUniformLocation(se_floorReflectionProgram, "reflectance"),
                    std::max(REAL(0), std::min(REAL(1), 1 - alpha)));
        glUniform1f(glGetUniformLocation(se_floorReflectionProgram, "gridWorld"), se_GridSize());
        eRectangle const& bounds = eWallRim::GetBounds();
        glUniform4f(glGetUniformLocation(se_floorReflectionProgram, "arenaBounds"),
                    bounds.GetLow().x, bounds.GetLow().y, bounds.GetHigh().x, bounds.GetHigh().y);
        glActiveTexture(GL_TEXTURE0);
        active_ = true;
#else
        (void)alpha;
#endif
    }
    ~se_FloorReflection()
    {
#ifdef HAVE_GLEW
        if (active_)
        {
            RenderEnd();
            glUseProgram(previous_);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, previousReflection_);
            glPopAttrib();
            glActiveTexture(previousTexture_);
        }
#endif
    }

private:
#ifdef HAVE_GLEW
    GLint previous_ = 0;
    GLint previousTexture_ = GL_TEXTURE0, previousReflection_ = 0;
    bool active_ = false;
#endif
};

// select the lower sky
static rFileTexture & se_Sky()
{
    static char const * skyPath="textures/sky.png";
    static char const * skyPathMoviepack="moviepack/sky.png";
    static rFileTexture sky(rTextureGroups::TEX_FLOOR,skyPath,1,1,true);
    static rFileTexture sky_moviepack(rTextureGroups::TEX_FLOOR,skyPathMoviepack,1,1,true);

    if (se_MoviePack())
    {
        // Since old movie packs usually don't include sky.png we need to
        // be nice and fall back to the default sky tecture. -k
        tString s = tDirectories::Data().GetReadPath( skyPathMoviepack );
        if(strlen(s) > 0)
            return sky_moviepack;
    }

    return sky;
}

static void se_SelectSky()
{
    static rFileTexture & sky = se_Sky();
    sky.Select();
}

static REAL se_upperSkyScale=1;
static REAL se_upperSkyColorR=.5;
static REAL se_upperSkyColorG=.5;
static REAL se_upperSkyColorB=1;
static tSettingItem<REAL> sec_upperSkyScale("UPPER_SKY_SCALE",se_upperSkyScale);
static tSettingItem<REAL> sec_upperSkyColorR("UPPER_SKY_RED",se_upperSkyColorR);
static tSettingItem<REAL> sec_upperSkyColorG("UPPER_SKY_GREEN",se_upperSkyColorG);
static tSettingItem<REAL> sec_upperSkyColorB("UPPER_SKY_BLUE",se_upperSkyColorB);

// select the upper sky
static rFileTexture * se_UpperSky()
{
    static char const * skyPath="textures/upper_sky.png";
    static char const * skyPathMoviepack="moviepack/upper_sky.png";
    static rFileTexture sky(rTextureGroups::TEX_FLOOR,skyPath,1,1,true);
    static rFileTexture sky_moviepack(rTextureGroups::TEX_FLOOR,skyPathMoviepack,1,1,true);

    if (se_MoviePack())
    {
        tString s = tDirectories::Data().GetReadPath( skyPathMoviepack );
        if(s.Len() > 1)
            return &sky_moviepack;
    }

    if( tDirectories::Data().GetReadPath( skyPath ).Len() > 1 ){
        return &sky;
    }

    return NULL;
}

static void se_SelectUpperSky()
{
    static rFileTexture * sky = se_UpperSky();
    if( sky )
    {
        sky->Select();
    }
    else
    {
        se_glFloorTexture();
    }
}

// if the rip bug is activated, don't use the rim to draw the floor
extern short se_bugRip;

// passes a vertex with z-projected texture coordinates to OpenGL
static inline void TexVertex( REAL x, REAL y, REAL h)
{
    glTexCoord2f(x, y);
    glVertex3f  (x, y, h);
}

// renders a finite rectangle
static void finite_xy_plane( const eCoord &pos,const eCoord &dir,REAL h, eRectangle rect )
{
    // expand plane to camera position to avoid embarrasing reflection bug
    if ( sr_floorMirror )
        rect.Include( pos );

    // fetch rectangle coordinates
    REAL lx = rect.GetLow().x;
    REAL ly = rect.GetLow().y;
    REAL hx = rect.GetHigh().x;
    REAL hy = rect.GetHigh().y;

    // draw rectangle as triangle fan (good for avoiding artefacts near pos)
    BeginTriangleFan();
    TexVertex( pos.x-dir.x, pos.y-dir.y, h );
    TexVertex(lx, ly, h);
    TexVertex(lx, hy, h);
    TexVertex(hx, hy, h);
    TexVertex(hx, ly, h);
    TexVertex(lx, ly, h);
    RenderEnd();
}

static void infinity_xy_plane(eCoord const & pos, const eCoord &dir,REAL h=0){
    bool use_rim=false;
    REAL zero=0;

    if (sr_highRim)
        use_rim=true;

    if ( se_bugRip )
        use_rim=false;

    // always use the rim if infinity rendering is turned off
    use_rim |= !sr_infinityPlane;

    if (use_rim){
        /*
          // the rim wall based rendering does not work properly for shaped arenas, so
          // it's been replaced.

                BeginTriangles();
                for(int i=se_rimWalls.Len()-1;i>=0;i--){
                    eCoord p1=se_rimWalls(i)->EndPoint(0);
                    eCoord p2=se_rimWalls(i)->EndPoint(1);

                    glTexCoord2f(pos.x, pos.y);
                    glVertex3f  (pos.x, pos.y, h);

                    glTexCoord2f(p1.x, p1.y);
                    glVertex3f  (p1.x, p1.y, h);

                    glTexCoord2f(p2.x, p2.y);
                    glVertex3f  (p2.x, p2.y, h);
                }
                RenderEnd();
        */
        finite_xy_plane( pos, dir, h, eWallRim::GetBounds() );
    }
    else
    {
        if (!sr_infinityPlane)
            zero=.001;

        BeginTriangleFan();

        glTexCoord4f(pos.x-dir.x, pos.y-dir.y, h, 1);
        glVertex4f  (pos.x-dir.x, pos.y-dir.y, h, 1);

        glTexCoord4f(1,0.1,zero*h,zero);
        glVertex4f  (1,0.1,zero*h,zero);

        glTexCoord4f(0.1,1.1,zero*h,zero);
        glVertex4f  (0.1,1.1,zero*h,zero);

        glTexCoord4f(-1,0.1,zero*h,zero);
        glVertex4f  (-1,0.1,zero*h,zero);


        glTexCoord4f(0.1,-1.1,zero*h,zero);
        glVertex4f  (0.1,-1.1,zero*h,zero);

        glTexCoord4f(1,0.1,zero*h,zero);
        glVertex4f  (1,0.1,zero*h,zero);

        RenderEnd();
    }
}

static REAL z=0;

/* Try to get ride of these functions as it seems useless to use them instead of the cam parameter itself
int           eGrid::NumberOfCameras(){return cameras.Len();}
const eCoord& eGrid::CameraPos(int i){return cameras(i)->CameraPos();}
eCoord eGrid::CameraGlancePos(int i){return cameras(i)->CameraGlancePos();}
const eCoord& eGrid::CameraDir(int i){return cameras(i)->CameraDir();}
REAL          eGrid::CameraHeight(int i){return cameras(i)->CameraZ();}
*/



class eZNearSensor: public eSensor
{
public:
    static bool AdaptZNear( REAL & zNear, eWall const * wall, eCamera const * camera )
    {
        REAL len = wall->Len();
        if ( len > .01)
        {
            REAL zDist = ::z - wall->Height();
            if ( zDist < zNear )
            {
                const eCoord& camPos = camera->CameraPos();
                const eCoord& camDir = camera->CameraDir();
                eCoord base = wall->EndPoint(0);
                eCoord end = wall->EndPoint(1);
                
                if ( eCoord::F( base-camPos, camDir ) > 0.01f || eCoord::F( end-camPos, camDir ) > 0.01f )
                {
                    eCoord dirNorm = end - base;
                    dirNorm.Normalize();
                    eCoord camRelative = ( camPos - base ).Turn( dirNorm.Conj() );
                    REAL dist = fabs( camRelative.y );
                    if ( camRelative.x < 0 )
                    {
                        dist -= camRelative.x;
                    }
                    if ( camRelative.x > len )
                    {
                        dist += camRelative.x - len;
                    }
                    if ( dist < zDist )
                    {
                        dist = zDist;
                    }
                    // TODO: better criterion for ingoring of walls
                    if ( dist < zNear && dist > 0.001f )
                    {
                        zNear = dist;
                        return true;
                    }
                }
            }
        }

        return false;
    }

    eZNearSensor(eGameObject *o,const eCoord &start,const eCoord &d, REAL & zNear, eCamera const * camera )
    :eSensor( o, start, d ), zNear_( zNear ), camera_( camera )
    {}

    void Detect()
    {
        detect( zNear_ * 2 );
    }

    // called when passing an edge
    ePassEdgeResult PassEdge( const eWall * w, REAL time, REAL, int) override
    {
        // adapt zNear and be done
        if( AdaptZNear( zNear_, w, camera_ ) )
        {
            return eAbort;
        }

        return eContinue;
    }
private:
    REAL & zNear_; // the reference to the near clipping plane value
    eCamera const * camera_; // the camera currently in use
};

void paint_sr_lowerSky(eGrid *grid, int viewer,bool sr_upperSky, eCamera* cam ){
    TexMatrix();
    glLoadIdentity();
    glScalef(.005,.005,.005);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);

    if (sr_skyWobble){
        glTranslatef(se_GameTime()*.1,se_GameTime()*.07145,0);
        glScalef(1+.2*sin(se_GameTime()),1+.1*cos(se_GameTime()),1);
        glTranslatef(-300,-200,0);
    }

    se_SelectSky();

    REAL sa=(se_lowerSkyHeight-z)*.1;
    if (sa>1) sa=1;
    if (!sr_upperSky){
        sa=1;
        glBlendFunc(GL_SRC_ALPHA,GL_ZERO);
    }
    if (sa>0){
        glColor4f(1,1,1,sa);
        infinity_xy_plane(cam->CameraPos(),cam->CameraDir(),se_lowerSkyHeight);
    }
    if (!sr_upperSky && sr_alphaBlend)
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
}

void eGrid::display_simple( eCamera* cam, int viewer,bool floor,
                            bool sr_upperSky,bool sr_lowerSky,
                            REAL flooralpha,
                            bool eWalls,bool gameObjects,
                            REAL& zNear){
    sr_CheckGLError();

    /*
    static GLfloat S[]={1,0,0,0};
    static GLfloat T[]={0,1,0,0};
    static GLfloat R[]={0,0,1,0};
    static GLfloat Q[]={0,0,0,1};

    glTexGeni(GL_S,GL_TEXTURE_GEN_MODE,GL_OBJECT_LINEAR);
    glTexGenfv(GL_S,GL_OBJECT_PLANE,S);

    glTexGeni(GL_T,GL_TEXTURE_GEN_MODE,GL_OBJECT_LINEAR);
    glTexGenfv(GL_T,GL_OBJECT_PLANE,T);

    glTexGeni(GL_R,GL_TEXTURE_GEN_MODE,GL_OBJECT_LINEAR);
    glTexGenfv(GL_R,GL_OBJECT_PLANE,R);

    glTexGeni(GL_Q,GL_TEXTURE_GEN_MODE,GL_OBJECT_LINEAR);
    glTexGenfv(GL_Q,GL_OBJECT_PLANE,Q);

    glDisable(GL_TEXTURE_GEN_T);
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_R);
    glDisable(GL_TEXTURE_GEN_Q);
    */


    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    glDisable(GL_CULL_FACE);

    eCoord camPos = cam->CameraGlancePos(); 
    // eWallRim::Bound( camPos, 10 );

    if (sr_upperSky || se_BlackSky()){
        if (se_BlackSky()){
            //glDisable(GL_TEXTURE);
            glDisable(GL_TEXTURE_2D);

            glColor3f(0,0,0);

            if ( z < se_lowerSkyHeight )
                infinity_xy_plane(cam->CameraPos(), cam->CameraDir(), se_lowerSkyHeight);

            glEnable(GL_TEXTURE_2D);
        }
        else {
            TexMatrix();
            glLoadIdentity();
            glScalef(se_upperSkyScale,se_upperSkyScale,se_upperSkyScale);

            se_SelectUpperSky();

            glColor3f(se_upperSkyColorR,se_upperSkyColorG,se_upperSkyColorB);

            if ( z < se_upperSkyHeight )
                infinity_xy_plane(cam->CameraPos(), cam->CameraDir(), se_upperSkyHeight);
        }
    }

    if (sr_lowerSky && !sr_highRim){
        paint_sr_lowerSky(this, viewer,sr_upperSky, cam);
    }

    if (floor){
        sr_DepthOffset(false);

        su_FetchAndStoreSDLInput();
        int floorDetail = sr_floorDetail;

        // no multitexturing without alpha blending
        if ( !sr_alphaBlend && floorDetail > rFLOOR_TEXTURE )
            floorDetail = rFLOOR_TEXTURE;

        switch(floorDetail){
        case rFLOOR_OFF:
            break;
        case rFLOOR_GRID:
            {
	#define SIDELEN   (se_GridSize())
	#define EXTENSION 10

                eCoord center = cam->CameraPos() + cam->CameraDir() * (SIDELEN * EXTENSION * .8);

                REAL x=center.x;
                REAL y=center.y;
                int xn=static_cast<int>(x/SIDELEN);
                int yn=static_cast<int>(y/SIDELEN);


                //glDisable(GL_TEXTURE);
                glDisable(GL_TEXTURE_2D);

	#define INTENSITY(x,xx) (1-(((x)-(xx))*((x)-(xx))/(EXTENSION*SIDELEN*EXTENSION*SIDELEN)))


                BeginLines();
                for(int i=xn-EXTENSION;i<=xn+EXTENSION;i++){
                    REAL intens=INTENSITY(i*SIDELEN,x);
                    if (intens<0) intens=0;
                    se_glFloorColor(intens,intens);
                    glVertex2f(i*SIDELEN,y-SIDELEN*(EXTENSION+1));
                    glVertex2f(i*SIDELEN,y+SIDELEN*(EXTENSION+1));
                }
                for(int j=yn-EXTENSION;j<=yn+EXTENSION;j++){
                    REAL intens=INTENSITY(j*SIDELEN,y);
                    if (intens<0) intens=0;
                    se_glFloorColor(intens,intens);
                    glVertex2f(x-(EXTENSION+1)*SIDELEN,j*SIDELEN);
                    glVertex2f(x+(EXTENSION+1)*SIDELEN,j*SIDELEN);
                }
                RenderEnd();
            }
            break;

        case rFLOOR_TEXTURE:
        {
            TexMatrix();
            glLoadIdentity();
            glScalef(1/se_GridSize(),1/se_GridSize(),1.);

            se_glFloorTexture();
            se_glFloorColor(flooralpha);

            se_FloorReflection reflection(flooralpha);
            infinity_xy_plane( cam->CameraPos(), cam->CameraDir()); 

            /* old way: draw every triangle
            for(int i=eFace::faces.Len()-1;i>=0;i--){
            eFace *f=eFace::faces(i);

            if (f->visHeight[viewer]<z){
            glBegin(GL_TRIANGLES);
            for(int j=0;j<=2;j++){
            glVertex3f(f->p[j]->x,f->p[j]->y,0);
            }
            glEnd();
            }
            }
            */

            break;
        }

        case rFLOOR_TWOTEXTURE:
            se_glFloorColor(flooralpha);

            TexMatrix();
            glLoadIdentity();
            REAL gs = 1/se_GridSize();
            glScalef(0.01*gs,gs,1.);

            se_glFloorTexture_a();
            infinity_xy_plane( cam->CameraPos(), cam->CameraDir()); 

            se_glFloorColor(flooralpha);

            TexMatrix();
            glLoadIdentity();
            glScalef(gs,.01*gs,1.);

            se_glFloorTexture_b();

            glDepthFunc(GL_LEQUAL);
            glBlendFunc(GL_SRC_ALPHA,GL_ONE);
            infinity_xy_plane( cam->CameraPos(), cam->CameraDir() );
            glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

            break;
        }
    }

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
	
    TexMatrix();
    glLoadIdentity();
    ModelMatrix();

    //  glDisable(GL_TEXTURE_GEN_S);
    //  glDisable(GL_TEXTURE_GEN_T);
    //  glDisable(GL_TEXTURE_GEN_Q);
    //  glDisable(GL_TEXTURE_GEN_R);

    if(eWalls){
        {
            su_FetchAndStoreSDLInput();
    
            eWallRim::RenderAll( cam );
        }

        if (sr_lowerSky && sr_highRim){
            //      glEnable(GL_TEXTURE_GEN_S);
            //      glEnable(GL_TEXTURE_GEN_T);
            //      glEnable(GL_TEXTURE_GEN_Q);
            //      glEnable(GL_TEXTURE_GEN_R);

            paint_sr_lowerSky(this, viewer,sr_upperSky, cam);

            //      glDisable(GL_TEXTURE_GEN_S);
            //      glDisable(GL_TEXTURE_GEN_T);
            //      glDisable(GL_TEXTURE_GEN_Q);
            //      glDisable(GL_TEXTURE_GEN_R);

            TexMatrix();
            glLoadIdentity();
            ModelMatrix();
        }
    }

    sr_CheckGLError();

    if (eWalls){
        // send out sensors to find walls close to the camera
        if ( z < 3 )
        {
            eCamera const * camera = cam;
            if( camera && camera->Center() )
            {
                eCoord dir = camera->CameraDir().Turn(1,.5);
                for(int i = 8; i > 0; --i)
                {
                    dir = dir.Turn(sqrt(.5),sqrt(.5));
                    eZNearSensor s( camera->Center(), camPos, dir, zNear, camera );
                    s.Detect();
                }
            }
        }

        // glDisable(GL_CULL_FACE);
        // draw_eWall(this,viewer,0,zNear,cam);

        /*
        #ifdef DEBUG
        for(int i=sg_netPlayerWalls.Len()-1;i>=0;i--){
          glMatrixMode(GL_MODELVIEW);
          glPushMatrix();
          if (sg_netPlayerWalls(i)->Preliminary())
        glTranslatef(0,0,4);
          else
        glTranslatef(0,0,8);
          if (sg_netPlayerWalls(i)->Wall())
        sg_netPlayerWalls(i)->Wall()->RenderList(false);
          glPopMatrix();
          }
        #endif
        */

        /*
        static int oldlen=0;
        int newlen=sg_netPlayerWalls.Len();
        if (newlen!=oldlen){
          con << "Number of player eWalls now " << newlen << '\n';
          oldlen=newlen;
        }
        */

    }

    sr_CheckGLError();

    if (gameObjects)
        eGameObject::RenderAll(this, cam);

    eDebugLine::Render();
#ifdef DEBUG

    ePath::RenderLast();

    if (debug_grid){
        //glDisable(GL_TEXTURE);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_LIGHTING);
        BeginLines();

        int i;
        for(i=edges.Len()-1;i>=0;i--){
            eHalfEdge *e=edges[i];
            if (e->Face())
                glColor4f(1,1,1,1);
            else
                glColor4f(0,0,1,1);

            glVertex3f(e->Point()->x,e->Point()->y,10);
            glVertex3f(e->Point()->x,e->Point()->y,15);
            glVertex3f(e->Point()->x,e->Point()->y,.1);
            glVertex3f(e->other->Point()->x,e->other->Point()->y,.1);
            glVertex3f(e->other->Point()->x,e->other->Point()->y,10);
            glVertex3f(e->other->Point()->x,e->other->Point()->y,15);

        }

        for(i=points.Len()-1;i>=0;i--){
            ePoint *p=points[i];
            glColor4f(1,0,0,1);
            glVertex3f(p->x,p->y,0);
            glVertex3f(p->x,p->y,(p->GetRefcount()+1)*5);
        }
        /*
        for(int i=sg_netPlayerWalls.Len()-1;i>=0;i--){
          eEdge *e=sg_netPlayerWalls[i]->Edge();
        glColor4f(0,1,0,1);

          glVertex3f(e->Point()->x,e->Point()->y,5);
          glVertex3f(e->Point()->x,e->Point()->y,10);
          glVertex3f(e->Point()->x,e->Point()->y,10);
          glVertex3f(e->other->Point()->x,e->other->Point()->y,10);
          glVertex3f(e->other->Point()->x,e->other->Point()->y,10);
          glVertex3f(e->other->Point()->x,e->other->Point()->y,5);
        }
        */
        RenderEnd();
    }
#endif

}
#endif

void eGrid::Render( eCamera* cam, int viewer, REAL& zNear ){
    if (!sr_glOut)
        return;
#ifndef DEDICATED
    ProjMatrix();

    z=cam->CameraZ();
    if ( zNear > z )
    {
        zNear = z;
    }

	if (sr_floorMirror){
		ModelMatrix();
		glScalef(1,1,-1);

		if (z>10) z=10;
		glFrontFace((cam->MirrorView())?GL_CCW:GL_CW);

		bool us=false;
		bool ls=false;

		if (sr_floorMirror>=rMIRROR_ALL){
			us=sr_upperSky;
			ls=sr_lowerSky;
		}
		else if (sr_floorMirror>=rMIRROR_WALLS){
			if (sr_lowerSky)
				ls=true;
			else if (sr_upperSky)
				us=true;
		}

		cam->SetRenderingMain(false && cam->CameraMain());
		display_simple(cam, viewer,false,
					   us,ls,
					   0,
					   sr_floorMirror>=rMIRROR_WALLS,
					   sr_floorMirror>=rMIRROR_OBJECTS,
					   zNear);
		z=cam->CameraZ();
		glFrontFace((cam->MirrorView())?GL_CW:GL_CCW);
		ModelMatrix();
		glScalef(1,1,-1);


		cam->SetRenderingMain(true && cam->CameraMain());
		display_simple(cam, viewer,true,
					   sr_upperSky,sr_lowerSky,
					   1-sr_floorMirror_strength,
					   true,true,zNear);

	}
	else
	{
		glFrontFace((cam->MirrorView())?GL_CW:GL_CCW);
		cam->SetRenderingMain(true && cam->CameraMain());
		display_simple(cam, viewer,true,
					   sr_upperSky,sr_lowerSky,
					   1,
					   true,true,zNear);
	}
	
#ifdef EVENT_DEB
    //  for(int i=eEdge_crossing.Len()-1;i>=0;i--){
    //    eEdge_crossing(i)->Render();
    //  }
#endif
#endif
}


//void eEdgeViewer::Render(){}

/*
void eViewerCrossesEdge::Render(){
#ifndef DEDICATED
  ePoint *p1=e->Point();
  ePoint *p2=e->other->Point();

  REAL timeLeft=value-se_GameTime();

  REAL h;

  if (viewer==1){
    if (timeLeft>0){
      h=timeLeft+4;
      glColor4f(0,0,1,.5);
    }
    else{
      h=-timeLeft+4;
      glColor4f(1,0,0,.5);
    }

    //  else
    //glColor4f(1,0,0,.5);

    static rTexture ArmageTron_invis_eWall(rTEX_WALL,"textures/eWall2.png",1,0);
    
    ArmageTron_invis_eWall.Select();
    
    eWall::Render_helper(e,(p1->x+p1->y)/4,(p2->x+p2->y)/4,h,1,4);
  }
#endif
}
*/

#endif


