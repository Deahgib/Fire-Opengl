////////////////////////////////////////////////////////////////////////////////
//
// (C) Andy Thomason 2012-2014
//
// Modular Framework for OpenGLES2 rendering on multiple platforms.
//

#include "fire_instance.h"

#include "camera_controller.h"

#include <time.h>

namespace octet {
  /// Scene containing a particle system
  class fire_opengl : public app {
    // scene for drawing box
    ref<visual_scene> app_scene;

    std::vector<fire_instance*> fires;

    ref<camera_instance> ci;


    camera_controller* camera_movement;

    random r;

    clock_t last_time;

  public:
    /// this is called when we construct the class before everything is initialised.
    fire_opengl(int argc, char **argv) : app(argc, argv) {
    }

    ~fire_opengl() {
      delete camera_movement;
      for (fire_instance* fire : fires) {
        delete fire;
      }
    }

    /// this is called once OpenGL is initialized
    void app_init() {
      app_scene =  new visual_scene();
      app_scene->create_default_camera_and_lights();

      ci = app_scene->get_camera_instance(0);
      ci->get_node()->translate(vec3(0,0,-25));
      ci->set_far_plane(1000.0f);
      camera_movement = new camera_controller(this);
      camera_movement->add_camera(ci);

      fires = std::vector<fire_instance*>();


      fire_instance* fire = new fire_instance(&r);
      fire->init(
        vec3(0, 0, 0),
        new image("assets/fire/seamless_fire_texture_2.gif"),
        new image("assets/fire/fire_particle_6.gif"),
        new image("assets/fire/seamless_fire_texture_2.gif"),
        new image("assets/fire/seamless_fire_texture_2.gif"),
        new image("assets/fire/seamless_fire_texture_2.gif"),
        true);
      fires.push_back(fire);
      mesh_instance *mi = fire->get_mesh_instance();
      app_scene->add_child(mi->get_node());
      app_scene->add_mesh_instance(mi);
      mesh_instance *mi_debug = fire->get_debug_mesh_instance();
      mi->get_node()->add_child(mi_debug->get_node());
      app_scene->add_mesh_instance(mi_debug);
      app_scene->add_mesh_instance(fire->get_debug_mesh_vel_instance());

      /*ref<mesh> point;
      ref<param_geom_shader> fire_shader = new param_geom_shader("shaders/fire.vs", "shaders/fire.fs", "shaders/test.gs");
      ref<material> fire_material = new material(vec4(1, 1, 1, 1), fire_shader);
      point = new mesh();
      point->add_attribute(attribute_pos, 3, GL_FLOAT, 0);
      point->add_attribute(attribute_normal, 3, GL_FLOAT, 12);
      point->add_attribute(attribute_uv, 2, GL_FLOAT, 24);
      point->add_attribute(attribute_color, 4, GL_FLOAT, 32);
      point->set_params(48, 0, 0, GL_POINTS, GL_UNSIGNED_INT);

      unsigned vsize = 1 * sizeof(fire_particle_system::fire_vertex);
      unsigned isize = 1 * sizeof(uint32_t);
      point->allocate(vsize, isize);

      gl_resource::wolock vlock(point->get_vertices());
      fire_particle_system::fire_vertex *vtx = (fire_particle_system::fire_vertex*)vlock.u8();
      gl_resource::wolock ilock(point->get_indices());
      uint32_t *idx = ilock.u32();
      unsigned num_vertices = 0;
      unsigned num_indices = 0;
      vtx->pos = vec3(0,0,0); vtx->normal = vec3(0.0,0.0,1.0); vtx->color = vec4(1,1,0,1); vtx++;
      idx[0] = num_vertices; idx++;
      num_vertices++; num_indices++;
      point->set_num_vertices(num_vertices);
      point->set_num_indices(num_indices);
      scene_node * node_ = new scene_node();
      mesh_instance *mi_ = new mesh_instance(node_, point, fire_material);*/
      //app_scene->add_child(node_);
      //app_scene->add_mesh_instance(mi_);

      last_time = clock();
    }

    /// this is called to draw the world
    void draw_world(int x, int y, int w, int h) {
      clock_t time_diff = clock() - last_time;
      last_time = clock();
      float dt = (float)time_diff / CLOCKS_PER_SEC;

      int vx = 0, vy = 0;
      get_viewport_size(vx, vy);
      app_scene->begin_render(vx, vy, vec4(0.0f, 0.0f, 0.0f, 1.0f));

      camera_movement->update();
      if (is_key_down(key::key_esc)) {
        exit(1);
      }

      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glEnable(GL_BLEND | GL_ALPHA_TEST);
      glDisable(GL_DEPTH_TEST);

      for (auto fire : fires) {
        fire->update_input(this);
        fire->update(ci, dt);
      }

      // update matrices. assume 30 fps.
      app_scene->update(dt);


      //glBlendEquation(GL_FUNC_ADD);
      //glDepthMask(true);
      //glBlendFuncSeparate(GL_DST_ALPHA, GL_ONE, GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
      //glBlendFunc(GL_ONE, GL_ONE);

      // draw the scene
      app_scene->render((float)vx / vy);
    }
  };
}
