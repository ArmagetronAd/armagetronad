#ifndef ARMAGETRON_LIGHTWALL_MATERIAL_H
#define ARMAGETRON_LIGHTWALL_MATERIAL_H

#include "aa_config.h"

#ifndef DEDICATED
#include "rGL.h"
#include "rRender.h"
#include "rScreen.h"
#include "rDisplayList.h"
#include "tConsole.h"
#include "gLightColour.h"
#include <algorithm>
#include <cmath>

// A continuous coloured sheet over the physical wall. Restrained flowing light
// supplies the liquid appearance while the geometry stays upright. The same
// material and derivative-filtered borders are used at every viewing distance.
class gLightwallMaterial
{
    static bool& ActiveState()
    {
        static bool value = false;
        return value;
    }
#ifdef HAVE_GLEW
    struct Bindings
    {
        GLint texture = -1, time = -1, head = -1, viewport = -1;
        GLint pixelScale = -1, bank = -1;
        GLint emitter = -1, forward = -1, bikeScale = -1, tyreShape = -1;
    };
    static GLuint& Program()
    {
        static GLuint value = 0;
        return value;
    }
    static bool& Failed()
    {
        static bool value = false;
        return value;
    }
    static Bindings& Uniforms()
    {
        static Bindings value;
        return value;
    }
    static void Unload()
    {
        if (Program())
            glDeleteProgram(Program());
        Program() = 0;
        Failed() = false;
        Uniforms() = Bindings();
        ActiveState() = false;
    }
    static GLuint Compile(GLenum kind, char const* source)
    {
        GLuint shader = glCreateShader(kind);
        char const* sources[] = {gLightColour::ShaderSource(), source};
        glShaderSource(shader, kind == GL_FRAGMENT_SHADER ? 2 : 1,
                       kind == GL_FRAGMENT_SHADER ? sources : &source, NULL);
        glCompileShader(shader);
        GLint success = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char message[1024] = "";
            glGetShaderInfoLog(shader, sizeof(message) - 1, NULL, message);
            con << "Lightwall material: " << message << "\n";
            glDeleteShader(shader);
            return 0;
        }
        return shader;
    }
    static bool Prepare()
    {
        static rCallbackBeforeScreenModeChange cleanup(&Unload);
        if (Program())
            return true;
        if (Failed() || !GLEW_VERSION_2_0)
            return false;
        char const* vertex = R"GLSL(#version 120
varying vec3 wallCoord;
varying vec4 wallColour;
varying vec3 glassTint;
varying vec3 glowTint;
varying vec3 coreTint;
varying vec4 wallBottomClip;
varying vec4 wallTopClip;
varying float wallMirror;
varying vec2 wallWorld;
varying float wallHeight;
void main() {
    wallWorld=gl_Vertex.xy;
    wallHeight=gl_Vertex.z;
    wallCoord=gl_MultiTexCoord0.xyz;
    wallColour=gl_Color;
    // Palette response is constant across a wall; evaluate it per vertex.
    vec3 tint=clamp(gl_Color.rgb,0.0,1.0);
    glassTint=pow(tint,vec3(1.35));
    glowTint=pow(tint,vec3(0.78));
    coreTint=mix(pow(tint,vec3(0.55)),vec3(1.0),0.45);
    float height=max(gl_MultiTexCoord0.w,0.0001);
    wallBottomClip=gl_ModelViewProjectionMatrix*vec4(gl_Vertex.xy,0.0,1.0);
    wallTopClip=gl_ModelViewProjectionMatrix*vec4(gl_Vertex.xy,height,1.0);
    gl_ClipVertex=gl_ModelViewMatrix*gl_Vertex;
    wallMirror=1.0-step(0.0,dot(cross(gl_ModelViewMatrix[0].xyz,gl_ModelViewMatrix[1].xyz),gl_ModelViewMatrix[2].xyz));
    gl_Position=ftransform();
}
)GLSL";
        char const* fragment = R"GLSL(
