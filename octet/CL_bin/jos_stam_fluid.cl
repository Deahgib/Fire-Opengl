#define IX(i, j, k, M, N) ((i)+(M+2)*(j) + (M+2)*(N+2)*(k))
#define SWAP(x0, x) {__global float *tmp = x0; x0 = x; x = tmp;}
#define MAX(a,b)            (((a) > (b)) ? (a) : (b))




void add_source(int M, int N, int O, __global float * x, __global float * s, float dt)
{
	int i, size = (M + 2)*(N + 2)*(O + 2);
	for (i = 0; i < size; i++) x[i] += dt*s[i];
}

void set_bnd(int M, int N, int O, int b, __global float * x)
{
	// bounds are cells at faces of the cube
	int i, j;

	for (i = 1; i <= M; i++) {
	  for (j = 1; j <= N; j++) {
		x[IX(i, j, 0, M, N)] = b == 3 ? -x[IX(i, j, 1, M, N)] : x[IX(i, j, 1, M, N)];
		x[IX(i, j, O + 1, M, N)] = b == 3 ? -x[IX(i, j, O, M, N)] : x[IX(i, j, O, M, N)];
	  }
	}

	for (i = 1; i <= N; i++) {
	  for (j = 1; j <= O; j++) {
		x[IX(0, i, j, M, N)] = b == 1 ? -x[IX(1, i, j, M, N)] : x[IX(1, i, j, M, N)];
		x[IX(M + 1, i, j, M, N)] = b == 1 ? -x[IX(M, i, j, M, N)] : x[IX(M, i, j, M, N)];
	  }
	}

	for (i = 1; i <= M; i++) {
	  for (j = 1; j <= O; j++) {
		x[IX(i, 0, j, M, N)] = b == 2 ? -x[IX(i, 1, j, M, N)] : x[IX(i, 1, j, M, N)];
		x[IX(i, N + 1, j, M, N)] = b == 2 ? -x[IX(i, N, j, M, N)] : x[IX(i, N, j, M, N)];
	  }
	}

	x[IX(0, 0, 0, M, N)] = 1.0 / 3.0*(x[IX(1, 0, 0, M, N)] + x[IX(0, 1, 0, M, N)] + x[IX(0, 0, 1, M, N)]);
	x[IX(0, N + 1, 0, M, N)] = 1.0 / 3.0*(x[IX(1, N + 1, 0, M, N)] + x[IX(0, N, 0, M, N)] + x[IX(0, N + 1, 1, M, N)]);

	x[IX(M + 1, 0, 0, M, N)] = 1.0 / 3.0*(x[IX(M, 0, 0, M, N)] + x[IX(M + 1, 1, 0, M, N)] + x[IX(M + 1, 0, 1, M, N)]);
	x[IX(M + 1, N + 1, 0, M, N)] = 1.0 / 3.0*(x[IX(M, N + 1, 0, M, N)] + x[IX(M + 1, N, 0, M, N)] + x[IX(M + 1, N + 1, 1, M, N)]);

	x[IX(0, 0, O + 1, M, N)] = 1.0 / 3.0*(x[IX(1, 0, O + 1, M, N)] + x[IX(0, 1, O + 1, M, N)] + x[IX(0, 0, O, M, N)]);
	x[IX(0, N + 1, O + 1, M, N)] = 1.0 / 3.0*(x[IX(1, N + 1, O + 1, M, N)] + x[IX(0, N, O + 1, M, N)] + x[IX(0, N + 1, O, M, N)]);

	x[IX(M + 1, 0, O + 1, M, N)] = 1.0 / 3.0*(x[IX(M, 0, O + 1, M, N)] + x[IX(M + 1, 1, O + 1, M, N)] + x[IX(M + 1, 0, O, M, N)]);
	x[IX(M + 1, N + 1, O + 1, M, N)] = 1.0 / 3.0*(x[IX(M, N + 1, O + 1, M, N)] + x[IX(M + 1, N, O + 1, M, N)] + x[IX(M + 1, N + 1, O, M, N)]);
}

void lin_solve(int M, int N, int O, int b, __global float * x, __global float * x0, float a, float c)
{
	int i, j, k, l;

	// iterate the solver
	for (l = 0; l < 10; l++) {
	  // update for each cell
	  for (i = 1; i <= M; i++) {
		for (j = 1; j <= N; j++) {
		  for (k = 1; k <= O; k++) {
			x[IX(i, j, k, M, N)] = (x0[IX(i, j, k, M, N)] + a*(x[IX(i - 1, j, k, M, N)] + x[IX(i + 1, j, k, M, N)] + x[IX(i, j - 1, k, M, N)] + x[IX(i, j + 1, k, M, N)] + x[IX(i, j, k - 1, M, N)] + x[IX(i, j, k + 1, M, N)])) / c;
		  }
		}
	  }
	  set_bnd(M, N, O, b, x);
	}
}

void diffuse(int M, int N, int O, int b, __global float * x, __global float * x0, float diff, float dt)
{
	int max = MAX(MAX(M, N), MAX(N, O));
	float a = dt*diff*max*max*max;
	lin_solve(M, N, O, b, x, x0, a, 1 + 6 * a);
}

