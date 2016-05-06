class A {
};
class B inherits A {
	f() : A { new SELF_TYPE };
};
class C inherits B {
};
class D inherits B {
};
class E inherits C {
};
class Main inherits IO {
	main():Object {
		case (new C).f() of 
		a : A => a;
		b : B => b;
		c : C => c;
		d : D => d;
		e : E => e;
		esac
	};
};
