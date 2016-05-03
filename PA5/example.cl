Class A {
	outa(a : Int, b : Int, c : Int) : Int { b };
};
Class Main {
	main(): Object{
		(new A).outa(1, 2, 3)
	};
};
