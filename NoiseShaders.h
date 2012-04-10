#ifndef NOISE_SHADERS_H
#define NOISE_SHADERS_H

static Wavelet wNoise(2);

class PhongShader : public Shader
{
	public:
		PhongShader() {}
		inline virtual ~PhongShader () {}

		// BRDF properties
		void setDiffuseRef (float s) {
			glUniform1fARB (diffuseRefLocation, s); 
		}

		void setSpecRef (float s) {
			glUniform1fARB (specRefLocation, s); 
		}

		void setShininess (float s) {
			glUniform1fARB (shininessLocation, s); 
		}
	protected:
		void init (const std::string & vertexShaderFilename,
				const std::string & fragmentShaderFilename) {
			loadFromFile (vertexShaderFilename, fragmentShaderFilename);

			// brdf uniform var
			specRefLocation = getUniLoc ("specRef");
			diffuseRefLocation = getUniLoc ("diffuseRef");
			shininessLocation = getUniLoc ("shininess");
		}

	private:
		// brdf
		GLint diffuseRefLocation;
		GLint specRefLocation;
		GLint shininessLocation;
};

class PerlinShader : public PhongShader {
	public:
		PerlinShader () { init ("shader.vert", "shaderPerlin.frag"); }
		inline virtual ~PerlinShader() {}

		// Perlin properties
		void setnbOctave (int s) {
			glUniform1iARB (nbOctaveLocation, s); 
		}

		void setF0 (float s) {
			glUniform1fARB (f0Location, s); 
		}   

		void setPersistence (float s) {
			glUniform1fARB (persistenceLocation, s); 
		}

		void setTime(float t) {
			glUniform1fARB(timeLocation, t);
		}

	private:
		void init (const std::string & vertexShaderFilename,
				const std::string & fragmentShaderFilename) {
			PhongShader::init(vertexShaderFilename, fragmentShaderFilename);

			// perlin uniform var
			nbOctaveLocation = getUniLoc ("octave");
			persistenceLocation = getUniLoc ("persistence");
			f0Location = getUniLoc ("f0");
			timeLocation = getUniLoc("t");
		}

		// perlin
		GLint nbOctaveLocation;
		GLint persistenceLocation;
		GLint f0Location;
		GLint timeLocation;
};

class WaveletShader : public PhongShader
{
	public:
		WaveletShader () { init ("shader.vert", "shaderWavelet.frag"); }
		inline virtual ~WaveletShader() {}

		// Wavelet properties
		void setnBandsRef (int s) {
			glUniform1iARB (nBandsLocation, s); 
		}

		void setfirstBand (int s) {
			glUniform1iARB (firstBandLocation, s); 
		}

		void setTileSize (int s) {
			wNoise.generateNoiseTile(s);
			glUniform1iARB (tileSizeLocation, s); 
			setNoiseData(s);
		}

		void setNoiseprojected (bool s) {
			glUniform1fARB (noiseProjectedLocation, s); 
		}

		void sets (float s) {
			glUniform1fARB (sLocation, s); 
		}

	private:
		void init (const std::string & vertexShaderFilename,
				const std::string & fragmentShaderFilename) {
			PhongShader::init(vertexShaderFilename, fragmentShaderFilename);

			// wavelet uniform var
			arrayLocation = getUniLoc ("noiseData");
			nBandsLocation = getUniLoc ("nbands");
			firstBandLocation = getUniLoc ("firstBand");
			tileSizeLocation = getUniLoc ("noiseTileSize");
			noiseProjectedLocation = getUniLoc ("noiseProjected");
			sLocation = getUniLoc ("s");

		}

		void setNoiseData(int s) {
			glUniform1fvARB(arrayLocation, s*s*s, wNoise.noiseTileData);
		}

		// wavelet
		GLint arrayLocation;
		GLint nBandsLocation;
		GLint firstBandLocation;
		GLint tileSizeLocation;
		GLint noiseProjectedLocation;
		GLint sLocation;
};

class GaborShader : public PhongShader {
	public:
		GaborShader () { init ("shader.vert", "shaderGabor.frag"); }
		inline virtual ~GaborShader() {}

		// Gabor properties
		void setKRef (float s) {
			glUniform1fARB (KRefLocation, s); 
		}

		void setARef (float s) {
			glUniform1fARB (ARefLocation, s); 
		}

		void setOmegaRef (float s) {
			glUniform1fARB (OmegaRefLocation, s); 
		}

		void setIsoRef (bool s) {
			glUniform1fARB (IsoRefLocation, s); 
		}

	private:
		void init (const std::string & vertexShaderFilename,
				const std::string & fragmentShaderFilename) {
			PhongShader::init(vertexShaderFilename, fragmentShaderFilename);

			// gabor uniform var
			KRefLocation = getUniLoc ("K");
			OmegaRefLocation = getUniLoc ("omega_0");
			ARefLocation = getUniLoc ("a");
			IsoRefLocation = getUniLoc ("iso");
		}

		// gabor
		GLint KRefLocation;
		GLint ARefLocation;
		GLint OmegaRefLocation;
		GLint IsoRefLocation;
};

#endif // ifndef NOISE_SHADERS_H
