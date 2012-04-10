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

// to select the noise type:
//   0: perlin
//   1: wavelet
//   2: gabor
uniform float noise_type;

// perlin noise properties
uniform int octave;
uniform float persistence;
uniform float f0;

// gabor kernel properties
uniform float K;
uniform float omega_0;
uniform float a;
uniform bool iso;

float F_0 = 0.0625;
float number_of_impulses_per_kernel = 64.0;
float radius = sqrt(-log(0.05) / 3.14159265) / a;
float impulseDensity = number_of_impulses_per_kernel / (3.14159265 * radius * radius);

// wavelet properties
uniform float noiseData[6*6*6];
uniform int noiseTileSize;
uniform bool noiseProjected;
uniform int nbands;
uniform float s;
uniform int firstBand;



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
















///////////////////////////////////////////////
///////
///////           PERLIN NOISE
///////
///////////////////////////////////////////////
float Interpolate(float a, float b, float x)
{
  float ft = x * 3.1415927;
  float f = (1.0 - cos(ft)) * 0.5;

  return  a * (1.0-f) + b*f;
}

float Noise(int x, int y)
{
  int n = (x + y * 57);

  n = (n<<13) ^ n;
  return ( 1.0 - ( (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0);    
}

float SmoothNoise1(float x, float y)
{
  float corners = ( Noise(int(x-1), int(y-1)) +
                    Noise(int(x+1), int(y-1)) +
                    Noise(int(x-1), int(y+1)) +
                    Noise(int(x+1), int(y+1)) ) / 16.0;

  float sides   = ( Noise(int(x-1), int(y))  +
                    Noise(int(x+1), int(y))  +
                    Noise(int(x), int(y-1))  +
                    Noise(int(x), int(y+1)) ) /  8.0;

  float center  =   Noise(int(x), int(y)) / 4.0;

  return corners + sides + center;
}

float InterpolatedNoise_1(float x, float y)
{

  int integer_X    = int(floor(x));
  float fractional_X = x - float(integer_X);

  int integer_Y    = int(floor(y));
  float fractional_Y = y - float(integer_Y);

  float v1 = SmoothNoise1(integer_X,     integer_Y);
  float v2 = SmoothNoise1(integer_X + 1, integer_Y);
  float v3 = SmoothNoise1(integer_X,     integer_Y + 1);
  float v4 = SmoothNoise1(integer_X + 1, integer_Y + 1);

  float i1 = Interpolate(v1 , v2 , fractional_X);
  float i2 = Interpolate(v3 , v4 , fractional_X);

  return Interpolate(i1 , i2 , fractional_Y);
}

float PerlinNoise_2D(float x, float y)
{	
  float p = persistence;
  int n = octave;

  float frequency=f0;
  float amplitude=1.0;

  float total = 0;
  for (int i=0; i<n; i++)	{

    total += InterpolatedNoise_1(x * frequency, y * frequency) * amplitude;

    frequency *= 2.0;
    amplitude *= p;	 
  }

  return total * (1.0 -p ) / (1.0 - amplitude);
}


















///////////////////////////////////////////////
///////
///////           WAVELET NOISE
///////
///////////////////////////////////////////////
/*float W[5] = {  1.0/pow(2.0, 0),
                1.0/pow(2.0, 1),
                1.0/pow(2.0, 2),
                1.0/pow(2.0, 3),
                1.0/pow(2.0, 4),
                };*/

float W[5] = {  1.0,
                0.9,
                0.8,
                0.7,
                0.6,
                };

int mod(int x, int n) {
  int m=x%n;
  return (m<0) ? m+n : m;
}

float wNoise(vec3 p) {
  // Non-projected 3D noise
  int i, f[3], c[3], mid[3], n=noiseTileSize;

  // f, c = filter, noise coeff indices
  float w[3*3], t, result = 0.0;

  // Evaluate quadratic B-spline basis functions
  for (i=0; i<3; i++) {
    mid[i]=int(ceil(p[i]-0.5)); t=float(mid[i])-(p[i]-0.5);
    w[3*i + 0] = t*t/2; 
    w[3*i + 2] = (1-t)*(1-t)/2; 
    w[3*i + 1] = 1-w[3*i+0]-w[3*i+2];
  }

  // Evaluate noise by weighting noise coefficients by basis function values
  for(f[2]=-1;f[2]<=1;f[2]++) {
    for(f[1]=-1;f[1]<=1;f[1]++) {
      for(f[0]=-1;f[0]<=1;f[0]++) {
        float weight = 1.0;
        for (i=0; i<3; i++) {
          c[i]=mod(mid[i]+f[i], n); 
          weight *= w[3*i + f[i]+1];
        }
        result += weight * noiseData[c[2]*n*n + c[1]*n + c[0]];
      }
    }
  }
  return result;
}


float wProjectedNoise(vec3 p,vec3 normal) {
  // 3D noise projected onto 2D
  int i, c[3], min[3], max[3], n=noiseTileSize;

  // c = noise coeff location
  float support;
  float result=0;

  // Bound the support of the basis functions for this projection direction
  for (i=0; i<3; i++) {
    support = 3*abs(normal[i]) + 3*sqrt((1-normal[i]*normal[i])/2);
    min[i] = int(ceil( p[i] - support ));
    max[i] = int(floor( p[i] + support ));
  }

  // Loop over the noise coefficients within the bound
  for(c[2]=min[2];c[2]<=max[2];c[2]++) {
    for(c[1]=min[1];c[1]<=max[1];c[1]++) {
      for(c[0]=min[0];c[0]<=max[0];c[0]++) {
        float t, t1, t2, t3, dot=0, weight=1.0;

        // Dot the normal with the vector from c to p
        for (i=0; i<3; i++) {dot+=normal[i]*(p[i]-c[i]);}

        // Evaluate the basis function at c moved halfway to p along the normal.
        for (i=0; i<3; i++) {
          t = (float(c[i])+normal[i]*dot/2.0)-(p[i]-1.5); t1=t-1; t2=2-t; t3=3-t;
          weight *= ( t<=0.0 || t>=3.0) ? 0.0 : (t<1.0) ? t*t/2.0 : (t<2.0)? 1.0-(t1*t1+t2*t2)/2.0 : t3*t3/2.0;
        }

        // Evaluate noise by weighting noise coefficients by basis function values
        result += weight * noiseData[mod(c[2],n)*n*n+mod(c[1],n)*n+mod(c[0],n)];
      }
    }
  }
  return result;
}


float multibandNoise(vec3 p, bool normal) {
  vec3 q;
  float result=0.0, variance=0.0;

  for(int b=0; (b<nbands) && (s + firstBand + b < 0.0); b++) {
    q = 2.0*p*pow(2,firstBand+b);
    result += (normal) ? W[b] * wProjectedNoise(q, N) : W[b]*wNoise(q);
  }
  for (int b=0; b<nbands; b++) {
    variance+=W[b]*W[b];
  }
  // Adjust the noise so it has a variance of 1.
  if(variance != 0.0)
    result /= sqrt(variance * ((normal) ? 0.296 : 0.210));
  return result;
}









void main(void) {
  gl_FragColor = vec4 (0.0, 0.0, 0.0, 1);

  vec3 p = vec3 (gl_ModelViewMatrix * P);


  // GABOR NOISE
  if(noise_type == 2.0) {
    float scale = 3.0 * sqrt(variance());
    float noise_gabor = 0.5 + 0.5 * GaborNoise(P.x*100, P.y*100)/scale;
    gl_FragColor.rgb += noise_gabor;
  } 
  // PERLIN NOISE
  else if(noise_type == 0.0) {
    float noise_perlin = (PerlinNoise_2D(P.x*10.0, P.y*10.0) + 1.0)/2.0;
    gl_FragColor.rgb += noise_perlin;
  } 
  // WAVELET NOISE
  else if(noise_type == 1.0) {
    float noise_wavelet = multibandNoise(vec3(P.x*100.0, P.y*100.0, P.z*100.0), noiseProjected);
    gl_FragColor.rgb += noise_wavelet;
  }



  // BRDF
  vec3 n = normalize (gl_NormalMatrix * N);
  vec3 l = normalize (gl_LightSource[0].position.xyz - p);

  vec3 r = reflect (-l, n);
  vec3 v = normalize (-p);

  float diffuse = max(0,dot(n, l));
  float spec = pow(max(0, dot(r, v)), shininess);

  vec4 LightContribution =  diffuseRef * diffuse * gl_LightSource[0].diffuse + 
                            specRef * spec * gl_LightSource[0].specular;


  gl_FragColor += vec4(LightContribution.xyz, 1 + noiseData[0]/1000);


}

