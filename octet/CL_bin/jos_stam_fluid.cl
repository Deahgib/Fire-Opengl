#define IX(i, j, k, M, N) ((i)+(M+2)*(j) + (M+2)*(N+2)*(k))
#define SWAP(x0, x) {__global float *tmp = x0; x0 = x; x = tmp;}
#define MAX(a,b)            (((a) > (b)) ? (a) : (b))

// unsigned int lc rc tc bc fc 



void add_source(int idx, __global float * x, __global float * s, float dt)
{
	x[idx] += dt*s[idx];
}

void set_bnd(int M, int N, int O, int i, int j, int k, int idx, int b, __global float * x, __global float * x0)
{
	// bounds are cells at faces of the cube
  if      (i == 0     && j == 0     && k == 0)      x[idx] = 1.0 / 3.0*(x[IX(1, 0, 0, M, N)] + x[IX(0, 1, 0, M, N)] + x[IX(0, 0, 1, M, N)]);
  else if (i == 0     && j == N + 1 && k == 0)      x[idx] = 1.0 / 3.0*(x[IX(1, N + 1, 0, M, N)] + x[IX(0, N, 0, M, N)] + x[IX(0, N + 1, 1, M, N)]);
  else if (i == 0     && j == 0     && k == O + 1)  x[idx] = 1.0 / 3.0*(x[IX(1, 0, O + 1, M, N)] + x[IX(0, 1, O + 1, M, N)] + x[IX(0, 0, O, M, N)]);
  else if (i == 0     && j == N + 1 && k == O + 1)  x[idx] = 1.0 / 3.0*(x[IX(1, N + 1, O + 1, M, N)] + x[IX(0, N, O + 1, M, N)] + x[IX(0, N + 1, O, M, N)]);
  else if (i == M + 1 && j == 0     && k == 0)      x[idx] = 1.0 / 3.0*(x[IX(M, 0, 0, M, N)] + x[IX(M + 1, 1, 0, M, N)] + x[IX(M + 1, 0, 1, M, N)]);
  else if (i == M + 1 && j == N + 1 && k == 0)      x[idx] = 1.0 / 3.0*(x[IX(M, N + 1, 0, M, N)] + x[IX(M + 1, N, 0, M, N)] + x[IX(M + 1, N + 1, 1, M, N)]);
  else if (i == M + 1 && j == 0     && k == O + 1)  x[idx] = 1.0 / 3.0*(x[IX(M, 0, O + 1, M, N)] + x[IX(M + 1, 1, O + 1, M, N)] + x[IX(M + 1, 0, O, M, N)]);
  else if (i == M + 1 && j == N + 1 && k == O + 1)  x[idx] = 1.0 / 3.0*(x[IX(M, N + 1, O + 1, M, N)] + x[IX(M + 1, N, O + 1, M, N)] + x[IX(M + 1, N + 1, O, M, N)]);

  else if (i == 0)      x[idx] = b == 1 ? -x[IX(1, j, k, M, N)] : x[IX(1, j, k, M, N)];
  else if (i == M + 1)  x[idx] = b == 1 ? -x[IX(M, j, k, M, N)] : x[IX(M, j, k, M, N)];

  else if (j == 0)      x[idx] = b == 2 ? -x[IX(i, 1, k, M, N)] : x[IX(i, 1, k, M, N)];
  else if (j == N + 1)  x[idx] = b == 2 ? -x[IX(i, N, k, M, N)] : x[IX(i, N, k, M, N)];

  else if (k == 0)      x[idx] = b == 3 ? -x[IX(i, j, 1, M, N)] : x[IX(i, j, 1, M, N)];
  else if (k == O + 1)  x[idx] = b == 3 ? -x[IX(i, j, O, M, N)] : x[IX(i, j, O, M, N)];
}

