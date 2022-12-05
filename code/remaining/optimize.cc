#include "optimize.hh"

/*** This file contains all code pertaining to AST optimisation. It currently
     implements a simple optimisation called "constant folding". Most of the
     methods in this file are empty, or just relay optimize calls downward
     in the AST. If a more powerful AST optimization scheme were to be
     implemented, only methods in this file should need to be changed. ***/


ast_optimizer *optimizer = new ast_optimizer();


/* The optimizer's interface method. Starts a recursive optimize call down
   the AST nodes, searching for binary operators with constant children. */
void ast_optimizer::do_optimize(ast_stmt_list *body)
{
    if (body != NULL) {
        body->optimize();
    }
}


/* Returns 1 if an AST expression is a subclass of ast_binaryoperation,
   ie, eligible for constant folding. */
bool ast_optimizer::is_binop(ast_expression *node)
{
    switch (node->tag) {
    case AST_ADD:
    case AST_SUB:
    case AST_OR:
    case AST_AND:
    case AST_MULT:
    case AST_DIVIDE:
    case AST_IDIV:
    case AST_MOD:
        return true;
    default:
        return false;
    }
}



/* We overload this method for the various ast_node subclasses that can
   appear in the AST. By use of virtual (dynamic) methods, we ensure that
   the correct method is invoked even if the pointers in the AST refer to
   one of the abstract classes such as ast_expression or ast_statement. */
void ast_node::optimize()
{
    fatal("Trying to optimize abstract class ast_node.");
}

void ast_statement::optimize()
{
    fatal("Trying to optimize abstract class ast_statement.");
}

void ast_expression::optimize()
{
    fatal("Trying to optimize abstract class ast_expression.");
}

void ast_lvalue::optimize()
{
    fatal("Trying to optimize abstract class ast_lvalue.");
}

void ast_binaryoperation::optimize()
{
    fatal("Trying to optimize abstract class ast_binaryoperation.");
}

void ast_binaryrelation::optimize()
{
    fatal("Trying to optimize abstract class ast_binaryrelation.");
}



/*** The optimize methods for the concrete AST classes. ***/

/* Optimize a statement list. */
void ast_stmt_list::optimize()
{
    if (preceding != NULL) {
        preceding->optimize();
    }
    if (last_stmt != NULL) {
        last_stmt->optimize();
    }
}


/* Optimize a list of expressions. */
void ast_expr_list::optimize()
{
    /* code completed */
    if (preceding != NULL) {
        preceding->optimize();
    }
    if (last_expr != NULL) {
        last_expr->optimize();
    }
}


/* Optimize an elsif list. */
void ast_elsif_list::optimize()
{
    /* code completed */
    if (preceding != NULL) {
        preceding->optimize();
    }
    if (last_elsif != NULL) {
        last_elsif->optimize();
    }
}


/* An identifier's value can change at run-time, so we can't perform
   constant folding optimization on it unless it is a constant.
   Thus we just do nothing here. It can be treated in the fold_constants()
   method, however. */
void ast_id::optimize()
{
}

void ast_indexed::optimize()
{
    /* code completed */
    index = optimizer->fold_constants(index);
}



/* This convenience method is used to apply constant folding to all
   binary operations. It returns either the resulting optimized node or the
   original node if no optimization could be performed. */
ast_expression *ast_optimizer::fold_constants(ast_expression *node)
{
    /* code completed */
    if(is_binop(node)){
        ast_binaryoperation *binoper = node->get_ast_binaryoperation();
        ast_expression *left_expr = binoper->left;
        ast_expression *right_expr = binoper->right;

        switch(node->tag){
            case AST_ADD:
                if (left_expr->tag == AST_INTEGER && right_expr->tag == AST_INTEGER)
                    return new ast_integer(left_expr->pos, left_expr->get_ast_integer()->value + right_expr->get_ast_integer()->value);
                else if (left_expr->tag == AST_REAL && right_expr->tag == AST_REAL)
                    return new ast_real(left_expr->pos, left_expr->get_ast_real()->value + right_expr->get_ast_real()->value);
                break;
            case AST_AND:
                if (left_expr->tag == AST_INTEGER && right_expr->tag == AST_INTEGER)
                    return new ast_integer(left_expr->pos, left_expr->get_ast_integer()->value && right_expr->get_ast_integer()->value);
                break;
            case AST_DIVIDE:
                if (left_expr->tag == AST_REAL && right_expr->tag == AST_REAL)
                    return new ast_real(left_expr->pos, left_expr->get_ast_real()->value / right_expr->get_ast_real()->value);
                break;
            case AST_IDIV:
                if (left_expr->tag == AST_INTEGER && right_expr->tag == AST_INTEGER)
                    return new ast_integer(left_expr->pos, left_expr->get_ast_integer()->value / right_expr->get_ast_integer()->value);
                break;
            case AST_MOD:
                if (left_expr->tag == AST_INTEGER && right_expr->tag == AST_INTEGER)
                    return new ast_integer(left_expr->pos, left_expr->get_ast_integer()->value % right_expr->get_ast_integer()->value);
                break;
            case AST_MULT:
                if (left_expr->tag == AST_INTEGER && right_expr->tag == AST_INTEGER)
                    return new ast_integer(left_expr->pos, left_expr->get_ast_integer()->value * right_expr->get_ast_integer()->value);
                else if (left_expr->tag == AST_REAL && right_expr->tag == AST_REAL)
                    return new ast_real(left_expr->pos, left_expr->get_ast_real()->value * right_expr->get_ast_real()->value);
                break;    
            case AST_OR:
                if (left_expr->tag == AST_INTEGER && right_expr->tag == AST_INTEGER)
                    return new ast_integer(left_expr->pos, left_expr->get_ast_integer()->value || right_expr->get_ast_integer()->value);
                break;
            case AST_SUB:
                if (left_expr->tag == AST_INTEGER && right_expr->tag == AST_INTEGER)
                    return new ast_integer(left_expr->pos, left_expr->get_ast_integer()->value - right_expr->get_ast_integer()->value);
                else if (left_expr->tag == AST_REAL && right_expr->tag == AST_REAL)
                    return new ast_real(left_expr->pos, left_expr->get_ast_real()->value - right_expr->get_ast_real()->value);
            default:
                break;
        }
    }

    return node;
}

