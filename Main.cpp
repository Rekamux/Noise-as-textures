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

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdlib>

#define GLEW_STATIC 1
#include <GL/glew.h>
#include <GL/glut.h>

#include "Shader.h"
#include "Vec3D.h"
#include "Vertex.h"
#include "Triangle.h"
#include "Mesh.h"
#include "Camera.h"
#include "Noise.h"
#include "NoiseShaders.h"

using namespace std;

static GLint window;
static unsigned int SCREENWIDTH = 1024;
static unsigned int SCREENHEIGHT = 768;
static Camera camera;
static bool mouseRotatePressed = false;
static bool mouseMovePressed = false;
static bool mouseZoomPressed = false;
static int lastX=0, lastY=0, lastZoom=0;
static unsigned int FPS = 0;

static PhongShader * shader;
static PerlinShader * perlinShader;
static GaborShader * gaborShader;
static WaveletShader * waveletShader;

static Mesh mesh;
static GLuint glID;


typedef enum {Solid, Phong} RenderingMode;
static RenderingMode mode = Phong;

Mesh openOFF (const std::string filename, unsigned int normWeight) {
	vector<Vertex> V;
	vector<Triangle> T;

	ifstream in (filename.c_str ());
	if (!in) 
		exit (EXIT_FAILURE);
	string offString;
	unsigned int sizeV, sizeT, tmp;
	in >> offString >> sizeV >> sizeT >> tmp;
	for (unsigned int i = 0; i < sizeV; i++) {
		Vec3Df p;
		in >> p;
		V.push_back (Vertex (p));
	}
	int s;
	for (unsigned int i = 0; i < sizeT; i++) {
		in >> s;
		unsigned int v[3];
		for (unsigned int j = 0; j < 3; j++)
			in >> v[j];
		T.push_back (Triangle (v[0], v[1], v[2]));
	}
	in.close ();

	Vec3Df center;
	float radius;
	Vertex::scaleToUnitBox (V, center, radius);
	Mesh mesh (V, T);
	mesh.recomputeSmoothVertexNormals (normWeight);
	return mesh;
}

inline void glVertexVec3Df (const Vec3Df & v) {
	glVertex3f (v[0], v[1], v[2]);
}

inline void glNormalVec3Df (const Vec3Df & n) {
	glNormal3f (n[0], n[1], n[2]);
}

inline void glDrawPoint (const Vec3Df & pos, const Vec3Df & normal) {
	glNormalVec3Df (normal);
	glVertexVec3Df (pos);
}

inline void glDrawPoint (const Vertex & v) { 
	glDrawPoint (v.getPos (), v.getNormal ()); 
}

// perlin properties
static int nbOctave = 4;
static float persistence = 0.5;
static int f0 = 1.0;
static float perlinTime = 0;

// gabor properties
static float K = 1.0f;
static float omega = 0.0;
static float a = 0.05;
static bool iso = true;

// Phong properties
static float diffuseRef = 0.8f;
static float specRef = 1.5f;
static float shininess = 16.0f;

// wavelet properties
static int nbands = 1;
static int firstBand = -1;
static int tileSize = 2;
static bool noiseProjected = false;
static float s = 0.0;

void setShaderValues () {

	// wavelet
	if (shader == waveletShader) {
		waveletShader->setTileSize (tileSize);
		waveletShader->setnBandsRef (nbands);
		waveletShader->setfirstBand (firstBand);
		waveletShader->setNoiseprojected (noiseProjected);
		waveletShader->sets (s);
	}

	// perlin
	if (shader == perlinShader) {
		perlinShader->setnbOctave (nbOctave);
		perlinShader->setPersistence (persistence);
		perlinShader->setF0 (f0); 
		perlinShader->setTime(perlinTime);
	}

	// gabor
	if (shader == gaborShader) {
		gaborShader->setKRef (K);
		gaborShader->setOmegaRef (omega);
		gaborShader->setARef (a);
		gaborShader->setIsoRef (iso);
	}

	// brdf
	shader->setDiffuseRef (diffuseRef);
	shader->setSpecRef (specRef);
	shader->setShininess (shininess);
}

