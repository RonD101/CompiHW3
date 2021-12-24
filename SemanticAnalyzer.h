//
// Created by Renen on 12/12/2021.
//
#ifndef COMPIHW3_SEMANTICANALYZER_H
#define COMPIHW3_SEMANTICANALYZER_H

#include <vector>
#include <string>
#include "hw3_output.hpp"
#include <iostream>

using std::vector;
using std::string;

void loop_entered();
void loop_exited();
void create_new_scope();
void create_global_scope();
void destroy_current_scope();
void check_for_main_correctness();

enum class OP_TYPE {
    EQUALITY,
    RELATION,
    BINADD, 
    BINMUL,
    AND,
    OR
};

enum class Break_Cont {
    BREAK,
    CONTINUE
};

class BaseType {
public:
    BaseType() { token_value = ""; }
    string token_value;
    explicit BaseType(const string& str) {
        if (str == "void")
            token_value = "VOID";
        else if (str == "bool")
            token_value = "BOOL";
        else if (str == "int")
            token_value = "INT";
        else if (str == "byte")
            token_value = "BYTE";
        else
            token_value = str;
    }
    virtual ~BaseType() = default;
};

#define YYSTYPE BaseType*

class SymbolEntry {
public:
    string name;
    vector<string> types;
    vector<bool> cost_indicator;
    int offset;
    bool is_func;
    bool is_const;
    SymbolEntry(const string& name, const vector<string>& types, int offset, bool is_func, bool is_const, const vector<bool>& const_indicator = {}) :
        name(name), types(types), cost_indicator(cost_indicator), offset(offset), is_func(is_func), is_const(is_const) {}
};

class SymbolTable {
public:
    vector<SymbolEntry> rows;
};

class Type : public BaseType {
public:
    explicit Type(BaseType* type) : BaseType(type->token_value) {}
};

class Program : public BaseType {
public:
    // FUNCS
    Program();
};

class Funcs : public BaseType {
public:
    // EPSILON
    Funcs();
};

class RetType : public BaseType {
public:
    // TYPE
    explicit RetType(BaseType* type) : BaseType(type->token_value) {}
};

class IDWrap : public BaseType {
public:
    string ID;
    int lineNo;
    explicit IDWrap(BaseType* ID, int lineNo) : BaseType(ID->token_value), ID(ID->token_value), lineNo(lineNo){}
};

class TypeAnnotation : public BaseType {
public:
    bool is_const = false;
    // EPSILON
    TypeAnnotation() = default;
    // CONST
    explicit TypeAnnotation(BaseType* type) : BaseType(type->token_value) {
        is_const = true;
    }
};

class FormalDecl : public BaseType {
public:
    string param_type;
    bool is_param_const;
    // Type ID
    FormalDecl(Type* t, IDWrap* id, TypeAnnotation* const_anno) :
        BaseType(id->ID), param_type(t->token_value), is_param_const(const_anno->is_const) {}
};

class FormalsList : public BaseType {
public:
    vector<FormalDecl> formal_list;
    // FormalDecl
    explicit FormalsList(FormalDecl* decl) {
        formal_list.push_back(*decl);
    }
    // FormalDecl COMMA FormalList
    FormalsList(FormalDecl* decl, FormalsList* f_list) {
        formal_list.push_back(*decl);
        formal_list.insert(std::end(formal_list), std::begin(f_list->formal_list), std::end(f_list->formal_list));
    }
};

class Formals : public BaseType {
public:
    vector<FormalDecl> formals;
    // EPSILON
    Formals() = default;
    // FormalList
    explicit Formals(FormalsList* f_list) : formals(f_list->formal_list) {}
};

class FuncDecl : public BaseType {
public:
    vector<string> param_types;
    vector<bool> const_indicator;
    // RetType ID LPAREN Formals RPAREN LBRACE Statements RBRACE
    FuncDecl(RetType* return_type, IDWrap* func_name, Formals* params);
};

class Call; 

class Exp : public BaseType {
public:
    string type;
    // NUM, NUM B, STRING, TRUE, FALSE
    Exp(BaseType* term, const string& rhs);
    // ID
    Exp(IDWrap* term);
    // Call
    explicit Exp(Call* call);
    // NOT
    Exp(bool not_mark, Exp* exp);
    // Exp RELOP/BINOP Exp
    Exp(Exp* first, const OP_TYPE& op, Exp* second);
    // LPAREN Exp RPAREN
    Exp(Exp* exp);
    // LPAREN Type RPAREN Exp
    Exp(Type* new_type, Exp* exp);
};

class ExpList : public BaseType {
public:
    vector<Exp*> list;
    // Exp
    explicit ExpList(Exp* exp) {
        list.push_back(exp);
    }
    // Exp COMMA ExpList
    ExpList(Exp* exp, ExpList* expList) {
        list.push_back(exp);
        list.insert(std::end(list), std::begin(expList->list), std::end(expList->list));
    }
};

class Call : public BaseType {
public:
    string ret_type_of_called_func;
    // ID LPAREN ExpList RPAREN
    Call(IDWrap* id, ExpList* list);
    // ID LPAREN RPAREN
    explicit Call(IDWrap* id);
};

class Statement; 

class Statements : public BaseType {
public:
    // Statement
    explicit Statements(Statement* rhs_statement) {}
    // Statements Statement
    Statements(Statements* rhs_statements, Statement* rhs_statement) {}
};

class Statement : public BaseType {
public:
    // LBRACE Statements RBRACE
    explicit Statement(Statements* rhs_statements) {}
    // TypeAnnotation Type ID SC
    Statement(Type* type, IDWrap* id, TypeAnnotation* const_anno);
    // TypeAnnotation Type ID Assign Exp SC
    Statement(Type* type, IDWrap* id, Exp* exp, TypeAnnotation* const_anno);
    // ID Assign Exp SC
    Statement(IDWrap* id, Exp* exp);
    // Call SC
    explicit Statement(Call* call) {}
    // Return SC (void)
    explicit Statement();
    // Return Exp SC (not void)
    explicit Statement(Exp* exp);
    // IF etc...
    Statement(const string& type, Exp* exp);
    // BREAK, CONTINUE
    explicit Statement(const Break_Cont& type);
};

#endif //COMPIHW3_SEMANTICANALYZER_H
