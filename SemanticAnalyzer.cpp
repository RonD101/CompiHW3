//
// Created by user on 12/12/2021.
//

#include "SemanticAnalyzer.h"
#include <cstring>

using namespace output;

vector<SymbolTable> tables_stack;
vector<int> offset_stack;
string current_function_name;
int num_of_loops;

/* ************************************************ */
void loop_entered() {
    num_of_loops++;
}

/* ************************************************ */
void loop_exited() {
    num_of_loops--;
}

/* ************************************************ */
void create_new_scope() {
    SymbolTable new_table;
    tables_stack.push_back(new_table);
    offset_stack.push_back(offset_stack.back());
}

/* ************************************************ */
void destroy_current_scope() {
    endScope();
    SymbolTable& closed_scope = tables_stack.back();
    for (auto& row : closed_scope.rows) {
        if (!row.is_func) 
            printID(row.name, row.offset, row.types[0]);
        else {
            const string& return_type = row.types[0];
            row.types.erase(row.types.begin());
            printID(row.name, row.offset, makeFunctionType(return_type, row.types));
        }
    }
    tables_stack.pop_back();
    offset_stack.pop_back();
}

/* ************************************************ */
void check_for_main_correctness() {
    SymbolTable global_scope = tables_stack.front();
    int num_of_main = 0;
    for (const auto& row : global_scope.rows) {
        if (row.is_func && row.name == "main") {
            if (row.types[0] == "VOID" && row.types.size() == 1)
                num_of_main++;
            else {
                // Found a function called main with non void return type or params.
                num_of_main = -1;
                break;
            }
        }
    }
    destroy_current_scope();
    if (num_of_main != 1) {
        errorMainMissing();
        exit(0);
    }
}

/* ************************************************ */
static bool is_sym_dec(const string& sym, bool is_search_for_func) {
    for (auto cur_tab = tables_stack.rbegin(); cur_tab != tables_stack.rend(); ++cur_tab) {
        for (const auto& row : cur_tab->rows) {
            if (row.name == sym && row.is_func == is_search_for_func)
                return true;
        }
    }
    return false;
}

/* Program : Funcs */
Program::Program() : BaseType("Program") {
    SymbolTable new_table;
    SymbolEntry print_func("print", {"VOID", "STRING"}, 0, true, false, { false });
    SymbolEntry printi_func("printi", {"VOID", "INT"}, 0, true, false, { false });
    new_table.rows.push_back(print_func);
    new_table.rows.push_back(printi_func);
    tables_stack.push_back(new_table);
    offset_stack.push_back(0);
}

/* Funcs : Epsilon */
Funcs::Funcs() {
    if (strlen(yytext) == 0) {
        errorSyn(yylineno);
        exit(0);
    }
}

/* FuncDecl : RetType ID LPAREN Formals RPAREN LBRACE Statements RBRACE */
FuncDecl::FuncDecl(const shared_ptr<RetType>& return_type, const shared_ptr<RetType>& func_name, const shared_ptr<Formals>& params) {
    // Redecleration of function.
    if (is_sym_dec(func_name->token_value, true)) {
        errorDef(yylineno, func_name->token_value);
        exit(0);
    }
    for (const auto& cur_param : params->formals) {
        for (const auto& next_param : params->formals) {
            if (&cur_param == &next_param)
                continue;
            // Two parameters with the same name.
            if (cur_param.token_value == next_param.token_value) {
                errorDef(yylineno, cur_param.token_value);
                exit(0);
            }
        }
    }
    param_types.push_back(return_type->token_value); // return type first.
    const_indicator.push_back(false); // Dummy const for return type.
    for (const auto& param : params->formals) {
        param_types.push_back(param.param_type);
        const_indicator.push_back(param.is_param_const);
    }

    SymbolEntry new_func(func_name->token_value, param_types, 0, true, false, const_indicator);
    tables_stack.back().rows.push_back(new_func);
    current_function_name = func_name->token_value;
}

/* Statement : BREAK SC */
/* Statement : CONTINUE SC */
Statement::Statement(const Break_Cont& type) {
    if (num_of_loops > 0)
        return;
    if (type == Break_Cont::CONTINUE) {
        errorUnexpectedContinue(yylineno);
        exit(0);
    } else if (type == Break_Cont::BREAK) {
        errorUnexpectedBreak(yylineno);
        exit(0);
    }
}

