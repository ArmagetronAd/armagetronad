#ifndef ARMAGETRON_LIGHTCYCLE_MATERIAL_H
#define ARMAGETRON_LIGHTCYCLE_MATERIAL_H

#include "aa_config.h"

#ifndef DEDICATED
#include "rGL.h"
#include "rScreen.h"
#include "tConsole.h"
#include "gLightColour.h"
#include <algorithm>
#include <cmath>

// Smooth materials for the sculpted lightcycle. Geometry continues to provide
// ordinary GL materials and normals, so machines without shaders use lighting
// from the compatibility renderer. Emissive trim keeps its exact player colour.
class gLightcycleMaterial
{
#ifdef HAVE_GLEW
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
    struct LightBindings
    {
        GLint colour = -1, axis = -1, shape = -1, rear = -1, front = -1, reactorA = -1, reactorB = -1;
        GLint clock = -1, drive = -1, assembly = -1, origin = -1, up = -1, forward = -1;
        GLint worldX = -1, worldY = -1, worldZ = -1;
        GLint opticalMask = -1;
    };
    static LightBindings& Lights()
    {
        static LightBindings value;
        return value;
    }

    static void Unload()
    {
        if (Program())
            glDeleteProgram(Program());
        Program() = 0;
        Failed() = false;
        Lights() = LightBindings();
    }

