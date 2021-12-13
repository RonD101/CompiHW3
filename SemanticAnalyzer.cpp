//
// Created by user on 12/12/2021.
//

#include "SemanticAnalyzer.h"

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
static bool is_sym_dec(const string& sym, bool is_search_for_func) {
    for (auto& cur_tab = tables_stack.rbegin(); cur_tab != tables_stack.rend(); ++cur_tab) {
        for (const auto& row : cur_tab->rows) {
            if (row.name == sym && row.is_func == is_search_for_func)
                return true;
        }
    }
    return false;
}

/* ************************************************ */
Program::Program() : BaseType("Program") {
    SymbolTable new_table;
    SymbolEntry print_func("print", {"STRING", "VOID"}, 0, true);
    SymbolEntry printi_func("printi", {"INT", "VOID"}, 0, true);
    new_table.rows.push_back(print_func);
    new_table.rows.push_back(printi_func);
    tables_stack.push_back(new_table);
    offset_stack.push_back(0);
}

/* ************************************************ */
Funcs::Funcs() {
    if (strcmp(yytext, "") != 0) {
        errorSyn(yylineno);
        exit(0);
    }
}

/* ************************************************ */
FuncDecl::FuncDecl(const RetType& return_type, const BaseType& id, const Formals& params) {

    // Redecleration of function.
    if (is_sym_dec(id.actual_type, true)) {
        errorDef(yylineno, id.actual_type);
        exit(0);
    }
    int num_of_params = params.formals.size();
    for (const auto& cur_param : params.formals) {

        // Parameter is same name as func or redeclaring parameter from existing one.
        if (is_sym_dec(cur_param.actual_type, false) || cur_param.actual_type == id.actual_type) {
            errorDef(yylineno, cur_param.actual_type);
            exit(0);
        }   
        for (const auto& next_param : params.formals) {
            // Two parameters with the same name.
            if (cur_param.actual_type == next_param.actual_type) {
                errorDef(yylineno, cur_param.actual_type);
                exit(0);
            }
        }
        
    }
    type.push_back(return_type.actual_type); // return type first.
    for (const auto& param : params.formals) 
        type.push_back(param.type);

    SymbolEntry new_func(id.actual_type, type, 0, true);
    tables_stack.back().rows.push_back(new_func);
    current_function_name = id.actual_type;
}

/* ************************************************ */
Statement::Statement(const BaseType& type) {
    if (num_of_loops == 0) {
        if (type.actual_type == "continue") {
            errorUnexpectedContinue(yylineno);
            exit(0);
        } else if (type.actual_type == "break") {
            errorUnexpectedBreak(yylineno);
            exit(0);
        }
    }
}

/* ************************************************ */
Statement::Statement(const string& type, const Exp& exp) {
    // Result of if must be bool.
    if (exp.type != "BOOL") {
        errorMismatch(yylineno);
        exit(0);
    }
}

/* ************************************************ */
Statement::Statement(const string& ret_type) {
    // Check if current function is void.
    for (auto& cur_tab = tables_stack.rbegin(); cur_tab != tables_stack.rend(); ++cur_tab) {
        for (const auto& row : cur_tab->rows) {
            if (row.is_func && row.name == current_function_name) {
                if (row.type[0] != ret_type) {
                    errorMismatch(yylineno);
                    exit(0);
                }
            }
        }
    }
}

/* ************************************************ */
Statement::Statement(const Exp& exp) {
    if (exp.type == "VOID") {
        // Return something from void func.
        errorMismatch(yylineno);
        exit(0);
    }

    for (auto& cur_tab = tables_stack.rbegin(); cur_tab != tables_stack.rend(); ++cur_tab) {
        for (const auto& row : cur_tab->rows) {
            if (!row.is_func || row.name != current_function_name) 
                continue;
            if (row.type[0] != exp.type) {
                if (row.type[0] == "INT" && exp.type == "BYTE" || row.type[0] == "BYTE" && exp.type == "INT") 
                    return;
                else {
                    errorMismatch(yylineno);
                    exit(0);
                }
            }
        }
    }
}

/* ************************************************ */
Statement::Statement(const BaseType& id, const Exp& exp) {
    // Assignment to undeclared var.
    if (!is_sym_dec(id.actual_type, false)) {
        errorUndef(yylineno, id.actual_type);
        exit(0);
    }

    for (auto& cur_tab = tables_stack.rbegin(); cur_tab != tables_stack.rend(); ++cur_tab) {
        for (const auto& row : cur_tab->rows) {
            if (!row.is_func && row.name == id.actual_type) {
                // We found the desired variable
                if (row.type[0] == exp.type)
                    return;
                if (row.type[0] == "INT" && exp.type == "BYTE")
                    return;
                errorMismatch(yylineno);
                exit(0);
            }
        }
    }
}

/* ************************************************ */
Statement::Statement(const Type& type, const BaseType& id, const Exp& exp) {
    // Symbol redefinition.
    if (is_sym_dec(id.actual_type, false)) {
        errorDef(yylineno, id.actual_type);
        exit(0);
    }

    if (type.actual_type == exp.type || (type.actual_type == "INT" && exp.type == "BYTE")) {
        int new_offset = offset_stack.back() + 1;
        vector<string> varType = { type.actual_type };
        SymbolEntry new_sym(id.actual_type, varType, new_offset, false);
        tables_stack.back().rows.push_back(new_sym);
    } else {
        errorMismatch(yylineno);
        exit(0);
    }
}

/* ************************************************ */
Statement::Statement(const Type& type, const BaseType& id) {
    // Symbol redefinition.
    if (is_sym_dec(id.actual_type, false)) {
        errorDef(yylineno, id.actual_type);
        exit(0);
    }

    int new_offset = offset_stack.back() + 1;
    vector<string> varType = { type.actual_type };
    SymbolEntry new_sym(id.actual_type, varType, new_offset, false);
    tables_stack.back().rows.push_back(new_sym);
}
