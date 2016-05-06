class Main inherits IO {
	main():Object {{
		out_int(if (true = false) then 1 else 0 fi);
		out_int(if (true = true) then 0 else 1 fi);
		out_int(if ("hello" = "hello".copy()) then 0 else 1 fi);
		out_int(let a:String in if (a = "") then 0 else 1 fi);
		out_int(if 5 = 6 then 1 else 0 fi);
	}};

};