void lin_solve(int M, int N, int O, int idx, int right, int left, int top, int bottom, int front, int back, int b, __global float * x, __global float * x0, float a, float c)
{
	int l;
	// iterate the solver
	for (l = 0; l < 10; l++) {
	  x[idx] = (x0[idx] + a*(x[left] + x[right] + x[bottom] + x[top] + x[back] + x[front])) / c;
	  //set_bnd(M, N, O, b, x);
	}
}

void diffuse(int M, int N, int O, int idx, int right, int left, int top, int bottom, int front, int back, int b, __global float * x, __global float * x0, float diff, float dt)
{
	int max = MAX(MAX(M, N), MAX(N, O));
	float a = dt*diff*max*max*max;
	lin_solve(M, N, O, idx, right, left, top, bottom, front, back, b, x, x0, a, 1 + 6 * a);
}

void advect(int M, int N, int O, int i, int j, int k, int idx, int b, __global float * d, __global float * d0, __global float * u, __global float * v, __global float * w, float dt)
{
	int i0, j0, k0, i1, j1, k1;
	float x, y, z, s0, t0, s1, t1, u1, u0, dtx, dty, dtz;

	dtx = dty = dtz = dt*MAX(MAX(M, N), MAX(N, O));

	x = (float)i - dtx*u[idx];
  y = (float)j - dty*v[idx];
  z = (float)k - dtz*w[idx];

	if (x < 0.5f) x = 0.5f; if (x > M + 0.5f) x = M + 0.5f; i0 = (int)x; i1 = i0 + 1;
	if (y < 0.5f) y = 0.5f; if (y > N + 0.5f) y = N + 0.5f; j0 = (int)y; j1 = j0 + 1;
	if (z < 0.5f) z = 0.5f; if (z > O + 0.5f) z = O + 0.5f; k0 = (int)z; k1 = k0 + 1;

	s1 = x - i0; s0 = 1 - s1; t1 = y - j0; t0 = 1 - t1; u1 = z - k0; u0 = 1 - u1;
	d[IX(i, j, k, M, N)] = s0*(t0*u0*d0[IX(i0, j0, k0, M, N)] + t1*u0*d0[IX(i0, j1, k0, M, N)] + t0*u1*d0[IX(i0, j0, k1, M, N)] + t1*u1*d0[IX(i0, j1, k1, M, N)]) +
	s1*(t0*u0*d0[IX(i1, j0, k0, M, N)] + t1*u0*d0[IX(i1, j1, k0, M, N)] + t0*u1*d0[IX(i1, j0, k1, M, N)] + t1*u1*d0[IX(i1, j1, k1, M, N)]);


	//set_bnd(M, N, O, i, j, k, b, d);
}


//void project(int M, int N, int O, __global float * u, __global float * v, __global float * w, __global float * p, __global float * div)
//{
//	int i, j, k;
//
//	for (i = 1; i <= M; i++) {
//	  for (j = 1; j <= N; j++) {
//		for (k = 1; k <= O; k++) {
//		  div[IX(i, j, k, M, N)] = -1.0 / 3.0*((u[IX(i + 1, j, k, M, N)] - u[IX(i - 1, j, k, M, N)]) / M + (v[IX(i, j + 1, k, M, N)] - v[IX(i, j - 1, k, M, N)]) / M + (w[IX(i, j, k + 1, M, N)] - w[IX(i, j, k - 1, M, N)]) / M);
//		  p[IX(i, j, k, M, N)] = 0;
//		}
//	  }
//	}
//
//	set_bnd(M, N, O, 0, div); set_bnd(M, N, O, 0, p);
//
//	lin_solve(M, N, O, 0, p, div, 1, 6);
//
//	for (i = 1; i <= M; i++) {
//	  for (j = 1; j <= N; j++) {
//		for (k = 1; k <= O; k++) {
//		  u[IX(i, j, k, M, N)] -= 0.5f*M*(p[IX(i + 1, j, k, M, N)] - p[IX(i - 1, j, k, M, N)]);
//		  v[IX(i, j, k, M, N)] -= 0.5f*M*(p[IX(i, j + 1, k, M, N)] - p[IX(i, j - 1, k, M, N)]);
//		  w[IX(i, j, k, M, N)] -= 0.5f*M*(p[IX(i, j, k + 1, M, N)] - p[IX(i, j, k - 1, M, N)]);
//		}
//	  }
//	}
//
//  set_bnd(M, N, O, 1, u); set_bnd(M, N, O, 2, v); set_bnd(M, N, O, 3, w);
//}

