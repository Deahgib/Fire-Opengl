#pragma once
namespace octet {
  class fire_instance {
  private:
    ref<mesh_instance> debug_msh_inst; // Density mesh GL_POINTS  debug
    ref<mesh_instance> debug_msh_vel_inst; // Velocity mesh GL_LINES  debug
    ref<mesh_instance> msh_inst; // Particle system Mesh - game output (GL_TRIANGLES)
    ref<param_shader> fire_shader;
    ref<material> fire_material;
    ref<material> debug_material;

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
    bool using_atlas_;

    bool debug_view_;

    enum {
      slot_particle_diffuse = 0,
      slot_particle_mask,
      slot_overlay_diffuse,
      slot_overlay_mask,
      slot_overlay_noise,
      tot_textures
    };


  public:
    fire_instance(random* rand) {
      r = rand;
    }

    void init(vec3 pos,
      ref<image> particle_diffuse_img = new image("assets/fire/seamless_fire_texture_2.gif"),
      ref<image> particle_mask_img = new image("assets/fire/particle_g_atlas.gif"),
      ref<image> overlay_diffuse_img = new image("assets/fire/seamless_fire_texture_3.gif"),
      ref<image> overlay_mask_img = new image("assets/fire/overlay_mask.gif"),
      ref<image> overlay_noise_img = new image("assets/fire/seamless_perlin.gif"),
      bool using_atlas = true) 
    {

      fire_shader = new param_shader("shaders/fire.vs", "shaders/panning_textured.fs");
      fire_material = new material(vec4(1, 1, 1, 1), fire_shader);
      //fire_shader->init(fire_material->get_params());

      debug_material = new material(vec4(1, 1, 1, 1), new param_shader("shaders/simple_color.vs", "shaders/simple_color.fs"));

      worldCoord = pos;
      using_atlas_ = using_atlas;
      
      // Time uniform for panning
      atom_t atom_time_index = app_utils::get_atom("time");
      float val = 1.0f / 30;
      time_index = fire_material->add_uniform(&val, atom_time_index, GL_FLOAT, 1, param::stage_fragment);

      // OVERLAY DATA
      atom_t atom_overlay_half_size_index = app_utils::get_atom("overlay_half_size");
      float overlay_half_size = 20;
      overlay_half_size_index = fire_material->add_uniform(&overlay_half_size, atom_overlay_half_size_index, GL_FLOAT, 1, param::stage_fragment);

      atom_t atom_overlay_bl_index = app_utils::get_atom("overlay_bl");
      vec3 overlay_bl = vec3(pos.x() - overlay_half_size, pos.y(), pos.z());
      overlay_bl_index = fire_material->add_uniform(&overlay_bl, atom_overlay_bl_index, GL_FLOAT_VEC3, 1, param::stage_fragment);
      fire_material->set_uniform(overlay_bl_index, &overlay_bl, sizeof(overlay_bl));

      // TEXTURE DATA
      atom_t atom_particle_diffuse = app_utils::get_atom("particle_diffuse");
      particle_diffuse = fire_material->add_sampler(slot_particle_diffuse, atom_particle_diffuse, particle_diffuse_img, new sampler());
      
      atom_t atom_particle_mask = app_utils::get_atom("particle_mask");
      particle_mask = fire_material->add_sampler(slot_particle_mask, atom_particle_mask, particle_mask_img, new sampler());

      atom_t atom_overlay_diffuse = app_utils::get_atom("overlay_diffuse");
      overlay_diffuse = fire_material->add_sampler(slot_overlay_diffuse, atom_overlay_diffuse, overlay_diffuse_img, new sampler());

      atom_t atom_overlay_mask = app_utils::get_atom("overlay_mask");
      overlay_mask = fire_material->add_sampler(slot_overlay_mask, atom_overlay_mask, overlay_mask_img, new sampler());

      atom_t atom_overlay_noise = app_utils::get_atom("overlay_noise");
      overlay_noise = fire_material->add_sampler(slot_overlay_noise, atom_overlay_noise, overlay_noise_img, new sampler());

      system = new fire_particle_system(aabb(pos, vec3(1, 2, 1)), 256, 0, 256, using_atlas);

      scene_node *node = new scene_node();
      msh_inst = new mesh_instance(node, system, fire_material);

      scene_node *debug_node = new scene_node();
      debug_msh_inst = new mesh_instance(debug_node, system->get_debug_mesh(), debug_material);

      scene_node *debug_vel_node = new scene_node();
      debug_node->add_child(debug_vel_node);
      debug_msh_vel_inst = new mesh_instance(debug_vel_node, system->get_debug_vel_mesh(), debug_material);

      debug_view_ = true;
    }
  
    void update(camera_instance* ci, float time) {
      float tex_toggle = r->get(0.0f, 1.0f);


      fire_particle_system::fire_billboard_particle p;
      memset(&p, 0, sizeof(p));
      //p.pos = vec3p(worldCoord[0] + r->get(-1.0f, 1.0f), worldCoord[1] + r->get(-1.0f, 1.0f), worldCoord[2] + r->get(-1.0f, 1.0f));
      p.pos = vec3p(worldCoord[0] + r->get(-0.1f, 0.1f), worldCoord[1]-1.8f, worldCoord[2]);
      //p.uv_bottom_left = vec2p(1.0f, 0.0f);
      //p.uv_top_right = vec2p(0.0f, 1.0f);
      if(using_atlas_){
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
      pa.acceleration = vec3p(0, 0, 0);
      pa.vel = vec3p(0.0f, 0.0f, 0.0f);
      //pa.spin = pow(2,31);
      pa.lifetime = 30;
      system->add_particle_animator(pa);

      //fire_material->set_uniform(time_index, &time, sizeof(time));

      system->set_cameraToWorld(ci->get_node()->calcModelToWorld());
      system->animate(time);
      
      system->update();
      if (debug_view_) {
        system->update_fluid_sim();
      }
      
      
    }

    void update_input(app_common *in) {
      if (in->is_key_down('X')) { system->addWind(); }

      if (in->is_key_going_down('C')) { system->clear_fluid_sim(); }

      if (in->is_key_going_down('H')) { 
        debug_view_ = !debug_view_; 
        debug_msh_inst->get_node()->set_enabled(debug_view_);
      }
      
    }

    ref<mesh_instance> get_mesh_instance() {
      return msh_inst;
    }

    ref<mesh_instance> get_debug_mesh_instance() {
      return debug_msh_inst;
    }
    ref<mesh_instance> get_debug_mesh_vel_instance() {
      return debug_msh_vel_inst;
    }



  };
}