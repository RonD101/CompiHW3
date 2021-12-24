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
    if (offset_stack.empty())
        offset_stack.push_back(0);
    else 
        offset_stack.push_back(offset_stack.back());
}

/* ************************************************ */
void create_global_scope() {
    SymbolTable new_table;
    SymbolEntry print_func("print", {"VOID", "STRING"}, 0, true, false, { false });
    SymbolEntry printi_func("printi", {"VOID", "INT"}, 0, true, false, { false });
    new_table.rows.push_back(print_func);
    new_table.rows.push_back(printi_func);
    tables_stack.push_back(new_table);
    offset_stack.push_back(0);
}

/* ************************************************ */
void destroy_current_scope() {
    endScope();
    SymbolTable& closed_scope = tables_stack.back();
    for (auto& row : closed_scope.rows) {
        if (!row.is_func) 
            printID(row.name, row.offset, row.types[0]);
        else {
            const string return_type = row.types[0];
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
    if (num_of_main != 1) {
        errorMainMissing();
        exit(0);
    }
    destroy_current_scope(); //
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
   
}

/* Funcs : Epsilon */
Funcs::Funcs() {
    /*if (strlen(yytext) == 0) {
        errorSyn(yylineno);
        exit(0);
    }*/
}

/* FuncDecl : RetType ID LPAREN Formals RPAREN LBRACE Statements RBRACE */
FuncDecl::FuncDecl(RetType* return_type, IDWrap* func_name, Formals* params) {
    // Redecleration of function.
    if (is_sym_dec(func_name->ID, true)) {
        errorDef(func_name->lineNo, func_name->ID);
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
    SymbolEntry new_func(func_name->ID, param_types, 0, true, false, const_indicator);
    tables_stack.front().rows.push_back(new_func);
    for (const auto& param : params->formals) {
        int new_offset = --offset_stack.back();
        vector<string> varType = { param.param_type };
        tables_stack.back().rows.push_back(SymbolEntry(param.token_value, varType, new_offset, false, param.is_param_const));
    }
    offset_stack.back() = 0;
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
Statement::Statement(const string& type, Exp* exp) {
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
Statement::Statement(Exp* exp) {
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
                if (row.types[0] == "INT" && exp->type == "BYTE")
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
Statement::Statement(IDWrap* id, Exp* exp) {
    // Assignment to undeclared var.
    if (!is_sym_dec(id->ID, false)) {
        errorUndef(id->lineNo, id->ID);
        exit(0);
    }

    for (auto cur_tab = tables_stack.rbegin(); cur_tab != tables_stack.rend(); ++cur_tab) {
        for (const auto& row : cur_tab->rows) {
            if (!row.is_func && row.name == id->ID) {
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
Statement::Statement(Type* type, IDWrap* id, Exp* exp, TypeAnnotation* const_anno) {
    // Symbol redefinition.
    if (is_sym_dec(id->ID, true) || is_sym_dec(id->ID, false)) {
        errorDef(id->lineNo, id->ID);
        exit(0);
    }
    if (type->token_value == exp->type || (type->token_value == "INT" && exp->type == "BYTE")) {
        int new_offset = offset_stack.back()++;
        vector<string> varType = { type->token_value };
        SymbolEntry new_sym(id->ID, varType, new_offset, false, const_anno->is_const);
        tables_stack.back().rows.push_back(new_sym);
    } else {
        errorMismatch(yylineno);
        exit(0);
    }
}

/* Statement : TypeAnnotation Type ID SC */
Statement::Statement(Type* type, IDWrap* id, TypeAnnotation* const_anno) {
    // Symbol redefinition.
    if (is_sym_dec(id->ID, true) || is_sym_dec(id->ID, false)) {
        errorDef(id->lineNo, id->ID);
        exit(0);
    }
    if (const_anno->is_const) {
        errorConstDef(yylineno);
        exit(0);
    }
    int new_offset = offset_stack.back()++;
    vector<string> varType = { type->token_value };
    SymbolEntry new_sym(id->ID, varType, new_offset, false, false);
    tables_stack.back().rows.push_back(new_sym);
}

/* Call : ID LPAREN ExpList RPAREN */
Call::Call(IDWrap* id, ExpList* param_list) {
    for (auto& table : tables_stack) {
        for (auto& row : table.rows) {
            if (row.name != id->ID)
                continue;
            // Found variable with a func name.
            if (!row.is_func) {
                errorUndefFunc(id->lineNo, id->ID);
                exit(0);
            }
            // Incorrect number of parameters.
            if (row.types.size() != param_list->list.size() + 1) {
                row.types.erase(row.types.begin()); // Remove return type.
                errorPrototypeMismatch(id->lineNo, id->ID, row.types);
                exit(0);
            }
            for (int i = 0; i < param_list->list.size(); i++) {
                if (param_list->list[i]->type == row.types[i + 1])
                    continue;
                if (param_list->list[i]->type == "BYTE" && row.types[i + 1] == "INT")
                    continue;
                row.types.erase(row.types.begin());
                errorPrototypeMismatch(id->lineNo, id->ID, row.types);
                exit(0);
            }
            ret_type_of_called_func = row.types[0];
            return; // Everything is fine.
        }
    }
    errorUndefFunc(id->lineNo, id->ID);
    exit(0);
}

/* Call : ID LPAREN RPAREN */
Call::Call(IDWrap* id) {
    for (auto& table : tables_stack) {
        for (auto& row : table.rows) {
            if (row.name != id->ID)
                continue;
            // Found variable with a func name.
            if (!row.is_func) {
                errorUndefFunc(id->lineNo, id->ID);
                exit(0);
            }
            // Incorrect number of parameters.
            if (row.types.size() != 1) {
                row.types.erase(row.types.begin()); // Remove return type.
                errorPrototypeMismatch(id->lineNo, id->ID, row.types);
                exit(0);
            }
            ret_type_of_called_func = row.types[0];
            return; // Everything is fine.
        }
    }
    errorUndefFunc(id->lineNo, id->ID);
    exit(0);
}

/* Exp : Call */
Exp::Exp(Call* call) {
    token_value = call->token_value;
    type = call->ret_type_of_called_func;
}

/* Exp : ID */
Exp::Exp(IDWrap* term) {
    if (!is_sym_dec(term->ID, false)) {
        errorUndef(term->lineNo, term->ID);
        exit(0);
    }

    for (const auto& table : tables_stack) {
        for (const auto& row : table.rows) {
            if (row.name == term->ID) {
                token_value = term->ID;
                type = row.types[0];
                return;
            }
        }
    }
}

/* Exp : NOT Exp */
Exp::Exp(bool not_mark, Exp* exp) {
    // Not performed on something wich is not boolean.
    if (exp->type != "BOOL") {
        errorMismatch(yylineno);
        exit(0);
    }
    type = "BOOL";
}

/*  Exp : NUM, NUM B, STRING, TRUE, FALSE */
Exp::Exp(BaseType* term, const string& rhs) : BaseType(term->token_value) {
    if (rhs == "BYTE") {
        if (stoi(term->token_value) > 255) {
            errorByteTooLarge(yylineno, term->token_value);
            exit(0);
        }
    }
    type = rhs;
}

/* Exp : LPAREN Exp RPAREN */
Exp::Exp(Exp* exp) {
    token_value = exp->token_value;
    type = exp->type;
}

/* Exp : Exp RELOP/BINOP Exp */
Exp::Exp(Exp* first, const OP_TYPE& op, Exp* second) {
    if (op == OP_TYPE::BINADD || op == OP_TYPE::BINMUL) {
        // Byte BINOP Byte <- Byte
        if (first->type == "BYTE" && second->type == "BYTE")
            type = "BYTE";
        // Byte\Int BINOP Byte\Int <- Int
        else if ((first->type == "INT" || first->type == "BYTE") && (second->type == "INT" || second->type == "BYTE"))
            type = "INT";
        // BINOP on non number types.
        else {
            errorMismatch(yylineno);
            exit(0);
        }
    }
    else if (op == OP_TYPE::EQUALITY || op == OP_TYPE::RELATION) {
        // RELOP on non number types.
        if ((first->type != "INT" && first->type != "BYTE") || (second->type != "INT" && second->type != "BYTE")) {
            errorMismatch(yylineno);
            exit(0);
        }
        type = "BOOL";
    }
    else if (op == OP_TYPE::AND || op == OP_TYPE::OR) {
        // AND\OR on non bool types.
        if (first->type != "BOOL" || second->type != "BOOL") {
            errorMismatch(yylineno);
            exit(0);
        }
        type = "BOOL";
    }
}

// Exp : LPAREN Type RPAREN Exp
Exp::Exp(Type* new_type, Exp* exp) {
    if (new_type->token_value == "BYTE" || new_type->token_value == "INT") {
        if (exp->type == "BYTE" || exp->type == "INT") {
            type = new_type->token_value;
            return;
        }
    }
    errorMismatch(yylineno);
    exit(0);
}
