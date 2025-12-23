#include <bits/stdc++.h>
#include <windows.h>
#include <psapi.h>
#include "DFA.h"
#include "keys_patterns.h"

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

    // auto constant_re = std::make_unique<RE>(CONSTANT_PATTERN);
    // auto identifier_re = std::make_unique<RE>(IDENTIFIER_PATTERN);
    // constant_re -> print_pattern();
    // identifier_re -> print_pattern();
    // return 0;
    auto t0 = std::chrono::high_resolution_clock::now();
    auto constant_re = RE(CONSTANT_PATTERN);
    auto t1 = std::chrono::high_resolution_clock::now();

    NFA constant_nfa = NFA(constant_re);
    auto t2 = std::chrono::high_resolution_clock::now();

    DFA constant_dfa = DFA(constant_nfa);
    auto t3 = std::chrono::high_resolution_clock::now();

    const auto re_us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
    const auto nfa_us = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    const auto dfa_us = std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2).count();
    std::cout << "Build timings (microseconds) - RE: " << re_us
              << ", NFA: " << nfa_us
              << ", DFA: " << dfa_us << std::endl;
    return 0;
    
}