////////////////////////////////////////////////////////////////////////////////
//
// (C) Andy Thomason 2012-2014
//
// Modular Framework for OpenGLES2 rendering on multiple platforms.
//
// Text overlay
//

#include "../../octet.h"

#include "fire_opengl.h"

/// Create a box with octet
int main(int argc, char **argv) {
  // set up the platform.
  octet::app::init_all(argc, argv);

  // our application.
  octet::fire_opengl app(argc, argv);
  app.init(1200, 1200);

  // open windows
  octet::app::run_all_apps();
}


