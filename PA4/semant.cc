

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "semant.h"
#include "utilities.h"
#include <map>
#include <string>
#include <cstring>

extern int semant_debug;
extern char *curr_filename;

//////////////////////////////////////////////////////////////////////
//
// Symbols
//
// For convenience, a large number of symbols are predefined here.
// These symbols include the primitive type and method names, as well
// as fixed names used by the runtime system.
//
//////////////////////////////////////////////////////////////////////
static Symbol 
    arg,
    arg2,
    Bool,
    concat,
    cool_abort,
    copy,
    Int,
    in_int,
    in_string,
    IO,
    length,
    Main,
    main_meth,
    No_class,
    No_type,
    Object,
    out_int,
    out_string,
    prim_slot,
    self,
    SELF_TYPE,
    Str,
    str_field,
    substr,
    type_name,
    val;
//
// Initializing the predefined symbols.
//
static void initialize_constants(void)
{
    arg         = idtable.add_string("arg");
    arg2        = idtable.add_string("arg2");
    Bool        = idtable.add_string("Bool");
    concat      = idtable.add_string("concat");
    cool_abort  = idtable.add_string("abort");
    copy        = idtable.add_string("copy");
    Int         = idtable.add_string("Int");
    in_int      = idtable.add_string("in_int");
    in_string   = idtable.add_string("in_string");
    IO          = idtable.add_string("IO");
    length      = idtable.add_string("length");
    Main        = idtable.add_string("Main");
    main_meth   = idtable.add_string("main");
    //   _no_class is a symbol that can't be the name of any 
    //   user-defined class.
    No_class    = idtable.add_string("_no_class");
    No_type     = idtable.add_string("_no_type");
    Object      = idtable.add_string("Object");
    out_int     = idtable.add_string("out_int");
    out_string  = idtable.add_string("out_string");
    prim_slot   = idtable.add_string("_prim_slot");
    self        = idtable.add_string("self");
    SELF_TYPE   = idtable.add_string("SELF_TYPE");
    Str         = idtable.add_string("String");
    str_field   = idtable.add_string("_str_field");
    substr      = idtable.add_string("substr");
    type_name   = idtable.add_string("type_name");
    val         = idtable.add_string("_val");
}

ClassTable::ClassTable(Classes classes) :
		semant_errors(0),
		error_stream(cerr) {

	install_basic_classes();

	//Detect previously defined classes
	for(int i = classes->first(); classes->more(i); i = classes->next(i)) {
		Class_ c = classes->nth(i);
		Symbol name = c->get_name();
		if(symbolClassMap.find(name) != symbolClassMap.end()) {
			semant_error(c) << "Class " << c->get_name()->get_string()
					<< " was previously defined." << std::endl;
		} else {
			symbolClassMap[name] = c;
		}
	}

	//Detect undefined parent classes or wrong parent classes
	if(!errors()) {
		for(int i = classes->first(); classes->more(i); i = classes->next(i)) {
			Class_ c = classes->nth(i);
			Symbol parent = c->get_parent();
			if (parent == Int || parent == Bool || parent == Str || parent == SELF_TYPE) {
				semant_error(c) << "Class " << c->get_name()->get_string()
						<< " cannot inherit from class " << parent->get_string() << std::endl;
			} else if(get_class_from_name(parent) == NULL) {
				semant_error(c) << "Class " <<  c->get_name()->get_string() << " inherits from class "
						<< parent->get_string() << " that is not defined." << std::endl;
			} else {
				c->set_parent_class(get_class_from_name(parent));
			}
		}
	}

	//Detect cycle
	if(!errors()) {
		for(int i = classes->first(); classes->more(i); i = classes->next(i)) {
			Class_ c = classes->nth(i);
			for(Class_ d = c->get_parent_class(); d != get_class_from_name(Object); d = d->get_parent_class()) {
				if(d == c) {
					semant_error(c) << "Class " << c->get_name()->get_string()
							<< " is involved in an inheritance cycle." << std::endl;
					break;
				}
			}
		}
	}

	//Detect Main
	if(!errors()) {
		if(get_class_from_name(Main) == NULL) {
			semant_error() << "Class Main does not exist." << std::endl;
		}
	}
}

