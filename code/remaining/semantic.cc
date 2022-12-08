#include "semantic.hh"


semantic *type_checker = new semantic();


/* Used to check that all functions contain return statements.
   Static means that it is only visible inside this file.
   It is set to false in do_typecheck() (ie, every time we start type checking
   a new block) and set to true if we find an ast_return node. See below. */
static bool has_return = false;


/* Interface for type checking a block of code represented as an AST node. */
void semantic::do_typecheck(symbol *env, ast_stmt_list *body)
{
    // Reset the variable, since we're checking a new block of code.
    has_return = false;
    if (body) {
        body->type_check();
    }

    // This is the only case we need this variable for - a function lacking
    // a return statement. All other cases are already handled in
    // ast_return::type_check(); see below.
    if (env->tag == SYM_FUNC && !has_return) {
        // Note: We could do this by overloading the do_typecheck() method -
        // one for ast_procedurehead and one for ast_functionhead, but this
        // will do... Hopefully people won't write empty functions often,
        // since in that case we won't have position information available.
        if (body != NULL) {
            type_error(body->pos) << "A function must return a value.\n";
        } else {
            type_error() << "A function must return a value.\n";
        }
    }
}


/* Compare formal vs. actual parameters recursively. */
bool semantic::chk_param(ast_id *env,
                        parameter_symbol *formals,
                        ast_expr_list *actuals)
{
    /* code completed - might be wrong */
    if(formals==NULL || actuals==NULL){
        if(formals==NULL && actuals!=NULL){
            type_error(actuals->pos) << "More actual than formal parameters.\n";
            return false;
        }
        if(formals!=NULL && actuals==NULL){
            type_error(env->pos) << "More formal than actual parameters.\n";
            return false;
        }
        return true;
    }
    
    if(formals->type!=actuals->last_expr->type_check()){
        type_error(actuals->pos) << "Type discrepancy between formal and actual parameters.\n";
        return false;
    }
    
    return chk_param(env, formals->preceding, actuals->preceding);
}


/* Check formal vs. actual parameters at procedure/function calls. */
void semantic::check_parameters(ast_id *call_id,
                                ast_expr_list *param_list)
{
    /* code completed */
    symbol *res_sym = sym_tab->get_symbol(call_id->sym_p);
    if(res_sym->tag==SYM_PROC){
        procedure_symbol * proc_sym = res_sym->get_procedure_symbol();
        chk_param(call_id, proc_sym->last_parameter, param_list);
    } else if(res_sym->tag==SYM_FUNC){
        function_symbol * func_sym = res_sym->get_function_symbol();
        chk_param(call_id, func_sym->last_parameter, param_list);
    } else {
        type_error() << "not callable\n";
    }
}



/* We overload this method for the various ast_node subclasses that can
   appear in the AST. By use of virtual (dynamic) methods, we ensure that
   the correct method is invoked even if the pointers in the AST refer to
   one of the abstract classes such as ast_expression or ast_statement. */
sym_index ast_node::type_check()
{
    fatal("Trying to type check abstract class ast_node.");
    return void_type;
}

sym_index ast_statement::type_check()
{
    fatal("Trying to type check abstract class ast_statement.");
    return void_type;
}

sym_index ast_expression::type_check()
{
    fatal("Trying to type check abstract class ast_expression.");
    return void_type;
}

sym_index ast_lvalue::type_check()
{
    fatal("Trying to type check abstract class ast_lvalue.");
    return void_type;
}

sym_index ast_binaryoperation::type_check()
{
    fatal("Trying to type check abstract class ast_binaryoperation.");
    return void_type;
}

sym_index ast_binaryrelation::type_check()
{
    fatal("Trying to type check abstract class ast_binaryrelation.");
    return void_type;
}



/* Type check a list of statements. */
sym_index ast_stmt_list::type_check()
{
    if (preceding != NULL) {
        preceding->type_check();
    }
    if (last_stmt != NULL) {
        last_stmt->type_check();
    }
    return void_type;
}


/* Type check a list of expressions. */
sym_index ast_expr_list::type_check()
{
    /* Your code here */
    return void_type;
}



/* Type check an elsif list. */
sym_index ast_elsif_list::type_check()
{
    /* code completed */
    if (preceding != NULL) {
        preceding->type_check();
    }
    if (last_elsif != NULL) {
        last_elsif->type_check();
    }
    return void_type;
}


/* "type check" an indentifier. We need to separate nametypes from other types
   here, since all nametypes are of type void, but should return an index to
   itself in the symbol table as far as typechecking is concerned. */
