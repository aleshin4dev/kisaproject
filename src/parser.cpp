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
                break;
            case State::Func: 
                break;
            case State::Algebraic_part: 
                break;
            case State::Tuple_elem:  
                break;
            case State::End_tuple:         
                break;
            case State::Algebraic_sep:
                break;
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

std::shared_ptr<ast::Expr> Parser::Impl::proc_expr()
{
    std::shared_ptr<ast::Expr> result;
    return result;
}
