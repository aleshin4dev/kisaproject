#ifndef ELEM_H
#define ELEM_H
struct Elem{
    /** A pointer to a string of characters that can be crossed. */
    char32_t*       symbols;
    /** A lexeme code. */
    Slecsem_code code;
    /** If the current character matches symbols[0], then the transition to the state
     *  first_state;
     *  if the current character matches symbols[1], then the transition to the state
     *  first_state + 1;
     *  if the current character matches symbols[2], then the transition to the state
     *  first_state + 2, and so on. */
    uint16_t        first_state;
};
#endif