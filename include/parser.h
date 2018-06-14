/*
    File:    parser.h
    Created: 22 April 2018 at 18:54 Moscow time
    Author:  Гаврилов Владимир Сергеевич
    E-mails: vladimir.s.gavrilov@gmail.com
             gavrilov.vladimir.s@mail.ru
             gavvs1977@yandex.ru
*/

#ifndef PARSER_H
#define PARSER_H
#include <memory>
#include "../include/ast.h"
#include "../include/symbol_table.h"
#include "../include/errors_and_tries.h"
#include "../include/main_scaner.h"
class Parser{
public:
    Parser();
    Parser(const Parser&) = default;
    ~Parser();
    
    Parser(const std::shared_ptr<Main_scaner>&                sc,
           const std::shared_ptr<symbol_table::Symbol_table>& st,
           const Errors_and_tries&                            et);
    
    void compile(ast::Ast& buf);
    size_t number_of_errors() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
#endif