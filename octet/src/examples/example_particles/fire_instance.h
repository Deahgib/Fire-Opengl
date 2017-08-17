#pragma once

namespace octet {
  class fire_instance {
  private:
#if FIRE_DEBUG
    ref<mesh_instance> debug_msh_inst; // Density mesh GL_POINTS  debug
    ref<mesh_instance> debug_msh_vel_inst; // Velocity mesh GL_LINES  debug
#endif
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

    // Fluid sim vars
    int x_length, y_length, z_length;
    u_int fs_size;
    float * u, *v, *w, *u_prev, *v_prev, *w_prev;
    float * dens, *dens_prev;
    float diffuse_rate;
    float viscosity;

    opencl* ocl;
    opencl::kernel fluid_kernel;
    opencl::mem dens_GPU_mem, dens_prev_GPU_mem;
    opencl::mem u_GPU_mem, v_GPU_mem, w_GPU_mem;
    opencl::mem u_prev_GPU_mem, v_prev_GPU_mem, w_prev_GPU_mem;

    enum {
      slot_particle_diffuse = 0,
      slot_particle_mask,
      slot_overlay_diffuse,
      slot_overlay_mask,
      slot_overlay_noise,
      tot_textures
    };

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
        //fprintf(stderr, "cannot allocate data\n");
        printf("cannot allocate data\n");
        return (0);
      }

      return (1);
    }

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
      particle_diffuse = fire_material->add_sampler(slot_particle_diffuse, app_utils::get_atom("particle_diffuse"), particle_diffuse_img, new sampler());
      particle_mask = fire_material->add_sampler(slot_particle_mask, app_utils::get_atom("particle_mask"), particle_mask_img, new sampler());
      overlay_diffuse = fire_material->add_sampler(slot_overlay_diffuse, app_utils::get_atom("overlay_diffuse"), overlay_diffuse_img, new sampler());
      overlay_mask = fire_material->add_sampler(slot_overlay_mask, app_utils::get_atom("overlay_mask"), overlay_mask_img, new sampler());
      overlay_noise = fire_material->add_sampler(slot_overlay_noise, app_utils::get_atom("overlay_noise"), overlay_noise_img, new sampler());

      system = new fire_particle_system(aabb(pos, vec3(1, 2, 1)), 256, 0, 256, using_atlas);

      scene_node *node = new scene_node();
      msh_inst = new mesh_instance(node, system, fire_material);

#if FIRE_DEBUG
      scene_node *debug_node = new scene_node();
      node->add_child(debug_node);
      debug_msh_inst = new mesh_instance(debug_node, system->get_debug_mesh(), debug_material);

      scene_node *debug_vel_node = new scene_node();
      debug_node->add_child(debug_vel_node);
      debug_msh_vel_inst = new mesh_instance(debug_vel_node, system->get_debug_vel_mesh(), debug_material);

      debug_view_ = true;
