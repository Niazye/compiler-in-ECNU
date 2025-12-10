#ifndef DFA_H
#define DFA_H

#include <regex>
#include <string>
#include <vector>
#include <list>
#include <cassert>
#include <iostream>
#include <memory>
struct state;
typedef std::pair<char, std::shared_ptr<state>> transfer_t;

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
    std::unique_ptr<RE_tree> left;
    std::unique_ptr<RE_tree> right;
    friend class NFA;
public:
    RE_tree(RE_operator oper, char val = 0, std::unique_ptr<RE_tree> l = nullptr, std::unique_ptr<RE_tree> r = nullptr);
    RE_tree(RE pattern_obj);
    ~RE_tree();
    operator bool() const;
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

struct state
{
    bool is_final;
    std::list<transfer_t> transfer;
};

class NFA
{
    std::shared_ptr<state> start_state;
    std::shared_ptr<state> final_state;
public:
    NFA(const RE_tree& re_tree);
    ~NFA();
    NFA(const NFA& other);
    NFA(const char terminal);
    bool onion(const NFA& other);
    bool concat(const NFA& other);
    bool kleene_star();
    bool plus();

};

class DFA
{

};


#endif