    static GLuint Compile(GLenum type, char const* source)
    {
        GLuint shader = glCreateShader(type);
        char const* sources[] = {gLightColour::ShaderSource(), source};
        glShaderSource(shader, type == GL_FRAGMENT_SHADER ? 2 : 1,
                       type == GL_FRAGMENT_SHADER ? sources : &source, NULL);
        glCompileShader(shader);
        GLint success = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char message[1024] = "";
            glGetShaderInfoLog(shader, sizeof(message) - 1, NULL, message);
            con << "Lightcycle material: " << message << "\n";
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

        char const* vertex =
            "#version 120\n"
            "varying vec3 viewPosition;\n"
            "varying vec3 viewNormal;\n"
            "varying vec3 localPosition;\n"
            "varying float riderPart;\n"
            "varying vec4 surfaceFinish;\n"
            "varying vec4 opticalData;\n"
            "varying float authoredMode;\n"
            "varying vec2 authoredRing;\n"
            "void main() {\n"
            "    vec4 p = gl_ModelViewMatrix * gl_Vertex;\n"
            "    viewPosition = p.xyz;\n"
            "    viewNormal = gl_NormalMatrix * gl_Normal;\n"
            "    localPosition = gl_Vertex.xyz;\n"
            "    riderPart = gl_MultiTexCoord3.x;\n"
            "    surfaceFinish = gl_MultiTexCoord2;\n"
            "    opticalData = gl_MultiTexCoord0;\n"
            "    authoredMode = gl_MultiTexCoord1.x;\n"
            "    authoredRing = gl_MultiTexCoord1.yz;\n"
            "    gl_ClipVertex = p;\n"
            "    gl_Position = ftransform();\n"
            "}\n";
        char const* fragment =
            "varying vec3 viewPosition;\n"
            "varying vec3 viewNormal;\n"
            "varying vec3 localPosition;\n"
            "varying float riderPart;\n"
            "varying vec4 surfaceFinish;\n"
            "varying vec4 opticalData;\n"
            "varying float authoredMode;\n"
            "varying vec2 authoredRing;\n"
            "uniform sampler2D opticalMask;\n"
            "uniform float cycleClock;\n"
            "uniform float driveEnergy;\n"
            "uniform float assembly;\n"
            "uniform vec3 cycleOrigin;\n"
            "uniform vec3 cycleUp;\n"
            "uniform vec3 cycleForward;\n"
            "uniform vec3 worldX;\n"
            "uniform vec3 worldY;\n"
            "uniform vec3 worldZ;\n"
            "uniform vec3 neonColour;\n"
            "uniform vec3 cycleAxis;\n"
            "uniform vec3 ringShape;\n" // radius, half width, local light reach
            "uniform vec3 rearLight;\n"
            "uniform vec3 frontLight;\n"
            "uniform vec3 reactorA;\n"
            "uniform vec3 reactorB;\n"
            // Procedural finish is filtered by its pixel footprint. Fine grain
            // appears as resolution permits and converges to its average at a
            // distance instead of turning into swimming one-pixel noise.
            "float grain(float phase) {\n"
            "    float visible = 1.0 - smoothstep(0.65, 3.0, fwidth(phase));\n"
            "    return sin(phase) * visible;\n"
            "}\n"
            "float highlight(vec3 n, vec3 v, vec3 l, float power) {\n"
            "    vec3 dx = dFdx(n), dy = dFdy(n);\n"
            "    float variance = max(dot(dx, dx), dot(dy, dy));\n"
            // A wider filtered highlight must get dimmer as it spreads. The
            // previous filter retained peak brightness while broadening, which
            // made small black fairings and metal barrels look uniformly grey.
            "    float filtered = power / (1.0 + power * variance * 0.75);\n"
            "    return pow(max(dot(n, normalize(v + l)), 0.0), filtered) * filtered / power;\n"
            "}\n"
            "float lightStrip(float coordinate, float width) {\n"
            "    float filteredWidth = sqrt(width * width + pow(fwidth(coordinate) * 0.65, 2.0));\n"
            "    return exp(-pow(coordinate / filteredWidth, 2.0)) * width / filteredWidth;\n"
            "}\n"
            "vec3 surfaceSpecular(vec3 n, vec3 v, vec3 l, float roughness, vec3 f0) {\n"
            "    vec3 h = normalize(v + l);\n"
            "    float nv = max(dot(n, v), 0.035), nl = max(dot(n, l), 0.0);\n"
            "    float nh = max(dot(n, h), 0.0), vh = max(dot(v, h), 0.0);\n"
            "    vec3 dx = dFdx(n), dy = dFdy(n);\n"
            "    float variance = max(dot(dx, dx), dot(dy, dy));\n"
            "    float alpha = max(0.035, roughness * roughness + variance * 0.65);\n"
            "    float a2 = alpha * alpha;\n"
            "    float denominator = nh * nh * (a2 - 1.0) + 1.0;\n"
            "    float distribution = a2 / max(3.14159265 * denominator * denominator, 0.00001);\n"
            "    float k = (roughness + 1.0) * (roughness + 1.0) * 0.125;\n"
            "    float visibility = nv / (nv * (1.0 - k) + k);\n"
            "    visibility *= nl / max(nl * (1.0 - k) + k, 0.0001);\n"
            "    vec3 fresnel = f0 + (vec3(1.0) - f0) * pow(1.0 - vh, 5.0);\n"
            "    return distribution * visibility * fresnel / (4.0 * nv);\n"
            "}\n"
            "vec3 displayColour(vec3 linear) {\n"
            "    linear = max(linear, vec3(0.0));\n"
            "    vec3 low = linear * 12.92;\n"
            "    vec3 high = 1.055 * pow(linear, vec3(1.0 / 2.4)) - 0.055;\n"
            "    return mix(low, high, step(vec3(0.0031308), linear));\n"
            "}\n"
            // Evaluate the nearest emitting part of an annulus/strip, not a
            // point at the empty wheel hub. The light follows the real bike in
            // normal views and in the mirrored floor pass.
            "vec3 neonSample(vec3 source, vec3 n, vec3 v, float reach) {\n"
            "    vec3 delta = source - viewPosition;\n"
            "    float distance2 = dot(delta, delta);\n"
            "    float distance = sqrt(max(distance2, 0.000001));\n"
            "    vec3 l = delta / distance;\n"
            "    float attenuation = 1.0 / (1.0 + distance2 / (reach * reach));\n"
            "    attenuation *= 1.0 - smoothstep(reach * 1.5, reach * 3.0, distance);\n"
            "    float diffuse = max(dot(n, l), 0.0);\n"
            "    float shine = highlight(n, v, l, max(12.0, gl_FrontMaterial.shininess * 1.25));\n"
            "    vec3 response = gl_FrontMaterial.diffuse.rgb * (1.35 * diffuse);\n"
            "    response += gl_FrontMaterial.specular.rgb * (0.29 * shine * diffuse);\n"
            "    return neonColour * response * attenuation;\n"
            "}\n"
            "vec3 ringLight(vec3 center, vec3 n, vec3 v) {\n"
            "    vec3 delta = viewPosition - center;\n"
            "    float axial = dot(delta, cycleAxis);\n"
            "    vec3 radial = delta - cycleAxis * axial;\n"
            "    radial /= max(length(radial), 0.0001);\n"
            "    float side = step(0.0, axial) * 2.0 - 1.0;\n"
            "    vec3 source = center + radial * ringShape.x + cycleAxis * side * ringShape.y;\n"
            "    return neonSample(source, n, v, ringShape.z);\n"
            "}\n"
            "vec3 reactorLight(vec3 n, vec3 v) {\n"
            "    vec3 axis = reactorB - reactorA;\n"
            "    float t = clamp(dot(viewPosition - reactorA, axis) / max(dot(axis, axis), 0.000001), 0.0, 1.0);\n"
            "    vec3 center = reactorA + axis * t;\n"
            "    float side = step(0.0, dot(viewPosition - center, cycleAxis)) * 2.0 - 1.0;\n"
            "    return neonSample(center + cycleAxis * side * ringShape.y * 0.96, n, v, ringShape.z * 0.8);\n"
            "}\n"
            "void main() {\n"
            "    float modelHeight = dot(viewPosition - cycleOrigin, cycleUp);\n"
            "    float modelX = dot(viewPosition - cycleOrigin, cycleForward);\n"
            "    float axle = modelX < 0.2 ? -1.45 : 1.55;\n"
            "    float radial = length(vec2(modelX - axle, modelHeight - 0.72));\n"
            "    float wheel = (1.0 - smoothstep(0.74, 0.82, radial)) * step(0.62, abs(modelX - 0.2));\n"
            "    float sweep = (atan(modelHeight - 0.72, modelX - axle) + 3.14159265) / 6.2831853;\n"
            "    float wheelBuild = 0.15 + sweep * 0.32;\n"
            "    float bodyBuild = 0.34 + 0.30 * (1.0 - smoothstep(-1.8, 2.2, modelX));\n"
            "    bodyBuild += 0.12 * smoothstep(0.5, 1.6, modelHeight);\n"
            "    float buildAt = mix(bodyBuild, wheelBuild, wheel);\n"
            "    if (riderPart > 0.5) buildAt = 0.0;\n"

            "    vec3 n = normalize(viewNormal);\n"
            "    vec3 v = normalize(-viewPosition);\n"
            "    if (!gl_FrontFacing) n = -n;\n"
            "    vec3 key = normalize(gl_LightSource[0].position.xyz);\n"
            "    vec3 fill = normalize(gl_LightSource[1].position.xyz);\n"
            // The mesh identifies each finish explicitly. Ordinary GL materials
            // still supply colour and remain usable by compatibility lighting.
            "    vec3 albedo = gl_FrontMaterial.diffuse.rgb;\n"
            "    vec3 specularColour = gl_FrontMaterial.specular.rgb;\n"
            "    float metal = surfaceFinish.y;\n"
            "    float coated = surfaceFinish.z;\n"
            "    float cloth = surfaceFinish.w;\n"
            "    float nv = max(dot(n, v), 0.0);\n"
            "    float fresnel = pow(1.0 - nv, 5.0);\n"
            "    float brushed = grain(localPosition.y * 930.0 + localPosition.z * 41.0);\n"
            "    float weave = grain(localPosition.x * 330.0) * grain(localPosition.z * 310.0);\n"
            "    float fineGrain = grain(dot(localPosition, vec3(680.0, 430.0, 750.0)));\n"
            "    float finish = 1.0 + metal * brushed * 0.065 + cloth * weave * 0.055;\n"
            "    finish += (1.0 - metal) * (1.0 - cloth) * fineGrain * 0.012;\n"
            "    vec3 keyColour = vec3(0.84, 0.91, 1.0);\n"
            "    vec3 fillColour = vec3(0.38, 0.49, 0.55);\n"
            "    vec3 illumination = vec3(0.075, 0.090, 0.105);\n"
            "    illumination += keyColour * max(dot(n, key), 0.0) * 0.46;\n"
            "    illumination += fillColour * max(dot(n, fill), 0.0) * 0.34;\n"
            "    vec3 body = albedo * illumination * finish * mix(1.0, 0.08, metal);\n"
            "    body += gl_FrontMaterial.ambient.rgb * 0.12;\n"
            "    float surfaceRoughness = surfaceFinish.x;\n"
            "    vec3 f0 = mix(vec3(mix(0.035, 0.022, cloth)), specularColour * 0.92, metal);\n"
            "    if (authoredMode > 0.5) f0 = mix(vec3(mix(0.012, 0.0096, cloth)), albedo, metal);\n"
            "    body += keyColour * surfaceSpecular(n, v, key, surfaceRoughness, f0) * 0.62;\n"
            "    body += fillColour * surfaceSpecular(n, v, fill, surfaceRoughness, f0) * 0.28;\n"
            // The arena lighting is anchored to world directions, including in
            // the mirrored-floor pass. Camera movement now travels across the
            // fairing instead of dragging a blue reflection along with the eye.
            "    vec3 reflected = reflect(-v, n);\n"
            "    vec3 environmentDirection = normalize(vec3(dot(reflected, worldX), dot(reflected, worldY), dot(reflected, worldZ)));\n"
            "    float sky = smoothstep(-0.10, 0.80, environmentDirection.z);\n"
            "    float roughness = surfaceRoughness;\n"
            // Stadium lamp banks and horizontal fascia replace the two
            // diagonal streaks that looked like painted zigzags in the hubs.
            "    float azimuth = atan(environmentDirection.y, environmentDirection.x) / 6.2831853;\n"
            "    float station = abs(fract(azimuth * 16.0 + 0.5) - 0.5);\n"
            "    float lampWidth = 0.055 + roughness * 0.16;\n"
            "    float lamps = 1.0 - smoothstep(lampWidth, lampWidth + max(fwidth(azimuth * 16.0), 0.02), station);\n"
            "    lamps *= lightStrip(environmentDirection.z - 0.28, 0.024 + roughness * 0.065);\n"
            "    float fascia = lightStrip(environmentDirection.z - 0.055, 0.010 + roughness * 0.020);\n"
            "    float overhead = lightStrip(environmentDirection.z - 0.72, 0.10 + roughness * 0.12);\n"
            "    vec3 environment = mix(vec3(0.008, 0.012, 0.018), vec3(0.045, 0.065, 0.090), sky);\n"
            // Broad cool arena fill reveals the curved shell between the thin
            // lamp reflections; its world direction is stable as the camera turns.
            "    float softbox = pow(max(dot(environmentDirection, normalize(vec3(-0.35, 0.55, 0.76))), 0.0), 5.0);\n"
            "    environment += vec3(0.18, 0.34, 0.43) * softbox;\n"
            "    environment += vec3(3.1, 3.35, 3.6) * lamps;\n"
            "    environment += vec3(0.16, 0.42, 0.56) * fascia + vec3(0.20, 0.25, 0.30) * overhead;\n"
            "    if (authoredMode > 0.5) environment = mix(vec3(0.004, 0.008, 0.012), vec3(0.018, 0.027, 0.035), sky) + vec3(0.055, 0.085, 0.110) * softbox;\n"
            // Rough surfaces lose the sharp grazing mirror, while coated panels
            // retain it. Cloth has a diffuse edge below, not a chrome silhouette.
            "    vec3 reflectance = f0 + (vec3(max(1.0 - roughness, 0.04)) - f0) * fresnel;\n"
            "    body += environment * reflectance * finish * mix(0.30, 1.0, max(coated, metal)) * (1.0 - cloth * 0.80);\n"
            // A softer cloth edge helps the rider read as fabric next to the
            // glass-coated armour without making the whole suit metallic.
            "    body += albedo * cloth * pow(1.0 - nv, 3.0) * 0.38;\n"
            "    body += (ringLight(rearLight, n, v) + ringLight(frontLight, n, v) + reactorLight(n, v)) * (authoredMode > 0.5 ? 0.12 : 1.0);\n"
            "    float cavity = wheel * (1.0 - smoothstep(0.49, 0.54, radial)) * (1.0 - smoothstep(0.10, 0.42, abs(localPosition.y)));\n"
            "    body *= 1.0 - cavity * metal * 0.48;\n"
            "    body = min(body, vec3(0.94));\n"
            // Lighting is evaluated in linear intensity; the compatibility
            // framebuffer expects display RGB. Encode the opaque material once
            // so dark paint retains its shape. Inlay colours are already display RGB.
            "    body = displayColour(body);\n"
            // Black paint may reflect the arena, but its highlights must not
            // become a second luminous skin in the scene's bloom extraction.
            "    if (authoredMode > 0.5) body *= min(1.0, 0.16 / max(max(body.r, body.g), max(body.b, 0.00001)));\n"
            // An emitter is not also a reflective black surface. Keep a small
            // optical core, then bound the complete light before the RGB8 target
            // clips channels; otherwise cyan wheels turn almost solid white.
            "    vec3 emission = gl_FrontMaterial.emission.rgb;\n"
            "    float coverage = 0.0;\n"
            "    if (authoredMode > 1.5 && authoredMode < 2.5) coverage = texture2D(opticalMask, opticalData.xy).r;\n"
            "    if (authoredMode > 2.5) {\n"
            "        float edgeWidth = max(0.002, fwidth(opticalData.w));\n"
            "        coverage = clamp(opticalData.z, 0.0, 1.0) * (1.0 - smoothstep(0.008 - edgeWidth * 0.5, 0.008 + edgeWidth * 0.5, opticalData.w));\n"
            "    }\n"
            "    emission = mix(emission, neonColour, coverage);\n"
            "    float luminous = max(emission.r, max(emission.g, emission.b));\n"
            "    float pulse = pow(0.5 + 0.5 * grain(localPosition.x * 19.0 - cycleClock * 9.0), 6.0);\n"
            // The ring's narrow bright centre and coloured shoulders share the
            // trail's RGB ratios. Filter the core at small screen sizes.
            "    float coreWidth = sqrt(0.012 * 0.012 + pow(fwidth(radial) * 0.65, 2.0));\n"
            "    float opticalCore = exp(-pow((radial - 0.608) / coreWidth, 2.0)) * 0.012 / coreWidth;\n"
            "    float opticalRing = wheel * smoothstep(0.550, 0.558, radial) * (1.0 - smoothstep(0.645, 0.652, radial));\n"
            "    float lightProfile = mix(1.0, 0.62 + opticalCore * 0.56, opticalRing);\n"
            "    if (authoredMode > 0.5) {\n"
            "        lightProfile = 1.0;\n"
            "        if (authoredRing.y > 0.0) {\n"
            "            float radius = length(localPosition.xz);\n"
            "            float width = authoredRing.y * 0.30;\n"
            "            float filtered = sqrt(width * width + pow(fwidth(radius) * 0.65, 2.0));\n"
            "            float core = exp(-pow((radius - authoredRing.x) / filtered, 2.0)) * width / filtered;\n"
            "            lightProfile = 0.80 + 0.20 * core;\n"
            "        }\n"
            "    }\n"
            "    emission = lightCore(emission, 1.0) * lightProfile;\n"
            "    if (authoredMode < 0.5) emission *= 1.08 + 0.025 * pulse + 0.04 * driveEnergy;\n"
            "    emission *= min(1.0, 0.98 / max(max(emission.r, emission.g), max(emission.b, 0.00001)));\n"
            "    body *= authoredMode > 1.5 ? 1.0 - coverage : 1.0 - step(0.0001, luminous);\n"
            "    float scan = exp(-pow((assembly - buildAt) / 0.035, 2.0)) * (1.0 - step(0.999, assembly));\n"
            "    scan *= 1.0 - step(0.5, riderPart);\n"
            "    if (assembly < buildAt && riderPart < 0.5) {\n"
            "        if (assembly + 0.22 < buildAt || assembly < 0.08) discard;\n"
            "        float gx = abs(sin(modelX * 20.0)), gz = abs(sin(modelHeight * 24.0));\n"
            "        float grid = min(gx / max(fwidth(gx), 0.06), gz / max(fwidth(gz), 0.06));\n"
            "        if (grid > 0.65) discard;\n"
            "        float build = 1.0 - smoothstep(0.02, 0.22, buildAt - assembly);\n"
            "        gl_FragColor = vec4(mix(neonColour, vec3(1.0), 0.35) * (0.30 + 0.55 * build), 1.0);\n"
            "        return;\n"
            "    }\n"
            "    gl_FragColor = vec4(body + emission + mix(neonColour, vec3(1.0), 0.65) * scan, gl_FrontMaterial.diffuse.a);\n"
            "}\n";
        GLuint vs = Compile(GL_VERTEX_SHADER, vertex);
        GLuint fs = Compile(GL_FRAGMENT_SHADER, fragment);
        if (vs && fs)
        {
            Program() = glCreateProgram();
            glAttachShader(Program(), vs);
            glAttachShader(Program(), fs);
            glLinkProgram(Program());
            GLint success = GL_FALSE;
            glGetProgramiv(Program(), GL_LINK_STATUS, &success);
            if (!success)
            {
                glDeleteProgram(Program());
                Program() = 0;
            }
            else
            {
                Lights().colour = glGetUniformLocation(Program(), "neonColour");
                Lights().axis = glGetUniformLocation(Program(), "cycleAxis");
                Lights().shape = glGetUniformLocation(Program(), "ringShape");
                Lights().rear = glGetUniformLocation(Program(), "rearLight");
                Lights().front = glGetUniformLocation(Program(), "frontLight");
                Lights().reactorA = glGetUniformLocation(Program(), "reactorA");
                Lights().reactorB = glGetUniformLocation(Program(), "reactorB");
                Lights().clock = glGetUniformLocation(Program(), "cycleClock");
                Lights().drive = glGetUniformLocation(Program(), "driveEnergy");
                Lights().assembly = glGetUniformLocation(Program(), "assembly");
                Lights().origin = glGetUniformLocation(Program(), "cycleOrigin");
                Lights().up = glGetUniformLocation(Program(), "cycleUp");
                Lights().forward = glGetUniformLocation(Program(), "cycleForward");
                Lights().worldX = glGetUniformLocation(Program(), "worldX");
                Lights().worldY = glGetUniformLocation(Program(), "worldY");
                Lights().worldZ = glGetUniformLocation(Program(), "worldZ");
                Lights().opticalMask = glGetUniformLocation(Program(), "opticalMask");
            }
        }
        if (vs)
            glDeleteShader(vs);
        if (fs)
            glDeleteShader(fs);
        if (!Program())
        {
            Failed() = true;
            con << "Lightcycle material: using compatibility lighting.\n";
        }
        if (Program() && sr_hiddenWindow)
            std::cerr << "Legacy cycle material: staged construction shader linked.\n";
        return Program() != 0;
    }
#endif

public:
    class Scope
    {
    public:
        Scope(float r, float g, float b, float rearAxle, float frontAxle, float wheelRadius,
              float seconds = 0, float drive = 0, float assembly = 1, float const* worldToView = nullptr)
        {
#ifdef HAVE_GLEW
            if (Prepare())
            {
                glGetIntegerv(GL_CURRENT_PROGRAM, &previous_);
                glUseProgram(Program());
                glUniform1i(Lights().opticalMask, 0);
                active_ = true;
                GLfloat matrix[16];
                glGetFloatv(GL_MODELVIEW_MATRIX, matrix);
                // Capture the arena transform before per-bike yaw/bank/scale.
                // Its reflected Z basis also handles the floor reflection pass.
                float const* environmentMatrix = worldToView ? worldToView : matrix;
                auto direction = [&](GLint uniform, int column)
                {
                    float x = environmentMatrix[column], y = environmentMatrix[column + 1], z = environmentMatrix[column + 2];
                    float length = std::max(std::sqrt(x * x + y * y + z * z), .0001f);
                    glUniform3f(uniform, x / length, y / length, z / length);
                };
                direction(Lights().worldX, 0);
                direction(Lights().worldY, 4);
                direction(Lights().worldZ, 8);
                auto point = [&](GLint uniform, float x, float y, float z)
                {
                    glUniform3f(uniform,
                                matrix[0] * x + matrix[4] * y + matrix[8] * z + matrix[12],
                                matrix[1] * x + matrix[5] * y + matrix[9] * z + matrix[13],
                                matrix[2] * x + matrix[6] * y + matrix[10] * z + matrix[14]);
                };
                float scale = std::sqrt(matrix[4] * matrix[4] + matrix[5] * matrix[5] + matrix[6] * matrix[6]);
                scale = std::max(scale, .0001f);
                glUniform3f(Lights().colour, r, g, b);
                glUniform1f(Lights().clock, std::isfinite(seconds) ? std::fmod(seconds, 2048.f) : 0.f);
                glUniform1f(Lights().drive, std::max(0.f, std::min(1.f, drive)));
                glUniform1f(Lights().assembly, std::max(0.f, std::min(1.f, assembly)));
                point(Lights().origin, 0, 0, 0);
                glUniform3f(Lights().up, matrix[8] / (scale * scale), matrix[9] / (scale * scale), matrix[10] / (scale * scale));
                glUniform3f(Lights().forward, matrix[0] / (scale * scale), matrix[1] / (scale * scale), matrix[2] / (scale * scale));
                glUniform3f(Lights().axis, matrix[4] / scale, matrix[5] / scale, matrix[6] / scale);
                glUniform3f(Lights().shape, wheelRadius * .84306f * scale, wheelRadius * .61944f * scale, wheelRadius * .50f * scale);
                point(Lights().rear, rearAxle, 0, wheelRadius);
                point(Lights().front, frontAxle, 0, wheelRadius);
                point(Lights().reactorA, -.32f, 0, .73f);
                point(Lights().reactorB, .22f, 0, .73f);
            }
#endif
        }
        ~Scope()
        {
#ifdef HAVE_GLEW
            if (active_)
                glUseProgram(previous_);
#endif
        }
        Scope(Scope const&) = delete;
        Scope& operator=(Scope const&) = delete;

    private:
#ifdef HAVE_GLEW
        GLint previous_ = 0;
        bool active_ = false;
#endif
    };
};
#endif
#endif
