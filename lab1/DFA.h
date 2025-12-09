#ifndef DFA_H
#define DFA_H

#include <regex>
#include <string>
#include <vector>
#include <list>
#include <cassert>
#include <iostream>

void trim_inplace(std::string& str);

enum RE_operator
{
    UNION,
    CONCAT,
    KLEENE_STAR,
    PLUS,
    LEFT_BRACKET,
    RIGHT_BRACKET,
    TERMINAL,
    UTERMINAL,
    ENUM_CNT
};

class RE;

class RE_tree
{
    RE_operator op;
    char value;
    RE_tree* left;
    RE_tree* right;
public:
    RE_tree(RE_operator oper, char val = 0, RE_tree* l = nullptr, RE_tree* r = nullptr);
    RE_tree(RE pattern_obj);
    ~RE_tree();
};

class RE
{
    std::string str_pattern;
    std::string pattern;
    std::vector<std::pair<std::string, std::string>> defination_patterns;
public:
    RE(std::string pattern = "");
    ~RE();
    std::string getPattern();
    void add_pattern_line(std::string input_pattern);
    void merge_patterns();
    void print_pattern();

};

class DFA
{

};


#endif