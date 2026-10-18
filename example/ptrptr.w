set main(): int {
  var x = 10;
  var p = &x;
  var pp = &p;
  **pp = 30;
  var y = **pp;
  return y;
}
