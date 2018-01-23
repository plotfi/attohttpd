DEBUG = -O0 -g
OPT = --std=c++11
LIBS = -lpthread
TARGET = attohttpd
MISC = Makefile
INCDIR = src/include
SRCDIR = src

all:
	$(CXX) $(OPT) *.cpp $(LIBS) -o $(TARGET)

debug:
	$(CXX) $(OPT) $(DEBUG) *.cpp $(LIBS) -o $(TARGET)

clean:
	rm -f $(TARGET)

