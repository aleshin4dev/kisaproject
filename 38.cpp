std::shared_ptr<ast::Expr> Parser::Impl::pars_expr()
{
	std::shared_ptr<ast::Expr> result;
	enum class State{
		Start, Second, Third, Fourth, Five,
               Six,    Sev,   Eight,  Nine,
			   Ten,	   El,	  Tw, 	  Thir, Fourt
	};
	State state = State::Start;
	std::shared_ptr<ast::Simplest_type_def> current_type;
	
	for(;;){
		Main_lexem_info li = sc_->current_lexem();
		Lexem_category cat = get_lexem_category(li);
		switch(state){
			case State::Start:
				if(cat != Lexem_category::Pars){
                    printf(expected_Pars_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Second; 
                break;
			case State::Second:
				if(cat != Lexem_category::Exp){
                    printf(expected_Exp_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Third; 
                break;
			case State::Third:
				if(cat != Lexem_category::Obrace){
                    printf(expected_Obrace_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Fourth; 
                break;
			case State::Fourth:
				if(cat != Lexem_category::Sample){
                    printf(expected_Sample_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Five; 
                break;
			case State::Five:
				if(cat != Lexem_category::Rarrow){
                    printf(expected_Rarrow_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Six; 
                break;
			case State::Six:
				if(cat != Lexem_category::Sample){
                    printf(expected_Sample_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Sev; 
                break;
			case State::Sev:
				if(cat != Lexem_category::BoB){
                    printf(expected_BoB_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Eight; 
                break;
			case State::Eight:
				if(cat != Lexem_category::Cbrace){
                    printf(expected_Cbrace_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Nine; 
                break;
			case State::Nine:
				switch(cat){
					case Lexem_category::Cbrace:
						state = State::Fourt;
						break;
					case Lexem_category::Sample:
						state = State::Five;
						break;
					case Lexem_category::Else:
						state = State::Ten;
						break;
					default:
                        printf(expected_Cbrace_Sample_Else_fmt,
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
				}
				break;
			case State::Ten:
				if(cat != Lexem_category::Obrace){
                    printf(expected_Obrace_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::El; 
                break;
			case State::El:
				if(cat != Lexem_category::BoB){
                    printf(expected_BoB_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Tw; 
                break;
			case State::Tw:
				if(cat != Lexem_category::Cbrace){
                    printf(expected_Cbrace_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Thir; 
                break;
			case State::Thir:
				if(cat != Lexem_category::Cbrace){
                    printf(expected_Cbrace_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Fourt; 
                break;
			case State::Fourt:
                sc_->back();
                return result; 
		}
	}
	return result;
}
