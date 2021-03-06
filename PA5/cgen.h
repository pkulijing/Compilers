#include <assert.h>
#include <stdio.h>
#include "emit.h"
#include "cool-tree.h"
#include "symtab.h"
#include <map>
#include <vector>
#include <utility>

enum Basicness     {Basic, NotBasic};
#define TRUE 1
#define FALSE 0

class CgenClassTable;
typedef CgenClassTable *CgenClassTableP;

class CgenNode;
typedef CgenNode *CgenNodeP;

class CgenClassTable : public SymbolTable<Symbol,CgenNode> {
private:
   List<CgenNode> *nds;
   ostream& str;
   int stringclasstag;
   int intclasstag;
   int boolclasstag;

///////////////////////////////////////////////////////////////////////////////////////////
   SymbolTable<Symbol, int>* frame_env; //enviroment in the current frame. Only modified in let and dispatch.
   int max_tag;							//convention: tag starts at 0. Basic classes (Object, IO, Int, Bool, String)
   	   	   	   	   	   	   	   	   	   	//use 0-4. Tag of non-basic classes starts at 5 and increments by 1 for
   	   	   	   	   	   	   	   	   	   	// each new class;
// The following methods emit code for
// constants and global declarations.

   void code_global_data();
   void code_global_text();
   void code_bools(int);
   void code_select_gc();
   void code_constants();

// The following creates an inheritance graph from
// a list of classes.  The graph is implemented as
// a tree of `CgenNode', and class names are placed
// in the base class symbol table.

   void install_basic_classes();
   void install_class(CgenNodeP nd);
   void install_classes(Classes cs);
   void build_inheritance_tree();
   void set_relations(CgenNodeP nd);
////////////////////////////////////////////////////////////////////////
   void code_class_nameTab();
   void code_class_nameTab(List<CgenNode> *);
   void code_class_objTab();
   void code_class_objTab(List<CgenNode> *);
   void code_dispTabs();
   void code_protObjs();
   void code_initializers();
   void code_class_methods();

public:
   CgenClassTable(Classes, ostream& str);
   void code();
   CgenNodeP root();
////////////////////////////////////////////////////////////////////////
   SymbolTable<Symbol, int>* get_frame_env() { return frame_env; }
   List<CgenNode>* get_nds() { return nds; }
};


class CgenNode : public class__class {
private: 
   CgenNodeP parentnd;                        // Parent of class
   List<CgenNode> *children;                  // Children of class
   Basicness basic_status;                    // `Basic' if class is basic
                                              // `NotBasic' otherwise
   ////////////////////////////////////////////////////////////////////////
   CgenClassTableP class_table;
   int tag;									  // tag of the class
   SymbolTable<Symbol, int>* attr_offset;	  // environment of attributes. map from name to offset.
   SymbolTable<Symbol, int>* method_offset;	  // map from method name to offset.

   std::pair<std::vector<Symbol>, std::map<Symbol,Symbol> > find_first_appearance_of_methods();
public:
   CgenNode(Class_ c,
            Basicness bstatus,
            CgenClassTableP class_table,
			int t);

   void add_child(CgenNodeP child);
   List<CgenNode> *get_children() { return children; }
   void set_parentnd(CgenNodeP p);
   CgenNodeP get_parentnd() { return parentnd; }
   int basic() { return (basic_status == Basic); }

   ////////////////////////////////////////////////////////////////////////
   int get_tag() { return tag; }
   CgenClassTableP get_class_table() { return class_table; }

   //get the offset of a method inside an object of this type (or its subtype). Useful for dispatch.
   int get_method_offset(Symbol type, Symbol name);
   //get the offset of an attr. Since attrs are invisible outside of its own object, no need to provide type.
   int get_attr_offset(Symbol name);

   //get the closest ancestor of type in options. return NULL if no ancestor of type is contained in the list.
   int get_closest_ancestor(Symbol type, Cases cases);

   //current_node is needed in the two methods because they will be called recursively, while we want to modify
   //attr_offset and method_offset in the recursive calls.
   //return: offset of the next attr
   int code_attrs(ostream& s, CgenNode* current_node);

   void code_dispTab(ostream& s);

   //size of an object of this class.
   int size_in_word();

   void code_initializer(ostream& s);
   void code_methods(ostream& s);
};

class BoolConst 
{
 private: 
  int val;
 public:
  BoolConst(int);
  void code_def(ostream&, int boolclasstag);
  void code_ref(ostream&) const;
};

