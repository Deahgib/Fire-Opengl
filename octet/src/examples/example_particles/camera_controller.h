#pragma once

namespace octet {
  /// Scene containing a particle system
  class camera_controller {
  private:
    ref<camera_instance> ci;
    app_common* input;

    int lastx, lasty;

    void get_delta_mouse_pos(int &dx, int &dy) {
      int x, y;
      input->get_mouse_pos(x, y);
      dx = x - lastx;
      dy = y - lasty;
      lastx = x;
      lasty = y;
    }

  public:
    camera_controller(app_common* in) { 
      input = in; 
      input->get_mouse_pos(lastx, lasty);
    }
  
    void add_camera(ref<camera_instance> cam) {
      ci = cam;
    }

    void update() {
      //if (is_key_down(key::key_esc)) {
      //  exit(1);
      //}

      float increment = 1;

      int x, y;
      get_delta_mouse_pos(x, y);

      // Translate camera - X-axis    strafe left & right
      if (input->is_key_down('A')) { ci->get_camera_instance()->get_node()->translate(vec3(-increment, 0, 0)); }
      if (input->is_key_down('D')) { ci->get_camera_instance()->get_node()->translate(vec3(increment, 0, 0)); }

      // Translate camera - Y-axis    up down
      if (input->is_key_down('R')) { ci->get_camera_instance()->get_node()->translate(vec3(0, increment, 0)); }
      if (input->is_key_down('F')) { ci->get_camera_instance()->get_node()->translate(vec3(0, -increment, 0)); }

      // Translate camera - Z-axis    forward backward
      if (input->is_key_down('W')) { ci->get_camera_instance()->get_node()->translate(vec3(0, 0, -increment)); }
      if (input->is_key_down('S')) { ci->get_camera_instance()->get_node()->translate(vec3(0, 0, increment)); }

      // Rotate camera - X-axis
      if (input->is_key_down('Q')) { ci->get_camera_instance()->get_node()->rotate(increment, vec3p(0, 1, 0)); }
      if (input->is_key_down('E')) { ci->get_camera_instance()->get_node()->rotate(-increment, vec3p(0, 1, 0)); }
      //if (abs(y) > 2) { ci->get_camera_instance()->get_node()->rotate(-y, vec3p(1, 0, 0)); }

      // Rotate camera - Y axis
      //if (abs(x) > 2) { ci->get_camera_instance()->get_node()->rotate(-x, vec3p(0,1,0)); }

      // Rotate camera - Y axis

      // Set wireframe on or off
      /*static bool wireframe = false;
      if (is_key_going_down(' ')) {
        wireframe = !wireframe;
        if (wireframe) {
          glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        else {
          glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
      }*/
    }
  
  };

}