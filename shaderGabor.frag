// --------------------------------------------------------------------------
// gMini,
// a minimal Glut/OpenGL app to extend                              
//
// Copyright(C) 2007-2009                
// Tamy Boubekeur
//                                                                            
// All rights reserved.                                                       
//                                                                            
// This program is free software; you can redistribute it and/or modify       
// it under the terms of the GNU General Public License as published by       
// the Free Software Foundation; either version 2 of the License, or          
// (at your option) any later version.                                        
//                                                                            
// This program is distributed in the hope that it will be useful,            
// but WITHOUT ANY WARRANTY; without even the implied warranty of             
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              
// GNU General Public License (http://www.gnu.org/licenses/gpl.txt)           
// for more details.                                                          
//                                                                          
// --------------------------------------------------------------------------

varying vec4 P;
varying vec3 N;

// seed
uint seed_kernel;

// phong brdf parameters
uniform float diffuseRef;
uniform float specRef;
uniform float shininess;

// gabor kernel properties
uniform float K;
uniform float omega_0;
uniform float a;
uniform bool iso;

float F_0 = 0.0625;
float number_of_impulses_per_kernel = 64.0;
float radius = sqrt(-log(0.05) / 3.14159265) / a;
float impulseDensity = number_of_impulses_per_kernel / (3.14159265 * radius * radius);

///////////////////////////////////////////////
///////
///////           GABOR NOISE
///////
///////////////////////////////////////////////
void seed(uint s_) { 
    seed_kernel = s_; 
}

uint random() { 
    seed_kernel *= 3039177861u; 
    return seed_kernel; 
}

float uniform_0_1() { 
    return float(random()) / 4294967295.0; 
}

float unif(float min, float max) { 
    return (min + (uniform_0_1() * (max - min))); 
}

uint poisson(float mean)
{
  float g_ = exp(-mean);
  uint em = 0;
  float t = uniform_0_1();
  while (t > g_) {
    ++em;
    t *= uniform_0_1();
  }
  return em;
}




float gabor(float omega, float x, float y)
{
  float gaussian_envelop = K * exp(-3.14159265 * (a * a) * ((x * x) + (y * y)));
  float sinusoidal_carrier = cos(2.0 * 3.14159265 * F_0 * ((x * cos(omega)) + (y * sin(omega))));
  return gaussian_envelop * sinusoidal_carrier;
}

uint morton(uint x, uint y)
{
  uint z = 0;
  for (uint i = 0; i < (4 * 8); ++i) {
    z |= ((x & (1 << i)) << i) | ((y & (1 << i)) << (i + 1));
  }
  return z;
}




float cell(int i, int j, float x, float y)
{

  uint seed_cell = morton(uint(i), uint(j));

  if (seed_cell == 0) {
    seed_cell = 1;
  }
  seed(seed_cell);

  float number_of_impulses_per_cell = impulseDensity * radius * radius;

  uint number_of_impulses = poisson(number_of_impulses_per_cell);

  float noise = 0.0;
  for (uint i = 0; i < number_of_impulses; ++i) {
    float x_i = uniform_0_1();
    float y_i = uniform_0_1();
    float w_i = unif(-1.0, +1.0);
    float omega_0_i = iso ? unif(0.0, 2.0 * 3.14159265) : omega_0;
    float x_i_x = x - x_i;
    float y_i_y = y - y_i;

    if (((x_i_x * x_i_x) + (y_i_y * y_i_y)) < 1.0) {
      noise = noise + w_i * gabor(omega_0_i, x_i_x*radius, y_i_y*radius);
    }
  }
  return noise;
}

float GaborNoise(float x, float y) {
    x /= radius; 
    y /= radius;

    int int_x = int(floor(x));
    int int_y = int(floor(y));

    float frac_x = x - float(int_x); 
    float frac_y = y - float(int_y);

    float noise = 0.0;

    for (int di = -1; di <= +1; ++di) {
      for (int dj = -1; dj <= +1; ++dj) {
        noise += cell(int_x + di, int_y + dj, frac_x - di, frac_y - dj);
      }
    }
    return noise;
}


float variance()
{
  float integral_gabor_filter_squared = ((K * K) / (4.0 * a * a)) * (1.0 + exp(-(2.0 * 3.14159265 * F_0 * F_0) / (a * a)));
  return impulseDensity * (1.0 / 3.0) * integral_gabor_filter_squared;
}

void main(void) {
	gl_FragColor = vec4 (0.0, 0.0, 0.0, 1);

	vec3 p = vec3 (gl_ModelViewMatrix * P);


	// GABOR NOISE
	float scale = 3.0 * sqrt(variance());
	float noise_gabor = 0.5 + 0.5 * GaborNoise(P.x*100, P.y*100)/scale;
	gl_FragColor.rgb += noise_gabor;



	// BRDF
	vec3 n = normalize (gl_NormalMatrix * N);
	vec3 l = normalize (gl_LightSource[0].position.xyz - p);

	vec3 r = reflect (-l, n);
	vec3 v = normalize (-p);

	float diffuse = max(0,dot(n, l));
	float spec = pow(max(0, dot(r, v)), shininess);

	vec4 LightContribution =  diffuseRef * diffuse * gl_LightSource[0].diffuse + 
		specRef * spec * gl_LightSource[0].specular;


	gl_FragColor += vec4(LightContribution.xyz, 1);


}