sym_index ast_id::type_check()
{
    if (sym_tab->get_symbol(sym_p)->tag != SYM_NAMETYPE) {
        return type;
    }
    return sym_p;
}


sym_index ast_indexed::type_check()
{
    /* code completed - question */
    array_symbol *arr_sym = sym_tab->get_symbol(id->sym_p)->get_array_symbol();
    if (arr_sym->tag != SYM_ARRAY){
        error(pos) << "Can't index not an array \n";
        return void_type;
    }

    if (index->type_check() != integer_type)
        type_error(index->pos) << "index must be of type integer!\n";

    type = arr_sym->type;
    return type;
}



/* This convenience function is used to type check all binary operations
   in which implicit casting of integer to real is done: plus, minus,
   multiplication. We synthesize type information as well. */
sym_index semantic::check_binop1(ast_binaryoperation *node)
{
    /* code completed */
    sym_index left_expr = node->left->type_check();
    sym_index right_expr = node->right->type_check();
    // cout << sym_tab->pool_lookup(sym_tab->get_symbol(left_expr)->id) << "  " << sym_tab->pool_lookup(sym_tab->get_symbol(right_expr)->id);
    if(left_expr==void_type){
        type_error(node->left->pos) << "binoperation cant use void\n";
        return right_expr; // check
    }
    if(right_expr==void_type){
        type_error(node->right->pos) << "binoperation cant use void\n";
        return left_expr; // check
    }

    if(left_expr==right_expr){
        // cout << " -> "<< sym_tab->pool_lookup(sym_tab->get_symbol(left_expr)->id) <<"  \n";
        return left_expr;
    }
    
    if(right_expr==real_type){
        node->left = new ast_cast(node->left->pos, node->left);
    }
    if(left_expr==real_type){
        node->right = new ast_cast(node->right->pos, node->right);
    }

    return real_type;
}

sym_index ast_add::type_check()
{
    /* code completed */
    type = type_checker->check_binop1(this);
    return type;
}

sym_index ast_sub::type_check()
{
    /* code completed */
    type = type_checker->check_binop1(this);
    return type;
}

sym_index ast_mult::type_check()
{
    /* code completed */
    type = type_checker->check_binop1(this);
    return type;
}

/* Divide is a special case, since it always returns real. We make sure the
   operands are cast to real too as needed. */
sym_index ast_divide::type_check()
{
    /* code completed */
    sym_index left_expr = left->type_check();
    if(left_expr != real_type){
        if(left_expr == void_type){
            type_error(left->pos) << "cant divide void \n";
        }else{
            left = new ast_cast(left->pos, left);
        }
    }
    sym_index right_expr = right->type_check();
    if(right->type_check() != real_type){
        if(right_expr == void_type){
            type_error(right->pos) << "cant divide with a void.\n";
        }else{
            right = new ast_cast(right->pos, right);
        }
    }
    type = real_type;
    return real_type;
}



/* This convenience method is used to type check all binary operations
   which only accept integer operands: AND, OR, MOD, DIV.
   The second argument is the name of the operator, so we can generate a
   good error message.
   All of these return integers, so we synthesize that.
   */
sym_index semantic::check_binop2(ast_binaryoperation *node, string s)
{
    /* code completed */
    if(node->left->type_check() != integer_type)
        type_error(node->left->pos) << s;
    if(node->right->type_check() != integer_type)
        type_error(node->right->pos) << s;
    return integer_type;
}

sym_index ast_or::type_check()
{
    /* code completed */
    type = type_checker->check_binop2(this, "or wrong");
    return type;
}

sym_index ast_and::type_check()
{
    /* code completed */
    type = type_checker->check_binop2(this, "and wrong");
    return type;
}

sym_index ast_idiv::type_check()
{
    /* code completed */
    type = type_checker->check_binop2(this, "idiv wrong");
    return type;
}

sym_index ast_mod::type_check()
{
    /* code completed */
    return type_checker->check_binop2(this, "mod wrong");
}



/* Convienience method for all binary relations, since they're all typechecked
   the same way. They all return integer types, 1 = true, 0 = false. */
sym_index semantic::check_binrel(ast_binaryrelation *node)
{
    /* code completed */
    sym_index left_expr = node->left->type_check();
    sym_index right_expr = node->right->type_check();

    if(left_expr==integer_type && right_expr==integer_type)
        return integer_type;
    
    if(left_expr!=real_type){
        if(left_expr!=integer_type)
            type_error(node->left->pos) << "binrel.\n";
        else
            node->left = new ast_cast(node->left->pos, node->left);
    }
    if(right_expr!=real_type){
        if(right_expr!=integer_type)
            type_error(node->right->pos) << "binrel.\n";
        else
            node->right = new ast_cast(node->right->pos, node->right);
    }

    return integer_type;
}

