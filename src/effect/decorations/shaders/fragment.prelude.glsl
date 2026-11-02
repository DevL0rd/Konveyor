#version 140
#include "colormanagement.glsl"
in vec4 attributes;
in vec2 framePosition;
out vec4 fragColor;
uniform vec4 colors[8];
uniform int colorCount;
uniform float colorMix;
uniform float layerOpacity;
uniform vec2 halfSize;
uniform vec4 radii;
uniform float pixels;
uniform float inset;
uniform float bandWidth;
uniform float phase;
uniform int kind;
uniform vec4 params;
uniform vec4 front;
uniform float additive;
vec4 finish(vec4 color)
{
    float alpha = color.a * layerOpacity;
    vec4 encoded = nitsToDestinationEncoding(sourceEncodingToNitsInDestinationColorspace(vec4(color.rgb * alpha, alpha)));
    float light = clamp(max(encoded.r, max(encoded.g, encoded.b)), 0.0, 1.0);
    return vec4(encoded.rgb, mix(encoded.a, light, additive));
}
