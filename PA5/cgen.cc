
//**************************************************************
//
// Code generator SKELETON
//
// Read the comments carefully. Make sure to
//    initialize the base class tags in
//       `CgenClassTable::CgenClassTable'
//
//    Add the label for the dispatch tables to
//       `IntEntry::code_def'
//       `StringEntry::code_def'
//       `BoolConst::code_def'
//
//    Add code to emit everyting else that is needed
//       in `CgenClassTable::code'
//
//
// The files as provided will produce code to begin the code
// segments, declare globals, and emit constants.  You must
// fill in the rest.
//
//**************************************************************

#include "cgen.h"
#include "cgen_gc.h"
#include <cassert>
#include <sstream>

extern void emit_string_constant(ostream& str, char *s);
extern int cgen_debug;

const int MY_OBJECT_TAG = 0;
const int MY_IO_TAG = 1;
const int MY_INT_TAG = 2;
const int MY_BOOL_TAG = 3;
const int MY_STRING_TAG = 4;
const int NO_ANCESTOR = -1;

int Expression_class::i_label = 0;
int let_class::let_layer = 0;
int typcase_class::case_layer = 0;
//
// Three symbols from the semantic analyzer (semant.cc) are used.
// If e : No_type, then no code is generated for e.
// Special code is generated for new SELF_TYPE.
// The name "self" also generates code different from other references.
//
//////////////////////////////////////////////////////////////////////
//
// Symbols
//
// For convenience, a large number of symbols are predefined here.
// These symbols include the primitive type and method names, as well
// as fixed names used by the runtime system.
//
//////////////////////////////////////////////////////////////////////
Symbol 
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

static char *gc_init_names[] =
  { "_NoGC_Init", "_GenGC_Init", "_ScnGC_Init" };
static char *gc_collect_names[] =
  { "_NoGC_Collect", "_GenGC_Collect", "_ScnGC_Collect" };


//  BoolConst is a class that implements code generation for operations
//  on the two booleans, which are given global names here.
BoolConst falsebool(FALSE);
BoolConst truebool(TRUE);

//*********************************************************
//
// Define method for code generation
//
// This is the method called by the compiler driver
// `cgtest.cc'. cgen takes an `ostream' to which the assembly will be
// emmitted, and it passes this and the class list of the
// code generator tree to the constructor for `CgenClassTable'.
// That constructor performs all of the work of the code
// generator.
//
//*********************************************************

void program_class::cgen(ostream &os) 
{
  // spim wants comments to start with '#'
  os << "# start of generated code\n";

  initialize_constants();
  CgenClassTable *codegen_classtable = new CgenClassTable(classes,os);

  os << "\n# end of generated code\n";
}


//////////////////////////////////////////////////////////////////////////////
//
//  emit_* procedures
//
//  emit_X  writes code for operation "X" to the output stream.
//  There is an emit_X for each opcode X, as well as emit_ functions
//  for generating names according to the naming conventions (see emit.h)
//  and calls to support functions defined in the trap handler.
//
//  Register names and addresses are passed as strings.  See `emit.h'
//  for symbolic names you can use to refer to the strings.
//
//////////////////////////////////////////////////////////////////////////////

static void emit_load(char *dest_reg, int offset, char *source_reg, ostream& s)
{
  s << LW << dest_reg << " " << offset * WORD_SIZE << "(" << source_reg << ")" 
    << endl;
}

static void emit_store(char *source_reg, int offset, char *dest_reg, ostream& s)
{
  s << SW << source_reg << " " << offset * WORD_SIZE << "(" << dest_reg << ")"
      << endl;
}

static void emit_load_imm(char *dest_reg, int val, ostream& s)
{ s << LI << dest_reg << " " << val << endl; }

static void emit_load_address(char *dest_reg, char *address, ostream& s)
{ s << LA << dest_reg << " " << address << endl; }

static void emit_partial_load_address(char *dest_reg, ostream& s)
{ s << LA << dest_reg << " "; }

static void emit_load_bool(char *dest, const BoolConst& b, ostream& s)
{
  emit_partial_load_address(dest,s);
  b.code_ref(s);
  s << endl;
}

static void emit_load_string(char *dest, StringEntry *str, ostream& s)
{
  emit_partial_load_address(dest,s);
  str->code_ref(s);
  s << endl;
}

static void emit_load_int(char *dest, IntEntry *i, ostream& s)
{
  emit_partial_load_address(dest,s);
  i->code_ref(s);
  s << endl;
}

static void emit_move(char *dest_reg, char *source_reg, ostream& s)
{ s << MOVE << dest_reg << " " << source_reg << endl; }

static void emit_neg(char *dest, char *src1, ostream& s)
{ s << NEG << dest << " " << src1 << endl; }

static void emit_add(char *dest, char *src1, char *src2, ostream& s)
{ s << ADD << dest << " " << src1 << " " << src2 << endl; }

static void emit_addu(char *dest, char *src1, char *src2, ostream& s)
{ s << ADDU << dest << " " << src1 << " " << src2 << endl; }

static void emit_addiu(char *dest, char *src1, int imm, ostream& s)
{ s << ADDIU << dest << " " << src1 << " " << imm << endl; }

static void emit_div(char *dest, char *src1, char *src2, ostream& s)
{ s << DIV << dest << " " << src1 << " " << src2 << endl; }

static void emit_mul(char *dest, char *src1, char *src2, ostream& s)
{ s << MUL << dest << " " << src1 << " " << src2 << endl; }

static void emit_sub(char *dest, char *src1, char *src2, ostream& s)
{ s << SUB << dest << " " << src1 << " " << src2 << endl; }

static void emit_sll(char *dest, char *src1, int num, ostream& s)
{ s << SLL << dest << " " << src1 << " " << num << endl; }

static void emit_jalr(char *dest, ostream& s)
{ s << JALR << "\t" << dest << endl; }

static void emit_jal(char *address,ostream &s)
{ s << JAL << address << endl; }

static void emit_return(ostream& s)
{ s << RET << endl; }

static void emit_gc_assign(ostream& s)
{ s << JAL << "_GenGC_Assign" << endl; }

static void emit_disptable_ref(Symbol sym, ostream& s)
{  s << sym << DISPTAB_SUFFIX; }

static void emit_init_ref(Symbol sym, ostream& s)
{ s << sym << CLASSINIT_SUFFIX; }

static void emit_label_ref(int l, ostream &s)
{ s << "label" << l; }

static void emit_protobj_ref(Symbol sym, ostream& s)
{ s << sym << PROTOBJ_SUFFIX; }

