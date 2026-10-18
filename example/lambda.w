set main() {
  func(:() return 10;);
  return;
}

set func(callback: () => int) {
  return callback();
}