/* Statement : IF LPAREN Exp RPAREN Statement */
/* Statement : IF LPAREN Exp RPAREN Statement ELSE Statement */
/* Statement : WHILE LPAREN Exp RPAREN Statement */
Statement::Statement(const string& type, const shared_ptr<Exp>& exp) {
    // Expression inside if/while statement must be boolean.
    if (exp->type != "BOOL") {
        errorMismatch(yylineno);
        exit(0);
    }
}

/* Statement : RETURN SC */
Statement::Statement() {
    // Check if current function is void.
    for (auto cur_tab = tables_stack.rbegin(); cur_tab != tables_stack.rend(); ++cur_tab) {
        for (const auto& row : cur_tab->rows) {
            if (row.is_func && row.name == current_function_name) {
                if (row.types[0] != "VOID") {
                    errorMismatch(yylineno);
                    exit(0);
                }
                return;
            }
        }
    }
}

/* Statement : RETURN Exp SC */
Statement::Statement(const shared_ptr<Exp>& exp) {
    for (auto cur_tab = tables_stack.rbegin(); cur_tab != tables_stack.rend(); ++cur_tab) {
        for (const auto& row : cur_tab->rows) {
            if (!row.is_func || row.name != current_function_name) 
                continue;
            if (row.types[0] == "VOID") {
                // Return something from void func.
                errorMismatch(yylineno);
                exit(0);
            }
            if (row.types[0] != exp->type) {
                if (row.types[0] == "INT" && exp->type == "BYTE" || row.types[0] == "BYTE" && exp->type == "INT") 
                    return;
                else {
                    // Return int from bool func.
                    errorMismatch(yylineno);
                    exit(0);
                }
            }
            return;
        }
    }
}

/* Statement : ID ASSIGN Exp SC */
Statement::Statement(const shared_ptr<BaseType>& id, const shared_ptr<Exp>& exp) {
    // Assignment to undeclared var.
    if (!is_sym_dec(id->token_value, false)) {
        errorUndef(yylineno, id->token_value);
        exit(0);
    }

    for (auto cur_tab = tables_stack.rbegin(); cur_tab != tables_stack.rend(); ++cur_tab) {
        for (const auto& row : cur_tab->rows) {
            if (!row.is_func && row.name == id->token_value) {
                // We found the desired variable
                if (row.is_const) {
                    errorConstMismatch(yylineno);
                    exit(0);
                }
                if (row.types[0] == exp->type)
                    return;
                if (row.types[0] == "INT" && exp->type == "BYTE")
                    return;
                errorMismatch(yylineno);
                exit(0);
            }
        }
    }
}

/* Statement : TypeAnnotation Type ID ASSIGN Exp SC */
Statement::Statement(const shared_ptr<Type>& type, const shared_ptr<BaseType>& id, const shared_ptr<Exp>& exp, const shared_ptr<TypeAnnotation>& const_anno) {
    // Symbol redefinition.
    if (is_sym_dec(id->token_value, false)) {
        errorDef(yylineno, id->token_value);
        exit(0);
    }

    if (type->token_value == exp->type || (type->token_value == "INT" && exp->type == "BYTE")) {
        int new_offset = offset_stack.back() + 1;
        vector<string> varType = { type->token_value };
        SymbolEntry new_sym(id->token_value, varType, new_offset, false, const_anno->is_const);
        tables_stack.back().rows.push_back(new_sym);
    } else {
        errorMismatch(yylineno);
        exit(0);
    }
}

/* Statement : TypeAnnotation Type ID SC */
Statement::Statement(const shared_ptr<Type>& type, const shared_ptr<BaseType>& id, const shared_ptr<TypeAnnotation>& const_anno) {
    // Symbol redefinition.
    if (is_sym_dec(id->token_value, false)) {
        errorDef(yylineno, id->token_value);
        exit(0);
    }
    if (const_anno->is_const) {
        errorConstDef(yylineno);
        exit(0);
    }
    int new_offset = offset_stack.back() + 1;
    vector<string> varType = { type->token_value };
    SymbolEntry new_sym(id->token_value, varType, new_offset, false, false);
    tables_stack.back().rows.push_back(new_sym);
}

/* Call : ID LPAREN ExpList RPAREN */
Call::Call(const shared_ptr<BaseType>& id, const shared_ptr<ExpList>& param_list) {
    for (auto& table : tables_stack) {
        for (auto& row : table.rows) {
            if (row.name != id->token_value)
                continue;
            // Found variable with a func name.
            if (!row.is_func) {
                errorUndefFunc(yylineno, id->token_value);
                exit(0);
            }
            // Incorrect number of parameters.
            if (row.types.size() != param_list->list.size() + 1) {
                row.types.erase(row.types.begin()); // Remove return type.
                errorPrototypeMismatch(yylineno, id->token_value, row.types);
                exit(0);
            }
            for (int i = 0; i < param_list->list.size(); i++) {
                if (param_list->list[i].type == row.types[i + 1])
                    continue;
                if (param_list->list[i].type == "BYTE" && row.types[i] == "INT")
                    continue;
                row.types.erase(row.types.begin());
                errorPrototypeMismatch(yylineno, id->token_value, row.types);
                exit(0);
            }
            return; // Everything is fine.
        }
    }
    errorUndefFunc(yylineno, id->token_value);
    exit(0);
}

