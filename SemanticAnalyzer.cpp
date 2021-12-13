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
static bool is_sym_dec(const string& sym, bool is_search_for_func) {
    for (auto cur_tab = tables_stack.rbegin(); cur_tab != tables_stack.rend(); ++cur_tab) {
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
    if (strlen(yytext) == 0) {
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
    for (const auto& cur_param : params.formals) {
        for (const auto& next_param : params.formals) {
            if (&cur_param == &next_param)
                continue;
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
    if (num_of_loops > 0)
        return;
    if (type.actual_type == "continue") {
        errorUnexpectedContinue(yylineno);
        exit(0);
    } else if (type.actual_type == "break") {
        errorUnexpectedBreak(yylineno);
        exit(0);
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
Statement::Statement() {
    // Check if current function is void.
    for (auto cur_tab = tables_stack.rbegin(); cur_tab != tables_stack.rend(); ++cur_tab) {
        for (const auto& row : cur_tab->rows) {
            if (row.is_func && row.name == current_function_name) {
                if (row.type[0] != "VOID") {
                    errorMismatch(yylineno);
                    exit(0);
                }
                return;
            }
        }
    }
}

/* ************************************************ */
Statement::Statement(const Exp& exp) {
    for (auto cur_tab = tables_stack.rbegin(); cur_tab != tables_stack.rend(); ++cur_tab) {
        for (const auto& row : cur_tab->rows) {
            if (!row.is_func || row.name != current_function_name) 
                continue;
            if (row.type[0] == "VOID") {
                // Return something from void func.
                errorMismatch(yylineno);
                exit(0);
            }
            if (row.type[0] != exp.type) {
                if (row.type[0] == "INT" && exp.type == "BYTE" || row.type[0] == "BYTE" && exp.type == "INT") 
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

/* ************************************************ */
Statement::Statement(const BaseType& id, const Exp& exp) {
    // Assignment to undeclared var.
    if (!is_sym_dec(id.actual_type, false)) {
        errorUndef(yylineno, id.actual_type);
        exit(0);
    }

    for (auto cur_tab = tables_stack.rbegin(); cur_tab != tables_stack.rend(); ++cur_tab) {
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

// TODO - check for definitions of parameters.
Call::Call(const BaseType& id, const ExpList& param_list) {
    for (auto& table : tables_stack) {
        for (auto& row : table.rows) {
            if (row.name != id.actual_type)
                continue;
            // Found variable with a func name.
            if (!row.is_func) {
                errorUndefFunc(yylineno, id.actual_type);
                exit(0);
            }
            // Incorrect number of parameters.
            if (row.type.size() != param_list.list.size() + 1) {
                row.type.erase(row.type.begin()); // Remove return type.
                errorPrototypeMismatch(yylineno, id.actual_type, row.type);
                exit(0);
            }
            for (int i = 0; i < param_list.list.size(); i++) {
                if (param_list.list[i].type == row.type[i + 1])
                    continue;
                if (param_list.list[i].type == "BYTE" && row.type[i] == "INT")
                    continue;
                row.type.erase(row.type.begin());
                errorPrototypeMismatch(yylineno, id.actual_type, row.type);
                exit(0);
            }
            return; // Everything is fine.
        }
    }
    errorUndefFunc(yylineno, id.actual_type);
    exit(0);
}