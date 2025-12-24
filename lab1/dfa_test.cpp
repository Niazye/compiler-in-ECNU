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

    auto constant_dfa = DFA(NFA(RE(CONSTANT_PATTERN)));
    auto identifier_dfa = DFA(NFA(RE(IDENTIFIER_PATTERN)));

    std::ofstream f_const("dfa_constant.txt");
    f_const << constant_dfa.export2str();
    f_const.close();
    f_const.open("dfa_identifier.txt");
    f_const << identifier_dfa.export2str();
    f_const.close();
    // while (1)
    // {
    //     std::string line;
    //     std::getline(std::cin, line);
    //     if (constant_dfa.all_match(line))
    //         std::cout << line << " is a constant." << std::endl;
    //     else
    //         std::cout << line << " is NOT a constant." << std::endl;
    // }
    // std::string constant_dfa_str;
    // std::string identifier_dfa_str;
    // std::ifstream f("dfa_constant.txt");
    // std::stringstream buffer;
    // buffer << f.rdbuf();
    // constant_dfa_str = buffer.str();
    // f.close();
    // f.open("dfa_identifier.txt");
    // buffer.str(std::string());
    // buffer << f.rdbuf();
    // identifier_dfa_str = buffer.str();

    // auto constant_dfa1 = DFA(constant_dfa_str);
    // auto constant_dfa2 = DFA(NFA(RE(CONSTANT_PATTERN)));

    // std::string line;
    // while(std::cin >> line)
    // {
    //     if(constant_dfa1.all_match(line))
    //         std::cout << line << " is a constant (from imported DFA)." << std::endl;
    //     else
    //         std::cout << line << " is NOT a constant (from imported DFA)." << std::endl;

    //     if(constant_dfa2.all_match(line))
    //         std::cout << line << " is a constant (from constructed DFA)." << std::endl;
    //     else
    //         std::cout << line << " is NOT a constant (from constructed DFA)." << std::endl;

    // }
    return 0;
    
}