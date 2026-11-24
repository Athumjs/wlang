extern set printf(format: char*, c1: char*, c2: int);

struct People {
  name: char*;
  old: int;

  set print() {
    printf("%s with %d years.\n", this.name, this.old);
    return 0;
  }
};

set main() {
  var x: People = {
    name: "Carlos",
    old: 29
  };

  x.print();
  return 0;
}
