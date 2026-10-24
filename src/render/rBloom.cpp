/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2026, Romil Bijarnia

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

#include "rBloom.h"

#include "aa_config.h"

#ifndef DEDICATED

// this file issues raw OpenGL calls on purpose
#define DONTDOIT
#include "rRender.h"
#include "rGL.h"
#include "rScreen.h"
#include "tConfiguration.h"
#include "tConsole.h"

// Keep a narrow glow around the light trails and a weaker, wider halo. Both are
// blurred below screen resolution; the scene and its HUD remain sharp. Settings:
//   BLOOM            on or off
//   BLOOM_INTENSITY  how much glow is added (0 disables)
//   BLOOM_THRESHOLD  brightness (max of r, g, b) from which a pixel glows
//   BLOOM_RADIUS     glow width, relative to a 1080-pixel-tall screen
//   BLOOM_HALO       strength of the wide halo relative to the narrow glow
//   BLOOM_VIGNETTE   how much the corners of the picture darken (0 disables)
static bool sr_bloom = false;
static REAL sr_bloomIntensity = 1.2;
static REAL sr_bloomThreshold = .5;
static REAL sr_bloomRadius = .8;
static REAL sr_bloomHalo = .25;
static REAL sr_bloomVignette = 0;

static tSettingItem<bool> sr_bloomConf("BLOOM", sr_bloom);
static tSettingItem<REAL> sr_bloomIntensityConf("BLOOM_INTENSITY", sr_bloomIntensity);
static tSettingItem<REAL> sr_bloomThresholdConf("BLOOM_THRESHOLD", sr_bloomThreshold);
static tSettingItem<REAL> sr_bloomRadiusConf("BLOOM_RADIUS", sr_bloomRadius);
static tSettingItem<REAL> sr_bloomHaloConf("BLOOM_HALO", sr_bloomHalo);
static tSettingItem<REAL> sr_bloomVignetteConf("BLOOM_VIGNETTE", sr_bloomVignette);

#ifdef HAVE_GLEW