static void emit_method_ref(Symbol classname, Symbol methodname, ostream& s)
{ s << classname << METHOD_SEP << methodname; }

static void emit_label_def(int l, ostream &s)
{
  emit_label_ref(l,s);
  s << ":" << endl;
}

//branch on equal zero
static void emit_beqz(char *source, int label, ostream &s)
{
  s << BEQZ << source << " ";
  emit_label_ref(label,s);
  s << endl;
}

//branch on equal
static void emit_beq(char *src1, char *src2, int label, ostream &s)
{
  s << BEQ << src1 << " " << src2 << " ";
  emit_label_ref(label,s);
  s << endl;
}

static void emit_bne(char *src1, char *src2, int label, ostream &s)
{
  s << BNE << src1 << " " << src2 << " ";
  emit_label_ref(label,s);
  s << endl;
}

static void emit_bleq(char *src1, char *src2, int label, ostream &s)
{
  s << BLEQ << src1 << " " << src2 << " ";
  emit_label_ref(label,s);
  s << endl;
}

static void emit_blt(char *src1, char *src2, int label, ostream &s)
{
  s << BLT << src1 << " " << src2 << " ";
  emit_label_ref(label,s);
  s << endl;
}

static void emit_blti(char *src1, int imm, int label, ostream &s)
{
  s << BLT << src1 << " " << imm << " ";
  emit_label_ref(label,s);
  s << endl;
}

static void emit_bgti(char *src1, int imm, int label, ostream &s)
{
  s << BGT << src1 << " " << imm << " ";
  emit_label_ref(label,s);
  s << endl;
}

static void emit_branch(int l, ostream& s)
{
  s << BRANCH;
  emit_label_ref(l,s);
  s << endl;
}

//
// Push a register on the stack. The stack grows towards smaller addresses.
//
static void emit_push(char *reg, ostream& str)
{
  emit_store(reg,0,SP,str);
  emit_addiu(SP,SP,-4,str);
}

//
// Fetch the integer value in an Int object.
// Emits code to fetch the integer value of the Integer object pointed
// to by register source into the register dest
//
static void emit_fetch_int(char *dest, char *source, ostream& s)
{ emit_load(dest, DEFAULT_OBJFIELDS, source, s); }

//
// Emits code to store the integer value contained in register source
// into the Integer object pointed to by dest.
//
static void emit_store_int(char *source, char *dest, ostream& s)
{ emit_store(source, DEFAULT_OBJFIELDS, dest, s); }


static void emit_test_collector(ostream &s)
{
  emit_push(ACC, s);
  emit_move(ACC, SP, s); // stack end
  emit_move(A1, ZERO, s); // allocate nothing
  s << JAL << gc_collect_names[cgen_Memmgr] << endl;
  emit_addiu(SP,SP,4,s);
  emit_load(ACC,0,SP,s);
}

static void emit_gc_check(char *source, ostream &s)
{
  if (source != (char*)A1) emit_move(A1, source, s);
  s << JAL << "_gc_check" << endl;
}


///////////////////////////////////////////////////////////////////////////////
//
// coding strings, ints, and booleans
//
// Cool has three kinds of constants: strings, ints, and booleans.
// This section defines code generation for each type.
//
// All string constants are listed in the global "stringtable" and have
// type StringEntry.  StringEntry methods are defined both for String
// constant definitions and references.
//
// All integer constants are listed in the global "inttable" and have
// type IntEntry.  IntEntry methods are defined for Int
// constant definitions and references.
//
// Since there are only two Bool values, there is no need for a table.
// The two booleans are represented by instances of the class BoolConst,
// which defines the definition and reference methods for Bools.
//
///////////////////////////////////////////////////////////////////////////////

//
// Strings
//
void StringEntry::code_ref(ostream& s)
{
  s << STRCONST_PREFIX << index;
}

//
// Emit code for a constant String.
// You should fill in the code naming the dispatch table.
//

void StringEntry::code_def(ostream& s, int stringclasstag)
{
  IntEntryP lensym = inttable.add_int(len);

  // Add -1 eye catcher
  s << WORD << "-1" << endl;

  code_ref(s);  s  << LABEL                                             // label
      << WORD << stringclasstag << endl                                 // tag
      << WORD << (DEFAULT_OBJFIELDS + STRING_SLOTS + (len+4)/4) << endl // size
      << WORD << STRINGNAME << DISPTAB_SUFFIX << endl; 					// dispatch table
      s << WORD;  lensym->code_ref(s);  s << endl;            // string length
  emit_string_constant(s,str);                                // ascii string
  s << ALIGN;                                                 // align to word
}

//
// StrTable::code_string
// Generate a string object definition for every string constant in the 
// stringtable.
//
void StrTable::code_string_table(ostream& s, int stringclasstag)
{  
  for (List<StringEntry> *l = tbl; l; l = l->tl())
    l->hd()->code_def(s,stringclasstag);
}

//
// Ints
//
void IntEntry::code_ref(ostream &s)
{
  s << INTCONST_PREFIX << index;
}

//
// Emit code for a constant Integer.
// You should fill in the code naming the dispatch table.
//

void IntEntry::code_def(ostream &s, int intclasstag)
{
  // Add -1 eye catcher
  s << WORD << "-1" << endl;

  code_ref(s);  s << LABEL                                // label
      << WORD << intclasstag << endl                      // class tag
      << WORD << (DEFAULT_OBJFIELDS + INT_SLOTS) << endl  // object size
      << WORD << INTNAME << DISPTAB_SUFFIX << endl;
      s << WORD << str << endl;                           // integer value
}


//
// IntTable::code_string_table
// Generate an Int object definition for every Int constant in the
// inttable.
//
void IntTable::code_string_table(ostream &s, int intclasstag)
{
  for (List<IntEntry> *l = tbl; l; l = l->tl())
    l->hd()->code_def(s,intclasstag);
}


//
// Bools
//
BoolConst::BoolConst(int i) : val(i) { assert(i == 0 || i == 1); }

void BoolConst::code_ref(ostream& s) const
{
  s << BOOLCONST_PREFIX << val;
}
  
//
// Emit code for a constant Bool.
// You should fill in the code naming the dispatch table.
//

void BoolConst::code_def(ostream& s, int boolclasstag)
{
  // Add -1 eye catcher
  s << WORD << "-1" << endl;

  code_ref(s);  s << LABEL                                  // label
      << WORD << boolclasstag << endl                       // class tag
      << WORD << (DEFAULT_OBJFIELDS + BOOL_SLOTS) << endl   // object size
      << WORD << BOOLNAME << DISPTAB_SUFFIX << endl;        // dispatch table
      s << WORD << val << endl;                             // value (0 or 1)
}

