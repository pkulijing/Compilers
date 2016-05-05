Class Main inherits IO {
	a : Int <- 2;
	main(): Object{
		while (a < 5) loop {
		a <- a + 1;
		out_int(a);
	} pool};
};
