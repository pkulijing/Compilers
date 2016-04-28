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

Class Main {
	c : C;
	i : Int <- 1 + 2;
	main():C {
	  (new C).init(1,true)
	};
};