//////////////////////////////////////////////////////////////////////////////
//
//  CgenClassTable methods
//
//////////////////////////////////////////////////////////////////////////////

//***************************************************
//
//  Emit code to start the .data segment and to
//  declare the global names.
//
//***************************************************

void CgenClassTable::code_global_data()
{
  Symbol main    = idtable.lookup_string(MAINNAME);
  Symbol string  = idtable.lookup_string(STRINGNAME);
  Symbol integer = idtable.lookup_string(INTNAME);
  Symbol boolc   = idtable.lookup_string(BOOLNAME);

  str << "\t.data\n" << ALIGN;
  //
  // The following global names must be defined first.
  //
  str << GLOBAL << CLASSNAMETAB << endl;
  str << GLOBAL; emit_protobj_ref(main,str);    str << endl;
  str << GLOBAL; emit_protobj_ref(integer,str); str << endl;
  str << GLOBAL; emit_protobj_ref(string,str);  str << endl;
  str << GLOBAL; falsebool.code_ref(str);  str << endl;
  str << GLOBAL; truebool.code_ref(str);   str << endl;
  str << GLOBAL << INTTAG << endl;
  str << GLOBAL << BOOLTAG << endl;
  str << GLOBAL << STRINGTAG << endl;

  //
  // We also need to know the tag of the Int, String, and Bool classes
  // during code generation.
  //
  str << INTTAG << LABEL
      << WORD << intclasstag << endl;
  str << BOOLTAG << LABEL 
      << WORD << boolclasstag << endl;
  str << STRINGTAG << LABEL 
      << WORD << stringclasstag << endl;    
}


//***************************************************
//
//  Emit code to start the .text segment and to
//  declare the global names.
//
//***************************************************

void CgenClassTable::code_global_text()
{
  str << GLOBAL << HEAP_START << endl
      << HEAP_START << LABEL 
      << WORD << 0 << endl
      << "\t.text" << endl
      << GLOBAL;
  emit_init_ref(idtable.add_string("Main"), str);
  str << endl << GLOBAL;
  emit_init_ref(idtable.add_string("Int"),str);
  str << endl << GLOBAL;
  emit_init_ref(idtable.add_string("String"),str);
  str << endl << GLOBAL;
  emit_init_ref(idtable.add_string("Bool"),str);
  str << endl << GLOBAL;
  emit_method_ref(idtable.add_string("Main"), idtable.add_string("main"), str);
  str << endl;
}

void CgenClassTable::code_bools(int boolclasstag)
{
  falsebool.code_def(str,boolclasstag);
  truebool.code_def(str,boolclasstag);
}

void CgenClassTable::code_select_gc()
{
  //
  // Generate GC choice constants (pointers to GC functions)
  //
  str << GLOBAL << "_MemMgr_INITIALIZER" << endl;
  str << "_MemMgr_INITIALIZER:" << endl;
  str << WORD << gc_init_names[cgen_Memmgr] << endl;
  str << GLOBAL << "_MemMgr_COLLECTOR" << endl;
  str << "_MemMgr_COLLECTOR:" << endl;
  str << WORD << gc_collect_names[cgen_Memmgr] << endl;
  str << GLOBAL << "_MemMgr_TEST" << endl;
  str << "_MemMgr_TEST:" << endl;
  str << WORD << (cgen_Memmgr_Test == GC_TEST) << endl;
}


//********************************************************
//
// Emit code to reserve space for and initialize all of
// the constants.  Class names should have been added to
// the string table (in the supplied code, is is done
// during the construction of the inheritance graph), and
// code for emitting string constants as a side effect adds
// the string's length to the integer table.  The constants
// are emmitted by running through the stringtable and inttable
// and producing code for each entry.
//
//********************************************************

void CgenClassTable::code_constants()
{
  //
  // Add constants that are required by the code generator.
  //
  stringtable.add_string("");
  inttable.add_string("0");

  stringtable.code_string_table(str,stringclasstag);
  inttable.code_string_table(str,intclasstag);
  code_bools(boolclasstag);
}


CgenClassTable::CgenClassTable(Classes classes, ostream& s) :
		nds(NULL),
		str(s),
		stringclasstag(MY_STRING_TAG),
		intclasstag(MY_INT_TAG),
		boolclasstag(MY_BOOL_TAG),
		frame_env(new SymbolTable<Symbol,int>()),
		max_tag(0)
{
	enterscope();
	frame_env->enterscope();
	if (cgen_debug) cout << "Building CgenClassTable" << endl;
	install_basic_classes();
	install_classes(classes);
	build_inheritance_tree();

	code();
	frame_env->exitscope();
	exitscope();
}

void CgenClassTable::install_basic_classes()
{

// The tree package uses these globals to annotate the classes built below.
  //curr_lineno  = 0;
  Symbol filename = stringtable.add_string("<basic class>");

//
// A few special class names are installed in the lookup table but not
// the class list.  Thus, these classes exist, but are not part of the
// inheritance hierarchy.
// No_class serves as the parent of Object and the other special classes.
// SELF_TYPE is the self class; it cannot be redefined or inherited.
// prim_slot is a class known to the code generator.
//
  addid(No_class,
	new CgenNode(class_(No_class,No_class,nil_Features(),filename),
			    Basic,this, -1));
  addid(SELF_TYPE,
	new CgenNode(class_(SELF_TYPE,No_class,nil_Features(),filename),
			    Basic,this, -1));
  addid(prim_slot,
	new CgenNode(class_(prim_slot,No_class,nil_Features(),filename),
			    Basic,this, -1));

// 
// The Object class has no parent class. Its methods are
//        cool_abort() : Object    aborts the program
//        type_name() : Str        returns a string representation of class name
//        copy() : SELF_TYPE       returns a copy of the object
//
// There is no need for method bodies in the basic classes---these
// are already built in to the runtime system.
//
  install_class(
   new CgenNode(
    class_(Object, 
	   No_class,
	   append_Features(
           append_Features(
           single_Features(method(cool_abort, nil_Formals(), Object, no_expr())),
           single_Features(method(type_name, nil_Formals(), Str, no_expr()))),
           single_Features(method(copy, nil_Formals(), SELF_TYPE, no_expr()))),
	   filename),
    Basic,this, MY_OBJECT_TAG));

// 
// The IO class inherits from Object. Its methods are
//        out_string(Str) : SELF_TYPE          writes a string to the output
//        out_int(Int) : SELF_TYPE               "    an int    "  "     "
//        in_string() : Str                    reads a string from the input
//        in_int() : Int                         "   an int     "  "     "
//
   install_class(
    new CgenNode(
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
	   filename),	    
    Basic,this, MY_IO_TAG));