void drawMesh (bool flat) {
	const vector<Vertex> & V = mesh.getVertices ();
	const vector<Triangle> & T = mesh.getTriangles ();
	glBegin (GL_TRIANGLES);
	for (unsigned int i = 0; i < T.size (); i++) {
		const Triangle & t = T[i];
		if (flat) {
			Vec3Df normal = Vec3Df::crossProduct (V[t.getVertex (1)].getPos ()
					- V[t.getVertex (0)].getPos (),
					V[t.getVertex (2)].getPos ()
					- V[t.getVertex (0)].getPos ());
			normal.normalize ();
			glNormalVec3Df (normal);
		}
		for (unsigned int j = 0; j < 3; j++) 
			if (!flat) {
				glNormalVec3Df (V[t.getVertex (j)].getNormal ());
				glVertexVec3Df (V[t.getVertex (j)].getPos ());
			} else
				glVertexVec3Df (V[t.getVertex (j)].getPos ());
	}
	glEnd ();
}

void drawSolidModel () {
	glEnable (GL_LIGHTING);
	glEnable (GL_COLOR_MATERIAL);
	glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	glPolygonOffset (1.0, 1.0);
	glEnable (GL_POLYGON_OFFSET_FILL);
	glShadeModel (GL_FLAT);
	shader->bind ();
	drawMesh (true);    
	glPolygonMode (GL_FRONT, GL_LINE);
	glPolygonMode (GL_BACK, GL_FILL);
	glColor3f (0.0, 0.0, 0.0);
	drawMesh (true);
	glDisable (GL_POLYGON_OFFSET_FILL);
	glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	glDisable (GL_COLOR_MATERIAL);
	glDisable (GL_LIGHTING);
	glShadeModel (GL_SMOOTH);
}

void drawPhongModel () {
	glCallList (glID);
}

void initLights () {
	GLfloat light_position0[4] = {-50, 50, -10, 0};
	GLfloat light_position1[4] = {42, 374, 161, 0};
	GLfloat light_position2[4] = {473, -351, -259, 0};
	GLfloat light_position3[4] = {-438, 167, -48, 0};

	GLfloat direction1[3] = {-42, -374, -161,};
	GLfloat direction2[3] = {-473, 351, 259};
	GLfloat direction3[3] = {438, -167, 48};

	GLfloat color1[4] = {1.0, 1.0, 1.0, 1};
	GLfloat color2[4] = {0.28, 0.39, 1.0, 1};
	GLfloat color3[4] = {1.0, 0.69, 0.23, 1};

	GLfloat specularColor1[4] = {0.8, 0.8, 0.8, 1};
	GLfloat specularColor2[4] = {0.8, 0.8, 0.8, 1};
	GLfloat specularColor3[4] = {0.8, 0.8, 0.8, 1};

	GLfloat ambient[4] = {0.3f, 0.3f, 0.3f, 0.5f};

	glLightfv (GL_LIGHT0, GL_POSITION, light_position0);

	glLightfv (GL_LIGHT1, GL_POSITION, light_position1);
	glLightfv (GL_LIGHT1, GL_SPOT_DIRECTION, direction1);
	glLightfv (GL_LIGHT1, GL_DIFFUSE, color1);
	glLightfv (GL_LIGHT1, GL_SPECULAR, specularColor1);

	glLightfv (GL_LIGHT2, GL_POSITION, light_position2);
	glLightfv (GL_LIGHT2, GL_SPOT_DIRECTION, direction2);
	glLightfv (GL_LIGHT2, GL_DIFFUSE, color2);
	glLightfv (GL_LIGHT2, GL_SPECULAR, specularColor2);

	glLightfv (GL_LIGHT3, GL_POSITION, light_position3);
	glLightfv (GL_LIGHT3, GL_SPOT_DIRECTION, direction3);
	glLightfv (GL_LIGHT3, GL_DIFFUSE, color3);
	glLightfv (GL_LIGHT3, GL_SPECULAR, specularColor3);

	glLightModelfv (GL_LIGHT_MODEL_AMBIENT, ambient);

	glEnable (GL_LIGHTING);
}

void setSunriseLight () {
	glDisable (GL_LIGHT0);
	glDisable (GL_LIGHT1);
	glDisable (GL_LIGHT2);
	glDisable (GL_LIGHT3);
}

void setSingleSpotLight () {
	glEnable (GL_LIGHT0);
	glDisable (GL_LIGHT1);
	glDisable (GL_LIGHT2);
	glDisable (GL_LIGHT3);
}

