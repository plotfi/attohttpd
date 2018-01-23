CC = clang++-5.0
OPT = -O0 -g --std=c++11
LIBS = -lpthread
TARGET = attohttpd
MISC = Makefile
INCDIR = src/include
SRCDIR = src

all:
	$(CC) $(OPT) *.cpp $(LIBS) -o $(TARGET)

clean:
	rm -f $(TARGET)

