#pragma once

#include "fluid_simulator.h"

namespace octet {
  namespace scene {
    /// Particle system: billboards, trails and cloth.
    /// Note all particles in the system must use the same material, but you
    /// can use a custom shader to select different effects.
    class fire_particle_system : public mesh_particle_system {
    public:
      struct fire_vertex : vertex {
        vec4 color;
      };

      struct fire_billboard_particle : billboard_particle {
        vec4 color;
      };
    private:
      fluid_simulator fluid_sim;

      vec4 concat_atlas_uvs[4];
      bool using_atlas_;

      vec3 applyTransform(mat4t trans, vec3 pos_in) {
        vec4 out = trans * vec4(pos_in, 0);
        return vec3(out[0], out[1], out[2]);
      }

      float smoothstep(float edge0, float edge1, float x)
      {
        // Scale, bias and saturate x to 0..1 range
        x = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
        // Evaluate polynomial
        return x*x*(3 - 2 * x);
      }

      float clamp(float x, float lowerlimit, float upperlimit)
      {
        if (x < lowerlimit) x = lowerlimit;
        if (x > upperlimit) x = upperlimit;
        return x;
      }


      random rand;

      dynarray<fire_billboard_particle> fire_billboard_particles;


    protected:
      void init(const aabb &size, int bbcap, int tpcap, int pacap) {

        fluid_sim.init(40);

        add_attribute(attribute_pos, 3, GL_FLOAT, 0);
        add_attribute(attribute_normal, 3, GL_FLOAT, 12);
        add_attribute(attribute_uv, 2, GL_FLOAT, 24);
        add_attribute(attribute_color, 4, GL_FLOAT, 32);
        set_params(48, 0, 0, GL_TRIANGLES, GL_UNSIGNED_INT);

        set_aabb(size);
        fire_billboard_particles.reserve(bbcap);
        trail_particles.reserve(tpcap);
        particle_animators.reserve(pacap);
        free_billboard_particle = -1;
        free_trail_particle = -1;
        free_particle_animator = -1;

        unsigned vsize = (bbcap * 4 + tpcap * 2) * sizeof(fire_vertex);
        unsigned isize = (bbcap * 6 + tpcap * 6) * sizeof(uint32_t);
        mesh::allocate(vsize, isize);
      }


    public:
      RESOURCE_META(fire_particle_system)

      /// Default constructor
      fire_particle_system(aabb_in size = aabb(vec3(0, 0, 0), vec3(1, 1, 1)), int bbcap = 256, int tpcap = 256, int pacap = 256, bool using_atlas = false) {
        init(size, bbcap, tpcap, pacap);
        concat_atlas_uvs[0] = vec4(0.0f, 1.0f, 0.5f, 0.5f);
        concat_atlas_uvs[1] = vec4(0.0f, 0.5f, 0.5f, 0.0f);
        concat_atlas_uvs[2] = vec4(0.5f, 1.0f, 1.0f, 0.5f);
        concat_atlas_uvs[3] = vec4(0.5f, 0.5f, 1.0f, 0.0f);
        using_atlas_ = using_atlas;
      }

      vec2 get_size_for_age(uint32_t age_, uint32_t lifetime_) {
        float lifeStep = (float)lifetime_ / 3.0f;
        float age = (float)age_;
        float ss;
        if (age < lifeStep) {
          ss = smoothstep(0, lifeStep, age);
        }
        if (age >= lifeStep && age < 2 * lifeStep) {
          ss = (1.0f - smoothstep(lifeStep, 2.0f * lifeStep, age)) * 0.5f + 0.5f;
        }
        if (age >= 2 * lifeStep) {
          ss = (1.0f - smoothstep(2.0f * lifeStep, (float)lifetime_, age)) * 0.5f;
        }
        return vec2(ss, ss);

        // Linear
        //if (age < lifetime / 3) {
        //  uint32_t end = lifetime / 3;
        //  float size = (float)age / (float)end;
        //  return vec2p(size, size);
        //}
        //return vec2p(1, 1);
      }

      vec4 get_color_for_age(uint32_t age_, uint32_t lifetime_) {
        float lifeStep = (float)lifetime_ * 0.2f;
        float age = (float)age_;
        float alpha;
        if (age < lifeStep) {
          alpha = smoothstep(0.0f, lifeStep, age);
        }
        else if (age > (float)lifetime_ - lifeStep) {
          alpha = 1.0f - smoothstep((float)lifetime_ - lifeStep, (float)lifetime_, age);
        }
        else {
          alpha = 1.0f;
        }
        /*if (age >= lifeStep && age < 2 * lifeStep) {
          ss = (1.0f - smoothstep(lifeStep, 2.0f * lifeStep, age)) * 0.5f + 0.5f;
        }
        if (age >= 2 * lifeStep) {
          ss = (1.0f - smoothstep(2.0f * lifeStep, (float)lifetime_, age)) * 0.5f;
        }*/
        return vec4(0.0f, 0.0f, 1.0f, alpha);
      }