#endif

      init_fluid_sim();
    }

    void init_fluid_sim() 
    {
      x_length = 16;
      y_length = 32;
      z_length = 16;
      //fluid_sim.init(x_length, y_length, z_length);
      fs_size = (x_length + 2) * (y_length + 2) * (z_length + 2);
      printf("size: %d \n", fs_size);
      allocate_data();
      clear_data();
      // 6 here from dt reversing: ( float a = dt*diff*max*max*max; )  for testing
      diffuse_rate = 0.0f;
      viscosity = 0.0f;

      for (int i = 0; i < fs_size; i++) {
        if (dens[i] != 0.0f) {
          printf("Non zero starting values\n");
        }
      }


      dens_prev[1000] = 500.0f;

      ocl = new opencl();
      ocl->init("CL_bin/jos_stam_fluid.cl");
      fluid_kernel      = opencl::kernel(ocl, "square");

      dens_GPU_mem      = opencl::mem(ocl, CL_MEM_READ_WRITE, sizeof(float) * fs_size, NULL);
      dens_prev_GPU_mem = opencl::mem(ocl, CL_MEM_READ_WRITE, sizeof(float) * fs_size, NULL);
      u_GPU_mem         = opencl::mem(ocl, CL_MEM_READ_WRITE, sizeof(float) * fs_size, NULL);
      v_GPU_mem         = opencl::mem(ocl, CL_MEM_READ_WRITE, sizeof(float) * fs_size, NULL);
      w_GPU_mem         = opencl::mem(ocl, CL_MEM_READ_WRITE, sizeof(float) * fs_size, NULL);
      u_prev_GPU_mem    = opencl::mem(ocl, CL_MEM_READ_WRITE, sizeof(float) * fs_size, NULL);
      v_prev_GPU_mem    = opencl::mem(ocl, CL_MEM_READ_WRITE, sizeof(float) * fs_size, NULL);
      w_prev_GPU_mem    = opencl::mem(ocl, CL_MEM_READ_WRITE, sizeof(float) * fs_size, NULL);

      cl_event dens_event       = dens_GPU_mem.write(     sizeof(float) * fs_size,      dens, 0, NULL, true);
      cl_event dens_prev_event  = dens_prev_GPU_mem.write(sizeof(float) * fs_size, dens_prev, 0, NULL, true);
      cl_event u_event          = u_GPU_mem.write(        sizeof(float) * fs_size,         u, 0, NULL, true);
      cl_event v_event          = v_GPU_mem.write(        sizeof(float) * fs_size,         v, 0, NULL, true);
      cl_event w_event          = w_GPU_mem.write(        sizeof(float) * fs_size,         w, 0, NULL, true);
      cl_event u_prev_event     = u_prev_GPU_mem.write(   sizeof(float) * fs_size,    u_prev, 0, NULL, true);
      cl_event v_prev_event     = v_prev_GPU_mem.write(   sizeof(float) * fs_size,    v_prev, 0, NULL, true);
      cl_event w_prev_event     = w_prev_GPU_mem.write(   sizeof(float) * fs_size,    w_prev, 0, NULL, true);
      ocl->wait(dens_event);
      ocl->wait(dens_prev_event);
      ocl->wait(u_event);
      ocl->wait(v_event);
      ocl->wait(w_event);
      ocl->wait(u_prev_event);
      ocl->wait(v_prev_event);
      ocl->wait(w_prev_event);

      fluid_kernel.push(dens_GPU_mem.get_obj()); 
      fluid_kernel.push(dens_prev_GPU_mem.get_obj());
      fluid_kernel.push(u_GPU_mem.get_obj());
      fluid_kernel.push(v_GPU_mem.get_obj());
      fluid_kernel.push(w_GPU_mem.get_obj());
      fluid_kernel.push(u_prev_GPU_mem.get_obj());
      fluid_kernel.push(v_prev_GPU_mem.get_obj());
      fluid_kernel.push(w_prev_GPU_mem.get_obj());
      fluid_kernel.push(x_length);
      fluid_kernel.push(y_length);
      fluid_kernel.push(z_length);
      fluid_kernel.push(diffuse_rate);
      fluid_kernel.push(viscosity);
      fluid_kernel.push(1.0f);

      cl_event exec_event = fluid_kernel.call(fs_size, 0, 0, NULL, true);
      ocl->wait(exec_event);

      //cl_event read_event = dens_GPU_mem.read(sizeof(float)*fs_size, dens, 0, NULL, true);
      //ocl->wait(read_event);

      int count = 0;
      for (int i = 0; i < fs_size; i++) {
        if (dens[i] == 69.0f) {
          count++;
        }
      }
      if (count == fs_size) printf("Kernel sweep. Test\n");

      //opencl::release(input.get_obj()); 
      //opencl::release(output.get_obj()); 
      //opencl::release(kernel.get_obj());
      //delete ocl;
    }
  
    void update_fluid() {
      dens_prev[IX(8, 8, 8)] = 100.0f;
      cl_event dens_event = dens_prev_GPU_mem.write(sizeof(float) * fs_size, dens_prev, 0, NULL, true);
      ocl->wait(dens_event);

      cl_event exec_event = fluid_kernel.call(fs_size/32, 32, 0, NULL, false);
      //ocl->wait(exec_event);

      cl_event read_event = dens_GPU_mem.read(sizeof(float)*fs_size, dens, 0, NULL, true);
      ocl->finish();

      int count = 0;
      for (int i = 0; i < fs_size; i++) {
        if (dens[i] == 69.0f) {
          count++;
        }
      }
      if (count == fs_size) printf("Kernel sweep.\n");
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

      update_fluid();

      system->add_source_force(vec3(worldCoord[0], worldCoord[1] - 1.8f, worldCoord[2]), vec3(0,1,0), 0.1f);

      system->set_cameraToWorld(ci->get_node()->calcModelToWorld());
      system->animate();
      
      system->update();

      #if FIRE_DEBUG
      if (debug_view_) {
        system->update_fluid_sim(dens, u, v, w);
      }
      #endif
      
      
    }

    void update_input(app_common *in) {
      if (in->is_key_down('X')) { system->addWind(); }

      if (in->is_key_going_down('C')) { system->clear_fluid_sim(); }

      #if FIRE_DEBUG
      if (in->is_key_going_down('H')) { 
        debug_view_ = !debug_view_; 
        debug_msh_inst->get_node()->set_enabled(debug_view_);
      }
      #endif
      
    }

    ref<mesh_instance> get_mesh_instance() {
      return msh_inst;
    }

    #if FIRE_DEBUG
    ref<mesh_instance> get_debug_mesh_instance() {
      return debug_msh_inst;
    }
    ref<mesh_instance> get_debug_mesh_vel_instance() {
      return debug_msh_vel_inst;
    }
    #endif


  };
}