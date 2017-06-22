//////////////////////////////////////////////////////////////////////////////////////////
//
// Default vertex shader for materials. Extend this to deal with bump mapping, defered rendering, shadows etc.
//

// matrices 
uniform mat4 modelToProjection;
uniform mat4 modelToCamera;

uniform float overlay_half_size;
uniform vec3 overlay_bl;

// attributes from vertex buffer
attribute vec4 pos;
attribute vec2 uv;
attribute vec3 normal;
attribute vec4 color;

// outputs
varying vec3 normal_;
varying vec2 overlay_uv_;
varying vec2 uv_;
varying vec4 color_;
varying vec3 model_pos_;
varying vec3 camera_pos_;

void main() {
  gl_Position = modelToProjection * pos;
  vec3 tnormal = (modelToCamera * vec4(normal, 0.0)).xyz;
  vec3 tpos = (modelToCamera * pos).xyz;
  normal_ = tnormal;
  uv_ = uv;
  color_ = color;
  camera_pos_ = tpos;
  model_pos_ = pos.xyz;

  //overlay_uv_ = vec2(0.5, 0.5);
  //overlay_uv_ = mod((pos.xy - overlay_bl), vec2(2*overlay_half_size, 2*overlay_half_size)) / vec2(2*overlay_half_size, 2*overlay_half_size);

  //vec3 ov_norm = camera_pos_ - (overlay_bl + vec3(overlay_half_size, overlay_half_size, overlay_bl.z));

  vec3 v = pos.xyz - overlay_bl;
  float dist = dot(v, normal_);
  vec3 proj_point = pos.xyz - dist * normal_;

  overlay_uv_ = (proj_point.xy - overlay_bl.xy) / vec2(2.0*overlay_half_size, 2.0*overlay_half_size);

  //overlay_uv_ = mod((pos.xy - overlay_bl), vec2(2.0*overlay_half_size, 2.0*overlay_half_size)) / vec2(2.0*overlay_half_size, 2.0*overlay_half_size);

  //vec3 ov_lat_vec = normalize(cross(vec3(0,1,0), normal_)) * overlay_half_size;
  //vec3 ov_bl = overlay_center - ov_lat_vec - vec(0,overlay_half_size,0);
  //vec3 ov_proj_relative = proj_point - ov_bl;
  //vec3 rel_2d_proj = ov_proj_relative / vec3(2*overlay_half_size,2*overlay_half_size,0.0);
  //overlay_uv_ = rel_2d_proj.xy;
}

