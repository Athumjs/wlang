set main(old: int): int {
  var isMajor = false;

  if (old > 30) {
    isMajor = true;
  } else if (old > 18) {
    isMajor = true;
  } else {
    isMajor = false;
  }

  var x = isMajor;
  return 0;
}
