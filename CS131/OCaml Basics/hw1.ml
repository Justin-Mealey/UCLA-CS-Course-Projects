(*1*)
(*Helper function: return true if x is in list l*)
let rec elem_in_list x l =
    match l with
        | [] -> false
        | y::ys -> y = x || elem_in_list x ys

let rec subset a b =
    match a with
        | [] -> true
        | x::xs -> elem_in_list x b && subset xs b

(*2*)
let equal_sets a b = subset a b && subset b a

(*3*)
let rec set_union a b = 
    match a with
        | [] -> b
        | x::xs -> x::set_union xs b

(*4*)
let rec set_all_union a = 
    match a with
        | [] -> []
        | [x] -> x
        | x::y::rest -> set_all_union ((set_union x y) :: rest)

(*5*)
(*
    self_member is impossible to write in OCaml because of type restrictions. 
    We are using 'a lists to represent sets. For a set to be contained within itself,
    we need to put 'a list inside 'a list, which is impossible in OCaml (the outer set/list
    would need to be type 'a list list). Therefore, it is impossible to check if a set is a 
    member of itself in Ocaml.
*)

(*6*)
let rec computed_fixed_point eq f x =
    if eq (f x) x then 
        x
    else 
        computed_fixed_point eq f (f x)

(*7*)
(*Helper function: get f(f(f(x))) if p = 3*)
let rec arg2 f p x = 
    match p with
        | 0 -> x 
        | _ -> f(arg2 f (p-1) x)

let rec computed_periodic_point eq f p x = 
    if eq (arg2 f p x) x then 
        x 
    else 
        computed_periodic_point eq f p (f x)

(*8*)
let rec whileseq s p x = 
    if p x then 
        x::(whileseq s p (s x))
    else 
        []

(*9*)
(*Necessary symbols*)
type ('nonterminal, 'terminal) symbol =
	| N of 'nonterminal
	| T of 'terminal

(*Helper: see if rule is terminal or not using terminals*)
let is_terminal rule terminals = 
    match rule with
        | T _ -> true
        | N name -> if List.mem name terminals then 
                        true
  	                else 
                        false

(*Helper: see if all rules are terminal or not*)
let rec all_terminal rules terminals = 
    match rules with 
        | [] -> true
        | head::tail -> (is_terminal head terminals) && (all_terminal tail terminals)

(*Helper: given (typ, [rules]) rulepair and terminals, output terminal rules*)
let rec generate_list rulepair terminals = 
    match rulepair with
        | [] -> terminals
        | (typ, rules)::tail -> if (all_terminal rules terminals) then 
                                    (generate_list tail (typ::terminals))
                                 else 
                                    (generate_list tail terminals)

(*Helper: bind the applicable terminals to the rules in a tuple*)
let bind_lists (r, t) = (r, (generate_list r t))

(*Helper: given (typ, rules) r, terminals t, and current list of terminal grammars res, 
output grammar without blind alley rules*)
let rec find_terminables r t res = 
    match r with
        | [] -> res
        | (typ, rules)::tail -> if (all_terminal rules t) then 
                                    (find_terminables tail t (res@[typ, rules]))
                                 else 
                                    (find_terminables tail t res)

(*Helper: equality for tuples of 2 sets based on snd*)
let equal_tuples (a1, b1) (a2, b2) = equal_sets b1 b2

let filter_blind_alleys g = 
    match g with 
	| a, b -> ((a), (find_terminables b 
                    (snd(computed_fixed_point 
                    equal_tuples 
                    bind_lists (b, []))) []))