namespace
{

//! a colour texture that can be drawn into
struct Target
{
    GLuint texture = 0;
    GLuint framebuffer = 0;
    int width = 0, height = 0;
};

GLuint sceneTexture = 0; // copy of the finished scene, screen size
int sceneWidth = 0, sceneHeight = 0;
Target glowTarget, scratchTarget;     // narrow glow, normally a quarter of the screen
Target haloTarget, haloScratchTarget; // wide halo, half the narrow glow's resolution
GLuint brightProgram = 0, blurProgram = 0, compositeProgram = 0;
GLint brightTexel = -1, brightThreshold = -1, brightTaps = -1, blurStep = -1;
GLint compositeIntensity = -1, compositeHalo = -1, compositeVignette = -1, compositeDither = -1;
bool failed = false;   // after an error the pass stays off until the next screen mode change
bool reported = false; // the reason is printed once

char const* vertexSource =
    "#version 120\n"
    "void main()\n"
    "{\n"
    "    gl_Position = gl_Vertex;\n"
    "    gl_TexCoord[0] = gl_MultiTexCoord0;\n"
    "}\n";

// taps x taps bilinear taps, two screen pixels apart, cover every screen pixel behind
// one glow pixel; each tap is thresholded on its own so thin bright lines survive
char const* brightSource =
    "#version 120\n"
    "uniform sampler2D scene;\n"
    "uniform vec2 texel;\n"
    "uniform float threshold;\n"
    "uniform int taps;\n"
    "vec3 bright( vec2 uv )\n"
    "{\n"
    "    vec3 c = texture2D( scene, uv ).rgb;\n"
    "    float m = max( c.r, max( c.g, c.b ) );\n"
    "    return c * smoothstep( threshold * .7, threshold, m );\n"
    "}\n"
    "void main()\n"
    "{\n"
    "    vec2 uv = gl_TexCoord[0].xy;\n"
    "    float start = -float( taps - 1 );\n"
    "    vec3 sum = vec3( 0.0 );\n"
    "    for ( int y = 0; y < taps; ++y )\n"
    "        for ( int x = 0; x < taps; ++x )\n"
    "            sum += bright( uv + texel * vec2( start + 2.0 * float( x ), start + 2.0 * float( y ) ) );\n"
    "    gl_FragColor = vec4( sum / float( taps * taps ), 1.0 );\n"
    "}\n";

// Nine-tap Gaussian, with adjacent taps combined by bilinear filtering: only five
// texture fetches per pixel. The centre stays much tighter than repeated blurs.
char const* blurSource =
    "#version 120\n"
    "uniform sampler2D source;\n"
    "uniform vec2 step;\n"
    "vec3 pair( vec2 uv, float i )\n"
    "{\n"
    "    return texture2D( source, uv + step * i ).rgb + texture2D( source, uv - step * i ).rgb;\n"
    "}\n"
    "void main()\n"
    "{\n"
    "    vec2 uv = gl_TexCoord[0].xy;\n"
    "    vec3 c = texture2D( source, uv ).rgb * .227027;\n"
    "    c += pair( uv, 1.384615 ) * .316216;\n"
    "    c += pair( uv, 3.230769 ) * .070270;\n"
    "    gl_FragColor = vec4( c, 1.0 );\n"
    "}\n";

// Add glow to the captured scene before applying one scalar highlight shoulder.
// Independent RGB curves and screen blending shift saturated blue toward cyan
// and violet toward pink. Scaling the whole vector retains its colour ratios.
char const* compositeSource =
    "#version 120\n"
    "uniform sampler2D scene;\n"
    "uniform sampler2D glow;\n"
    "uniform sampler2D halo;\n"
    "uniform float intensity;\n"
    "uniform float haloStrength;\n"
    "uniform float vignette;\n"
    "uniform float ditherStrength;\n"
    "void main()\n"
    "{\n"
    "    vec2 uv = gl_TexCoord[0].xy;\n"
    "    vec3 base = texture2D( scene, uv ).rgb;\n"
    "    vec3 energy = texture2D( glow, uv ).rgb;\n"
    "    energy += texture2D( halo, uv ).rgb * haloStrength;\n"
    "    vec3 colour = base + energy * intensity;\n"
    "    float peak = max( colour.r, max( colour.g, colour.b ) );\n"
    "    float shoulder = peak;\n"
    "    if ( peak > 0.75 ) shoulder = 0.75 + 0.25 * ( 1.0 - exp( -( peak - 0.75 ) * 4.0 ) );\n"
    // Keep the brightness and chroma of bright source cores. Otherwise blue
    // halo falling on a white headlight would turn the headlight itself blue.
    // Include the coloured shoulders of bike inlays and ribbon edges, not just
    // near-white peaks: these surfaces must retain their own colour at overlaps.
    "    float basePeak = max( base.r, max( base.g, base.b ) );\n"
    "    shoulder = max( shoulder, basePeak );\n"
    "    vec3 mapped = colour * ( shoulder / max( peak, 0.00001 ) );\n"
    "    vec3 core = base * ( shoulder / max( basePeak, 0.00001 ) );\n"
    "    vec3 result = mix( mapped, core, smoothstep( 0.55, 0.85, basePeak ) );\n"
    "    float resultPeak = max( result.r, max( result.g, result.b ) );\n"
    // The frame falls off towards its corners, the centre stays untouched.
    "    float falloff = 1.0 - vignette * smoothstep( 0.45, 1.05, length( ( uv - 0.5 ) * 2.0 ) * 0.78 );\n"
    // Sub-LSB, stationary monochrome dither smooths 8-bit dark gradients after
    // bloom without softening edges or adding temporal noise. DITHER controls it.
    "    float noise = fract(52.9829189 * fract(dot(gl_FragCoord.xy, vec2(0.06711056, 0.00583715)))) - 0.5;\n"
    "    vec3 outputColour = result * ( shoulder / max( resultPeak, 0.00001 ) ) * falloff;\n"
    "    gl_FragColor = vec4(outputColour + noise * ditherStrength / 255.0, 1.0);\n"
    "}\n";

// Program and framebuffer bindings are not part of glPushAttrib. Saving before
// Prepare also covers allocation and shader failures on the first frame.
class BloomState
{
public:
    BloomState()
    {
        glGetIntegerv(GL_CURRENT_PROGRAM, &program_);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture_);
        glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &framebuffer_);
        separateRead_ = GLEW_EXT_framebuffer_blit;
        if (separateRead_)
            glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING_EXT, &readFramebuffer_);
        glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_VIEWPORT_BIT | GL_TEXTURE_BIT | GL_CURRENT_BIT | GL_POLYGON_BIT);
        glActiveTexture(GL_TEXTURE0);
    }

    ~BloomState()
    {
        glUseProgram(program_);
        if (separateRead_)
        {
            glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, framebuffer_);
            glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, readFramebuffer_);
        }
        else
            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, framebuffer_);
        glPopAttrib();
        glActiveTexture(activeTexture_);
    }

    void BindScene() const
    {
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, framebuffer_);
    }

