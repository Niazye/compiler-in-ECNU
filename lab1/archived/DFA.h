#ifndef DFA_H
#define DFA_H

#include <list>
#include <memory>
#include <vector>
#include <string>
#include <regex>
#include <iostream>
#include <cassert>
#include <queue>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <set>

enum RE_operator    // 操作符类型
{
    UNION,
    CONCAT,
    KLEENE_STAR,
    PLUS,
    LEFT_BRACKET,
    RIGHT_BRACKET,
    TERMINAL,
};
enum RE_char_type   // 操作符 or 字符
{
    OPTR,
    RECHAR
};

struct nfa_state;
struct dfa_state;
class RE;
class RE_tree;
class NFA;
class DFA;

typedef std::pair<char, std::weak_ptr<nfa_state>> nfa_transfer_t;
typedef std::pair<char, std::weak_ptr<dfa_state>> dfa_transfer_t;
typedef std::set<std::shared_ptr<nfa_state>> nfa_state_set_t;

void trim_inplace(std::string& str);
bool pattern_cmp(const std::pair<std::string, std::string>& a, const std::pair<std::string, std::string>& b);
bool drop_global_bracket(std::string& str, std::vector<bool>& op_pattern);

struct nfa_state
{
    bool is_final = false;
    std::multimap<char, std::weak_ptr<nfa_state>> transfers;
};
struct dfa_state
{
    bool is_final = false;
    std::map<char, std::weak_ptr<dfa_state>> transfers;
};

class RE_tree
{
    RE_operator op;
    char value = 0;
    std::unique_ptr<RE_tree> left;
    std::unique_ptr<RE_tree> right;
    friend class NFA;
    std::unordered_set<char> terminal_chars;
public:
    RE_tree(RE_operator oper, char val = 0, std::unique_ptr<RE_tree> l = nullptr, std::unique_ptr<RE_tree> r = nullptr);
    RE_tree(RE pattern_obj);
    ~RE_tree();
    operator bool() const;
};

class RE
{
    std::string pattern;
    std::vector<bool> op_pattern;
    std::vector<std::pair<std::string, std::string>> defination_patterns;
    std::unordered_set<char> terminal_chars;
    std::pair<std::string, std::vector<bool>> getPattern();
    friend class RE_tree;
public:
    RE(std::string pattern, std::vector<bool> op_pattern);
    RE(std::string pattern = "");
    ~RE();
    void add_pattern_line(std::string input_pattern);
    void merge_patterns();
    void parse_pattern();
    void print_pattern();
};

class NFA
{
    std::shared_ptr<nfa_state> start_state;
    std::shared_ptr<nfa_state> final_state;
    std::unordered_set<char> terminal_chars;
    std::vector<std::shared_ptr<nfa_state>> owned_states; // owns all states to keep weak_ptr targets alive
    static void swap(NFA& first, NFA& second);
    friend class DFA;
public:
    NFA(const RE_tree& re_tree);
    ~NFA();
    NFA(const NFA& other);
    NFA(const char terminal);
    NFA& operator=(const NFA& other);
    bool union_other(const NFA& other);
    bool concat_other(const NFA& other);
    bool kleene_star();
    bool plus();
};

class DFA
{
    std::shared_ptr<dfa_state> start_state;
    std::unordered_set<char> terminal_chars;
    std::vector<std::shared_ptr<dfa_state>> owned_states; // owns DFA states
public:
    ~DFA();
    DFA(const NFA& nfa);
    nfa_state_set_t move(const nfa_state_set_t& states, char input);
    nfa_state_set_t epsilon_closure(const nfa_state_set_t& states);
    bool all_match(const std::string& input, size_t start_pos = 0);
    size_t longest_match(const std::string& input, size_t start_pos = 0);
};

#endif