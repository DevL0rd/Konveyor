float cornerRadius(vec2 p)
{
    return p.x > 0.0 ? (p.y > 0.0 ? radii.z : radii.y) : (p.y > 0.0 ? radii.w : radii.x);
}
float frameDistance(vec2 p)
{
    float r = cornerRadius(p);
    vec2 q = abs(p) - halfSize + vec2(r);
    if (r <= 0.0) {
        return max(q.x, q.y);
    }
    return min(max(q.x, q.y), 0.0) + length(max(q, vec2(0.0))) - r;
}
vec4 palette(float t, bool looped)
{
    float count = float(colorCount);
    bool loops = looped && colorMix < 0.0;
    float span = loops ? count : max(count - 1.0, 1.0);
    float x = (colorMix >= 0.0 ? colorMix : (loops ? fract(t) : clamp(t, 0.0, 1.0))) * span;
    float index = floor(x);
    return mix(colors[int(mod(index, count))], colors[int(mod(index + 1.0, count))], x - index);
}