sym_index ast_equal::type_check()
{
    /* code completed */
    return type_checker->check_binrel(this);
}

sym_index ast_notequal::type_check()
{
    /* code completed */
    return type_checker->check_binrel(this);
}

sym_index ast_lessthan::type_check()
{
    /* code completed */
    return type_checker->check_binrel(this);
}

sym_index ast_greaterthan::type_check()
{
    /* code completed */
    return type_checker->check_binrel(this);
}



/*** The various classes derived from ast_statement. ***/

sym_index ast_procedurecall::type_check()
{
    /* code completed - add check if function/procedure */
    type_checker->check_parameters(id, parameter_list);
    return void_type;
}


sym_index ast_assign::type_check()
{
    /* code completed */
    sym_index left_expr = lhs->type_check();
    sym_index right_expr = rhs->type_check();
    if(left_expr==real_type && right_expr==integer_type){
        rhs = new ast_cast(rhs->pos, rhs);
        return real_type;
    }
    else if(left_expr==right_expr){
        return left_expr;
    }

    type_error(rhs->pos) << "Can't assign a " << sym_tab->pool_lookup(sym_tab->get_symbol_id(right_expr)) 
                         << " value to " << sym_tab->pool_lookup(sym_tab->get_symbol_id(left_expr)) 
                         << " variable.\n";
    return void_type;
}


sym_index ast_while::type_check()
{
    if (condition->type_check() != integer_type) {
        type_error(condition->pos) << "while predicate must be of integer "
                                   << "type.\n";
    }

    if (body != NULL) {
        body->type_check();
    }
    return void_type;
}


sym_index ast_if::type_check()
{
    /* code completed */
    if (condition->type_check() != integer_type) {
        type_error(condition->pos) << "if predicate must be of integer "
                                   << "type.\n";
    }

    if (body != NULL) {
        body->type_check();
    }
    if (elsif_list != NULL) {
        elsif_list->type_check();
    }
    if (else_body != NULL) {
        else_body->type_check();
    }
    return void_type;
}


sym_index ast_return::type_check()
{
    // This static global (meaning it is global for all methods in this file,
    // but not visible outside this file) variable is reset to 0 every time
    // we start type checking a new block of code. If we find a return
    // statement, we set it to 1. It is checked in the do_typecheck() method
    // of semantic.cc.
    has_return = true;

    // Get the current environment. We don't yet know if it's a procedure or
    // a function.
    symbol *tmp = sym_tab->get_symbol(sym_tab->current_environment());
    if (value == NULL) {
        // If the return value is NULL,
        if (tmp->tag != SYM_PROC)
            // ...and we're not inside a procedure, something is wrong.
        {
            type_error(pos) << "Must return a value from a function.\n";
        }
        return void_type;
    }

    sym_index value_type = value->type_check();

    // The return value is not NULL,
    if (tmp->tag != SYM_FUNC) {
        // ...so if we're not inside a function, something is wrong too.
        type_error(pos) << "Procedures may not return a value.\n";
        return void_type;
    }

    // Now we know it's a function and can safely downcast.
    function_symbol *func = tmp->get_function_symbol();

    // Must make sure that the return type matches the function's
    // declared return type.
    if (func->type != value_type) {
        type_error(value->pos) << "Bad return type from function.\n";
    }

    return void_type;
}


sym_index ast_functioncall::type_check()
{
    /* code completed */
    symbol *res_sym = sym_tab->get_symbol(id->sym_p);
    if(res_sym->tag!=SYM_FUNC){
        type_error(pos) << "not defined as func\n";
    }
    type_checker->check_parameters(id, parameter_list);
    return type;
}

sym_index ast_uminus::type_check()
{
    /* code completed */
    type = expr->type_check();
    if(type==void_type)
        type_error(pos) << "Uminus\n";
    
    return type;
}

sym_index ast_not::type_check()
{
    /* code completed */
    if(expr->type_check() != integer_type)
        type_error(pos) << "not\n";
    
    return integer_type;
}


sym_index ast_elsif::type_check()
{
    /* Your code here */
    if (condition->type_check() != integer_type) {
        type_error(condition->pos) << "elsif predicate must be of integer "
                                   << "type.\n";
    }
    if (body != NULL) {
        body->type_check();
    }
    return void_type;
}



sym_index ast_integer::type_check()
{
    return integer_type;
}

sym_index ast_real::type_check()
{
    return real_type;
}
