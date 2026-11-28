extern set printf(format: char*, c1: int);

struct Test {
  num: int;
};

set main() {
  struct Tests {
    tests: Test[2];
  };
  var num = 2;
  var x: Tests = {
    tests: [{ num }, { num: 20 }]
  };

  for (var i = 0; i < 2; i++)
    printf("%d\n", x.tests[i].num);

  return 0;
}
