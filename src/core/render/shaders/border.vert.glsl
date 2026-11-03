void main()
{
    attributes = texcoord;
    framePosition = position.xy;
    gl_Position = frameMatrix * position;
}
