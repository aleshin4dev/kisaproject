/*
    File:    ast.h
    Created: 27 March 2018 at 19:21 Moscow time
    Author:  Гаврилов Владимир Сергеевич
    E-mails: vladimir.s.gavrilov@gmail.com
             gavrilov.vladimir.s@mail.ru
             gavvs1977@yandex.ru
*/

#ifndef AST_H
#define AST_H
#include <memory>
#include <map>
#include <cstddef>
#include <list>
#include <utility>
namespace ast{
    using Id = size_t;
    
    struct Ast_node{
        Ast_node()                = default;
        Ast_node(const Ast_node&) = default;
        virtual ~Ast_node()       = default;
    };

    struct Definition : public Ast_node{
        Definition()                  = default;
        Definition(const Definition&) = default;
        virtual ~Definition()         = default;
    };

    using Module_body = std::list<std::shared_ptr<Definition>>;

    struct Program : public Ast_node{
        Program() : module_name_(0),
                    imports_(),
                    exports_(),
                    body_() {};

        Program(Id                     module_name,
                const std::list<Id>&   imports,
                const std::list<Id>&   exports,
                const Module_body&     body) :
            module_name_(module_name),
            imports_(imports),
            exports_(exports),
            body_(body) {};

        Program(const Program&) = default;
        ~Program()              = default;

        Id            module_name_;
        std::list<Id> imports_;
        std::list<Id> exports_;
        Module_body   body_;
    };

    struct Type_def : public Definition{
        Type_def()                  = default;
        Type_def(const Type_def&)   = default;
        virtual ~Type_def()         = default;
    };

    struct Simplest_type_def : public Type_def{
        Simplest_type_def()                         = default;
        Simplest_type_def(const Simplest_type_def&) = default;
        virtual ~Simplest_type_def()                = default;
    };

    struct Expr : public Ast_node{
        Expr()            = default;
        Expr(const Expr&) = default;
        virtual ~Expr()   = default;
    };

    struct Elementary_type : public Simplest_type_def{
        Elementary_type()                       = default;

        Elementary_type(size_t ref_order) : ref_order_(ref_order) {};

        Elementary_type(const Elementary_type&) = default;
        virtual ~Elementary_type()              = default;

        size_t ref_order_ = 0;
    };

    struct Array_type_def : public Simplest_type_def{
        Array_type_def()                      = default;
        Array_type_def(const Array_type_def&) = default;
        virtual ~Array_type_def()             = default;

        Array_type_def(const std::list<std::shared_ptr<Expr>>& dim_size,
                       const std::shared_ptr<Elementary_type>& elem_type) :
            dim_size_(dim_size), elem_type_(elem_type) {};

        std::list<std::shared_ptr<Expr>> dim_size_;
        std::shared_ptr<Elementary_type> elem_type_;
    };

    enum class Embedded_type_kind{
        Void,    Uint8,   Uint16,   Uint32,   Uint64,  Uint128,
        Int8,    Int16,   Int32,    Int64,    Int128,  Bool8,
        Bool16,  Bool32,  Bool64,   Bool128,  Char8,   Char16,
        Char32,  String8, String16, String32, Float32, Float64,
        Float80, Float128
    };

    struct Embedded_type : public Elementary_type{
        Embedded_type()                     = default;
        Embedded_type(const Embedded_type_kind kind, size_t ref_order) :
            Elementary_type(ref_order), kind_(kind) {};

        Embedded_type(const Embedded_type&) = default;
        virtual ~Embedded_type()            = default;

        Embedded_type_kind kind_ = Embedded_type_kind::Void;
    };

    struct Type_alias : public Elementary_type{
        Type_alias()                  = default;
        Type_alias(Id module_name, Id type_name, size_t ref_order) :
            Elementary_type(ref_order),
            module_name_(module_name),
            type_name_(type_name) {};

        Type_alias(const Type_alias&) = default;
        virtual ~Type_alias()         = default;

        Id module_name_ = 0;
        Id type_name_   = 0;
    };

