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

class BaseType {
public:
    BaseType();
    string actual_type;
    explicit BaseType(const string& str) {
        if (str == "void")
            actual_type = "VOID";
        else if (str == "bool")
            actual_type = "BOOL";
        else if (str == "int")
            actual_type = "INT";
        else if (str == "byte")
            actual_type = "BYTE";
        else
            actual_type = str;
    }
};

#define YYSTYPE BaseType*

class SymbolEntry {
public:
    string name;
    vector<string> type;
    int offset;
    bool is_func;
    SymbolEntry(const string& name, const vector<string>& type, int offset, bool is_func) :
        name(name), type(type), offset(offset), is_func(is_func) {}
};

class SymbolTable {
public:
    vector<SymbolEntry> rows;
};

class Type : public BaseType {
public:
    explicit Type(const BaseType& type);
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
    explicit RetType(const BaseType& type) : BaseType(type) {}
};

class FormalDecl : public BaseType {
public:
    // The parameter type
    string type;
    // Type ID
    FormalDecl(const Type& t, const BaseType& id) : BaseType(id.actual_type), type(t.actual_type) {}
};

class FormalsList : public BaseType {
public:
    vector<FormalDecl> formal_list;
    // FormalDecl
    explicit FormalsList(const FormalDecl& decl) { 
        formal_list.push_back(decl); // int x ---> formal_list[0] = int x
    }
    // FormalDecl COMMA FormalList
    FormalsList(const FormalDecl& decl, const FormalsList& f_list) {
        formal_list.push_back(decl); // int x, y; ---> formal_list[0] = int x, formal_list[1] = int y
        formal_list.insert(std::end(formal_list), std::begin(f_list.formal_list), std::end(f_list.formal_list));
    }
};

class Formals : public BaseType {
public:
    vector<FormalDecl> formals;
    // EPSILON
    Formals() = default;
    // FormalList
    explicit Formals(const FormalsList& f_list) : formals(f_list.formal_list) {}
};

class FuncDecl : public BaseType {
public:
    // Parameter types.
    vector<string> type;
    // RetType ID LPAREN Formals RPAREN LBRACE Statements RBRACE
    FuncDecl(const RetType& return_type, const BaseType& id, const Formals& params);
};

class Call; 

class Exp : public BaseType {
public:
    string type;
    // NUM, NUM B, STRING, TRUE, FALSE, ID
    Exp(const BaseType& terminal, const string& rhs);
    // Call
    explicit Exp(const Call& call);
    // NOT
    Exp(const BaseType& not_node, const Exp& exp);
    // Exp RELOP/BINOP Exp
    Exp(const Exp& first, const BaseType& operation, const Exp& second, const string& rhs);
    // LPAREN Exp RPAREN
    Exp(const Exp& ex);
};

class ExpList : public BaseType {
public:
    vector<Exp> list;
    // Exp
    explicit ExpList(const Exp& exp) {
        list.push_back(exp);
    }
    // Exp COMMA ExpList
    ExpList(const Exp& exp, const ExpList& expList) {
        list.push_back(exp);
        list.insert(std::end(list), std::begin(expList.list), std::end(expList.list));
    }
};

class Call : public BaseType {
public:
    // ID LPAREN ExpList RPAREN
    Call(const BaseType& id, const ExpList& list);
    // ID LPAREN RPAREN
    explicit Call(const BaseType& id);
};

class Statement; 

class Statements : public BaseType {
public:
    // Statement
    explicit Statements(const Statement& rhs_statement);
    // Statements Statement
    Statements(const Statements& rhs_statements, const Statement& rhs_statement);
};

class Statement : public BaseType {
public:
    // LBRACE Statements RBRACE
    explicit Statement(const Statements& rhs_statements);
    // Type ID SC
    Statement(const Type& type, const BaseType& id);
    // Type ID Assign Exp SC
    Statement(const Type& type, const BaseType& id, const Exp& exp);
    // ID Assign Exp SC
    Statement(const BaseType& id, const Exp& exp);
    // Call SC
    explicit Statement(const Call& call);
    // Return SC (void)
    explicit Statement();
    // Return Exp SC (not void)
    explicit Statement(const Exp& exp);
    // IF etc...
    Statement(const string& type, const Exp& exp);
    // BREAK, CONTINUE
    explicit Statement(const BaseType& type);
};

#endif //COMPIHW3_SEMANTICANALYZER_H
