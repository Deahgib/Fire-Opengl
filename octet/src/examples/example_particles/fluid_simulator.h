#pragma once

#define IX(x, y, z) ((x) + (dim+2)*(y) + (dim+2)*(dim+2)*(z))
#define SWAP(x0, x) {float *tmp = x0; x0 = x; x = tmp;}

namespace octet {
  /// Scene containing a particle system
  class fluid_simulator {
    int size;
    int dim;

    const int LINEARSOLVERTIMES = 10;

    float *densities; // Densities
    float *densities_prev; // Densities

    float *vel_x;
    float *vel_y;
    float *vel_z;

    float *vel_x_prev;
    float *vel_y_prev;
    float *vel_z_prev;

    float diffuse_rate;
    float viscosity;


  private:

    void advect(int b, float * d, float * d0, float * u, float * v, float * w, float dt) {
      int x0, y0, z0, x1, y1, z1;
      float vx, vy, vz, r0, s0, t0, r1, s1, t1, dt0;
      dt0 = dt*dim;
      for (int z = 1; z <= dim; z++) {
        for (int y = 1; y <= dim; y++) {
          for (int x = 1; x <= dim; x++) {
            vx = x - dt0 * u[IX(x, y, z)];
            vy = y - dt0 * v[IX(x, y, z)];
            vz = z - dt0 * w[IX(x, y, z)];
            if (vx < 0.5) vx = 0.5; if (vx > dim + 0.5) vx = dim + 0.5; x0 = (int)vx; x1 = x0 + 1;
            if (vy < 0.5) vy = 0.5; if (vy > dim + 0.5) vy = dim + 0.5; y0 = (int)vy; y1 = y0 + 1;
            if (vz < 0.5) vz = 0.5; if (vz > dim + 0.5) vz = dim + 0.5; z0 = (int)vz; z1 = z0 + 1;

            r1 = vx - x0; r0 = 1 - r1;
            s1 = vy - y0; s0 = 1 - s1;
            t1 = vz - z0; t0 = 1 - t1;

            d[IX(x, y, z)] =
              r0*(s0*t0*d0[IX(x0, y0, z0)] + s1*t0*d0[IX(x0, y1, z0)] + s0*t1*d0[IX(x0, y0, z1)] + s1*t1*d0[IX(x0, y1, z1)]) +
              r1*(s0*t0*d0[IX(x1, y0, z0)] + s1*t0*d0[IX(x1, y1, z0)] + s0*t1*d0[IX(x1, y0, z1)] + s1*t1*d0[IX(x1, y1, z1)]);
          }
        }
        set_bnd(b, d);
      }
    }

    void diffuse(int b, float* p, float* p0, float diff, float dt) {
      float a = dt * diff * dim * dim * dim;
      lin_solve(b, p, p0, a, 1 + 6 * a);
    }

    void project(float * u, float * v, float * w, float * p, float * div)
    {
      int x, y, z;

      for (z = 1; z <= dim; z++) {
        for (y = 1; y <= dim; y++) {
          for (x = 1; x <= dim; x++) {
            div[IX(x, y, z)] = -1.0 / 3.0*((u[IX(x + 1, y, z)] - u[IX(x - 1, y, z)]) / dim + (v[IX(x, y + 1, z)] - v[IX(x, y - 1, z)]) / dim + (w[IX(x, y, z + 1)] - w[IX(x, y, z - 1)]) / dim);
            p[IX(x, y, z)] = 0;
          }
        }
      }

      set_bnd(0, div); set_bnd(0, p);

      lin_solve(0, p, div, 1, 6);

      for (z = 1; z <= dim; z++) {
        for (y = 1; y <= dim; y++) {
          for (x = 1; x <= dim; x++) {
            u[IX(x, y, z)] -= 0.5f*dim*(p[IX(x + 1, y, z)] - p[IX(x - 1, y, z)]);
            v[IX(x, y, z)] -= 0.5f*dim*(p[IX(x, y + 1, z)] - p[IX(x, y - 1, z)]);
            w[IX(x, y, z)] -= 0.5f*dim*(p[IX(x, y, z + 1)] - p[IX(x, y, z - 1)]);
          }
        }
      }

      set_bnd(1, u); set_bnd(2, v); set_bnd(3, w);
    }

