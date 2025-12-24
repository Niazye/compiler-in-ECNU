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

#ifndef DFA_ONLY
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
typedef std::set<std::shared_ptr<nfa_state>> nfa_state_set_t;

void trim_inplace(std::string& str);

struct nfa_state
{
    bool is_final = false;
    std::multimap<char, std::weak_ptr<nfa_state>> transfers;
};

class RE
{
    std::string pattern;
    std::vector<bool> op_pattern;
    std::unordered_set<char> terminal_chars;
    std::string raw_pattern;
    friend class RE_tree;
    static std::pair<std::string, std::string> split_pattern_line(std::string &input_pattern);
    static bool pattern_cmp(const std::pair<std::string, std::string>& a, const std::pair<std::string, std::string>& b);
    void merge_patterns(std::vector<std::pair<std::string, std::string>> &defination_patterns);
    void parse_pattern();
public:
    std::pair<std::string, std::vector<bool>> getPattern();
    RE(std::string &pattern);
    RE(std::string &pattern, std::vector<bool> &op_pattern);
    ~RE();
    RE postfix_form() const;
};

class NFA
{
    std::shared_ptr<nfa_state> start_state;
    std::shared_ptr<nfa_state> final_state;
    std::unordered_set<char> terminal_chars;
    std::vector<std::shared_ptr<nfa_state>> owned_states; // owns all states to keep weak_ptr targets alive
    friend class DFA;
public:
    NFA(const RE& re);
    ~NFA();
    NFA(const char terminal);
    NFA(const NFA& other);
    NFA& operator=(const NFA& other);
    bool union_other(const NFA& other);
    bool concat_other(const NFA& other);
    bool kleene_star();
    bool plus();
};
#endif

struct dfa_state
{
    bool is_final = false;
    std::map<char, std::weak_ptr<dfa_state>> transfers;
};

class DFA
{
    std::shared_ptr<dfa_state> start_state;
    std::unordered_set<char> terminal_chars;
    std::vector<std::shared_ptr<dfa_state>> owned_states; // owns DFA states
#ifndef DFA_ONLY
    nfa_state_set_t move(const nfa_state_set_t& states, char input);
    nfa_state_set_t epsilon_closure(const nfa_state_set_t& states);
    void minimize();
#endif

public:
    ~DFA();
    DFA(const std::string &import_str);
    bool all_match(const std::string& input, size_t start_pos = 0);
    size_t longest_match(const std::string& input, size_t start_pos = 0);
    std::string export2str();
    
#ifndef DFA_ONLY
    DFA(const NFA& nfa);
#endif
};

#endif