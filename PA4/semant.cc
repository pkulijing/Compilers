

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "semant.h"
#include "utilities.h"
#include "list.h"

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
static const char* nb_postfix(int i) {
	if(i == 11 || i == 12 || i == 13)
		return "th";
	switch (i % 10) {
	case 1: return "st";
	case 2: return "nd";
	case 3: return "rd";
	default: return "th";
	}
}
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

CompRes ClassTable::check_method_redefinition (Class_ c, method_class *method, List<Entry>* sig){
	Formals fs = method->get_formals();
	Symbol return_type = method->get_return_type();
	if(fs->len() + 1 != list_length(sig)) {
		return COMP_DIFF_LENGTH;
	}
	for(int i = fs->first(); fs->more(i); i = fs->next(i)) {
		Formal f = fs->nth(i);
		Symbol type_decl = f->get_type_decl();
		if(type_decl != sig->hd()) {
			return COMP_ARGU_MISS_MATCH;
		}
		sig = sig->tl();
	}
	if(return_type != sig->hd()) {
		return COMP_RETURN_MISS_MATCH;
	}
	return COMP_OK;
}

ClassTable::ClassTable(Classes classes) :
		semant_errors(0),
		error_stream(cerr),
		classMap (new SymbolTable<Symbol, ClassDecl>()){
	classMap->enterscope();
	install_basic_classes();

	//Add classes to classMap, also detect previously defined classes
	for(int i = classes->first(); classes->more(i); i = classes->next(i)) {
		Class_ c = classes->nth(i);
		if(c->get_name() == SELF_TYPE) {
			semant_error(c) << "SELF_TYPE cannot be used as class name." << std::endl;
		} else if(classMap->lookup(c->get_name())) {
			semant_error(c) << "Class " << c->get_name()->get_string()
					<< " was previously defined." << std::endl;
		} else {
			add_new_class_basic(c);
		}
	}

	//Add children, also detect undefined parent classes or wrong parent classes
	if(!errors()) {
		for(int i = classes->first(); classes->more(i); i = classes->next(i)) {
			Class_ c = classes->nth(i);
			Symbol parent = c->get_parent();
			if (parent == Int || parent == Bool || parent == Str || parent == SELF_TYPE) {
				semant_error(c) << "Class " << c->get_name()->get_string()
						<< " cannot inherit from class " << parent->get_string() << std::endl;
			} else if(classMap->lookup(parent) == NULL) {
				semant_error(c) << "Class " <<  c->get_name()->get_string() << " inherits from class "
						<< parent->get_string() << " that is not defined." << std::endl;
			} else {
				ClassDecl* parentDecl = classMap->lookup(parent);
				parentDecl->children = new List<Entry>(c->get_name(), parentDecl->children);
			}
		}
	}

	//Detect cycle
	if(!errors()) {
		for(int i = classes->first(); classes->more(i); i = classes->next(i)) {
			Symbol c = classes->nth(i)->get_name();
			for(Symbol d = classMap->lookup(c)->parent;
					d != Object;
					d = classMap->lookup(d)->parent) {
				if(d == c) {
					semant_error(classes->nth(i)) << "Class " << c->get_string()
							<< " is involved in an inheritance cycle." << std::endl;
					break;
				}
			}
		}
	}

	//Detect Main and main()
	if(!errors()) {
		ClassDecl *mainDecl = classMap->lookup(Main);
		if(mainDecl == NULL) {
			semant_error() << "Class Main is not defined." << std::endl;
		} else if (mainDecl->methodTable->lookup(main_meth) == NULL) {
			semant_error() << "Method main() is not defined in class Main." << std::endl;
		}
	}

	//Check for redefined attributes and incorrectly redefined methods
	if(!errors()) {
		for(int i = classes->first(); classes->more(i); i = classes->next(i)) {
			Class_ c = classes->nth(i);
		    Features fs = c->get_features();
			ClassDecl* decl = classMap->lookup(c->get_name());

		    for(int j = fs->first(); fs->more(j); j = fs->next(j)) {
		    	Feature f = fs->nth(j);
		    	if(f->get_feature_type() == FEATURE_ATTR) {
		    		attr_class *attr = dynamic_cast<attr_class*>(f);
		    		for(Symbol d = decl->parent; d != No_class; d = classMap->lookup(d)->parent) {
		    			if(classMap->lookup(d)->attrTable->lookup(attr->get_name()) != NULL) {
							semant_error(c) << "Attribute " << attr->get_name()->get_string()
									<< " is an attribute of an inherited class." << std::endl;
						}
					}
		    		Symbol attrType = attr->get_type_decl();
		    		if(classMap->lookup(attrType) == NULL) {
		    			semant_error(c) << "Attribute " << attr->get_name()->get_string()
		    					<< " is of undefined type " << attrType->get_string()
								<< ". " << std::endl;
		    		}
		    	} else if (f->get_feature_type() == FEATURE_METHOD) {
		    		method_class *method = dynamic_cast<method_class*>(f);

		    		//check validity of signiture (undefined types). Note that return type can be SELF_TYPE
    				List<Entry>* sig = classMap->lookup(c->get_name())->methodTable->lookup(method->get_name());
    				while(sig != NULL && sig->tl() != NULL) {
    					if(classMap->lookup(sig->hd()) == NULL) {
    						semant_error(c) << "Method " << method->get_name()->get_string()
    								<< " contains an undefined type " << sig->hd()->get_string()
									<< "." << std::endl;
    					}
    					sig = sig->tl();
    				}
    				if(sig->hd() != SELF_TYPE && classMap->lookup(sig->hd()) == NULL) {
    				    	semant_error(c) << "Method " << method->get_name()->get_string()
    				    			<< " returns to an undefined type " << sig->hd()->get_string()
									<< "." << std::endl;
    				}

		    		for(Symbol d = decl->parent; d != No_class; d = classMap->lookup(d)->parent) {
		    			if(classMap->lookup(d)->methodTable->lookup(method->get_name()) != NULL) {
		    				List<Entry>* sigd = classMap->lookup(d)->methodTable->lookup(method->get_name());
		    				auto comp = check_method_redefinition(c, method, sigd);
		    				if(comp == COMP_DIFF_LENGTH) {
		    					semant_error(c) << "Method " << method->get_name()->get_string()
		    						<< " is redefined with different number of arguments." << std::endl;
		    				} else if (comp == COMP_ARGU_MISS_MATCH) {
		    					semant_error(c) << "Method " << method->get_name()->get_string()
		    						<< " is redefined with different types of arguments." << std::endl;
		    				} else if (comp == COMP_RETURN_MISS_MATCH) {
		    					semant_error(c) << "Method " << method->get_name()->get_string()
		    						<< " is redefined with different return type." << std::endl;
		    				}
		    			}
		    		}
		    	}
		    }
		}
	}
}

