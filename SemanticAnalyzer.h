//
// Created by Renen on 12/12/2021.
//
#ifndef COMPIHW3_SEMANTICANALYZER_H
#define COMPIHW3_SEMANTICANALYZER_H

#include <memory>
#include <vector>
#include <string>
#include <utility>
#include <ostream>
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

#define YYSTYPE BaseType&

class Type : public BaseType {
public:
    explicit Type(BaseType& type);
};

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

class FuncDecl : public BaseType {
public:
    // Parameter types.
    vector<string> type;
    // RetType ID LPAREN Formals RPAREN LBRACE Statements RBRACE
    FuncDecl(RetType& return_type, BaseType& id, Formals& params);
};

class RetType : public BaseType {
public:
    // TYPE
    explicit RetType(BaseType& type) : BaseType(type) {}
};

class Formals : public BaseType {
public:
    vector<FormalDecl> formals;
    // EPSILON
    Formals() = default;
    // FormalList
    explicit Formals(const FormalsList& f_list) : formals(f_list.formal_list) {}
};

class FormalsList : public BaseType {
public:
    vector<FormalDecl> formal_list;
    // FormalDecl
    explicit FormalsList(FormalDecl& decl) { 
        formal_list.push_back(decl); // int x ---> formal_list[0] = int x
    }
    // FormalDecl COMMA FormalList
    FormalsList(FormalDecl& decl, const FormalsList& f_list) {
        formal_list.push_back(decl); // int x, y; ---> formal_list[0] = int x, formal_list[1] = int y
        formal_list.insert(std::end(formal_list), std::begin(f_list.formal_list), std::end(f_list.formal_list));
    }
};

class FormalDecl : public BaseType {
public:
    // The parameter type
    string type;
    // Type ID
    FormalDecl(Type& type, BaseType& id) : BaseType(id.actual_type), type(type.actual_type) {}
};

class Statements : public BaseType {
public:
    // Statement
    explicit Statements(Statement& rhs_statement);
    // Statements Statement
    Statements(Statements& rhs_statements, Statement& rhs_statement);
};

class Statement : public BaseType {
public:
    string dataTag;
    // LBRACE Statements RBRACE
    explicit Statement(Statements& rhs_statements);
    // Type ID SC
    Statement(Type& type, BaseType& id);
    // Type ID Assign Exp SC
    Statement(Type& type, BaseType& id, Exp& exp);
    // ID Assign Exp SC
    Statement(BaseType& id, Exp &exp);
    // Call SC
    explicit Statement(Call &call);
    // Return SC (void)
    explicit Statement(const string& ret_type);
    // Return Exp SC (not void)
    explicit Statement(Exp &exp);
    // IF etc...
    Statement(const string& type, Exp& exp);
    // BREAK, CONTINUE
    explicit Statement(BaseType &type);
};

class Exp : public BaseType {
public:
    string type;
    // NUM, NUM B, STRING, TRUE, FALSE, ID
    Exp(BaseType &terminal, const string& rhs);
    // Call
    explicit Exp(Call &call);
    // NOT
    Exp(BaseType &not_node, Exp &exp);
    // Exp RELOP/BINOP Exp
    Exp(Exp &first, BaseType &operation, Exp &second, const string &rhs);
    // LPAREN Exp RPAREN
    Exp(Exp &ex);
};

class ExpList : public BaseType {
public:
    vector<Exp> list;
    // Exp
    explicit ExpList(Exp &exp);
    // Exp COMMA ExpList
    ExpList(Exp &exp, ExpList &expList);
};

class Call : public BaseType {
public:
    // ID LPAREN ExpList RPAREN
    Call(BaseType &id, ExpList &list);
    // ID LPAREN RPAREN
    explicit Call(BaseType &id);
};














#endif //COMPIHW3_SEMANTICANALYZER_H