//
// The Int class has no methods and only a single attribute, the
// "val" for the integer. 
//
   install_class(
    new CgenNode(
     class_(Int, 
	    Object,
            single_Features(attr(val, prim_slot, no_expr())),
	    filename),
     Basic,this, MY_INT_TAG));

//
// Bool also has only the "val" slot.
//
    install_class(
     new CgenNode(
      class_(Bool, Object, single_Features(attr(val, prim_slot, no_expr())),filename),
      Basic,this, MY_BOOL_TAG));

//
// The class Str has a number of slots and operations:
//       val                                  ???
//       str_field                            the string itself
//       length() : Int                       length of the string
//       concat(arg: Str) : Str               string concatenation
//       substr(arg: Int, arg2: Int): Str     substring
//       
   install_class(
    new CgenNode(
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
	     filename),
        Basic,this, MY_STRING_TAG));
   max_tag = MY_STRING_TAG;
}

// CgenClassTable::install_class
// CgenClassTable::install_classes
//
// install_classes enters a list of classes in the symbol table.
//
void CgenClassTable::install_class(CgenNodeP nd)
{
  Symbol name = nd->get_name();

  if (probe(name))
      return;

  if(cgen_debug) {
	  cout << "installing class " << name << " with tag " << nd->get_tag() << endl;
  }
  // The class name is legal, so add it to the list of classes
  // and the symbol table.
  nds = new List<CgenNode>(nd,nds);
  addid(name,nd);
}

void CgenClassTable::install_classes(Classes cs)
{
	for(int i = cs->first(); cs->more(i); i = cs->next(i)) {
		install_class(new CgenNode(cs->nth(i),NotBasic,this, ++max_tag));
	}
}

//
// CgenClassTable::build_inheritance_tree
//
void CgenClassTable::build_inheritance_tree()
{
  for(List<CgenNode> *l = nds; l; l = l->tl())
      set_relations(l->hd());
}

//
// CgenClassTable::set_relations
//
// Takes a CgenNode and locates its, and its parent's, inheritance nodes
// via the class table.  Parent and child pointers are added as appropriate.
//
void CgenClassTable::set_relations(CgenNodeP nd)
{
  CgenNode *parent_node = probe(nd->get_parent());
  nd->set_parentnd(parent_node);
  parent_node->add_child(nd);
}

void CgenNode::add_child(CgenNodeP n)
{
  children = new List<CgenNode>(n,children);
}

void CgenNode::set_parentnd(CgenNodeP p)
{
  assert(parentnd == NULL);
  assert(p != NULL);
  parentnd = p;
}

void CgenClassTable::code_class_nameTab() {
	str << CLASSNAMETAB << LABEL;
	code_class_nameTab(nds);
}

void CgenClassTable::code_class_nameTab(List<CgenNode> *nds_left) {
	if(!nds_left)
		return;
	code_class_nameTab(nds_left->tl());
	str << WORD;
	stringtable.lookup_string(nds_left->hd()->get_name()->get_string())->code_ref(str);
	str << endl;
}

void CgenClassTable::code_class_objTab() {
	str << CLASSOBJTAB << LABEL;
	code_class_objTab(nds);
}

void CgenClassTable::code_class_objTab(List<CgenNode> *nds_left) {
	if(!nds_left)
		return;
	code_class_objTab(nds_left->tl());
	str << WORD << nds_left->hd()->get_name() << PROTOBJ_SUFFIX << endl;
	str << WORD << nds_left->hd()->get_name() << CLASSINIT_SUFFIX << endl;
}

void CgenClassTable::code_dispTabs() {
	for(List<CgenNode>* l = nds; l; l = l->tl()) {
		CgenNode* node = l->hd();
		if(cgen_debug) cout << "\tcoding dispatch table for class " << node->get_name() << endl;
		str << node->get_name() << DISPTAB_SUFFIX << LABEL;
		node->code_dispTab(str);
	}
}

//I don't the ugly implementation but I failed to find a neat approach to do this.
std::pair<std::vector<Symbol>, std::map<Symbol,Symbol> > CgenNode::find_first_appearance_of_methods()
{
	std::pair<std::vector<Symbol>, std::map<Symbol,Symbol> > res;
	if(get_name() == No_class)
		return res;
	res = get_parentnd()->find_first_appearance_of_methods();
	for(int i = features->first(); features->more(i); i = features->next(i)) {
		Feature f = features->nth(i);
		if(f->get_feature_type() == FEATURE_METHOD) {
			method_class* m = dynamic_cast<method_class*>(f);
			if(res.second.find(m->name) == res.second.end())
				res.first.push_back(m->name);
			res.second[m->name] = get_name();
		}
	}
	return res;
}

void CgenNode::code_dispTab(ostream& s) {
	std::pair<std::vector<Symbol>, std::map<Symbol,Symbol> > first_app = find_first_appearance_of_methods();
	for(size_t i = 0; i < first_app.first.size(); ++i) {
		s << WORD;
		emit_method_ref(first_app.second[first_app.first[i]], first_app.first[i], s);
		s << endl;
		method_offset->addid(first_app.first[i], new int(i));
	}
}


int CgenNode::code_attrs(ostream& s, CgenNode* current_node) {
	if(basic()) {
		if(name == Object || name == IO) return DEFAULT_OBJFIELDS;
		else if(name == Int || name == Bool)  {
			s << WORD << 0 << endl;
			//only one attr
			current_node->attr_offset->addid(
					dynamic_cast<attr_class*>(features->nth(features->first()))->name,
					new int(DEFAULT_OBJFIELDS));
			return DEFAULT_OBJFIELDS + 1;
		}
		else {
			assert(name == Str);
			s << WORD;
			inttable.lookup_string("0")->code_ref(s);	//length = 0
			s << endl << WORD << 0 << endl;				//no character

			int i1 = features->first();					//two attrs
			int i2 = features->next(i1);
			attr_class* attr1 = dynamic_cast<attr_class*>(features->nth(i1));
			attr_class* attr2 = dynamic_cast<attr_class*>(features->nth(i2));
			current_node->attr_offset->addid(attr1->name, new int(DEFAULT_OBJFIELDS));
			current_node->attr_offset->addid(attr2->name, new int(DEFAULT_OBJFIELDS + STRING_SLOTS));
			return DEFAULT_OBJFIELDS + STRING_SLOTS + 1;
		}
	}
	//code attrs of parent recursively.
	int next_offset = parentnd->code_attrs(s, current_node);
	for(int i = features->first(); features->more(i); i = features->next(i)) {
		Feature f = features->nth(i);
		if(f->get_feature_type() == FEATURE_ATTR) {
			attr_class* a = dynamic_cast<attr_class*>(f);
			s << WORD;
			if(a->type_decl == Bool) falsebool.code_ref(s);	//Bool, Int, Str have default values.
			else if(a->type_decl == Int) inttable.lookup_string("0")->code_ref(s);
			else if(a->type_decl == Str) stringtable.lookup_string("")->code_ref(s);
			else s << 0;
			s << endl;

			current_node->attr_offset->addid(a->name, new int(next_offset++));
		}
	}
	return next_offset;
}

