#pragma once
namespace octet {
  namespace scene {
    /// Particle system: billboards, trails and cloth.
    /// Note all particles in the system must use the same material, but you
    /// can use a custom shader to select different effects.
    class fire_particle_system : public mesh_particle_system {
    public:
      struct fire_particle : billboard_particle {
        vec2p scale_size;             /// half-size in world space
      };


    private:
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


    public:
      RESOURCE_META(fire_particle_system)

      /// Default constructor
      fire_particle_system(aabb_in size = aabb(vec3(0, 0, 0), vec3(1, 1, 1)), int bbcap = 256, int tpcap = 256, int pacap = 256) {
        init(size, bbcap, tpcap, pacap);
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


      /// Update the vertices for newtonian physics.
      void animate(float time_step) {
        for (unsigned i = 0; i != particle_animators.size(); ++i) {
          particle_animator &g = particle_animators[i];
          if (g.link >= 0) {
            billboard_particle &p = billboard_particles[g.link];
            if (g.age >= g.lifetime) {
              p.enabled = false;
              free(billboard_particles, free_billboard_particle, g.link);
              g.link = -1;
              free(particle_animators, free_particle_animator, i);
            }
            else {
              p.pos = (vec3)p.pos + (vec3)g.vel * time_step;
              g.vel = (vec3)g.vel + (vec3)g.acceleration * time_step;
              p.angle += (uint32_t)(g.spin * time_step);
              p.size = get_size_for_age(g.age, g.lifetime) * 5.0f;
              //p.size = vec2p(3.0f, 3.0f);
              g.age++;
            }
          }
        }
      }

      virtual void update() {
        //unsigned np = billboard_particles.size();
        //unsigned vsize = billboard_particles.capacity() * sizeof(vertex) * 4;
        //unsigned isize = billboard_particles.capacity() * sizeof(uint32_t) * 4;

        gl_resource::wolock vlock(get_vertices());
        vertex *vtx = (vertex*)vlock.u8();
        gl_resource::wolock ilock(get_indices());
        uint32_t *idx = ilock.u32();
        unsigned num_vertices = 0;
        unsigned num_indices = 0;

        vec3 cx = cameraToWorld.x().xyz();
        vec3 cy = cameraToWorld.y().xyz();
        vec3 n = cameraToWorld.z().xyz();
        mat4t transform;
        for (unsigned i = 0; i != billboard_particles.size(); ++i) {
          billboard_particle &p = billboard_particles[i];
          if (p.enabled) {
            vec2 size = p.size;
            vec3 dx = size.x() * cx;
            vec3 dy = size.y() * cy;
            vec2 bl = p.uv_bottom_left;
            vec2 tr = p.uv_top_right;
            vec2 tl = vec2(bl.x(), tr.y());
            vec2 br = vec2(tr.x(), bl.y());

            vtx->pos = (vec3)p.pos - dx + dy; vtx->normal = n; vtx->uv = tl; vtx++;
            vtx->pos = (vec3)p.pos + dx + dy; vtx->normal = n; vtx->uv = tr; vtx++;
            vtx->pos = (vec3)p.pos + dx - dy; vtx->normal = n; vtx->uv = br; vtx++;
            vtx->pos = (vec3)p.pos - dx - dy; vtx->normal = n; vtx->uv = bl; vtx++;

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
    
    };
  }
}