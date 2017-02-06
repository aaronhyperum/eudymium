# Install
BIN = demo

# Flags
CFLAGS = -std=c99 -pedantic -O2
CXXFLAGS += -std=c++11 -Wall -Wformat

SRC = src/*.cpp
SRC += lib/gl3w/GL/gl3w.c
OBJ = $(SRC:.cpp=.o)

# Mac OS X:
INCLUDE = -I/usr/local/include -Isrc -Ilib/glfw/include -Ilib/gl3w -Ilib/CSerial
SEARCH = -L/tmp/usr/local/lib -L/usr/local/lib -I/tmp/usr/local/include
LIBS = -lserial -lglfw3 -framework OpenGL -lm -framework Cocoa -framework IOKit -framework CoreVideo


$(BIN):
	@mkdir -p bin
	rm -f bin/$(BIN) $(OBJS)
	$(CXX) $(SRC) $(CXXFLAGS) -o bin/$(BIN) $(LIBS) $(INCLUDE) $(SEARCH)
