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
            printID(row.name, row.offset, row.type[0]);
        else {
            const string& return_type = row.type[0];
            row.type.erase(row.type.begin());
            printID(row.name, row.offset, makeFunctionType(return_type, row.type));
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
            if (row.type[0] == "VOID" && row.type.size() == 1)
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

/* ************************************************ */
Program::Program() : BaseType("Program") {
    SymbolTable new_table;
    SymbolEntry print_func("print", {"VOID", "STRING"}, 0, true);
    SymbolEntry printi_func("printi", {"VOID", "INT"}, 0, true);
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
FuncDecl::FuncDecl(const RetType& return_type, const BaseType& func_name, const Formals& params) {
    // Redecleration of function.
    if (is_sym_dec(func_name.token_value, true)) {
        errorDef(yylineno, func_name.token_value);
        exit(0);
    }
    for (const auto& cur_param : params.formals) {
        for (const auto& next_param : params.formals) {
            if (&cur_param == &next_param)
                continue;
            // Two parameters with the same name.
            if (cur_param.token_value == next_param.token_value) {
                errorDef(yylineno, cur_param.token_value);
                exit(0);
            }
        }
    }
    type.push_back(return_type.token_value); // return type first.
    for (const auto& param : params.formals) 
        type.push_back(param.type);

    SymbolEntry new_func(func_name.token_value, type, 0, true);
    tables_stack.back().rows.push_back(new_func);
    current_function_name = func_name.token_value;
}

/* ************************************************ */
Statement::Statement(const BaseType& type) {
    if (num_of_loops > 0)
        return;
    if (type.token_value == "continue") {
        errorUnexpectedContinue(yylineno);
        exit(0);
    } else if (type.token_value == "break") {
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
    if (!is_sym_dec(id.token_value, false)) {
        errorUndef(yylineno, id.token_value);
        exit(0);
    }

    for (auto cur_tab = tables_stack.rbegin(); cur_tab != tables_stack.rend(); ++cur_tab) {
        for (const auto& row : cur_tab->rows) {
            if (!row.is_func && row.name == id.token_value) {
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
    if (is_sym_dec(id.token_value, false)) {
        errorDef(yylineno, id.token_value);
        exit(0);
    }

    if (type.token_value == exp.type || (type.token_value == "INT" && exp.type == "BYTE")) {
        int new_offset = offset_stack.back() + 1;
        vector<string> varType = { type.token_value };
        SymbolEntry new_sym(id.token_value, varType, new_offset, false);
        tables_stack.back().rows.push_back(new_sym);
    } else {
        errorMismatch(yylineno);
        exit(0);
    }
}

/* ************************************************ */
Statement::Statement(const Type& type, const BaseType& id) {
    // Symbol redefinition.
    if (is_sym_dec(id.token_value, false)) {
        errorDef(yylineno, id.token_value);
        exit(0);
    }

    int new_offset = offset_stack.back() + 1;
    vector<string> varType = { type.token_value };
    SymbolEntry new_sym(id.token_value, varType, new_offset, false);
    tables_stack.back().rows.push_back(new_sym);
}

// TODO - check for definitions of parameters.
Call::Call(const BaseType& id, const ExpList& param_list) {
    for (auto& table : tables_stack) {
        for (auto& row : table.rows) {
            if (row.name != id.token_value)
                continue;
            // Found variable with a func name.
            if (!row.is_func) {
                errorUndefFunc(yylineno, id.token_value);
                exit(0);
            }
            // Incorrect number of parameters.
            if (row.type.size() != param_list.list.size() + 1) {
                row.type.erase(row.type.begin()); // Remove return type.
                errorPrototypeMismatch(yylineno, id.token_value, row.type);
                exit(0);
            }
            for (int i = 0; i < param_list.list.size(); i++) {
                if (param_list.list[i].type == row.type[i + 1])
                    continue;
                if (param_list.list[i].type == "BYTE" && row.type[i] == "INT")
                    continue;
                row.type.erase(row.type.begin());
                errorPrototypeMismatch(yylineno, id.token_value, row.type);
                exit(0);
            }
            return; // Everything is fine.
        }
    }
    errorUndefFunc(yylineno, id.token_value);
    exit(0);
}

/* ************************************************ */
Call::Call(const BaseType& id) {
    for (auto& table : tables_stack) {
        for (auto& row : table.rows) {
            if (row.name != id.token_value)
                continue;
            // Found variable with a func name.
            if (!row.is_func) {
                errorUndefFunc(yylineno, id.token_value);
                exit(0);
            }
            // Incorrect number of parameters.
            if (row.type.size() != 1) {
                row.type.erase(row.type.begin()); // Remove return type.
                errorPrototypeMismatch(yylineno, id.token_value, row.type);
                exit(0);
            }
            return; // Everything is fine.
        }
    }
    errorUndefFunc(yylineno, id.token_value);
    exit(0);
}

/* ************************************************ */
Exp::Exp(const Call& call) {
    token_value = call.token_value;
    type = call.token_value;
}

/* ************************************************ */
Exp::Exp(const BaseType& term) {
    if (!is_sym_dec(term.token_value, false)) {
        errorUndef(yylineno, term.token_value);
        exit(0);
    }

    for (const auto& table : tables_stack) {
        for (const auto& row : table.rows) {
            if (row.name == term.token_value) {
                token_value = term.token_value;
                type = row.type[0];
                return;
            }
        }
    }
}

/* ************************************************ */
Exp::Exp(const BaseType& not_mark, const Exp& exp) {
    // Not performed on something wich is not boolean.
    if (exp.type != "BOOL") {
        errorMismatch(yylineno);
        exit(0);
    }
    res_type = !res_type;
    type = "BOOL";
}

/* ************************************************ */
Exp::Exp(const BaseType& term, const string& rhs) : BaseType(term.token_value) {
    if (rhs == "BYTE") {
        if (stoi(term.token_value) > 255) {
            errorByteTooLarge(yylineno, term.token_value);
            exit(0);
        }
    }
    else if (rhs == "BOOL") {
        if (term.token_value == "true")
            res_type = true;
        else
            res_type = false;
    }
    type = rhs;
}

/* ************************************************ */
Exp::Exp(const Exp& exp) {
    token_value = exp.token_value;
    type = exp.type;
    res_type = exp.res_type;
}

/* ************************************************ */
Exp::Exp(const Exp& first, const BaseType& operation, const Exp& second, const OP_TYPE& rhs) {
    // Int\Bool vs Int\Bool
    if ((first.type == "INT" || first.type == "BYTE") && (second.type == "INT" || second.type == "BYTE")) {
        if (rhs == OP_TYPE::EQUALITY || rhs == OP_TYPE::RELATION) 
            type = "BOOL";
        else if (rhs == OP_TYPE::BINADD || rhs == OP_TYPE::BINMUL) {
            if (first.type == "INT" || second.type == "INT")
                type = "INT";
            else 
                type = "BYTE";
        }
    // Bool vs Bool
    } else if (first.type == "BOOL" && second.type == "BOOL") {
        type = "BOOL";
        if (rhs == OP_TYPE::AND) {
            if (first.res_type && second.res_type)
                res_type = true;
            else
                res_type = false;
        }
        else if (rhs == OP_TYPE::OR) {
            if (first.res_type || second.res_type)
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
