Class A {
	f() : Int { 1 };
	g(a : Int, b : Int, c : Int) : Int { b };
};
Class B inherits A {
	f() : Int { 2 };
};
Class C inherits B {
	f() : Int { 3 };
};
Class Main {
	io : IO <- new IO;
	main(): Object{ {
		io.out_int((new A).f());
		io.out_int((new B).f());
		io.out_int((new C).f());		
	} };
};
