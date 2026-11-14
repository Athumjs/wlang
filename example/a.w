extern set printf(format: char*, c1: int);

set main() {
  var x = 10;
  var y = &x;
  printf("number: %d\n", *y);
  return 0;
}
