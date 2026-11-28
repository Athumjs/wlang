set main(): int {
  var x = sum(10, 10)();
  return x;
}

set sum(x: int, y: int): () => int {
  return func;
}

set func(): int {
  return 10;
}