    void lin_solve(int b, float * p, float * p0, float a, float c)
    {
      int x, y, z, k;

      // iterate the solver
      for (k = 0; k<LINEARSOLVERTIMES; k++) {
        // update for each cell
        for (x = 1; x <= dim; x++) {
          for (y = 1; y <= dim; y++) {
            for (z = 1; z <= dim; z++) {
              p[IX(x, y, z)] = (p0[IX(x, y, z)] + a*(
                  p[IX(x - 1, y, z)] + p[IX(x + 1, y, z)] + 
                  p[IX(x, y - 1, z)] + p[IX(x, y + 1, z)] + 
                  p[IX(x, y, z - 1)] + p[IX(x, y, z + 1)])) / c;
            }
          }
        }
        set_bnd(b, p);
      }
    }

    void  set_bnd(int b, float * x)
    {
      int i, j;

      for (i = 1; i <= dim; i++) {
        for (j = 1; j <= dim; j++) {
          x[IX(i, j, 0)] = b == 3 ? -x[IX(i, j, 1)] : x[IX(i, j, 1)];
          x[IX(i, j, dim + 1)] = b == 3 ? -x[IX(i, j, dim)] : x[IX(i, j, dim)];
        }
      }

      for (i = 1; i <= dim; i++) {
        for (j = 1; j <= dim; j++) {
          x[IX(0, i, j)] = b == 1 ? -x[IX(1, i, j)] : x[IX(1, i, j)];
          x[IX(dim + 1, i, j)] = b == 1 ? -x[IX(dim, i, j)] : x[IX(dim, i, j)];
        }
      }

      for (i = 1; i <= dim; i++) {
        for (j = 1; j <= dim; j++) {
          x[IX(i, 0, j)] = b == 2 ? -x[IX(i, 1, j)] : x[IX(i, 1, j)];
          x[IX(i, dim + 1, j)] = b == 2 ? -x[IX(i, dim, j)] : x[IX(i, dim, j)];
        }
      }

      x[IX(0, 0, 0)] = 1.0 / 3.0*(x[IX(1, 0, 0)] + x[IX(0, 1, 0)] + x[IX(0, 0, 1)]);
      x[IX(0, dim + 1, 0)] = 1.0 / 3.0*(x[IX(1, dim + 1, 0)] + x[IX(0, dim, 0)] + x[IX(0, dim + 1, 1)]);

      x[IX(dim + 1, 0, 0)] = 1.0 / 3.0*(x[IX(dim, 0, 0)] + x[IX(dim + 1, 1, 0)] + x[IX(dim + 1, 0, 1)]);
      x[IX(dim + 1, dim + 1, 0)] = 1.0 / 3.0*(x[IX(dim, dim + 1, 0)] + x[IX(dim + 1, dim, 0)] + x[IX(dim + 1, dim + 1, 1)]);

      x[IX(0, 0, dim + 1)] = 1.0 / 3.0*(x[IX(1, 0, dim + 1)] + x[IX(0, 1, dim + 1)] + x[IX(0, 0, dim)]);
      x[IX(0, dim + 1, dim + 1)] = 1.0 / 3.0*(x[IX(1, dim + 1, dim + 1)] + x[IX(0, dim, dim + 1)] + x[IX(0, dim + 1, dim)]);

      x[IX(dim + 1, 0, dim + 1)] = 1.0 / 3.0*(x[IX(dim, 0, dim + 1)] + x[IX(dim + 1, 1, dim + 1)] + x[IX(dim + 1, 0, dim)]);
      x[IX(dim + 1, dim + 1, dim + 1)] = 1.0 / 3.0*(x[IX(dim, dim + 1, dim + 1)] + x[IX(dim + 1, dim, dim + 1)] + x[IX(dim + 1, dim + 1, dim)]);
    }


    void dens_step(float dt) {
      add_sources(densities, densities_prev, dt);
      SWAP(densities_prev, densities);
      diffuse(0, densities, densities_prev, diffuse_rate, dt);
      SWAP(densities_prev, densities);
      advect(0, densities, densities_prev, vel_x, vel_y, vel_z, dt);
    }


