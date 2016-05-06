Class A {
	f() : Int { 1 };
};
class B inherits A {
	f() : Int { 2 };
};
Class Main inherits IO {
	b : B <- new B;
	main(): Object { {
		out_int(b@A.f());
		out_int(b.f());
	}};
};
