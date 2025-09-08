CC = clang++
CFLAGS = -g -Wall -Wextra -Wcast-qual -Wconversion-null -Wformat-security -Wmissing-declarations -Woverlength-strings -Wpointer-arith -Wundef -Wunused-local-typedefs -Wunused-result -Wvarargs -Wvla -Wwrite-strings -DNOMINMAX -Werror -fno-omit-frame-pointer -std=c++20 -fPIC
PROFILE_FLAGS= -pg
LDFLAGS = -g
OPT = -O3

BIN = bin
SRC = ${wildcard *.cpp}
OBJ = $(patsubst %.cpp, $(BIN)/%.o, $(SRC)) # (from, to, string)
DLIB = $(BIN)/libtmxparser.so
# < is first dep, ^ is all dependencies, @ is left of colon

.PHONY: all clean

all: lib 

lib: $(DLIB)

$(DLIB): $(OBJ)
	mkdir -p $(@D)
	$(CC) -o $@ -shared $^ $(LIBS) $(OPT)

$(BIN)/%.o: %.cpp
	mkdir -p $(@D)
	$(CC) -o $@ -c $< $(CFLAGS) $(INCLUDE) $(OPT)

clean:
	rm -rf $(BIN)