int CgenNode::size_in_word() {
	if(basic()) {
		if(name == IO || name == Object)
			return DEFAULT_OBJFIELDS;
		else if(name == Int)
			return DEFAULT_OBJFIELDS + INT_SLOTS;
		else if(name == Bool)
			return DEFAULT_OBJFIELDS + BOOL_SLOTS;
		else {
			assert(name == Str);
			return DEFAULT_OBJFIELDS + STRING_SLOTS + 1;
		}
	}
	int sz = parentnd->size_in_word();
	for(int i = features->first(); features->more(i); i = features->next(i)) {
		if(features->nth(i)->get_feature_type() == FEATURE_ATTR)
			sz++;
	}
	return sz;
}

void CgenNode::code_initializer(ostream& s) {
	s << get_name() << CLASSINIT_SUFFIX << LABEL;
	emit_push(FP,s); 			//store the frame pointer $fp
	emit_push(SELF,s);			//store the self pointer $self
	emit_push(RA,s);			//store the return address $ra

	emit_addiu(FP,SP,4,s);		//set the new frame pointer $fp. but why this value?
	//emit_move(FP,SP,s);
	emit_move(SELF,ACC,s);		//set $self to the prototype object in ACC.
	if(get_name() != Object) {
		s << JAL;				//initialize parent class. No need for Object class.
		emit_init_ref(get_parentnd()->get_name(), s);
		s << endl;
	}
	//Initialize all attributes
	for(int i = features->first(); features->more(i); i = features->next(i)) {
		Feature f = features->nth(i);
		if(f->get_feature_type() == FEATURE_ATTR) {
			attr_class* attr = dynamic_cast<attr_class*>(f);
			//If init expression does not exist, get_type() == NULL. This is not the same
			//as my own implementation of semant.
			if(attr->init->get_type()) {
				//This will put the result of the init expression in ACC
				attr->init->code(s, this, class_table->get_frame_env());
				//Store the value of the init expression at the correct position
				emit_store(ACC,*attr_offset->probe(attr->name),SELF,s);
			}
		}
	}
	emit_move(ACC,SELF,s);	//The initialized object should be saved in ACC
	emit_load(FP,3,SP,s);	//Retrieve old $fp, $self and $ra.
	emit_load(SELF,2,SP,s);
	emit_load(RA,1,SP,s);
	emit_addiu(SP,SP,12,s);	//Restore $sp
	emit_return(s);			//return
}
//
void CgenNode::code_methods(ostream& s) {
	for(int i = features->first(); features->more(i); i = features->next(i)) {
		Feature f = features->nth(i);
		if(f->get_feature_type() == FEATURE_METHOD) {
			method_class* method = dynamic_cast<method_class*>(f);
			if(cgen_debug) {
				cout << "\t\tcoding for ";
				emit_method_ref(name, method->name, cout);
				cout << endl;
			}
			emit_method_ref(name,method->name,s);
			s  << LABEL;

			emit_push(FP,s); 			//store the frame pointer $fp
			emit_push(SELF,s);			//store the self pointer $self
			emit_push(RA,s);			//store the return address $ra

			//set up $fp
			emit_addiu(FP,SP,4,s);		//set the new frame pointer $fp. but why this value?
			//emit_move(FP,SP,s);

			//In dispatch_class, the value of expr is saved in ACC. During the execution of
			//the method, SELF should use this value.
			emit_move(SELF,ACC,s);

			//Put all formals in the frame_env
			class_table->get_frame_env()->enterscope();
			//offset of the first arg: len + 2 (fp + self)
			int offset = method->formals->len() + 2;
			for(int i = method->formals->first(); method->formals->more(i); i = method->formals->next(i)) {
				formal_class* formal = dynamic_cast<formal_class*>(method->formals->nth(i));
				class_table->get_frame_env()->addid(formal->name, new int(offset--));
			}
			method->expr->code(s, this, class_table->get_frame_env());
			class_table->get_frame_env()->exitscope();

			emit_load(FP,3,SP,s);	//Retrieve old $fp, $self and $ra.
			emit_load(SELF,2,SP,s);
			emit_load(RA,1,SP,s);
			emit_addiu(SP,SP,12 + 4 * method->formals->len(),s);	//Restore $sp
			emit_return(s);			//return
		}
	}
}

int CgenNode::get_method_offset(Symbol type, Symbol name) {
	CgenNode* node = (type == SELF_TYPE) ? this : class_table->lookup(type);
	assert(node);
	int* offset = node->method_offset->lookup(name);
	assert(offset);
	return *offset;
}

int CgenNode::get_attr_offset(Symbol name) {
	int* offset = attr_offset->lookup(name);
	if(cgen_debug) {
		cout << this->name << "." << name << endl;
	}
	assert(offset);
	return *offset;
}

void CgenClassTable::code_protObjs() {
	for(List<CgenNode>* l = nds; l; l = l->tl()) {
		CgenNode* node = l->hd();
		if(cgen_debug) cout << "\tcoding prototype object for class " << node->get_name() << " with tage "
				<< node->get_tag() << endl;
		str << WORD << "-1" << endl;
		str << node->get_name() << PROTOBJ_SUFFIX << LABEL;
		str << WORD << node->get_tag() << endl; //tag
		str << WORD << node->size_in_word() << endl; //size
		str << WORD << node->get_name() << DISPTAB_SUFFIX << endl;
		node->code_attrs(str, node);
	}
}

void CgenClassTable::code_initializers() {
	for(List<CgenNode>* l = nds; l; l = l->tl()) {
		CgenNode* node = l->hd();
		if(cgen_debug) cout << "\tcoding initializer for class " << node->get_name() << endl;
		node->code_initializer(str);
	}

}

