#include <cstdlib>
#include <cmath>

#include "Noise.h"
using namespace std;

float Noise::uniform() {
  return uniform(-1.f,1.f);
}

float Noise::uniform(float a, float b) {
  return (rand()/((float)RAND_MAX/(b-a)))+a;
}

float Noise::gaussianNoise() {
  float noise = 0.f;
  for(unsigned int i=0 ; i < gaussianNoiseIterations ; i++){
	noise+=uniform(-1.f, 1.f);
  }
  return noise/gaussianNoiseIterations;
}

float Noise::gaussianNoise(float var, float mean) {
  return var*gaussianNoise()+mean;
}

float Noise::cosineInterpolation(float a, float b, float x) {
  float ft = x * M_PI;
  float f = (1 - cos(ft)) * 0.5;

  return  a*(1-f) + b*f;
}

/******************************************************************************/

#define ARAD 16

void Wavelet::downsample (float *from, float *to, int n, int stride) {
  float *a;
  static float aCoeffs[2*ARAD] = {
	0.000334,-0.001528, 0.000410, 0.003545,-0.000938,-0.008233, 0.002172, 0.019120,
	-0.005040,-0.044412, 0.011655, 0.103311,-0.025936,-0.243780, 0.033979, 0.655340,
	0.655340, 0.033979,-0.243780,-0.025936, 0.103311, 0.011655,-0.044412,-0.005040,
	0.019120, 0.002172,-0.008233,-0.000938, 0.003546, 0.000410,-0.001528, 0.000334};
  a = &aCoeffs[ARAD];
  for (int i=0; i<n/2; i++) {
	to[i*stride] = 0;
	for (int k=2*i-ARAD; k<=2*i+ARAD; k++)
	  to[i*stride] += a[k-2*i] * from[mod(k,n)*stride];
  }
}

void Wavelet::upsample( float *from, float *to, int n, int stride) {
  float *p, pCoeffs[4] = { 0.25, 0.75, 0.75, 0.25 };
  p = &pCoeffs[2];
  for (int i=0; i<n; i++) {
	to[i*stride] = 0;
	for (int k=i/2; k<=i/2+1; k++)
	  to[i*stride] += p[i-2*k] * from[mod(k,n/2)*stride];
  }
}

float Wavelet::wNoise(const Vec3Df &p) {
  /* Non-projected 3D noise */
  int i, f[3], c[3], mid[3], n=noiseTileSize;

  /* f, c = filter, noise coeff indices */
  float w[3][3], t, result =0;

  /* Evaluate quadratic B-spline basis functions */
  for (i=0; i<3; i++) {
	mid[i]=ceil(p[i]-0.5); t=mid[i]-(p[i]-0.5);
	w[i][0]=t*t/2; w[i][2]=(1-t)*(1-t)/2; w[i][1]=1-w[i][0]-w[i][2];
  }

  /* Evaluate noise by weighting noise coefficients by basis function values */
  for(f[2]=-1;f[2]<=1;f[2]++)
	for(f[1]=-1;f[1]<=1;f[1]++)
	  for(f[0]=-1;f[0]<=1;f[0]++) {
		float weight=1;
		for (i=0; i<3; i++) {
		  c[i]=mod(mid[i]+f[i],n); weight*=w[i][f[i]+1];
		}
		result += weight * noiseTileData[c[2]*n*n+c[1]*n+c[0]];
	  }
  return result;
}

