TARGET = wl

CC = gcc
CFLAGS = -Iinclude -fsanitize=address -g
LDFLAGS = -lm

SRC = $(shell find src -name "*.c")
OBJ = $(patsubst src/%.c, build/obj/%.o, $(SRC))

all: $(TARGET)

$(TARGET): $(OBJ)
	mkdir -p build
	$(CC) $(LDFLAGS) $(CFLAGS) -o build/$(TARGET) $(OBJ)

build/obj/%.o: src/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build