    void vel_step(float dt)
    {
      add_sources(vel_x, vel_x_prev, dt); 
      add_sources(vel_y, vel_y_prev, dt); 
      add_sources(vel_z, vel_z_prev, dt);

      SWAP(vel_x_prev, vel_x); 
      diffuse(1, vel_x, vel_x_prev, viscosity, dt);
      SWAP(vel_y_prev, vel_y); 
      diffuse(2, vel_y, vel_y_prev, viscosity, dt);
      SWAP(vel_z_prev, vel_z);
      diffuse(3, vel_z, vel_z_prev, viscosity, dt);

      project(vel_x, vel_y, vel_z, vel_x_prev, vel_y_prev);

      SWAP(vel_x_prev, vel_x); 
      SWAP(vel_y_prev, vel_y); 
      SWAP(vel_z_prev, vel_z);
      advect(1, vel_x, vel_x_prev, vel_x_prev, vel_y_prev, vel_z_prev, dt);
      advect(2, vel_y, vel_y_prev, vel_x_prev, vel_y_prev, vel_z_prev, dt);
      advect(3, vel_z, vel_z_prev, vel_x_prev, vel_y_prev, vel_z_prev, dt);

      project(vel_x, vel_y, vel_z, vel_x_prev, vel_y_prev);
    }


    void render_debug() {
      //glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
      //glBegin(GL_LINES);
      //for (int z = 1; z <= dim; z++) {
      //  for (int y = 1; y <= dim; y++) {
      //    for (int x = 1; x <= dim; x++) {
      //      int idx = IX(x, y, z);
      //      glVertex3f((float)x / (float)dim - 0.5f, (float)y / (float)dim - 0.5f, (float)z / (float)dim - 0.5f);

      //      glVertex3f((float)x / (float)dim - 0.5f + vel_x[idx], (float)y / (float)dim - 0.5f + vel_y[idx], (float)z / (float)dim - 0.5f + vel_z[idx]);
      //    }
      //  }
      //}
      //glEnd();

      glPointSize(10.0f);
      glBegin(GL_POINTS);
      for (int z = 1; z <= dim; z++) {
        for (int y = 1; y <= dim; y++) {
          for (int x = 1; x <= dim; x++) {
            float tone = densities[IX(x, y, z)];
            glColor4f(1.0f, 1.0f, 1.0f, tone);
            if (tone > 2) {
              glColor4f(tone - 2.0f, 0.0f, 0.0f, tone);
            }
            if (tone > 5) {
              glColor4f(0.0f, tone - 5.0f, 0.0f, tone);
            }
            if (tone > 10) {
              glColor4f(0.0f, 0.0f, tone - 10.0f, tone);
            }
            
            glVertex3f((float)x / (float)dim - 0.5f, (float)y / (float)dim - 0.5f, (float)z / (float)dim - 0.5f);
          }
        }
      }
      glEnd();
    }


  public:
    fluid_simulator() {}
    ~fluid_simulator() {
      delete densities;
      delete densities_prev;
    }


    void init(int dimentions) {
      dim = dimentions;
      diffuse_rate = 0.2f;
      viscosity = 0.0f;

      size = (dim + 2) * (dim + 2) * (dim + 2);
      densities = new float[size];
      densities_prev = new float[size];
      vel_x = new float[size];
      vel_y = new float[size];
      vel_z = new float[size];

      vel_x_prev = new float[size];
      vel_y_prev = new float[size];
      vel_z_prev = new float[size];

      for (int i = 0; i < size; i++) {
        densities[i] = 0.0f;
        densities_prev[i] = 0.0001f;

        vel_x[i] = 0.0f;
        vel_y[i] = 0.0f;
        vel_z[i] = 0.0f;
        vel_x_prev[i] = 0.0f;
        vel_y_prev[i] = 0.0f;
        vel_z_prev[i] = 0.0f;
      }

      densities_prev[IX(20, 20, 20)] = 300.0f;
      vel_y[IX(22, 22, 20)] = 70.0f;

      //for (int k = 18; k < 23; k++) {
      //  for (int i = 1; i <= dim; i++) {
      //    vel_x[IX(i, k, 20)] = 0.2f;
      //    vel_y[IX(i, k, 20)] = 0.01f;
      //  }
      //}
    }

    int loop = 0;
    void update(float dt) {

      printf("p: %f p_old: %f\n", densities[IX(10, 9, 10)], densities_prev[IX(10, 9, 10)]);

      dt = dt * 2.0f;

      vel_step(dt);
      dens_step(dt);


      render_debug();
    }

    void add_sources(float *p, float *s, float dt) {
      for (int i = 0; i < size; i++) p[i] += dt * s[i];
    }




  };

}