    struct Tuple : public Type_def{
        Tuple()             = default;
        Tuple(const std::list<std::shared_ptr<Elementary_type>>& elems) :
            elems_(elems) {};
        Tuple(const Tuple&) = default;
        virtual ~Tuple()    = default;

        std::list<std::shared_ptr<Elementary_type>> elems_;
    };

    enum class Formal_arg_kind{
        Val, Ref, Const_ref
    };

    struct Formal_arg_info : public Ast_node{
        Id                                 name_     = 0;
        Formal_arg_kind                    kind_     = Formal_arg_kind::Val;
        std::shared_ptr<Simplest_type_def> arg_type_;
    };

    struct Func_ptr : public Type_def{
        Func_ptr()                = default;
        Func_ptr(const Func_ptr&) = default;
        virtual ~Func_ptr()       = default;

        std::list<Formal_arg_info> params_;
        std::shared_ptr<Type_def>  value_type_;
    };

    struct Algebraic_type_component : public Ast_node{
        Algebraic_type_component()                                = default;
        Algebraic_type_component(const Algebraic_type_component&) = default;
        virtual ~Algebraic_type_component()                       = default;
    };

    struct Enum_def : public Algebraic_type_component{
        Enum_def()                = default;
        Enum_def(const Enum_def&) = default;
        virtual ~Enum_def()       = default;

        Id                   name_;
        std::map<Id, size_t> elems_;
    };

    struct Struct_field_info : public Ast_node{
        Id                                 name_ = 0;
        std::shared_ptr<Simplest_type_def> type_;
    };

    struct Struct_def : public Algebraic_type_component{
        Struct_def()                  = default;
        Struct_def(const Struct_def&) = default;
        virtual ~Struct_def()         = default;

        std::list<Struct_field_info> fields_;
    };

    struct Algebraic_type_def : public Type_def{
        Algebraic_type_def()                          = default;
        Algebraic_type_def(const Algebraic_type_def&) = default;
        virtual ~Algebraic_type_def()                 = default;

        std::list<std::shared_ptr<Algebraic_type_component>> components_;
    };

    struct Type : public Definition{
        Type()            = default;
        Type(const Type&) = default;
        virtual ~Type()   = default;

        Type(Id name, const std::shared_ptr<Type_def>& def) : name_(name), def_(def) {};

        Id                        name_ = 0;
        std::shared_ptr<Type_def> def_;
    };

    struct Var : public Definition{
        Var()           = default;
        Var(const Var&) = default;
        virtual ~Var()  = default;

        Var(Id name, const std::shared_ptr<Simplest_type_def>& type) : 
            name_(name), type_(type) {};

        Id                                 name_ = 0;
        std::shared_ptr<Simplest_type_def> type_;
    };

    struct Prototype : public Definition{
        Prototype()                 = default;
        Prototype(const Prototype&) = default;
        virtual ~Prototype()        = default;

        std::list<Formal_arg_info> params_;
        std::shared_ptr<Type_def>  value_type_;
        Id                         name_        = 0;
    };

    struct Block_elem : public Ast_node{
        Block_elem()                  = default;
        Block_elem(const Block_elem&) = default;
        virtual ~Block_elem()         = default;
    };

    struct Decl_in_block : public Block_elem{
        Decl_in_block()                     = default;
        Decl_in_block(const Decl_in_block&) = default;
        virtual ~Decl_in_block()            = default;

        std::shared_ptr<Definition> decl_;
    };

    struct Statement : public Block_elem{
        Statement()                 = default;
        Statement(const Statement&) = default;
        virtual ~Statement()        = default;
    };

    using Block_body = std::list<std::shared_ptr<Block_elem>>;

    struct Function : public Definition{
        Function()                = default;
        Function(const Function&) = default;
        virtual ~Function()       = default;

        std::list<Formal_arg_info> params_;
        std::shared_ptr<Type_def>  value_type_;
        Id                         name_        = 0;
        Block_body                 body_;
    };

