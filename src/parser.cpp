/*
    File:    parser.cpp
    Created: 22 April 2018 at 19:32 Moscow time
    Author:  Гаврилов Владимир Сергеевич
    E-mails: vladimir.s.gavrilov@gmail.com
             gavrilov.vladimir.s@mail.ru
             gavvs1977@yandex.ru
*/

#include <map>
#include <utility>
#include "../include/main_lexem_info.h"
#include "../include/scope.h"
#include "../include/parser.h"
#include "../include/token_classification.h"
#include "../include/idx_to_string.h"

struct Parser::Impl final{
public:
    Impl()            = default;
    Impl(const Impl&) = default;
    ~Impl()           = default;

    Impl(const std::shared_ptr<Main_scaner>&                sc,
         const std::shared_ptr<symbol_table::Symbol_table>& st,
         const Errors_and_tries&                            et) :
        sc_(sc), st_(st), et_(et) {}

    void compile(ast::Ast& buf);
    size_t number_of_errors() const;
private:
    std::shared_ptr<Main_scaner>                sc_;
    std::shared_ptr<symbol_table::Symbol_table> st_;
    Errors_and_tries                            et_;

    ast::Ast                                    buf_;

    std::shared_ptr<ast::Program>               proc_program();
    ast::Module_body                            proc_module_body();

    void add_export_name(size_t name_idx);
    void add_imported_module(size_t name_idx);

    std::list<std::shared_ptr<ast::Var>>        proc_var();
    std::list<std::shared_ptr<ast::Type>>       proc_types();
    std::list<std::shared_ptr<ast::Definition>> proc_func_proto();
    std::list<std::shared_ptr<ast::Constant>>   proc_const();

    bool                                        check_var_name(ast::Id name);

    std::shared_ptr<ast::Simplest_type_def>     proc_simplest_type_def();
    std::shared_ptr<ast::Type_def>              proc_type_definition();
    std::shared_ptr<ast::Type_def>              proc_compound_type_definition();
    std::shared_ptr<ast::Elementary_type>       proc_elementary_type();
    std::shared_ptr<ast::Expr>                  proc_expr();
    std::shared_ptr<ast::Embedded_type>         proc_embedded_type();
    
    bool add_type_to_symbol_table(ast::Id                               name, 
                                  const std::shared_ptr<ast::Type_def>& definition);
};
std::shared_ptr<ast::Unary_op> Parser::Impl::proc_bnot();
std::shared_ptr<ast::Binary_op> Parser::Impl::proc_add_sub();
std::shared_ptr<ast::Binary_op> Parser::Impl::proc_mult_div_rmndr_fdiv();
Parser::Parser() : impl_(std::make_unique<Impl>()){}

Parser::Parser(const std::shared_ptr<Main_scaner>&                sc,
               const std::shared_ptr<symbol_table::Symbol_table>& st,
               const Errors_and_tries&                            et) :
    impl_(std::make_unique<Impl>(sc, st, et)) {}

Parser::~Parser() = default;

void Parser::compile(ast::Ast& buf)
{
    impl_->compile(buf);
}

size_t Parser::number_of_errors() const
{
    return impl_->number_of_errors();
}

size_t Parser::Impl::number_of_errors() const
{
    return et_.ec->get_number_of_errors();
}

void Parser::Impl::compile(ast::Ast& buf)
{
    buf_.clear();
    auto prog = proc_program();
    buf_.push_back(prog);
    buf = buf_;
}

static const char* expected_kw_module_fmt                           =
    "Error at line %zu: expected the keyword модуль.\n";
static const char* expected_module_name_fmt                         =
    "Error at line %zu: expected the module name.\n";
static const char* expected_open_curly_bracket_fmt                  =
    "Error at line %zu: expected {\n";
static const char* expected_uses_export_var_type_func_const_fmt     =
    "Error at line %zu: expected one of the following keywords: "
    "использует предоставляет перем тип функция конст.\n";
static const char* expected_comma_export_var_type_func_const_fmt    =
    "Error at line %zu: expected comma or one of the following keywords: "
    "предоставляет перем тип функция конст.\n";
static const char* expected_imported_module_name_fmt                =
    "Error at line %zu: expected name of a used module.\n";
static const char* expected_exported_name_fmt                       =
    "Error at line %zu: expected name of an exported thing.\n";
static const char* expected_comma_var_type_func_const_fmt           =
    "Error at line %zu: expected comma or one of the following keywords: "
    "перем тип функция конст.\n";
static const char* already_exported_name_fmt                        =
    "Error at line %zu: name %s is already exported.\n";
static const char* already_imported_name_fmt                        =
    "Error at line %zu: the module %s is already imported.\n";
static const char* expected_var_fmt                                 =
    "Error at line %zu: expected the keyword перем.\n";
static const char* expected_variable_name_fmt                       =
    "Error at line %zu: expected variable name.\n";
static const char* expected_comma_colon_fmt                         =
    "Error at line %zu: expected comma or semicolon.\n";
static const char* expected_type_fmt                                =
    "Error at line %zu: expected keyword тип.\n";
static const char* expected_type_name_fmt                           =
    "Error at line %zu: expected type name.\n";
static const char* expected_equal_sign_fmt                          =
    "Error at line %zu: expected character =\n";
static const char* expected_type_def_begin_fmt                      = 
    "Error at line %zu: expected an embedded type, a tuple begin, the sign @, an "
    "identifier, the module name prefix (i.e. |:), or the keyword функция.\n";
static const char* expected_array_at_ident_prefix_embedded_type_fmt =   
    "Error at line %zu: expected the keyword массив, the sign @, an identifier, "
    "the module name prefix (i.e. |:), or an embedded type.\n";
static const char* expected_open_sq_bracket_fmt                     =     
    "Error at line %zu: expected [\n";
static const char* expected_comma_or_expression_fmt                 =
    "Error at line %zu: expected comma or expression.\n";
static const char* expected_comma_or_closed_square_bracket_fmt      =
    "Error at line %zu: expected comma or closed square bracket.\n";
static const char* expected_embedded_or_id_or_mod_name_prefix_fmt   = 
    "Error at line %zu: expected an embedded type, an identifier, the sign @, or "
    "the module name prefix (i.e. |:).\n";
static const char* expected_scope_resolution_fmt_name_fmt           =
    "Error at line %zu: expected scope resolution operator.\n";
static const char* expected_embedded_type_fmt                       =
    "Error at line %zu: expected an embedded type.\n";
static const char* already_defined_name_fmt                         =
    "Error at line %zu: identifier %s is already defined.\n";    
    
static const char* var_name_diagnosis_fmt[] = {
    "Error at line %zu: variable name %s is already defined as an imported "
    "module name.\n",

    "",

    "Error at line %zu: variable name %s is already defined as the current "
    "module name.\n",

    "Error at line %zu: variable name %s is already defined as a type name.\n",

    "Error at line %zu: variable name %s is repeatedly defined.\n",

    "Error at line %zu: variable name %s is already defined as a constant name.\n",

    "Error at line %zu: variable name %s is already defined as a function name.\n",

    "Error at line %zu: variable name %s is already defined as a cycle label.\n",

    "Error at line %zu: variable name %s is already defined as a type name.\n",

    "Error at line %zu: variable name %s is repeatedly defined.\n",

    "Error at line %zu: variable name %s is already defined as a constant name.\n",

    "Error at line %zu: variable name %s is already defined as a function name.\n",

    "Error at line %zu: variable name %s is already defined as a function name.\n",

    "Error at line %zu: variable name %s is already defined as a field of a struct.\n",

    "Error at line %zu: variable name %s is already defined as a enumeration element.\n",

    "Error at line %zu: variable name %s is already defined as a formal parameter of "
    "a function.\n",

    "Error at line %zu: variable name %s is already defined as a formal parameter of "
    "a function.\n",

    "Error at line %zu: variable name %s is already defined as a formal parameter of "
    "a function.\n"
};

void Parser::Impl::add_export_name(size_t name_idx)
{
    scope::Id_info id_info;
    id_info.attr = scope::Id_attribute::Unknown_but_exported;

    auto sr = st_->search(name_idx);
    if(sr.status ==  symbol_table::Search_status::Not_found){
        st_->insert(name_idx, id_info);
    }else{
        auto id_str = idx_to_string(et_.ids_trie, name_idx);
        printf(already_exported_name_fmt,
               sc_->lexem_begin_line_number(),
               id_str.c_str());
        et_.ec->increment_number_of_errors();
    }
}

void Parser::Impl::add_imported_module(size_t name_idx)
{
    scope::Id_info id_info;
    id_info.attr = scope::Id_attribute::Used_module;

    auto sr = st_->search(name_idx);
    if(sr.status ==  symbol_table::Search_status::Not_found){
        st_->insert(name_idx, id_info);
    }else{
        auto id_str = idx_to_string(et_.ids_trie, name_idx);
        printf(already_imported_name_fmt,
               sc_->lexem_begin_line_number(),
               id_str.c_str());
        et_.ec->increment_number_of_errors();
    }
}

