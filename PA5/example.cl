class C {
	a : Int;
	b : Bool;
	c : String;
	init(x : Int, y : Bool) : C {
           {
		a <- x;
		b <- y;
		self;
           }
	};
};

class D inherits C {
	d : IO;
};

class A {
	f(a : Int, b : IO, c : Int, d : Int) : Int { 102 };
	g() : Int { 103};
};

class B {
	a:Int <- 1;
	b: String <- "hehe";
};

Class Main {
	c : C;
	i : Int <- 1 + 2;
	main(): Int{ 
		2 + 2
	};
};