      /// Update the vertices for newtonian physics.
      void animate(float time_step) {
        fluid_sim.update(time_step);

        vec3 wind = vec3(0.1f, 0.0f, 0.0f);
        vec3 wind_turb = vec3(rand.get(-1.0f, 1.0f), rand.get(-0.3f, 0.3f), rand.get(-1.0f, 1.0f));
        for (unsigned i = 0; i != particle_animators.size(); ++i) {
          particle_animator &g = particle_animators[i];
          if (g.link >= 0) {
            fire_billboard_particle &p = fire_billboard_particles[g.link];
            if (g.age >= g.lifetime) {
              p.enabled = false;
              free(fire_billboard_particles, free_billboard_particle, g.link);
              g.link = -1;
              free(particle_animators, free_particle_animator, i);
            }
            else {
              p.pos = (vec3)p.pos + (vec3)g.vel * time_step;
              g.vel = (vec3)g.vel + (vec3)g.acceleration * time_step;
              //g.vel = (vec3)g.vel + wind_turb*0.3f + wind;
              p.angle += (uint32_t)(g.spin * time_step);
              p.size = get_size_for_age(g.age, g.lifetime) * 5.0f;
              p.color = get_color_for_age(g.age, g.lifetime);
              //p.size = vec2p(3.0f, 3.0f);
              g.age++;

              if (using_atlas_) {
                float change_uvs = rand.get(0.0f, 1.0f);
                if (change_uvs > 0.99f){
                  float tex_toggle = rand.get(0.0f, 1.0f);
                  if (tex_toggle < 0.25f) {
                    p.uv_bottom_left = vec2p(concat_atlas_uvs[0][0], concat_atlas_uvs[0][1]);
                    p.uv_top_right = vec2p(concat_atlas_uvs[0][2], concat_atlas_uvs[0][3]);
                  }
                  else if (tex_toggle < 0.5f) {
                    p.uv_bottom_left = vec2p(concat_atlas_uvs[1][0], concat_atlas_uvs[1][1]);
                    p.uv_top_right = vec2p(concat_atlas_uvs[1][2], concat_atlas_uvs[1][3]);
                  }
                  else if (tex_toggle < 0.75f) {
                    p.uv_bottom_left = vec2p(concat_atlas_uvs[2][0], concat_atlas_uvs[2][1]);
                    p.uv_top_right = vec2p(concat_atlas_uvs[2][2], concat_atlas_uvs[2][3]);
                  }
                  else {
                    p.uv_bottom_left = vec2p(concat_atlas_uvs[3][0], concat_atlas_uvs[3][1]);
                    p.uv_top_right = vec2p(concat_atlas_uvs[3][2], concat_atlas_uvs[3][3]);
                  }
                }
              }
            }
          }
        }
      }

      virtual void update() {
        //unsigned np = billboard_particles.size();
        //unsigned vsize = billboard_particles.capacity() * sizeof(vertex) * 4;
        //unsigned isize = billboard_particles.capacity() * sizeof(uint32_t) * 4;
        gl_resource::wolock vlock(get_vertices());
        fire_vertex *vtx = (fire_vertex*)vlock.u8();
        gl_resource::wolock ilock(get_indices());
        uint32_t *idx = ilock.u32();
        unsigned num_vertices = 0;
        unsigned num_indices = 0;

        vec3 cx = cameraToWorld.x().xyz();
        vec3 cy = cameraToWorld.y().xyz();
        vec3 n = cameraToWorld.z().xyz();
        mat4t transform;
        for (unsigned i = 0; i != fire_billboard_particles.size(); ++i) {
          fire_billboard_particle &p = fire_billboard_particles[i];
          if (p.enabled) {
            vec2 size = p.size;
            vec3 dx = size.x() * cx;
            vec3 dy = size.y() * cy;
            vec2 bl = p.uv_bottom_left;
            vec2 tr = p.uv_top_right;
            vec2 tl = vec2(bl.x(), tr.y());
            vec2 br = vec2(tr.x(), bl.y());

            vtx->pos = (vec3)p.pos - dx + dy; vtx->normal = n; vtx->uv = tl; vtx->color = p.color; vtx++;
            vtx->pos = (vec3)p.pos + dx + dy; vtx->normal = n; vtx->uv = tr; vtx->color = p.color; vtx++;
            vtx->pos = (vec3)p.pos + dx - dy; vtx->normal = n; vtx->uv = br; vtx->color = p.color; vtx++;
            vtx->pos = (vec3)p.pos - dx - dy; vtx->normal = n; vtx->uv = bl; vtx->color = p.color; vtx++;

            idx[0] = num_vertices; idx[1] = num_vertices + 1; idx[2] = num_vertices + 2;
            idx[3] = num_vertices; idx[4] = num_vertices + 2; idx[5] = num_vertices + 3;
            idx += 6;
            num_vertices += 4;
            num_indices += 6;
          }
        }

        set_num_vertices(num_vertices);
        set_num_indices(num_indices);
        //dump(log("mesh\n"));
      }
    

      /// Add a billboard particle. Returns -1 if capacity reached.
      int add_billboard_particle(const fire_billboard_particle &p) {
        int i = allocate(fire_billboard_particles, free_billboard_particle);
        if (i != -1) {
          fire_billboard_particles[i] = p;
        }
        return i;
      }
    };
  }
}