void CgenClassTable::code_class_methods() {
	for(List<CgenNode>* l = nds; l; l = l->tl()) {
		CgenNode* node = l->hd();
		if(node->basic()) continue;
		if(cgen_debug) cout << "\tcoding methods for class " << node->get_name() << endl;
		node->code_methods(str);
	}
}

void CgenClassTable::code()
{
  if (cgen_debug) cout << "coding global data" << endl;
  code_global_data();

  if (cgen_debug) cout << "choosing gc" << endl;
  code_select_gc();

  if (cgen_debug) cout << "coding constants" << endl;
  code_constants();

  ////////////////////////////////////////////////////////////////////
  if (cgen_debug) cout << "coding class name table" << endl;
  code_class_nameTab();

  if (cgen_debug) cout << "coding class object table" << endl;
  code_class_objTab();

  if (cgen_debug) cout << "coding class dispatch table" << endl;
  code_dispTabs();

  if (cgen_debug) cout << "coding prototype objects" << endl;
  code_protObjs();


//                 Add your code to emit
//                   - prototype objects 			done
//                   - class_nameTab				done
//                   - dispatch tables				done
//

  if (cgen_debug) cout << "coding global text" << endl;
  code_global_text();

  if (cgen_debug) cout << "coding object initializer" << endl;
  code_initializers();

  if (cgen_debug) cout << "coding class methods" << endl;
  code_class_methods();

//                 Add your code to emit
//                   - object initializer
//                   - the class methods
//                   - etc...

}


CgenNodeP CgenClassTable::root()
{
   return probe(Object);
}


///////////////////////////////////////////////////////////////////////
//
// CgenNode methods
//
///////////////////////////////////////////////////////////////////////

CgenNode::CgenNode(Class_ nd, Basicness bstatus, CgenClassTableP ct, int t) :
   class__class((const class__class &) *nd),
   parentnd(NULL),
   children(NULL),
   basic_status(bstatus),
   class_table(ct),
   tag(t),
   attr_offset(new SymbolTable<Symbol, int>()),
   method_offset(new SymbolTable<Symbol, int>())
{ 
   stringtable.add_string(name->get_string());          // Add class name to string table
   attr_offset->enterscope();
   method_offset->enterscope();
}


//******************************************************************
//
//   Fill in the following methods to produce code for the
//   appropriate expression.  You may add or remove parameters
//   as you wish, but if you do, remember to change the parameters
//   of the declarations in `cool-tree.h'  Sample code for
//   constant integers, strings, and booleans are provided.
//
//*****************************************************************

void assign_class::code(ostream &s, CgenNode* current_node, SymbolTable<Symbol, int>* frame_env) {
	expr->code(s, current_node, frame_env);
	int* frame_offset = frame_env->lookup(name);
	if(frame_offset != NULL) {
		emit_store(ACC, *frame_offset, FP, s);
	} else {
		emit_store(ACC, current_node->get_attr_offset(name), SELF, s);
	}
}

void static_dispatch_class::code(ostream &s, CgenNode* current_node, SymbolTable<Symbol, int>* frame_env) {
	for(int i = actual->first(); actual->more(i); i = actual->next(i)) {
		Expression ei = actual->nth(i);
		ei->code(s, current_node, frame_env);
		emit_push(ACC, s);
	}
	expr->code(s, current_node, frame_env);
	int branch_label = i_label++;
	emit_bne(ACC, ZERO, branch_label, s);

	//handle dispatch on void
	//name of current file in $a0
	emit_load_string(ACC, stringtable.lookup_string(current_node->filename->get_string()), s);
	//current line number in $t1
	emit_load_imm(T1,curr_lineno,s);
	emit_jal(DISPATCH_ABORT,s);

	//execute dispatch
	emit_label_def(branch_label,s);
	//load dispTab of type_name
	s << LA << T1 << "\t";
	emit_disptable_ref(type_name, s);
	s << endl;
	emit_load(T1,current_node->get_method_offset(type_name, name),T1,s);
	emit_jalr(T1,s);
}

void dispatch_class::code(ostream &s, CgenNode* current_node, SymbolTable<Symbol, int>* frame_env) {
	if(cgen_debug) {
		cout << "\t\t\tcoding " << expr->type << "." << name << " inside " << current_node->name << endl;
	}
	for(int i = actual->first(); actual->more(i); i = actual->next(i)) {
		Expression ei = actual->nth(i);
		ei->code(s, current_node, frame_env);
		emit_push(ACC, s);
	}
	expr->code(s, current_node, frame_env);
	int branch_label = i_label++;
	emit_bne(ACC, ZERO, branch_label, s);

	//handle dispatch on void
	//name of current file in $a0
	emit_load_string(ACC, stringtable.lookup_string(current_node->filename->get_string()), s);
	//current line number in $t1
	emit_load_imm(T1,curr_lineno,s);
	emit_jal(DISPATCH_ABORT,s);

	//execute dispatch
	emit_label_def(branch_label,s);
	//load dispTab of expr
	emit_load(T1,DISPTABLE_OFFSET,ACC,s);
	emit_load(T1,current_node->get_method_offset(expr->get_type(), name),T1,s);
	emit_jalr(T1,s);
}

void cond_class::code(ostream &s, CgenNode* current_node, SymbolTable<Symbol, int>* frame_env) {
	pred->code(s, current_node, frame_env);
	emit_load_bool(T1,truebool,s);
	int true_branch = i_label++;
	int end_branch = i_label++;

	emit_beq(ACC,T1,true_branch, s);
	else_exp->code(s, current_node, frame_env);
	emit_branch(end_branch, s);

	emit_label_def(true_branch, s);
	then_exp->code(s, current_node, frame_env);

	emit_label_def(end_branch, s);
}

void loop_class::code(ostream &s, CgenNode* current_node, SymbolTable<Symbol, int>* frame_env) {
	int init_branch = i_label++;
	emit_label_def(init_branch,s);

	pred->code(s, current_node, frame_env);

	emit_load_bool(T1,truebool,s);
	int true_branch = i_label++;
	int end_branch = i_label++;

	emit_beq(ACC,T1,true_branch, s);
	emit_branch(end_branch, s);

	emit_label_def(true_branch,s);
	body->code(s, current_node, frame_env);
	emit_branch(init_branch, s);

	emit_label_def(end_branch, s);
	emit_move(ACC,ZERO,s);
}