void setDefaultMaterial () {
	GLfloat material_color[4] = {1,1,1,1.0f};
	GLfloat material_specular[4] = {0.5,0.5,0.5,1.0};
	GLfloat material_ambient[4] = {0.0,0.0,0.0,1.0};

	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, material_specular);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, material_color);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, material_ambient);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 128);

	glDisable (GL_COLOR_MATERIAL);
}

void initGLList () {
	glID = glGenLists (1);
	glNewList (glID, GL_COMPILE);
	drawMesh (false);
	glEndList ();
}

void init (const std::string & filename) {
	glewInit();
	if (glewGetExtension ("GL_ARB_vertex_shader")        != GL_TRUE ||
			glewGetExtension ("GL_ARB_shader_objects")       != GL_TRUE ||
			glewGetExtension ("GL_ARB_shading_language_100") != GL_TRUE) {
		cerr << "Driver does not support OpenGL Shading Language" << endl;
		exit (EXIT_FAILURE);
	}
	if (glewGetExtension ("GL_ARB_vertex_buffer_object") != GL_TRUE) {
		cerr << "Driver does not support Vertex Buffer Objects" << endl;
		exit (EXIT_FAILURE);
	}

	camera.resize (SCREENWIDTH, SCREENHEIGHT);
	glClearColor (0.0, 0.0, 0.0, 1.0);

	initLights ();
	setSingleSpotLight ();
	setDefaultMaterial ();
	mesh = openOFF (filename, 0);
	initGLList ();

	try {
		cout << "Binding shaders...\n";
		cout << "Perlin...\n";
		perlinShader = new PerlinShader;
		cout << "Gabor...\n";
		gaborShader = new GaborShader;
		cout << "Wavelet...\n";
		waveletShader = new WaveletShader;
		cout << "Setting default values...\n";
		shader = perlinShader;
		shader->bind();
		setShaderValues ();
		cout << "Current shader is Perlin\n";
	} catch (ShaderException e) {
		cerr << e.getMessage () << endl;
		exit (EXIT_FAILURE);
	}
}

void clear () {
	delete perlinShader;
	delete gaborShader;
	delete waveletShader;
	glDeleteLists (glID, 1);
}

void reshape(int w, int h) {
	camera.resize (w, h);
}

void display () {
	glLoadIdentity ();
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	camera.apply ();
	if (mode == Solid)
		drawSolidModel ();
	else if (mode == Phong)
		drawPhongModel ();
	glFlush ();
	glutSwapBuffers ();
	setShaderValues();
}

void idle () {
	static float lastTime = glutGet ((GLenum)GLUT_ELAPSED_TIME);
	static unsigned int counter = 0;
	counter++;
	float currentTime = glutGet ((GLenum)GLUT_ELAPSED_TIME);
	float elapsed = currentTime - lastTime;
	if (elapsed >= 1000.0f) {
		FPS = counter;
		counter = 0;
		static char FPSstr [128];
		unsigned int numOfTriangles = mesh.getTriangles ().size ();
		if (mode == Solid)
			sprintf (FPSstr, "gMini: %d tri. - solid shading - %d FPS.",
					numOfTriangles, FPS);
		else if (mode == Phong)
			sprintf (FPSstr, "gMini: %d tri. - Phong shading - %d FPS.",
					numOfTriangles, FPS);
		glutSetWindowTitle (FPSstr);
		lastTime = currentTime;

	}
	perlinTime+=counter?elapsed/(float)(counter*10000):0;
	glutPostRedisplay ();
}

