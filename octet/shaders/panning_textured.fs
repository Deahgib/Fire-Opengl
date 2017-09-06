//////////////////////////////////////////////////////////////////////////////////////////
//
// default frament shader for textures
//

#version 150
// constant parameters
uniform vec4 lighting[17];
uniform int num_lights;


uniform vec3 overlay_bl;

// Texture samplers
uniform sampler2D particle_diffuse;
uniform sampler2D particle_mask;
uniform sampler2D overlay_diffuse;
uniform sampler2D overlay_mask;
uniform sampler2D overlay_noise;

// Total elapsed time assuming 30 FPS ( TotalCurrentFrames / 30 )
uniform float time;

// inputs
in vec3 normal_;
in vec2 overlay_uv_;
in vec2 uv_;
in vec4 color_;
in vec3 model_pos_;
in vec3 camera_pos_;

out vec4 fragColor;

void main() {
  //vec4 diffuse = texture2D(particle_diffuse, vec2(uv_.x, uv_.y));
  //vec4 mask = texture2D(particle_mask, vec2(uv_.x, uv_.y));

  //vec4 overlay = texture2D(overlay_diffuse, vec2(overlay_uv_.x, overlay_uv_.y - time));
  //vec4 overlay_mask = texture2D(overlay_mask, vec2(overlay_uv_.x, overlay_uv_.y));
  //vec4 overlay_noise = texture2D(overlay_noise, vec2(overlay_uv_.x, overlay_uv_.y));
  //diffuse = vec4(max(diffuse.xyz, (overlay.xyz*0.7)), max(max(diffuse.xyz));
  
  
  
//  vec3 nnormal = normalize(normal_);
//  vec3 npos = camera_pos_;
//  vec3 diffuse_light = lighting[0].xyz;
//  for (int i = 0; i != num_lights; ++i) {
//    vec3 light_pos = lighting[i * 4 + 1].xyz;
//    vec3 light_direction = lighting[i * 4 + 2].xyz;
//    vec3 light_color = lighting[i * 4 + 3].xyz;
//    vec3 light_atten = lighting[i * 4 + 4].xyz;
//    float diffuse_factor = max(dot(light_direction, nnormal), 0.0);
//    diffuse_light += diffuse_factor * light_color;
//  }

//  if(overlay_mask.x > 0.8){
//	overlay = vec4(overlay.xyz, overlay_mask.x * overlay_noise.x);
//  }else{
//	overlay = vec4(overlay.xyz, overlay_mask.x * (overlay_noise.x * 0.03));
//  }


  //diffuse = mix(diffuse, overlay, 0.4);
  
  fragColor = vec4(color_/*.xzy, mask.x * color_.a*/);

}
