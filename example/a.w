extern set printf(format: char*, c1: char*);
extern set malloc(size: ulong): char*;
extern set strlen(str: char*): ulong;
extern set strcpy(dst: char*, src: char*);
extern set free(mem: char*);

set main() {
  var x = malloc(strlen("Hello World") + (1 as ulong));
  strcpy(x, "Hello World");
  printf("%s\n", x);
  free(x);
  return 0;
}