std::shared_ptr<ast::Program> Parser::Impl::proc_program() // 1
{
    std::shared_ptr<ast::Program> node;
    enum class State{
        Start,        Module,      Module_name,
        Module_begin, Uses,        Exports,
        Body,         Used_module, Exported_name,
        Module_end
    };
    State state = State::Start;
    st_->create_new_scope();

    ast::Id            module_name;
    std::list<ast::Id> imports;
    std::list<ast::Id> exports;
    ast::Module_body   body;
    
    scope::Id_info     id_info;

    for(;;){
        Main_lexem_info li  = sc_->current_lexem();
        Lexem_category  cat = get_lexem_category(li);
        switch(state){
            case State::Start:
                if(cat != Lexem_category::Kw_module){
                    printf(expected_kw_module_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return node;
                }
                state = State::Module;
                break;
            case State::Module:
                if(cat != Lexem_category::Ident){
                    printf(expected_module_name_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return node;
                }
                state       = State::Module_name;
                module_name = li.ident_index;
                {
                    scope::Id_info name;
                    name.attr = scope::Id_attribute::Module_name;
                    st_->insert(module_name, name);
                }
                break;
            case State::Module_name:
                if(cat != Lexem_category::Open_curly_bracket){
                    printf(expected_open_curly_bracket_fmt,
                           sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return node;
                }
                state = State::Module_begin;
                break;
            case State::Module_begin:
                switch(cat){
                    case Lexem_category::Kw_uses:
                        state = State::Uses;
                        break;
                    case Lexem_category::Kw_export:
                        state = State::Exports;
                        break;
                    case Lexem_category::Kw_var:
                    case Lexem_category::Kw_type:
                    case Lexem_category::Kw_function:
                    case Lexem_category::Kw_const:
                        sc_->back();
                        body  = proc_module_body();
                        state = State::Body;
                        break;
                    default:
                        printf(expected_uses_export_var_type_func_const_fmt,
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return node;
                }
                break;
            case State::Uses:
                if(cat != Lexem_category::Ident){
                    printf(expected_imported_module_name_fmt,
                           sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return node;
                }
                state = State::Used_module;
                imports.push_back(li.ident_index);
                add_imported_module(li.ident_index);
                break;
            case State::Exports:
                if(cat != Lexem_category::Ident){
                    printf(expected_exported_name_fmt,
                           sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return node;
                }
                state = State::Exported_name;
                exports.push_back(li.ident_index);
                add_export_name(li.ident_index);
                break;
            case State::Body:
                if(cat != Lexem_category::Closed_curly_bracket){
                    printf(expected_closed_curly_bracket_fmt,
                           sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return node;
                }
                state = State::Module_end;
                break;
            case State::Used_module:
                switch(cat){
                    case Lexem_category::Comma:
                        state = State::Uses;
                        break;
                    case Lexem_category::Kw_export:
                        state = State::Exports;
                        break;
                    case Lexem_category::Kw_var:
                    case Lexem_category::Kw_type:
                    case Lexem_category::Kw_function:
                    case Lexem_category::Kw_const:
                        sc_->back();
                        body  = proc_module_body();
                        state = State::Body;
                        break;
                    default:
                        printf(expected_comma_export_var_type_func_const_fmt,
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return node;
                }
                break;
            case State::Exported_name:
                switch(cat){
                    case Lexem_category::Comma:
                        state = State::Exports;
                        break;
                    case Lexem_category::Kw_var:
                    case Lexem_category::Kw_type:
                    case Lexem_category::Kw_function:
                    case Lexem_category::Kw_const:
                        sc_->back();
                        body  = proc_module_body();
                        state = State::Body;
                        break;
                    default:
                        printf(expected_comma_var_type_func_const_fmt,
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return node;
                }
                break;
            case State::Module_end:
                node    = std::make_shared<ast::Program>(module_name,
                                                         imports,
                                                         exports,
                                                         body);
                return node;
        }
    }
    return node;
}

ast::Module_body Parser::Impl::proc_module_body() // 2
{
    ast::Module_body body;
    for(;;){
        Main_lexem_info li  = sc_->current_lexem();
        Lexem_category  cat = get_lexem_category(li);
        sc_->back();
        switch(cat){
            case Lexem_category::Kw_var:
                {
                    auto vars = proc_var();
                    body.insert(body.end(), vars.begin(), vars.end());
                }
                break;
            case Lexem_category::Kw_type:
                {
                    auto types = proc_types();
                    body.insert(body.end(), types.begin(), types.end());
                }
                break;
            case Lexem_category::Kw_function:
                {
                    auto funcs = proc_func_proto();
                    body.insert(body.end(), funcs.begin(), funcs.end());
                }
                break;
            case Lexem_category::Kw_const:
                {
                    auto consts = proc_const();
                    body.insert(body.end(), consts.begin(), consts.end());
                }
                break;
            default:
                return body;
        }
    }
    return body;
}

static std::list<std::shared_ptr<ast::Var>>
  collect_vars_defs(const std::list<ast::Id>&                      vars,
                    const std::shared_ptr<ast::Simplest_type_def>& current_type)
{
    static std::list<std::shared_ptr<ast::Var>> result;
    for(const auto& v : vars){
        result.push_back(std::make_shared<ast::Var>(v, current_type));
    }
    return result;
}

bool Parser::Impl::check_var_name(ast::Id name)
{
    auto sr = st_->search(name);
    if((sr.status != symbol_table::Search_status::Found_in_the_current_scope) ||
       (sr.attr   == scope::Id_attribute::Unknown_but_exported))
    {
        return true;
    }else{
        auto id_str = idx_to_string(et_.ids_trie, name);
        printf(var_name_diagnosis_fmt[static_cast<unsigned>(sr.attr)],
               sc_->lexem_begin_line_number(),
               id_str.c_str());
        et_.ec->increment_number_of_errors();
        return false;
    }
}

std::list<std::shared_ptr<ast::Var>> Parser::Impl::proc_var() // 3
{
    std::list<std::shared_ptr<ast::Var>> result;
    enum class State{
        Start, Var_kw, Var_name, Colon, Simplest_typedef
    };
    State state = State::Start;
    std::list<ast::Id>                      vars;
    std::shared_ptr<ast::Simplest_type_def> current_type;
    bool                                    correct_var_name;
    for(;;){
        Main_lexem_info li  = sc_->current_lexem();
        Lexem_category  cat = get_lexem_category(li);
        switch(state){
            case State::Start:
                if(cat != Lexem_category::Kw_var){
                    printf(expected_var_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return collect_vars_defs(vars, current_type);
                }
                state = State::Var_kw;
                break;
            case State::Var_kw:
                if(cat != Lexem_category::Ident){
                    printf(expected_variable_name_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    auto v = collect_vars_defs(vars, current_type);
                    result.insert(result.end(), v.begin(), v.end());
                    return result;
                }
                correct_var_name = check_var_name(li.ident_index);
                if(!correct_var_name){
                    auto v = collect_vars_defs(vars, current_type);
                    result.insert(result.end(), v.begin(), v.end());
                    return result;
                }
                vars.push_back(li.ident_index);
                state = State::Var_name;
                break;
            case State::Var_name:
                switch(cat){
                    case Lexem_category::Comma:
                        state = State::Var_kw;
                        break;
                    case Lexem_category::Colon:
                        state = State::Colon;
                        break;
                    default:
                        {
                            printf(expected_comma_colon_fmt,
                                   sc_->lexem_begin_line_number());
                            et_.ec->increment_number_of_errors();
                            auto v = collect_vars_defs(vars, current_type);
                            result.insert(result.end(), v.begin(), v.end());
                            return result;
                        }
                }
                break;
            case State::Colon:
                sc_->back();
                current_type = proc_simplest_type_def();
                state = State::Simplest_type_def;
                break;
            case State::Simplest_typedef:
                {
                    auto v = collect_vars_defs(vars, current_type);
                    result.insert(result.end(), v.begin(), v.end());                 
                }
                if(cat == Lexem_category::Semicolon){
                    state = State::Var_kw;
                    vars.clear();
                }else{
                    sc_->back();
                    return result;
                }
                break;
        }
    }
    return result;
}

std::list<std::shared_ptr<ast::Type>> Parser::Impl::proc_types() // 4
{
    std::list<std::shared_ptr<ast::Type>> result;
    enum class State{
        Start, Type_kw, Typename, Is, Def
    };
    State state = State::Start;
    ast::Id                        current_name = 0;
    std::shared_ptr<ast::Type_def> def;
    for(;;){
        Main_lexem_info li  = sc_->current_lexem();
        Lexem_category  cat = get_lexem_category(li);
        switch(state){
            case State::Start:
                if(cat != Lexem_category::Kw_type){
                    printf(expected_type_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    result.push_back(std::make_shared<ast::Type>(current_name, def));
                    return result;
                }
                state = State::Type_kw;
                break;
            case State::Type_kw:
                if(cat != Lexem_category::Ident){
                    printf(expected_type_name_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    result.push_back(std::make_shared<ast::Type>(current_name, def));
                    return result;
                }
                current_name = li.ident_index;
                state        = State::Typename;
                break;
            case State::Typename:
                if(cat != Lexem_category::Equal_sign){
                    printf(expected_equal_sign_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    result.push_back(std::make_shared<ast::Type>(current_name, def));
                    return result;
                }
                state = State::Is;
                break;
            case State::Is:
                sc_->back();
                def   = proc_type_definition();
                state = State::Def;
                break;
            case State::Def:
                result.push_back(std::make_shared<ast::Type>(current_name, def));
                if(cat != Lexem_category::Semicolon){
                    sc_->back();                    
                    return result;
                }else{
                    state = State::Type_kw;
                }
                break;
        }
    }
    return result;
}

std::list<std::shared_ptr<ast::Definition>> Parser::Impl::proc_func_proto()
{
    std::list<std::shared_ptr<ast::Definition>> result;
    return result;
}

std::list<std::shared_ptr<ast::Constant>> Parser::Impl::proc_const()
{
    std::list<std::shared_ptr<ast::Constant>> result;
    return result;
}

std::shared_ptr<ast::Type_def> Parser::Impl::proc_type_definition() // 5
{
    std::shared_ptr<ast::Type_def> result;
    Main_lexem_info                li     = sc_->current_lexem();
    Lexem_category                 cat    = get_lexem_category(li);
    sc_->back();
    switch(cat){
        case Lexem_category::Kw_array:
        case Lexem_category::At_sign:
        case Lexem_category::Ident:
        case Lexem_category::Module_name_prefix:
        case Lexem_category::Modified_type:
        case Lexem_category::Non_modified_type:
        case Lexem_category::Size_changer:
            result = proc_simplest_type_def();
            break;
        case Lexem_category::Tuple_begin:
        case Lexem_category::Kw_function:
        case Lexem_category::Kw_enum:
        case Lexem_category::Kw_struct:
            result = proc_compound_type_definition();
            break;
        default:
            printf(expected_type_def_begin_fmt, sc_->lexem_begin_line_number());
            et_.ec->increment_number_of_errors();
    }
    return result;
}

std::shared_ptr<ast::Simplest_type_def> Parser::Impl::proc_simplest_type_def() // 6
{
    std::shared_ptr<ast::Simplest_type_def> result;
    enum class State{
        Start,        Array_kw, Elementary, 
        Sq_br_opened, Ind
    };

    std::list<std::shared_ptr<Expr>> dims;
    std::shared_ptr<Elementary_type> elem_type;

    State state = State::Start;
    for(;;){
        Main_lexem_info li  = sc_->current_lexem();
        Lexem_category  cat = get_lexem_category(li);
        switch(state){
            case State::Start: 
                switch(cat){
                    case Lexem_category::Kw_array:
                        state = State::Array_kw;
                        break;
                    case Lexem_category::At_sign:
                    case Lexem_category::Ident:
                    case Lexem_category::Module_name_prefix:
                    case Lexem_category::Modified_type:
                    case Lexem_category::Non_modified_type:
                    case Lexem_category::Size_changer:
                        sc_->back();
                        elem_type = proc_elementary_type();
                        state     = State::Elementary;                        
                        break;
                    default:
                        printf(expected_array_at_ident_prefix_embedded_type_fmt, 
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
                }
                break;
            case State::Array_kw: 
                if(cat != Lexem_category::Open_square_bracket){
                    printf(expected_open_sq_bracket_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Sq_br_opened; 
                break;
            case State::Elementary: 
                sc_->back();
                if(dims.empty()){
                    return elem_type;
                }else{
                    result = std::make_shared<Array_type_def>(dims, elem_type);
                    return result;
                }
                break;                
            case State::Sq_br_opened: 
                switch(cat){
                    case Lexem_category::Comma:
                        dims.push_back(std::shared_ptr<ast::Expr>());
                        break;
                    case Lexem_category::Lnot:
                    case Lexem_category::Bnot:
                    case Lexem_category::Add_op:
                    case Lexem_category::Size:
                    case Lexem_category::Literal:
                    case Lexem_category::Open_round_bracket:
                    case Lexem_category::Kw_array:
                    case Lexem_category::Kw_alloc:
                    case Lexem_category::Ident:
                    case Lexem_category::Module_name_prefix:
                        {
                            auto e = proc_expr();
                            dims.push_back(e);
                            state = State::Ind;
                        }
                        break;
                    default:
                        printf(expected_comma_or_expression_fmt, 
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        result = std::make_shared<Array_type_def>(dims, elem_type);
                        return result;
                }
                break;
            case State::Ind:
                switch(cat){
                    case Lexem_category::Comma:
                        state = State::Sq_br_opened;
                        break;
                    case Lexem_category::Closed_square_bracket:
                        state = State::Start;
                        break;
                    default:
                        printf(expected_comma_or_closed_square_bracket_fmt, 
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        result = std::make_shared<Array_type_def>(dims, elem_type);
                        return result;
                }
                break;
        }
    }
    return result;
}

std::shared_ptr<ast::Elementary_type> Parser::Impl::proc_elementary_type() // 7
{
    std::shared_ptr<ast::Elementary_type> result;
    enum class State{
        Start,           Embedded_or_id, Mod_name_prefix, 
        In_which_module, Scope_res
    };
    State state = State::Start;
    
    size_t ref_order = 0;
    size_t type_name = 0;
    size_t mod_name  = 0;
    for(;;){
        Main_lexem_info li  = sc_->current_lexem();
        Lexem_category  cat = get_lexem_category(li);
        bool            is_quit;
        switch(state){
            case State::Start: 
                switch(cat){
                    case Lexem_category::At_sign:
                        ref_order++;
                        break;
                    case Lexem_category::Modified_type:
                    case Lexem_category::Non_modified_type:
                    case Lexem_category::Size_changer:
                        state = State::Embedded_or_id;
                        {
                            auto p             = proc_embedded_type();
                            result             = p;
                            result->ref_order_ = ref_order;
                        }
                        break;
                    case Lexem_category::Ident:
                        state   = State::Embedded_or_id;
                        result  = std::make_shared<Type_alias>(mod_name,
                                                               type_name, 
                                                               ref_order);
                        t       = add_type_to_symbol_table(type_name, result);
                        if(!t){
                            return result;
                        }
                        break;
                    case Lexem_category::Module_name_prefix:
                        state = State::Mod_name_prefix;
                        break;
                    default:
                        printf(expected_embedded_or_id_or_mod_name_prefix_fmt, 
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;                    
                }
                break;
            case State::Embedded_or_id: 
                sc_->back();
                return result;
            case State::Mod_name_prefix: 
                if(cat != Lexem_category::Ident){
                    sc_->back();
                    printf(expected_module_name_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state    = State::In_which_module;
                mod_name = li.ident_index;
                break;
            case State::In_which_module: 
                if(cat != Lexem_category::Scope_resolution){
                    sc_->back();
                    printf(expected_scope_resolution_fmt_name_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state    = State::Scope_res;
                break;
            case State::Scope_res:
                if(cat != Lexem_category::Ident){
                    sc_->back();
                    printf(expected_type_name_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state     = State::Embedded_or_id;
                type_name = li.ident_index;
                result = std::make_shared<Type_alias>(mod_name,
                                                      type_name, 
                                                      ref_order);
                break;
        }
    }
    return result;
}

std::shared_ptr<ast::Embedded_type> Parser::Impl::proc_embedded_type() // 8
{
    std::shared_ptr<ast::Embedded_type> result;
    enum class State{ //переменная состояний для вспомогательных переменных
        Start, Changer, Last_state
    }
    State           state   = State::Start;
    int             mod_power = 0;
    Main_lexem_code code;
    for(;;){
        Main_lexem_info li  = sc_->current_lexem(); //получаем текущую лексему
        Lexem_category  cat = get_lexem_category(li); //получаем категорию лексемы
        switch(state){
            case State::Start: //переход не по ключевому слову, а по категории лексем
                switch(cat){
                    case Lexem_category::Modified_type:
                        {
                            auto k = modified2embedded_kind(li.code, mod_power);
                            result = std::make_shared<Embedded_type>(k, 0);
                            state  = State::Last_state;
                        }                        
                        break;
                    case Lexem_category::Non_modified_type: //20 с чем-то ключевых слов - это одна категория
                        {
                            auto k = non_modified2embedded_kind(li.code);
                            result = std::make_shared<Embedded_type>(k, 0);
                            state  = State::Last_state;
                        }                        
                        break;
                    case Lexem_category::Size_changer:
                        if(li.code == Main_lexem_code::Kw_long){
                            ++mod_power;
                        }else{
                            --mod_power;
                        }  
                        state = State::Changer;
                        break;
                    default:
                        sc_->back();
                        printf(expected_embedded_type_fmt, 
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result; 
                }
                break;
            case State::Changer: 
                switch(cat){
                    case Lexem_category::Modified_type:
                        {
                            auto k = modified2embedded_kind(li.code, mod_power);
                            result = std::make_shared<Embedded_type>(k, 0);
                            state  = State::Last_state;
                        }                        
                        break;
                    case Lexem_category::Size_changer:
                        if(li.code == Main_lexem_code::Kw_long){
                            ++mod_power;
                        }else{
                            --mod_power;
                        }  
                        break;
                    default:
                        sc_->back();
                        printf(expected_embedded_type_fmt, 
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result; 
                }
                break;
            case State::Last_state:
                sc_->back();
                return result; 
        }
    }
    return result;
}

std::shared_ptr<ast::Type_def> Parser::Impl::proc_compound_type_definition() // 9
{
    std::shared_ptr<ast::Type_def> result;
    enum class State{
        Start,          Begin_tuple, Func, 
        Algebraic_part, Tuple_elem,  End_tuple, 
        Algebraic_sep
    };
    State state = State::Start;
    for(;;){
        Main_lexem_info li  = sc_->current_lexem();
        Lexem_category  cat = get_lexem_category(li);
        switch(state){
            case State::Start:   
                switch(cat){
                    case Lexem_category::Tuple_begin:
                        state = State::Begin_tuple;
                        break;
                    case Lexem_category::Kw_function:
                        state = State::Func;
                        break;
                    case Lexem_category::Kw_enum:
                    case Lexem_category::Kw_struct:
                        state = State::Algebraic_part;
                        break;
                    default:
                        ;
                }
                break;
            case State::Begin_tuple:
                if(cat != Lexem_category::ElType){
                    printf(expected_ElType_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Tuple_elem;
                break;
            case State::Func: 
                if(cat != Lexem_category::FuncSign){
                    printf(expected_FuncSign_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::End_tuple;
                break;
            case State::Algebraic_part: 
                if(cat != Lexem_category::DbarD){
                    sc_->back();                    
                    return result;
                }else{
                    state = State::Algebraic_sep;
                }
                break;
            case State::Tuple_elem:
                switch(cat){
                    case Lexem_category::Comma:
                        state = State::Begin_tuple;
                        break;
                    case Lexem_category::Col_cbr:
						state = State::End_tuple;
						break;
                    default:
                        printf(expected_enum_struct_fmt, 
                               sc_->lexem_Comma_Col_cbr());
                        et_.ec->increment_number_of_errors();
                        return result;
                }  
                break;
            case State::End_tuple:
                sc_->back();
                return result;
            case State::Algebraic_sep:
                if(cat != Lexem_category::Part_alg_type){
                    printf(expected_Part_alg_type_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Algebraic_part;
                break;
        }        
    }
    return result;
}

std::shared_ptr<ast::Algebraic_type_component> Parser::Impl::proc_alg_type_component() // 10
{
	std::shared_ptr<ast::Algebraic_type_component> result;
	enum class State{
		Start, Enum, Struct, FirstId, SecondId,
		FirstOpen, SecondOpen, Iden, Group, Finish
	};
	State state = State::Start;
	
	scope::Id_info     id_info;
	
	for(;;){
		Main_lexem_info li = sc_->current_lexem();
		Lexem_category cat = get_lexem_category(li);
		switch(state){
			case State::Start:
				switch(cat){
                    case Lexem_category::Kw_enum:
                        state = State::Enum;
                        break;
                    case Lexem_category::Kw_struct:
						state = State::Struct;
						break;
                    default:
                        printf(expected_enum_struct_fmt, 
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
                }
                break;
			case State::Enum:
				if(cat != Lexem_category::Ident){
                    printf(expected_ident_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::FirstId; 
                break;
			case State::Struct:
				if(cat != Lexem_category::Ident){
                    printf(expected_ident_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::SecondId; 
                break;
			case State::FirstId:
				if(cat != Lexem_category::Open_curly_bracket){
                    printf(expected_open_curly_bracket_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::FirstOpen; 
                break;
			case State::SecondId:
				if(cat != Lexem_category::Open_curly_bracket){
                    printf(expected_open_curly_bracket_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::SecondOpen; 
                break;
			case State::FirstOpen:
				if(cat != Lexem_category::Ident){
                    printf(expected_ident_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Iden; 
                break;
			case State::SecondOpen:
				if(cat != Lexem_category::Ident){
					sc_->back();
                    printf(expected_field_group_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Group; 
                break;
			case State::Iden:
				switch(cat){
                    case Lexem_category::Closed_curly_bracket:
						sc_->back();
                        state = State::Finish;
                        break;
                    case Lexem_category::Comma:
						state = State::FirstOpen;
						break;
                    default:
                        printf(expected_bracket_comma_fmt, 
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
                }
                break;
			case State::Group:
				switch(cat){
                    case Lexem_category::Closed_curly_bracket:
						sc_->back();
                        state = State::Finish;
                        break;
                    case Lexem_category::Semicolon:
						state = State::SecondOpen;
						break;
                    default:
                        printf(expected_bracket_semicolon_fmt, 
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
                }
                break;
			case State::Finish
                sc_->back();
                return result; 
		}
	}
	return result;
}

std::shared_ptr<ast::Enum_def> Parser::Impl::proc_enum_def() // 11
{
	std::shared_ptr<ast::Enum_def> result;
	enum class State{
		Start, Var, NewColon, Simplest_typedef
	};
	State state = State::Start;
	std::list<ast::Id>                      vars;
    std::shared_ptr<ast::Simplest_type_def> current_type;
    bool                                    correct_var_name;
	for(;;){
		Main_lexem_info li = sc_->current_lexem();
		Lexem_category cat = get_lexem_category(li);
		switch(state){
			case State::Start:
				if(cat != Lexem_category::Ident){
                    printf(expected_ident_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
				correct_var_name = check2_var_name(li.ident_index);
                if(!correct_var_name){
                    printf(uncorrect_var_name_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::Var; 
                break;
			case State::Var:
				switch(cat){
					case Lexem_category::Comma:
                        state = State::Start;
                        break;
					case Lexem_category::Colon:
						state = State::NewColon;
						break;
					default:
                        printf(expected_comma_colon_fmt,
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
				}
				break;
			case State::NewColon:
				switch(cat){
					case Lexem_category::Kw_array:
					case Lexem_category::At_sign:
					case Lexem_category::Ident:
					case Lexem_category::Module_name_prefix:
					case Lexem_category::Modified_type:
					case Lexem_category::Non_modified_type:
					case Lexem_category::Size_changer:
                        sc_->back();
                        current_type = proc_simplest_type_def();
                        state = State::Simplest_typedef;
                        break;
					default:
						sc_->back();
						printf(expected_form_var_fmt, 
								sc_->lexem_begin_line_number());
						et_.ec->increment_number_of_errors();
						return result;	
				}
			case State::Simplest_typedef
                sc_->back();
                return result; 
		}
	}
	return result;
}

std::shared_ptr<ast::Func_ptr> Parser::Impl::proc_func_ptr() // 12
{
	std::shared_ptr<ast::Func_ptr> result;
	enum class State{
		Start, RoundOpen, FormArg, RoundClose, Semi,
		Colon, Finish
	};
	
	std::shared_ptr<Elementary_type> elem_type;
	std::shared_ptr<Formal_arg_info> form_var;
	
	State state = State::Start;
	for(;;){
		Main_lexem_info li = sc_->current_lexem();
		Lexem_category cat = get_lexem_category(li);
		switch(state){
			case State::Start:
				if(cat != Lexem_category::Open_round_bracket){
                    printf(expected_Open_round_bracket_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::RoundOpen; 
                break;
			case State::RoundOpen:
				switch(cat){
					case Lexem_category::Kw_value:
					case Lexem_category::Kw_ref:
					case Lexem_category::Kw_const:
						sc_->back();
						form_var = proc_form_var();
						state = State::FormArg;
						break;
					case Lexem_category::Closed_round_bracket:
						state = State::RoundClose;
						break;
					default:
                        printf(expected_value_ref_const_bracket_fmt,
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
				}
				break;
			case State::FormArg:
				switch(cat){
					case Lexem_category::Semicolon:
						state = State::Semi;
						break;
					case Lexem_category::Closed_round_bracket:
						state = State::RoundClose;
						break;
					default:
                        printf(expected_semicolon_bracket_fmt,
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
				}
				break;
			case State::RoundClose:
				if(cat != Lexem_category::Colon){
                    printf(expected_colon_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Colon; 
                break;
			case State::Semi:
				switch(cat){
					case Lexem_category::Kw_value:
					case Lexem_category::Kw_ref:
					case Lexem_category::Kw_const:
						sc_->back();
						form_var = proc_form_var();
						state = State::FormArg;
						break;
					default:
                        printf(expected_value_ref_const_fmt,
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
				}
				break;
			case State::Colon:
				switch(cat){
					case Lexem_category::At_sign:
                    case Lexem_category::Ident:
                    case Lexem_category::Module_name_prefix:
                    case Lexem_category::Modified_type:
                    case Lexem_category::Non_modified_type:
                    case Lexem_category::Size_changer:
                        sc_->back();
                        elem_type = proc_elementary_type();
                        state     = State::Finish;                        
                        break;
					default:
                        printf(expected_elementary_fmt,
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
				}
				break;
			case State::Five
                sc_->back();
                return result; 
		}
	}
	return result;
}

std::shared_ptr<ast::Formal_arg_info> Parser::Impl::proc_form_var() // 13
{
	std::shared_ptr<ast::Formal_arg_info> result;
	enum class State{
		Start, Val, Second, Third, Fourth, Five
	};
	State state = State::Start;
	std::list<ast::Id>                      vars;
    std::shared_ptr<ast::Simplest_type_def> current_type;
    bool                                    correct_var_name;
	for(;;){
		Main_lexem_info li = sc_->current_lexem();
		Lexem_category cat = get_lexem_category(li);
		switch(state){
			case State::Start:
				switch(cat){
					case Lexem_category::Kw_value:
					case Lexem_category::Kw_ref:
                        state = State::Val;
                        break;
					case Lexem_category::Kw_const:
						state = State::Second;
						break;
					default:
                        printf(expected_value_ref_const_fmt,
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
				}
				break;
			case State::Val:
				if(cat != Lexem_category::Ident){
                    printf(expected_ident_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
				correct_var_name = check2_var_name(li.ident_index);
                if(!correct_var_name){
                    printf(uncorrect_var_name_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::Third; 
                break;
			case State::Second:
				if(cat != Lexem_category::Kw_ref){
                    printf(expected_Kw_ref_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Val; 
                break;
			case State::Third:
				switch(cat){
					case Lexem_category::Comma:
						state = State::Val;
						break;
					case Lexem_category::Colon:
						state = State::Fourth;
						break;
					default:
                        printf(expected_comma_colon_fmt,
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
				}
				break;
			case State::Fourth:
				switch(cat){
					case Lexem_category::Kw_array:
					case Lexem_category::At_sign:
					case Lexem_category::Ident:
					case Lexem_category::Module_name_prefix:
					case Lexem_category::Modified_type:
					case Lexem_category::Non_modified_type:
					case Lexem_category::Size_changer:
                        sc_->back();
                        current_type = proc_simplest_type_def();
                        state = State::Five;
                        break;
					default:
						sc_->back();
						printf(expected_form_var_fmt, 
								sc_->lexem_begin_line_number());
						et_.ec->increment_number_of_errors();
						return result;	
				}
                break;
			case State::Five
                sc_->back();
                return result; 
		}
	}
	return result;
}

std::list<std::shared_ptr<ast::Definition>> Parser::Impl::proc_func_proto() // 14
{
    std::list<std::shared_ptr<ast::Definition>> result;
	enum class State{
		Start, Fun, Name, Sign_func, Semi, 
		Open_curly, Block
	};
	State state = State::Start;
	ast::Id   current_name = 0;
	ast::Module_block   block;
	std::shared_ptr<ast::Func_ptr>   func;
    for(;;){
        Main_lexem_info li  = sc_->current_lexem();
        Lexem_category  cat = get_lexem_category(li);
        switch(state){
            case State::Start:
                if(cat != Lexem_category::Kw_function){
                    printf(expected_function_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::Fun;
                break;
			case State::Fun:
				if(cat != Lexem_category::Ident){
                    printf(expected_ident_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::Name;
                break;
			case State::Name:
				switch(cat){
					case Lexem_category::Open_round_bracket:
						sc_->back();
						func = proc_func_ptr();
						state = State::Sign_func;
						break;
					default:
                        printf(expected_open_round_bracket_fmt,
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
				}
				break;
			case State::Sign_func:
				switch(cat){
					case Lexem_category::Semicolon:
						sc_->back();
						state = State::Semi;
						break;
					case Lexem_category::Open_curly_bracket:
						state = State::Open_curly;
						break;
					default:
                        printf(expected_semicolon_bracket_fmt,
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
				}
				break;
			case State::Semi:
				sc_->back();
                return result;
			case State::Open_curly:
				switch(cat){
					case Lexem_category::Kw_var:
                    case Lexem_category::Kw_type:
                    case Lexem_category::Kw_function:
                    case Lexem_category::Kw_const:
					case Lexem_category::Semicolon:
					case Lexem_category::Label_prefix:
                    case Lexem_category::Kw_for:
                    case Lexem_category::Kw_while:
                    case Lexem_category::Kw_repeat:
					case Lexem_category::Module_name_prefix:
                    case Lexem_category::Ident:
                    case Lexem_category::Kw_exit:
                    case Lexem_category::Kw_read:
					case Lexem_category::Kw_print:
                    case Lexem_category::Kw_if:
                    case Lexem_category::Kw_match:
						sc_->back();
						state = State::Block;
						break;
					default:
                        printf(expected_block_body_fmt,
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
				}
				break;
			case State::Block:
				if(cat != Lexem_category::Closed_curly_bracket){
                    printf(expected_closed_curly_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
				sc_->back();
                state = State::Semi;
                break;
		}
	}
	return result;
}

std::list<std::shared_ptr<ast::Constant>> Parser::Impl::proc_const() // 15
{
    std::list<std::shared_ptr<ast::Constant>> result;
	enum class State{
		Start, Const, Name, Colon, Simplest_typedef, 
		Equal, Finish
	};
	State state = State::Start;
	ast::Id   current_name = 0;
	std::shared_ptr<ast::Simplest_type_def> current_type;
	std::shared_ptr<ast::Expr> expression;
    for(;;){
        Main_lexem_info li  = sc_->current_lexem();
        Lexem_category  cat = get_lexem_category(li);
        switch(state){
            case State::Start:
                if(cat != Lexem_category::Kw_const){
                    printf(expected_const_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::Const;
                break;
			case State::Const:
				if(cat != Lexem_category::Ident){
                    printf(expected_ident_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::Name;
                break;
			case State::Name:
				if(cat != Lexem_category::Colon){
                    printf(expected_colon_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::Colon;
                break;
			case State::Colon:
				switch(cat){
					case Lexem_category::Kw_array:
					case Lexem_category::At_sign:
					case Lexem_category::Ident:
					case Lexem_category::Module_name_prefix:
					case Lexem_category::Modified_type:
					case Lexem_category::Non_modified_type:
					case Lexem_category::Size_changer:
                        sc_->back();
                        current_type = proc_simplest_type_def();
                        state = State::Simplest_typedef;
                        break;
					default:
						printf(expected_form_var_fmt, 
								sc_->lexem_begin_line_number());
						et_.ec->increment_number_of_errors();
						return result;	
				}
				break;
			case State::Simplest_typedef:
				if(cat != Lexem_category::Equal_sign){
                    printf(expected_colon_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::Equal;
                break;
			case State::Equal:
				switch(cat){
					case Lexem_category::Logical_not:
					case Lexem_category::Bitwise_not:
					case Lexem_category::Plus:
					case Lexem_category::Minus:
					case Lexem_category::Sizeof:
					case Lexem_category::Kw_int:
					case Lexem_category::Kw_float:
					case Lexem_category::Kw_string:
					case Lexem_category::Kw_true:
					case Lexem_category::Kw_false:
					case Lexem_category::Open_round_bracket:
					case Lexem_category::Ident:
					case Lexem_category::Module_name_prefix:
					case Lexem_category::Kw_array:
                        sc_->back();
                        expression = proc_expr();
                        state = State::Finish;
                        break;
					default:
						sc_->back();
						printf(expected_form_var_fmt, 
								sc_->lexem_begin_line_number());
						et_.ec->increment_number_of_errors();
						return result;	
				}
				break;
			case State::Finish:
				if(cat != Lexem_category::Comma){
                    printf(expected_colon_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
				sc_->back();
                state = State::Const;
                break;
		}
	}
    return result;
}

std::shared_ptr<ast::Binary_op > Parser::Bin::input() // 16
{
	std::shared_ptr<ast::Binary_op > result;
	enum class State{
		Start, Second, Third, Fourth, Five, Six

	};
	State state = State::Start;
	std::shared_ptr<ast::Simplest_type_def> current_type;
	
	for(;;){
		Main_lexem_info li = sc_->current_lexem();
		Lexem_category cat = get_lexem_category(li);
		switch(state){
			case State::Start:
				if(cat != Lexem_category::expr){
                    printf(expected_expr_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Second; 
                break;
			case State::Second:
				if(cat != Lexem_category::quest){
                    printf(expected_quest_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Third; 
                break;
			case State::Third:
				if(cat != Lexem_category::expr){
                    printf(expected_expr_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Fourth; 
                break;
case State::Fourth:
				if(cat != Lexem_category::dpoints){
                    printf(expected_dpoints_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Five; 
break;

case State::Five:
				if(cat != Lexem_category::expr){
                    printf(expected_expr_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Six; 
				sc_->back();
                return result; 
		}
	}
	return result;
}

std::shared_ptr<ast::Binary_op > Parser::Bin::input() // 17
{
	std::shared_ptr<ast::Binary_op > result;
	enum class State{
		Start, Second,
	};
	State state = State::Start;
	std::shared_ptr<ast::Simplest_type_def> current_type;
	
	for(;;){
		Main_lexem_info li = sc_->current_lexem();
		Lexem_category cat = get_lexem_category(li);
		switch(state){
			case State::Start:
				if(cat != Lexem_category::expr){
                    printf(expected_expr_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Second; 
                break;
			case State::Second:
            case Lexem_category::norm:
            case Lexem_category::onorm:
            case Lexem_category::br:
            case Lexem_category::obr:
                if(cat != Lexem_category::norm){
                    if(cat != Lexem_category::onorm){
                        if(cat != Lexem_category::br){
                            if(cat != Lexem_category::obr){
                                return result;
                            }else{
                                sc_->back();                    
                            }
                            break;
                        }
                    }
                }
	        return result;
        }
}       

std::shared_ptr<ast::Binary_op > Parser::Bin::input() // 18
{
    std::shared_ptr<ast::Binary_op > result;
    enum class State{
        Start, Second
    };
    State state = State::Start;
    std::shared_ptr<ast::Simplest_type_def> current_type;
    for(;;){
        Main_lexem_info li = sc_->current_lexem();
        Lexem_category cat = get_lexem_category(li);
        switch(state){
            case State::Start:
                if(cat != Lexem_category::expr2){
                printf(expected_expr2_fmt, sc_->lexem_begin_line_number());
                et_.ec->increment_number_of_errors();
                return result;
            }
            state = State::Second;
            break;
            case State::Second:
            case Lexem_category::and:
            case Lexem_category::oand:
                if(cat != Lexem_category::and){
                    if(cat != Lexem_category::oand){
                        return result;
                    }else{
                        sc_->back();
                    }
                    break;
                }
                return result;
        }

std::shared_ptr<ast::Unary_op> Parser::Impl::proc_lnot() { // 19
	std::shared_ptr<ast::Unary_op> result;
	enum class State{
		Start, Final
	};
	
	// счетчик операторов логич. отрицания
	int count = 0;
	// начинаем в состоянии Start
	State state = State::Start;
	// соответствует выр3 в 19-м ДКА
	ast::Expr expr3;
	while(true) {
		Main_lexem_info li = sc_->current_lexem(); //получаем текущую лексему
		Lexem_category cat = get_lexem_category(li); //получаем категорию лексемы
		
		switch(cat) {
			case Lexem_category::Logical_not : // пока что подсчитываем кол-во тильд и остаемся в первом состоянии
					count++;
					break;
			// как только считали выражение, переходим в финальное состояние
			case Lexem_category::Integer :
			case Lexem_category::Ident :
					expr7 = proc_expr(); // получаем выражение типа Expr
					state = Final;
					break;
			default :
					printf("Error at line %zu: expected sign of logical negation or expression.\n", 
                             sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                    return result;
						
		}
	
		if(state == Final)
			break;
	}
	
	result.arg_ = expr3;
	if(count % 2 == 0) \\ если чётное число лог.отрицаний, то будет просто выражение
		result.kind_ = ast::Unary_op_kind::Plus
	else
		result.kind_ = ast::Unary_op_kind::Lnot;
		
	sc_->back();
		
	return result;
}

std::shared_ptr<ast::Binary_op> Parser::Impl::proc_relat_oper() { // 20
	std::shared_ptr<ast::Binary_op> result;
	// вспомогательный указатель
	std::shared_ptr<ast::Binary_op> tmp;
	enum class State{
		Start, Final
	};
	
	// начинаем в состоянии Start
	State state = State::Start;
	
	//для обозначения наличия еще одного операнда
	int count = 0;
	
	Lexem_category cat;
	result.kind_ = ast::Binary_op_kind::Add; // в случае когда лишь один операнд будет 
	
	while(true) {
		Main_lexem_info li = sc_->current_lexem(); //получаем текущую лексему
		cat = get_lexem_category(li); //получаем категорию лексемы
		
		switch(state) {
			case Start :
				switch(cat) {
					// как только считали выражение, переходим в финальное состояние
					case Lexem_category::Integer :
					case Lexem_category::Ident :
						count++; // увеличиваем счетчик выр8
						if(count == 1) { // если это первое встретившееся выражение
							result.lhs_ = proc_expr(); // получаем выражение типа Expr
						} else if(count == 2){ // если это второе встретившееся выражение
							result.rhs_ = proc_expr(); // получаем выражение типа Expr
						} else { /* например, если обрабатываем выражение вида exp1 < expr2 <= expr3 
									то сначала result такие значения полей имеет: lhs_ = expr1, rhs_ = expr2, kind_ = ast::Binary_op_kind::Less.
									Когда мы обрабатываем третье выражение (expr3), то result будет иметь вид такой:
									lhs_ = "такой result, который представляет собой exp1 + expr2", rhs_ = expr3, kind_ = ast::Binary_op_kind::Leq
								*/
							tmp.lhs_ = result; // сохраняем предыдущее выражение с суммами/разностями как левый операнд. 
							tmp.rhs_ = proc_expr();
							tmp.kind_ = result.kind_; // знак известен, т.к. вернулись из состояния Final
							
							result = tmp;
						}
						
						state = Final;
						break;
					default :
						printf("Error at line %zu: expected expression.\n", 
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
						
				}
				break;
				
			case Final : 
				switch(cat) {
					// продумать случай, когда больше ничего нет
					// например, когда дошли до ; или ) или ,
					case Lexem_category::Semicolon :
					case Lexem_category::Closed_round_bracket :
					case Lexem_category::Comma :
						sc_->back();
						return result;
				
					case Lexem_category::Less_than : 
						result.kind_ = ast::Binary_op_kind::Less;
						
						state = Start;
						break;
					case Lexem_category::Lower_or_equals :
						result.kind_ = ast::Binary_op_kind::Leq;
						
						state = Start;
						break;
						
					case Lexem_category::Greater_than : 
						result.kind_ = ast::Binary_op_kind::Greater;
						
						state = Start;
						break;
						
					case Lexem_category::Greater_or_equals :
						result.kind_ = ast::Binary_op_kind::Geq;
						
						state = Start;
						break;
						
					case Lexem_category::Equal_sign : 
						result.kind_ = ast::Binary_op_kind::Equals;
						
						state = Start;
						break;
					case Lexem_category::Not_equals :
						result.kind_ = ast::Binary_op_kind::Not_equals;
						
						state = Start;
						break;
						
					default :
						printf("Error at line %zu: expected signs of relational operators.\n", 
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
				}
			default :
				
				sc_->back();
				return result;
			}
	}
	
	return result;
}

std::shared_ptr<ast::Binary_op> Parser::Impl::proc_bitwise_opers() { // 21
	std::shared_ptr<ast::Binary_op> result;
	// вспомогательный указатель
	std::shared_ptr<ast::Binary_op> tmp;
	enum class State{
		Start, Final
	};
	
	// начинаем в состоянии Start
	State state = State::Start;
	
	//для обозначения наличия еще одного операнда
	int count = 0;
	
	Lexem_category cat;
	result.kind_ = ast::Binary_op_kind::Add; // в случае когда лишь один операнд будет 
	
	while(true) {
		Main_lexem_info li = sc_->current_lexem(); //получаем текущую лексему
		cat = get_lexem_category(li); //получаем категорию лексемы
		
		switch(state) {
			case Start :
				switch(cat) {
					// как только считали выражение, переходим в финальное состояние
					case Lexem_category::Integer :
					case Lexem_category::Ident :
						count++; // увеличиваем счетчик выр8
						if(count == 1) { // если это первое встретившееся выражение
							result.lhs_ = proc_expr(); // получаем выражение типа Expr
						} else if(count == 2){ // если это второе встретившееся выражение
							result.rhs_ = proc_expr(); // получаем выражение типа Expr
						} else { /* например, если обрабатываем выражение вида exp1 | expr2 ^ expr3 
									то сначала result такие значения полей имеет: lhs_ = expr1, rhs_ = expr2, kind_ = ast::Binary_op_kind::Bor.
									Когда мы обрабатываем третье выражение (expr3), то result будет иметь вид такой:
									lhs_ = "такой result, который представляет собой exp1 | expr2", rhs_ = expr3, kind_ = ast::Binary_op_kind::Bxor.
								*/
							tmp.lhs_ = result; // сохраняем предыдущее выражение с суммами/разностями как левый операнд. 
							tmp.rhs_ = proc_expr();
							tmp.kind_ = result.kind_; // знак известен, т.к. вернулись из состояния Final
							
							result = tmp;
						}
						
						state = Final;
						break;
					default :
						printf("Error at line %zu: expected expression.\n", 
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
						
				}
				break;
				
			case Final : 
				switch(cat) {
					// продумать случай, когда больше ничего нет
					// например, когда дошли до ; или ) или ,
					case Lexem_category::Semicolon :
					case Lexem_category::Closed_round_bracket :
					case Lexem_category::Comma :
						sc_->back();
						return result;
				
					case Lexem_category::Bitwise_or : 
						result.kind_ = ast::Binary_op_kind::Bor;
						
						state = Start;
						break;
					case Lexem_category::Bitwise_or_not :
						result.kind_ = ast::Binary_op_kind::Bor_not;
						
						state = Start;
						break;
					
					case Lexem_category::Bitwise_xor : 
						result.kind_ = ast::Binary_op_kind::Bxor;
						
						state = Start;
						break;
					case Lexem_category::Bitwise_xor_not :
						result.kind_ = ast::Binary_op_kind::Bxor_not;
						
						state = Start;
						break;
						
					default :
						printf("Error at line %zu: expected signs of bitwise operators: or, or_not, xor, xor_not.\n", 
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
				}
			default :
				
				sc_->back();
				return result;
			}
	}
	
	return result;
}

std::shared_ptr<ast::Binary_op> Parser::Impl::proc_bitwise_shifts_and() { // 22
	std::shared_ptr<ast::Binary_op> result;
	// вспомогательный указатель
	std::shared_ptr<ast::Binary_op> tmp;
	enum class State{
		Start, Final
	};
	
	// начинаем в состоянии Start
	State state = State::Start;
	
	//для обозначения наличия еще одного операнда
	int count = 0;
	
	Lexem_category cat;
	result.kind_ = ast::Binary_op_kind::Add; // в случае когда лишь один операнд будет 
	
	while(true) {
		Main_lexem_info li = sc_->current_lexem(); //получаем текущую лексему
		cat = get_lexem_category(li); //получаем категорию лексемы
		
		switch(state) {
			case Start :
				switch(cat) {
					// как только считали выражение, переходим в финальное состояние
					case Lexem_category::Integer :
					case Lexem_category::Ident :
						count++; // увеличиваем счетчик выр6
						if(count == 1) { // если это первое встретившееся выражение
							result.lhs_ = proc_expr(); // получаем выражение типа Expr
						} else if(count == 2){ // если это второе встретившееся выражение
							result.rhs_ = proc_expr(); // получаем выражение типа Expr
						} else { /* например, если обрабатываем выражение вида exp1 & expr2 >> expr3 
									то сначала result такие значения полей имеет: lhs_ = expr1, rhs_ = expr2, kind_ = ast::Binary_op_kind::Rshift.
									Когда мы обрабатываем третье выражение (expr3), то result будет иметь вид такой:
									lhs_ = "такой result, который представляет собой exp1 & expr2", rhs_ = expr3, kind_ = ast::Binary_op_kind::Bxor.
								*/
							tmp.lhs_ = result; // сохраняем предыдущее выражение с суммами/разностями как левый операнд. 
							tmp.rhs_ = proc_expr();
							tmp.kind_ = result.kind_; // знак известен, т.к. вернулись из состояния Final
							
							result = tmp;
						}
						
						state = Final;
						break;
					default :
						printf("Error at line %zu: expected expression.\n", 
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
						
				}
				break;
				
			case Final : 
				switch(cat) {
					// продумать случай, когда больше ничего нет
					// например, когда дошли до ; или ) или ,
					case Lexem_category::Semicolon :
					case Lexem_category::Closed_round_bracket :
					case Lexem_category::Comma :
						sc_->back();
						return result;
				
					case Lexem_category::Bitwise_and : 
						result.kind_ = ast::Binary_op_kind::Band;
						
						state = Start;
						break;
					case Lexem_category::Bitwise_and_not :
						result.kind_ = ast::Binary_op_kind::Band_not;
						
						state = Start;
						break;
					
					case Lexem_category::Left_shift : 
						result.kind_ = ast::Binary_op_kind::Lshift;
						
						state = Start;
						break;
					case Lexem_category::Right_shift :
						result.kind_ = ast::Binary_op_kind::Rshift;
						
						state = Start;
						break;
					
					case Lexem_category::Signed_right_shiftt :
						result.kind_ = ast::Binary_op_kind::SRshift;
						
						state = Start;
						break;
						
					default :
						printf("Error at line %zu: expected signs of bitwise shifts and bitwise and.\n", 
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
				}
			default :
				
				sc_->back();
				return result;
			}
	}
	
	return result;
}

std::shared_ptr<ast::Unary_op> Parser::Impl::proc_bnot() { // 23
	std::shared_ptr<ast::Unary_op> result;
	enum class State{
		Start, Final
	};
	
	// счетчик операторов побитового отрицания
	int count = 0;
	// начинаем в состоянии Start
	State state = State::Start;
	// соответствует выр7 в 23-м ДКА
	ast::Expr expr7;
	while(true) {
		Main_lexem_info li = sc_->current_lexem(); //получаем текущую лексему
		Lexem_category cat = get_lexem_category(li); //получаем категорию лексемы
		
		switch(cat) {
			case Lexem_category::Bitwise_not : // пока что подсчитываем кол-во тильд и остаемся в первом состоянии
					count++;
					break;
			// как только считали выражение, переходим в финальное состояние
			case Lexem_category::Integer :
			case Lexem_category::Ident :
					expr7 = proc_expr(); // получаем выражение типа Expr
					state = Final;
					break;
			default :
					printf("Error at line %zu: expected sign of bitwise negation or expression.\n", 
                             sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                    return result;
						
		}
	
		if(state == Final)
			break;
	}
	
	result.arg_ = expr7;
	if(count % 2 == 0) \\ если чётное число тильд, то будет просто выражение
		result.kind_ = ast::Unary_op_kind::Plus
	else
		result.kind_ = ast::Unary_op_kind::Bnot;
		
	sc_->back();
		
	return result;
}

std::shared_ptr<ast::Binary_op> Parser::Impl::proc_add_sub() { // 24
	std::shared_ptr<ast::Binary_op> result;
	// вспомогательный указатель
	std::shared_ptr<ast::Binary_op> tmp;
	enum class State{
		Start, Final
	};
	
	// начинаем в состоянии Start
	State state = State::Start;
	
	//для обозначения наличия еще одного операнда
	int count = 0;
	
	Lexem_category cat;
	result.kind_ = ast::Binary_op_kind::Add; // в случае когда лишь один операнд будет 
	
	while(true) {
		Main_lexem_info li = sc_->current_lexem(); //получаем текущую лексему
		cat = get_lexem_category(li); //получаем категорию лексемы
		
		switch(state) {
			case Start :
				switch(cat) {
					// как только считали выражение, переходим в финальное состояние
					case Lexem_category::Integer :
					case Lexem_category::Ident :
						count++; // увеличиваем счетчик выр8
						if(count == 1) { // если это первое встретившееся выражение
							result.lhs_ = proc_expr(); // получаем выражение типа Expr
						} else if(count == 2){ // если это второе встретившееся выражение
							result.rhs_ = proc_expr(); // получаем выражение типа Expr
						} else { /* например, если обрабатываем выражение вида exp1 + expr2 - expr3 
									то сначала result такие значения полей имеет: lhs_ = expr1, rhs_ = expr2, kind_ = ast::Binary_op_kind::Add.
									Когда мы обрабатываем третье выражение (expr3), то result будет иметь вид такой:
									lhs_ = "такой result, который представляет собой exp1 + expr2", rhs_ = expr3, kind_ = ast::Binary_op_kind::Sub
								*/
							tmp.lhs_ = result; // сохраняем предыдущее выражение с суммами/разностями как левый операнд. 
							tmp.rhs_ = proc_expr();
							tmp.kind_ = result.kind_; // знак известен, т.к. вернулись из состояния Final
							
							result = tmp;
						}
						
						state = Final;
						break;
					default :
						printf("Error at line %zu: expected expression.\n", 
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
						
				}
				break;
				
			case Final : 
				switch(cat) {
					// продумать случай, когда больше ничего нет
					// например, когда дошли до ; или ) или ,
					case Lexem_category::Semicolon :
					case Lexem_category::Closed_round_bracket :
					case Lexem_category::Comma :
						sc_->back();
						return result;
				
					case Lexem_category::Plus : 
						result.kind_ = ast::Binary_op_kind::Add;
						
						state = Start;
						break;
					case Lexem_category::Minus :
						result.kind_ = ast::Binary_op_kind::Sub;
						
						state = Start;
						break;
					default :
						printf("Error at line %zu: expected plus or minus.\n", 
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
				}
			default :
				
				sc_->back();
				return result;
			}
	}
	
	return result;
}

std::shared_ptr<ast::Binary_op> Parser::Impl::proc_mult_div_rmndr_fdiv() { // 25
	std::shared_ptr<ast::Binary_op> result;
	// вспомогательный указатель
	std::shared_ptr<ast::Binary_op> tmp;
	enum class State{
		Start, Final
	};
	
	// начинаем в состоянии Start
	State state = State::Start;
	
	//для обозначения наличия еще одного операнда
	int count = 0;
	
	Lexem_category cat;
	result.kind_ = ast::Binary_op_kind::Add; // в случае когда лишь один операнд будет 
	
	while(true) {
		Main_lexem_info li = sc_->current_lexem(); //получаем текущую лексему
		cat = get_lexem_category(li); //получаем категорию лексемы
		
		switch(state) {
			case Start :
				switch(cat) {
					// как только считали выражение, переходим в финальное состояние
					case Lexem_category::Integer :
					case Lexem_category::Ident :
						count++; // увеличиваем счетчик выр8
						if(count == 1) { // если это первое встретившееся выражение
							result.lhs_ = proc_expr(); // получаем выражение типа Expr
						} else if(count == 2){ // если это второе встретившееся выражение
							result.rhs_ = proc_expr(); // получаем выражение типа Expr
						} else { /* например, если обрабатываем выражение вида exp1 * expr2 / expr3 
									то сначала result такие значения полей имеет: lhs_ = expr1, rhs_ = expr2, kind_ = ast::Binary_op_kind::Mul.
									Когда мы обрабатываем третье выражение (expr3), то result будет иметь вид такой:
									lhs_ = "такой result, который представляет собой exp1 * expr2", rhs_ = expr3, kind_ = ast::Binary_op_kind::Div
								*/
							tmp.lhs_ = result; // сохраняем предыдущее выражение с суммами/разностями как левый операнд. 
							tmp.rhs_ = proc_expr();
							tmp.kind_ = result.kind_; // знак известен, т.к. вернулись из состояния Final
							
							result = tmp;
						}
						
						state = Final;
						break;
					default :
						printf("Error at line %zu: expected expression.\n", 
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
						
				}
				break;
				
			case Final : 
				switch(cat) {
					// продумать случай, когда больше ничего нет
					// например, когда дошли до ; или ) или ,
					case Lexem_category::Semicolon :
					case Lexem_category::Closed_round_bracket :
					case Lexem_category::Comma :
						sc_->back();
						return result;
				
					case Lexem_category::Mul : 
						result.kind_ = ast::Binary_op_kind::Mul;
						
						state = Start;
						break;
					case Lexem_category::Div :
						result.kind_ = ast::Binary_op_kind::Div;
						
						state = Start;
						break;
					case Lexem_category::Fdiv : 
						result.kind_ = ast::Binary_op_kind::Fdiv;
						
						state = Start;
						break;
					case Lexem_category::Mod : //
						result.kind_ = ast::Binary_op_kind::Mod;
						
						state = Start;
						break;
					default :
						printf("Error at line %zu: expected signs of multiplication, division, remainder, float-point division.\n", 
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
				}
			default :
				
				sc_->back();
				return result;
			}
	}
	
	return result;
}

std::shared_ptr<ast::Binary_op> Parser::Impl::prog_expr_nine() // 26
{
	std::shared_ptr<ast::Binary_op> result;
	enum class State{
		Start, Second, Third, Fourth
	};
	State state = State::Start;
	std::shared_ptr<ast::Unary_op> expr10;
	std::shared_ptr<ast::Unary_op> expr9;
	
	
	for(;;){
		Main_lexem_info li = sc_->current_lexem();
		Lexem_category cat = get_lexem_category(li);
		switch(state){
			case State::Start:
				if(cat != Lexem_category::Exp){
                    printf(expected_Exp_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Second; 
                break;
			case State::Second:
				if(cat != Lexem_category::Dmuls){
                    printf(expected_Dmuls_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Third; 
                break;
			case State::Third:
				if(cat != Lexem_category::Exp){
                    printf(expected_Exp_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Fourth; 
                break;
			case State::Fourth:
				sc_->back();
                return result; 
		}
	}
	return result;
}

std::shared_ptr<ast::Unary_op> Parser::Impl::proc_expr_ten() // 27
{
	std::shared_ptr<ast::Unary_op> result;
	enum class State{
		Start, Val, Second
	};
	State state = State::Start;
	std::shared_ptr<ast::Binary_op> expr11;
	for(;;){
		Main_lexem_info li = sc_->current_lexem();
		Lexem_category cat = get_lexem_category(li);
		switch(state){
			case State::Start:
				switch(cat){
					case Lexem_category::Plus:
					case Lexem_category::Minus:
					case Lexem_category::Sizeof:
						state = State::Val;
						break;
					case Lexem_category::Kw_const:
					case Lexem_category::Kw_int:
					case Lexem_category::Kw_float:
					case Lexem_category::Kw_true:
					case Lexem_category::Kw_false:
					case Lexem_category::Kw_string:
					case Lexem_category::Open_round_bracket:
					case Lexem_category::Ident:
					case Lexem_category::Kw_array:
					case Lexem_category::Module_name_prefix:
						sc_->back();
						expr11 = proc_expr_eleven();
						state = State::Second;
						break;
					default:
						printf(expected_plus_minus_sizeof_expression_fmt,
							sc_->lexem_begin_line_number());
						et_.ec->increment_number_of_errors();
					return result;
				}
				break
			case State::Val:
				if( cat != Lexem_category::Ident ) {
					printf( expected_ident_fmt, sc->lexem_begin_line_number() );
					et_.ec->increment_number_of_errors();
					return result;
				}
				state = State::Second;
				break;
			case State::Second:
				sc_->back();
				return result; 
		}
	}
	return result;
}

std::shared_ptr<ast::Binary_op> Parser::Impl::proc_expr_eleven() // 28
{
	std::shared_ptr<ast::Binary_op> result;
	enum class State{
		Start, Twelve, Size, Finish
	};
	State state = State::Start;
	std::shared_ptr<ast::Name_component> expr;
	for(;;){
		Main_lexem_info li = sc_->current_lexem();
		Lexem_category cat = get_lexem_category(li);
		switch(state){
			case State::Start:
				switch(cat){
					case Lexem_category::Kw_int:
					case Lexem_category::Kw_float:
					case Lexem_category::Kw_string:
					case Lexem_category::Kw_true:
					case Lexem_category::Kw_false:
					case Lexem_category::Open_round_bracket:
					case Lexem_category::Ident:
					case Lexem_category::Module_name_prefix:
					case Lexem_category::Kw_array:
                        sc_->back();
                        expr = proc_expr_twelve();
                        state = State::Twelve;
                        break;
					default:
						printf(expected_expr_twelve_fmt, 
								sc_->lexem_begin_line_number());
						et_.ec->increment_number_of_errors();
						return result;	
				}
				break;
			case State::Twelve:
				if(cat != Lexem_category::Dimension_size){
                    printf(expected_dim_size_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
				sc_->back();
                state = State::Size;
                break;
			case State::Size:
				switch(cat){
					case Lexem_category::Kw_int:
					case Lexem_category::Kw_float:
					case Lexem_category::Kw_string:
					case Lexem_category::Kw_true:
					case Lexem_category::Kw_false:
					case Lexem_category::Open_round_bracket:
					case Lexem_category::Ident:
					case Lexem_category::Module_name_prefix:
					case Lexem_category::Kw_array:
                        sc_->back();
                        expr = proc_expr_twelve();
                        state = State::Finish;
                        break;
					default:
						printf(expected_expr_twelve_fmt, 
								sc_->lexem_begin_line_number());
						et_.ec->increment_number_of_errors();
						return result;	
				}
				break;
			case State::Finish:
				sc_->back();
                return result; 
		}
	}
	return result;
}

std::shared_ptr<ast::Loop_stmt> Parser::Impl::proc_loop_stmt() // 31
{
	std::shared_ptr<ast::Loop_stmt> result;
	enum class State{
		Start, Prefix, Loop, Id, Suffix
	};
	State state = State::Start;
	size_t loop_label_ = 0;
	for(;;){
		Main_lexem_info li = sc_->current_lexem();
		Lexem_category cat = get_lexem_category(li);
		switch(state){
			case State::Start:
				switch(cat){
					case Lexem_category::Label_prefix:
                        state = State::Prefix;
                        break;
					case Lexem_category::Kw_spider:
					case Lexem_category::Kw_repeat:
					case Lexem_category::Kw_while:
					case Lexem_category::Kw_for:
						sc_->back();
						state = State::Loop;
						break;
					default:
						printf(expected_prefix_loop_fmt, 
								sc_->lexem_begin_line_number());
						et_.ec->increment_number_of_errors();
						return result;	
				}
				break;
			case State::Prefix:
				if(cat != Lexem_category::Ident){
                    printf(expected_ident_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::Id;
                break;
			case State::Loop:
				sc_->back();
                return result;
			case State::Id:
				if(cat != Lexem_category::Label_suffix){
                    printf(expected_label_suffix_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::Suffix;
                break;
			case State::Suffix:
				switch(cat){
					case Lexem_category::Kw_spider:
					case Lexem_category::Kw_repeat:
					case Lexem_category::Kw_while:
					case Lexem_category::Kw_for:
						sc_->back();
						state = State::Loop;
						break;
					default:
						printf(expected_loop_fmt, 
								sc_->lexem_begin_line_number());
						et_.ec->increment_number_of_errors();
						return result;	
				}
				break;
		}
	}
	return result;
}

std::shared_ptr<ast::Loop_stmt> Parser::Impl::proc_loop_without_label()
{
    std::shared_ptr<ast::Loop_stmt> result;
    Main_lexem_info                li     = sc_->current_lexem();
    Lexem_category                 cat    = get_lexem_category(li);
    sc_->back();
    switch(cat){
        case Lexem_category::Kw_while:
            result = proc_while_stmt();
            break;
        case Lexem_category::Kw_repeat:
            result = proc_as_lomg_as_stmt();
            break;
		case Lexem_category::Kw_spider:
            result = proc_spider_stmt();
            break;
        case Lexem_category::Kw_for:
            result = proc_for_stmt();
            break;	
        default:
            printf(expected_loop_without_label_fmt, sc_->lexem_begin_line_number());
            et_.ec->increment_number_of_errors();
    }
    return result;
}

std::shared_ptr<ast::While_stmt> Parser::Impl::proc_while_stmt()
{
	std::shared_ptr<ast::While_stmt> result;
	enum class State{
		Start, Second, Expression, Open_bracket, Block, Closed_bracket
	};
	State state = State::Start;
	std::shared_ptr<Expr> condition;
    ast::Block_body            body;
	for(;;){
		Main_lexem_info li = sc_->current_lexem();
		Lexem_category cat = get_lexem_category(li);
		switch(state){
			case State::Start:
				if(cat != Lexem_category::Kw_while){
                    printf(expected_kw_while_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::Second;
                break;
			case State::Second:
				switch(cat){
					case Lexem_category::Logical_not:
					case Lexem_category::Bitwise_not:
					case Lexem_category::Plus:
					case Lexem_category::Minus:
					case Lexem_category::Sizeof:
					case Lexem_category::Kw_int:
					case Lexem_category::Kw_float:
					case Lexem_category::Kw_string:
					case Lexem_category::Kw_true:
					case Lexem_category::Kw_false:
					case Lexem_category::Open_round_bracket:
					case Lexem_category::Ident:
					case Lexem_category::Module_name_prefix:
					case Lexem_category::Kw_array:
                        sc_->back();
                        condition = proc_expr();
                        state = State::Expression;
                        break;
					default:
						printf(expected_expression_fmt, 
								sc_->lexem_begin_line_number());
						et_.ec->increment_number_of_errors();
						return result;	
				}
				break;
			case State::Expression:
				if(cat != Lexem_category::Open_curly_bracket){
                    printf(expected_open_curly_bracket_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::Open_bracket;
                break;
			case State::Open_bracket:
				switch(cat){
					case Lexem_category::Kw_var:
                    case Lexem_category::Kw_type:
                    case Lexem_category::Kw_function:
                    case Lexem_category::Kw_const:
					case Lexem_category::Semicolon:
					case Lexem_category::Label_prefix:
                    case Lexem_category::Kw_for:
                    case Lexem_category::Kw_while:
                    case Lexem_category::Kw_repeat:
					case Lexem_category::Module_name_prefix:
                    case Lexem_category::Ident:
                    case Lexem_category::Kw_exit:
                    case Lexem_category::Kw_read:
					case Lexem_category::Kw_print:
                    case Lexem_category::Kw_if:
                    case Lexem_category::Kw_match:
						sc_->back();
						body = proc_block_body();
						state = State::Block;
						break;
					default:
                        printf(expected_block_body_fmt,
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
				}
				break;
			case State::Block:
				if(cat != Lexem_category::Closed_curly_bracket){
                    printf(expected_closed_curly_bracket_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::Closed_bracket;
                break;
			case State::Closed_bracket:
				sc_->back();
                return result;
		}
	}
	return result;
}

std::shared_ptr<ast::While_stmt> Parser::Impl::proc_while_stmt()
{
	std::shared_ptr<ast::While_stmt> result;
	enum class State{
		Start, Second, Expression, Open_bracket, Block, Closed_bracket
	};
	State state = State::Start;
	std::shared_ptr<Expr> condition;
    ast::Block_body            body;
	for(;;){
		Main_lexem_info li = sc_->current_lexem();
		Lexem_category cat = get_lexem_category(li);
		switch(state){
			case State::Start:
				if(cat != Lexem_category::Kw_while){
                    printf(expected_kw_while_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::Second;
                break;
			case State::Second:
				switch(cat){
					case Lexem_category::Logical_not:
					case Lexem_category::Bitwise_not:
					case Lexem_category::Plus:
					case Lexem_category::Minus:
					case Lexem_category::Sizeof:
					case Lexem_category::Kw_int:
					case Lexem_category::Kw_float:
					case Lexem_category::Kw_string:
					case Lexem_category::Kw_true:
					case Lexem_category::Kw_false:
					case Lexem_category::Open_round_bracket:
					case Lexem_category::Ident:
					case Lexem_category::Module_name_prefix:
					case Lexem_category::Kw_array:
                        sc_->back();
                        condition = proc_expr();
                        state = State::Expression;
                        break;
					default:
						printf(expected_expression_fmt, 
								sc_->lexem_begin_line_number());
						et_.ec->increment_number_of_errors();
						return result;	
				}
				break;
			case State::Expression:
				if(cat != Lexem_category::Open_curly_bracket){
                    printf(expected_open_curly_bracket_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::Open_bracket;
                break;
			case State::Open_bracket:
				switch(cat){
					case Lexem_category::Kw_var:
                    case Lexem_category::Kw_type:
                    case Lexem_category::Kw_function:
                    case Lexem_category::Kw_const:
					case Lexem_category::Semicolon:
					case Lexem_category::Label_prefix:
                    case Lexem_category::Kw_for:
                    case Lexem_category::Kw_while:
                    case Lexem_category::Kw_repeat:
					case Lexem_category::Module_name_prefix:
                    case Lexem_category::Ident:
                    case Lexem_category::Kw_exit:
                    case Lexem_category::Kw_read:
					case Lexem_category::Kw_print:
                    case Lexem_category::Kw_if:
                    case Lexem_category::Kw_match:
						sc_->back();
						body = proc_block_body();
						state = State::Block;
						break;
					default:
                        printf(expected_block_body_fmt,
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
				}
				break;
			case State::Block:
				if(cat != Lexem_category::Closed_curly_bracket){
                    printf(expected_closed_curly_bracket_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::Closed_bracket;
                break;
			case State::Closed_bracket:
				sc_->back();
                return result;
		}
	}
	return result;
}

std::shared_ptr<ast::Assign_stmt> Parser::Impl::assign() // 34
{
	std::shared_ptr<ast::Assign_stmt> result;
	enum class State{
		Start, Second, Third, Fourth
	};
	State state = State::Start;
	std::shared_ptr<ast::Simplest_type_def> current_type;
	
	for(;;){
		Main_lexem_info li = sc_->current_lexem();
		Lexem_category cat = get_lexem_category(li);
		switch(state){
			case State::Start:
				if(cat != Lexem_category::Name){
                    printf(expected_Name_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Second; 
                break;
			case State::Second:
				if(cat != Lexem_category::Assignment_kind){
                    printf(expected_Assignment_kind_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Third; 
                break;
			case State::Third:
				if(cat != Lexem_category::Exp){
                    printf(expected_Exp_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Fourth; 
                break;
			case State::Fourth:
				sc_->back();
                return result; 
		}
	}
	return result;
}

std::shared_ptr<ast::Switch> Parser::Impl::swt() // 35
{
	std::shared_ptr<ast::Switch> result;
	enum class State{
		Start, Second, Third, Fourth, Five, Six
	};
	State state = State::Start;
	std::shared_ptr<ast::Simplest_type_def> current_type;

	scope::Id info			id info;
	
	for(;;){
		Main_lexem_info li = sc_->current_lexem();
		Lexem_category cat = get_lexem_category(li);
		switch(state){
			case State::Start:
				switch(cat){
					case Lexem_category::Exit:
						state = State::Second;
						break;
					case Lexem_category::Continue:
						state = State::Third;
						break;
					case Lexem_category::Return:
						state = State::Fourth;
						break;
					default:
                        printf(expected_Exit_Continue_Return_fmt,
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
				}
				break;
			case State::Second:
				if(cat != Lexem_category::From){
                    sc_->back();                    
                    return result;
                }else{
                    state = State::Five;
                }
                break;
			case State::Third:
				if(cat != Lexem_category::Id){
                    sc_->back();                    
                    return result;
                }else{
                    state = State::Six;
                }
                break;
			case State::Fourth:
				if(cat != Lexem_category::Exp){
                    sc_->back();                    
                    return result;
                }else{
                    state = State::Six;
                }
                break;
			case State::Five:
				if(cat != Lexem_category::Id){
                    printf(expected_Id_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Six; 
                break;
			case State::Six:
				sc_->back();
                return result; 
		}
	}
	return result;
}

std::shared_ptr<ast::Inp> Parser::Impl::input() // 36
{
	std::shared_ptr<ast::Inp> result;
	enum class State{
		Start, Second, Third, Fourth, Five
	};
	State state = State::Start;
	std::shared_ptr<ast::Simplest_type_def> current_type;
	
	for(;;){
		Main_lexem_info li = sc_->current_lexem();
		Lexem_category cat = get_lexem_category(li);
		switch(state){
			case State::Start:
				if(cat != Lexem_category::Input){
                    printf(expected_Input_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Second; 
                break;
			case State::Second:
				if(cat != Lexem_category::Obracket){
                    printf(expected_Obracket_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Third; 
                break;
			case State::Third:
				if(cat != Lexem_category::Name){
                    printf(expected_Name_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Fourth; 
                break;
			case State::Fourth:
				switch(cat){
					case Lexem_category::Comma:
						state = State::Third;
						break;
					case Lexem_category::Cbracket:
						state = State::Five;
						break;
					default:
                        printf(expected_Comma_Cbracket_fmt,
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
				}
				break;
			case State::Five:
				sc_->back();
                return result; 
		}
	}
	return result;
}

std::shared_ptr<ast::Outp> Parser::Impl::output() // 37
{
	std::shared_ptr<ast::Outp> result;
	enum class State{
		Start, Second, Third, Fourth, Five
	};
	State state = State::Start;
	std::shared_ptr<ast::Simplest_type_def> current_type;
	
	for(;;){
		Main_lexem_info li = sc_->current_lexem();
		Lexem_category cat = get_lexem_category(li);
		switch(state){
			case State::Start:
				if(cat != Lexem_category::Output){
                    printf(expected_Output_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Second; 
                break;
			case State::Second:
				if(cat != Lexem_category::Obracket){
                    printf(expected_Obracket_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Third; 
                break;
			case State::Third:
				if(cat != Lexem_category::Exp){
                    printf(expected_Exp_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Fourth; 
                break;
			case State::Fourth:
				switch(cat){
					case Lexem_category::Comma:
						state = State::Third;
						break;
					case Lexem_category::Cbracket:
						state = State::Five;
						break;
					default:
                        printf(expected_Comma_Cbracket_fmt,
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
				}
				break;
			case State::Five:
				sc_->back();
                return result; 
		}
	}
	return result;
}

std::shared_ptr<ast::Expr> Parser::Impl::pars_expr() // 38
{
	std::shared_ptr<ast::Expr> result;
	enum class State{
		Start, Second, Third, Fourth, Five,
               Six,    Sev,   Eight,  Nine,
			   Ten,	   El,	  Tw, 	  Thir, Fourt
	};
	State state = State::Start;
	std::shared_ptr<ast::Simplest_type_def> current_type;
	
	for(;;){
		Main_lexem_info li = sc_->current_lexem();
		Lexem_category cat = get_lexem_category(li);
		switch(state){
			case State::Start:
				if(cat != Lexem_category::Pars){
                    printf(expected_Pars_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Second; 
                break;
			case State::Second:
				if(cat != Lexem_category::Exp){
                    printf(expected_Exp_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Third; 
                break;
			case State::Third:
				if(cat != Lexem_category::Obrace){
                    printf(expected_Obrace_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Fourth; 
                break;
			case State::Fourth:
				if(cat != Lexem_category::Sample){
                    printf(expected_Sample_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Five; 
                break;
			case State::Five:
				if(cat != Lexem_category::Rarrow){
                    printf(expected_Rarrow_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Six; 
                break;
			case State::Six:
				if(cat != Lexem_category::Sample){
                    printf(expected_Sample_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Sev; 
                break;
			case State::Sev:
				if(cat != Lexem_category::BoB){
                    printf(expected_BoB_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Eight; 
                break;
			case State::Eight:
				if(cat != Lexem_category::Cbrace){
                    printf(expected_Cbrace_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Nine; 
                break;
			case State::Nine:
				switch(cat){
					case Lexem_category::Cbrace:
						state = State::Fourt;
						break;
					case Lexem_category::Sample:
						state = State::Five;
						break;
					case Lexem_category::Else:
						state = State::Ten;
						break;
					default:
                        printf(expected_Cbrace_Sample_Else_fmt,
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
				}
				break;
			case State::Ten:
				if(cat != Lexem_category::Obrace){
                    printf(expected_Obrace_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::El; 
                break;
			case State::El:
				if(cat != Lexem_category::BoB){
                    printf(expected_BoB_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Tw; 
                break;
			case State::Tw:
				if(cat != Lexem_category::Cbrace){
                    printf(expected_Cbrace_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Thir; 
                break;
			case State::Thir:
				if(cat != Lexem_category::Cbrace){
                    printf(expected_Cbrace_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Fourt; 
                break;
			case State::Fourt:
                sc_->back();
                return result; 
		}
	}
	return result;
}

bool Parser::Impl::add_type_to_symbol_table(ast::Id                               name, 
                                            const std::shared_ptr<ast::Type_def>& definition)
{
    bool           success = true;
    scope::Id_info id_info;
    id_info.attr           = scope::Id_attribute::Type_name;
    id_info.definition     = definition;
    auto sr                = st_->search(name);
    return success;
    if((sr.status ==  symbol_table::Search_status::Not_found) || 
       (sr.status !=  symbol_table::Search_status::Found_in_the_current_scope))
    {
        st_->insert(name_idx, id_info);
    }else{
        auto id_str = idx_to_string(et_.ids_trie, name);
        printf(already_defined_name_fmt,
               sc_->lexem_begin_line_number(),
               id_str.c_str());
        et_.ec->increment_number_of_errors();
        success     = false;
    }
    return success;
}

static const std::map<Main_lexem_code, ast::Embedded_type_kind> non_modified2embedded_kind_map = {
    {Main_lexem_code::Kw_byte,     ast::Embedded_type_kind::Uint8   }, 
    {Main_lexem_code::Kw_uint8,    ast::Embedded_type_kind::Uint8   }, 
    {Main_lexem_code::Kw_uint16,   ast::Embedded_type_kind::Uint16  }, 
    {Main_lexem_code::Kw_uint32,   ast::Embedded_type_kind::Uint32  }, 
    {Main_lexem_code::Kw_uint64,   ast::Embedded_type_kind::Uint64  }, 
    {Main_lexem_code::Kw_uint128,  ast::Embedded_type_kind::Uint128 }, 
    {Main_lexem_code::Kw_int8,     ast::Embedded_type_kind::Int8    }, 
    {Main_lexem_code::Kw_int16,    ast::Embedded_type_kind::Int16   }, 
    {Main_lexem_code::Kw_int32,    ast::Embedded_type_kind::Int32   }, 
    {Main_lexem_code::Kw_int64,    ast::Embedded_type_kind::Int64   }, 
    {Main_lexem_code::Kw_int128,   ast::Embedded_type_kind::Int128  },  
    {Main_lexem_code::Kw_bool8,    ast::Embedded_type_kind::Bool8   }, 
    {Main_lexem_code::Kw_bool16,   ast::Embedded_type_kind::Bool16  }, 
    {Main_lexem_code::Kw_bool32,   ast::Embedded_type_kind::Bool32  }, 
    {Main_lexem_code::Kw_bool64,   ast::Embedded_type_kind::Bool64  },    
    {Main_lexem_code::Kw_char8,    ast::Embedded_type_kind::Char8   }, 
    {Main_lexem_code::Kw_char16,   ast::Embedded_type_kind::Char16  }, 
    {Main_lexem_code::Kw_char32,   ast::Embedded_type_kind::Char32  },    
    {Main_lexem_code::Kw_string8,  ast::Embedded_type_kind::String8 }, 
    {Main_lexem_code::Kw_string16, ast::Embedded_type_kind::String16},    
    {Main_lexem_code::Kw_string32, ast::Embedded_type_kind::String32}, 
    {Main_lexem_code::Kw_Float32,  ast::Embedded_type_kind::Float32 }, 
    {Main_lexem_code::Kw_float64,  ast::Embedded_type_kind::Float64 }, 
    {Main_lexem_code::Kw_float80,  ast::Embedded_type_kind::Float80 }, 
    {Main_lexem_code::Kw_float128, ast::Embedded_type_kind::Float128}, 
    {Main_lexem_code::Kw_void,     ast::Embedded_type_kind::Void    }    
};
    
static ast::Embedded_type_kind non_modified2embedded_kind(Main_lexem_code code)
{
    ast::Embedded_type_kind kind;
    auto it = non_modified2embedded_kind_map.find(code);
    if(it != non_modified2embedded_kind_map.end()){
        kind = it->second;
    }else{
        kind = ast::Embedded_type_kind::Void;
    }
    return kind;
}

static unsigned shift(unsigned lhs, int rhs)
{
    if(!rhs)return lhs;
    return (rhs > 0) ? (lhs << rhs) : (lhs >> (-rhs));
}

static constexpr unsigned sizeof_int            = 8;
static constexpr unsigned sizeof_float          = 8;
static constexpr unsigned sizeof_bool           = 1;
static constexpr unsigned sizeof_char           = 4;
static constexpr unsigned sizeof_uint           = 8;
static constexpr int      max_shift_for_changer = 7;
static constexpr int      min_shift_for_changer = -7;
static constexpr unsigned maxint_t_size         = 16;
static constexpr unsigned minint_t_size         = 1;
static constexpr unsigned maxfloat_t_size       = 16;
static constexpr unsigned minfloat_t_size       = 4;
static constexpr unsigned maxbool_t_size        = 8;
static constexpr unsigned minbool_t_size        = 1;
static constexpr unsigned maxchar_t_size        = 4;
static constexpr unsigned minchar_t_size        = 1;
static constexpr unsigned maxuint_t_size        = 16;
static constexpr unsigned minuint_t_size        = 1;
static constexpr int      min_modpow_for_float  = -1;
static constexpr int      max_modpow_for_float  = 2;

static constexpr unsigned sizeof_float32        = 4;
static constexpr unsigned sizeof_float64        = 8;
static constexpr unsigned sizeof_float80        = 10;
static constexpr unsigned sizeof_float128       = 16;

static inline int correct_val(int val, int lower_bound, int upper_bound)
{
    int result = val;
    result     = (result > upper_bound) ? upper_bound : result;
    result     = (result < lower_bound) ? lower_bound : result;
    return result;
}
 
static unsigned calc_sizeof_for_modfloat(int mod_power)
{
    unsigned result  = 8;
    unsigned changer = correct_val(mod_power, 
                                   min_modpow_for_float, 
                                   max_modpow_for_float);
    switch(changer){
        case -1:
            result = sizeof_float32;
            break;
        case 0:
            result = sizeof_float64;
            break;
        case 1:
            result = sizeof_float80;
            break;
        case 2:
            result = sizeof_float128;
            break;
    }
    return result;
}
 
static unsigned calculate_sizeof_for_modified(Main_lexem_code code, int mod_power)
{
    unsigned result  = 1;
    int      shift_v = correct_val(mod_power, 
                                   min_shift_for_changer, 
                                   max_shift_for_changer); 
    switch(code){
        case Main_lexem_code::Kw_int:
            result = shift(sizeof_int, shift_v);
            result = correct_val(result, minint_t_size, maxint_t_size);
            break;
        case Main_lexem_code::Kw_unsigned:
            result = shift(sizeof_uint, shift_v);
            result = correct_val(result, minuint_t_size, maxuint_t_size);
            break;
        case Main_lexem_code::Kw_float:
            result = calc_sizeof_for_modfloat(shift_v);
            break;
        case Main_lexem_code::Kw_bool:
            result = shift(sizeof_bool, shift_v);
            result = correct_val(result, minbool_t_size, maxbool_t_size);
            break;
        case Main_lexem_code::Kw_char:
            result = shift(sizeof_char, shift_v);
            result = correct_val(result, minchar_t_size, maxchar_t_size);
            break;
        case Main_lexem_code::Kw_string:
            result = shift(sizeof_int, shift_v);
            result = correct_val(result, minchar_t_size, maxchar_t_size);
            break;
        default:
            ;
    }
    return result;
}   

static const std::map<std::pair<Main_lexem_code, unsigned>, ast::Embedded_type_kind> mod2emb = {
    {{Main_lexem_code::Kw_unsigned, 1 }, ast::Embedded_type_kind::Uint8      },
    {{Main_lexem_code::Kw_unsigned, 2 }, ast::Embedded_type_kind::Uint16     },
    {{Main_lexem_code::Kw_unsigned, 4 }, ast::Embedded_type_kind::Uint32     },  
    {{Main_lexem_code::Kw_unsigned, 8 }, ast::Embedded_type_kind::Uint64     },
    {{Main_lexem_code::Kw_unsigned, 16}, ast::Embedded_type_kind::Uint128    }, 
    {{Main_lexem_code::Kw_int,      1 }, ast::Embedded_type_kind::Int8       }, 
    {{Main_lexem_code::Kw_int,      2 }, ast::Embedded_type_kind::Int16      }, 
    {{Main_lexem_code::Kw_int,      4 }, ast::Embedded_type_kind::Int32      }, 
    {{Main_lexem_code::Kw_int,      8 }, ast::Embedded_type_kind::Int64      }, 
    {{Main_lexem_code::Kw_int,      16}, ast::Embedded_type_kind::Int128     }, 
    {{Main_lexem_code::Kw_bool,     1 }, ast::Embedded_type_kind::Bool8      },  
    {{Main_lexem_code::Kw_bool,     2 }, ast::Embedded_type_kind::Bool16     },  
    {{Main_lexem_code::Kw_bool,     4 }, ast::Embedded_type_kind::Bool32     },  
    {{Main_lexem_code::Kw_bool,     8 }, ast::Embedded_type_kind::Bool64     },  
    {{Main_lexem_code::Kw_char,     1 }, ast::Embedded_type_kind::Char8      }, 
    {{Main_lexem_code::Kw_char,     2 }, ast::Embedded_type_kind::Char16     }, 
    {{Main_lexem_code::Kw_char,     4 }, ast::Embedded_type_kind::Char32     }, 
    {{Main_lexem_code::Kw_string,   1 }, ast::Embedded_type_kind::String8    },
    {{Main_lexem_code::Kw_string,   2 }, ast::Embedded_type_kind::String16   },
    {{Main_lexem_code::Kw_string,   4 }, ast::Embedded_type_kind::String32   },
    {{Main_lexem_code::Kw_int,      4 }, ast::Embedded_type_kind::Float32    },
    {{Main_lexem_code::Kw_int,      8 }, ast::Embedded_type_kind::Float64    },
    {{Main_lexem_code::Kw_int,      10}, ast::Embedded_type_kind::Float80    },
    {{Main_lexem_code::Kw_int,      16}, ast::Embedded_type_kind::Float128   }
};
   
static ast::Embedded_type_kind modified2embedded_kind(Main_lexem_code code, int mod_power)
{
    ast::Embedded_type_kind kind;
    unsigned                size = calculate_sizeof_for_modified(code, mod_power);
    auto                    it   = mod2emb.find({code, size});
    if(it != mod2emb.end()){
        kind = it->second;
    }else{
        kind = ast::Embedded_type_kind::Char8;
    }
    return kind;
}

std::shared_ptr<ast::Expr> Parser::Impl::proc_expr()
{
    std::shared_ptr<ast::Expr> result;
    return result;
}