uniform sampler2D energyTexture;
uniform float flowTime;
uniform float headCoord;
uniform float pixelScale;
uniform vec2 referenceViewport;
uniform vec2 rearEmitter;
uniform vec2 tyreShape;
uniform vec2 bikeForward;
uniform vec2 bankSinCos;
uniform float bikeScale;
varying vec3 wallCoord;
varying vec4 wallColour;
varying vec3 glassTint;
varying vec3 glowTint;
varying vec3 coreTint;
varying vec4 wallBottomClip;
varying vec4 wallTopClip;
varying float wallMirror;
varying vec2 wallWorld;
varying float wallHeight;
float edgeCoverage(float v,float rate,float footprint) {
    float lo=max(0.0,v-footprint*0.5),hi=min(1.0,v+footprint*0.5);
    return (exp(-rate*lo)-exp(-rate*hi))/(rate*footprint);
}
float hash21(vec2 p) {
    vec3 q=fract(vec3(p.xyx)*vec3(0.1031,0.1030,0.0973));
    q+=dot(q,q.yzx+33.33);
    return fract((q.x+q.y)*q.z);
}
float opticalNoise(vec2 p) {
    vec2 i=floor(p),f=fract(p);
    f=f*f*(3.0-2.0*f);
    return mix(mix(hash21(i),hash21(i+vec2(1.0,0.0)),f.x),
               mix(hash21(i+vec2(0.0,1.0)),hash21(i+vec2(1.0)),f.x),f.y);
}
float contour(float field,float level,float width) {
    float aa=max(fwidth(field),0.001);
    float filtered=sqrt(width*width+aa*aa);
    return exp(-pow((field-level)/filtered,2.0))*width/filtered;
}
void main() {
    // Very tight local reversals can place the physical wall cut inside the
    // drawn rear tyre. Clip just that newborn overlap, in both main/mirror
    // passes; settled collision walls elsewhere keep their full silhouette.
    vec2 relative=wallWorld-rearEmitter;
    vec2 tyre=vec2(dot(relative,bikeForward)-tyreShape.x*bikeScale,
                   dot(relative,vec2(-bikeForward.y,bikeForward.x)));
    float tyreDistance=length(tyre/(vec2(tyreShape.y,0.46)*max(bikeScale,0.001)));
    if (wallCoord.z >= -1.5) {
        float side=tyre.y*bankSinCos.y+wallHeight*bankSinCos.x;
        float vertical=(wallHeight*bankSinCos.y-tyre.y*bankSinCos.x)/max(bikeScale,0.001)-tyreShape.y;
        tyreDistance=length(vec3(tyre.x/max(bikeScale,0.001)/tyreShape.y,
                                 side/max(bikeScale,0.001)/0.46,vertical/tyreShape.y));
    }
    float fresh=wallCoord.z < -1.5 ? 1.0 : 1.0-smoothstep(2.4,3.2,headCoord-wallCoord.x);
    float contact=mix(1.0,smoothstep(1.01,1.01+max(0.05,fwidth(tyreDistance)*0.75),tyreDistance),fresh);
    if (contact<=0.0) discard;
    vec2 bottom=wallBottomClip.xy/max(wallBottomClip.w,0.0001);
    vec2 top=wallTopClip.xy/max(wallTopClip.w,0.0001);
    vec2 vertical=(top-bottom)*referenceViewport;
    float ribbonPixels=length(vertical);
    vertical/=max(ribbonPixels,0.001);
    if (wallCoord.z < -0.5) {
        if (wallCoord.z < -1.5) {
            vec4 spill=wallColour*texture2D(energyTexture,wallCoord.xy);
            gl_FragColor=vec4(spill.rgb*spill.a*contact,0.0);
        } else discard;
        return;
    }
    float u=wallCoord.x,v=clamp(wallCoord.y,0.0,1.0);
    float footprint=max(fwidth(wallCoord.y),0.0001);
    // One optical material at every distance. Only unresolved surface detail
    // fades out; its colour, opacity and edge energy never switch to another LOD.
    float sheetPixels=1.0/max(length(vec2(dFdx(wallCoord.y),dFdy(wallCoord.y)))*pixelScale,0.0001);
    float resolved=smoothstep(3.0,18.0,min(ribbonPixels,sheetPixels));
    // The reference is a luminous membrane: a fine upper lip, a stronger
    // lower band and moving soft highlights through a see-through interior.
    float upper=edgeCoverage(v,1.0/0.007,footprint);
    float lower=edgeCoverage(1.0-v,1.0/0.022,footprint);
    float upperGlow=edgeCoverage(v,1.0/0.055,footprint);
    float lowerGlow=edgeCoverage(1.0-v,1.0/0.13,footprint);
    float inside=smoothstep(0.0,0.06,v)*(1.0-smoothstep(0.94,1.0,v));

    // Advected, elongated lenses supply the softness of flowing liquid while
    // all geometry remains fixed. Filter the narrow highlights at distance.
    vec2 p=vec2(u*0.48+flowTime*0.43,v*4.2);
    vec2 drift=vec2(opticalNoise(p*vec2(0.62,0.77)+vec2(8.1,flowTime*0.10)),
                    opticalNoise(p*vec2(0.71,0.54)+vec2(2.7,-flowTime*0.12)))-0.5;
    float field=opticalNoise(p+drift*vec2(0.60,0.85));
    float fine=opticalNoise(p*vec2(1.8,2.2)+drift+vec2(3.4,-flowTime*0.13));
    float broad=contour(field,0.52,0.13);
    float glint=contour(field+fine*0.12,0.56,0.035);
    float flowing=inside*resolved;

    vec3 tint=clamp(wallColour.rgb,0.0,1.0);
    // Preserve the player's colour family, with richer coloured transmission
    // and hot edge highlights. Whitening belongs only to the narrow light core,
    // never to the entire sheet (the earlier colour LUT made it all cream).
    float body=0.22+flowing*(0.12*field+0.13*broad);
    vec3 emission=glassTint*body;
    emission+=glowTint*(0.14*upperGlow+0.36*lowerGlow);
    emission+=glowTint*flowing*(0.075*glint+0.035*fine);
    emission+=coreTint*(0.83*upper+0.98*lower);
    emission=boundedLight(emission,tint);

    // Keep the membrane transparent and stop repeated overlaps clipping its
    // colour. A hot edge may be pale; overlapping coloured interiors must not
    // turn into white rectangles. No duplicate opaque-scene/reflection copy.
    float peak=max(emission.r,max(emission.g,emission.b));
    float opacity=max(0.18+0.10*flowing*field,peak);
    float mirror=mix(1.0,0.46,wallMirror);
    float fade=wallColour.a*contact*mirror;
    gl_FragColor=vec4(emission*fade,min(opacity,0.98)*fade);
}
)GLSL";
        GLuint vs = Compile(GL_VERTEX_SHADER, vertex), fs = Compile(GL_FRAGMENT_SHADER, fragment);
        if (vs && fs)
        {
            Program() = glCreateProgram();
            glAttachShader(Program(), vs);
            glAttachShader(Program(), fs);
            glLinkProgram(Program());
            GLint linked = GL_FALSE;
            glGetProgramiv(Program(), GL_LINK_STATUS, &linked);
            if (!linked)
            {
                char message[1024] = "";
                glGetProgramInfoLog(Program(), sizeof(message) - 1, NULL, message);
                con << "Lightwall material link: " << message << "\n";
                glDeleteProgram(Program());
                Program() = 0;
            }
        }
        if (vs)
            glDeleteShader(vs);
        if (fs)
            glDeleteShader(fs);
        if (!Program())
        {
            Failed() = true;
            con << "Lightwall material: using the textured energy sheet.\n";
            return false;
        }
        if (sr_hiddenWindow)
            std::cerr << "Legacy wall material: continuous coloured ribbon shader linked.\n";
        Uniforms().texture = glGetUniformLocation(Program(), "energyTexture");
        Uniforms().time = glGetUniformLocation(Program(), "flowTime");
        Uniforms().head = glGetUniformLocation(Program(), "headCoord");
        Uniforms().viewport = glGetUniformLocation(Program(), "referenceViewport");
        Uniforms().bank = glGetUniformLocation(Program(), "bankSinCos");
        Uniforms().pixelScale = glGetUniformLocation(Program(), "pixelScale");
        Uniforms().emitter = glGetUniformLocation(Program(), "rearEmitter");
        Uniforms().tyreShape = glGetUniformLocation(Program(), "tyreShape");
        Uniforms().forward = glGetUniformLocation(Program(), "bikeForward");
        Uniforms().bikeScale = glGetUniformLocation(Program(), "bikeScale");
        return true;
    }
