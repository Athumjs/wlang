struct People {
  name: char*;
  old: int;

  set showOld() {
    return this.old;
  }
};

set main() {
  var x: People = {
    name: "Carlos",
    old: 29
  };

  return x.showOld();
}