Class_ ClassTable::get_class_from_name(Symbol s) {
	if(symbolClassMap.find(s) == symbolClassMap.end()) {
		return NULL;
	}
	return symbolClassMap.at(s);
}

void ClassTable::install_basic_classes() {

    // The tree package uses these globals to annotate the classes built below.
   // curr_lineno  = 0;
    Symbol filename = stringtable.add_string("<basic class>");
    
    // The following demonstrates how to create dummy parse trees to
    // refer to basic Cool classes.  There's no need for method
    // bodies -- these are already built into the runtime system.
    
    // IMPORTANT: The results of the following expressions are
    // stored in local variables.  You will want to do something
    // with those variables at the end of this method to make this
    // code meaningful.

    // 
    // The Object class has no parent class. Its methods are
    //        abort() : Object    aborts the program
    //        type_name() : Str   returns a string representation of class name
    //        copy() : SELF_TYPE  returns a copy of the object
    //
    // There is no need for method bodies in the basic classes---these
    // are already built in to the runtime system.

    Class_ Object_class =
	class_(Object, 
	       No_class,
	       append_Features(
			       append_Features(
					       single_Features(method(cool_abort, nil_Formals(), Object, no_expr())),
					       single_Features(method(type_name, nil_Formals(), Str, no_expr()))),
			       single_Features(method(copy, nil_Formals(), SELF_TYPE, no_expr()))),
	       filename);

    symbolClassMap[Object] = Object_class;
    Object_class->set_parent_class(NULL);
    auto methodTable = Object_class->get_method_table();
    methodTable->enterscope();
    methodTable->addid(cool_abort, new std::vector<Symbol>{Object});
    methodTable->addid(type_name, new std::vector<Symbol>{Str});
    methodTable->addid(copy, new std::vector<Symbol>{SELF_TYPE});
    methodTable->exitscope();

    // 
    // The IO class inherits from Object. Its methods are
    //        out_string(Str) : SELF_TYPE       writes a string to the output
    //        out_int(Int) : SELF_TYPE            "    an int    "  "     "
    //        in_string() : Str                 reads a string from the input
    //        in_int() : Int                      "   an int     "  "     "
    //
    Class_ IO_class = 
	class_(IO, 
	       Object,
	       append_Features(
			       append_Features(
					       append_Features(
							       single_Features(method(out_string, single_Formals(formal(arg, Str)),
										      SELF_TYPE, no_expr())),
							       single_Features(method(out_int, single_Formals(formal(arg, Int)),
										      SELF_TYPE, no_expr()))),
					       single_Features(method(in_string, nil_Formals(), Str, no_expr()))),
			       single_Features(method(in_int, nil_Formals(), Int, no_expr()))),
	       filename);  
    symbolClassMap[IO] = IO_class;
    IO_class->set_parent_class(Object_class);
    methodTable = IO_class->get_method_table();
    methodTable->enterscope();
    methodTable->addid(out_string, new std::vector<Symbol>{Str, SELF_TYPE});
    methodTable->addid(out_int, new std::vector<Symbol>{Int, SELF_TYPE});
    methodTable->addid(in_string, new std::vector<Symbol>{Str});
    methodTable->addid(in_int, new std::vector<Symbol>{Int});
    methodTable->exitscope();

    //
    // The Int class has no methods and only a single attribute, the
    // "val" for the integer. 
    //
    Class_ Int_class =
	class_(Int, 
	       Object,
	       single_Features(attr(val, prim_slot, no_expr())),
	       filename);
    symbolClassMap[Int] = Int_class;
    Int_class->set_parent_class(Object_class);

    //
    // Bool also has only the "val" slot.
    //
    Class_ Bool_class =
	class_(Bool, Object, single_Features(attr(val, prim_slot, no_expr())),filename);
    symbolClassMap[Bool] = Bool_class;
    Bool_class->set_parent_class(Object_class);


    //
    // The class Str has a number of slots and operations:
    //       val                                  the length of the string
    //       str_field                            the string itself
    //       length() : Int                       returns length of the string
    //       concat(arg: Str) : Str               performs string concatenation
    //       substr(arg: Int, arg2: Int): Str     substring selection
    //       
    Class_ Str_class =
	class_(Str, 
	       Object,
	       append_Features(
			       append_Features(
					       append_Features(
							       append_Features(
									       single_Features(attr(val, Int, no_expr())),
									       single_Features(attr(str_field, prim_slot, no_expr()))),
							       single_Features(method(length, nil_Formals(), Int, no_expr()))),
					       single_Features(method(concat, 
								      single_Formals(formal(arg, Str)),
								      Str, 
								      no_expr()))),
			       single_Features(method(substr, 
						      append_Formals(single_Formals(formal(arg, Int)), 
								     single_Formals(formal(arg2, Int))),
						      Str, 
						      no_expr()))),
	       filename);
    symbolClassMap[Str] = Str_class;
    Str_class->set_parent_class(Object_class);

    methodTable = Str_class->get_method_table();
    methodTable->enterscope();
    methodTable->addid(length, new std::vector<Symbol>{Int});
    methodTable->addid(concat, new std::vector<Symbol>{Str, Str});
    methodTable->addid(substr, new std::vector<Symbol>{Int, Int, Str});
    methodTable->exitscope();
}

