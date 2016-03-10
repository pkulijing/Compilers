#ifndef SEMANT_H_
#define SEMANT_H_

#include <assert.h>
#include <iostream>  
#include "cool-tree.h"
#include "stringtab.h"
#include "symtab.h"
#include "list.h"

#define TRUE 1
#define FALSE 0

enum CompRes {
	COMP_DIFF_LENGTH,
	COMP_ARGU_MISS_MATCH,
	COMP_RETURN_MISS_MATCH,
	COMP_OK
};

class ClassTable;
typedef ClassTable *ClassTableP;

// This is a structure that may be used to contain the semantic
// information such as the inheritance graph.  You may use it or not as
// you like: it is only here to provide a container for the supplied
// methods.

struct ClassDecl {
	Class_ body;
	Symbol parent;
	List<Entry>* children;
	SymbolTable<Symbol, Entry>* attrTable;
	SymbolTable<Symbol, List<Entry> >* methodTable;
};

class ClassTable {
private:
  int semant_errors;
  std::ostream& error_stream;
  SymbolTable<Symbol, ClassDecl>* classMap;

  void install_basic_classes();

public:
  ClassTable(Classes);
  int errors() { return semant_errors; }

  std::ostream& semant_error();
  std::ostream& semant_error(Class_ c);
  std::ostream& semant_error(Symbol filename, tree_node *t);

  ClassDecl* add_new_class_basic(Class_ c);

  Symbol find_symbol_type(Class_ c, Symbol s) ;
  List<Entry>* find_method_signature(Class_ c, Symbol f);

  Symbol check_type(Class_, Expression);
  CompRes check_method_redefinition (Class_ , method_class*, List<Entry>*);
  SymbolTable<Symbol, ClassDecl>* get_class_map() { return classMap; }


  bool subtype(Class_, Symbol, Symbol);
  Symbol lub(Class_, Symbol, Symbol);
};


#endif

