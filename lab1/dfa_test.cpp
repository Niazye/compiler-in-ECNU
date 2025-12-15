#include <bits/stdc++.h>
#include <windows.h>
#include <psapi.h>
#include "DFA.h"

// Print current process memory usage (working set and private bytes).
static void print_current_memory_usage()
{
    PROCESS_MEMORY_COUNTERS_EX pmc{};
    if (GetProcessMemoryInfo(GetCurrentProcess(),
                             reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc),
                             sizeof(pmc)))
    {
        std::cout << "[mem] WorkingSet(KB): " << pmc.WorkingSetSize / 1024
                  << "  PrivateUsage(KB): " << pmc.PrivateUsage / 1024 << std::endl;
    }
    else
    {
        std::cerr << "[mem] GetProcessMemoryInfo failed" << std::endl;
    }
}

int main()
{

    /*
     * 该解析器所采用的正则表达式（正则定义）部分语法如下
     * d  ->  r
     * 之间必须使用  ->   连接，默认无视空字符
     * 支持以下基本操作符：
     * 1. 并（union）：   |
     * 2. 连接（concatenation）：   .     允许省略
     * 3. 克林闭包（Kleene star）：   *
     * 4. 正闭包（plus）：   +
     * 5. 分组（grouping）：   (   )
     * 6. 转义字符（escape）：   \   （除常规的 \n 换行 \t 制表符外，针对正则表达式额外支持 \* \+ \. 等表示字符本身）
     *                               （\\ 则表示反斜杠本身，若反斜杠后跟随无效字符，则不认作转义，正常识别）
     *                               （\0 表示空串 ε，0 则是字符 0 本身）
    */

    std::string FULL_CONSTANT_PATTERN =
    R"delimiter(constant    -> inte | frac

    inte        -> (bin_inte | oct_inte | dec_inte | hex_inte) opt_inte_suf
    bin_inte    -> 0(b|B) bin_digit+
    oct_inte    -> 0 oct_digit+
    dec_inte    -> (dec_digit_no_zero) dec_digit* 
    hex_inte    -> 0(x|X) hex_digit+
    opt_inte_suf     -> \0 | unsigned_suf | long_suf | (unsigned_suf long_suf) | (long_suf unsigned_suf)
    
    frac        -> (dec_frac | hex_frac) opt_frac_suf
    dec_frac    -> (dec_point dec_opt_exp) | dec_digits dec_exp
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
    hex_digit   -> 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | a | b | c | d | e | f | A | B | C | D | E | F)delimiter";

    auto constant_re = std::make_unique<RE>();
    std::string line;
    std::istringstream pattern_stream(FULL_CONSTANT_PATTERN);
    while (std::getline(pattern_stream, line))
    {
        if (line.empty()) continue;
        constant_re -> add_pattern_line(line);
    }
    constant_re -> merge_patterns();
    constant_re -> parse_pattern();
    //constant_re.print_pattern();

    std::cout << "after RE construction" << std::endl;
    print_current_memory_usage();

    auto constant_re_tree = std::make_unique<RE_tree>(*constant_re);

    std::cout << "after RE_tree construction" << std::endl;
    print_current_memory_usage();

    auto constant_nfa = std::make_unique<NFA>(*constant_re_tree);

    std::cout << "after NFA construction" << std::endl;
    print_current_memory_usage();

    DFA constant_dfa(*constant_nfa);

    std::cout << "after DFA construction" << std::endl;
    print_current_memory_usage();

    constant_re.reset();
    constant_re_tree.reset();
    constant_nfa.reset();
    std::cout << "after RE, RE_tree, NFA destruction" << std::endl;
    print_current_memory_usage();
    while(std::cin >> line)
    {
        if (constant_dfa.match(line))
        {
            std::cout << "Matched!" << std::endl;
        }
        else
        {
            std::cout << "Not matched!" << std::endl;
        }
    }
    
    return 0;
    
}