/* Call : ID LPAREN RPAREN */
Call::Call(const shared_ptr<BaseType>& id) {
    for (auto& table : tables_stack) {
        for (auto& row : table.rows) {
            if (row.name != id->token_value)
                continue;
            // Found variable with a func name.
            if (!row.is_func) {
                errorUndefFunc(yylineno, id->token_value);
                exit(0);
            }
            // Incorrect number of parameters.
            if (row.types.size() != 1) {
                row.types.erase(row.types.begin()); // Remove return type.
                errorPrototypeMismatch(yylineno, id->token_value, row.types);
                exit(0);
            }
            return; // Everything is fine.
        }
    }
    errorUndefFunc(yylineno, id->token_value);
    exit(0);
}

/* Exp : Call */
Exp::Exp(const shared_ptr<Call>& call) {
    token_value = call->token_value;
    type = call->token_value;
}

/* Exp : ID */
Exp::Exp(const shared_ptr<BaseType>& term) {
    if (!is_sym_dec(term->token_value, false)) {
        errorUndef(yylineno, term->token_value);
        exit(0);
    }

    for (const auto& table : tables_stack) {
        for (const auto& row : table.rows) {
            if (row.name == term->token_value) {
                token_value = term->token_value;
                type = row.types[0];
                return;
            }
        }
    }
}

/* Exp : NOT Exp */
Exp::Exp(bool not_mark, const shared_ptr<Exp>& exp) {
    // Not performed on something wich is not boolean.
    if (exp->type != "BOOL") {
        errorMismatch(yylineno);
        exit(0);
    }
    res_type = !res_type;
    type = "BOOL";
}

/*  Exp : NUM, NUM B, STRING, TRUE, FALSE */
Exp::Exp(const shared_ptr<BaseType>& term, const string& rhs) : BaseType(term.token_value) {
    if (rhs == "BYTE") {
        if (stoi(term->token_value) > 255) {
            errorByteTooLarge(yylineno, term->token_value);
            exit(0);
        }
    }
    else if (rhs == "BOOL") {
        if (term->token_value == "true")
            res_type = true;
        else
            res_type = false;
    }
    type = rhs;
}

/* Exp : LPAREN Exp RPAREN */
Exp::Exp(const shared_ptr<Exp>& exp) {
    token_value = exp->token_value;
    type = exp->type;
    res_type = exp->res_type;
}

/* Exp : Exp RELOP/BINOP Exp */
Exp::Exp(const shared_ptr<Exp>& first, const OP_TYPE& op, const shared_ptr<Exp>& second) {
    // Int\Bool vs Int\Bool
    if ((first->type == "INT" || first->type == "BYTE") && (second->type == "INT" || second->type == "BYTE")) {
        if (op == OP_TYPE::EQUALITY || op == OP_TYPE::RELATION) 
            type = "BOOL";
        else if (op == OP_TYPE::BINADD || op == OP_TYPE::BINMUL) {
            if (first->type == "INT" || second->type == "INT")
                type = "INT";
            else 
                type = "BYTE";
        }
    // Bool vs Bool
    } else if (first->type == "BOOL" && second->type == "BOOL") {
        type = "BOOL";
        if (op == OP_TYPE::AND) {
            if (first->res_type && second->res_type)
                res_type = true;
            else
                res_type = false;
        }
        else if (op == OP_TYPE::OR) {
            if (first->res_type || second->res_type)
                res_type = true;
            else
                res_type = false;
        }
        else {
            errorMismatch(yylineno);
            exit(0);
        }
    }
    else {
        errorMismatch(yylineno);
        exit(0);
    }
}

// Exp : LPAREN Type RPAREN Exp
Exp::Exp(const shared_ptr<Type>& type, const shared_ptr<Exp>& exp) {
    if (type->token_value == "BOOL" || type->token_value == "INT") {
        if (exp->type == "BOOL" || exp->type == "INT")
            return;
        errorMismatch(yylineno);
        exit(0);
    }
    errorMismatch(yylineno);
    exit(0);
}