private:
    GLint program_, activeTexture_, framebuffer_, readFramebuffer_;
    bool separateRead_;
};

void Fail(char const* reason)
{
    failed = true;
    if (!reported)
    {
        reported = true;
        con << "Bloom: " << reason << ", the glow is off.\n";
    }
}

GLuint Compile(GLenum type, char const* source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        char log[1024] = "";
        glGetShaderInfoLog(shader, sizeof(log) - 1, NULL, log);
        con << "Bloom: shader did not compile: " << log << "\n";
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint Link(char const* fragmentSource)
{
    GLuint vertex = Compile(GL_VERTEX_SHADER, vertexSource);
    GLuint fragment = Compile(GL_FRAGMENT_SHADER, fragmentSource);
    GLuint program = 0;
    if (vertex && fragment)
    {
        program = glCreateProgram();
        glAttachShader(program, vertex);
        glAttachShader(program, fragment);
        glLinkProgram(program);
        GLint ok = 0;
        glGetProgramiv(program, GL_LINK_STATUS, &ok);
        if (!ok)
        {
            char log[1024] = "";
            glGetProgramInfoLog(program, sizeof(log) - 1, NULL, log);
            con << "Bloom: shader did not link: " << log << "\n";
            glDeleteProgram(program);
            program = 0;
        }
    }
    // the program keeps the shaders it needs alive
    if (vertex)
        glDeleteShader(vertex);
    if (fragment)
        glDeleteShader(fragment);
    return program;
}

void Allocate(GLuint texture, int width, int height)
{
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void Destroy(Target& target)
{
    if (target.framebuffer)
        glDeleteFramebuffersEXT(1, &target.framebuffer);
    if (target.texture)
        glDeleteTextures(1, &target.texture);
    target = Target();
}

bool Create(Target& target, int width, int height)
{
    Destroy(target);
    glGenTextures(1, &target.texture);
    Allocate(target.texture, width, height);
    glGenFramebuffersEXT(1, &target.framebuffer);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, target.framebuffer);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, target.texture, 0);
    bool complete = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) == GL_FRAMEBUFFER_COMPLETE_EXT;
    target.width = width;
    target.height = height;
    return complete;
}

void Quad()
{
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);
    glVertex2f(-1, -1);
    glTexCoord2f(1, 0);
    glVertex2f(1, -1);
    glTexCoord2f(1, 1);
    glVertex2f(1, 1);
    glTexCoord2f(0, 1);
    glVertex2f(-1, 1);
    glEnd();
}

