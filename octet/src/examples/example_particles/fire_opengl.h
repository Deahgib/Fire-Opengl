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
      // OPEN CL TESTING -------------- START -------------------
      //const unsigned int count = 256;
      //
      //float data[count];
      //float results[count];
      //for (int i = 0; i < count; i++) {
      //  data[i] = 2.0f;
      //}

      //opencl* ocl = new opencl();
      //ocl->init("CL_bin/test.cl");
      //opencl::kernel kernel(ocl, "square");
      //opencl::mem input(ocl, CL_MEM_READ_ONLY, sizeof(float)*count, data);
      //opencl::mem output(ocl, CL_MEM_WRITE_ONLY, sizeof(float)*count, results);

      //opencl::event w_event(ocl, input.write(sizeof(float)*count, data, 0, NULL, true));
      //ocl->wait(w_event.get_obj());
      //
      //kernel.push(input.get_obj());
      //kernel.push(output.get_obj());
      //kernel.push(count);

      //opencl::event exec_event(ocl, kernel.call(count, 1, 0, NULL, true));
      //ocl->wait(exec_event.get_obj());
      ////ocl->flush();
      //opencl::event r_event(ocl, output.read(sizeof(float)*count, results, 0, NULL, true));
      //ocl->wait(r_event.get_obj());

      //u_int correct = 0;
      //for (int i = 0; i < count; i++) {
      //  if (data[i] * data[i] == results[i]) {
      //    correct ++;
      //  }
      //}
      //if (correct = count) {
      //  printf("OPENCL:  Working Correctly\n");
      //}

      //opencl::release(input.get_obj()); 
      //opencl::release(output.get_obj()); 
      //opencl::release(kernel.get_obj());
      //delete ocl;
      // OPEN CL TESTING --------------- END ---------------------

      // Fire 
      app_scene =  new visual_scene();
      app_scene->create_default_camera_and_lights();

      ci = app_scene->get_camera_instance(0);
      ci->get_node()->translate(vec3(0,0,0.5f));
      ci->set_far_plane(1000.0f);
      camera_movement = new camera_controller(this);
      camera_movement->add_camera(ci);

      fires = std::vector<fire_instance*>();


      fire_instance* fire = new fire_instance(&r);
      fire->init(
        vec3(0, 0, 0),
        new image("assets/fire/seamless_fire_texture_test.gif"),
        new image("assets/fire/fire_particle_6.gif"),
        new image("assets/fire/seamless_fire_texture_test.gif"),
        new image("assets/fire/seamless_fire_texture_test.gif"),
        new image("assets/fire/seamless_fire_texture_test.gif"),
        true);
      fires.push_back(fire);
      mesh_instance *mi = fire->get_mesh_instance();
      app_scene->add_child(mi->get_node());
      app_scene->add_mesh_instance(mi);
      #if FIRE_DEBUG
      app_scene->add_mesh_instance(fire->get_debug_mesh_instance());
      app_scene->add_mesh_instance(fire->get_debug_mesh_vel_instance());
      #endif





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
