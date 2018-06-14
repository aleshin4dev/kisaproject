std::shared_ptr<ast::Switch> Parser::Impl::swt()
{
	std::shared_ptr<ast::Switch> result;
	enum class State{
		Start, Second, Third, Fourth, Five, Six
	};
	State state = State::Start;
	std::shared_ptr<ast::Simplest_type_def> current_type;

	scope::Id info			id info;
	
	for(;;){
		Main_lexem_info li = sc_->current_lexem();
		Lexem_category cat = get_lexem_category(li);
		switch(state){
			case State::Start:
				switch(cat){
					case Lexem_category::Exit:
						state = State::Second;
						break;
					case Lexem_category::Continue:
						state = State::Third;
						break;
					case Lexem_category::Return:
						state = State::Fourth;
						break;
					default:
                        printf(expected_Exit_Continue_Return_fmt,
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
				}
				break;
			case State::Second:
				if(cat != Lexem_category::From){
                    sc_->back();                    
                    return result;
                }else{
                    state = State::Five;
                }
                break;
			case State::Third:
				if(cat != Lexem_category::Id){
                    sc_->back();                    
                    return result;
                }else{
                    state = State::Six;
                }
                break;
			case State::Fourth:
				if(cat != Lexem_category::Exp){
                    sc_->back();                    
                    return result;
                }else{
                    state = State::Six;
                }
                break;
			case State::Five:
				if(cat != Lexem_category::Id){
                    printf(expected_Id_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Six; 
                break;
			case State::Six:
				sc_->back();
                return result; 
		}
	}
	return result;
}
