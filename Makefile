CXX = g++
CXXFLAGS = -std=c++0x -Wall
PSLIB = -DMODELDIR=\"`pkg-config --variable=modeldir pocketsphinx`\" `pkg-config --cflags --libs pocketsphinx sphinxbase`

OBJECTS = spicerack.o GPIO.o motor.o I2CDevice.o

spicerack: $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(PSLIB)

spicerack.o: spicerack.cpp gpio/GPIO.h
	$(CXX) $(CXXFLAGS) -c spicerack.cpp $(PSLIB)

motor.o: motor/motor.cpp gpio/GPIO.h
	$(CXX) $(CXXFLAGS) -c motor/motor.cpp

GPIO.o: gpio/GPIO.cpp
	$(CXX) $(CXXFLAGS) -c gpio/GPIO.cpp

I2CDevice.o: i2c/I2CDevice.cpp
	$(CXX) $(CXXFLAGS) -c i2c/i2CDevice.cpp
