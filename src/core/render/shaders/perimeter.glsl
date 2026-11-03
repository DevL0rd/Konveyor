const float PI = 3.14159265359;
const float TAU = 6.28318530718;
const float QUARTER = 1.57079632679;
float perimeterLength()
{
    return 4.0 * (halfSize.x + halfSize.y) + (QUARTER - 2.0) * (radii.x + radii.y + radii.z + radii.w);
}
float perimeterFraction(vec2 p)
{
    vec2 b = halfSize;
    vec4 r = radii;
    float top = 2.0 * b.x - r.x - r.y;
    float right = 2.0 * b.y - r.y - r.z;
    float bottom = 2.0 * b.x - r.z - r.w;
    float left = 2.0 * b.y - r.w - r.x;
    float atTopRight = top;
    float atRight = atTopRight + QUARTER * r.y;
    float atBottomRight = atRight + right;
    float atBottom = atBottomRight + QUARTER * r.z;
    float atBottomLeft = atBottom + bottom;
    float atLeft = atBottomLeft + QUARTER * r.w;
    float atTopLeft = atLeft + left;
    vec2 c = vec2(b.x - r.y, r.y - b.y);
    float s;
    if (p.x > c.x && p.y < c.y) {
        s = atTopRight + r.y * atan(p.x - c.x, c.y - p.y);
    } else if (p.x > b.x - r.z && p.y > b.y - r.z) {
        s = atBottomRight + r.z * atan(p.y - b.y + r.z, p.x - b.x + r.z);
    } else if (p.x < r.w - b.x && p.y > b.y - r.w) {
        s = atBottomLeft + r.w * atan(r.w - b.x - p.x, p.y - b.y + r.w);
    } else if (p.x < r.x - b.x && p.y < r.x - b.y) {
        s = atTopLeft + r.x * atan(r.x - b.y - p.y, r.x - b.x - p.x);
    } else if (abs(p.y) - b.y > abs(p.x) - b.x) {
        s = p.y < 0.0 ? p.x + b.x - r.x : atBottom + b.x - r.z - p.x;
    } else {
        s = p.x > 0.0 ? atRight + p.y + b.y - r.y : atLeft + b.y - r.w - p.y;
    }
    return s / perimeterLength();
}
float band(float d, float width)
{
    return clamp(d * pixels + 0.5, 0.0, 1.0) * clamp((width - d) * pixels + 0.5, 0.0, 1.0);
}
vec4 tinted(vec4 color, float coverage)
{
    return vec4(color.rgb, color.a * coverage);
}
vec4 cometLayer(vec2 p, float d)
{
    float count = params.x;
    float x = fract((phase - perimeterFraction(p)) * count * params.z);
    float behind = x / count / max(params.y, 0.001);
    if (behind >= 1.0) {
        return vec4(0.0);
    }
    float head = clamp((1.0 - x) / count * perimeterLength() / max(bandWidth, 1.0), 0.0, 1.0);
    float fade = 1.0 - behind;
    return tinted(palette(behind, false), fade * fade * head * band(d, bandWidth));
}