////////////////////////////////////////////////////////////////////
//
// semant_error is an overloaded function for reporting errors
// during semantic analysis.  There are three versions:
//
//    ostream& ClassTable::semant_error()                
//
//    ostream& ClassTable::semant_error(Class_ c)
//       print line number and filename for `c'
//
//    ostream& ClassTable::semant_error(Symbol filename, tree_node *t)  
//       print a line number and filename
//
///////////////////////////////////////////////////////////////////

ostream& ClassTable::semant_error(Class_ c)
{                                                             
    return semant_error(c->get_filename(),c);
}    

ostream& ClassTable::semant_error(Symbol filename, tree_node *t)
{
    error_stream << filename << ":" << t->get_line_number() << ": ";
    return semant_error();
}

ostream& ClassTable::semant_error()                  
{                                                 
    semant_errors++;                            
    return error_stream;
} 

ostream& program_class::semant_error(Class_ c)
{
    return semant_error(c->get_filename(),c);
}

ostream& program_class::semant_error(Symbol filename, tree_node *t)
{
    error_stream << filename << ":" << t->get_line_number() << ": ";
    return semant_error();
}

ostream& program_class::semant_error()
{
    semant_errors++;
    return error_stream;
}



/*   This is the entry point to the semantic checker.

     Your checker should do the following two things:

     1) Check that the program is semantically correct
     2) Decorate the abstract syntax tree with type information
        by setting the `type' field in each Expression node.
        (see `tree.h')

     You are free to first do 1), make sure you catch all semantic
     errors. Part 2) can be done in a second stage, when you want
     to build mycoolc.
 */



