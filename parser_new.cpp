std::shared_ptr<ast::Algebraic_type_component> Parser::Impl::proc_alg_type_component() // 10
{
	std::shared_ptr<ast::Algebraic_type_component> result;
	enum class State{
		Start, Enum, Struct, FirstId, SecondId,
		FirstOpen, SecondOpen, Iden, Group, Finish
	};
	State state = State::Start;
	
	scope::Id_info     id_info;
	
	for(;;){
		Main_lexem_info li = sc_->current_lexem();
		Lexem_category cat = get_lexem_category(li);
		switch(state){
			case State::Start:
				switch(cat){
                    case Lexem_category::Kw_enum:
                        state = State::Enum;
                        break;
                    case Lexem_category::Kw_struct:
						state = State::Struct;
						break;
                    default:
                        printf(expected_enum_struct_fmt, 
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
                }
                break;
			case State::Enum:
				if(cat != Lexem_category::Ident){
                    printf(expected_ident_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::FirstId; 
                break;
			case State::Struct:
				if(cat != Lexem_category::Ident){
                    printf(expected_ident_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::SecondId; 
                break;
			case State::FirstId:
				if(cat != Lexem_category::Open_curly_bracket){
                    printf(expected_open_curly_bracket_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::FirstOpen; 
                break;
			case State::SecondId:
				if(cat != Lexem_category::Open_curly_bracket){
                    printf(expected_open_curly_bracket_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::SecondOpen; 
                break;
			case State::FirstOpen:
				if(cat != Lexem_category::Ident){
                    printf(expected_ident_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Iden; 
                break;
			case State::SecondOpen:
				if(cat != Lexem_category::Ident){
					sc_->back();
                    printf(expected_field_group_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Group; 
                break;
			case State::Iden:
				switch(cat){
                    case Lexem_category::Closed_curly_bracket:
						sc_->back();
                        state = State::Finish;
                        break;
                    case Lexem_category::Comma:
						state = State::FirstOpen;
						break;
                    default:
                        printf(expected_bracket_comma_fmt, 
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
                }
                break;
			case State::Group:
				switch(cat){
                    case Lexem_category::Closed_curly_bracket:
						sc_->back();
                        state = State::Finish;
                        break;
                    case Lexem_category::Semicolon:
						state = State::SecondOpen;
						break;
                    default:
                        printf(expected_bracket_semicolon_fmt, 
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
                }
                break;
			case State::Finish
                sc_->back();
                return result; 
		}
	}
	return result;
}

std::shared_ptr<ast::Enum_def> Parser::Impl::proc_enum_def() // 11
{
	std::shared_ptr<ast::Enum_def> result;
	enum class State{
		Start, Var, NewColon, Simplest_typedef
	};
	State state = State::Start;
	std::list<ast::Id>                      vars;
    std::shared_ptr<ast::Simplest_type_def> current_type;
    bool                                    correct_var_name;
	for(;;){
		Main_lexem_info li = sc_->current_lexem();
		Lexem_category cat = get_lexem_category(li);
		switch(state){
			case State::Start:
				if(cat != Lexem_category::Ident){
                    printf(expected_ident_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
				correct_var_name = check2_var_name(li.ident_index);
                if(!correct_var_name){
                    printf(uncorrect_var_name_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::Var; 
                break;
			case State::Var:
				switch(cat){
					case Lexem_category::Comma:
                        state = State::Start;
                        break;
					case Lexem_category::Colon:
						state = State::NewColon;
						break;
					default:
                        printf(expected_comma_colon_fmt,
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
				}
				break;
			case State::NewColon:
				switch(cat){
					case Lexem_category::Kw_array:
					case Lexem_category::At_sign:
					case Lexem_category::Ident:
					case Lexem_category::Module_name_prefix:
					case Lexem_category::Modified_type:
					case Lexem_category::Non_modified_type:
					case Lexem_category::Size_changer:
                        sc_->back();
                        current_type = proc_simplest_type_def();
                        state = State::Simplest_typedef;
                        break;
					default:
						printf(expected_form_var_fmt, 
								sc_->lexem_begin_line_number());
						et_.ec->increment_number_of_errors();
						return result;	
				}
			case State::Simplest_typedef
                sc_->back();
                return result; 
		}
	}
	return result;
}

std::shared_ptr<ast::Func_ptr> Parser::Impl::proc_func_ptr() // 12
{
	std::shared_ptr<ast::Func_ptr> result;
	enum class State{
		Start, RoundOpen, FormArg, RoundClose, Semi,
		Colon, Finish
	};
	
	std::shared_ptr<Elementary_type> elem_type;
	std::shared_ptr<Formal_arg_info> form_var;
	
	State state = State::Start;
	for(;;){
		Main_lexem_info li = sc_->current_lexem();
		Lexem_category cat = get_lexem_category(li);
		switch(state){
			case State::Start:
				if(cat != Lexem_category::Open_round_bracket){
                    printf(expected_Open_round_bracket_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::RoundOpen; 
                break;
			case State::RoundOpen:
				switch(cat){
					case Lexem_category::Kw_value:
					case Lexem_category::Kw_ref:
					case Lexem_category::Kw_const:
						sc_->back();
						form_var = proc_form_var();
						state = State::FormArg;
						break;
					case Lexem_category::Closed_round_bracket:
						state = State::RoundClose;
						break;
					default:
                        printf(expected_value_ref_const_bracket_fmt,
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
				}
				break;
			case State::FormArg:
				switch(cat){
					case Lexem_category::Semicolon:
						state = State::Semi;
						break;
					case Lexem_category::Closed_round_bracket:
						state = State::RoundClose;
						break;
					default:
                        printf(expected_semicolon_bracket_fmt,
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
				}
				break;
			case State::RoundClose:
				if(cat != Lexem_category::Colon){
                    printf(expected_colon_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Colon; 
                break;
			case State::Semi:
				switch(cat){
					case Lexem_category::Kw_value:
					case Lexem_category::Kw_ref:
					case Lexem_category::Kw_const:
						sc_->back();
						form_var = proc_form_var();
						state = State::FormArg;
						break;
					default:
                        printf(expected_value_ref_const_fmt,
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
				}
				break;
			case State::Colon:
				switch(cat){
					case Lexem_category::At_sign:
                    case Lexem_category::Ident:
                    case Lexem_category::Module_name_prefix:
                    case Lexem_category::Modified_type:
                    case Lexem_category::Non_modified_type:
                    case Lexem_category::Size_changer:
                        sc_->back();
                        elem_type = proc_elementary_type();
                        state     = State::Finish;                        
                        break;
					default:
                        printf(expected_elementary_fmt,
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
				}
				break;
			case State::Five
                sc_->back();
                return result; 
		}
	}
	return result;
	
}

bool Parser::Impl::check2_var_name(ast::Id name)
{
    auto sr = st_->search(name);
    if(sr.status != symbol_table::Search_status::Found_in_the_current_scope)
    {
        return true;
    }else{
        auto id_str = idx_to_string(et_.ids_trie, name);
        printf(var_name_diagnosis_fmt[static_cast<unsigned>(sr.attr)],
               sc_->lexem_begin_line_number(),
               id_str.c_str());
        et_.ec->increment_number_of_errors();
        return false;
    }
}

std::shared_ptr<ast::Formal_arg_info> Parser::Impl::proc_form_var() // 13
{
	std::shared_ptr<ast::Formal_arg_info> result;
	enum class State{
		Start, Val, Second, Third, Fourth, Five
	};
	State state = State::Start;
	std::list<ast::Id>                      vars;
    std::shared_ptr<ast::Simplest_type_def> current_type;
    bool                                    correct_var_name;
	for(;;){
		Main_lexem_info li = sc_->current_lexem();
		Lexem_category cat = get_lexem_category(li);
		switch(state){
			case State::Start:
				switch(cat){
					case Lexem_category::Kw_value:
					case Lexem_category::Kw_ref:
                        state = State::Val;
                        break;
					case Lexem_category::Kw_const:
						state = State::Second;
						break;
					default:
                        printf(expected_value_ref_const_fmt,
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
				}
				break;
			case State::Val:
				if(cat != Lexem_category::Ident){
                    printf(expected_ident_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
				correct_var_name = check2_var_name(li.ident_index);
                if(!correct_var_name){
                    printf(uncorrect_var_name_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::Third; 
                break;
			case State::Second:
				if(cat != Lexem_category::Kw_ref){
                    printf(expected_Kw_ref_fmt, sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;                    
                }
                state = State::Val; 
                break;
			case State::Third:
				switch(cat){
					case Lexem_category::Comma:
						state = State::Val;
						break;
					case Lexem_category::Colon:
						state = State::Fourth;
						break;
					default:
                        printf(expected_comma_colon_fmt,
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
				}
				break;
			case State::Fourth:
				switch(cat){
					case Lexem_category::Kw_array:
					case Lexem_category::At_sign:
					case Lexem_category::Ident:
					case Lexem_category::Module_name_prefix:
					case Lexem_category::Modified_type:
					case Lexem_category::Non_modified_type:
					case Lexem_category::Size_changer:
                        sc_->back();
                        current_type = proc_simplest_type_def();
                        state = State::Five;
                        break;
					default:
						sc_->back();
						printf(expected_form_var_fmt, 
								sc_->lexem_begin_line_number());
						et_.ec->increment_number_of_errors();
						return result;	
				}
                break;
			case State::Five
                sc_->back();
                return result; 
		}
	}
	return result;
}

std::list<std::shared_ptr<ast::Definition>> Parser::Impl::proc_func_proto() // 14
{
    std::list<std::shared_ptr<ast::Definition>> result;
	enum class State{
		Start, Fun, Name, Sign_func, Semi, 
		Open_curly, Block
	};
	State state = State::Start;
	ast::Id   current_name = 0;
	ast::Block_body   block;
	std::shared_ptr<ast::Func_ptr>   func;
    for(;;){
        Main_lexem_info li  = sc_->current_lexem();
        Lexem_category  cat = get_lexem_category(li);
        switch(state){
            case State::Start:
                if(cat != Lexem_category::Kw_function){
                    printf(expected_function_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::Fun;
                break;
			case State::Fun:
				if(cat != Lexem_category::Ident){
                    printf(expected_ident_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::Name;
                break;
			case State::Name:
				switch(cat){
					case Lexem_category::Open_round_bracket:
						sc_->back();
						func = proc_func_ptr();
						state = State::Sign_func;
						break;
					default:
                        printf(expected_open_round_bracket_fmt,
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
				}
				break;
			case State::Sign_func:
				switch(cat){
					case Lexem_category::Semicolon:
						sc_->back();
						state = State::Semi;
						break;
					case Lexem_category::Open_curly_bracket:
						state = State::Open_curly;
						break;
					default:
                        printf(expected_semicolon_bracket_fmt,
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
				}
				break;
			case State::Semi:
				sc_->back();
                return result;
			case State::Open_curly:
				switch(cat){
					case Lexem_category::Kw_var:
                    case Lexem_category::Kw_type:
                    case Lexem_category::Kw_function:
                    case Lexem_category::Kw_const:
					case Lexem_category::Semicolon:
					case Lexem_category::Label_prefix:
                    case Lexem_category::Kw_for:
                    case Lexem_category::Kw_while:
                    case Lexem_category::Kw_repeat:
					case Lexem_category::Module_name_prefix:
                    case Lexem_category::Ident:
                    case Lexem_category::Kw_exit:
                    case Lexem_category::Kw_read:
					case Lexem_category::Kw_print:
                    case Lexem_category::Kw_if:
                    case Lexem_category::Kw_match:
						sc_->back();
						state = State::Block;
						break;
					default:
                        printf(expected_block_body_fmt,
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
				}
				break;
			case State::Block:
				if(cat != Lexem_category::Closed_curly_bracket){
                    printf(expected_closed_curly_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
				sc_->back();
                state = State::Semi;
                break;
		}
	}
	return result;
}	

std::list<std::shared_ptr<ast::Constant>> Parser::Impl::proc_const() // 15
{
    std::list<std::shared_ptr<ast::Constant>> result;
	enum class State{
		Start, Const, Name, Colon, Simplest_typedef, 
		Equal, Finish
	};
	State state = State::Start;
	ast::Id   current_name = 0;
	std::shared_ptr<ast::Simplest_type_def> current_type;
	std::shared_ptr<ast::Expr> expression;
    for(;;){
        Main_lexem_info li  = sc_->current_lexem();
        Lexem_category  cat = get_lexem_category(li);
        switch(state){
            case State::Start:
                if(cat != Lexem_category::Kw_const){
                    printf(expected_const_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::Const;
                break;
			case State::Const:
				if(cat != Lexem_category::Ident){
                    printf(expected_ident_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::Name;
                break;
			case State::Name:
				if(cat != Lexem_category::Colon){
                    printf(expected_colon_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::Colon;
                break;
			case State::Colon:
				switch(cat){
					case Lexem_category::Kw_array:
					case Lexem_category::At_sign:
					case Lexem_category::Ident:
					case Lexem_category::Module_name_prefix:
					case Lexem_category::Modified_type:
					case Lexem_category::Non_modified_type:
					case Lexem_category::Size_changer:
                        sc_->back();
                        current_type = proc_simplest_type_def();
                        state = State::Simplest_typedef;
                        break;
					default:
						printf(expected_form_var_fmt, 
								sc_->lexem_begin_line_number());
						et_.ec->increment_number_of_errors();
						return result;	
				}
				break;
			case State::Simplest_typedef:
				if(cat != Lexem_category::Equal_sign){
                    printf(expected_colon_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::Equal;
                break;
			case State::Equal:
				switch(cat){
					case Lexem_category::Logical_not:
					case Lexem_category::Bitwise_not:
					case Lexem_category::Plus:
					case Lexem_category::Minus:
					case Lexem_category::Sizeof:
					case Lexem_category::Kw_int:
					case Lexem_category::Kw_float:
					case Lexem_category::Kw_string:
					case Lexem_category::Kw_true:
					case Lexem_category::Kw_false:
					case Lexem_category::Open_round_bracket:
					case Lexem_category::Ident:
					case Lexem_category::Module_name_prefix:
					case Lexem_category::Kw_array:
                        sc_->back();
                        expression = proc_expr();
                        state = State::Finish;
                        break;
					default:
						sc_->back();
						printf(expected_form_var_fmt, 
								sc_->lexem_begin_line_number());
						et_.ec->increment_number_of_errors();
						return result;	
				}
				break;
			case State::Finish:
				if(cat != Lexem_category::Comma){
                    printf(expected_colon_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
				sc_->back();
                state = State::Const;
                break;
		}
	}
    return result;
}	





std::shared_ptr<ast::Binary_op> Parser::Impl::proc_expr_eleven() // 28
{
	std::shared_ptr<ast::Binary_op> result;
	enum class State{
		Start, Twelve, Size, Finish
	};
	State state = State::Start;
	std::shared_ptr<ast::Alloc_array_expr> expr;
	for(;;){
		Main_lexem_info li = sc_->current_lexem();
		Lexem_category cat = get_lexem_category(li);
		switch(state){
			case State::Start:
				switch(cat){
					case Lexem_category::Kw_int:
					case Lexem_category::Kw_float:
					case Lexem_category::Kw_string:
					case Lexem_category::Kw_true:
					case Lexem_category::Kw_false:
					case Lexem_category::Open_round_bracket:
					case Lexem_category::Ident:
					case Lexem_category::Module_name_prefix:
					case Lexem_category::Kw_array:
                        sc_->back();
                        expr = proc_expr_twelve();
                        state = State::Twelve;
                        break;
					default:
						printf(expected_expr_twelve_fmt, 
								sc_->lexem_begin_line_number());
						et_.ec->increment_number_of_errors();
						return result;	
				}
				break;
			case State::Twelve:
				if(cat != Lexem_category::Dimension_size){
                    printf(expected_dim_size_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
				sc_->back();
                state = State::Size;
                break;
			case State::Size:
				switch(cat){
					case Lexem_category::Kw_int:
					case Lexem_category::Kw_float:
					case Lexem_category::Kw_string:
					case Lexem_category::Kw_true:
					case Lexem_category::Kw_false:
					case Lexem_category::Open_round_bracket:
					case Lexem_category::Ident:
					case Lexem_category::Module_name_prefix:
					case Lexem_category::Kw_array:
                        sc_->back();
                        expr = proc_expr_twelve();
                        state = State::Finish;
                        break;
					default:
						printf(expected_expr_twelve_fmt, 
								sc_->lexem_begin_line_number());
						et_.ec->increment_number_of_errors();
						return result;	
				}
				break;
			case State::Finish:
				sc_->back();
                return result; 
		}
	}
	return result;
}

std::shared_ptr<ast::Alloc_array_expr> Parser::Impl::proc_expr_twelve() // 29
{
	std::shared_ptr<ast::Alloc_array_expr> result;
	enum class State{
		Start, Finish, CBracket, DName, EArray, Factor, 
		GFirst_Expr, HOpen_curly, IFirst_Array_open,
		KSecond_Array_open, MIdent, NType, OList, PArrow,
		QFirst_Array_closed, RSemi, Second_Expr, 
		TNew_open_curly, USecond_Type, XSecond_List
	};
	State state = State::Start;
	
	std::shared_ptr<ast::Expr> expr;
	ast::Id   current_name = 0;
	std::shared_ptr<ast::Name> name;
	std::shared_ptr<ast::Elementary_type> elem_type;
	std::shared_ptr<ast::Actual_args> list;
	
	for(;;){
		Main_lexem_info li = sc_->current_lexem();
		Lexem_category cat = get_lexem_category(li);
		switch(state){
			case State::Start:
				switch(cat){
					case Lexem_category::Kw_int:
					case Lexem_category::Kw_float:
					case Lexem_category::Kw_string:
					case Lexem_category::Kw_true:
					case Lexem_category::Kw_false:
                        state = State::Finish;
                        break;
					case Lexem_category::Open_round_bracket:
						state = State::CBracket;
						break;
					case Lexem_category::Module_name_prefix:
					case Lexem_category::Ident:
						sc_->back();
						name = proc_name();
						state = State::DName;
						break;
					case Lexem_category::Kw_array:
						state = State::EArray;
						break;
					case Lexem_category::Kw_alloc:
						state = State::Factor;
						break;
					default:
						printf(expected_int_float_string_true_false_bracket_prefix_array_fmt, 
								sc_->lexem_begin_line_number());
						et_.ec->increment_number_of_errors();
						return result;	
				}
				break;
			case State::Finish:
				sc_->back();
                return result;
			case State::CBracket:
				switch(cat){
					case Lexem_category::Logical_not:
					case Lexem_category::Bitwise_not:
					case Lexem_category::Plus:
					case Lexem_category::Minus:
					case Lexem_category::Sizeof:
					case Lexem_category::Kw_int:
					case Lexem_category::Kw_float:
					case Lexem_category::Kw_string:
					case Lexem_category::Kw_true:
					case Lexem_category::Kw_false:
					case Lexem_category::Open_round_bracket:
					case Lexem_category::Ident:
					case Lexem_category::Module_name_prefix:
					case Lexem_category::Kw_array:
                        sc_->back();
                        expr = proc_expr();
                        state = State::GFirst_Expr;
                        break;
					default:
						printf(expected_expression_fmt, 
								sc_->lexem_begin_line_number());
						et_.ec->increment_number_of_errors();
						return result;	
				}
				break;
			case State::DName:
				if(cat != Lexem_category::Open_curly_bracket){
                    sc_->back();                    
                    return result;
                }else{
                    state = State::HOpen_curly;
                }
                break;
			case State::EArray:
				if(cat != Lexem_category::Allocated_array_open){
                    printf(expected_allocated_array_open_size_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::IFirst_Array_open;
                break;
			case State::Factor:
				switch(cat){
					case Lexem_category::Module_name_prefix:
					case Lexem_category::Ident:
						sc_->back();
						name = proc_name();
						state = State::Finish;
						break;
					case Lexem_category::Allocated_array_open:
						state = State::KSecond_Array_open;
						break;
					default:
						printf(expected_name_allocated_array_open_fmt, 
								sc_->lexem_begin_line_number());
						et_.ec->increment_number_of_errors();
						return result;	
				}
				break;
			case State::GFirst_Expr:
				if(cat != Lexem_category::Closed_round_bracket){
                    printf(expected_closed_round_bracket_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::Finish;
                break;
			case State::HOpen_curly:
				if(cat != Lexem_category::Ident){
                    printf(expected_ident_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::MIdent;
                break;
			case State::IFirst_Array_open:
				switch(cat){
                    case Lexem_category::At_sign:
                    case Lexem_category::Ident:
                    case Lexem_category::Module_name_prefix:
                    case Lexem_category::Modified_type:
                    case Lexem_category::Non_modified_type:
                    case Lexem_category::Size_changer:
                        sc_->back();
                        elem_type = proc_elementary_type();
                        state     = State::NType;                        
                        break;
                    default:
                        printf(expected_elem_type_fmt, 
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
                }
                break;
			case State::KSecond_Array_open:
				switch(cat){
					case Lexem_category::Logical_not:
					case Lexem_category::Bitwise_not:
					case Lexem_category::Plus:
					case Lexem_category::Minus:
					case Lexem_category::Sizeof:
					case Lexem_category::Kw_int:
					case Lexem_category::Kw_float:
					case Lexem_category::Kw_string:
					case Lexem_category::Kw_true:
					case Lexem_category::Kw_false:
					case Lexem_category::Open_round_bracket:
					case Lexem_category::Ident:
					case Lexem_category::Module_name_prefix:
					case Lexem_category::Kw_array:
                        sc_->back();
                        list = proc_expr_list();
                        state = State::OList;
                        break;
					default:
						printf(expected_expression_list_fmt, 
								sc_->lexem_begin_line_number());
						et_.ec->increment_number_of_errors();
						return result;	
				}
				break;
			case State::MIdent:
				if(cat != Lexem_category::Assign_to_field){
                    printf(expected_assign_to_field_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::PArrow;
                break;
			case State::NType:
				if(cat != Lexem_category::Allocated_array_close){
                    printf(expected_allocated_array_close_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::QFirst_Array_closed;
                break;
			case State::OList:
				if(cat != Lexem_category::Semicolon){
                    printf(expected_semicolon_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::RSemi;
                break;
			case State::PArrow:
				switch(cat){
					case Lexem_category::Logical_not:
					case Lexem_category::Bitwise_not:
					case Lexem_category::Plus:
					case Lexem_category::Minus:
					case Lexem_category::Sizeof:
					case Lexem_category::Kw_int:
					case Lexem_category::Kw_float:
					case Lexem_category::Kw_string:
					case Lexem_category::Kw_true:
					case Lexem_category::Kw_false:
					case Lexem_category::Open_round_bracket:
					case Lexem_category::Ident:
					case Lexem_category::Module_name_prefix:
					case Lexem_category::Kw_array:
                        sc_->back();
                        expr = proc_expr();
                        state = State::Second_Expr;
                        break;
					default:
						printf(expected_expression_fmt, 
								sc_->lexem_begin_line_number());
						et_.ec->increment_number_of_errors();
						return result;	
				}
				break;
			case State::QFirst_Array_closed:
				if(cat != Lexem_category::Open_curly_bracket){
                    printf(expected_open_curly_bracket_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::TNew_open_curly;
                break;
			case State::RSemi:
				switch(cat){
                    case Lexem_category::At_sign:
                    case Lexem_category::Ident:
                    case Lexem_category::Module_name_prefix:
                    case Lexem_category::Modified_type:
                    case Lexem_category::Non_modified_type:
                    case Lexem_category::Size_changer:
                        sc_->back();
                        elem_type = proc_elementary_type();
                        state     = State::USecond_Type;                        
                        break;
                    default:
                        printf(expected_elem_type_fmt, 
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
                }
                break;
			case State::Second_Expr:
				switch(cat){
                    case Lexem_category::Closed_curly_bracket:
                        state     = State::Finish;                        
                        break;
					case Lexem_category::Comma:
                        state     = State::HOpen_curly;                        
                        break;
                    default:
                        printf(expected_closed_curly_bracket_comma_fmt, 
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
                }
                break;
			case State::TNew_open_curly:
				switch(cat){
					case Lexem_category::Logical_not:
					case Lexem_category::Bitwise_not:
					case Lexem_category::Plus:
					case Lexem_category::Minus:
					case Lexem_category::Sizeof:
					case Lexem_category::Kw_int:
					case Lexem_category::Kw_float:
					case Lexem_category::Kw_string:
					case Lexem_category::Kw_true:
					case Lexem_category::Kw_false:
					case Lexem_category::Open_round_bracket:
					case Lexem_category::Ident:
					case Lexem_category::Module_name_prefix:
					case Lexem_category::Kw_array:
                        sc_->back();
                        list = proc_expr_list();
                        state = State::XSecond_List;
                        break;
					default:
						printf(expected_expression_list_fmt, 
								sc_->lexem_begin_line_number());
						et_.ec->increment_number_of_errors();
						return result;	
				}
				break;
			case State::USecond_Type:
				if(cat != Lexem_category::Allocated_array_close){
                    printf(expected_allocated_array_close_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::Finish;
                break;
			case State::XSecond_List:
				if(cat != Lexem_category::Closed_curly_bracket){
                    printf(expected_closed_curly_bracket_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::Finish;
                break;
		}
	}
	return result;
}

std::shared_ptr<ast::Name> Parser::Impl::proc_name() // 30
{
	std::shared_ptr<ast::Name> result;
	enum class State{
		Start, BModule_prefix, CId, DIdent, EResolution, 
		FOpen_round, GOpen_square, HClose, IDot,
		LFirst_component, MSecond_component
	};
	State state = State::Start;
	
	ast::Id module_name    = 0;
    ast::Id type_name      = 0;
    ast::Id type_part_name = 0;
    ast::Id id            = 0;
    std::list<std::shared_ptr<Name_component>> components;
	
	for(;;){
		Main_lexem_info li = sc_->current_lexem();
		Lexem_category cat = get_lexem_category(li);
		switch(state){
			case State::Start:
				switch(cat){
					case Lexem_category::Module_name_prefix:
                        state = State::BModule_prefix;
                        break;
					case Lexem_category::Ident:
						state = State::CId;
						break;
					default:
						printf(expected_prefix_ident_fmt, 
								sc_->lexem_begin_line_number());
						et_.ec->increment_number_of_errors();
						return result;	
				}
				break;
			case State::BModule_prefix:
				if(cat != Lexem_category::Ident){
                    printf(expected_ident_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::DIdent;
                break;
			case State::CId:
				switch(cat){
					case Lexem_category::Scope_resolution:
                        state = State::EResolution;
                        break;
					case Lexem_category::Open_round_bracket:
						state = State::FOpen_round;
						break;
					case Lexem_category::Open_square_bracket:
						state = State::GOpen_square;
						break;
					case Lexem_category::At_sign:
						state = State::HClose;
						break;
					case Lexem_category::Dot:
						state = State::IDot;
						break;
					default:
						sc_->back();                    
						return result;	
				}
				break;
			case State::DIdent:
				if(cat != Lexem_category::Scope_resolution){
                    printf(expected_scope_resolution_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::EResolution;
                break;
			case State::EResolution:
				if(cat != Lexem_category::Ident){
                    printf(expected_ident_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::CId;
                break;
			case State::FOpen_round:
				switch(cat){
					case Lexem_category::Closed_round_bracket:
                        state = State::HClose;
                        break;
					case Lexem_category::Logical_not:
					case Lexem_category::Bitwise_not:
					case Lexem_category::Plus:
					case Lexem_category::Minus:
					case Lexem_category::Sizeof:
					case Lexem_category::Kw_int:
					case Lexem_category::Kw_float:
					case Lexem_category::Kw_string:
					case Lexem_category::Kw_true:
					case Lexem_category::Kw_false:
					case Lexem_category::Open_round_bracket:
					case Lexem_category::Ident:
					case Lexem_category::Module_name_prefix:
					case Lexem_category::Kw_array:
                        sc_->back();
                        components = proc_expr_list();
                        state = State::LFirst_component;
                        break;
					default:
						printf(expected_bracket_component_fmt, 
								sc_->lexem_begin_line_number());
						et_.ec->increment_number_of_errors();
						return result;	
				}
				break;
			case State::GOpen_square:
				switch(cat){
					case Lexem_category::Logical_not:
					case Lexem_category::Bitwise_not:
					case Lexem_category::Plus:
					case Lexem_category::Minus:
					case Lexem_category::Sizeof:
					case Lexem_category::Kw_int:
					case Lexem_category::Kw_float:
					case Lexem_category::Kw_string:
					case Lexem_category::Kw_true:
					case Lexem_category::Kw_false:
					case Lexem_category::Open_round_bracket:
					case Lexem_category::Ident:
					case Lexem_category::Module_name_prefix:
					case Lexem_category::Kw_array:
                        sc_->back();
                        components = proc_expr_list();
                        state = State::MSecond_component;
                        break;
					default:
						printf(expected_component_fmt, 
								sc_->lexem_begin_line_number());
						et_.ec->increment_number_of_errors();
						return result;	
				}
				break;
			case State::HClose:
				switch(cat){
					case Lexem_category::Open_round_bracket:
						state = State::FOpen_round;
						break;
					case Lexem_category::Open_square_bracket:
						state = State::GOpen_square;
						break;
					case Lexem_category::At_sign:
						state = State::HClose;
						break;
					case Lexem_category::Dot:
						state = State::IDot;
						break;
					default:
						sc_->back();                    
						return result;	
				}
				break;
			case State::IDot:
				if(cat != Lexem_category::Ident){
                    printf(expected_ident_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::HClose;
                break;
			case State::LFirst_component:
				if(cat != Lexem_category::Closed_round_bracket){
                    printf(expected_closed_round_bracket_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::HClose;
                break;
			case State::MSecond_component:
				if(cat != Lexem_category::Closed_square_bracket){
                    printf(expected_closed_square_bracket_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::HClose;
                break;
		}
	}
	return result;
}





std::shared_ptr<ast::Loop_stmt> Parser::Impl::proc_loop_stmt()
{
	std::shared_ptr<ast::Loop_stmt> result;
	enum class State{
		Start, Prefix, Loop, Id, Suffix
	};
	State state = State::Start;
	size_t loop_label_ = 0;
	for(;;){
		Main_lexem_info li = sc_->current_lexem();
		Lexem_category cat = get_lexem_category(li);
		switch(state){
			case State::Start:
				switch(cat){
					case Lexem_category::Label_prefix:
                        state = State::Prefix;
                        break;
					case Lexem_category::Kw_spider:
					case Lexem_category::Kw_repeat:
					case Lexem_category::Kw_while:
					case Lexem_category::Kw_for:
						sc_->back();
						state = State::Loop;
						break;
					default:
						printf(expected_prefix_loop_fmt, 
								sc_->lexem_begin_line_number());
						et_.ec->increment_number_of_errors();
						return result;	
				}
				break;
			case State::Prefix:
				if(cat != Lexem_category::Ident){
                    printf(expected_ident_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::Id;
                break;
			case State::Loop:
				sc_->back();
                return result;
			case State::Id:
				if(cat != Lexem_category::Label_suffix){
                    printf(expected_label_suffix_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::Suffix;
                break;
			case State::Suffix:
				switch(cat){
					case Lexem_category::Kw_spider:
					case Lexem_category::Kw_repeat:
					case Lexem_category::Kw_while:
					case Lexem_category::Kw_for:
						sc_->back();
						state = State::Loop;
						break;
					default:
						printf(expected_loop_fmt, 
								sc_->lexem_begin_line_number());
						et_.ec->increment_number_of_errors();
						return result;	
				}
				break;
		}
	}
	return result;
}

std::shared_ptr<ast::Loop_stmt> Parser::Impl::proc_loop_without_label()
{
    std::shared_ptr<ast::Loop_stmt> result;
    Main_lexem_info                li     = sc_->current_lexem();
    Lexem_category                 cat    = get_lexem_category(li);
    sc_->back();
    switch(cat){
        case Lexem_category::Kw_while:
            result = proc_while_stmt();
            break;
        case Lexem_category::Kw_repeat:
            result = proc_as_lomg_as_stmt();
            break;
		case Lexem_category::Kw_spider:
            result = proc_spider_stmt();
            break;
        case Lexem_category::Kw_for:
            result = proc_for_stmt();
            break;	
        default:
            printf(expected_loop_without_label_fmt, sc_->lexem_begin_line_number());
            et_.ec->increment_number_of_errors();
    }
    return result;
}

std::shared_ptr<ast::While_stmt> Parser::Impl::proc_while_stmt()
{
	std::shared_ptr<ast::While_stmt> result;
	enum class State{
		Start, Second, Expression, Open_bracket, Block, Closed_bracket
	};
	State state = State::Start;
	std::shared_ptr<Expr> condition;
    ast::Block_body            body;
	for(;;){
		Main_lexem_info li = sc_->current_lexem();
		Lexem_category cat = get_lexem_category(li);
		switch(state){
			case State::Start:
				if(cat != Lexem_category::Kw_while){
                    printf(expected_kw_while_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::Second;
                break;
			case State::Second:
				switch(cat){
					case Lexem_category::Logical_not:
					case Lexem_category::Bitwise_not:
					case Lexem_category::Plus:
					case Lexem_category::Minus:
					case Lexem_category::Sizeof:
					case Lexem_category::Kw_int:
					case Lexem_category::Kw_float:
					case Lexem_category::Kw_string:
					case Lexem_category::Kw_true:
					case Lexem_category::Kw_false:
					case Lexem_category::Open_round_bracket:
					case Lexem_category::Ident:
					case Lexem_category::Module_name_prefix:
					case Lexem_category::Kw_array:
                        sc_->back();
                        condition = proc_expr();
                        state = State::Expression;
                        break;
					default:
						printf(expected_expression_fmt, 
								sc_->lexem_begin_line_number());
						et_.ec->increment_number_of_errors();
						return result;	
				}
				break;
			case State::Expression:
				if(cat != Lexem_category::Open_curly_bracket){
                    printf(expected_open_curly_bracket_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::Open_bracket;
                break;
			case State::Open_bracket:
				switch(cat){
					case Lexem_category::Kw_var:
                    case Lexem_category::Kw_type:
                    case Lexem_category::Kw_function:
                    case Lexem_category::Kw_const:
					case Lexem_category::Semicolon:
					case Lexem_category::Label_prefix:
                    case Lexem_category::Kw_for:
                    case Lexem_category::Kw_while:
                    case Lexem_category::Kw_repeat:
					case Lexem_category::Module_name_prefix:
                    case Lexem_category::Ident:
                    case Lexem_category::Kw_exit:
                    case Lexem_category::Kw_read:
					case Lexem_category::Kw_print:
                    case Lexem_category::Kw_if:
                    case Lexem_category::Kw_match:
						sc_->back();
						body = proc_block_body();
						state = State::Block;
						break;
					default:
                        printf(expected_block_body_fmt,
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
				}
				break;
			case State::Block:
				if(cat != Lexem_category::Closed_curly_bracket){
                    printf(expected_closed_curly_bracket_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::Closed_bracket;
                break;
			case State::Closed_bracket:
				sc_->back();
                return result;
		}
	}
	return result;
}

std::shared_ptr<ast::While_stmt> Parser::Impl::proc_while_stmt()
{
	std::shared_ptr<ast::While_stmt> result;
	enum class State{
		Start, Second, Expression, Open_bracket, Block, Closed_bracket
	};
	State state = State::Start;
	std::shared_ptr<Expr> condition;
    ast::Block_body            body;
	for(;;){
		Main_lexem_info li = sc_->current_lexem();
		Lexem_category cat = get_lexem_category(li);
		switch(state){
			case State::Start:
				if(cat != Lexem_category::Kw_while){
                    printf(expected_kw_while_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::Second;
                break;
			case State::Second:
				switch(cat){
					case Lexem_category::Logical_not:
					case Lexem_category::Bitwise_not:
					case Lexem_category::Plus:
					case Lexem_category::Minus:
					case Lexem_category::Sizeof:
					case Lexem_category::Kw_int:
					case Lexem_category::Kw_float:
					case Lexem_category::Kw_string:
					case Lexem_category::Kw_true:
					case Lexem_category::Kw_false:
					case Lexem_category::Open_round_bracket:
					case Lexem_category::Ident:
					case Lexem_category::Module_name_prefix:
					case Lexem_category::Kw_array:
                        sc_->back();
                        condition = proc_expr();
                        state = State::Expression;
                        break;
					default:
						printf(expected_expression_fmt, 
								sc_->lexem_begin_line_number());
						et_.ec->increment_number_of_errors();
						return result;	
				}
				break;
			case State::Expression:
				if(cat != Lexem_category::Open_curly_bracket){
                    printf(expected_open_curly_bracket_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::Open_bracket;
                break;
			case State::Open_bracket:
				switch(cat){
					case Lexem_category::Kw_var:
                    case Lexem_category::Kw_type:
                    case Lexem_category::Kw_function:
                    case Lexem_category::Kw_const:
					case Lexem_category::Semicolon:
					case Lexem_category::Label_prefix:
                    case Lexem_category::Kw_for:
                    case Lexem_category::Kw_while:
                    case Lexem_category::Kw_repeat:
					case Lexem_category::Module_name_prefix:
                    case Lexem_category::Ident:
                    case Lexem_category::Kw_exit:
                    case Lexem_category::Kw_read:
					case Lexem_category::Kw_print:
                    case Lexem_category::Kw_if:
                    case Lexem_category::Kw_match:
						sc_->back();
						body = proc_block_body();
						state = State::Block;
						break;
					default:
                        printf(expected_block_body_fmt,
                               sc_->lexem_begin_line_number());
                        et_.ec->increment_number_of_errors();
                        return result;
				}
				break;
			case State::Block:
				if(cat != Lexem_category::Closed_curly_bracket){
                    printf(expected_closed_curly_bracket_fmt,  sc_->lexem_begin_line_number());
                    et_.ec->increment_number_of_errors();
                    return result;
                }
                state = State::Closed_bracket;
                break;
			case State::Closed_bracket:
				sc_->back();
                return result;
		}
	}
	return result;
}
