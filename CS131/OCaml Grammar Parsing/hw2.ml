type ('nonterminal, 'terminal) symbol =
  | N of 'nonterminal
  | T of 'terminal

type ('nonterminal, 'terminal) parse_tree =
  | Node of 'nonterminal * ('nonterminal, 'terminal) parse_tree list
  | Leaf of 'terminal

(*1*)
let rec alt_list rules nonterminal = match rules with
    | [] -> []
    | head::tail -> if (fst head) = nonterminal then (snd head)::alt_list tail nonterminal
              else alt_list tail nonterminal

let convert_grammar gram1 = 
    let start_symbol = fst gram1 in
    let rules = snd gram1 in
    let rules_to_function = alt_list rules in
    (start_symbol, rules_to_function)

(*2*)
let rec parse_list tree = match tree with 
  | [] -> []
  | head::tail -> match head with 
    | Leaf term -> term::(parse_list tail)
    | Node (nonterm, children) -> (parse_list children) @ (parse_list tail)

let parse_tree_leaves tree = parse_list [tree]

(*3*)
let rec try_rules grammar_func rule_list accept frag = match rule_list with 
  | [] -> None
  | current_rule::remaining_rules -> 
    match apply_rule grammar_func current_rule accept frag with
      | None -> try_rules grammar_func remaining_rules accept frag
      | success_val -> success_val

and apply_rule grammar_func rule accept frag = match rule with
  | [] -> accept frag
  | _ -> match frag with
    | [] -> None
    | current_symbol::rest_of_fragment -> match rule with
      | [] -> None
      | (T terminal)::rule_tail -> 
        if current_symbol = terminal then 
        (apply_rule grammar_func rule_tail accept rest_of_fragment) 
        else None
      | (N non_terminal)::rule_tail -> 
        (try_rules grammar_func (grammar_func non_terminal) (apply_rule grammar_func rule_tail accept) frag)

let make_matcher gram accept frag = 
    (try_rules (snd gram) ((snd gram)(fst gram)) accept frag)

(*4*)
let accept_empty_wrapper = function
  | [] -> Some []
  | _ -> None

let rec try_rules grammar_func rule_list accept frag = match rule_list with 
  | [] -> None
  | current_rule::remaining_rules -> match apply_rule grammar_func current_rule accept frag with
    | None -> try_rules grammar_func remaining_rules accept frag
    | Some success_match -> Some (current_rule::success_match)

and apply_rule grammar_func rule accept frag = match rule with
  | [] -> accept frag
  | _ -> match frag with
    | [] -> None
    | current_symbol::rest_of_frag -> match rule with
      | [] -> None
      | (T terminal)::rule_tail -> 
        if current_symbol = terminal then 
        apply_rule grammar_func rule_tail accept rest_of_frag 
        else None
      | (N non_terminal)::rule_tail -> 
        try_rules grammar_func (grammar_func non_terminal) 
        (apply_rule grammar_func rule_tail accept) frag

let traversed_rules gram accept frag = 
  try_rules (snd gram) ((snd gram)(fst gram)) accept_empty_wrapper frag
			
let rec build_tree start list = match start with 
        | [] -> (list, []) 
        | current_symbol::remaining_symbols -> 
          match (process_symbol current_symbol list) with 
          | (remaining_list, current_tree) -> 
            match build_tree remaining_symbols remaining_list with 
            | (success_list, subtrees) -> (success_list, current_tree::subtrees) 

and process_symbol start list = match start with 
  | (T terminal) -> (match list with 
    | [] -> ([], Leaf terminal) 
    | head::tail -> (head::tail, Leaf terminal))
  | (N non_terminal) -> match list with 
    | [] -> ([], Node (non_terminal, [])) 
    | head::tail -> match build_tree head tail with 
        | (remaining_list, subtrees) -> (remaining_list, Node (non_terminal, subtrees))
							
let get_traversed_grammar gram frag = traversed_rules gram accept_empty_wrapper frag

let make_parser gram frag = match get_traversed_grammar gram frag with 
  | None -> None 
  | Some [] -> None 
  | Some parsing -> match build_tree [N (fst gram)] parsing with 
    | (_, tree_list) -> match tree_list with 
      | [] -> None 
      | tree::_ -> Some tree
