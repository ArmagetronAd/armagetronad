#ifndef ARMAGETRON_LIGHT_COLOUR_H
#define ARMAGETRON_LIGHT_COLOUR_H

// Bike inlays, ribbon edges and the illuminated membrane share display RGB.
// Vary optical intensity without lifting dark channels toward white or changing
// hue. Tone-map the complete contribution with one scalar before RGB8 clipping.
namespace gLightColour
{
inline char const* ShaderSource()
{
    return R"GLSL(#version 120
float cyanLight(vec3 tint) {
    float peak=max(max(tint.r,tint.g),max(tint.b,0.00001));
    return smoothstep(0.35,0.70,(min(tint.g,tint.b)-tint.r)/peak);
}
vec3 lightCore(vec3 tint,float heat) {
    return tint*mix(0.80,1.0,clamp(heat,0.0,1.0));
}
vec3 boundedLight(vec3 emission,vec3 tint) {
    float peak=max(emission.r,max(emission.g,emission.b));
    float scale=min(1.0,0.98/max(peak,0.00001));
    return emission*scale;
}
)GLSL";
}
} // namespace gLightColour
#endif
