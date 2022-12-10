#include <iostream>
#include <iomanip>
#include <fstream>
#include <stdio.h>
#include <string.h>

#include "symtab.hh"
#include "quads.hh"
#include "codegen.hh"

using namespace std;

// Defined in main.cc.
extern bool assembler_trace;

// Used in parser.y. Ideally the filename should be parametrized, but it's not
// _that_ important...
code_generator *code_gen = new code_generator("d.out");

// Constructor.
code_generator::code_generator(const string object_file_name)
{
    out.open(object_file_name);

    reg[RAX] = "rax";
    reg[RCX] = "rcx";
    reg[RDX] = "rdx";
}


/* Destructor. */
code_generator::~code_generator()
{
    // Make sure we close the outfile before exiting the compiler.
    out << flush;
    out.close();
}



/* This method is called from parser.y when code generation is to start.
   The argument is a quad_list representing the body of the procedure, and
   the symbol for the environment for which code is being generated. */
void code_generator::generate_assembler(quad_list *q, symbol *env)
{
    prologue(env);
    expand(q);
    epilogue(env);
}



/* This method aligns a frame size on an 8-byte boundary. Used by prologue().
 */
int code_generator::align(int frame_size)
{
    return ((frame_size + 7) / 8) * 8;
}



/* This method generates assembler code for initialisating a procedure or
   function. */
void code_generator::prologue(symbol *new_env)
{
    int ar_size;
    int label_nr;
    // Used to count parameters.
    parameter_symbol *last_arg;

    // Again, we need a safe downcast for a procedure/function.
    // Note that since we have already generated quads for the entire block
    // before we expand it to assembler, the size of the activation record
    // is known here (ar_size).
    if (new_env->tag == SYM_PROC) {
        procedure_symbol *proc = new_env->get_procedure_symbol();
        ar_size = align(proc->ar_size);
        label_nr = proc->label_nr;
        last_arg = proc->last_parameter;
    } else if (new_env->tag == SYM_FUNC) {
        function_symbol *func = new_env->get_function_symbol();
        /* Make sure ar_size is a multiple of eight */
        ar_size = align(func->ar_size);
        label_nr = func->label_nr;
        last_arg = func->last_parameter;
    } else {
        fatal("code_generator::prologue() called for non-proc/func");
        return;
    }

    /* Print out the label number (a SYM_PROC/ SYM_FUNC attribute) */
    out << "L" << label_nr << ":" << "\t\t\t" << "# " <<
        /* Print out the function/procedure name */
        sym_tab->pool_lookup(new_env->id) << endl;

    if (assembler_trace) {
        out << "\t" << "# PROLOGUE (" << short_symbols << new_env
            << long_symbols << ")" << endl;
    }

    /* code completed */
    // push previous rbp
    out << "\t\t" << "push" << "\t" << "rbp" << endl;

    // save rsp for updating the rdp after pushing the display
    out << "\t\t" << "mov" << "\t" << "rcx, rsp" << endl;

    // push the display (rcx has current rdp stored)
    for(int i = 1; new_env->level >= i; i++)
        out << "\t\t" << "push" << "\t" << "[rbp-" << (i*STACK_WIDTH) << "]" << endl;
    out << "\t\t" << "push" << "\t" << "rcx" << endl;

    // updating current rbp
    out << "\t\t" << "mov" << "\t" << "rbp, rcx" << endl;

    // allocating space for local and global variables
    out << "\t\t" << "sub" << "\t" << "rsp, " << ar_size << endl;

    out << flush;
}



/* This method generates assembler code for leaving a procedure or function. */
void code_generator::epilogue(symbol *old_env)
{
    if (assembler_trace) {
        out << "\t" << "# EPILOGUE (" << short_symbols << old_env
            << long_symbols << ")" << endl;
    }

    /* code completed */
    out << "\t\t" << "leave" << endl; // copies old rdp from stack back in the register -> releasing activation record
    out << "\t\t" << "ret" << endl; // Return to the caller (pops return address from stack)

    out << flush;
}



/* This function finds the display register level and offset for a variable,
   array or a parameter. Note the pass-by-pointer arguments. */
