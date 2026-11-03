#version 440
layout(location = 0) in vec4 position;
layout(location = 1) in vec4 texcoord;
layout(location = 0) out vec4 attributes;
layout(location = 1) out vec2 framePosition;
#define frameMatrix qt_Matrix
