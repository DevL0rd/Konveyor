float bandAt(float index)
{
    return audioBands[int(index / 4.0)][int(mod(index, 4.0))];
}
float bandLevel(float t)
{
    float x = clamp(t, 0.0, 1.0) * 15.0;
    float index = min(floor(x), 14.0);
    return mix(bandAt(index), bandAt(index + 1.0), x - index);
}
float waveAt(float index)
{
    return audioWave[int(index / 4.0)][int(mod(index, 4.0))];
}
float fromBottom(float along)
{
    float distance = along - perimeterFraction(vec2(0.0, halfSize.y));
    return abs(fract(distance + 0.5) - 0.5) * 2.0;
}
float noise(float seed)
{
    return fract(sin(seed * 12.9898) * 43758.5453);
}
float lit(float value)
{
    return reactive.x + (1.0 - reactive.x) * clamp(value, 0.0, 1.0);
}
vec4 equalizerLayer(vec2 p, float d)
{
    float bars = max(params.x, 1.0);
    float pitch = perimeterLength() / bars;
    float x = perimeterFraction(p) * bars;
    float index = floor(x);
    float value = bandLevel(fromBottom((index + 0.5) / bars));
    float solid = max(pitch - params.y, 1.0);
    float position = (x - index) * pitch;
    float bar = clamp(min(position, solid - position) * pixels + 0.5, 0.0, 1.0);
    float height = max(bandWidth * max(value, reactive.x * 0.6), 1.0);
    return tinted(palette(d / max(bandWidth, 1.0), false), bar * band(d, height) * lit(value));
}
vec4 waveformLayer(vec2 p, float d)
{
    float along = perimeterFraction(p);
    float x = fromBottom(along) * 63.0;
    float index = min(floor(x), 62.0);
    float first = waveAt(index);
    float second = waveAt(index + 1.0);
    float amplitude = bandWidth * 0.5;
    float slope = (second - first) * amplitude * 126.0 / max(perimeterLength(), 1.0);
    float center = amplitude + amplitude * mix(first, second, smoothstep(0.0, 1.0, x - index));
    float gap = abs(d - center) / sqrt(1.0 + slope * slope);
    float line = max(reactive.w * 0.5, 0.5);
    float stroke = clamp((line - gap) * pixels + 0.5, 0.0, 1.0);
    float halo = exp(-gap * gap / (line * line * 8.0)) * 0.35;
    float strength = lit(abs(first) * 2.0);
    return tinted(palette(along, true), max(stroke, halo) * clamp(d * pixels + 0.5, 0.0, 1.0) * strength);
}
vec4 spectrumLayer(vec2 p, float d)
{
    float t = fromBottom(perimeterFraction(p));
    float value = bandLevel(t);
    return tinted(palette(t, false), band(d, bandWidth * (0.55 + 0.45 * value)) * lit(value));
}
vec4 shimmerLayer(vec2 p, float d)
{
    float level = reactive.x;
    float along = perimeterFraction(p);
    float cells = max(round(perimeterLength() / 3.0), 1.0);
    float cell = floor(along * cells);
    float seed = noise(cell + 1.0);
    float rate = 1.0 + floor(noise(cell + 7.0) * 3.0);
    float twinkle = pow(max(sin(TAU * (phase * rate + seed)), 0.0), 6.0);
    float shown = noise(cell + 13.0) < 0.15 + 0.75 * level ? 1.0 : 0.0;
    float x = fract(along * cells);
    float dotShape = clamp(min(x, 1.0 - x) * 3.0 * pixels + 0.5, 0.0, 1.0);
    float sparkle = shown * twinkle * dotShape * band(d, bandWidth * (0.4 + 0.6 * seed)) * (0.2 + 0.8 * level);
    float rest = band(d, max(bandWidth * 0.25, 1.0)) * 0.18;
    return tinted(palette(seed, false), max(sparkle, rest));
}
vec4 bassThumpLayer(float d)
{
    float level = clamp(reactive.x, 0.0, 1.0);
    return tinted(palette(level, false), band(d, bandWidth * (0.45 + 1.55 * level * level)) * (0.3 + 0.7 * level));
}
vec4 speedLinesLayer(vec2 p, float d)
{
    float level = clamp(reactive.x, 0.0, 1.0);
    float lane = floor(clamp(d / max(bandWidth, 1.0), 0.0, 0.99) * 3.0);
    float count = max(params.x, 1.0) * (1.0 + lane * 0.5);
    float x = fract(perimeterFraction(p) * count - phase * count + noise(lane + 3.0));
    float span = max(params.y * level, 0.001);
    float streak = x < span ? 1.0 - x / span : 0.0;
    float shown = noise(floor(perimeterFraction(p) * count - phase * count) + lane * 17.0) < 0.3 + 0.7 * level ? 1.0 : 0.0;
    return tinted(palette(x / span, false), streak * shown * band(d, bandWidth) * level);
}
vec4 scrollTrailLayer(vec2 p, float d)
{
    float level = clamp(reactive.x, 0.0, 1.0);
    float behind = fract((reactive.y - perimeterFraction(p)) * reactive.z) / max(params.y, 0.001);
    float rest = 0.1 * level * band(d, bandWidth);
    if (behind >= 1.0) {
        return tinted(palette(1.0, false), rest);
    }
    float fade = 1.0 - behind;
    return tinted(palette(behind, false), max(fade * fade * (0.25 + 0.75 * level) * band(d, bandWidth), rest));
}
float descent(vec2 p)
{
    return 1.0 - fromBottom(perimeterFraction(p));
}
float stroked(float gap, float line)
{
    return clamp((line - gap) * pixels + 0.5, 0.0, 1.0);
}
vec4 bulgeLayer(vec2 p, float d)
{
    float level = clamp(reactive.x, 0.0, 1.0);
    float x = fract(descent(p) * max(params.x, 1.0) - phase) - 0.5;
    float bump = 0.5 + 0.5 * cos(PI * clamp(x / max(params.y, 0.02), -1.0, 1.0));
    float rest = min(reactive.w, bandWidth);
    float height = rest + (bandWidth - rest) * sqrt(level) * bump;
    return tinted(palette(bump, false), band(d, height));
}
vec4 wavyLayer(vec2 p, float d)
{
    float level = clamp(reactive.x, 0.0, 1.0);
    float along = descent(p);
    float cycles = max(params.x, 1.0);
    float angle = TAU * (along * cycles - phase);
    float amplitude = bandWidth * 0.5 * sqrt(level);
    float line = max(reactive.w * 0.5, 0.5);
    float center = line + amplitude * (1.0 + sin(angle));
    float slope = amplitude * cos(angle) * TAU * cycles * 2.0 / max(perimeterLength(), 1.0);
    float gap = abs(d - center) / sqrt(1.0 + slope * slope);
    float halo = exp(-gap * gap / (line * line * 6.0)) * 0.4;
    return tinted(palette(along, false), max(stroked(gap, line), halo) * clamp(d * pixels + 0.5, 0.0, 1.0));
}
vec4 elasticLayer(vec2 p, float d)
{
    float stretch = reactive.x;
    float across = clamp(p.x / max(halfSize.x, 1.0), -1.0, 1.0);
    float toward = clamp(sign(stretch) * p.y / max(halfSize.y, 1.0), 0.0, 1.0);
    float reach = bandWidth * abs(stretch) * smoothstep(0.0, 1.0, toward) * (1.0 - 0.45 * across * across);
    float line = max(reactive.w * 0.5, 0.5);
    float gap = abs(d - line - reach);
    float fill = d < reach ? 0.55 * d / max(reach, 1.0) : 0.0;
    return tinted(palette(d / max(reach + line, 1.0), false), max(stroked(gap, line), fill) * clamp(d * pixels + 0.5, 0.0, 1.0));
}
vec4 chevronsLayer(vec2 p, float d)
{
    float level = clamp(reactive.x, 0.0, 1.0);
    float corner = cornerRadius(p);
    float ends = smoothstep(0.0, bandWidth, halfSize.y - corner - abs(p.y));
    float pitch = 2.0 * halfSize.y / max(params.x, 1.0);
    float cell = fract(p.y / pitch - phase);
    float u = clamp(d / max(bandWidth, 1.0), 0.0, 1.0) * 2.0 - 1.0;
    float arm = reactive.z * (cell - 0.5) * pitch + abs(u) * bandWidth * 0.5 - bandWidth * 0.25;
    float stroke = stroked(abs(arm) * 0.7071, max(reactive.w * 0.5, 0.5));
    return tinted(palette(abs(u), false), stroke * ends * band(d, bandWidth) * step(halfSize.x, abs(p.x)) * sqrt(level));
}
vec4 edgeFlashLayer(vec2 p, float d)
{
    float level = clamp(reactive.x, 0.0, 1.0);
    float edge = reactive.z * p.y / max(halfSize.y, 1.0);
    float along = smoothstep(1.0 - 2.0 * max(params.y, 0.01), 1.0, edge);
    float falloff = clamp(1.0 - d / max(bandWidth, 1.0), 0.0, 1.0);
    return tinted(palette(1.0 - falloff, false), band(d, bandWidth) * along * pow(falloff, 0.6) * sqrt(level));
}
vec4 gaugeLayer(vec2 p, float d)
{
    float travel = 2.0 * halfSize.y;
    float size = max(params.y, 0.02) * travel;
    float y = (reactive.y - 0.5) * (travel - size);
    float radius = bandWidth * 0.5;
    vec2 q = abs(vec2(d - radius, p.y - y)) - vec2(radius, size * 0.5) + radius;
    float pill = length(max(q, vec2(0.0))) + min(max(q.x, q.y), 0.0) - radius;
    float inner = cornerRadius(p);
    float track = band(d, min(2.0, bandWidth)) * 0.25 * step(abs(p.y), halfSize.y - inner);
    float shown = clamp(-pill * pixels + 0.5, 0.0, 1.0) * step(0.0, p.x);
    return tinted(palette(reactive.y, false), max(shown, track * step(halfSize.x, p.x)));
}
vec4 scrollKindLayer(vec2 p, float d)
{
    if (kind == 7) {
        return bulgeLayer(p, d);
    } else if (kind == 8) {
        return wavyLayer(p, d);
    } else if (kind == 9) {
        return elasticLayer(p, d);
    } else if (kind == 10) {
        return chevronsLayer(p, d);
    } else if (kind == 11) {
        return edgeFlashLayer(p, d);
    }
    return gaugeLayer(p, d);
}
void main()
{
    vec2 p = attributes.xy;
    float d = frameDistance(p) - inset;
    vec4 color;
    if (kind == 0) {
        color = equalizerLayer(p, d);
    } else if (kind == 1) {
        color = waveformLayer(p, d);
    } else if (kind == 2) {
        color = spectrumLayer(p, d);
    } else if (kind == 3) {
        color = shimmerLayer(p, d);
    } else if (kind == 4) {
        color = bassThumpLayer(d);
    } else if (kind == 5) {
        color = speedLinesLayer(p, d);
    } else if (kind == 6) {
        color = scrollTrailLayer(p, d);
    } else {
        color = scrollKindLayer(p, d);
    }
    fragColor = finish(color);
}