void printUsage () {
	cerr << endl
		<< "--------------------------------------" << endl
		<< "gMini" << endl 
		<< "--------------------------------------" << endl
		<< "Author : Tamy Boubekeur (http://www.telecom-paristech.fr/~boubek)" << endl
		<< "--------------------------------------" << endl 
		<< "USAGE: ./Main <file>.off" << endl
		<< "--------------------------------------" << endl 
		<< "Keyboard commands" << endl 
		<< "--------------------------------------" << endl 
		<< " ?: Print help" << endl 
		<< endl
		<< " G: Switch to Gabor noise" << endl 
		<< " A: (GABOR) increase the a parameter" << endl
		<< " a: (GABOR) decrease the a parameter" << endl
		<< " i: (GABOR) isometric noise <--> anisometric noise" << endl
		<< " w: (GABOR) increase the omega0 (only useful with anisotropic noise)" << endl
		<< endl
		<< " P: Switch to Perlin noise" << endl 
		<< " O: (PERLIN) increase the number of octaves" << endl
		<< " o: (PERLIN) decrease the number of octaves" << endl
		<< " E: (PERLIN) increase the persistence" << endl
		<< " e: (PERLIN) decrease the persistence" << endl
		<< " F: (PERLIN) increase the frequence" << endl
		<< " f: (PERLIN) decrease the frequence" << endl
		<<endl
		<< " P: Switch to Wavelet noise" << endl 
		<< " B: (WAVELET) increase the number of bands" << endl
		<< " b: (WAVELET) decrease the number of bands" << endl
		<< " R: (WAVELET) increase the first band" << endl
		<< " r: (WAVELET) decrease the first band" << endl
		/*<< " T: (WAVELET) increase the tile size" << endl
		<< " t: (WAVELET) decrease the tile size" << endl*/
		<< " S: (WAVELET) increase s" << endl
		<< " s: (WAVELET) decrease s" << endl
		<< " p: (WAVELET) enable/disable noise projection" << endl
		<<endl
		<< " D: (ALL) increase diffuse ref" << endl
		<< " d: (ALL) decrease diffuse ref" << endl
		<< " C: (ALL) increase spec ref" << endl
		<< " c: (ALL) decrease spec ref" << endl
		<< " N: (ALL) increase shininess" << endl
		<< " n: (ALL) decrease shininess" << endl
		<< " <drag>+<left button>: rotate model" << endl 
		<< " <drag>+<right button>: move model" << endl
		<< " <drag>+<middle button>: zoom" << endl
		<< " q, <esc>: Quit" << endl << endl
		<< "--------------------------------------" << endl;
}

void key (unsigned char keyPressed, int x, int y) {
	switch (keyPressed) {

		// quit
		case 'q':
		case 27:
			clear ();
			exit (0);
			break;

			// Phong
		case 'D':
			diffuseRef = min(2.0f, diffuseRef+0.1f);
			cout << "PHONG: new diffuse ref: " << diffuseRef << endl;
			break;
		case 'd':
			diffuseRef = max(0.0f, diffuseRef-0.1f);
			cout << "PHONG: new diffuse ref: " << diffuseRef << endl;
			break;
		case 'C':
			specRef = min(3.0f, specRef+0.1f);
			cout << "PHONG: new spec ref: " << specRef << endl;
			break;
		case 'c':
			specRef = max(0.0f, specRef-0.1f);
			cout << "PHONG: new spec ref: " << specRef << endl;
			break;
		case 'N':
			shininess = min(30.0f, shininess+1.0f);
			cout << "PHONG: new shininess: " << shininess << endl;
			break;
		case 'n':
			shininess = max(0.0f, shininess-1.0f);
			cout << "PHONG: new shininess: " << shininess << endl;
			break;


			// gabor
		case 'A':
			a = min(0.1, a + 0.01);
			cout << "GABOR: new a: " << a << endl;
			break;
		case 'a':
			a = max(0.001, a - 0.01);
			cout << "GABOR: new a: " << a << endl;
			break;
		case 'w':
			omega += M_PI/10.0;
			cout << "GABOR: new omega: " << omega << endl;
			break;
		case 'i':
			iso = !iso;
			cout << "GABOR: is iso: " << iso << endl;
			break;

			// Noise type
		case 'P':
			shader = perlinShader;
			shader->bind();
			cout << "Applied Perlin noise" << endl;
			break;
		case 'W':
			shader = waveletShader;
			shader->bind();
			cout << "Applied Wavelet noise" << endl;
			break;
		case 'G':
			shader = gaborShader;
			shader->bind();
			cout << "Applied Gabor noise" << endl;
			break;

			// Wavelet
		case 'B':
			nbands = min(5, nbands + 1);
			cout << "WAVELET: number of bands: " << nbands << endl;
			break;
		case 'b':
			nbands = max(1, nbands - 1);
			cout << "WAVELET: number of bands: " << nbands << endl;
			break;
		case 'R':
			firstBand = min(0, firstBand + 1);
			cout << "WAVELET: first bands: " << firstBand << endl;
			break;
		case 'r':
			firstBand = max(-10, firstBand - 1);
			cout << "WAVELET: first bands: " << firstBand << endl;
			break;
		/*case 'T':
			tileSize = min(6, tileSize + 2);
			cout << "WAVELET: tile size: " << tileSize << endl;
			break;
		case 't':
			tileSize = max(2, tileSize - 2);
			cout << "WAVELET: tile size: " << tileSize << endl;
			break;*/
		case 'p':
			noiseProjected = !noiseProjected;
			cout << "WAVELET: is noise projected: " << noiseProjected << endl;
			break;
		case 'S':
			s = min(0.0, s + 1.0);
			cout << "WAVELET: tile size: " << tileSize << endl;
			break;
		case 's':
			s = max(-10.0, s - 1.0);
			cout << "WAVELET: s: " << s << endl;
			break;


			// perlin
		case 'O':
			nbOctave = min(8, nbOctave + 1);
			cout << "PERLIN: number of octaves: " << nbOctave << endl;
			break;
		case 'o':
			nbOctave = max(1, nbOctave - 1);
			cout << "PERLIN: number of octaves: " << nbOctave << endl;
			break;
		case 'E':
			persistence = min(0.9, persistence + 0.05);
			cout << "PERLIN: persistence: " << persistence << endl;
			break;
		case 'e':
			persistence = max(0.1, persistence - 0.05);
			cout << "PERLIN: persistence: " << persistence << endl;
			break;
		case 'F':
			f0 = min(10.0, f0 + 1.0);
			cout << "PERLIN: frequence: " << f0 << endl;
			break;
		case 'f':
			f0 = max(1.0, f0 - 1.0);
			cout << "PERLIN: frequence: " << f0 << endl;
			break;

		case '?':
		default:
			printUsage ();
			break;
	}
	setShaderValues ();
	idle ();
}