void advect(int M, int N, int O, int b, __global float * d, __global float * d0, __global float * u, __global float * v, __global float * w, float dt)
{
	int i, j, k, i0, j0, k0, i1, j1, k1;
	float x, y, z, s0, t0, s1, t1, u1, u0, dtx, dty, dtz;

	dtx = dty = dtz = dt*MAX(MAX(M, N), MAX(N, O));

	for (i = 1; i <= M; i++) {
	  for (j = 1; j <= N; j++) {
		for (k = 1; k <= O; k++) {
		  x = i - dtx*u[IX(i, j, k, M, N)]; y = j - dty*v[IX(i, j, k, M, N)]; z = k - dtz*w[IX(i, j, k, M, N)];
		  if (x < 0.5f) x = 0.5f; if (x > M + 0.5f) x = M + 0.5f; i0 = (int)x; i1 = i0 + 1;
		  if (y < 0.5f) y = 0.5f; if (y > N + 0.5f) y = N + 0.5f; j0 = (int)y; j1 = j0 + 1;
		  if (z < 0.5f) z = 0.5f; if (z > O + 0.5f) z = O + 0.5f; k0 = (int)z; k1 = k0 + 1;

		  s1 = x - i0; s0 = 1 - s1; t1 = y - j0; t0 = 1 - t1; u1 = z - k0; u0 = 1 - u1;
		  d[IX(i, j, k, M, N)] = s0*(t0*u0*d0[IX(i0, j0, k0, M, N)] + t1*u0*d0[IX(i0, j1, k0, M, N)] + t0*u1*d0[IX(i0, j0, k1, M, N)] + t1*u1*d0[IX(i0, j1, k1, M, N)]) +
			s1*(t0*u0*d0[IX(i1, j0, k0, M, N)] + t1*u0*d0[IX(i1, j1, k0, M, N)] + t0*u1*d0[IX(i1, j0, k1, M, N)] + t1*u1*d0[IX(i1, j1, k1, M, N)]);
		}
	  }
	}

	set_bnd(M, N, O, b, d);
}


void project(int M, int N, int O, __global float * u, __global float * v, __global float * w, __global float * p, __global float * div)
{
	int i, j, k;

	for (i = 1; i <= M; i++) {
	  for (j = 1; j <= N; j++) {
		for (k = 1; k <= O; k++) {
		  div[IX(i, j, k, M, N)] = -1.0 / 3.0*((u[IX(i + 1, j, k, M, N)] - u[IX(i - 1, j, k, M, N)]) / M + (v[IX(i, j + 1, k, M, N)] - v[IX(i, j - 1, k, M, N)]) / M + (w[IX(i, j, k + 1, M, N)] - w[IX(i, j, k - 1, M, N)]) / M);
		  p[IX(i, j, k, M, N)] = 0;
		}
	  }
	}

	set_bnd(M, N, O, 0, div); set_bnd(M, N, O, 0, p);

	lin_solve(M, N, O, 0, p, div, 1, 6);

	for (i = 1; i <= M; i++) {
	  for (j = 1; j <= N; j++) {
		for (k = 1; k <= O; k++) {
		  u[IX(i, j, k, M, N)] -= 0.5f*M*(p[IX(i + 1, j, k, M, N)] - p[IX(i - 1, j, k, M, N)]);
		  v[IX(i, j, k, M, N)] -= 0.5f*M*(p[IX(i, j + 1, k, M, N)] - p[IX(i, j - 1, k, M, N)]);
		  w[IX(i, j, k, M, N)] -= 0.5f*M*(p[IX(i, j, k + 1, M, N)] - p[IX(i, j, k - 1, M, N)]);
		}
	  }
	}

  set_bnd(M, N, O, 1, u); set_bnd(M, N, O, 2, v); set_bnd(M, N, O, 3, w);
}

void density_step(int M, int N, int O, __global float * x, __global float * x0, __global float * u, __global float * v, __global float * w, float diff, float dt)
{
	add_source(M, N, O, x, x0, dt);
	SWAP(x0, x); diffuse(M, N, O, 0, x, x0, diff, dt);
	SWAP(x0, x); advect(M, N, O, 0, x, x0, u, v, w, dt);
}

void velocity_step(int M, int N, int O, __global float * u, __global float * v, __global float * w, __global float * u0, __global float * v0, __global float * w0, float visc, float dt)
{
	add_source(M, N, O, u, u0, dt); add_source(M, N, O, v, v0, dt); add_source(M, N, O, w, w0, dt);
	SWAP(u0, u); diffuse(M, N, O, 1, u, u0, visc, dt);
	SWAP(v0, v); diffuse(M, N, O, 2, v, v0, visc, dt);
	SWAP(w0, w); diffuse(M, N, O, 3, w, w0, visc, dt);
	project(M, N, O, u, v, w, u0, v0);
	SWAP(u0, u); SWAP(v0, v); SWAP(w0, w);
	advect(M, N, O, 1, u, u0, u0, v0, w0, dt); advect(M, N, O, 2, v, v0, u0, v0, w0, dt); advect(M, N, O, 3, w, w0, u0, v0, w0, dt);
	project(M, N, O, u, v, w, u0, v0);
}

__kernel void square(                                                        
   __global float* dens,                                      
   __global float* dens_prev,  
   __global float* u,                                      
   __global float* v, 
   __global float* w,                                      
   __global float* u_prev,  
   __global float* v_prev,                                      
   __global float* w_prev,
   int x_length,
   int y_length,
   int z_length,
   float diffuse_rate,
   float viscosity,
   float dt)                                  
{                                                       

  velocity_step(x_length, y_length, z_length, u, v, w, u_prev, v_prev, w_prev, viscosity, dt);
  density_step(x_length, y_length, z_length, dens, dens_prev, u, v, w, diffuse_rate, dt);
   
}     