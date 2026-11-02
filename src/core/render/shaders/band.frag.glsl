vec4 gradientLayer(vec2 p, float d)
{
    float angle = params.x + phase * TAU;
    vec2 axis = vec2(cos(angle), sin(angle));
    float reach = abs(axis.x) * halfSize.x + abs(axis.y) * halfSize.y;
    return tinted(palette(dot(p, axis) / max(reach, 1.0) * 0.5 + 0.5, false), band(d, bandWidth));
}
vec4 sweepLayer(vec2 p, float d)
{
    return tinted(palette(atan(p.y, p.x) / TAU + phase, true), band(d, bandWidth));
}
vec4 glowLayer(float d)
{
    float wave = 0.5 - 0.5 * cos(phase * TAU);
    float falloff = clamp(1.0 - d / bandWidth, 0.0, 1.0);
    return tinted(palette(wave, false), band(d, bandWidth) * falloff * falloff * (0.45 + 0.55 * wave));
}
vec4 rippleLayer(vec2 p, float d)
{
    float top = 2.0 * halfSize.x - radii.x - radii.y;
    float along = fract(perimeterFraction(p) - 0.5 * top / perimeterLength() + 0.5);
    float x = abs(along - 0.5) * 2.0;
    float offset = (x - phase * 1.2) / 0.14;
    float wave = exp(-offset * offset);
    return tinted(palette(x, false), wave * (1.0 - phase) * band(d, bandWidth * (1.0 + 2.0 * wave)));
}
vec4 dashesLayer(vec2 p, float d)
{
    float period = params.x + params.y;
    float total = perimeterLength();
    float repeats = max(round(total / period), 1.0);
    float actual = total / repeats;
    float along = perimeterFraction(p);
    float position = fract(along * repeats - phase) * actual;
    float solid = params.x / period * actual;
    float dash = clamp(min(position, solid - position) * pixels + 0.5, 0.0, 1.0);
    return tinted(palette(along, true), dash * band(d, bandWidth));
}
vec4 pulseLayer(float d)
{
    float wave = 0.5 + 0.5 * cos(phase * TAU);
    float halo = d > 0.0 ? clamp(1.0 - (d - bandWidth) / bandWidth, 0.0, 1.0) : 0.0;
    float coverage = max(band(d, bandWidth) * (0.35 + 0.65 * wave), halo * halo * 0.5 * wave);
    return tinted(palette(wave, false), coverage);
}
float edgeLight(float d)
{
    float falloff = clamp(1.0 - d / bandWidth, 0.0, 1.0);
    return band(d, bandWidth) * sqrt(falloff);
}
float frontLight(float x)
{
    float offset = x / (x > 0.0 ? params.x : params.y);
    return exp(-offset * offset) * params.z;
}
vec4 pulseFrontLayer(vec2 p, float d)
{
    float distance = length(max(abs(p - front.xy) - front.zw, vec2(0.0)));
    return tinted(palette(params.w, false), frontLight(distance - phase) * edgeLight(d));
}
vec4 waveLayer(vec2 p, float d)
{
    float travelled = dot(p - front.xy, front.zw);
    return tinted(palette(params.w, false), frontLight(travelled - phase) * edgeLight(d));
}
vec4 drawInLayer(vec2 p, float d)
{
    float behind = phase - perimeterFraction(p);
    if (behind < 0.0) {
        return vec4(0.0);
    }
    float head = exp(-behind * params.x);
    return tinted(palette(behind, false), (0.45 + 0.55 * head) * params.z * edgeLight(d));
}
vec4 fadeOutLayer(float d)
{
    float left = 1.0 - phase;
    return tinted(palette(phase, false), left * left * edgeLight(d));
}
vec4 eventLayer(vec2 p, float d)
{
    if (kind == 100) {
        return pulseFrontLayer(p, d);
    }
    if (kind == 101) {
        return waveLayer(p, d);
    }
    if (kind == 102) {
        return drawInLayer(p, d);
    }
    return fadeOutLayer(d);
}
void main()
{
    vec2 p = attributes.xy;
    float d = frameDistance(p) - inset;
    vec4 color;
    if (kind == 0) {
        color = gradientLayer(p, d);
    } else if (kind == 1) {
        color = sweepLayer(p, d);
    } else if (kind == 2) {
        color = cometLayer(p, d);
    } else if (kind == 3) {
        color = glowLayer(d);
    } else if (kind == 4) {
        color = rippleLayer(p, d);
    } else if (kind == 5) {
        color = dashesLayer(p, d);
    } else if (kind == 6) {
        color = pulseLayer(d);
    } else {
        color = eventLayer(p, d);
    }
    fragColor = finish(color);
}
