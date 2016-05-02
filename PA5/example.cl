Class A {
	a : Int <- 1;
	c : Int <- 2;
	b : Int <- c + 1;
	out_b() : Object { (new IO).out_int(b) };
};
Class Main {
	main(): Object{
		(new A).out_b()
	};
};
