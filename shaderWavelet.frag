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

// phong brdf parameters
uniform float diffuseRef;
uniform float specRef;
uniform float shininess;

// wavelet properties
uniform float noiseData[6*6*6];
uniform int noiseTileSize;
uniform bool noiseProjected;
uniform int nbands;
uniform float s;
uniform int firstBand;

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


	// WAVELET NOISE
	float noise_wavelet = multibandNoise(vec3(P.x*100.0, P.y*100.0, P.z*100.0), noiseProjected);
	gl_FragColor.rgb += noise_wavelet;



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

