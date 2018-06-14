/*
    File:    idx_to_string.cpp
    Created: 13 December 2015 at 09:05 Moscow time
    Author:  Гаврилов Владимир Сергеевич
    E-mails: vladimir.s.gavrilov@gmail.com
             gavrilov.vladimir.s@mail.ru
             gavvs1977@yandex.ru
*/

#include "../include/idx_to_string.h"
#include "../include/char_conv.h"
std::string idx_to_string(std::shared_ptr<Char_trie> t,
                          size_t                     idx,
                          std::string                default_value)
{
    auto u32str = t->get_string(idx);
    std::string s = u32string_to_utf8(u32str);
    return idx ? s : default_value;
}