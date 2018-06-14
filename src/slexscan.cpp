#include <set>
#include <string>
#include <vector>
#include "../include/slexscan.h"
#include "../include/get_init_state.h"
#include "../include/search_char.h"
#include "../include/belongs.h"
#include "../include/operations_with_sets.h"
#include "../include/mysize.h"
#include "../include/elem.h"


SLexScan::::Automaton_proc SLexScan::procs[]  = {
    &SLexScan::start_proc,     &SLexScan::unknown_proc, 
    &SLexScan::keyword_proc,   &SLexScan::ident_proc,   
    &SLexScan::delimiter_proc, &SLexScan::string_proc,  
    &SLexScan::number_proc
};

SLexScan::Final_proc       SLexScan::finals[] = {
    &SLexScan::none_final_proc,      &SLexScan::unknown_final_proc, 
    &SLexScan::keyword_final_proc,   &SLexScan::ident_final_proc,   
    &SLexScan::delimiter_final_proc, &SLexScan::string_final_proc,  
    &SLexScan::number_final_proc
};

enum Category{

};

/*
 * It happens that in std::map<K,V> the key type is integer, and a lot of keys with
 * the same corresponding values. If such a map must be a generated constant, then
 * this map can be optimized. Namely, iterating through a map using range-based for,
 * we will build a std::vector<std::pair<K, V>>. Then we group pairs std::pair<K, V>
 * in pairs in the form (segment, a value of type V), where 'segment' is a struct
 * consisting of lower bound and upper bound. Next, we permute the grouped pair in
 * the such way that in order to search for in the array of the resulting values we
 * can use the algorithm from the answer to exercise 6.2.24 of the book
 *     Knuth D.E. The art of computer programming. Volume 3. Sorting and search. ---
 *     2nd ed. --- Addison-Wesley, 1998.
 */

#define RandomAccessIterator typename
#define Callable             typename
#define Integral             typename
template<typename T>
struct Segment{
    T lower_bound;
    T upper_bound;

    Segment()               = default;
    Segment(const Segment&) = default;
    ~Segment()              = default;
};

template<typename T, typename V>
struct Segment_with_value{
    Segment<T> bounds;
    V          value;

    Segment_with_value()                          = default;
    Segment_with_value(const Segment_with_value&) = default;
    ~Segment_with_value()                         = default;
};

/* This function uses algorithm from the answer to the exercise 6.2.24 of the monography
 *  Knuth D.E. The art of computer programming. Volume 3. Sorting and search. --- 2nd ed.
 *  --- Addison-Wesley, 1998.
*/
template<RandomAccessIterator I, typename K>
std::pair<bool, size_t> knuth_find(I it_begin, I it_end, K key)
{
    std::pair<bool, size_t> result = {false, 0};
    size_t                  i      = 1;
    size_t                  n      = it_end - it_begin;
    while (i <= n) {
        const auto& curr        = it_begin[i - 1];
        const auto& curr_bounds = curr.bounds;
        if(key < curr_bounds.lower_bound){
            i = 2 * i;
        }else if(key > curr_bounds.upper_bound){
            i = 2 * i + 1;
        }else{
            result.first = true; result.second = i - 1;
            break;
        }
    }
    return result;
}

