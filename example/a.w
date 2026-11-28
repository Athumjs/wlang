extern set printf(format: char*, c1: int);

set main() {
  var x = [[10, 20], [30, 40]];

  for (var i = 0; i < 2; i++) {
    for (var j = 0; j < 2; j++)
      printf("%d\n", x[i][j]);
  }

  return 0;
}