void typcase_class::code(ostream &s, CgenNode* current_node, SymbolTable<Symbol, int>* frame_env) {
	expr->code(s, current_node, frame_env);
	int non_void_branch = i_label++;
	int end_branch = i_label++;
	//case on void: _case_abort (predefined in runtime system)
	emit_bne(ACC,ZERO,non_void_branch,s);
	emit_load_imm(T1,curr_lineno,s);
	emit_load_string(ACC,stringtable.lookup_string(current_node->filename->get_string()),s);
	emit_jal(CASE_ABORT2,s);

	//dynamic type is not void
	emit_label_def(non_void_branch,s);
	//get the tag of its dynamic type. Ok to use T1 because there can only be one successful comparison.
	//Even if T1 gets edited by expr->code(), no problem shall be caused because T1 is no longer needed.
	//The only case in which T1 is needed later is the case of no match. T1 will not be editted in this case.
	emit_load(T1,TAG_OFFSET,ACC,s);
	//Generate code for all classes whose ancestors appear in the cases.
	for(List<CgenNode>* l = current_node->get_class_table()->get_nds(); l; l = l->tl()) {
		int closest_ancestor = current_node->get_closest_ancestor(l->hd()->name, cases);
		if(closest_ancestor != NO_ANCESTOR) {
			branch_class* branch = dynamic_cast<branch_class*>(cases->nth(closest_ancestor));
			int label = i_label++;
			emit_load_imm(T2,l->hd()->get_tag(),s);
			emit_bne(T1,T2,label,s);
			emit_push(ACC,s);
			frame_env->enterscope();
			frame_env->addid(branch->name, new int(-(++case_layer + let_class::let_layer)));
			branch->expr->code(s, current_node, frame_env);
			frame_env->exitscope();
			--case_layer;
			emit_branch(end_branch,s);
			emit_label_def(label,s);
		}
	}
	emit_jal(CASE_ABORT,s);
	emit_label_def(end_branch,s);
	emit_addiu(SP,SP,4,s);
}

int CgenNode::get_closest_ancestor(Symbol type, Cases cases){
	for(Symbol t = type; t != No_class; t = class_table->lookup(t)->parent) {
		for(int i = cases->first(); cases->more(i); i = cases->next(i)) {
			branch_class* branch = dynamic_cast<branch_class*>(cases->nth(i));
			if(branch->type_decl == t)
				return i;
		}
	}
	return NO_ANCESTOR;
}


void block_class::code(ostream &s, CgenNode* current_node, SymbolTable<Symbol, int>* frame_env) {
	for(int i = body->first(); body->more(i); i = body->next(i)) {
		body->nth(i)->code(s, current_node, frame_env);
	}
}

void let_class::code(ostream &s, CgenNode* current_node, SymbolTable<Symbol, int>* frame_env) {
	if(init->get_type()) {
		//explicit initialization
		init->code(s, current_node, frame_env);
	} else {
		//default initialization.Bool, Int, Str have default values, other types are initialzed to void.
		if(type_decl == Bool) {
			emit_partial_load_address(ACC, s);
			falsebool.code_ref(s);
			s << endl;
		} else if(type_decl == Int) {
			emit_partial_load_address(ACC, s);
			inttable.lookup_string("0")->code_ref(s);
			s << endl;
		}
		else if(type_decl == Str) {
			emit_partial_load_address(ACC, s);
			stringtable.lookup_string("")->code_ref(s);
			s << endl;
		}
		else {
			emit_move(ACC,ZERO,s);
		}
	}
	emit_push(ACC,s);
	frame_env->enterscope();
	frame_env->addid(identifier, new int(-(++let_layer + typcase_class::case_layer)));
	body->code(s, current_node, frame_env);
	frame_env->exitscope();
	--let_layer;
	emit_addiu(SP,SP,4,s);
}

void plus_class::code(ostream &s, CgenNode* current_node, SymbolTable<Symbol, int>* frame_env) {
	e1->code(s, current_node, frame_env);
	emit_push(ACC,s);
	e2->code(s, current_node, frame_env);
	emit_jal(OBJECTCOPY,s);
	emit_load(T1,1,SP,s);

	//load the int values
	emit_fetch_int(T2,T1,s);
	emit_fetch_int(T3,ACC,s);

	//calculation on int values
	emit_add(T1,T2,T3,s);

	//store the int value in the object
	emit_store_int(T1,ACC,s);
	emit_addiu(SP,SP,4,s);
}

void sub_class::code(ostream &s, CgenNode* current_node, SymbolTable<Symbol, int>* frame_env) {
	e1->code(s, current_node, frame_env);
	emit_push(ACC,s);
	e2->code(s, current_node, frame_env);
	emit_jal(OBJECTCOPY,s);
	emit_load(T1,1,SP,s);

	//load the int values
	emit_fetch_int(T2,T1,s);
	emit_fetch_int(T3,ACC,s);

	//calculation on int values
	emit_sub(T1,T2,T3,s);

	//store the int value in the object
	emit_store_int(T1,ACC,s);
	emit_addiu(SP,SP,4,s);
}

void mul_class::code(ostream &s, CgenNode* current_node, SymbolTable<Symbol, int>* frame_env) {
	e1->code(s, current_node, frame_env);
	emit_push(ACC,s);
	e2->code(s, current_node, frame_env);
	emit_jal(OBJECTCOPY,s);
	emit_load(T1,1,SP,s);

	//load the int values
	emit_fetch_int(T2,T1,s);
	emit_fetch_int(T3,ACC,s);

	//calculation on int values
	emit_mul(T1,T2,T3,s);

	//store the int value in the object
	emit_store_int(T1,ACC,s);
	emit_addiu(SP,SP,4,s);
}

void divide_class::code(ostream &s, CgenNode* current_node, SymbolTable<Symbol, int>* frame_env) {
	e1->code(s, current_node, frame_env);
	emit_push(ACC,s);
	e2->code(s, current_node, frame_env);
	emit_jal(OBJECTCOPY,s);
	emit_load(T1,1,SP,s);

	//load the int values
	emit_fetch_int(T2,T1,s);
	emit_fetch_int(T3,ACC,s);

	//calculation on int values
	emit_div(T1,T2,T3,s);

	//store the int value in the object
	emit_store_int(T1,ACC,s);
	emit_addiu(SP,SP,4,s);
}

void neg_class::code(ostream &s, CgenNode* current_node, SymbolTable<Symbol, int>* frame_env) {
	e1->code(s, current_node, frame_env);
	//load the int value
	emit_jal(OBJECTCOPY,s);
	emit_fetch_int(T1,ACC,s);
	emit_neg(T1,T1,s);
	//store the int value
	emit_store_int(T1,ACC,s);
}