#endif
public:
    // Wall lists record the matching blend mode; this is true only after a
    // program has actually been bound, not merely when shaders are supported.
    static bool IsActive() { return ActiveState(); }

    class Scope
    {
    public:
        Scope(bool enabled, float seconds, float head, float emitterX, float emitterY, float forwardX, float forwardY, float bikeScale, float rearOffset = .80f, float rearRadius = .73f, float bank = 0)
        {
#ifdef HAVE_GLEW
            // The caller enters before creating/calling any wall display list.
            // A surrounding renderer-owned list also falls back rather than
            // baking a program switch or stale animation uniforms into itself.
            if (!enabled || rDisplayList::IsRecording() || !GLEW_VERSION_2_0)
                return;
            RenderEnd();
            glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram_);
            glGetIntegerv(GL_ACTIVE_TEXTURE, &previousTexture_);
            previousActive_ = ActiveState();
            glActiveTexture(GL_TEXTURE0);
            bound_ = true;
            if (!Prepare())
            {
                // A failed optional shader must leave a real fixed-function
                // fallback, rather than inheriting another material's program.
                glUseProgram(0);
                ActiveState() = false;
                return;
            }
            glUseProgram(Program());
            float clock = std::isfinite(seconds) ? std::fmod(std::max(0.f, seconds), 2048.f) : 0;
            float distance = std::isfinite(head) ? std::max(0.f, head) : 0;
            glUniform1i(Uniforms().texture, 0);
            glUniform1f(Uniforms().time, clock);
            glUniform1f(Uniforms().head, distance);
            glUniform2f(Uniforms().emitter, emitterX, emitterY);
            glUniform2f(Uniforms().tyreShape, rearOffset, rearRadius);
            glUniform2f(Uniforms().forward, forwardX, forwardY);
            glUniform1f(Uniforms().bikeScale, bikeScale);
            glUniform1f(Uniforms().pixelScale, sr_screenHeight / 1080.f);
            glUniform2f(Uniforms().bank, std::sin(bank), std::cos(bank));
            GLint viewport[4];
            glGetIntegerv(GL_VIEWPORT, viewport);
            // Measure apparent height in 1080p reference pixels. Raising native
            // resolution improves sampling without restoring distant rail bars.
            glUniform2f(Uniforms().viewport, 540.f * viewport[2] / std::max(1, viewport[3]), 540.f);
            ActiveState() = true;
#else
            (void)enabled;
            (void)seconds;
            (void)head;
            (void)emitterX;
            (void)emitterY;
            (void)forwardX;
            (void)forwardY;
            (void)bikeScale;
            (void)rearOffset;
            (void)rearRadius;
            (void)bank;
#endif
        }
        ~Scope()
        {
#ifdef HAVE_GLEW
            if (bound_)
            {
                RenderEnd();
                ActiveState() = previousActive_;
                glUseProgram(previousProgram_);
                glActiveTexture(previousTexture_);
            }
#endif
        }
        Scope(Scope const&) = delete;
        Scope& operator=(Scope const&) = delete;

    private:
#ifdef HAVE_GLEW
        GLint previousProgram_ = 0, previousTexture_ = GL_TEXTURE0;
        bool bound_ = false, previousActive_ = false;
#endif
    };
};
#endif
#endif
