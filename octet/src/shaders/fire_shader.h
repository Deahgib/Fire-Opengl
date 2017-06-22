////////////////////////////////////////////////////////////////////////////////
//
// (C) Andy Thomason 2012-2014
//
// Modular Framework for OpenGLES2 rendering on multiple platforms.
//
// Bump shader that uses textures for all material channels

namespace octet {
  namespace shaders {
    class fire_shader : public shader {
      // indices to use with glUniform*()

      GLuint modelToProjection_index; // index for model space to projection space matrix
      GLuint modelToCamera_index;     // second matrix used for lighting maps model to camera space
      GLuint cameraToProjection_index;// used in skinned shader
      GLuint light_uniforms_index;    // lighting parameters for fragment shader
      GLuint num_lights_index;        // how many lights?
      GLuint samplers_index;          // index for texture samplers

      GLuint time_index;              // Current time

      void init_uniforms(const char *vertex_shader, const char *fragment_shader) {
        // use the common shader code to compile and link the shaders
        // the result is a shader program
        shader::init(vertex_shader, fragment_shader);

        // extract the indices of the uniforms to use later
        modelToProjection_index = glGetUniformLocation(program(), "modelToProjection");
        cameraToProjection_index = glGetUniformLocation(program(), "cameraToProjection");
        modelToCamera_index = glGetUniformLocation(program(), "modelToCamera");
        light_uniforms_index = glGetUniformLocation(program(), "light_uniforms");
        num_lights_index = glGetUniformLocation(program(), "num_lights");
        samplers_index = glGetUniformLocation(program(), "samplers");
        time_index = glGetUniformLocation(program(), "time");
      }

    public:
      void init(bool is_skinned = false) {
        // this is the vertex shader for regular geometry
        // it is called for each corner of each triangle
        // it inputs pos and uv from each corner
        // it outputs gl_Position, normal_ and uv_ to the rasterizer
        // normal_ is the normal in camera space to make calculations easier
        const char vertex_shader[] = SHADER_STR(
          // VERTEX SHADER -------

          // matrices
          uniform mat4 modelToProjection;
          uniform mat4 modelToCamera;

          // attributes from vertex buffer
          attribute vec4 pos;
          attribute vec2 uv;
          attribute vec3 normal;
          attribute vec4 color;

          // outputs
          varying vec3 normal_;
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
          }

          // VERTEX SHADER END -------
        );

        // this is the fragment shader
        // after the rasterizer breaks the triangle into fragments
        // this is called for every fragment
        // it outputs gl_FragColor, the color of the pixel and inputs normal_ and uv_
        // the four samplers give emissive, diffuse, specular and ambient colors
        const char fragment_shader[] = SHADER_STR(
          // constant parameters
          uniform vec4 lighting[17];
          uniform int num_lights;
          uniform sampler2D diffuse_sampler;

          // inputs
          varying vec3 normal_;
          varying vec3 camera_pos_;
          varying vec2 uv_;
          varying vec4 color_;
          varying vec3 model_pos_;

          void main() {
            vec4 diffuse = texture2D(diffuse_sampler, uv_);
            vec3 nnormal = normalize(normal_);
            vec3 npos = camera_pos_;
            vec3 diffuse_light = lighting[0].xyz;
            for (int i = 0; i != num_lights; ++i) {
              vec3 light_pos = lighting[i * 4 + 1].xyz;
              vec3 light_direction = lighting[i * 4 + 2].xyz;
              vec3 light_color = lighting[i * 4 + 3].xyz;
              vec3 light_atten = lighting[i * 4 + 4].xyz;
              float diffuse_factor = max(dot(light_direction, nnormal), 0.0);
              diffuse_light += diffuse_factor * light_color;
            }
            gl_FragColor = vec4(diffuse.xyz * diffuse_light, 1.0);
          }


        );

        // use the common shader code to compile and link the shaders
        // the result is a shader program
        init_uniforms(vertex_shader, fragment_shader);
      }

      void render(const mat4t &modelToProjection, const mat4t &modelToCamera, const vec4 *light_uniforms, int num_light_uniforms, int num_lights) {
        // tell openGL to use the program
        shader::render();

        // customize the program with uniforms
        glUniformMatrix4fv(modelToProjection_index, 1, GL_FALSE, modelToProjection.get());
        glUniformMatrix4fv(modelToCamera_index, 1, GL_FALSE, modelToCamera.get());

        glUniform4fv(light_uniforms_index, num_light_uniforms, (float*)light_uniforms);
        glUniform1i(num_lights_index, num_lights);

        // we use textures 0-3 for material properties.
        static const GLint samplers[] = { 0, 1, 2, 3, 4, 5 };
        glUniform1iv(samplers_index, 6, samplers);
      }
    };
  }
}
