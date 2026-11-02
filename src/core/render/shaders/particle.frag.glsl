void main()
{
    float outside = clamp(frameDistance(framePosition / pixels - halfSize) * pixels + 0.5, 0.0, 1.0);
    float coverage = outside * (1.0 - smoothstep(0.35, 1.0, length(attributes.xy)));
    vec4 color = palette(attributes.z, false);
    fragColor = finish(vec4(color.rgb, color.a * coverage * attributes.w));
}