Symbol ClassTable::find_symbol_type(Class_ c, Symbol s) {
	ClassDecl *decl = classMap->lookup(c->get_name());
	Symbol res = decl->attrTable->lookup(s);
	if(res != NULL) {
		return res;
	} else if(decl->parent == No_class) { //Error should be reported!
		return No_type;
	} else {
		Class_ parentC = classMap->lookup(decl->parent)->body;
		return find_symbol_type(parentC, s);
	}
}

List<Entry>* ClassTable::find_method_signature(Class_ c, Symbol f) {
	ClassDecl *decl = classMap->lookup(c->get_name());
	List<Entry>* res = decl->methodTable->lookup(f);
	if(res != NULL) {
		return res;
	} else if(decl->parent == No_class) {
		return NULL;
	} else {
		Class_ parentC = classMap->lookup(decl->parent)->body;
		return find_method_signature(parentC, f);
	}
}

ClassDecl* ClassTable::add_new_class_basic(Class_ c) {

    ClassDecl* decl = new ClassDecl();
    decl->body = c;
    decl->parent = c->get_parent();
    decl->children = NULL;

    decl->attrTable = new SymbolTable<Symbol, Entry>();
    decl->methodTable = new SymbolTable<Symbol, List<Entry> >();
    decl->attrTable->enterscope();
    decl->methodTable->enterscope();

    Features fs = c->get_features();

    //Add features, also detect multiply defined features and duplicate names in formal
    for(int i = fs->first(); fs->more(i); i = fs->next(i)) {
    	Feature f = fs->nth(i);
    	if(f->get_feature_type() == FEATURE_ATTR) {
    		attr_class *attr = dynamic_cast<attr_class*>(f);
    		if(decl->attrTable->lookup(attr->get_name()) != NULL) {
    			semant_error(c) << "Attribute " << attr->get_name()->get_string()
    					<< " is multiply defined in class " << c->get_name()->get_string()
						<< std::endl;
    		}
    		if(attr->get_name() == self) {
    			semant_error(c) << "Class " << c->get_name()->get_string()
    					<< " has an attribute named self." << std::endl;
    		}
    		decl->attrTable->addid(attr->get_name(), attr->get_type_decl());
    	} else if (f->get_feature_type() == FEATURE_METHOD) {
    		method_class *method = dynamic_cast<method_class*>(f);
    		if(decl->methodTable->lookup(method->get_name()) != NULL) {
    			semant_error(c) << "Method " << method->get_name()->get_string()
							<< " is multiply defined in class " << c->get_name()->get_string()
							<< std::endl;
    		}
    		List<Entry>* l = new List<Entry>(method->get_return_type());
    		Formals formals = method->get_formals();
    		decl->attrTable->enterscope();
    		for(int j = formals->first(); formals->more(j); j = formals->next(j)) {
    			Formal formal = formals->nth(formals->len() - 1 - j);
    			Symbol name = formal->get_name();
    			Symbol type_decl = formal->get_type_decl();
    			if(decl->attrTable->probe(name) != NULL) {
    				semant_error(c) << "Duplicate names in formals." << std::endl;
    			} else if (name == self){
    				semant_error(c) << "self cannot be used as a formal name." << std::endl;
    			} else {
    				decl->attrTable->addid(name, type_decl);
    			}
    			l = new List<Entry>(type_decl, l);
    		}
    		decl->attrTable->exitscope();
    		decl->methodTable->addid(method->get_name(), l);
    	}
    }
    classMap->addid(c->get_name(), decl);
    return decl;
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

    //
    // The Int class has no methods and only a single attribute, the
    // "val" for the integer. 
    //
    Class_ Int_class =
	class_(Int, 
	       Object,
	       single_Features(attr(val, prim_slot, no_expr())),
	       filename);

    //
    // Bool also has only the "val" slot.
    //
    Class_ Bool_class =
	class_(Bool, Object, single_Features(attr(val, prim_slot, no_expr())),filename);


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

    ClassDecl *objDecl = add_new_class_basic(Object_class);
    add_new_class_basic(IO_class);
    add_new_class_basic(Int_class);
    add_new_class_basic(Bool_class);
    add_new_class_basic(Str_class);
    objDecl->children = new List<Entry>(IO,
    		new List<Entry>(Int,
					new List<Entry>(Bool,
							new List<Entry>(Str))));
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

//Check if s1 is subtype of s2 (s1 <= s2)
bool ClassTable::subtype(Class_ c, Symbol s1, Symbol s2) {
	if(s1 == s2) { //same type, including SELF_TYPE
		return true;
	} else if (s1 == SELF_TYPE) {
		return subtype(c, c->get_name(), s2);
	} else if (s2 == SELF_TYPE || s1 == Object) {
		return false;
	} else {
		return subtype(c, classMap->lookup(s1)->parent, s2);
	}
}

Symbol ClassTable::lub(Class_ c, Symbol s1, Symbol s2) {
	if(subtype(c, s1, s2)) { //including both SELF_TYPE
		return s2;
	} else if (subtype(c, s2, s1)) {
		return s1;
	} else if (s1 == SELF_TYPE) {
		return lub(c, c->get_name(), s2);
	} else if (s2 == SELF_TYPE) {
		return lub(c, s1, c->get_name());
	} else {
		return lub(c, s1, classMap->lookup(s2)->parent);
	}
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

    //Type checking
    for(int i = classes->first(); classes->more(i); i = classes->next(i)) {
    	Class_ c = classes->nth(i);
	    Features fs = c->get_features();

		SymbolTable<Symbol, Entry>* attrTable =
				classtable->get_class_map()->lookup(c->get_name())->attrTable;
		attrTable->enterscope();
		attrTable->addid(self, SELF_TYPE);

	    for(int j = fs->first(); fs->more(j); j = fs->next(j)) {
	    	Feature f = fs->nth(j);
	    	if(f->get_feature_type() == FEATURE_ATTR) {
	    		attr_class *attr = dynamic_cast<attr_class*>(f);
	    		Symbol exprType = classtable->check_type(c, attr->get_init());

	    		if(exprType == No_type || classtable->subtype(c, exprType, attr->get_type_decl()))
	    			continue;
	    		else {
	    			classtable->semant_error(c) << "Attribute " << attr->get_name()
	    					<< " of declared type " << attr->get_type_decl()->get_string()
							<< " in Class " << c->get_name() << " is initialized with "
							<< "expresion of type" << exprType->get_string() << "."
							<< std::endl;
	    		}
	    	} else if (f->get_feature_type() == FEATURE_METHOD) {
	    		method_class *method = dynamic_cast<method_class*>(f);
	    		Formals formals = method->get_formals();
	    		for(int k = formals->first(); formals->more(k); k = formals->next(k)) {
	    			Formal formal = formals->nth(k);
	    			attrTable->addid(formal->get_name(), formal->get_type_decl());
	    		}
	    		Symbol return_type = method->get_return_type();
	    		Symbol actual_type = classtable->check_type(c, method->get_expr());
	    		if(!classtable->subtype(c, actual_type, return_type)) {
	    			classtable->semant_error(c) << "In method " << method->get_name()
	    					<< ", type of expression " << actual_type->get_string()
							<< " does not conform to declared return type "
							<< return_type->get_string() << "." << std::endl;
	    		}
	    	}
	    }
		attrTable->exitscope();
    }
    if(classtable->errors()) {
    	std::cerr << "Compilation halted due to static semantic errors." << endl;
    	exit(1);
    }
}

static bool contain(List<Entry>* types, Symbol type) {
	while(types != NULL) {
		if(types->hd() == type)
			return true;
		types = types->tl();
	}
	return false;
}
Symbol ClassTable::check_type( Class_ c, Expression expr) {
	switch(expr->get_expr_init()) {
	case EXPR_ASSIGN : {
		assign_class* expr_assign = dynamic_cast<assign_class*>(expr);
		Symbol t1 = find_symbol_type(c, expr_assign->get_name());
		Symbol t2 = check_type(c, expr_assign->get_expr());
		if(expr_assign->get_name() == self) {
			semant_error(c) << "Assignment to self is forbidden." << std::endl;
			expr->set_type(Object);
		} else if(t1 == No_type) {
			expr->set_type(Object);
			semant_error(c) << "Identifier " << expr_assign->get_name()->get_string()
					<< " is not defined." << std::endl;
		} else if(subtype(c, t2,t1)) {
			expr->set_type(t2);
		} else {
			expr->set_type(Object);
			semant_error(c) << "Type " << t2->get_string()
					<< " of assigned expression does not conform to declared type "
					<< t1->get_string() << "of identifier " << expr_assign->get_name()->get_string()
					<< "." << std::endl;
		}
		break;
	}
	case EXPR_DISPATCH: {
		dispatch_class* expr_dispatch = dynamic_cast<dispatch_class*>(expr);
		Expression expr1 = expr_dispatch->get_expr();
		Symbol name = expr_dispatch->get_name();
		Expressions actual = expr_dispatch->get_actual();

		Symbol t1 = check_type(c, expr1);
		Symbol t = (t1 == SELF_TYPE) ? c->get_name() : t1;
		Class_ m = classMap->lookup(t)->body;

		List<Entry>* sig = find_method_signature(m, name);
		if(sig == NULL) {
			semant_error(c) << "Function " << name->get_string() << " is not defined for type "
					<< t->get_string() << "." << std::endl;
			expr->set_type(Object);
		} else if(list_length(sig) != actual->len() + 1) {
			semant_error(c) << "Method " << name->get_string()
					<< " called with wrong number of arguments." << std::endl;
			expr->set_type(Object);
		} else {
			for(int i = actual->first(); actual->more(i); i = actual->next(i)) {
				if(!subtype(c, check_type(c, actual->nth(i)), sig->hd())) {
					semant_error(c) << "In the call of method " << name->get_string()
							<< ", type " << check_type(m, actual->nth(i))->get_string()
							<< " of the " << i << nb_postfix(i)
							<< " argument does not conform to declared type "
							<< sig->hd()->get_string() << "."
							<< std::endl;
				}
				sig = sig->tl();
			}
			expr->set_type((sig->hd() == SELF_TYPE) ? t1 : sig->hd());
		}
		break;
	}
	case EXPR_STATIC_DISPATCH: {
		static_dispatch_class* expr_static_dispatch = dynamic_cast<static_dispatch_class*>(expr);
		Expression expr1 = expr_static_dispatch->get_expr();
		Symbol type_name = expr_static_dispatch->get_type_name();
		Symbol name = expr_static_dispatch->get_name();
		Expressions actual = expr_static_dispatch->get_actual();

		Symbol t1 = check_type(c, expr1);
		if(!subtype(c, t1, type_name)) {
			semant_error(c) << "In static dispatch, " << t1->get_string()
					<< " is not a subtype of " << type_name->get_string() << std::endl;
			expr->set_type(Object);
		} else {
			Class_ m = classMap->lookup(type_name)->body;
			List<Entry>* sig = find_method_signature(m, name);
			if(sig == NULL) {
				semant_error(c) << "Function " << name->get_string() << " is not defined for type "
					<< type_name->get_string() << "." << std::endl;
			} else	if(list_length(sig) != actual->len() + 1) {
				semant_error(c) << "Method " << name->get_string()
						<< " called with wrong number of arguments." << std::endl;
			} else {
				for(int i = actual->first(); actual->more(i); i = actual->next(i)) {
					if(!subtype(c, check_type(c, actual->nth(i)), sig->hd())) {
						semant_error(c) << "In the call of method " << name->get_string()
								<< ", type " << check_type(m, actual->nth(i))->get_string()
								<< " of the " << i << nb_postfix(i)
								<< " argument does not conform to declared type "
								<< sig->hd()->get_string() << "."
								<< std::endl;
					}
					sig = sig->tl();
				}
				expr->set_type((sig->hd() == SELF_TYPE) ? t1 : sig->hd());
			}
		}
		break;
	}
	case EXPR_LET: {
		let_class *expr_let = dynamic_cast<let_class*>(expr);
		Symbol identifier = expr_let->get_identifier();
		Symbol type_decl = expr_let->get_type_decl();
		Expression init = expr_let->get_init();
		Expression body = expr_let->get_body();

		Symbol t1 = check_type(c, init);
		SymbolTable<Symbol, Entry>* attrTable = classMap->lookup(c->get_name())->attrTable;
		attrTable->enterscope();

		if(identifier == self) {
			expr->set_type(Object);
			semant_error(c) << "Let binding contains self." << std::endl;
		} else if(t1 == No_type) {
			attrTable->addid(identifier, type_decl);
			Symbol t2 = check_type(c, body);
			expr->set_type(t2);
		} else if(!subtype(c, t1, type_decl)) {
				semant_error(c) << "In let expression, " << identifier->get_string()
						<< " of type " << type_decl->get_string()
						<< " is initialised with expression of type " << t1->get_string()
						<< "." << std::endl;
				expr->set_type(Object);
		} else {
			attrTable->addid(identifier, type_decl);
			Symbol t2 = check_type(c, body);
			expr->set_type(t2);
		}
		attrTable->exitscope();
		break;
	}
	case EXPR_COND: {
		cond_class *expr_cond = dynamic_cast<cond_class*>(expr);
		Symbol tThen = check_type(c, expr_cond->get_then_exp());
		Symbol tElse = check_type(c, expr_cond->get_else_exp());
		if(check_type(c, expr_cond->get_pred()) == Bool) {
			Symbol res = lub(c, tThen, tElse);
			expr->set_type(res);
		} else {
			expr->set_type(Object);
			semant_error(c) << "Predicate used in condition expression is not of type Bool."
					<< std::endl;
		}
		break;
	}
	case EXPR_LOOP: {
		loop_class *expr_loop = dynamic_cast<loop_class*>(expr);
		Symbol t1 = check_type(c, expr_loop->get_pred());
		Symbol t2 = check_type(c, expr_loop->get_body());
		if(t1 != Bool) {
			semant_error(c) << "Predicate of while expression is not of type Bool." << std::endl;
		}
		expr->set_type(Object);
		break;
	}
	case EXPR_BLOCK: {
		block_class *expr_block = dynamic_cast<block_class*>(expr);
		Expressions body = expr_block->get_body();
		Symbol blockType = NULL;
		for(int i = body->first(); body->more(i); i = body->next(i)) {
			Expression expr1 = body->nth(i);
			blockType = check_type(c, expr1);
		}
		expr->set_type(blockType);
		return blockType;
		break;
	}
	case EXPR_TYPCASE: {
		typcase_class *expr_case = dynamic_cast<typcase_class*>(expr);
		Expression expr1 = expr_case->get_expr();
		check_type(c, expr1);
		Cases cases = expr_case->get_cases();
		SymbolTable<Symbol, Entry>* attrTable = classMap->lookup(c->get_name())->attrTable;
		List<Entry>* return_types = NULL;
		List<Entry>* decl_types = NULL;
		for(int i = cases->first(); cases->more(i); i = cases->next(i)) {
			Case case_ = cases->nth(i);
			branch_class* branch = dynamic_cast<branch_class*>(case_);
			if(contain(decl_types, branch->get_type_decl())) {
				semant_error(c) << "Case contains branches with identical type." << std::endl;
				expr->set_type(Object);
				break;
			} else if(branch->get_type_decl() == SELF_TYPE) {
				semant_error(c) << "Identifier in a case branch cannot have type SELF_TYPE." << std::endl;
				expr->set_type(Object);
				break;
			} else if(branch->get_name() == self) {
				semant_error(c) << "Identifier in a case branch cannot have name self." << std::endl;
				expr->set_type(Object);
				break;
			} else {
				attrTable->enterscope();
				attrTable->addid(branch->get_name(), branch->get_type_decl());
				return_types = new List<Entry>(check_type(c, branch->get_expr()), return_types);
				decl_types = new List<Entry>(branch->get_type_decl(), decl_types);
				attrTable->exitscope();
			}
		}
		if(!errors()) {
			Symbol final_type = return_types->hd();
			while(return_types != NULL) {
				final_type = lub(c, final_type, return_types->hd());
				return_types = return_types->tl();
			}
			expr->set_type(final_type);
		}
		break;
	}
	case EXPR_PLUS: {
		plus_class* expr_plus = dynamic_cast<plus_class*>(expr);
		Expression e1 = expr_plus->get_e1();
		Expression e2 = expr_plus->get_e2();
		if(check_type(c, e1) == Int && check_type(c, e2) == Int) {
			expr->set_type(Int);
		} else {
			semant_error(c) << "+ sign can only be used with two Int values." << std::endl;
			expr->set_type(Object);
		}
		break;
	}
	case EXPR_SUB: {
		sub_class* expr_sub = dynamic_cast<sub_class*>(expr);
		Expression e1 = expr_sub->get_e1();
		Expression e2 = expr_sub->get_e2();
		if(check_type(c, e1) == Int && check_type(c, e2) == Int) {
			expr->set_type(Int);
		} else {
			semant_error(c) << "- sign can only be used with two Int values." << std::endl;
			expr->set_type(Object);
		}
		break;
	}
	case EXPR_MUL: {
		mul_class* expr_mul = dynamic_cast<mul_class*>(expr);
		Expression e1 = expr_mul->get_e1();
		Expression e2 = expr_mul->get_e2();
		if(check_type(c, e1) == Int && check_type(c, e2) == Int) {
			expr->set_type(Int);
		} else {
			semant_error(c) << "* sign can only be used with two Int values." << std::endl;
			expr->set_type(Object);
		}
		break;
	}
	case EXPR_DIVIDE: {
		divide_class* expr_divide = dynamic_cast<divide_class*>(expr);
		Expression e1 = expr_divide->get_e1();
		Expression e2 = expr_divide->get_e2();
		if(check_type(c, e1) == Int && check_type(c, e2) == Int) {
			expr->set_type(Int);
		} else {
			semant_error(c) << "/ sign can only be used with two Int values." << std::endl;
			expr->set_type(Object);
		}
		break;
	}
	case EXPR_NEG: {
		neg_class* expr_neg = dynamic_cast<neg_class*>(expr);
		Expression e1 = expr_neg->get_e1();
		if(check_type(c, e1)) {
			expr->set_type(Int);
		} else {
			semant_error(c) << "~ sign can only be used with an Int value." << std::endl;
			expr->set_type(Object);
		}
		break;
	}
	case EXPR_LT: {
		lt_class* expr_lt = dynamic_cast<lt_class*>(expr);
		Expression e1 = expr_lt->get_e1();
		Expression e2 = expr_lt->get_e2();
		if(check_type(c, e1) == Int && check_type(c, e2) == Int) {
			expr->set_type(Bool);
		} else {
			semant_error(c) << "< sign can only be used with two Int values." << std::endl;
			expr->set_type(Object);
		}
		break;
	}
	case EXPR_EQ: {
		eq_class* expr_eq = dynamic_cast<eq_class*>(expr);
		Expression e1 = expr_eq->get_e1();
		Expression e2 = expr_eq->get_e2();
		Symbol t1 = check_type(c, e1);
		Symbol t2 = check_type(c, e2);
		if((t1 == t2) || (t1 != Int && t2 != Int && t1 != Bool && t2 != Bool && t1 != Str && t2 != Str)) {
			expr->set_type(Bool);
		} else {
			expr->set_type(Object);
			semant_error(c) << "Int, Bool, Str values can only be compard against objects of the same type"
					<< std::endl;
		}
		break;
	}
	case EXPR_LEQ: {
		leq_class* expr_leq = dynamic_cast<leq_class*>(expr);
		Expression e1 = expr_leq->get_e1();
		Expression e2 = expr_leq->get_e2();
		if(check_type(c, e1) == Int && check_type(c, e2) == Int) {
			expr->set_type(Bool);
		} else {
			semant_error(c) << "<= sign can only be used with two Int values." << std::endl;
			expr->set_type(Object);
		}
		break;
	}
	case EXPR_OBJECT: {
		object_class* expr_object = dynamic_cast<object_class*>(expr);
		Symbol name = expr_object->get_name();
		Symbol name_type = find_symbol_type(c, name);
		if(name_type == No_type) {
			semant_error(c) << "Identifier " << name->get_string()
				<< " is not defined." << std::endl;
			expr->set_type(Object);
		} else {
			expr->set_type(name_type);
		}
		break;
	}
	case EXPR_INT_CONST:
		expr->set_type(Int);
		break;
	case EXPR_STRING_CONST:
		expr->set_type(Str);
		break;
	case EXPR_BOOL_CONST:
		expr->set_type(Bool);
		break;
	case EXPR_NEW: {
		new__class* expr_new = dynamic_cast<new__class*>(expr);
		Symbol type_name = expr_new->get_type_name();
		if(type_name != SELF_TYPE)
			expr->set_type(type_name);
		else expr->set_type(SELF_TYPE);
		break;
	}
	case EXPR_COMP: {
		comp_class* expr_comp = dynamic_cast<comp_class*>(expr);
		Expression e1 = expr_comp->get_e1();
		if(check_type(c, e1) == Bool) {
			expr->set_type(Bool);
		} else {
			expr->set_type(Object);
			semant_error(c) << "NOT used on an expr not of type Bool." << std::endl;
		}
		break;
	}
	case EXPR_ISVOID: {
		isvoid_class* expr_isvoid = dynamic_cast<isvoid_class*>(expr);
		Expression e1 = expr_isvoid->get_e1();
		check_type(c, e1);
		expr->set_type(Bool);
		break;
	}
	case EXPR_NO_EXPR:
		expr->set_type(No_type);
		break;
	default:
		std::cerr << "Impossible" << std::endl;
		exit(1);
	}
	return expr->get_type();
}


