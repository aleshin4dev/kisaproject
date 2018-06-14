std::shared_ptr<ast::Outp> Parser::Impl::output()
{
	std::shared_ptr<ast::Outp> result;
	enum class State{
		Start, Second, Third, Fourth, Five
	};
	State state = State::Start;
	std::shared_ptr<ast::Simplest_type_def> current_type;
	
	for(;;){
		Main_lexem_info li = sc_->current_lexem();
		Lexem_category cat = get_lexem_category(li);
		switch(state){
			case State::Start:
				if(cat != Lexem_category::Output){
                    printf(expected_Output_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Second; 
                break;
			case State::Second:
				if(cat != Lexem_category::Obracket){
                    printf(expected_Obracket_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Third; 
                break;
			case State::Third:
				if(cat != Lexem_category::Exp){
                    printf(expected_Exp_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Fourth; 
                break;
			case State::Fourth:
				switch(cat){
					case Lexem_category::Comma:
						state = State::Third;
						break;
					case Lexem_category::Cbracket:
						state = State::Five;
						break;
					default:
                        printf(expected_Comma_Cbracket_fmt,
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
				}
				break;
			case State::Five:
				sc_->back();
                return result; 
		}
	}
	return result;
}