    struct Constant : public Definition{
        Constant()                = default;
        Constant(const Constant&) = default;
        virtual ~Constant()       = default;

        Id                                 name_;
        std::shared_ptr<Simplest_type_def> type_;
        std::shared_ptr<Expr>              val_;
    };

    struct Conditional_expr : public Expr{
        Conditional_expr()                        = default;
        Conditional_expr(const Conditional_expr&) = default;
        virtual ~Conditional_expr()               = default;

        std::shared_ptr<Expr> condition_;
        std::shared_ptr<Expr> true_branch_;
        std::shared_ptr<Expr> false_branch_;
    };

    enum class Binary_op_kind{
        Lor,    Lor_not, Lxor,    Lxor_not, Land,   Land_not,
        Less,   Greater, Leq,     Geq,      Equals, Not_equals,
        Bor,    Bor_not, Bxor,    Bxor_not, Band,   Band_not,
        Lshift, Rshift,  SRshift, Add,      Sub,    Mul,
        Div,    Mod,     Fdiv,    Pow,      Fpow,   Dim_size
    };

    struct Binary_op : public Expr{
        Binary_op()                 = default;
        Binary_op(const Binary_op&) = default;
        virtual ~Binary_op()        = default;

        Binary_op_kind        kind_ = Binary_op_kind::Lor;
        std::shared_ptr<Expr> lhs_;
        std::shared_ptr<Expr> rhs_;
    };

    struct Unary_op_kind{
        Lnot, Bnot, Plus, Minus, Sizeof
    };

    struct Unary_op : public Expr{
        Unary_op()                = default;
        Unary_op(const Unary_op&) = default;
        virtual ~Unary_op()       = default;

        Unary_op_kind         kind_ = Unary_op_kind::Lnot;
        std::shared_ptr<Expr> arg_;
    };

    struct Literal : public Expr{
        Literal()               = default;
        Literal(const Literal&) = default;
        virtual ~Literal()      = default;
    };

    struct Integer_literal : public Literal{
        Integer_literal()                       = default;
        Integer_literal(const Integer_literal&) = default;
        virtual ~Integer_literal()              = default;

        __int128 val_ = 0;
    };

    struct Float_literal : public Literal{
        Float_literal()                     = default;
        Float_literal(const Float_literal&) = default;
        virtual ~Float_literal()            = default;

        __float128 val_ = 0;
    };

    enum class Str_literal_kind{
        U8Str,U16Str, U32Str
    };

    struct String_literal : public Literal{
        String_literal()                      = default;
        String_literal(const String_literal&) = default;
        virtual ~String_literal()             = default;

        Str_literal_kind kind_ = Str_literal_kind::U32Str;
        size_t           val_  = 0;
    };

    enum class Char_literal_kind{
        U8Char, U16Char, U32Char
    };

    struct Char_literal : public Literal{
        Char_literal()                    = default;
        Char_literal(const Char_literal&) = default;
        virtual ~Char_literal()           = default;

        Char_literal_kind kind_ = Char_literal_kind::U8Char;
        char32_t          val_  = 0;
    };

    struct Bool_literal : public Literal{
        Bool_literal()                    = default;
        Bool_literal(const Bool_literal&) = default;
        virtual ~Bool_literal()           = default;

        bool val_  = false;
    };

    struct Name_component : public Ast_node{
        Name_component()                      = default;
        Name_component(const Name_component&) = default;
        virtual ~Name_component()             = default;
    };

    struct Actual_args : public Name_component{
        Actual_args()                   = default;
        Actual_args(const Actual_args&) = default;
        virtual ~Actual_args()          = default;

        std::list<std::shared_ptr<Expr>> args_;
    };

    struct Array_elem_indeces : public Name_component{
        Array_elem_indeces()                          = default;
        Array_elem_indeces(const Array_elem_indeces&) = default;
        virtual ~Array_elem_indeces()                 = default;

        std::list<std::shared_ptr<Expr>> indeces_;
    };

