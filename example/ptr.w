set main(): int {
  var x = 10;
  var p = &x;
  *p = 20;
  var y = *p;
  return y;
}
