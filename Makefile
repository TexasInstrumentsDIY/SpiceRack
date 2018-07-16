CXX = g++
CXXFLAGS = -std=c++0x -Wall

OBJECTS = spicerack.o GPIO.o

main: $(OBJECTS)
        $(CXX) $(CXXFLAGS) -o $@ $^

$(OBJECTS): GPIO.h