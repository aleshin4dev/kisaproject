#include <cstdlib>
#include <cstdio>
#include <memory>
#include "../include/fsize.h"
#include "../include/error_count.h"
#include "../include/location.h"
#include "../include/trie.h"
#include "../include/slexscan.h"
#include "../include/char_conv.h"
#include "../include/print_lexem.h"
#include "../include/errors_and_tries.h"
#include "../include/file_contents.h"
#include "../include/char_trie.h"

/* Function that opens a file with text. Returns a string with text if the file was
 * opened and the file size is not zero, and an empty string otherwise. */
static std::u32string get_processed_text(const char* name)
{
    auto contents = get_contents(name);
    auto str      = contents.second;
    switch(contents.first){
        case Get_contents_return_code::Normal:
            if(!str.length()){
                puts("File length is equal to zero.");
                return std::u32string();
            }else{
                return utf8_to_u32string(str.c_str());
            }
            break;

        case Get_contents_return_code::Impossible_open:
            puts("Unable to open file.");
            return std::u32string();

        case Get_contents_return_code::Read_error:
            puts("Error reading file.");
            return std::u32string();
    }
    return std::u32string();
}

enum class Myauka_exit_codes{
    Success, No_args
};

int main(int argc, char** argv)
{
    if(1 == argc){
        puts("Arguments are not specified.");
        return static_cast<int>(Myauka_exit_codes::No_args);
    }

    std::u32string t = get_processed_text(argv[1]);
    if(t.length()){
        char32_t* p   = const_cast<char32_t*>(t.c_str());
        auto      loc = std::make_shared<Location>(p);

        Errors_and_tries etr;
        etr.ec        = std::make_shared<Error_count>();
        etr.ids_trie  = std::make_shared<Char_trie>();
        etr.strs_trie = std::make_shared<Char_trie>();
        auto sc       = std::make_shared<SLexScan>(loc, etr);

        test_scaner(sc);
    }

    return static_cast<int>(Myauka_exit_codes::Success);
}