void code_generator::find(sym_index sym_p, int *level, int *offset)
{
    /* code completed */
    symbol *symb = sym_tab->get_symbol(sym_p);
    *level = symb->level;
    if(symb->tag == SYM_VAR || symb->tag == SYM_ARRAY){
        // offset calculation: previous rdp + display + offset from symtab (minus since stack grows downwards)
        *offset = -(STACK_WIDTH + (*level)*STACK_WIDTH + symb->offset);
    } else {
        // offset calculation: return adress + offset + addition Stack_width in order to point to the start of an element not the end
        *offset = STACK_WIDTH + symb->offset + STACK_WIDTH;
    }
}

/*
 * Generates code for getting the address of a frame for the specified scope level.
 */
void code_generator::frame_address(int level, const register_type dest)
{
    /* code completed */
    // mov display of a specified level into the destination register
    // adress calculation: STACK_WIDTH (skip previous rdp) + (level-1) * STACK_WIDTH to get the right element from the display
    //                      --> can be rewritten to (STACK_WIDTH*level)
    out << "\t\t" << "mov" << "\t" << reg[dest] << ", [rdp-" << (STACK_WIDTH*level) << "]" << endl;
}

/* This function fetches the value of a variable or a constant into a
   register. */
void code_generator::fetch(sym_index sym_p, register_type dest)
{
    /* code completed */
    block_level level;      // Current scope level.
    int offset;             // Offset within current activation record.
    symbol *symb = sym_tab->get_symbol(sym_p);

    /* we differ between variable (+paramater) and constants 
       for constants we can make a simple "mov reg, value"
       while we have to fetch the value from the stack for variables 
                --> mov reg, [address] */
    if(symb->tag == SYM_CONST){
        constant_symbol* cs = symb->get_constant_symbol();

        out << "\t\t" << "mov" << "\t" << reg[dest];
        out << cs->const_value.ival << endl;
    } else if(symb->tag == SYM_VAR || symb->tag == SYM_PARAM) {
        // we calculate the level and offset first with find
        find(sym_p, &level, &offset);
        /* fetch the rdp of the level in the destination register 
           (we reuse the desitnation reg to avoid side effects) */
        frame_address(level, dest);

        out << "\t\t" << "mov" << "\t" << reg[dest] << ", [" << reg[dest];
        if (offset >= 0) {
            out << "+" << offset;
        } else {
            out << offset; // Implicit "-"
        }
        out << "]" << endl;
    }
}

void code_generator::fetch_float(sym_index sym_p)
{
    /* Your code here - continue here */
    block_level level;      // Current scope level.
    int offset;             // Offset within current activation record.
    symbol *symb = sym_tab->get_symbol(sym_p);

    /*  */
    if(symb->tag == SYM_CONST){
        constant_symbol* cs = symb->get_constant_symbol();

        out << "\t\t" << "fld" << "\t" << cs->const_value.rval << endl;
    } else if(symb->tag == SYM_VAR || symb->tag == SYM_PARAM) {
        find(sym_p, &level, &offset);
        frame_address(level, RCX);

        out << "\t\t" << "fld" << "\t[" << reg[RCX];
        if (offset >= 0) {
            out << "+" << offset;
        } else {
            out << offset; // Implicit "-"
        }
        out << "]" << endl;
    }
}



/* This function stores the value of a register into a variable. */
void code_generator::store(register_type src, sym_index sym_p)
{
    /* code completed */
    block_level level;      // Current scope level.
    int offset;             // Offset within current activation record.
    
    // again find the address of variable and store it in the src-register
    find(sym_p, &level, &offset);
    frame_address(level, src);

    // "mov adress, src" moves the value of the src-register into variable at "adress"
    out << "\t\t" << "mov" << "\t" << "[" << reg[src];
    if (offset >= 0) {
        out << "+" << offset;
    } else {
        out << offset; // Implicit "-"
    }
    out << "], " << reg[src] << endl;
}

void code_generator::store_float(sym_index sym_p)
{
    /* Your code here */
}


/* This function fetches the base address of an array. */
void code_generator::array_address(sym_index sym_p, register_type dest)
{
    /* code completed */
    block_level level;      // Current scope level.
    int offset;             // Offset within current activation record.
    
    // again find the address of variable and store it in the src-register
    find(sym_p, &level, &offset);
    frame_address(level, dest);

    // add/sub the offset to the frame address (which is currently in dest) 
    if (offset >= 0) {
        out << "\t\t" << "add" << "\t" << reg[dest] << offset;
    } else {
        out << "\t\t" << "sub" << "\t" << reg[dest] << (-offset);
    }
    out << endl;
}

