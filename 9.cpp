std::shared_ptr<ast::Type_def> Parser::Impl::proc_compound_type_definition() // 9
{
    std::shared_ptr<ast::Type_def> result;
    enum class State{
        Start,          Begin_tuple, Func, 
        Algebraic_part, Tuple_elem,  End_tuple, 
        Algebraic_sep
    };
    State state = State::Start;
    for(;;){
        Main_lexem_info li  = sc_->current_lexem();
        Lexem_category  cat = get_lexem_category(li);
        switch(state){
            case State::Start:   
                switch(cat){
                    case Lexem_category::Tuple_begin:
                        state = State::Begin_tuple;
                        break;
                    case Lexem_category::Kw_function:
                        state = State::Func;
                        break;
                    case Lexem_category::Kw_enum:
                    case Lexem_category::Kw_struct:
                        state = State::Algebraic_part;
                        break;
                    default:
                        ;
                }
                break;
            case State::Begin_tuple:
                if(cat != Lexem_category::ElType){
                    printf(expected_ElType_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Tuple_elem;
                break;
            case State::Func: 
                if(cat != Lexem_category::FuncSign){
                    printf(expected_FuncSign_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::End_tuple;
                break;
            case State::Algebraic_part: 
                if(cat != Lexem_category::DbarD){
                    sc_->back();                    
                    return result;
                }else{
                    state = State::Algebraic_sep;
                }
                break;
            case State::Tuple_elem:
                switch(cat){
                    case Lexem_category::Comma:
                        state = State::Begin_tuple;
                        break;
                    case Lexem_category::Col_cbr:
						state = State::End_tuple;
						break;
                    default:
                        printf(expected_enum_struct_fmt, 
                               sc_->lexem_Comma_Col_cbr());
                        et_.ec->increment_number_of_errors();
                        return result;
                }  
                break;
            case State::End_tuple:
                sc_->back();
                return result;
            case State::Algebraic_sep:
                if(cat != Lexem_category::Part_alg_type){
                    printf(expected_Part_alg_type_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Algebraic_part;
                break;
        }        
    }
    return result;
}
