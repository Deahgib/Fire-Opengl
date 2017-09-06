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
      // Fluid sim
      fluid_simulator fluid_sim;
      int x_length, y_length, z_length;
      int fs_size;
      float * u, *v, *w, *u_prev, *v_prev, *w_prev;
      float * dens, *dens_prev;
      float diffuse_rate;
      float viscosity;

      ref<mesh> fluid_sim_debug;
      ref<mesh> fluid_sim_vel_debug;
      // particle system
      vec4 concat_atlas_uvs[4];
      bool using_atlas_;
      random rand;
      dynarray<fire_billboard_particle> fire_billboard_particles;

      void init_fluid_sim() {
        x_length = 16;
        y_length = 32;
        z_length = 16;
        fluid_sim.init(x_length, y_length, z_length);
        fs_size = (x_length + 2) * (y_length + 2) * (z_length + 2);
        printf("size: %d \n", fs_size);
        allocate_data();
        clear_data();
        // 6 here from dt reversing: ( float a = dt*diff*max*max*max; )  for testing
        diffuse_rate = 0.0f;
        viscosity = 0.0f;

        fluid_sim_debug = new mesh();
        fluid_sim_debug->add_attribute(attribute_pos, 3, GL_FLOAT, 0);
        fluid_sim_debug->add_attribute(attribute_normal, 3, GL_FLOAT, 12);
        fluid_sim_debug->add_attribute(attribute_uv, 2, GL_FLOAT, 24);
        fluid_sim_debug->add_attribute(attribute_color, 4, GL_FLOAT, 32);
        fluid_sim_debug->set_params(48, 0, 0, GL_POINTS, GL_UNSIGNED_INT);

        uint32_t size = (x_length + 2) * (y_length + 2) * (z_length + 2);
        unsigned vsize = (size + 8) * sizeof(fire_vertex);
        unsigned isize = (size + 8) * sizeof(uint32_t);
        fluid_sim_debug->allocate(vsize, isize);

        fluid_sim_vel_debug = new mesh();
        fluid_sim_vel_debug->add_attribute(attribute_pos, 3, GL_FLOAT, 0);
        fluid_sim_vel_debug->add_attribute(attribute_normal, 3, GL_FLOAT, 12);
        fluid_sim_vel_debug->add_attribute(attribute_uv, 2, GL_FLOAT, 24);
        fluid_sim_vel_debug->add_attribute(attribute_color, 4, GL_FLOAT, 32);
        fluid_sim_vel_debug->set_params(48, 0, 0, GL_LINES, GL_UNSIGNED_INT);

        vsize = size * 2 * sizeof(fire_vertex);
        isize = size * 2 * sizeof(uint32_t);
        fluid_sim_vel_debug->allocate(vsize, isize);
        glPointSize(5.0f);
      }

      void free_data()
      {
        if (u) free(u);
        if (v) free(v);
        if (w) free(w);
        if (u_prev) free(u_prev);
        if (v_prev) free(v_prev);
        if (w_prev) free(w_prev);
        if (dens) free(dens);
        if (dens_prev) free(dens_prev);
      }

      void clear_data()
      {
        int i, size = (x_length + 2)*(y_length + 2)*(z_length + 2);

        for (i = 0; i < size; i++) {
          u[i] = v[i] = w[i] = u_prev[i] = v_prev[i] = w_prev[i] = dens[i] = dens_prev[i] = 0.0f;
        }
      }

      int allocate_data()
      {
        int size = (x_length + 2)*(y_length + 2)*(z_length + 2);

        u = (float *)malloc(size * sizeof(float));
        v = (float *)malloc(size * sizeof(float));
        w = (float *)malloc(size * sizeof(float));
        u_prev = (float *)malloc(size * sizeof(float));
        v_prev = (float *)malloc(size * sizeof(float));
        w_prev = (float *)malloc(size * sizeof(float));
        dens = (float *)malloc(size * sizeof(float));
        dens_prev = (float *)malloc(size * sizeof(float));

        if (!u || !v || !w || !u_prev || !v_prev || !w_prev || !dens || !dens_prev) {
          fprintf(stderr, "cannot allocate data\n");
          return (0);
        }

        return (1);
      }

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

      void get_velocity(const aabb bounds, vec3 pos, vec3 &vel, float &den) {
        if (bounds.intersects(pos)) {
          vec3 min = bounds.get_min();
          vec3 max = bounds.get_max();
          int x_idx = round((pos[0] - min[0]) / (max[0] - min[0]) * (float)x_length);
          int y_idx = round((pos[1] - min[1]) / (max[1] - min[1]) * (float)y_length);
          int z_idx = round((pos[2] - min[2]) / (max[2] - min[2]) * (float)z_length);
          vel[0] = u[IX(x_idx, y_idx, z_idx)];
          vel[1] = v[IX(x_idx, y_idx, z_idx)];
          vel[2] = w[IX(x_idx, y_idx, z_idx)];
          den = dens[IX(x_idx, y_idx, z_idx)];
          //printf("New vel: %f %f %f\n", vel.x(), vel.y(), vel.z());
        }
        else
        {
          printf("Out of bounds: %f %f %f\n", pos.x(), pos.y(), pos.z());
        }
      }



    protected:
      void init(const aabb &size, int bbcap, int tpcap, int pacap) {
        init_fluid_sim();

        add_attribute(attribute_pos, 3, GL_FLOAT, 0);
        add_attribute(attribute_normal, 3, GL_FLOAT, 12);
        add_attribute(attribute_uv, 2, GL_FLOAT, 24);
        add_attribute(attribute_color, 4, GL_FLOAT, 32);
        set_params(48, 0, 0, GL_POINTS, GL_UNSIGNED_INT);

        set_aabb(size);
        fire_billboard_particles.reserve(bbcap);
        trail_particles.reserve(tpcap);
        particle_animators.reserve(pacap);
        free_billboard_particle = -1;
        free_trail_particle = -1;
        free_particle_animator = -1;

        unsigned vsize = (bbcap) * sizeof(fire_vertex);
        unsigned isize = (bbcap) * sizeof(uint32_t);
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
          ss = 1.0f;
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
          alpha = smoothstep(lifeStep, lifeStep, age);
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
        return vec4(1.0f, 1.0f, 1.0f, alpha);
      }

      void clear_fluid_sim() {
        clear_data();
      }

      void addWind() {
        v_prev[IX(x_length / 2, y_length / 8, z_length / 2)] = 100.0f;
        dens_prev[IX(x_length / 2, y_length / 8, z_length / 2)] = 200.0f;
      }

      void animate(float time_step) {
        time_step *= 10.0f;
        //float t = rand.get(0.0f, 1.0f);
        //if (t < 0.01f) {
        //  //u_prev[IX(x_length / 2, y_length / 2, 3 * z_length / 4)] = -2000.0f;
        //}
        v_prev[IX(x_length / 2, 2, z_length / 2)] = 1.0f;
        dens_prev[IX(x_length / 2, 2, z_length / 2)] = 20.0f;
        fluid_sim.vel_step(x_length, y_length, z_length, u, v, w, u_prev, v_prev, w_prev, viscosity, time_step);
        fluid_sim.dens_step(x_length, y_length, z_length, dens, dens_prev, u, v, w, diffuse_rate, time_step);
        vec3 n_vel;
        float density = 0.0f;

        for (unsigned i = 0; i != particle_animators.size(); ++i) {
          particle_animator &g = particle_animators[i];
          if (g.link >= 0) {
            fire_billboard_particle &p = fire_billboard_particles[g.link];
            if (g.age >= g.lifetime) {
              p.enabled = false;
              free_particle(fire_billboard_particles, free_billboard_particle, g.link);
              g.link = -1;
              free_particle(particle_animators, free_particle_animator, i);
            }
            else {
              if (get_aabb().intersects(p.pos)) {
                get_velocity(get_aabb(), p.pos, n_vel, density);
                p.pos = (vec3)p.pos + n_vel;
                g.vel = n_vel;
              }
              else {
                p.pos = (vec3)p.pos + g.vel;
              }
              //p.angle += (uint32_t)(g.spin * time_step);
              p.size = get_size_for_age(g.age, g.lifetime) * 0.2f;
              p.color = vec4(density, density - 2.0f, density - 1.0f, (density < 0.8f)? density : 0.8f);
                //get_color_for_age(g.age, g.lifetime);
              g.age++;
            }
          }
        }

        for (int i = 0; i < fs_size; i++) {
          u_prev[i] = v_prev[i] = w_prev[i] = dens_prev[i] = 0.0f;
        }
      }

      void update_fluid_sim() {
        // DENSITIES
        {
          gl_resource::wolock vlock(fluid_sim_debug->get_vertices());
          fire_vertex *vtx = (fire_vertex*)vlock.u8();
          gl_resource::wolock ilock(fluid_sim_debug->get_indices());
          uint32_t *idx = ilock.u32();
          unsigned num_vertices = 0;
          unsigned num_indices = 0;

          vec3 origin_pos = get_aabb().get_min();
          vec3 dimentions = get_aabb().get_half_extent() * 2.0f;
          vec3 n = cameraToWorld.z().xyz();
          float x_step = dimentions[0] / (x_length + 2);
          float y_step = dimentions[1] / (y_length + 2);
          float z_step = dimentions[2] / (z_length + 2);
          for (int i = 0; i <= (x_length + 1); i++) {
            for (int j = 0; j <= (y_length + 1); j++) {
              for (int k = 0; k <= (z_length + 1); k++) {
                vtx->pos = vec3(origin_pos.x() + x_step * 0.5f + i * x_step, origin_pos.y() + y_step * 0.5f + j * y_step, origin_pos.z() + z_step * 0.5f + k * z_step);
                vtx->normal = n;
                vtx->uv = vec2p(0, 0);
                float density = dens[IX(i, j, k)];
                vtx->color = vec4(density, density - 2.0f, density - 1.0f, (density < 0.8f) ? density : 0.8f);
                vtx++;
                idx[0] = num_vertices;
                idx++;
                num_vertices++;
                num_indices++;
              }
            }
          }
          // AABB CORNERS IN GREEN
          vtx->pos = vec3p(origin_pos);                                                                                    /* | */ vtx->normal = n; vtx->uv = vec2p(0, 0); vtx->color = vec4(0.0f, 1.0f, 0.0f, 1.0f); vtx++;
          vtx->pos = vec3(origin_pos[0] + dimentions[0], origin_pos[1], origin_pos[2]);                                   /* | */ vtx->normal = n; vtx->uv = vec2p(0, 0); vtx->color = vec4(0.0f, 1.0f, 0.0f, 1.0f); vtx++;
          vtx->pos = vec3(origin_pos[0], origin_pos[1], origin_pos[2] + dimentions[2]);                                   /* | */ vtx->normal = n; vtx->uv = vec2p(0, 0); vtx->color = vec4(0.0f, 1.0f, 0.0f, 1.0f); vtx++;
          vtx->pos = vec3(origin_pos[0] + dimentions[0], origin_pos[1], origin_pos[2] + dimentions[2]);                  /* | */ vtx->normal = n; vtx->uv = vec2p(0, 0); vtx->color = vec4(0.0f, 1.0f, 0.0f, 1.0f); vtx++;
          vtx->pos = vec3(origin_pos[0], origin_pos[1] + dimentions[1], origin_pos[2]);                                   /* | */ vtx->normal = n; vtx->uv = vec2p(0, 0); vtx->color = vec4(0.0f, 1.0f, 0.0f, 1.0f); vtx++;
          vtx->pos = vec3(origin_pos[0] + dimentions[0], origin_pos[1] + dimentions[1], origin_pos[2]);                  /* | */ vtx->normal = n; vtx->uv = vec2p(0, 0); vtx->color = vec4(0.0f, 1.0f, 0.0f, 1.0f); vtx++;
          vtx->pos = vec3(origin_pos[0], origin_pos[1] + dimentions[1], origin_pos[2] + dimentions[2]);                  /* | */ vtx->normal = n; vtx->uv = vec2p(0, 0); vtx->color = vec4(0.0f, 1.0f, 0.0f, 1.0f); vtx++;
          vtx->pos = vec3(origin_pos[0] + dimentions[0], origin_pos[1] + dimentions[1], origin_pos[2] + dimentions[2]); /* | */ vtx->normal = n; vtx->uv = vec2p(0, 0); vtx->color = vec4(0.0f, 1.0f, 0.0f, 1.0f); vtx++;

          idx[0] = num_vertices;     idx[1] = num_vertices + 1; idx[2] = num_vertices + 2; idx[3] = num_vertices + 3;
          idx[4] = num_vertices + 4; idx[5] = num_vertices + 5; idx[6] = num_vertices + 6; idx[7] = num_vertices + 7;
          idx += 8;
          num_vertices += 8;
          num_indices += 8;

          fluid_sim_debug->set_num_vertices(num_vertices);
          fluid_sim_debug->set_num_indices(num_indices);
        }
        {
          // VELOCITIES
          gl_resource::wolock vlock(fluid_sim_vel_debug->get_vertices());
          fire_vertex *vtx = (fire_vertex*)vlock.u8();
          gl_resource::wolock ilock(fluid_sim_vel_debug->get_indices());
          uint32_t *idx = ilock.u32();
          unsigned num_vertices = 0;
          unsigned num_indices = 0;

          vec3 origin_pos = get_aabb().get_min();
          vec3 dimentions = get_aabb().get_half_extent() * 2.0f;
          vec3 n = cameraToWorld.z().xyz();
          float x_step = dimentions[0] / (x_length + 2);
          float y_step = dimentions[1] / (y_length + 2);
          float z_step = dimentions[2] / (z_length + 2);
          for (int i = 0; i <= (x_length + 1); i++) {
            for (int j = 0; j <= (y_length + 1); j++) {
              for (int k = 0; k <= (z_length + 1); k++) {
                vtx->pos = vec3(origin_pos.x() + x_step * 0.5f + i * x_step, origin_pos.y() + y_step * 0.5f + j * y_step, origin_pos.z() + z_step * 0.5f + k * z_step);
                vtx->normal = n;
                vtx->uv = vec2p(0, 0);
                vtx->color = vec4(0.5f, 0.5f, 1.0f, 1.0f);
                vtx++;
                vtx->pos = vec3(origin_pos.x() + x_step * 0.5f + i * x_step + u[IX(i,j,k)] , origin_pos.y() + y_step * 0.5f + j * y_step + v[IX(i, j, k)], origin_pos.z() + z_step * 0.5f + k * z_step + w[IX(i, j, k)]);
                vtx->normal = n;
                vtx->uv = vec2p(0, 0);
                vtx->color = vec4(0.5f, 0.5f, 1.0f, 1.0f);
                vtx++;

                idx[0] = num_vertices; idx[1] = num_vertices+1;
                idx += 2;
                num_vertices += 2;
                num_indices += 2;
              }
            }
          }
          fluid_sim_vel_debug->set_num_vertices(num_vertices);
          fluid_sim_vel_debug->set_num_indices(num_indices);
        }
      }

      virtual void update() {
        //unsigned np = billboard_particles.size();
        //unsigned vsize = billboard_particles.capacity() * sizeof(vertex) * 4;
        //unsigned isize = billboard_particles.capacity() * sizeof(uint32_t) * 4;
        unsigned num_vertices = 0;
        unsigned num_indices = 0;
        gl_resource::wolock vlock(get_vertices());
        fire_vertex *vtx = (fire_vertex*)vlock.u8();
        gl_resource::wolock ilock(get_indices());
        uint32_t *idx = ilock.u32();

        vec3 cx = cameraToWorld.x().xyz();
        vec3 cy = cameraToWorld.y().xyz();
        vec3 n = cameraToWorld.z().xyz();
        for (unsigned i = 0; i != fire_billboard_particles.size(); ++i) {
          fire_billboard_particle &p = fire_billboard_particles[i];
          if (p.enabled) {
            /*vec2 size = p.size;
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
            num_indices += 6;*/
            vtx->pos = p.pos; vtx->normal = n; vtx->color = p.color; vtx++;
            idx[0] = num_vertices;
            idx++;
            num_vertices++; num_indices++;
          }
        }

        set_num_vertices(num_vertices);
        set_num_indices(num_indices);
        //dump(log("mesh\n"));

        //render_debug();

      }
    

      /// Add a billboard particle. Returns -1 if capacity reached.
      int add_billboard_particle(const fire_billboard_particle &p) {
        int i = allocate(fire_billboard_particles, free_billboard_particle);
        if (i != -1) {
          fire_billboard_particles[i] = p;
        }
        return i;
      }

      ref<mesh> get_debug_mesh() {
        return fluid_sim_debug;
      }

      ref<mesh> get_debug_vel_mesh() {
        return fluid_sim_vel_debug;
      }

      
    };
  }
}