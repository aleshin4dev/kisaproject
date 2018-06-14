#include <cstdio>
#include <cstddef>
#include "../include/print_lexem.h"

static const char* lexem_names[] = {
    "None",     "Unknown",  "Mod",     
    "Use",      "Get",      "Var",     
    "Type",     "Arr",      "Big",     
    "Small",    "Int",      "Float",   
    "Unsgn",    "Bool",     "Symb",    
    "Str",      "Nothng",   "Byte",    
    "Int8",     "Int16",    "Int32",   
    "Int64",    "Int128",   "Unsgn8",  
    "Unsgn16",  "Unsgn32",  "Unsgn64", 
    "Unsgn128", "Float32",  "Float64", 
    "Float128", "Bool8",    "Bool16",  
    "Bool32",   "Bool64",   "Symb8",   
    "Symb16",   "Symb32",   "Str8",    
    "Str16",    "Str32",    "Func",    
    "Val",      "Ref",      "Enum",    
    "Struct",   "Const",    "True",    
    "False",    "If",       "Then",    
    "Ines",     "Else",     "End",     
    "Compl",    "While",    "Rep",     
    "AsLongAs", "Spider",   "At",      
    "Break",    "Continue", "Back",    
    "Ins",      "Out",      "Pars",    
    "",         "",         "",        
    "",         "",         "",        
    "",         "",         "",        
    "",         "",         "",        
    "",         "",         "",        
    "",         "",         "",        
    "",         "",         "",        
    "",         "",         "",        
    "",         "",         "",        
    "",         "",         "",        
    "",         "",         "",        
    "",         "",         "",        
    "",         "",         "",        
    "",         "",         "",        
    "",         "",         "",        
    "",         "",         "",        
    "",         "",         "",        
    "",         "",         "",        
    "",         "",         "",        
    "",         "",         "",        
    "",         "",         "",        
    "",         "",         "",        
    "",         "",         "",        
    "",         "",         "",        
    "",         "",         "",        
    "",         "",         "",        
    "",         "",         "",        
    ""
};

void print_lexem(const Lexem_info& li)
{
    printf("%s\n", lexem_names[static_cast<size_t>(li.code)]);
}

void test_scaner(std::shared_ptr<SLexScan>& sc)
{
    Lexem_info li;
    do{
        li = sc->current_lexem();
        print_lexem(li);
    }while(li.code != Slecsem_code::None);
}
