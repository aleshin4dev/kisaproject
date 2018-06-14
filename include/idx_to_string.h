/*
    File:    idx_to_string.h
    Created: 13 December 2015 at 09:05 Moscow time
    Author:  Гаврилов Владимир Сергеевич
    E-mails: vladimir.s.gavrilov@gmail.com
             gavrilov.vladimir.s@mail.ru
             gavvs1977@yandex.ru
*/

#ifndef IDX_TO_STRING_H
#define IDX_TO_STRING_H

# include "../include/char_trie.h"
#include <string>
#include <memory>
/**
 *  \param [in] t    pointer to prefix tree
 *  \param [in] idx  index of string in the prefix tree t
 *
 *  \return          string corresponding to the index idx
 *  */
std::string idx_to_string(std::shared_ptr<Char_trie> t,
                          size_t                     idx,
                          std::string                default_value = std::string());
#endif