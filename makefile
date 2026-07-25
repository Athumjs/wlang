TARGET = wl
TARGET-DEBUG = wl-debug

CC = gcc
CFLAGS = -Iinclude -fsanitize=address -g
LDFLAGS = -lm

SRC = $(shell find src -name "*.c")
OBJ = $(patsubst src/%.c, build/obj/%.o, $(SRC))

all: $(TARGET)

$(TARGET): $(OBJ)
	mkdir -p build
	$(CC) $(LDFLAGS) $(CFLAGS) -o build/$(TARGET) $(OBJ)
	$(CC) $(LDFLAGS) $(CFLAGS) -pg -o build/$(TARGET-DEBUG) $(OBJ)

build/obj/%.o: src/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build

debug:
	gprof ./build/$(TARGET-DEBUG) gmon.out > build/logs.txt
	rm gmon.out
