CXX = g++
CXXFLAGS = -std=c++0x -Wall
PSLIB = -DMODELDIR=\"`pkg-config --variable=modeldir pocketsphinx`\" `pkg-config --cflags --libs pocketsphinx sphinxbase`

OBJECTS = spicerack.o GPIO.o

spicerack: $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(PSLIB)

spicerack.o: spicerack.cpp GPIO.h
	$(CXX) $(CXXFLAGS) -c spicerack.cpp $(PSLIB)
