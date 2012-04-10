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

#extension GL_EXT_gpu_shader4 : enable
#extension GL_ARB_gpu_shader5 : enable

varying vec4 P;
varying vec3 N;

// phong brdf parameters
uniform float diffuseRef;
uniform float specRef;
uniform float shininess;

// perlin noise properties
uniform int octave;
uniform float persistence;
uniform float f0;
uniform float t;

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

float Noise4(int x, int y, int z, int t)
{

	int c = 57;
	int n = x*c;
	c *= 57;
	n += c*y;
	c *= 57;
	n += c*z;
	c *= 57;
	n += c*t;

	n = (n<<13) ^ n;
	return 1.0 - ( (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0;
}

float InterpolatedNoise2D(float x, float y, int z, int t)
{
	int integer_X    = int(floor(x));
	float fractional_X = x - integer_X;

	int integer_Y    = int(floor(y));
	float fractional_Y = y - integer_Y;

	float v1 = Noise4(integer_X,     integer_Y, z, t);
	float v2 = Noise4(integer_X + 1, integer_Y, z, t);
	float v3 = Noise4(integer_X,     integer_Y + 1, z, t);
	float v4 = Noise4(integer_X + 1, integer_Y + 1, z, t);
	

	float i1 = Interpolate(v1 , v2 , fractional_X);
	float i2 = Interpolate(v3 , v4 , fractional_X);

	return Interpolate(i1 , i2 , fractional_Y);
}

float InterpolatedNoise3D(float x, float y, float z, int t)
{
	int integer_Z    = int(floor(z));
	float fractional_Z = z - integer_Z;

	float v1 = InterpolatedNoise2D(x, y, integer_Z, t);
	float v2 = InterpolatedNoise2D(x, y, integer_Z+1, t);

	return Interpolate(v1, v2, fractional_Z);
}

float InterpolatedNoise4D(float x, float y, float z, float t)
{
	int integer_T    = int(floor(t));
	float fractional_T = t - integer_T;

	float v1 = InterpolatedNoise3D(x, y, z, integer_T);
	float v2 = InterpolatedNoise3D(x, y, z, integer_T+1);

	return Interpolate(v1, v2, fractional_T);
}

float PerlinNoise_4D(float x, float y, float z, float t)
{	
  float p = persistence;
  float n = octave;

  float frequency = f0;
  float amplitude = 1.0;

  float total = 0;
  for (int i=0; i<=n; i++)	{

	  total += InterpolatedNoise4D(x * frequency, y * frequency, z*frequency, t*frequency) * amplitude;

    frequency *= 2.0;
    amplitude *= p;	 
  }

  return total * (1.0 -p) / (1.0 - amplitude);
}

void main(void) {
	gl_FragColor = vec4 (0.0, 0.0, 0.0, 1);

	vec3 c1 = vec3(0.5, 0.5, 0.7); // shiny grey
	vec3 c2 = vec3(0.3,0.3,0.3); // white
	vec3 p = vec3 (gl_ModelViewMatrix * P);

	// PERLIN NOISE

	float c = (PerlinNoise_4D(P.x, P.y, P.z, t) + 1.0)/2.0;
	float value = 1 - sqrt(abs(sin(2 * 3.141592 *c)));
	gl_FragColor.rgb += (c1.r * (1 - value) + c2.r * value, c1.g * (1 - value) + c2.g * value, c1.b * (1 - value) + c2.b * value);

	// BRDF
	vec3 n = normalize (gl_NormalMatrix * N);
	vec3 l = normalize (gl_LightSource[0].position.xyz - p);

	vec3 r = reflect (-l, n);
	vec3 v = normalize (-p);

	float diffuse = max(0.0,dot(n, l));
	float spec = pow(max(0.0, dot(r, v)), shininess);

	vec4 LightContribution =  diffuseRef * diffuse * gl_LightSource[0].diffuse + 
		specRef * spec * gl_LightSource[0].specular;


	gl_FragColor += vec4(LightContribution.xyz, 1);


}

