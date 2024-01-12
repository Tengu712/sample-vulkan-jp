#version 450

layout(binding=0) uniform Camera {
    mat4 proj;
} camera;

layout(push_constant) uniform PushConstant {
    vec4 scl;
    vec4 trs;
    vec4 uv;
} constant;

layout(location=0) in vec3 inPos;
layout(location=1) in vec3 inUV;

void main() {
    vec4 pos = vec4(inPos, 1.0);
    vec4 scl = vec4(constant.scl.xyz, 1.0);
    vec4 trs = vec4(constant.trs.xyz, 0.0);
    pos *= scl;
    pos += trs;
    pos *= camera.proj;
    gl_Position = pos;
}