void lt_class::code(ostream &s, CgenNode* current_node, SymbolTable<Symbol, int>* frame_env) {
	e1->code(s, current_node, frame_env);
	emit_push(ACC,s);
	e2->code(s, current_node, frame_env);
	emit_load(T1,1,SP,s);

	int true_branch = i_label++;
	int end_branch = i_label++;

	//load the int values
	emit_fetch_int(T2,T1,s);
	emit_fetch_int(T3,ACC,s);

	//if e1 < e2 goto true_branch; otherwise continue
	emit_blt(T2,T3,true_branch,s);

	emit_load_bool(ACC,falsebool,s);
	emit_branch(end_branch,s);

	//true_branch
	emit_label_def(true_branch, s);
	emit_load_bool(ACC,truebool,s);

	//end_branch branch
	emit_label_def(end_branch,s);
	emit_addiu(SP,SP,4,s);

}

void eq_class::code(ostream &s, CgenNode* current_node, SymbolTable<Symbol, int>* frame_env) {
	e1->code(s, current_node, frame_env);
	emit_push(ACC,s);
	e2->code(s, current_node, frame_env);
	emit_load(T1,1,SP,s);

	int true_branch = i_label++;
	int end_branch = i_label++;

	//if they are the same object return true
	emit_beq(T1,ACC,true_branch,s);
	//otherwise check for equality of basic classes. euqality_test is predefined in runtime system
	emit_move(T2,ACC,s);
	emit_load_bool(ACC,truebool,s);
	emit_load_bool(A1,falsebool,s);
	emit_jal(EQUALITY_TEST,s);
	emit_branch(end_branch,s);

	//true_branch
	emit_label_def(true_branch, s);
	emit_load_bool(ACC,truebool,s);

	//end_branch branch
	emit_label_def(end_branch,s);
	emit_addiu(SP,SP,4,s);
}

void leq_class::code(ostream &s, CgenNode* current_node, SymbolTable<Symbol, int>* frame_env) {
	e1->code(s, current_node, frame_env);
	emit_push(ACC,s);
	e2->code(s, current_node, frame_env);
	emit_load(T1,1,SP,s);

	int true_branch = i_label++;
	int end_branch = i_label++;

	//load the int values
	emit_fetch_int(T2,T1,s);
	emit_fetch_int(T3,ACC,s);

	//if e1 <= e2 goto true_branch; otherwise continue
	emit_bleq(T2,T3,true_branch,s);

	emit_load_bool(ACC,falsebool,s);
	emit_branch(end_branch,s);

	//true_branch
	emit_label_def(true_branch, s);
	emit_load_bool(ACC,truebool,s);

	//end_branch branch
	emit_label_def(end_branch,s);
	emit_addiu(SP,SP,4,s);
}

void comp_class::code(ostream &s, CgenNode* current_node, SymbolTable<Symbol, int>* frame_env) {
	e1->code(s, current_node, frame_env);
	int false_branch = i_label++;
	int end_branch = i_label++;
	//load the value in the bool object.
	emit_load(ACC,DEFAULT_OBJFIELDS,ACC,s);
	emit_beqz(ACC,false_branch,s);

	emit_load_bool(ACC,falsebool,s);
	emit_branch(end_branch,s);

	//false_branch
	emit_label_def(false_branch, s);
	emit_load_bool(ACC,truebool,s);

	emit_label_def(end_branch,s);
}

void int_const_class::code(ostream &s, CgenNode* current_node, SymbolTable<Symbol, int>* frame_env)
{
  //
  // Need to be sure we have an IntEntry *, not an arbitrary Symbol
  //
  emit_load_int(ACC,inttable.lookup_string(token->get_string()),s);
}

void string_const_class::code(ostream &s, CgenNode* current_node, SymbolTable<Symbol, int>* frame_env)
{
	emit_load_string(ACC,stringtable.lookup_string(token->get_string()),s);
}

void bool_const_class::code(ostream &s, CgenNode* current_node, SymbolTable<Symbol, int>* frame_env)
{
	emit_load_bool(ACC, BoolConst(val), s);
}

void new__class::code(ostream &s, CgenNode* current_node, SymbolTable<Symbol, int>* frame_env) {
	if(type_name != SELF_TYPE) {
		emit_partial_load_address(ACC,s);
		emit_protobj_ref(type_name,s);
		s << endl;
		emit_jal(OBJECTCOPY,s);
		s << JAL;
		emit_init_ref(type_name,s);
		s << endl;
	} else {
		//address of class_objTab
		emit_load_address(T1,CLASSOBJTAB,s);
		//tag of SELF_TYPE
		emit_load(T2,TAG_OFFSET,SELF,s);
		//shift left by 1 (X2). Offset of prototype object of SELF_TYPE. why 3????
		//TODO
		emit_sll(T2,T2,3,s);
		//address of protObj of SELF_TYPE
		emit_addu(T1,T1,T2,s);
		//t0-t4 are used by the runtime system. We will use the value of T1 again afterwards, thus we save it in S1.
		emit_move(S1,T1,s);
		//load protObj of SELF_TYPE
		emit_load(ACC,0,T1,s);
		//copy.
		emit_jal(OBJECTCOPY,s);
		//load Init of SELF_TYPE
		emit_load(T1,1,S1,s);
		//jump to Init of SELF_TYPE
		emit_jalr(T1,s);
	}
}

void isvoid_class::code(ostream &s, CgenNode* current_node, SymbolTable<Symbol, int>* frame_env) {
	e1->code(s, current_node, frame_env);
	int true_branch = i_label++;
	int end_branch = i_label++;
	emit_beq(ACC,ZERO,true_branch,s);

	emit_load_bool(ACC, falsebool, s);
	emit_branch(end_branch, s);

	emit_label_def(true_branch, s);
	emit_load_bool(ACC, truebool, s);

	emit_label_def(end_branch, s);
}

void no_expr_class::code(ostream &s, CgenNode* current_node, SymbolTable<Symbol, int>* frame_env) {
}

void object_class::code(ostream &s, CgenNode* current_node, SymbolTable<Symbol, int>* frame_env) {
	if(name == self) {
		emit_move(ACC, SELF, s);
	} else {
		int* frame_offset = frame_env->lookup(name);
		if(frame_offset != NULL) {
			emit_load(ACC, *frame_offset, FP, s);
		} else {
			emit_load(ACC, current_node->get_attr_offset(name), SELF, s);
		}
	}
}


