#pragma once
namespace octet {
  class fire_instance {
  private:
    ref<param_shader> fire_shader;
    ref<material> fire_material;

    //enum {
    //  particle_sampler_index = 0,
    //  diffuse_sampler_index,
    //  overlay_sampler_index
    //};

    // particle system
    ref<fire_particle_system> system;

    ref<param_uniform> time_index;

    ref<param_uniform> overlay_half_size_index;
    ref<param_uniform> overlay_bl_index;

    ref<param_sampler> particle_mask;
    ref<param_sampler> particle_diffuse;
    ref<param_sampler> overlay_diffuse;
    ref<param_sampler> overlay_mask;
    ref<param_sampler> overlay_noise;

    random* r;


    vec3 worldCoord;
  
    bool using_atlas;

  public:
    fire_instance(random* rand) {
      r = rand;
    }

    void init(vec3 pos) {
      fire_shader = new param_shader("shaders/fire.vs", "shaders/panning_textured.fs");
      fire_material = new material(vec4(1, 1, 1, 1), fire_shader);
      //fire_shader->init(fire_material->get_params());

      worldCoord = pos;

      using_atlas = true;

      atom_t atom_time_index = app_utils::get_atom("time");
      float val = 1.0f / 30;
      time_index = fire_material->add_uniform(&val, atom_time_index, GL_FLOAT, 1, param::stage_fragment);

      atom_t atom_overlay_half_size_index = app_utils::get_atom("overlay_half_size");
      float overlay_half_size = 20;
      overlay_half_size_index = fire_material->add_uniform(&overlay_half_size, atom_overlay_half_size_index, GL_FLOAT, 1, param::stage_fragment);

      atom_t atom_overlay_bl_index = app_utils::get_atom("overlay_bl");
      vec3 overlay_bl = vec3(pos.x() - overlay_half_size, pos.y(), pos.z());
      overlay_bl_index = fire_material->add_uniform(&overlay_bl, atom_overlay_bl_index, GL_FLOAT_VEC3, 1, param::stage_fragment);
      fire_material->set_uniform(overlay_bl_index, &overlay_bl, sizeof(overlay_bl));

      atom_t atom_particle_diffuse = app_utils::get_atom("particle_diffuse");
      particle_diffuse = fire_material->add_sampler(0, atom_particle_diffuse, new image("assets/fire/seamless_fire_texture_2.gif"), new sampler());
      
      atom_t atom_particle_mask = app_utils::get_atom("particle_mask");
      particle_mask = fire_material->add_sampler(1, atom_particle_mask, new image("assets/fire/particle_g_atlas.gif"), new sampler());

      atom_t atom_overlay_diffuse = app_utils::get_atom("overlay_diffuse");
      overlay_diffuse = fire_material->add_sampler(2, atom_overlay_diffuse, new image("assets/fire/seamless_fire_texture_3.gif"), new sampler());

      atom_t atom_overlay_mask = app_utils::get_atom("overlay_mask");
      overlay_mask = fire_material->add_sampler(3, atom_overlay_mask, new image("assets/fire/overlay_mask.gif"), new sampler());

      atom_t atom_overlay_noise = app_utils::get_atom("overlay_noise");
      overlay_noise = fire_material->add_sampler(4, atom_overlay_noise, new image("assets/fire/seamless_perlin.gif"), new sampler());

      system = new fire_particle_system();
    }
  
    void update(camera_instance* ci, float time) {
      float tex_toggle = r->get(0.0f, 1.0f);


      fire_particle_system::fire_particle p;
      memset(&p, 0, sizeof(p));
      p.pos = vec3p(worldCoord[0] + r->get(-1.0f, 1.0f), worldCoord[1] + r->get(-1.0f, 1.0f), worldCoord[2] + r->get(-1.0f, 1.0f));
      p.scale_size = vec2p(3.0f, 3.0f);
      if(using_atlas){
        if (tex_toggle < 0.25f) {
          p.uv_bottom_left = vec2p(0.0f, 1.0f);
          p.uv_top_right = vec2p(0.5f, 0.5f);
        }
        else if (tex_toggle < 0.5f) {
          p.uv_bottom_left = vec2p(0.0f, 0.5f);
          p.uv_top_right = vec2p(0.5f, 0.0f);
        }
        else if (tex_toggle < 0.75f) {
          p.uv_bottom_left = vec2p(0.5f, 1.0f);
          p.uv_top_right = vec2p(1.0f, 0.5f);
        }
        else{
          p.uv_bottom_left = vec2p(0.5f, 0.5f);
          p.uv_top_right = vec2p(1.0f, 0.0f);
        }
      }
      else {
        if (tex_toggle < 0.5f) {
          p.uv_bottom_left = vec2p(1.0f, 0.0f);
          p.uv_top_right = vec2p(0.0f, 1.0f);
        }else{
          p.uv_bottom_left = vec2p(0.0f, 0.0f);
          p.uv_top_right = vec2p(1.0f, 1.0f);
        }
      }
      p.enabled = true;
      int pidx = system->add_billboard_particle(p);

      mesh_particle_system::particle_animator pa;
      memset(&pa, 0, sizeof(pa));
      pa.link = pidx;
      pa.acceleration = vec3p(0, 2.0f, 0);
      pa.vel = vec3p(r->get(-2.0f, 2.0f), r->get(2.0f, 5.0f), r->get(-2.0f, 2.0f));
      //pa.spin = pow(2,31);
      pa.lifetime = 120;
      system->add_particle_animator(pa);

      fire_material->set_uniform(time_index, &time, sizeof(time));

      system->set_cameraToWorld(ci->get_node()->calcModelToWorld());
      system->animate(1.0f / 30);
      system->update();

    }


    ref<material> get_material() {
      return fire_material;
    }

    ref<mesh_particle_system> get_particle_system() {
      return system;
    }
    
  };
}