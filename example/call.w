set main(): int {
  var x = sum(10, 10)();
  return 0;
}

set sum(x: int, y: int): () => void {
  return func;
}

set func(): void {
  return;
}