/* own function definition for binaryoperation to reduce code redundant code */
void ast_optimizer::optimize_binop(ast_binaryoperation* binop_node){
    binop_node->right->optimize();
    binop_node->left->optimize();   
    binop_node->right = optimizer->fold_constants(binop_node->right);
    binop_node->left = optimizer->fold_constants(binop_node->left);
}

/* own function definition for binaryrelation to reduce code redundant code */
void ast_optimizer::optimize_binrel(ast_binaryrelation* binrel_node){
    binrel_node->right->optimize();
    binrel_node->left->optimize();   
    binrel_node->right = optimizer->fold_constants(binrel_node->right);
    binrel_node->left = optimizer->fold_constants(binrel_node->left);
}

/* All the binary operations should already have been detected in their parent
   nodes, so we don't need to do anything at all here. */
void ast_add::optimize()
{
    /* code completed */
    optimizer->optimize_binop(this);
}

void ast_sub::optimize()
{
    /* code completed */
    optimizer->optimize_binop(this);
}

void ast_mult::optimize()
{
    /* code completed */
    optimizer->optimize_binop(this);
}

void ast_divide::optimize()
{
    /* code completed */
    optimizer->optimize_binop(this);
}

void ast_or::optimize()
{
    /* code completed */
    optimizer->optimize_binop(this);
}

void ast_and::optimize()
{
    /* code completed */
    optimizer->optimize_binop(this);
}

void ast_idiv::optimize()
{
    /* code completed */
    optimizer->optimize_binop(this);
}

void ast_mod::optimize()
{
    /* code completed */
    optimizer->optimize_binop(this);
}



/* We can apply constant folding to binary relations as well. */
void ast_equal::optimize()
{
    /* code completed */
    optimizer->optimize_binrel(this);
}

void ast_notequal::optimize()
{
    /* code completed */
    optimizer->optimize_binrel(this);
}

void ast_lessthan::optimize()
{
    /* code completed */
    optimizer->optimize_binrel(this);
}

void ast_greaterthan::optimize()
{
    /* code completed */
    optimizer->optimize_binrel(this);
}



/*** The various classes derived from ast_statement. ***/

void ast_procedurecall::optimize()
{
    /* code completed */
    if(parameter_list != NULL)
        parameter_list->optimize();
}


void ast_assign::optimize()
{
    /* code completed */
    lhs->optimize();
    rhs->optimize();
    rhs = optimizer->fold_constants(rhs);
}


void ast_while::optimize()
{
    /* code completed  */
    condition->optimize();
    condition = optimizer->fold_constants(condition);
    if (body != NULL) {
        body->optimize();
    }
}


void ast_if::optimize()
{
    /* code completed */
    condition->optimize();
    condition = optimizer->fold_constants(condition);
    if (body != NULL) {
        body->optimize();
    }
    if (elsif_list != NULL) {
        elsif_list->optimize();
    }
    if (else_body != NULL) {
        else_body->optimize();
    }

}


void ast_return::optimize()
{
    /* code completed */
    if(value != NULL){
        value->optimize();
        value = optimizer->fold_constants(value);
    }
}


void ast_functioncall::optimize()
{
    /* code completed */
    if(parameter_list != NULL)
        parameter_list ->optimize();
}

void ast_uminus::optimize()
{
    /* code completed */
    expr->optimize();
    expr = optimizer->fold_constants(expr);
}

void ast_not::optimize()
{
    /* code completed*/
    expr->optimize();
    expr = optimizer->fold_constants(expr);
}


void ast_elsif::optimize()
{
    /* code completed */
    condition->optimize();
    condition = optimizer->fold_constants(condition);
    if (body != NULL) {
        body->optimize();
    }
}



void ast_integer::optimize()
{
    /* code completed */
    // nothing can be done here
}

void ast_real::optimize()
{
    /* code completed */
    // nothing can be done here
}

/* Note: See the comment in fold_constants() about casts and folding. */
void ast_cast::optimize()
{
    /* code completed */
    expr->optimize();
    expr = optimizer->fold_constants(expr);
}



void ast_procedurehead::optimize()
{
    fatal("Trying to call ast_procedurehead::optimize()");
}


void ast_functionhead::optimize()
{
    fatal("Trying to call ast_functionhead::optimize()");
}