void mouse (int button, int state, int x, int y) {
	if (state == GLUT_UP) {
		mouseMovePressed = false;
		mouseRotatePressed = false;
		mouseZoomPressed = false;
	} else {
		if (button == GLUT_LEFT_BUTTON) {
			camera.beginRotate (x, y);
			mouseMovePressed = false;
			mouseRotatePressed = true;
			mouseZoomPressed = false;
		} else if (button == GLUT_RIGHT_BUTTON) {
			lastX = x;
			lastY = y;
			mouseMovePressed = true;
			mouseRotatePressed = false;
			mouseZoomPressed = false;
		} else if (button == GLUT_MIDDLE_BUTTON) {
			if (mouseZoomPressed == false) {
				lastZoom = y;
				mouseMovePressed = false;
				mouseRotatePressed = false;
				mouseZoomPressed = true;
			}
		}
	}
	idle ();
}

void motion (int x, int y) {
	if (mouseRotatePressed == true) 
		camera.rotate (x, y);
	else if (mouseMovePressed == true) {
		camera.move ((x-lastX)/static_cast<float>(SCREENWIDTH),
				(lastY-y)/static_cast<float>(SCREENHEIGHT),
				0.0);
		lastX = x;
		lastY = y;
	}
	else if (mouseZoomPressed == true) {
		camera.zoom (float (y-lastZoom)/SCREENHEIGHT);
		lastZoom = y;
	}
}

void usage () {
	printUsage ();
	exit (EXIT_FAILURE);
}



int main (int argc, char ** argv) {
	glutInit (&argc, argv);
	glutInitDisplayMode (GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
	glutInitWindowSize (SCREENWIDTH, SCREENHEIGHT);
	window = glutCreateWindow ( "gMini");

	if (argc != 2)
		usage ();

	init (string (argv[1]));

	glCullFace (GL_BACK);
	glEnable (GL_CULL_FACE);
	glutIdleFunc (idle);
	glutDisplayFunc (display);
	glutKeyboardFunc (key);
	glutReshapeFunc (reshape);
	glutMotionFunc (motion);
	glutMouseFunc (mouse);

	key ('?', 0, 0);

	glDepthFunc (GL_LESS);
	glEnable (GL_DEPTH_TEST);

	shader->bind ();
	setShaderValues();
	glutMainLoop ();
	return EXIT_SUCCESS;
}

