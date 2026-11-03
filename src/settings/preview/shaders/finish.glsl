vec4 finish(vec4 color)
{
    float alpha = color.a * layerOpacity * qt_Opacity;
    vec3 rgb = color.rgb * alpha;
    return vec4(rgb, mix(alpha, max(rgb.r, max(rgb.g, rgb.b)), additive));
}
