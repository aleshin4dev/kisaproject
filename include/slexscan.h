#ifndef SLEXSCAN_H
#define SLEXSCAN_H
#include <string>
#include "../include/abstract_scaner.h"
#include "../include/error_count.h"
#include "../include/location.h"
#include "../include/lexem_info.h"


class SLexScan : public Abstract_scaner<Lexem_info>{
public:
    SLexScan()           = default;
    SLexScan(const Location_ptr& location, const Errors_and_tries& et) :
        Abstract_scaner<Lexem_info>(location, et) {}
    SLexScan(const SLexScan&) = default;
    virtual ~SLexScan()  = default;
    virtual Lexem_info current_lexem();
private:
        Lexem_info current_lexem_();
    void omit_multilined_comment();
    void omit_singlelined_comment();
add_dec_digit


    enum Automaton_name{
        A_start,  A_unknown,   A_keyword, 
        A_ident,  A_delimiter, A_string,  
        A_number
    };

    Automaton_name automaton; /* current automaton */

    typedef bool (SLexScan::*Automaton_proc)();
    /* This is the type of pointer to the member function that implements the
     * automaton that processes the lexeme. This function must return true if
     * the lexeme is not yet parsed, and false otherwise. */

    typedef void (SLexScan::*Final_proc)();
    /* And this is the type of the pointer to the member function that performs
     * the necessary actions in the event of an unexpected end of the lexeme. */

    static Automaton_proc procs[];
    static Final_proc     finals[];

    /* Lexeme processing functions: */
    bool start_proc();     bool unknown_proc(); 
    bool keyword_proc();   bool ident_proc();   
    bool delimiter_proc(); bool string_proc();  
    bool number_proc()

    /* Functions for performing actions in case of an
     * unexpected end of the token: */
    void none_final_proc();      void unknown_final_proc(); 
    void keyword_final_proc();   void ident_final_proc();   
    void delimiter_final_proc(); void string_final_proc();  
    void number_final_proc()
};
#endif