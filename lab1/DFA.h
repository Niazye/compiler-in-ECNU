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

enum RE_operator    // 操作符类型
{
    UNION,
    CONCAT,
    KLEENE_STAR,
    PLUS,
    LEFT_BRACKET,
    RIGHT_BRACKET,
    TERMINAL,
    UTERMINAL,
    UNKNOWN,
};
enum RE_char_type   // 操作符 or 字符
{
    OPTR,
    CHAR
};

class RE;

class RE_tree
{
    RE_operator op = UNKNOWN;
    char value = 0;
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
    std::vector<bool> op_pattern;
    std::vector<std::pair<std::string, std::string>> defination_patterns;
public:
    RE(std::string pattern = "", std::vector<bool> op_pattern = {});
    ~RE();
    std::pair<std::string, std::vector<bool>> getPattern();
    void add_pattern_line(std::string input_pattern);
    void merge_patterns();
    void parse_pattern();
    void print_pattern();

};

struct state
{
    bool is_final;
    std::list<transfer_t> transfers;
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
    bool union_other(const NFA& other);
    bool concat_other(const NFA& other);
    bool kleene_star();
    bool plus();
};

class DFA
{
    std::shared_ptr<state> start_state;
    std::shared_ptr<state> final_state;
public:
    DFA();
    ~DFA();
    DFA(const NFA& nfa);
    void minimize();
    bool match(const std::string& input);
    
};


#endif