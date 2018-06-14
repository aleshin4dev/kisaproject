std::shared_ptr<ast::Assign_stmt> Parser::Impl::assign()
{
	std::shared_ptr<ast::Assign_stmt> result;
	enum class State{
		Start, Second, Third, Fourth
	};
	State state = State::Start;
	std::shared_ptr<ast::Simplest_type_def> current_type;
	
	for(;;){
		Main_lexem_info li = sc_->current_lexem();
		Lexem_category cat = get_lexem_category(li);
		switch(state){
			case State::Start:
				if(cat != Lexem_category::Name){
                    printf(expected_Name_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Second; 
                break;
			case State::Second:
				if(cat != Lexem_category::Assignment_kind){
                    printf(expected_Assignment_kind_fmt, sc_->lexem_begin_line_number());
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
				sc_->back();
                return result; 
		}
	}
	return result;
}
