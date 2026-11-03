#version 140
in vec4 position;
in vec4 texcoord;
out vec4 attributes;
out vec2 framePosition;
uniform mat4 modelViewProjectionMatrix;
#define frameMatrix modelViewProjectionMatrix