void program_class::semant()
{
    initialize_constants();

    ClassTable *classtable = new ClassTable(classes);

    if (classtable->errors()) {
    	std::cerr << "Compilation halted due to static semantic errors." << endl;
    	exit(1);
    }


    //Add names of attributes and methods to symbol tables.
    for(int i = classes->first(); classes->more(i); i = classes->next(i)) {
    	Class_ c = classes->nth(i);
    	auto objectTable = c->get_object_table();
    	auto methodTable = c->get_method_table();
    	objectTable->enterscope();
    	methodTable->enterscope();

    	Features fs = c->get_features();
    	for(int j = fs->first(); fs->more(j); j = fs->next(j)) {
    		Feature f = fs->nth(j);
    		if(f->get_feature_type() == FEATURE_ATTR) {
    			attr_class* attr = dynamic_cast<attr_class*>(f);
    			if(objectTable->probe(attr->get_name())) {
					semant_error(c) << "Attribute " << attr->get_name()->get_string()
							<< " is multiply defined in class " << c->get_name()->get_string()
							<< std::endl;
    			}
    			objectTable->addid(attr->get_name(), new Symbol(attr->get_type_decl()));
    		} else if (f->get_feature_type() == FEATURE_METHOD) {
    			method_class* method = dynamic_cast<method_class*>(f);
    			if(methodTable->probe(method->get_name())) {
					semant_error(c) << "Method " << method->get_name()->get_string()
							<< " is multiply defined in class " << c->get_name()->get_string()
							<< std::endl;
    			}
    			Formals formals = method->get_formals();
    			std::vector<Symbol>* types = new std::vector<Symbol>();
    			for(int k = formals->first(); formals->more(k); k = formals->next(k)){
    			    Formal formal = formals->nth(k);
    			    types->push_back(formal->get_type_decl());
    			}
    			types->push_back(method->get_return_type());
    			methodTable->addid(method->get_name(), types);
    		}
    	}
    }

    //Check for redefined attributes and incorrectly redefined methods
    for(int i = classes->first(); classes->more(i); i = classes->next(i)) {
    	Class_ c = classes->nth(i);
    	auto objectTable = c->get_object_table();
    	auto methodTable = c->get_method_table();

    	Features fs = c->get_features();
    	for(int j = fs->first(); fs->more(j); j = fs->next(j)) {
    		Feature f = fs->nth(j);
    		if(f->get_feature_type() == FEATURE_ATTR) {
    			attr_class* attr = dynamic_cast<attr_class*>(f);
    			for(Class_ d = c->get_parent_class(); d != NULL; d = d->get_parent_class()) {
    				if(d->get_object_table()->lookup(attr->get_name()) != NULL) {
    					semant_error(c) << "Attribute " << attr->get_name()->get_string()
    							<< " is an attribute of an inherited class." << std::endl;
    				}
    			}
    		} else if (f->get_feature_type() == FEATURE_METHOD) {
    			method_class* method = dynamic_cast<method_class*>(f);
    			Formals formals = method->get_formals();
    			std::vector<Symbol>* types = new std::vector<Symbol>();
    			for(int k = formals->first(); formals->more(k); k = formals->next(k)){
    			    Formal formal = formals->nth(k);
    			    types->push_back(formal->get_type_decl());
    			}
    			types->push_back(method->get_return_type());
    			for(Class_ d = c->get_parent_class(); d != NULL; d = d->get_parent_class()) {
    				auto parentTypes = d->get_method_table()->lookup(method->get_name());
    				if(parentTypes != NULL && *parentTypes != *types) {
    					semant_error(c) << "Method " << method->get_name()->get_string()
    							<< " is redefined incorrectly." << std::endl;
    				}
    			}
    		}
    	}
    }

    if(errors()) {
    	std::cerr << "Compilation halted due to static semantic errors." << endl;
        exit(1);
    }

    for(int i = classes->first(); classes->more(i); i = classes->next(i)) {
     	Class_ c = classes->nth(i);
     	auto objectTable = c->get_object_table();
     	auto methodTable = c->get_method_table();

     	Features fs = c->get_features();
     	for(int j = fs->first(); fs->more(j); j = fs->next(j)) {
     		Feature f = fs->nth(j);
     		if(f->get_feature_type() == FEATURE_ATTR) {
    			attr_class* attr = dynamic_cast<attr_class*>(f);
     		}
     	}
    }
}

//Symbol Class__class::type_check(Expression expr) {
//	switch(expr->get_expr_init()) {
//	case EXPR_ASSIGN :
//		assign_class* expr_assign = dynamic_cast<assign_class*>(expr);
//		if(*(objectTable->lookup(expr_assign->get_name())) == type_check(expr_assign->get_expr())) {
//
//		}
//	EXPR_DISPATCH,
//	EXPR_STATIC_DISPATCH,
//	EXPR_LET,
//	EXPR_COND,
//	EXPR_LOOP,
//	EXPR_BLOCK,
//	EXPR_TYPCASE,
//	EXPR_PLUS,
//	EXPR_SUB,
//	EXPR_MUL,
//	EXPR_DIVIDE,
//	EXPR_NEG,
//	EXPR_LT,
//	EXPR_EQ,
//	EXPR_LEQ,
//	EXPR_OBJECT,
//	EXPR_INT_CONST,
//	EXPR_STRING_CONST,
//	EXPR_BOOL_CONST,
//	EXPR_NEW,
//	EXPR_COMP,
//	EXPR_ISVOID,
//	EXPR_NO_EXPR
//}