// frees everything; the next frame builds it again for the new context
void Unload()
{
    if (brightProgram)
        glDeleteProgram(brightProgram);
    if (blurProgram)
        glDeleteProgram(blurProgram);
    if (compositeProgram)
        glDeleteProgram(compositeProgram);
    brightProgram = blurProgram = compositeProgram = 0;
    brightTexel = brightThreshold = brightTaps = blurStep = -1;
    compositeIntensity = compositeHalo = compositeVignette = compositeDither = -1;
    if (sceneTexture)
        glDeleteTextures(1, &sceneTexture);
    sceneTexture = 0;
    sceneWidth = sceneHeight = 0;
    Destroy(glowTarget);
    Destroy(scratchTarget);
    Destroy(haloTarget);
    Destroy(haloScratchTarget);
    failed = false;
    reported = false;
}

// A quarter-size core retains thin trails on Retina displays. Small windows need
// half-size buffers so the glow does not turn into visibly enlarged pixels.
int Shrink(int height)
{
    return height < 720 ? 2 : 4;
}

bool Prepare(int width, int height)
{
    if (!brightProgram)
    {
        brightProgram = Link(brightSource);
        if (!brightProgram)
            return false;
        glUseProgram(brightProgram);
        glUniform1i(glGetUniformLocation(brightProgram, "scene"), 0);
        brightTexel = glGetUniformLocation(brightProgram, "texel");
        brightThreshold = glGetUniformLocation(brightProgram, "threshold");
        brightTaps = glGetUniformLocation(brightProgram, "taps");
        glUseProgram(0);
    }
    if (!blurProgram)
    {
        blurProgram = Link(blurSource);
        if (!blurProgram)
            return false;
        glUseProgram(blurProgram);
        glUniform1i(glGetUniformLocation(blurProgram, "source"), 0);
        blurStep = glGetUniformLocation(blurProgram, "step");
        glUseProgram(0);
    }
    if (!compositeProgram)
    {
        compositeProgram = Link(compositeSource);
        if (!compositeProgram)
            return false;
        glUseProgram(compositeProgram);
        glUniform1i(glGetUniformLocation(compositeProgram, "scene"), 2);
        glUniform1i(glGetUniformLocation(compositeProgram, "glow"), 0);
        glUniform1i(glGetUniformLocation(compositeProgram, "halo"), 1);
        compositeIntensity = glGetUniformLocation(compositeProgram, "intensity");
        compositeHalo = glGetUniformLocation(compositeProgram, "haloStrength");
        compositeVignette = glGetUniformLocation(compositeProgram, "vignette");
        compositeDither = glGetUniformLocation(compositeProgram, "ditherStrength");
        glUseProgram(0);
    }
    if (sceneWidth != width || sceneHeight != height)
    {
        if (!sceneTexture)
            glGenTextures(1, &sceneTexture);
        Allocate(sceneTexture, width, height);
        sceneWidth = width;
        sceneHeight = height;
    }
    int shrink = Shrink(height);
    int glowWidth = (width + shrink - 1) / shrink, glowHeight = (height + shrink - 1) / shrink;
    if (glowTarget.width != glowWidth || glowTarget.height != glowHeight)
    {
        if (!Create(glowTarget, glowWidth, glowHeight) || !Create(scratchTarget, glowWidth, glowHeight))
            return false;
    }
    int haloWidth = (glowWidth + 1) / 2, haloHeight = (glowHeight + 1) / 2;
    if (haloTarget.width != haloWidth || haloTarget.height != haloHeight)
    {
        if (!Create(haloTarget, haloWidth, haloHeight) || !Create(haloScratchTarget, haloWidth, haloHeight))
            return false;
    }
    return true;
}

void Blur(Target const& source, Target const& scratch, Target const& destination, float radius)
{
    glUseProgram(blurProgram);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, scratch.framebuffer);
    glViewport(0, 0, scratch.width, scratch.height);
    glBindTexture(GL_TEXTURE_2D, source.texture);
    glUniform2f(blurStep, radius / scratch.width, 0);
    Quad();
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, destination.framebuffer);
    glBindTexture(GL_TEXTURE_2D, scratch.texture);
    glUniform2f(blurStep, 0, radius / scratch.height);
    Quad();
}

float Clamp(REAL value, float low, float high)
{
    return value < low ? low : (value > high ? high : value);
}

