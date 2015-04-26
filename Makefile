SOURCES = videoFile.cpp pm_minimal.cpp
TARGET = diminish

OPTFLAGS = -O6 -s -ffast-math -fomit-frame-pointer -fstrength-reduce -msse2 -funroll-loops
CFLAGS = `pkg-config --cflags opencv`
LIBS = `pkg-config --libs opencv`

all: $(SOURCES)
	g++ $(OPTFLAGS) -o $(TARGET) $< $(CFLAGS) $(LIBS)

patchmatch: PatchMatch/pm_minimal_fixed.cpp
	g++ -o pmm $<
