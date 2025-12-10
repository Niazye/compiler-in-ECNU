#include <bits/stdc++.h>
#include "DFA.h"

int main()
{
    RE constant_re;
    std::string FULL_CONSTANT_PATTERN =
    R"delimiter(constant    -> inte | frac

    inte        -> (bin_inte | oct_inte | dec_inte | hex_inte) opt_inte_suf
    bin_inte    -> 0(b|B) bin_digit+
    oct_inte    -> 0 oct_digit+
    dec_inte    -> (dec_digit_no_zero) dec_digit* 
    hex_inte    -> 0(x|X) hex_digit+
    opt_inte_suf     -> @ | unsigned_suf | long_suf | (unsigned_suf long_suf) | (long_suf unsigned_suf)
    
    frac        -> (dec_frac | hex_frac) opt_frac_suf
    dec_frac    -> dec_base dec_opt_exp
    hex_frac    -> 0(x|X) hex_base hex_opt_exp
    dec_base    -> (dec_digit+ \. dec_digit*) | (dec_digit* \. dec_digit+) | (dec_digit+)
    dec_opt_exp -> @ | ((e | E) opt_sign dec_inte)
    hex_base    -> (hex_digit+ \. hex_digit*) | (hex_digit* \. hex_digit+) | (hex_digit+)
    hex_opt_exp -> @ | ((p | P) opt_sign hex_inte)
    opt_frac_suf    -> @ | float_suf | long_double_suf
    
    unsigned_suf        -> u | U
    long_suf    -> l | L | ll | LL
    float_suf   -> f | F
    long_double_suf -> l | L
    opt_sign    -> @ | \+ | -
    bin_digit   -> 0 | 1
    oct_digit   -> 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7
    dec_digit   -> 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9
    dec_digit_no_zero   -> 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9
    hex_digit   -> 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | a | b | c | d | e | f | A | B | C | D | E | F)delimiter";
    std::string line;
    std::istringstream pattern_stream(FULL_CONSTANT_PATTERN);
    while (std::getline(pattern_stream, line))
    {
        if (line.empty()) continue;
        constant_re.add_pattern_line(line);
    }
    constant_re.merge_patterns();
    constant_re.print_pattern();

    RE_tree constant_tree(constant_re);

    return 0;
    
}