/* This method expands a quad_list into assembler code, quad for quad. */
void code_generator::expand(quad_list *q_list)
{
    long quad_nr = 0;       // Just to make debug output easier to read.
    

    // We use this iterator to loop through the quad list.
    quad_list_iterator *ql_iterator = new quad_list_iterator(q_list);

    quadruple *q = ql_iterator->get_current(); // This is the head of the list.

    while (q != NULL) {
        quad_nr++;

        // We always do labels here so that a branch doesn't miss the
        // trace code.
        if (q->op_code == q_labl) {
            out << "L" << q->int1 << ":" << endl;
        }

        // Debug output.
        if (assembler_trace) {
            out << "\t" << "# QUAD " << quad_nr << ": "
                << short_symbols << q << long_symbols << endl;
        }

        // The main switch on quad type. This is where code is actually
        // generated.
        switch (q->op_code) {
        case q_rload:
        case q_iload:
            out << "\t\t" << "mov" << "\t" << "rax, " << q->int1 << endl;
            store(RAX, q->sym3);
            break;

        case q_inot: {
            int label = sym_tab->get_next_label();
            int label2 = sym_tab->get_next_label();

            fetch(q->sym1, RAX);
            out << "\t\t" << "cmp" << "\t" << "rax, 0" << endl;
            out << "\t\t" << "je" << "\t" << "L" << label << endl;
            // Not equal branch
            out << "\t\t" << "mov" << "\t" << "rax, 0" << endl;
            out << "\t\t" << "jmp" << "\t" << "L" << label2 << endl;
            // Equal branch
            out << "\t\t" << "L" << label << ":" << endl;
            out << "\t\t" << "mov" << "\t" << "rax, 1" << endl;

            out << "\t\t" << "L" << label2 << ":" <<  endl;
            store(RAX, q->sym3);
            break;
        }
        case q_ruminus:
            fetch_float(q->sym1);
            out << "\t\t" << "fchs" << endl;
            store_float(q->sym3);
            break;

        case q_iuminus:
            fetch(q->sym1, RAX);
            out << "\t\t" << "neg" << "\t" << "rax" << endl;
            store(RAX, q->sym3);
            break;

        case q_rplus:
            fetch_float(q->sym1);
            fetch_float(q->sym2);
            out << "\t\t" << "faddp" << endl;
            store_float(q->sym3);
            break;

        case q_iplus:
            fetch(q->sym1, RAX);
            fetch(q->sym2, RCX);
            out << "\t\t" << "add" << "\t" << "rax, rcx" << endl;
            store(RAX, q->sym3);
            break;

        case q_rminus:
            fetch_float(q->sym1);
            fetch_float(q->sym2);
            out << "\t\t" << "fsubp" << endl;
            store_float(q->sym3);
            break;

        case q_iminus:
            fetch(q->sym1, RAX);
            fetch(q->sym2, RCX);
            out << "\t\t" << "sub" << "\t" << "rax, rcx" << endl;
            store(RAX, q->sym3);
            break;

        case q_ior: {
            int label = sym_tab->get_next_label();
            int label2 = sym_tab->get_next_label();

            fetch(q->sym1, RAX);
            out << "\t\t" << "cmp" << "\t" << "rax, 0" << endl;
            out << "\t\t" << "jne" << "\t" << "L" << label << endl;
            fetch(q->sym2, RAX);
            out << "\t\t" << "cmp" << "\t" << "rax, 0" << endl;
            out << "\t\t" << "jne" << "\t" << "L" << label << endl;
            // False branch
            out << "\t\t" << "mov" << "\t" << "rax, 0" << endl;
            out << "\t\t" << "jmp" << "\t" << "L" << label2 << endl;
            // True branch
            out << "\t\t" << "L" << label << ":" << endl;
            out << "\t\t" << "mov" << "\t" << "rax, 1" << endl;

            out << "\t\t" << "L" << label2 << ":" << endl;
            store(RAX, q->sym3);
            break;
        }
        case q_iand: {
            int label = sym_tab->get_next_label();
            int label2 = sym_tab->get_next_label();

            fetch(q->sym1, RAX);
            out << "\t\t" << "cmp" << "\t" << "rax, 0" << endl;
            out << "\t\t" << "je" << "\t" << "L" << label << endl;
            fetch(q->sym2, RAX);
            out << "\t\t" << "cmp" << "\t" << "rax, 0" << endl;
            out << "\t\t" << "je" << "\t" << "L" << label << endl;
            // True branch
            out << "\t\t" << "mov" << "\t" << "rax, 1" << endl;
            out << "\t\t" << "jmp" << "\t" << "L" << label2 << endl;
            // False branch
            out << "\t\t" << "L" << label << ":" << endl;
            out << "\t\t" << "mov" << "\t" << "rax, 0" << endl;

            out << "\t\t" << "L" << label2 << ":" << endl;
            store(RAX, q->sym3);
            break;
        }
        case q_rmult:
            fetch_float(q->sym1);
            fetch_float(q->sym2);
            out << "\t\t" << "fmulp" << endl;
            store_float(q->sym3);
            break;

        case q_imult:
            fetch(q->sym1, RAX);
            fetch(q->sym2, RCX);
            out << "\t\t" << "imul" << "\t" << "rax, rcx" << endl;
            store(RAX, q->sym3);
            break;

        case q_rdivide:
            fetch_float(q->sym1);
            fetch_float(q->sym2);
            out << "\t\t" << "fdivp" << endl;
            store_float(q->sym3);
            break;

        case q_idivide:
            fetch(q->sym1, RAX);
            fetch(q->sym2, RCX);
            out << "\t\t" << "cqo" << endl;
            out << "\t\t" << "idiv" << "\t" << "rax, rcx" << endl;
            store(RAX, q->sym3);
            break;

        case q_imod:
            fetch(q->sym1, RAX);
            fetch(q->sym2, RCX);
            out << "\t\t" << "cqo" << endl;
            out << "\t\t" << "idiv" << "\t" << "rax, rcx" << endl;
            store(RDX, q->sym3);
            break;

        case q_req: {
            int label = sym_tab->get_next_label();
            int label2 = sym_tab->get_next_label();

            fetch_float(q->sym1);
            fetch_float(q->sym2);
            out << "\t\t" << "fcomip" << "\t" << "ST(0), ST(1)" << endl;
            // Clear the stack
            out << "\t\t" << "fstp" << "\t" << "ST(0)" << endl;
            out << "\t\t" << "je" << "\t" << "L" << label << endl;
            // False branch
            out << "\t\t" << "mov" << "\t" << "rax, 0" << endl;
            out << "\t\t" << "jmp" << "\t" << "L" << label2 << endl;
            // True branch
            out << "\t\t" << "L" << label << ":" << endl;
            out << "\t\t" << "mov" << "\t" << "rax, 1" << endl;

            out << "\t\t" << "L" << label2 << ":" << endl;
            store(RAX, q->sym3);
            break;
        }
        case q_ieq: {
            int label = sym_tab->get_next_label();
            int label2 = sym_tab->get_next_label();

            fetch(q->sym1, RAX);
            fetch(q->sym2, RCX);
            out << "\t\t" << "cmp" << "\t" << "rax, rcx" << endl;
            out << "\t\t" << "je" << "\t" << "L" << label << endl;
            // False branch
            out << "\t\t" << "mov" << "\t" << "rax, 0" << endl;
            out << "\t\t" << "jmp" << "\t" << "L" << label2 << endl;
            // True branch
            out << "\t\t" << "L" << label << ":" << endl;
            out << "\t\t" << "mov" << "\t" << "rax, 1" << endl;

            out << "\t\t" << "L" << label2 << ":" << endl;
            store(RAX, q->sym3);
            break;
        }
        case q_rne: {
            int label = sym_tab->get_next_label();
            int label2 = sym_tab->get_next_label();

            fetch_float(q->sym1);
            fetch_float(q->sym2);
            out << "\t\t" << "fcomip" << "\t" << "ST(0), ST(1)" << endl;
            // Clear the stack
            out << "\t\t" << "fstp" << "\t" << "ST(0)" << endl;
            out << "\t\t" << "jne" << "\t" << "L" << label << endl;
            // False branch
            out << "\t\t" << "mov" << "\t" << "rax, 0" << endl;
            out << "\t\t" << "jmp" << "\t" << "L" << label2 << endl;
            // True branch
            out << "\t\t" << "L" << label << ":" << endl;
            out << "\t\t" << "mov" << "\t" << "rax, 1" << endl;

            out << "\t\t" << "L" << label2 << ":" << endl;
            store(RAX, q->sym3);
            break;
        }
        case q_ine: {
            int label = sym_tab->get_next_label();
            int label2 = sym_tab->get_next_label();

            fetch(q->sym1, RAX);
            fetch(q->sym2, RCX);
            out << "\t\t" << "cmp" << "\t" << "rax, rcx" << endl;
            out << "\t\t" << "jne" << "\t" << "L" << label << endl;
            // False branch
            out << "\t\t" << "mov" << "\t" << "rax, 0" << endl;
            out << "\t\t" << "jmp" << "\t" << "L" << label2 << endl;
            // True branch
            out << "\t\t" << "L" << label << ":" << endl;
            out << "\t\t" << "mov" << "\t" << "rax, 1" << endl;

            out << "\t\t" << "L" << label2 << ":" << endl;
            store(RAX, q->sym3);
            break;
        }
        case q_rlt: {
            int label = sym_tab->get_next_label();
            int label2 = sym_tab->get_next_label();

            // We need to push in reverse order for this to work
            fetch_float(q->sym2);
            fetch_float(q->sym1);
            out << "\t\t" << "fcomip" << "\t" << "ST(0), ST(1)" << endl;
            // Clear the stack
            out << "\t\t" << "fstp" << "\t" << "ST(0)" << endl;
            out << "\t\t" << "jb" << "\t" << "L" << label << endl;
            // False branch
            out << "\t\t" << "mov" << "\t" << "rax, 0" << endl;
            out << "\t\t" << "jmp" << "\t" << "L" << label2 << endl;
            // True branch
            out << "\t\t" << "L" << label << ":" << endl;
            out << "\t\t" << "mov" << "\t" << "rax, 1" << endl;

            out << "\t\t" << "L" << label2 << ":" << endl;
            store(RAX, q->sym3);
            break;
        }
        case q_ilt: {
            int label = sym_tab->get_next_label();
            int label2 = sym_tab->get_next_label();

            fetch(q->sym1, RAX);
            fetch(q->sym2, RCX);
            out << "\t\t" << "cmp" << "\t" << "rax, rcx" << endl;
            out << "\t\t" << "jl" << "\t" << "L" << label << endl;
            // False branch
            out << "\t\t" << "mov" << "\t" << "rax, 0" << endl;
            out << "\t\t" << "jmp" << "\t" << "L" << label2 << endl;
            // True branch
            out << "\t\t" << "L" << label << ":" << endl;
            out << "\t\t" << "mov" << "\t" << "rax, 1" << endl;

            out << "\t\t" << "L" << label2 << ":" << endl;
            store(RAX, q->sym3);
            break;
        }
        case q_rgt: {
            int label = sym_tab->get_next_label();
            int label2 = sym_tab->get_next_label();

            // We need to push in reverse order for this to work
            fetch_float(q->sym2);
            fetch_float(q->sym1);
            out << "\t\t" << "fcomip" << "\t" << "ST(0), ST(1)" << endl;
            // Clear the stack
            out << "\t\t" << "fstp" << "\t" << "ST(0)" << endl;
            out << "\t\t" << "ja" << "\t" << "L" << label << endl;
            // False branch
            out << "\t\t" << "mov" << "\t" << "rax, 0" << endl;
            out << "\t\t" << "jmp" << "\t" << "L" << label2 << endl;
            // True branch
            out << "\t\t" << "L" << label << ":" << endl;
            out << "\t\t" << "mov" << "\t" << "rax, 1" << endl;

            out << "\t\t" << "L" << label2 << ":" << endl;
            store(RAX, q->sym3);
            break;
        }
        case q_igt: {
            int label = sym_tab->get_next_label();
            int label2 = sym_tab->get_next_label();

            fetch(q->sym1, RAX);
            fetch(q->sym2, RCX);
            out << "\t\t" << "cmp" << "\t" << "rax, rcx" << endl;
            out << "\t\t" << "jg" << "\t" << "L" << label << endl;
            // False branch
            out << "\t\t" << "mov" << "\t" << "rax, 0" << endl;
            out << "\t\t" << "jmp" << "\t" << "L" << label2 << endl;
            // True branch
            out << "\t\t" << "L" << label << ":" << endl;
            out << "\t\t" << "mov" << "\t" << "rax, 1" << endl;

            out << "\t\t" << "L" << label2 << ":" << endl;
            store(RAX, q->sym3);
            break;
        }
        case q_rstore:
        case q_istore:
            fetch(q->sym1, RAX);
            fetch(q->sym3, RCX);
            out << "\t\t" << "mov" << "\t" << "[rcx], rax" << endl;
            break;

        case q_rassign:
        case q_iassign:
            fetch(q->sym1, RAX);
            store(RAX, q->sym3);
            break;

        case q_param:
            /* code completed */
            // this quad just fetches a value and pushes the result onto the stack
            fetch(q->sym1, RAX);
            out << "\t\t" << "push" << "\t" << reg[RAX] << endl;
            break;

        case q_call: {
            /* code completed */
            symbol *symb = sym_tab->get_symbol(q->sym1);
            /* function and procedure differ since procdure doesnt return anything (no store) 
               but besides that both have the same structure... */
            if(symb->tag == SYM_FUNC){
                function_symbol *func_symb = symb->get_function_symbol(); // simple cast
                // is dont by call out << "\t\t" << "push" << "\t" << "rsp" << endl; // push the return address
                out << "\t\t" << "call" << "\t" << "L" << func_symb->label_nr;
                out << "\t# " << sym_tab->pool_lookup(func_symb->id) << endl; // call the correct label
                out << "\t\t" << "add" << "\t" << "rsp, " << (STACK_WIDTH*q->int2) << endl; // cleanup stack - remove calling parameter
                store(RAX, q->sym3);
            } else if (symb->tag == SYM_PROC) {
                procedure_symbol *proc_symb = symb->get_procedure_symbol();
                // out << "\t\t" << "push" << "\t" << "rsp" << endl;
                out << "\t\t" << "call" << "\t" << "L" << proc_symb->label_nr;
                out << "\t# " << sym_tab->pool_lookup(proc_symb->id) << endl;
                out << "\t\t" << "add" << "\t" << "rsp, " << (STACK_WIDTH*q->int2) << endl;
            }
            break;
        }
        case q_rreturn:
        case q_ireturn:
            fetch(q->sym2, RAX);
            out << "\t\t" << "jmp" << "\t" << "L" << q->int1 << endl;
            break;

        case q_lindex:
            array_address(q->sym1, RAX);
            fetch(q->sym2, RCX);
            out << "\t\t" << "imul" << "\t" << "rcx, " << STACK_WIDTH << endl;
            out << "\t\t" << "sub" << "\t" << "rax, rcx" << endl;
            store(RAX, q->sym3);
            break;

        case q_rrindex:
        case q_irindex:
            array_address(q->sym1, RAX);
            fetch(q->sym2, RCX);
            out << "\t\t" << "imul" << "\t" << "rcx, " << STACK_WIDTH << endl;
            out << "\t\t" << "sub" << "\t" << "rax, rcx" << endl;
            out << "\t\t" << "mov" << "\t" << "rax, [rax]" << endl;
            store(RAX, q->sym3);
            break;

        case q_itor: {
            block_level level;      // Current scope level.
            int offset;             // Offset within current activation record.

            find(q->sym1, &level, &offset);
            frame_address(level, RCX);
            out << "\t\t" << "fild" << "\t" << "qword ptr [rcx";
            if (offset >= 0) {
                out << "+" << offset;
            } else {
                out << offset; // Implicit "-"
            }
            out << "]" << endl;
            store_float(q->sym3);
        }
        break;

        case q_jmp:
            out << "\t\t" << "jmp" << "\t" << "L" << q->int1 << endl;
            break;

        case q_jmpf:
            fetch(q->sym2, RAX);
            out << "\t\t" << "cmp" << "\t" << "rax, 0" << endl;
            out << "\t\t" << "je" << "\t" << "L" << q->int1 << endl;
            break;

        case q_labl:
            // We handled this one above already.
            break;

        case q_nop:
            // q_nop quads should never be generated.
            fatal("code_generator::expand(): q_nop quadruple produced.");
            return;
        }

        // Get the next quad from the list.
        q = ql_iterator->get_next();
    }

    // Flush the generated code to file.
    out << flush;
}
