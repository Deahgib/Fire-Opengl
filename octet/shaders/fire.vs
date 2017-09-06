//////////////////////////////////////////////////////////////////////////////////////////
//
// Default vertex shader for materials. Extend this to deal with bump mapping, defered rendering, shadows etc.
//
#version 150
uniform mat4 modelToProjection;
uniform mat4 modelToCamera;
uniform float overlay_half_size;
uniform vec3 overlay_bl;


in vec3 pos;
in vec2 uv;
in vec3 normal;
in vec4 color;

void main(void) {
 gl_Position = vec4(pos, 0.0);
 //gl_Position = vec4(0.0, 0.0, 0.0, 0.0);
}