    struct Dereference : public Name_component{
        Dereference()                   = default;
        Dereference(const Dereference&) = default;
        virtual ~Dereference()          = default;

        size_t fictive_;
    };

    struct Field_selector : public Name_component{
        Field_selector()                      = default;
        Field_selector(const Field_selector&) = default;
        virtual ~Field_selector()             = default;

        size_t field_name_ = 0;
    };

    struct Name : public Expr{
        Name()            = default;
        Name(const Name&) = default;
        virtual ~Name()   = default;

        Id module_name_    = 0;
        Id type_name_      = 0;
        Id type_part_name_ = 0;
        Id id_             = 0;
        std::list<std::shared_ptr<Name_component>> components_;
    };

    struct Struct_val : public Expr{
        Struct_val()                  = default;
        Struct_val(const Struct_val&) = default;
        virtual ~Struct_val()         = default;

        std::list<std::pair<Id, std::shared_ptr<Expr>>> components_;
    };

    struct Array_val : public Expr{
        Array_val()                 = default;
        Array_val(const Array_val&) = default;
        virtual ~Array_val()        = default;

        std::shared_ptr<Elementary_type> value_type_;
        std::list<std::shared_ptr<Expr>> elems_;
    };

    struct Alloc_scalar_expr : public Expr{
        Alloc_scalar_expr()                         = default;
        Alloc_scalar_expr(const Alloc_scalar_expr&) = default;
        virtual ~Alloc_scalar_expr()                = default;

        std::shared_ptr<Name> allocated_var_;
    };

    struct Alloc_array_expr : public Expr{
        Alloc_array_expr()                        = default;
        Alloc_array_expr(const Alloc_array_expr&) = default;
        virtual ~Alloc_array_expr()               = default;

        std::shared_ptr<Name> allocated_var_;

        std::shared_ptr<Elementary_type> value_type_;
        std::list<std::shared_ptr<Expr>> indeces_;
    };

    struct Delete_stmt : public Statement{
        Delete_stmt()                   = default;
        Delete_stmt(const Delete_stmt&) = default;
        virtual ~Delete_stmt()          = default;

        std::shared_ptr<Name> deallocated_var_;
    };

    struct Delete_array_stmt : public Statement{
        Delete_array_stmt()                         = default;
        Delete_array_stmt(const Delete_array_stmt&) = default;
        virtual ~Delete_array_stmt()                = default;

        std::shared_ptr<Name> deallocated_var_;
    };

    struct Cond_branch : public Ast_node{
        Cond_branch()                   = default;
        Cond_branch(const Cond_branch&) = default;
        virtual ~Cond_branch()          = default;

        std::shared_ptr<Expr> condition_;
        Block_body            body_;
    };

    struct If_stmt : public Statement{
        If_stmt()               = default;
        If_stmt(const If_stmt&) = default;
        virtual ~If_stmt()      = default;

        std::list<std::shared_ptr<Cond_branch>> cond_branches_;
        Block_body                              else_branch_;
    };

    struct Loop_stmt : public Statement{
        Loop_stmt()                 = default;
        Loop_stmt(const Loop_stmt&) = default;
        virtual ~Loop_stmt()        = default;

        size_t loop_label_ = 0;
    };

    struct For_stmt : public Loop_stmt{
        For_stmt()                = default;
        For_stmt(const For_stmt&) = default;
        virtual ~For_stmt()       = default;

        size_t                loop_var_   = 0;
        std::shared_ptr<Expr> begin_val_;
        std::shared_ptr<Expr> end_val_;
        std::shared_ptr<Expr> step_;
        Block_body            body_;
        Block_body            else_branch_;
    };

    struct Foreach_stmt : public Loop_stmt{
        Foreach_stmt()                    = default;
        Foreach_stmt(const Foreach_stmt&) = default;
        virtual ~Foreach_stmt()           = default;

        size_t                loop_var_   = 0;
        std::shared_ptr<Expr> collection_;
        Block_body            body_;
        Block_body            else_branch_;
    };