void density_step(int M, int N, int O, int i, int j, int k, int idx, int right, int left, int top, int bottom, int front, int back, __global float * x, __global float * x0, __global float * u, __global float * v, __global float * w, float diff, float dt)
{
  if (i == 0 || j == 0 || k == 0 || i == (M + 1) || j == (N + 1) || k == (O + 1)) {
    set_bnd(M, N, O, i, j, k, idx, 0, x, x0);
  }
  else {
    add_source(idx, x, x0, dt);
    diffuse(M, N, O, idx, right, left, top, bottom, front, back, 0, x0, x, diff, dt);
    advect(M, N, O, i, j, k, idx, 0, x, x0, u, v, w, dt);
  }
}

//void density_step(int M, int N, int O, __global float * x, __global float * x0, __global float * u, __global float * v, __global float * w, float diff, float dt)
//{
//  add_source(M, N, O, x, x0, dt);
//  SWAP(x0, x); diffuse(M, N, O, 0, x, x0, diff, dt);
//  SWAP(x0, x); advect(M, N, O, 0, x, x0, u, v, w, dt);
//}

//void velocity_step(int M, int N, int O, __global float * u, __global float * v, __global float * w, __global float * u0, __global float * v0, __global float * w0, float visc, float dt)
//{
//	add_source(M, N, O, u, u0, dt); add_source(M, N, O, v, v0, dt); add_source(M, N, O, w, w0, dt);
//	SWAP(u0, u); diffuse(M, N, O, 1, u, u0, visc, dt);
//	SWAP(v0, v); diffuse(M, N, O, 2, v, v0, visc, dt);
//	SWAP(w0, w); diffuse(M, N, O, 3, w, w0, visc, dt);
//	project(M, N, O, u, v, w, u0, v0);
//	SWAP(u0, u); SWAP(v0, v); SWAP(w0, w);
//	advect(M, N, O, 1, u, u0, u0, v0, w0, dt); advect(M, N, O, 2, v, v0, u0, v0, w0, dt); advect(M, N, O, 3, w, w0, u0, v0, w0, dt);
//	project(M, N, O, u, v, w, u0, v0);
//}

//// Kernel: Opengl x y z space.  left -1 in x, back -1 in z, top +1 in y  etc...
//__private int idx;
//__private int right, left, top, bottom, front, back;

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
  int idx = get_global_id(0);
  int k = idx / (x_length+2)*(y_length+2);
  int j = (idx - k*(x_length+2)*(y_length+2)) / (x_length+2);
  int i = (idx - k*(x_length+2)*(y_length+2) - j*(x_length+2));  
  
  int right, left, top, bottom, front, back;

  right = IX(i+1, j, k, x_length, y_length);
  left = IX(i-1, j, k, x_length, y_length);
  top = IX(i, j+1, k, x_length, y_length);
  bottom = IX(i, j-1, k, x_length, y_length);
  front = IX(i, j, k+1, x_length, y_length);
  back = IX(i, j, k-1, x_length, y_length);
  
  //velocity_step(x_length, y_length, z_length, u, v, w, u_prev, v_prev, w_prev, viscosity, dt);
  density_step(x_length, y_length, z_length, i, j , k, idx, right, left, top, bottom, front, back, dens, dens_prev, u, v, w, diffuse_rate, dt);
}     