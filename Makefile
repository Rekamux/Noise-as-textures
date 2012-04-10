# Toggle the following line/comment under windows
LIBS =  -lglut -lGLU -lGL -lGLEW -lm
#LIBS =  -lglut32 -lGLU32 -lopengl32 -lglew32 -lm

CFLAGS = -Wall -O3 
CXXFLAGS = -Wall -O3 
CPPFLAGS = -I$(INCDIR) -I/include -I.
LDFLAGS = -L/usr/X11R6/lib -L/lib
LDLIBS = $(LIBS)  
CC = g++
CPP = g++

CIBLE = gmini
SRCS =  Camera.cpp Main.cpp Shader.cpp Vertex.cpp Triangle.cpp Mesh.cpp Noise.cpp


OBJS = $(SRCS:.cpp=.o)   

$(CIBLE): $(OBJS)
	g++ $(LDFLAGS) $(OBJS) $(LIBS) -o $(CIBLE)

clean:
	rm -f  *~ $(OBJS) *.swp $(CIBLE)

dep:
	gcc $(CPPFLAGS) -MM $(SRCS)

# Dependencies
Camera.o: Camera.cpp Camera.h Vec3D.h
Mesh.o: Mesh.cpp Mesh.h Vertex.h Vec3D.h Triangle.h Edge.h
Main.o: Main.cpp Shader.h Vec3D.h Vertex.h \
  Triangle.h Mesh.h Edge.h Camera.h Noise.h	
Triangle.o: Triangle.cpp Triangle.h
Vertex.o: Vertex.cpp Vertex.h Vec3D.h