    struct While_stmt : public Loop_stmt{
        While_stmt()                  = default;
        While_stmt(const While_stmt&) = default;
        virtual ~While_stmt()         = default;

        std::shared_ptr<Expr> condition_;
        Block_body            body_;
    };

    struct As_lomg_as_stmt : public Loop_stmt{
        As_lomg_as_stmt()                       = default;
        As_lomg_as_stmt(const As_lomg_as_stmt&) = default;
        virtual ~As_lomg_as_stmt()              = default;

        std::shared_ptr<Expr> condition_;
        Block_body            body_;
    };

    struct Spider_case : public Ast_node{
        Spider_case()                   = default;
        Spider_case(const Spider_case&) = default;
        virtual ~Spider_case()          = default;

        std::shared_ptr<Expr> condition_;
        Block_body            body_;
    };

    struct Spider_stmt : public Loop_stmt{
        Spider_stmt()                   = default;
        Spider_stmt(const Spider_stmt&) = default;
        virtual ~Spider_stmt()          = default;

        std::list<Spider_case> branches_;
        Block_body             else_branch_;
    };

    enum class Assignment_kind{
        Assign,               Add_assign,      Sub_assign,
        Mul_assign,           Div_assign,      Fdiv_assign,
        Mod_assign,           Lor_assign,      Lor_not_assign,
        Lxor_assign,          Lxor_not_assign, Land_assign,
        Land_not_assign,      Bor_assign,      Bor_not_assign,
        Bxor_assign,          Bxor_not_assign, Band_assign,
        Band_not_assign,      Lshift_assign,   Rshift_assign,
        Signed_rshift_assign, Power_assign,    Fpower_assign
    };

    struct Assign_stmt : public Statement{
        Assign_stmt()                   = default;
        Assign_stmt(const Assign_stmt&) = default;
        virtual ~Assign_stmt()          = default;

        Assignment_kind       kind_ = Assignment_kind::Assign;
        std::shared_ptr<Name> lhs_;
        Block_body            rhs_;
    };

    struct Break_stmt : public Statement{
        Break_stmt()                  = default;
        Break_stmt(const Break_stmt&) = default;
        virtual ~Break_stmt()         = default;

        Id loop_label_ = 0;
    };

    struct Continue_stmt : public Statement{
        Continue_stmt()                     = default;
        Continue_stmt(const Continue_stmt&) = default;
        virtual ~Continue_stmt()            = default;

        Id loop_label_ = 0;
    };

    struct Return_stmt : public Statement{
        Return_stmt()                   = default;
        Return_stmt(const Return_stmt&) = default;
        virtual ~Return_stmt()          = default;

        std::shared_ptr<Expr> returned_val_;
    };

    struct Read_stmt : public Statement{
        Read_stmt()                 = default;
        Read_stmt(const Read_stmt&) = default;
        virtual ~Read_stmt()        = default;

        std::list<std::shared_ptr<Name>> vars_for_input_;
    };

    struct Print_stmt : public Statement{
        Print_stmt()                  = default;
        Print_stmt(const Print_stmt&) = default;
        virtual ~Print_stmt()         = default;

        std::list<std::shared_ptr<Expr>> vals_for_printing_;
    };

    struct Pattern : public Ast_node{
        Pattern()               = default;
        Pattern(const Pattern&) = default;
        ~Pattern()              = default;

        Id module_name_ = 0;
        Id type_name_   = 0;
        Id type_part_   = 0;
    };

    struct Match_case : public Ast_node{
        Match_case()                  = default;
        Match_case(const Match_case&) = default;
        ~Match_case()                 = default;

        Pattern    pattern_;
        Block_body body_;
    };

    struct Match_stmt : public Statement{
        Match_stmt()                  = default;
        Match_stmt(const Match_stmt&) = default;
        virtual ~Match_stmt()         = default;

        std::shared_ptr<Expr> matched_val_;
        std::list<Match_case> cases_;
        Block_body            else_branch_;
    };
    
    using Ast = std::list<std::shared_ptr<Ast_node>>;
};
#endif