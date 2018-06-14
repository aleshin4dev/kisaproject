#ifndef LEXEM_INFO_H
#define LEXEM_INFO_H
enum class Slecsem_code : unsigned short {
    None,    Unknown, Mod,    
    Use,     Get,     Var,    
    Type,    Arr,     Big,    
    Small,   Int,     Float,  
    Unsgn,   Bool,    Symb,   
    Str,     Nothng,  Byte,   
    Int8,    Int16,   Int32,  
    Int64,   Int128,  Unsgn8, 
    Unsgn16, Unsgn32, Unsgn64,
    Unsgn128,Float32, Float64,
    Float128,Bool8,   Bool16, 
    Bool32,  Bool64,  Symb8,  
    Symb16,  Symb32,  Str8,   
    Str16,   Str32,   Func,   
    Val,     Ref,     Enum,   
    Struct,  Const,   True,   
    False,   If,      Then,   
    Ines,    Else,    End,    
    Compl,   While,   Rep,    
    AsLongAs,Spider,  At,     
    Break,   Continue,Back,   
    Ins,     Out,     Pars,   
    ,        ,        ,       
    ,        ,        ,       
    ,        ,        ,       
    ,        ,        ,       
    ,        ,        ,       
    ,        ,        ,       
    ,        ,        ,       
    ,        ,        ,       
    ,        ,        ,       
    ,        ,        ,       
    ,        ,        ,       
    ,        ,        ,       
    ,        ,        ,       
    ,        ,        ,       
    ,        ,        ,       
    ,        ,        ,       
    ,        ,        ,       
    ,        ,        ,       
    ,        ,        ,       
    ,        ,        ,       
    ,        ,        ,       
    ,        ,        ,       
    ,        ,        ,       
    ,        ,        ,       
    ,        ,        ,       
    ,        ,        ,       
    ,        ,        ,       
    
};

struct Lexem_info{
    Slecsem_code codes;
    union{
        size_t    ident_index;
        size_t    string_index;
        char32_t  c;
        Small
    }
};
#endif