static const Segment_with_value<char32_t, uint64_t> categories_table[] = {
    {{'a', 'a'}, 65920},   {{':', '@'}, 512},     {{'А', 'а'}, 384},    
    {{',', ','}, 512},     {{'R', 'R'}, 384},     {{'q', 'q'}, 524672},   
    {{'о', 'о'}, 384},   {{'%', '&'}, 512},     {{'0', '0'}, 65784},    
    {{'D', 'E'}, 590208},  {{'Y', 'Z'}, 384},     {{'f', 'f'}, 65920},    
    {{'x', 'x'}, 16768},   {{'ж', 'ж'}, 384},   {{'х', 'х'}, 384},    
    {{'#', '#'}, 512},     {{'(', '*'}, 512},     {{'.', '.'}, 1049088},  
    {{'2', '7'}, 196832},  {{'B', 'B'}, 74112},   {{'G', 'P'}, 384},      
    {{'T', 'W'}, 384},     {{']', '^'}, 512},     {{'c', 'c'}, 65920},    
    {{'o', 'o'}, 33152},   {{'s', 's'}, 524672},  {{'{', '~'}, 512},      
    {{'г', 'д'}, 384},   {{'й', 'й'}, 384},   {{'у', 'у'}, 384},    
    {{'ч', 'я'}, 384},   {{'!', '!'}, 512},     {{'$', '$'}, 2048},     
    {{\', \'}, 4096},      {{'+', '+'}, 262656},  {{'-', '-'}, 262656},   
    {{'/', '/'}, 512},     {{'1', '1'}, 196848},  {{'8', '9'}, 196800},   
    {{'A', 'A'}, 65920},   {{'C', 'C'}, 65920},   {{'F', 'F'}, 65920},    
    {{'Q', 'Q'}, 524672},  {{'S', 'S'}, 524672},  {{'X', 'X'}, 16768},    
    {{'[', '['}, 512},     {{'_', '_'}, 384},     {{'b', 'b'}, 74112},    
    {{'d', 'e'}, 590208},  {{'g', 'n'}, 384},     {{'p', 'p'}, 384},      
    {{'r', 'r'}, 384},     {{'t', 'w'}, 384},     {{'y', 'z'}, 384},      
    {{'Ё', 'Ё'}, 384},   {{'б', 'в'}, 388},   {{'е', 'е'}, 388},    
    {{'з', 'и'}, 388},   {{'к', 'н'}, 388},   {{'п', 'т'}, 388},    
    {{'ф', 'ф'}, 388},   {{'ц', 'ц'}, 388},   {{'ё', 'ё'}, 384},    
    {{'\0', ' '}, 2},      {{\", \"}, 1024}
};

static constexpr size_t num_of_elems_in_categories_table = size(categories_table);

uint64_t get_categories_set(char32_t c)
{
    auto t = knuth_find(categories_table,
                        categories_table + num_of_elems_in_categories_table,
                        c);
    return t.first ? categories_table[t.second].value : (1ULL << Other)
}



bool SLexScan::start_proc(){
    bool t = true;
    state  = -1;
    /* For an automaton that processes a lexeme, the state with the number (-1) is
     * the state in which this automaton is initialized. */
    if(belongs(SPACES, char_categories)){
        if(U'\n' == ch){
            token.code        =  Slecsem_code::Newline;
            lexem_begin_line  =  loc->current_line;
            loc->current_line += U'\n' == ch;
        }
        return t;
    }
    lexem_begin_line = loc->current_line;
    
    if(belongs(KEYWORD_BEGIN, char_categories)){
        (loc->pcurrent_char)--;
        automaton = A_keyword;
        state     = -1;
        return t;
    }


    if(belongs(IDENTIFIER_BEGIN, char_categories)){
        (loc->pcurrent_char)--;
        buffer.clear();
        automaton = A_ident;
        state     = -1;
        return t;
    }


    if(belongs(DELIMITER_BEGIN, char_categories)){
        (loc->pcurrent_char)--;
        automaton = A_delimiter;
        state = -1;
        return t;
    }


    if(belongs(IDENTIFIER_BEGIN, char_categories)){
        (loc->pcurrent_char)--;
        buffer.clear();
        automaton = A_string;
        state     = -1;
        return t;
    }


    if(belongs(IDENTIFIER_BEGIN, char_categories)){
        (loc->pcurrent_char)--;
        int_val = 0;
          float_val = 0;
          is_float = false;
          integer_part = 0;
          fractional_part = 0;
          exponent = 1;
          exp_sign = 1;
          frac_part_num_digits = 0;
          token.code = Integer;
        automaton = A_string;
        state     = -1;
        return t;
    }

    return t;
}

bool SLexScan::unknown_proc(){
    return belongs(Other, char_categories);
}
static const State_for_char init_table_for_keywords[] = {
    {89, U'б'}, {124, U'в'}, {228, U'е'}, {206, U'з'}, {18, U'и'}, {223, U'к'}, {152, U'л'}, {0, U'м'}, {194, U'н'}, {38, U'п'}, {232, U'р'}, {164, U'с'}, {85, U'т'}, {199, U'ф'}, {112, U'ц'}
};

static const Elem keyword_jump_table[] = {
    {const_cast<char32_t*>(U"оа"), Slecsem_code::Unknown, 1},
{const_cast<char32_t*>(U"д"), Slecsem_code::Unknown, 3},
{const_cast<char32_t*>(U"сл"), Slecsem_code::Unknown, 4},
{const_cast<char32_t*>(U"у"), Slecsem_code::Unknown, 6},
{const_cast<char32_t*>(U"с"), Slecsem_code::Unknown, 7},
{const_cast<char32_t*>(U"е"), Slecsem_code::Unknown, 8},
{const_cast<char32_t*>(U"л"), Slecsem_code::Unknown, 9},
{const_cast<char32_t*>(U"и"), Slecsem_code::Unknown, 10},
{const_cast<char32_t*>(U"н"), Slecsem_code::Unknown, 11},
{const_cast<char32_t*>(U"ь"), Slecsem_code::Unknown, 12},
{const_cast<char32_t*>(U"в"), Slecsem_code::Unknown, 13},
{const_cast<char32_t*>(U"ь"), Slecsem_code::Unknown, 14},
{const_cast<char32_t*>(U""), Slecsem_code::Mod, 0},
{const_cast<char32_t*>(U""), Slecsem_code::Arr, 0},
{const_cast<char32_t*>(U"к"), Slecsem_code::Unknown, 17},
{const_cast<char32_t*>(U"о"), Slecsem_code::Unknown, 16},
{const_cast<char32_t*>(U"е"), Slecsem_code::Unknown, 17},
{const_cast<char32_t*>(U""), Slecsem_code::Small, 0},
{const_cast<char32_t*>(U"сн"), Slecsem_code::Unknown, 19},
{const_cast<char32_t*>(U"пт"), Slecsem_code::Unknown, 21},
{const_cast<char32_t*>(U"еа"), Slecsem_code::Unknown, 23},
{const_cast<char32_t*>(U"о"), Slecsem_code::Unknown, 25},
{const_cast<char32_t*>(U"и"), Slecsem_code::Unknown, 26},
{const_cast<char32_t*>(U"с"), Slecsem_code::Unknown, 27},
{const_cast<char32_t*>(U"ч"), Slecsem_code::Unknown, 28},
{const_cast<char32_t*>(U"л"), Slecsem_code::Unknown, 29},
{const_cast<char32_t*>(U"н"), Slecsem_code::Unknown, 30},
{const_cast<char32_t*>(U""), Slecsem_code::Ines, 0},
{const_cast<char32_t*>(U"е"), Slecsem_code::Unknown, 32},
{const_cast<char32_t*>(U"ь"), Slecsem_code::Unknown, 32},
{const_cast<char32_t*>(U"а"), Slecsem_code::Unknown, 33},
{const_cast<char32_t*>(U""), Slecsem_code::Else, 0},
{const_cast<char32_t*>(U"з"), Slecsem_code::Unknown, 34},
{const_cast<char32_t*>(U""), Slecsem_code::True, 0},
{const_cast<char32_t*>(U"у"), Slecsem_code::Unknown, 35},
{const_cast<char32_t*>(U"е"), Slecsem_code::Unknown, 36},
{const_cast<char32_t*>(U"т"), Slecsem_code::Unknown, 37},
{const_cast<char32_t*>(U""), Slecsem_code::Use, 0},
{const_cast<char32_t*>(U"реоа"), Slecsem_code::Unknown, 39},
{const_cast<char32_t*>(U"еио"), Slecsem_code::Unknown, 43},
{const_cast<char32_t*>(U"р"), Slecsem_code::Unknown, 46},
{const_cast<char32_t*>(U"кв"), Slecsem_code::Unknown, 47},
{const_cast<char32_t*>(U"у"), Slecsem_code::Unknown, 49},
{const_cast<char32_t*>(U"д"), Slecsem_code::Unknown, 50},
{const_cast<char32_t*>(U""), Slecsem_code::At, 0},
{const_cast<char32_t*>(U"д"), Slecsem_code::Unknown, 52},
{const_cast<char32_t*>(U"е"), Slecsem_code::Unknown, 53},
{const_cast<char32_t*>(U"ау"), Slecsem_code::Unknown, 54},
{const_cast<char32_t*>(U"т"), Slecsem_code::Unknown, 56},
{const_cast<char32_t*>(U"к"), Slecsem_code::Unknown, 57},
{const_cast<char32_t*>(U"о"), Slecsem_code::Unknown, 57},
{const_cast<char32_t*>(U"о"), Slecsem_code::Unknown, 58},
{const_cast<char32_t*>(U"мч"), Slecsem_code::Unknown, 59},
{const_cast<char32_t*>(U""), Slecsem_code::While, 0},
{const_cast<char32_t*>(U"д"), Slecsem_code::Unknown, 62},
{const_cast<char32_t*>(U"о"), Slecsem_code::Unknown, 63},
{const_cast<char32_t*>(U""), Slecsem_code::Spider, 0},
{const_cast<char32_t*>(U"с"), Slecsem_code::Unknown, 63},
{const_cast<char32_t*>(U"л"), Slecsem_code::Unknown, 64},
{const_cast<char32_t*>(U""), Slecsem_code::Var, 0},
{const_cast<char32_t*>(U"и"), Slecsem_code::Unknown, 66},
{const_cast<char32_t*>(U"а"), Slecsem_code::Unknown, 67},
{const_cast<char32_t*>(U"р"), Slecsem_code::Unknown, 68},
{const_cast<char32_t*>(U"т"), Slecsem_code::Unknown, 68},
{const_cast<char32_t*>(U"ж"), Slecsem_code::Unknown, 69},
{const_cast<char32_t*>(U"с"), Slecsem_code::Unknown, 70},
{const_cast<char32_t*>(U""), Slecsem_code::AsLongAs, 0},
{const_cast<char32_t*>(U"я"), Slecsem_code::Unknown, 72},
{const_cast<char32_t*>(U"а"), Slecsem_code::Unknown, 72},
{const_cast<char32_t*>(U"и"), Slecsem_code::Unknown, 73},
{const_cast<char32_t*>(U"л"), Slecsem_code::Unknown, 74},
{const_cast<char32_t*>(U"й"), Slecsem_code::Unknown, 75},
{const_cast<char32_t*>(U"в"), Slecsem_code::Unknown, 76},
{const_cast<char32_t*>(U""), Slecsem_code::Continue, 0},
{const_cast<char32_t*>(U"е"), Slecsem_code::Unknown, 78},
{const_cast<char32_t*>(U""), Slecsem_code::Rep, 0},
{const_cast<char32_t*>(U"л"), Slecsem_code::Unknown, 78},
{const_cast<char32_t*>(U"н"), Slecsem_code::Unknown, 79},
{const_cast<char32_t*>(U"я"), Slecsem_code::Unknown, 80},
{const_cast<char32_t*>(U"и"), Slecsem_code::Unknown, 81},
{const_cast<char32_t*>(U"е"), Slecsem_code::Unknown, 82},
{const_cast<char32_t*>(U"е"), Slecsem_code::Unknown, 83},
{const_cast<char32_t*>(U"т"), Slecsem_code::Unknown, 84},
{const_cast<char32_t*>(U""), Slecsem_code::Enum, 0},
{const_cast<char32_t*>(U""), Slecsem_code::Get, 0},
{const_cast<char32_t*>(U"ио"), Slecsem_code::Unknown, 86},
{const_cast<char32_t*>(U"п"), Slecsem_code::Unknown, 88},
{const_cast<char32_t*>(U""), Slecsem_code::Then, 0},
{const_cast<char32_t*>(U""), Slecsem_code::Type, 0},
{const_cast<char32_t*>(U"оеа"), Slecsem_code::Unknown, 90},
{const_cast<char32_t*>(U"л"), Slecsem_code::Unknown, 93},
{const_cast<char32_t*>(U"з"), Slecsem_code::Unknown, 94},
{const_cast<char32_t*>(U"й"), Slecsem_code::Unknown, 95},
{const_cast<char32_t*>(U"ь"), Slecsem_code::Unknown, 96},
{const_cast<char32_t*>(U"з"), Slecsem_code::Unknown, 97},
{const_cast<char32_t*>(U"т"), Slecsem_code::Unknown, 98},
{const_cast<char32_t*>(U"ш"), Slecsem_code::Unknown, 99},
{const_cast<char32_t*>(U"н"), Slecsem_code::Unknown, 100},
{const_cast<char32_t*>(U""), Slecsem_code::Byte, 0},
{const_cast<char32_t*>(U"о"), Slecsem_code::Unknown, 101},
{const_cast<char32_t*>(U"8136"), Slecsem_code::Unsgn, 102},
{const_cast<char32_t*>(U"е"), Slecsem_code::Unknown, 106},
{const_cast<char32_t*>(U""), Slecsem_code::Unsgn8, 0},
{const_cast<char32_t*>(U"62"), Slecsem_code::Unknown, 108},
{const_cast<char32_t*>(U"2"), Slecsem_code::Unknown, 110},
{const_cast<char32_t*>(U"4"), Slecsem_code::Unknown, 111},
{const_cast<char32_t*>(U""), Slecsem_code::Big, 0},
{const_cast<char32_t*>(U""), Slecsem_code::Unsgn16, 0},
{const_cast<char32_t*>(U"8"), Slecsem_code::Unknown, 113},
{const_cast<char32_t*>(U""), Slecsem_code::Unsgn32, 0},
{const_cast<char32_t*>(U""), Slecsem_code::Unsgn64, 0},
{const_cast<char32_t*>(U""), Slecsem_code::Unsgn128, 0},
{const_cast<char32_t*>(U"е"), Slecsem_code::Unknown, 113},
{const_cast<char32_t*>(U"л"), Slecsem_code::Unknown, 114},
{const_cast<char32_t*>(U"8136"), Slecsem_code::Int, 115},
{const_cast<char32_t*>(U""), Slecsem_code::Int8, 0},
{const_cast<char32_t*>(U"62"), Slecsem_code::Unknown, 120},
{const_cast<char32_t*>(U"2"), Slecsem_code::Unknown, 122},
{const_cast<char32_t*>(U"4"), Slecsem_code::Unknown, 123},
{const_cast<char32_t*>(U""), Slecsem_code::Int16, 0},
{const_cast<char32_t*>(U"8"), Slecsem_code::Unknown, 124},
{const_cast<char32_t*>(U""), Slecsem_code::Int32, 0},
{const_cast<char32_t*>(U""), Slecsem_code::Int64, 0},
{const_cast<char32_t*>(U""), Slecsem_code::Int128, 0},
{const_cast<char32_t*>(U"есыов"), Slecsem_code::Unknown, 125},
{const_cast<char32_t*>(U"щ"), Slecsem_code::Unknown, 130},
{const_cast<char32_t*>(U"ё"), Slecsem_code::Unknown, 131},
{const_cast<char32_t*>(U"йв"), Slecsem_code::Unknown, 132},
{const_cast<char32_t*>(U"з"), Slecsem_code::Unknown, 134},
{const_cast<char32_t*>(U"о"), Slecsem_code::Unknown, 135},
{const_cast<char32_t*>(U"361"), Slecsem_code::Float, 136},
{const_cast<char32_t*>(U""), Slecsem_code::End, 0},
{const_cast<char32_t*>(U"д"), Slecsem_code::Unknown, 140},
{const_cast<char32_t*>(U"о"), Slecsem_code::Unknown, 141},
{const_cast<char32_t*>(U"в"), Slecsem_code::Unknown, 142},
{const_cast<char32_t*>(U"д"), Slecsem_code::Unknown, 143},
{const_cast<char32_t*>(U"2"), Slecsem_code::Unknown, 143},
{const_cast<char32_t*>(U"4"), Slecsem_code::Unknown, 144},
{const_cast<char32_t*>(U"2"), Slecsem_code::Unknown, 145},
{const_cast<char32_t*>(U"и"), Slecsem_code::Unknown, 146},
{const_cast<char32_t*>(U"д"), Slecsem_code::Unknown, 147},
{const_cast<char32_t*>(U"р"), Slecsem_code::Unknown, 148},
{const_cast<char32_t*>(U""), Slecsem_code::Ins, 0},
{const_cast<char32_t*>(U""), Slecsem_code::Float32, 0},
{const_cast<char32_t*>(U""), Slecsem_code::Float64, 0},
{const_cast<char32_t*>(U"8"), Slecsem_code::Unknown, 151},
{const_cast<char32_t*>(U""), Slecsem_code::Break, 0},
{const_cast<char32_t*>(U""), Slecsem_code::Out, 0},
{const_cast<char32_t*>(U"а"), Slecsem_code::Unknown, 154},
{const_cast<char32_t*>(U""), Slecsem_code::Float128, 0},
{const_cast<char32_t*>(U"т"), Slecsem_code::Unknown, 152},
{const_cast<char32_t*>(U""), Slecsem_code::Back, 0},
{const_cast<char32_t*>(U"о"), Slecsem_code::Unknown, 153},
{const_cast<char32_t*>(U"гж"), Slecsem_code::Unknown, 154},
{const_cast<char32_t*>(U"8136"), Slecsem_code::Bool, 156},
{const_cast<char32_t*>(U"ь"), Slecsem_code::Unknown, 160},
{const_cast<char32_t*>(U""), Slecsem_code::Bool8, 0},
{const_cast<char32_t*>(U"6"), Slecsem_code::Unknown, 162},
{const_cast<char32_t*>(U"2"), Slecsem_code::Unknown, 163},
{const_cast<char32_t*>(U"4"), Slecsem_code::Unknown, 164},
{const_cast<char32_t*>(U""), Slecsem_code::False, 0},
{const_cast<char32_t*>(U""), Slecsem_code::Bool16, 0},
{const_cast<char32_t*>(U""), Slecsem_code::Bool32, 0},
{const_cast<char32_t*>(U""), Slecsem_code::Bool64, 0},
{const_cast<char32_t*>(U"итс"), Slecsem_code::Unknown, 165},
{const_cast<char32_t*>(U"м"), Slecsem_code::Unknown, 168},
{const_cast<char32_t*>(U"р"), Slecsem_code::Unknown, 169},
{const_cast<char32_t*>(U"ы"), Slecsem_code::Unknown, 170},
{const_cast<char32_t*>(U"в"), Slecsem_code::Unknown, 171},
{const_cast<char32_t*>(U"оу"), Slecsem_code::Unknown, 172},
{const_cast<char32_t*>(U"л"), Slecsem_code::Unknown, 174},
{const_cast<char32_t*>(U"813"), Slecsem_code::Symb, 175},
{const_cast<char32_t*>(U"к"), Slecsem_code::Unknown, 178},
{const_cast<char32_t*>(U"к"), Slecsem_code::Unknown, 179},
{const_cast<char32_t*>(U"к"), Slecsem_code::Unknown, 180},
{const_cast<char32_t*>(U""), Slecsem_code::Symb8, 0},
{const_cast<char32_t*>(U"6"), Slecsem_code::Unknown, 182},
{const_cast<char32_t*>(U"2"), Slecsem_code::Unknown, 183},
{const_cast<char32_t*>(U"а"), Slecsem_code::Unknown, 184},
{const_cast<char32_t*>(U"т"), Slecsem_code::Unknown, 185},
{const_cast<char32_t*>(U"а"), Slecsem_code::Unknown, 186},
{const_cast<char32_t*>(U""), Slecsem_code::Symb16, 0},
{const_cast<char32_t*>(U""), Slecsem_code::Symb32, 0},
{const_cast<char32_t*>(U"813"), Slecsem_code::Str, 188},
{const_cast<char32_t*>(U"у"), Slecsem_code::Unknown, 191},
{const_cast<char32_t*>(U""), Slecsem_code::Ref, 0},
{const_cast<char32_t*>(U""), Slecsem_code::Str8, 0},
{const_cast<char32_t*>(U"6"), Slecsem_code::Unknown, 191},
{const_cast<char32_t*>(U"2"), Slecsem_code::Unknown, 192},
{const_cast<char32_t*>(U"р"), Slecsem_code::Unknown, 193},
{const_cast<char32_t*>(U""), Slecsem_code::Str16, 0},
{const_cast<char32_t*>(U""), Slecsem_code::Str32, 0},
{const_cast<char32_t*>(U"а"), Slecsem_code::Unknown, 195},
{const_cast<char32_t*>(U""), Slecsem_code::Struct, 0},
{const_cast<char32_t*>(U"и"), Slecsem_code::Unknown, 195},
{const_cast<char32_t*>(U"ч"), Slecsem_code::Unknown, 196},
{const_cast<char32_t*>(U"т"), Slecsem_code::Unknown, 197},
{const_cast<char32_t*>(U"о"), Slecsem_code::Unknown, 198},
{const_cast<char32_t*>(U""), Slecsem_code::Nothng, 0},
{const_cast<char32_t*>(U"у"), Slecsem_code::Unknown, 200},
{const_cast<char32_t*>(U"н"), Slecsem_code::Unknown, 201},
{const_cast<char32_t*>(U"к"), Slecsem_code::Unknown, 202},
{const_cast<char32_t*>(U"ц"), Slecsem_code::Unknown, 203},
{const_cast<char32_t*>(U"и"), Slecsem_code::Unknown, 204},
{const_cast<char32_t*>(U"я"), Slecsem_code::Unknown, 205},
{const_cast<char32_t*>(U""), Slecsem_code::Func, 0},
{const_cast<char32_t*>(U"на"), Slecsem_code::Unknown, 207},
{const_cast<char32_t*>(U"а"), Slecsem_code::Unknown, 209},
{const_cast<char32_t*>(U"в"), Slecsem_code::Unknown, 210},
{const_cast<char32_t*>(U"ч"), Slecsem_code::Unknown, 211},
{const_cast<char32_t*>(U"е"), Slecsem_code::Unknown, 212},
{const_cast<char32_t*>(U"е"), Slecsem_code::Unknown, 213},
{const_cast<char32_t*>(U"р"), Slecsem_code::Unknown, 214},
{const_cast<char32_t*>(U"н"), Slecsem_code::Unknown, 215},
{const_cast<char32_t*>(U"ш"), Slecsem_code::Unknown, 216},
{const_cast<char32_t*>(U"и"), Slecsem_code::Unknown, 217},
{const_cast<char32_t*>(U"е"), Slecsem_code::Unknown, 218},
{const_cast<char32_t*>(U"е"), Slecsem_code::Unknown, 219},
{const_cast<char32_t*>(U"н"), Slecsem_code::Unknown, 220},
{const_cast<char32_t*>(U""), Slecsem_code::Val, 0},
{const_cast<char32_t*>(U"и"), Slecsem_code::Unknown, 222},
{const_cast<char32_t*>(U"е"), Slecsem_code::Unknown, 222},
{const_cast<char32_t*>(U""), Slecsem_code::Compl, 0},
{const_cast<char32_t*>(U"о"), Slecsem_code::Unknown, 224},
{const_cast<char32_t*>(U"н"), Slecsem_code::Unknown, 225},
{const_cast<char32_t*>(U"с"), Slecsem_code::Unknown, 226},
{const_cast<char32_t*>(U"т"), Slecsem_code::Unknown, 227},
{const_cast<char32_t*>(U""), Slecsem_code::Const, 0},
{const_cast<char32_t*>(U"с"), Slecsem_code::Unknown, 229},
{const_cast<char32_t*>(U"л"), Slecsem_code::Unknown, 230},
{const_cast<char32_t*>(U"и"), Slecsem_code::Unknown, 231},
{const_cast<char32_t*>(U""), Slecsem_code::If, 0},
{const_cast<char32_t*>(U"а"), Slecsem_code::Unknown, 233},
{const_cast<char32_t*>(U"з"), Slecsem_code::Unknown, 234},
{const_cast<char32_t*>(U"б"), Slecsem_code::Unknown, 235},
{const_cast<char32_t*>(U"о"), Slecsem_code::Unknown, 236},
{const_cast<char32_t*>(U"р"), Slecsem_code::Unknown, 237},
{const_cast<char32_t*>(U""), Slecsem_code::Pars, 0}
};bool SLexScan::keyword_proc(){
    bool t = false;
    if(-1 == state){
        state = get_init_state(ch, init_table_for_delimiters,
                               sizeof(init_table_for_keywords)/sizeof(State_for_char));
        token.code = keyword_jump_table[state].code;
        t = true;
        return t;
    }
    Elem elem  = keyword_jump_table[state];
    token.code = elem.code;
    int y = search_char(ch, elem.symbols);
    if(y != THERE_IS_NO_CHAR){
        state  =  elem.first_state + y;
        t      =  true;
        buffer += ch;
    }else if(belongs(AFTER_KEYWORDS, char_categories)){
        automaton  = A_ident;
        token.code = SA;
        t          = true;
    }else{
        
    }
    return t;


static const State_for_char init_table_for_delimiters[] = {
    {32, U'!'}, {109, U'#'}, {106, U'%'}, {52, U'&'}, {13, U'('}, {15, U')'}, {91, U'*'}, {84, U'+'}, {0, U','}, {87, U'-'}, {17, U'.'}, {100, U'/'}, {1, U':'}, {8, U';'}, {58, U'<'}, {73, U'='}, {65, U'>'}, {25, U'\?'}, {12, U'@'}, {9, U'['}, {11, U']'}, {46, U'^'}, {20, U'{'}, {26, U'|'}, {24, U'}'}, {74, U'~'}
};

static const Elem delim_jump_table[] = {
    {const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U"():]>="), Slecsem_code::, 2},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U":"), Slecsem_code::, 10},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U":"), Slecsem_code::, 14},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U":"), Slecsem_code::, 16},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U"|"), Slecsem_code::Unknown, 18},
{const_cast<char32_t*>(U"."), Slecsem_code::Unknown, 19},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U"."), Slecsem_code::, 21},
{const_cast<char32_t*>(U"."), Slecsem_code::Unknown, 22},
{const_cast<char32_t*>(U"}"), Slecsem_code::Unknown, 23},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U"|:"), Slecsem_code::, 27},
{const_cast<char32_t*>(U":"), Slecsem_code::, 29},
{const_cast<char32_t*>(U"="), Slecsem_code::, 30},
{const_cast<char32_t*>(U"="), Slecsem_code::Unknown, 31},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U"|^&="), Slecsem_code::, 33},
{const_cast<char32_t*>(U"|"), Slecsem_code::Unknown, 37},
{const_cast<char32_t*>(U"^"), Slecsem_code::Unknown, 38},
{const_cast<char32_t*>(U"&"), Slecsem_code::Unknown, 39},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U":"), Slecsem_code::, 40},
{const_cast<char32_t*>(U":"), Slecsem_code::, 41},
{const_cast<char32_t*>(U":"), Slecsem_code::, 42},
{const_cast<char32_t*>(U"="), Slecsem_code::Unknown, 43},
{const_cast<char32_t*>(U"="), Slecsem_code::Unknown, 44},
{const_cast<char32_t*>(U"="), Slecsem_code::Unknown, 45},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U"^:"), Slecsem_code::, 47},
{const_cast<char32_t*>(U":"), Slecsem_code::, 49},
{const_cast<char32_t*>(U"="), Slecsem_code::Unknown, 50},
{const_cast<char32_t*>(U"="), Slecsem_code::Unknown, 51},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U"&:"), Slecsem_code::, 53},
{const_cast<char32_t*>(U":"), Slecsem_code::, 55},
{const_cast<char32_t*>(U"="), Slecsem_code::Unknown, 56},
{const_cast<char32_t*>(U"="), Slecsem_code::Unknown, 57},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U"=<-:"), Slecsem_code::, 59},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U":"), Slecsem_code::, 64},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U"="), Slecsem_code::Unknown, 64},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U"=>"), Slecsem_code::, 66},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U">:"), Slecsem_code::, 69},
{const_cast<char32_t*>(U":"), Slecsem_code::, 70},
{const_cast<char32_t*>(U"="), Slecsem_code::Unknown, 71},
{const_cast<char32_t*>(U"="), Slecsem_code::Unknown, 72},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U"|^&"), Slecsem_code::, 75},
{const_cast<char32_t*>(U":"), Slecsem_code::, 78},
{const_cast<char32_t*>(U":"), Slecsem_code::, 79},
{const_cast<char32_t*>(U":"), Slecsem_code::, 80},
{const_cast<char32_t*>(U"="), Slecsem_code::Unknown, 81},
{const_cast<char32_t*>(U"="), Slecsem_code::Unknown, 82},
{const_cast<char32_t*>(U"="), Slecsem_code::Unknown, 83},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U":"), Slecsem_code::, 85},
{const_cast<char32_t*>(U"="), Slecsem_code::Unknown, 86},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U":>"), Slecsem_code::, 88},
{const_cast<char32_t*>(U"="), Slecsem_code::Unknown, 90},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U"*:"), Slecsem_code::, 92},
{const_cast<char32_t*>(U".:"), Slecsem_code::, 94},
{const_cast<char32_t*>(U"="), Slecsem_code::Unknown, 96},
{const_cast<char32_t*>(U":"), Slecsem_code::, 97},
{const_cast<char32_t*>(U"="), Slecsem_code::Unknown, 98},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U"="), Slecsem_code::Unknown, 99},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U".:"), Slecsem_code::, 101},
{const_cast<char32_t*>(U":"), Slecsem_code::, 103},
{const_cast<char32_t*>(U"="), Slecsem_code::Unknown, 104},
{const_cast<char32_t*>(U"="), Slecsem_code::Unknown, 105},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U":"), Slecsem_code::, 107},
{const_cast<char32_t*>(U"="), Slecsem_code::Unknown, 108},
{const_cast<char32_t*>(U""), Slecsem_code::, 0},
{const_cast<char32_t*>(U"#"), Slecsem_code::, 110},
{const_cast<char32_t*>(U""), Slecsem_code::, 0}
};bool SLexScan::delimiter_proc(){
    bool t = false;
    if(-1 == state){
        state = get_init_state(ch, init_table_for_delimiters,
                               sizeof(init_table_for_delimiters)/sizeof(State_for_char));
        token.code = delim_jump_table[state].code;
        t = true;
        return t;
    }
    Elem elem  = delim_jump_table[state];
    token.code = elem.code;
    int y = search_char(ch, elem.symbols);
    if(y != THERE_IS_NO_CHAR){
        state = elem.first_state + y; t = true;
    }
    return t;




void SLexScan::none_final_proc(){
    /* This subroutine will be called if, after reading the input text, it turned
     * out to be in the A_start automaton. Then we do not need to do anything. */
}
bool SLexScan::unknown_proc(){
    return belongs(Other, char_categories);
}
void SLexScan::keyword_final_proc(){
    token.code = keyword_jump_table[state].code;
}
void SLexScan::string_final_proc(){
    buffer.clear();
token.ident_index = ids->insert(buffer);
}
void SLexScan::delimiter_final_proc(){
    token.code = delim_jump_table[state].code;
}
void SLexScan::string_final_proc(){
    token.code=(buffer.length()==1)?Char:String;
    token.string_index = strs->insert(buffer);
}
void SLexScan::string_final_proc(){
    
          if(is_float){
            token.float_val=build_float();
            token.code = precision2code(precision);
          } else {
            token.int_val=integer_part;
            token.code = Integer;
          }

}


struct Jump_for_multilined_end{
    uint32_t next_state;
    char32_t jump_char;
};

static const Jump_for_multilined_end multilined_jumps[] = {

};

void SLexScan::omit_multilined_comment()
{
    size_t st = 0;
    while((ch = (loc->pcurrent_char)++)){
        auto j  = multilined_jumps[st];
        auto jc = j.jump_char;
        if(!jc){
            (loc->pcurrent_char)--;
            return;
        }
        if(ch == jc){
            st = j.next_state;
        }else{
            st = 0;
        }
    }
    printf("Unexpected end of a multi-line comment at line %zu.\n",
           lexem_begin_line_number());
    en->increment_number_of_errors();
    return;
}

void SLexScan::omit_singlelined_comment()
{
    while((ch = (loc->pcurrent_char)++)){
        if('\n' == ch){
            (loc->pcurrent_char)--;
            return;
        }
    }
    (loc->pcurrent_char)--;
    return;
}

Lexem_info SLexScan::current_lexem_()
{
    automaton   = A_start;
    token.code  = Slecsem_code::None;
    lexem_begin = loc->pcurrent_char;
    bool t      = true;
    while((ch = *(loc->pcurrent_char)++)){
        char_categories = get_categories_set(ch);
        t = (this->*procs[automaton])();
        if(!t){
            /* We get here only if the lexeme has already been read. At the same time,
             * the current automaton reads the character immediately after the end of
             * the token read, based on this symbol, it is decided that the token has
             * been read and the transition to the next character has been made.
             * Therefore, in order to not miss the first character of the next lexeme,
             * we need to decrease the pcurrent_char pointer by one. */
            (loc->pcurrent_char)--;
            return token;

        }
    }
    /* Here we can be, only if we have already read all the processed text. In this
     * case, the pointer to the current symbol indicates a byte, which is immediately
     * after the zero character, which is a sign of the end of the text. To avoid
     * entering subsequent calls outside the text, we need to go back to the null
     * character. */
    (loc->pcurrent_char)--;
    /* Further, since we are here, the end of the current token (perhaps unexpected)
     * has not yet been processed. It is necessary to perform this processing, and,
     * probably, to display any diagnostics. */
    (this->*finals[automaton])();
    return token;
}

Lexem_info SLexScan::current_lexem()
{
    for( ; ; ){
        auto l = current_lexem_();
        switch(l.code){
            case Slecsem_code::SINGLE_LINED_COMMENT_MARK:
                omit_singlelined_comment();
                break;
            case Slecsem_code::MULTI_LINED_COMMENT_MARK:
                omit_multilined_comment();
                break;
            case Slecsem_code::MULTI_LINED_COMMENT_END:
                printf("Unexpected end of a multi-line comment at line %zu.\n",
                       lexem_begin_line_number());
                en->increment_number_of_errors();
                break;
            default:
                return token;
        }
    }
    return token;
}