static rCallbackBeforeScreenModeChange sr_bloomUnload(&Unload);

} // namespace

void sr_Bloom()
{
    if (!sr_bloom || !sr_glOut || failed || sr_bloomIntensity <= 0)
        return;
    int width = sr_screenWidth, height = sr_screenHeight;
    if (width < 8 || height < 8)
        return;
    if (!GLEW_VERSION_2_0 || !GLEW_EXT_framebuffer_object)
    {
        Fail("this OpenGL has no shaders or framebuffer objects");
        return;
    }

    // close whatever the renderer wrapper still has open, forget stale errors
    RenderEnd();
    while (glGetError() != GL_NO_ERROR)
        ;

    {
        BloomState state;
        if (!Prepare(width, height) || glGetError() != GL_NO_ERROR)
        {
            Fail("the glow shaders or buffers could not be created");
            return;
        }

        // 1. grab the finished scene (a multisampled window is resolved by the copy)
        state.BindScene();
        glBindTexture(GL_TEXTURE_2D, sceneTexture);
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, width, height);

        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glDisable(GL_ALPHA_TEST);
        glDisable(GL_BLEND);
        glDisable(GL_LIGHTING);
        glDisable(GL_CULL_FACE);
        glDisable(GL_FOG);
        glDisable(GL_SCISSOR_TEST);
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_COLOR_LOGIC_OP);
        glDisable(GL_POLYGON_SMOOTH);
        glDisable(GL_POLYGON_OFFSET_FILL);
        glDisable(GL_DITHER);
        glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
        glDisable(GL_SAMPLE_ALPHA_TO_ONE);
        glDisable(GL_SAMPLE_COVERAGE);
        for (int plane = 0; plane < 6; ++plane)
            glDisable(GL_CLIP_PLANE0 + plane);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_TEXTURE_2D);

        // 2. bright parts, glow buffer size
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, glowTarget.framebuffer);
        glViewport(0, 0, glowTarget.width, glowTarget.height);
        glUseProgram(brightProgram);
        glUniform2f(brightTexel, 1.f / width, 1.f / height);
        glUniform1f(brightThreshold, Clamp(sr_bloomThreshold, .001f, 1.f));
        glUniform1i(brightTaps, Shrink(height) / 2);
        glBindTexture(GL_TEXTURE_2D, sceneTexture);
        Quad();

        // 3. a tight core plus a lower-resolution halo. Scale the radius continuously
        // with the picture height, including when the buffer divisor changes.
        float radiusPixels = Clamp(sr_bloomRadius, .1f, 4.f) * 4.f * height / 1080.f;
        float coreRadius = radiusPixels * glowTarget.height / height;
        float haloRadius = radiusPixels * 4.f * haloTarget.height / height;
        Blur(glowTarget, scratchTarget, glowTarget, coreRadius);
        Blur(glowTarget, haloScratchTarget, haloTarget, haloRadius);

        // 4. Combine the scene copy and glow with a scalar colour-preserving
        // shoulder. The complete result replaces the old window pixels.
        state.BindScene();
        glViewport(0, 0, width, height);
        glDisable(GL_BLEND);
        glUseProgram(compositeProgram);
        glUniform1f(compositeIntensity, Clamp(sr_bloomIntensity, 0.f, 16.f));
        glUniform1f(compositeHalo, Clamp(sr_bloomHalo, 0.f, 2.f));
        glUniform1f(compositeVignette, Clamp(sr_bloomVignette, 0.f, 1.f));
        glUniform1f(compositeDither, sr_dither ? 1.f : 0.f);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, sceneTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, haloTarget.texture);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, glowTarget.texture);
        Quad();
    }

    if (glGetError() != GL_NO_ERROR)
        Fail("an OpenGL error happened");
}

#else // HAVE_GLEW

void sr_Bloom()
{
}

#endif // HAVE_GLEW

#else // DEDICATED

void sr_Bloom()
{
}

#endif // DEDICATED