float Wavelet::wProjectedNoise(const Vec3Df &p, const Vec3Df &normal) {
  /* 3D noise projected onto 2D */
  int i, c[3], min[3], max[3], n=noiseTileSize;

  /* c = noise coeff location */
  float support;
  float result=0;

  /* Bound the support of the basis functions for this projection direction */
  for (i=0; i<3; i++) {
	support = 3*abs(normal[i]) + 3*sqrt((1-normal[i]*normal[i])/2);
	min[i] = ceil( p[i] - support );
	max[i] = floor( p[i] + support );
  }

  /* Loop over the noise coefficients within the bound. */
  for(c[2]=min[2];c[2]<=max[2];c[2]++) {
	for(c[1]=min[1];c[1]<=max[1];c[1]++) {
	  for(c[0]=min[0];c[0]<=max[0];c[0]++) {
		float t, t1, t2, t3, dot=0, weight=1;

		/* Dot the normal with the vector from c to p */
		for (i=0; i<3; i++) {dot+=normal[i]*(p[i]-c[i]);}

		/* Evaluate the basis function at c moved halfway to p along the normal. */
		for (i=0; i<3; i++) {
		  t = (c[i]+normal[i]*dot/2)-(p[i]-1.5); t1=t-1; t2=2-t; t3=3-t;
		  weight*=(t<=0||t>=3)? 0 : (t<1) ? t*t/2 : (t<2)? 1-(t1*t1+t2*t2)/2 : t3*t3/2;
		}

		/* Evaluate noise by weighting noise coefficients by basis function values. */
		result += weight * noiseTileData[mod(c[2],n)*n*n+mod(c[1],n)*n+mod(c[0],n)];
	  }
	}
  }
  return result;
}

void Wavelet::generateNoiseTile() {
  const int n = noiseTileSize;

  int ix, iy, iz, i, sz=n*n*n;
  float *temp1 = new float[sz];
  float *temp2 = new float[sz];
  float *noise = new float[sz];

  /* Step 1. Fill the tile with random numbers in the range -1 to 1. */
  for (i=0; i<n*n*n; i++)
	noise[i] = random();

  /* Steps 2 and 3. Downsample and upsample the tile */
  for (iy=0; iy<n; iy++)
	for (iz=0; iz<n; iz++) {
	  /* each x row */
	  i = iy*n + iz*n*n;
	  downsample( &noise[i], &temp1[i], n, 1 );
	  upsample(&temp1[i], &temp2[i], n, 1 );
	}
  for (ix=0; ix<n; ix++)
	for (iz=0; iz<n; iz++) {
	  /* each y row */
	  i = ix + iz*n*n;
	  downsample( &temp2[i], &temp1[i], n, n );
	  upsample(&temp1[i], &temp2[i], n, n );
	}
  for (ix=0; ix<n; ix++)
	for (iy=0; iy<n; iy++) {
	  /* each z row */
	  i = ix + iy*n;
	  downsample( &temp2[i], &temp1[i], n, n*n );
	  upsample(&temp1[i], &temp2[i], n, n*n );
	}

  /* Step 4. Subtract out the coarse-scale contribution */
  for (i=0; i<n*n*n; i++) {
	noise[i]-=temp2[i];
  }

  /* Avoid even/odd variance difference by adding odd-offset
	 version of noise to itself.*/
  int offset=n/2;
  if (offset%2==0) offset++;

  for (i=0,ix=0; ix<n; ix++)
	for (iy=0; iy<n; iy++)
	  for (iz=0; iz<n; iz++)
		temp1[i++] = noise[mod(ix+offset,n) +
						   mod(iy+offset,n)*n +
						   mod(iz+offset,n)*n*n];

  for (i=0; i<n*n*n; i++) {
	noise[i]+=temp1[i];
  }

  delete[] noiseTileData;
  delete[] temp1;
  delete[] temp2;
  noiseTileData=noise;
}

float Wavelet::multibandNoise(const Vec3Df &p, const Vec3Df * normal) {
  Vec3Df q;
  float result=0, variance=0;
  int nbands = w.size();

  for(int b=0; b<nbands && s+firstBand+b<0; b++) {
	for (int i=0; i<=2; i++) {
	  q[i]=2*p[i]*pow(2,firstBand+b);
	}
	result += (normal) ? w[b] * wProjectedNoise(q,*normal) : w[b] * wNoise(q);
  }
  for (int b=0; b<nbands; b++) {
	variance+=w[b]*w[b];
  }
  /* Adjust the noise so it has a variance of 1. */
  if(variance)
	result /= sqrt(variance * ((normal) ? 0.296 : 0.210));
  return result;
}
