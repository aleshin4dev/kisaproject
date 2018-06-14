#ifndef PRINT_LEXEM_H
#define PRINT_LEXEM_H
#include <memory>
#include "../include/lexem_info.h"
#include "../include/slexscan.h"
void print_lexem(const Lexem_info& li);
void test_scaner(std::shared_ptr<SLexScan>& sc);
#endif