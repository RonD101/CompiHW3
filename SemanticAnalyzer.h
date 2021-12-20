//
// Created by Renen on 12/12/2021.
//
#ifndef COMPIHW3_SEMANTICANALYZER_H
#define COMPIHW3_SEMANTICANALYZER_H

#include <vector>
#include <string>
#include "hw3_output.hpp"

using std::vector;
using std::string;
using std::shared_ptr;

void loop_entered();
void loop_exited();
void create_new_scope();
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
    BaseType();
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
    explicit Type(const shared_ptr<BaseType>& type) : BaseType(*type) {}
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
    explicit RetType(const shared_ptr<BaseType>& type) : BaseType(*type) {}
};

class TypeAnnotation : BaseType {
public:
    bool is_const = false;
    // EPSILON
    TypeAnnotation() = default;
    // CONST
    TypeAnnotation(const string& rhs) : BaseType(rhs) {
        is_const = true;
    }
};

class FormalDecl : public BaseType {
public:
    string param_type;
    bool is_param_const;
    // Type ID
    FormalDecl(const shared_ptr<Type>& t, const shared_ptr<BaseType>& id, const shared_ptr<TypeAnnotation>& const_anno) :
        BaseType(id->token_value), param_type(t->token_value), is_param_const(const_anno->is_const) {}
};

class FormalsList : public BaseType {
public:
    vector<FormalDecl> formal_list;
    // FormalDecl
    explicit FormalsList(const shared_ptr<FormalDecl>& decl) {
        formal_list.push_back(*decl);
    }
    // FormalDecl COMMA FormalList
    FormalsList(const shared_ptr<FormalDecl>& decl, const shared_ptr<FormalsList>& f_list) {
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
    explicit Formals(const shared_ptr<FormalsList>& f_list) : formals(f_list->formal_list) {}
};

class FuncDecl : public BaseType {
public:
    vector<string> param_types;
    vector<bool> const_indicator;
    // RetType ID LPAREN Formals RPAREN LBRACE Statements RBRACE
    FuncDecl(const shared_ptr<RetType>& return_type, const shared_ptr<BaseType>& func_name, const shared_ptr<Formals>& params);
};

class Call; 

class Exp : public BaseType {
public:
    string type;
    bool res_type;
    // NUM, NUM B, STRING, TRUE, FALSE
    Exp(const shared_ptr<BaseType>& term, const shared_ptr<string>& rhs);
    // ID
    Exp(const shared_ptr<BaseType>& term);
    // Call
    explicit Exp(const shared_ptr<Call>& call);
    // NOT
    Exp(bool not_mark, const shared_ptr<Exp>& exp);
    // Exp RELOP/BINOP Exp
    Exp(const shared_ptr<Exp>& first, const OP_TYPE& op, const shared_ptr<Exp>& second);
    // LPAREN Exp RPAREN
    Exp(const shared_ptr<Exp>& exp);
    // LPAREN Type RPAREN Exp
    Exp(const shared_ptr<Type>& type, const shared_ptr<Exp>& exp);
};

class ExpList : public BaseType {
public:
    vector<shared_ptr<Exp>> list;
    // Exp
    explicit ExpList(const shared_ptr<Exp>& exp) {
        list.push_back(exp);
    }
    // Exp COMMA ExpList
    ExpList(const shared_ptr<Exp>& exp, const shared_ptr<ExpList>& expList) {
        list.push_back(exp);
        list.insert(std::end(list), std::begin(expList->list), std::end(expList->list));
    }
};

class Call : public BaseType {
public:
    // ID LPAREN ExpList RPAREN
    Call(const shared_ptr<BaseType>& id, const shared_ptr<ExpList>& list);
    // ID LPAREN RPAREN
    explicit Call(const shared_ptr<BaseType>& id);
};

class Statement; 

class Statements : public BaseType {
public:
    // Statement
    explicit Statements(const shared_ptr<Statement>& rhs_statement);
    // Statements Statement
    Statements(const shared_ptr<Statements>& rhs_statements, const shared_ptr<Statement>& rhs_statement);
};

class Statement : public BaseType {
public:
    // LBRACE Statements RBRACE
    explicit Statement(const shared_ptr<Statements>& rhs_statements);
    // TypeAnnotation Type ID SC
    Statement(const shared_ptr<Type>& type, const shared_ptr<BaseType>& id, const shared_ptr<TypeAnnotation>& const_anno);
    // TypeAnnotation Type ID Assign Exp SC
    Statement(const shared_ptr<Type>& type, const shared_ptr<BaseType>& id, const shared_ptr<Exp>& exp, const shared_ptr<TypeAnnotation>& const_anno);
    // ID Assign Exp SC
    Statement(const shared_ptr<BaseType>& id, const shared_ptr<Exp>& exp);
    // Call SC
    explicit Statement(const shared_ptr<Call>& call);
    // Return SC (void)
    explicit Statement();
    // Return Exp SC (not void)
    explicit Statement(const shared_ptr<Exp>& exp);
    // IF etc...
    Statement(const string& type, const shared_ptr<Exp>& exp);
    // BREAK, CONTINUE
    explicit Statement(const shared_ptr<Break_Cont>& type);
};

#endif //COMPIHW3_SEMANTICANALYZER_H
