#ifndef KEYS_PATTERNS_H
#define KEYS_PATTERNS_H

#include <string>
    std::string CONSTANT_PATTERN =
    R"delimiter(
    constant    -> inte | frac

    inte        -> (bin_inte | oct_inte | dec_inte | hex_inte) opt_inte_suf
    bin_inte    -> 0(b|B) bin_digit+
    oct_inte    -> 0 oct_digit+
    dec_inte    -> (dec_digit_no_zero dec_digit*) | 0
    hex_inte    -> 0(x|X) hex_digit+
    opt_inte_suf     -> \0 | unsigned_suf | long_suf | (unsigned_suf long_suf) | (long_suf unsigned_suf)
    
    frac        -> (dec_frac | hex_frac) opt_frac_suf
    dec_frac    -> (dec_point dec_opt_exp) | (dec_digit+ dec_exp)
    hex_frac    -> 0(x|X) hex_base hex_opt_exp
    dec_point    -> (dec_digit+ \. dec_digit*) | (dec_digit* \. dec_digit+)
    dec_opt_exp -> \0 | ((e | E) opt_sign dec_digit)
    dec_exp     -> (e | E) opt_sign dec_digit+
    hex_base    -> (hex_digit+ \. hex_digit*) | (hex_digit* \. hex_digit+) | (hex_digit+)
    hex_opt_exp -> \0 | ((p | P) opt_sign hex_digit)
    opt_frac_suf    -> \0 | float_suf | long_double_suf
    
    unsigned_suf        -> u | U
    long_suf    -> l | L | ll | LL
    float_suf   -> f | F
    long_double_suf -> l | L
    opt_sign    -> \0 | \+ | -
    bin_digit   -> 0 | 1
    oct_digit   -> 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7
    dec_digit   -> 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9
    dec_digit_no_zero   -> 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9
    hex_digit   -> 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | a | b | c | d | e | f | A | B | C | D | E | F
    )delimiter";

    std::string IDENTIFIER_PATTERN =
    R"delimiter(
    identifier    -> (letter | _ ) (letter | digit | _ )*
    letter      -> lowercase | uppercase
    lowercase -> a | b | c | d | e | f | g | h | i | j | k | l | m | n | o | p | q | r | s | t | u | v | w | x | y | z
    uppercase -> A | B | C | D | E | F | G | H | I | J | K | L | M | N | O | P | Q | R | S | T | U | V | W | X | Y | Z
    digit       -> 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9
    )delimiter";

    std::string SIMPLE_CONSTANT_PATTERN =
    R"delimiter(const -> (0|1|2|3|4|5|6|7|8|9)+)delimiter";

    std::string EXPAND_CONSTANT_PATTERN =
    R"delimiter(const -> (((0.(b|B).(0|1)+)|(0.(0|1|2|3|4|5|6|7)+)|(((1|2|3|4|5|6|7|8|9).(0|1|2|3|4|5|6|7|8|9)*)|0)|(0.(x|X).(0|1|2|3|4|5|6|7|8|9|a|b|c|d|e|f|A|B|C|D|E|F)+)).(\0|(u|U)|(l|L|l.l|L.L)|((u|U).(l|L|l.l|L.L))|((l|L|l.l|L.L).(u|U))))|(((((((0|1|2|3|4|5|6|7|8|9)+.\..(0|1|2|3|4|5|6|7|8|9)*)|((0|1|2|3|4|5|6|7|8|9)*.\..(0|1|2|3|4|5|6|7|8|9)+)).(\0|((e|E).(\0|\+|-).(0|1|2|3|4|5|6|7|8|9))))|((0|1|2|3|4|5|6|7|8|9)+.((e|E).(\0|\+|-).(0|1|2|3|4|5|6|7|8|9)+)))|(0.(x|X).(((0|1|2|3|4|5|6|7|8|9|a|b|c|d|e|f|A|B|C|D|E|F)+.\..(0|1|2|3|4|5|6|7|8|9|a|b|c|d|e|f|A|B|C|D|E|F)*)|((0|1|2|3|4|5|6|7|8|9|a|b|c|d|e|f|A|B|C|D|E|F)*.\..(0|1|2|3|4|5|6|7|8|9|a|b|c|d|e|f|A|B|C|D|E|F)+)|((0|1|2|3|4|5|6|7|8|9|a|b|c|d|e|f|A|B|C|D|E|F)+)).(\0|((p|P).(\0|\+|-).(0|1|2|3|4|5|6|7|8|9|a|b|c|d|e|f|A|B|C|D|E|F))))).(\0|(f|F)|(l|L))))delimiter";
    std::string EXPAND_IDENTIFIER_PATTERN =
    R"delimiter(id -> (((a|b|c|d|e|f|g|h|i|j|k|l|m|n|o|p|q|r|s|t|u|v|w|x|y|z)|(A|B|C|D|E|F|G|H|I|J|K|L|M|N|O|P|Q|R|S|T|U|V|W|X|Y|Z))|_).(((a|b|c|d|e|f|g|h|i|j|k|l|m|n|o|p|q|r|s|t|u|v|w|x|y|z)|(A|B|C|D|E|F|G|H|I|J|K|L|M|N|O|P|Q|R|S|T|U|V|W|X|Y|Z))|(0|1|2|3|4|5|6|7|8|9)|_)*)delimiter";
#endif // KEYS_PATTERNS_H