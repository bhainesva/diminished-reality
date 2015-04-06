SOURCES = videoFile.cpp pm_minimal.cpp
TARGET = diminish

CFLAGS = `pkg-config --cflags opencv`
LIBS = `pkg-config --libs opencv`

all: $(SOURCES)
	g++ -o $(TARGET) $< $(CFLAGS) $(LIBS)

patchmatch: PatchMatch/pm_minimal_fixed.cpp
	g++ -o pmm $<
