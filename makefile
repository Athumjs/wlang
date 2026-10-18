TARGET = wl
TARGET-DEBUG = wl-debug

CC = gcc
CFLAGS = -Iinclude -fsanitize=address -g
LDFLAGS = -lm

SRC = $(shell find src -name "*.c")
OBJ = $(patsubst src/%.c, mbuild/obj/%.o, $(SRC))

all: $(TARGET)

$(TARGET): $(OBJ)
	mkdir -p build
	$(CC) $(CFLAGS) -o mbuild/$(TARGET) $(OBJ) $(LDFLAGS)
	$(CC) $(CFLAGS) -pg -o mbuild/$(TARGET-DEBUG) $(OBJ) $(LDFLAGS)

build/obj/%.o: src/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf mbuild

debug:
	gprof ./mbuild/$(TARGET-DEBUG) gmon.out > mbuild/logs.txt
	rm gmon.out
