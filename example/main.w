import "lib.w";

// comment line;

/*
   comment block;
*/

enum CalcKind {
  Add,
  Sub
};

struct Calc {
  kind: CalcKind;
  x: int;
  y: int;

  set add(): int {
    return this.x + this.y;
  }

  set sub(): int {
    return this.x - this.y;
  }
};

set main(): int {
  var calc: Calc = {
    kind: CalcKind.Add,
    x: 10,
    y: 20
  };

  if (calc.kind == CalcKind.Add) return calc.add();
  else return calc.sub();
  return func();
}
