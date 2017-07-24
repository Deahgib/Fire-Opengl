////////////////////////////////////////////////////////////////////////////////
//
// (C) Andy Thomason 2012-2014
//
// Modular Framework for OpenGLES2 rendering on multiple platforms.
//

#include "fire_instance.h"

#include "camera_controller.h"

namespace octet {
  /// Scene containing a particle system
  class fire_opengl : public app {
    // scene for drawing box
    ref<visual_scene> app_scene;

    std::vector<fire_instance*> fires;

    ref<camera_instance> ci;


    camera_controller* camera_movement;

    random r;

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
      ci->get_node()->translate(vec3(0,0,1));
      ci->set_far_plane(1000.0f);
      camera_movement = new camera_controller(this);
      camera_movement->add_camera(ci);

      fires = std::vector<fire_instance*>();


      fire_instance* fire = new fire_instance(&r);
      fire->init(
        vec3(0, 0, 0),
        new image("assets/fire/seamless_fire_texture_test.gif"),
        new image("assets/fire/fire_particle_5.gif"),
        new image("assets/fire/seamless_fire_texture_test.gif"),
        new image("assets/fire/seamless_fire_texture_test.gif"),
        new image("assets/fire/seamless_fire_texture_test.gif"),
        true);
      fires.push_back(fire);
      scene_node *node = new scene_node();
      app_scene->add_child(node);
      app_scene->add_mesh_instance(new mesh_instance(node, fire->get_particle_system(), fire->get_material()));

      /*fire_instance* fire2 = new fire_instance(&r);
      fire2->init(
        vec3(-5, -7, 0), 
        new image("assets/fire/overlay_fire_texture_test_2.gif"), 
        new image("assets/fire/overlay_fire_texture_test_2.gif"), 
        new image("assets/fire/overlay_fire_texture_test_2.gif"), 
        new image("assets/fire/overlay_fire_texture_test_2.gif"), 
        new image("assets/fire/overlay_fire_texture_test_2.gif"), 
        false);*/
      //fires.push_back(fire2);
      //scene_node *node2 = new scene_node();
      //app_scene->add_child(node2);
      //app_scene->add_mesh_instance(new mesh_instance(node2, fire2->get_particle_system(), fire2->get_material()));
    }

    /// this is called to draw the world
    void draw_world(int x, int y, int w, int h) {
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
        fire->update(ci, (float)get_frame_number() / 30.0f);
      }

      // update matrices. assume 30 fps.
      app_scene->update(1.0f/30);


      //glBlendEquation(GL_FUNC_ADD);
      //glDepthMask(true);
      //glBlendFuncSeparate(GL_DST_ALPHA, GL_ONE, GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
      //glBlendFunc(GL_ONE, GL_ONE);

      // draw the scene
      //app_scene->render((float)vx / vy);
    }
  };
}
