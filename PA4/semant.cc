

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
		error_stream(cerr),
		classMap (new SymbolTable<Symbol, ClassDecl>()){
	classMap->enterscope();
	install_basic_classes();

	//Add classes to classMap, also detect previously defined classes
	for(int i = classes->first(); classes->more(i); i = classes->next(i)) {
		Class_ c = classes->nth(i);
		if(classMap->lookup(c->get_name())) {
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
			semant_error() << "Class Main does not exist." << std::endl;
		} else if (mainDecl->methodTable->lookup(main_meth) == NULL) {
			semant_error() << "Class Main does not have main method." << std::endl;
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
		    	} else if (f->get_feature_type() == FEATURE_METHOD) {
		    		method_class *method = dynamic_cast<method_class*>(f);
		    		for(Symbol d = decl->parent; d != No_class; d = classMap->lookup(d)->parent) {
		    			if(classMap->lookup(d)->methodTable->lookup(method->get_name()) != NULL) {
		    				semant_error(c) << "Method " << method->get_name()->get_string()
		    						<< " is redefined incorrectly." << std::endl;
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
	} else if(decl->parent == No_class) {
		semant_error(c) << "Identifier " << s->get_string() << " is not defined.";
		return NULL;
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
		semant_error(c) << "Function " << f->get_string() << " is not defined.";
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
    decl->children = new List<Entry>(NULL);

    decl->attrTable = new SymbolTable<Symbol, Entry>();
    decl->methodTable = new SymbolTable<Symbol, List<Entry> >();
    decl->attrTable->enterscope();
    decl->methodTable->enterscope();

    Features fs = c->get_features();

    //Add features, also detect multiply defined features
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
    		for(int j = formals->first(); formals->more(j); j = formals->next(j)) {
    			Formal formal = formals->nth(formals->len() - 1 - j);
    			l = new List<Entry>(formal->get_name(), l);
    		}
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
							new List<Entry>(Str, objDecl->children))));
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


}

Symbol ClassTable::check_type( Class_ c, Expression expr) {
	switch(expr->get_expr_init()) {
	case EXPR_ASSIGN :
		assign_class* expr_assign = dynamic_cast<assign_class*>(expr);
		Symbol t1 = find_symbol_type(c, expr_assign->get_name());
		Symbol t2 = check_type(c, expr_assign->get_expr());
		if(subtype(c, t2,t1)) {
			expr->set_type(t2);
		} else {
			expr->set_type(Object);
			semant_error(c) << "Type " << t2->get_string()
					<< " of assigned expression does not conform to declared type "
					<< t1->get_string() << "of identifier " << expr_assign->get_name()->get_string()
					<< "." << std::endl;
		}
		break;
	case EXPR_DISPATCH:
		dispatch_class* expr_dispatch = dynamic_cast<dispatch_class*>(expr);
		Expression expr = expr_dispatch->get_expr();
		Symbol name = expr_dispatch->get_name();
		Expressions actual = expr_dispatch->get_actual();

		Symbol t1 = check_type(c, expr);
		Symbol t = (t1 == SELF_TYPE) ? c->get_name() : t1;
		Class_ m = classMap->lookup(t)->body;

		List<Entry>* sig = find_method_signature(m, name);
		if(list_length(sig) != actual->len() + 1) {
			semant_error(c) << "Method " << name->get_string()
					<< " called with wrong number of arguments." << std::endl;
		} else {
			for(int i = actual->first(); actual->more(i); i = actual->next(i)) {
				if(!subtype(m, check_type(m, actual->nth(i)), sig->hd())) {
					semant_error(c) << "In method " << name->get_string()
							<< ", type " << check_type(m, actual->nth(i))->get_string()
							<< " of argument " << actual->nth(i)->get_string()
							<< " does not conform to declared type "
							<< std::endl;

				}
			}
		}

		break;
//	EXPR_STATIC_DISPATCH,
//	EXPR_LET,
	case EXPR_COND:
		cond_class *expr_cond = dynamic_cast<cond_class*>(expr);
		Symbol tThen = check_type(c, expr_cond->get_then_exp());
		Symbol tElse = check_type(c, expr_cond->get_else_exp());
		if(check_type(c, expr_cond->get_pred()) == Bool) {
			expr->set_type(lub(c, tThen, tElse));
		} else {
			expr->set_type(Object);
			semant_error(c) << "Predicate used in condition expression is not of type Bool."
					<< std::endl;
		}
		break;
//	EXPR_LOOP,
	case EXPR_BLOCK:
		block_class *expr_block = dynamic_cast<block_class*>(expr);
		Expressions body = expr_block->get_body();
		Symbol blockType = NULL;
		for(int i = body->first(); body->more(i); i = body->next(i)) {
			Expression expr = body->nth(i);
			blockType = check_type(c, expr);
		}
		expr->set_type(blockType);
		break;
//	EXPR_TYPCASE,
	case EXPR_PLUS:
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
	case EXPR_SUB:
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
	case EXPR_MUL:
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
	case EXPR_DIVIDE:
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
	case EXPR_NEG:
		neg_class* expr_neg = dynamic_cast<neg_class*>(expr);
		Expression e1 = expr_neg->get_e1();
		if(check_type(c, e1)) {
			expr->set_type(Int);
		} else {
			semant_error(c) << "~ sign can only be used with an Int value." << std::endl;
			expr->set_type(Object);
		}
		break;
	case EXPR_LT:
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
	case EXPR_EQ:
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
	case EXPR_LEQ:
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
	case EXPR_OBJECT:
		object_class* expr_object = dynamic_cast<object_class*>(expr);
		Symbol name = expr_object->get_name();
		expr->set_type(find_symbol_type(c, name));
		break;
	case EXPR_INT_CONST:
		expr->set_type(Int);
		break;
	case EXPR_STRING_CONST:
		expr->set_type(Str);
		break;
	case EXPR_BOOL_CONST:
		expr->set_type(Bool);
		break;
	case EXPR_NEW:
		new__class* expr_new = dynamic_cast<new__class*>(expr);
		Symbol type_name = expr_new->get_type_name();
		if(type_name != SELF_TYPE)
			expr->set_type(type_name);
		else expr->set_type(SELF_TYPE);
		break;
	case EXPR_COMP:
		comp_class* expr_comp = dynamic_cast<comp_class*>(expr);
		Expression e1 = expr_comp->get_e1();
		if(check_type(c, e1) == Bool) {
			expr->set_type(Bool);
		} else {
			expr->set_type(Object);
			semant_error(c) << "NOT used on an expr not of type Bool." << std::endl;
		}
		break;
	case EXPR_ISVOID:
		isvoid_class* expr_isvoid = dynamic_cast<isvoid_class*>(expr);
		Expression e1 = expr_isvoid->get_e1();
		check_type(c, e1);
		expr->set_type(Bool);
		break;
//	EXPR_NO_EXPR
	}
}


