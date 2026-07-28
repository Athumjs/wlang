#include <front/init.h>
#include <utils/args.h>
#include <utils/error.h>

int main(int argc, char **argv) {
  struct Arena arena;
  arena.current = arenaBlock_create(4096);
  struct Args *args = resolveArgs(argc, argv, &arena);
  front_init(&arena, args->input_file);
  arena_destroy(&arena);
  return 0;
}
