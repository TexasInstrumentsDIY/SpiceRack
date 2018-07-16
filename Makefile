CXX = g++
CXXFLAGS = -std=c++0x -Wall

OBJECTS = spicerack.o GPIO.o

spicerack: $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ -DMODELDIR=\"`pkg-config --variable=modeldir pocketsphinx`\" `pkg-config --cflags --libs pocketsphinx sphinxbase`

$(OBJECTS): GPIO.h
