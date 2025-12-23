#include "DFA.h"

void trim_inplace(std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) {
        str.clear();
        return;
    }
    size_t end = str.find_last_not_of(" \t\n\r");
    str = str.substr(start, end - start + 1);
}
bool pattern_cmp(const std::pair<std::string, std::string>& a, const std::pair<std::string, std::string>& b)
{
    return a.first.length() > b.first.length();
}
bool drop_global_bracket(std::string& str, std::vector<bool>& op_pattern)
{
    if (str.front() == LEFT_BRACKET && str.back() == RIGHT_BRACKET && op_pattern.front() == OPTR && op_pattern.back() == OPTR)
    {
        int bracket_count = 0;
        for (size_t i = 0; i < str.length(); i++)
        {
            if (str[i] == LEFT_BRACKET && op_pattern[i] == OPTR)
                bracket_count++;
            else if (str[i] == RIGHT_BRACKET && op_pattern[i] == OPTR)
                bracket_count--;
            if (bracket_count == 0 && i != str.length() - 1)
                return 0;
        }
        str = str.substr(1, str.length() - 2);
        op_pattern.erase(op_pattern.begin());
        op_pattern.pop_back();
        return 1;
    }
    return 0;
}
RE::RE(std::string pattern, std::vector<bool> op_pattern)
{
    this -> pattern = pattern;
    this -> op_pattern = op_pattern;
}
RE::RE(std::string pattern)
{
    std::istringstream pattern_stream(pattern);
    std::string line;
    while (std::getline(pattern_stream, line))
    {
        if (line.empty()) continue;
        add_pattern_line(line);
    }
    merge_patterns();
    parse_pattern();
}
RE::~RE() {}
std::pair<std::string, std::vector<bool>> RE::getPattern()
{
    return {pattern, op_pattern};
}
void RE::add_pattern_line(std::string input_pattern)
{
    std::string def_name, def_pattern;
    trim_inplace(input_pattern);
    if(input_pattern.empty())
        return;
    def_name = input_pattern.substr(0, input_pattern.find("->"));
    def_pattern = input_pattern.substr(input_pattern.find("->") + 2);
    trim_inplace(def_name);
    trim_inplace(def_pattern);
    defination_patterns.push_back({def_name, def_pattern});
}
void RE::merge_patterns()
{
    if (defination_patterns.empty() || defination_patterns.size() == 1)
        return;
    std::sort(defination_patterns.begin() + 1, defination_patterns.end(), pattern_cmp);
    for (size_t i = 1; i < defination_patterns.size(); i++)
    {
        for (size_t j = 0; j < defination_patterns.size(); j++)
        {
            if (j == i)
                continue;
            defination_patterns[j].second = std::regex_replace(
                defination_patterns[j].second,
                std::regex(defination_patterns[i].first),
                "(" + defination_patterns[i].second + ")");
        }
    }
    defination_patterns.resize(1);
}
void RE::parse_pattern()
{
    if (defination_patterns.empty())
        return;
    std::string str_pattern = defination_patterns[0].second;
    for (auto itr = str_pattern.begin(); itr != str_pattern.end(); itr++)
    {
        if (isspace(*itr))
            continue;
        char to_push;
        RE_char_type to_type;
        if (*itr == '\\')
        {
            switch (*(itr + 1))
            {
            case 'n':
                to_push = '\n';
                to_type = RECHAR;
                itr++;
                break;
            case 't':
                to_push = '\t';
                to_type = RECHAR;
                itr++;
                break;
            case 'r':
                to_push = '\r';
                to_type = RECHAR;
                itr++;
                break;
            case '+':
                to_push = '+';
                to_type = RECHAR;
                itr++;
                break;
            case '*':
                to_push = '*';
                to_type = RECHAR;
                itr++;
                break;
            case '|':
                to_push = '|';
                to_type = RECHAR;
                itr++;
                break;
            case '(':
                to_push = '(';
                to_type = RECHAR;
                itr++;
                break;
            case ')':
                to_push = ')';
                to_type = RECHAR;
                itr++;
                break;
            case '.':
                to_push = '.';
                to_type = RECHAR;
                itr++;
                break;
            case '0':
                to_push = '\0';
                to_type = RECHAR;
                itr++;
                break;
            case '\\':
                to_push = '\\';
                to_type = RECHAR;
                itr++;
                break;
            default:
                to_push = *itr;
                to_type = RECHAR;
                break;
            }
        }
        else
        {
            switch (*itr)
            {
            case '+':
                to_push = PLUS;
                to_type = OPTR;
                break;
            case '*':
                to_push = KLEENE_STAR;
                to_type = OPTR;
                break;
            case '|':
                to_push = UNION;
                to_type = OPTR;
                break;
            case '(':
                to_push = LEFT_BRACKET;
                to_type = OPTR;
                break;
            case ')':
                to_push = RIGHT_BRACKET;
                to_type = OPTR;
                break;
            case '.':
                to_push = CONCAT;
                to_type = OPTR;
                break;
            default:
                to_push = *itr;
                to_type = RECHAR;
                break;
            }
        }
        if(!pattern.empty())
        {
            if ((op_pattern.back() == RECHAR                                      && to_type == RECHAR                           ) ||
                (op_pattern.back() == RECHAR                                      && to_type == OPTR && to_push == LEFT_BRACKET) ||  
                (op_pattern.back() == OPTR && pattern.back() == RIGHT_BRACKET   && to_type == RECHAR                           ) ||
                (op_pattern.back() == OPTR && pattern.back() == RIGHT_BRACKET   && to_type == OPTR && to_push == LEFT_BRACKET) ||
                (op_pattern.back() == OPTR && pattern.back() == KLEENE_STAR     && to_type == RECHAR                           ) ||
                (op_pattern.back() == OPTR && pattern.back() == PLUS            && to_type == RECHAR                           ) ||
                (op_pattern.back() == OPTR && pattern.back() == KLEENE_STAR     && to_type == OPTR && to_push == LEFT_BRACKET) ||
                (op_pattern.back() == OPTR && pattern.back() == PLUS            && to_type == OPTR && to_push == LEFT_BRACKET))
            {
                op_pattern.push_back(OPTR);
                pattern.push_back(CONCAT);
            }
        }
        op_pattern.push_back(to_type);
        pattern.push_back(to_push);
        if (to_type == RECHAR && to_push != '\0')
            terminal_chars.insert(to_push);
    }
}
void RE::print_pattern()
{
    std::cout << defination_patterns[0].first << " -> " << defination_patterns[0].second << std::endl;
}

RE_tree::RE_tree(RE_operator oper, char val, std::unique_ptr<RE_tree> l, std::unique_ptr<RE_tree> r)
{
    op = oper;
    value = val;
    left = std::move(l);
    right = std::move(r);
}
RE_tree::RE_tree(RE pattern_obj)
{
    this -> terminal_chars = pattern_obj.terminal_chars;
    auto pattern = pattern_obj.getPattern().first;
    auto op_pattern = pattern_obj.getPattern().second;
    assert(pattern.length() != 0);
    while(drop_global_bracket(pattern, op_pattern));
    int bracket_count = 0;
    if (pattern.length() == 1 && op_pattern[0] == RECHAR)
    {
        op = TERMINAL;
        value = pattern[0];
        left = nullptr;
        right = nullptr;
        return;
    }
    for (size_t i = 0; i < pattern.length(); i++)
    {
        if (pattern[i] == LEFT_BRACKET && op_pattern[i] == OPTR)
            bracket_count++;
        else if (pattern[i] == RIGHT_BRACKET && op_pattern[i] == OPTR)
            bracket_count--;
        else if (bracket_count == 0)
        {
            if (pattern[i] == UNION && op_pattern[i] == OPTR)
            {
                op = UNION;
                left = std::make_unique<RE_tree>(RE(pattern.substr(0, i), std::vector<bool>(op_pattern.begin(), op_pattern.begin() + i)));
                right = std::make_unique<RE_tree>(RE(pattern.substr(i + 1), std::vector<bool>(op_pattern.begin() + i + 1, op_pattern.end())));
            }
            else if (pattern[i] == KLEENE_STAR && op_pattern[i] == OPTR && i + 1 == pattern.length())
            {
                op = KLEENE_STAR;
                left = std::make_unique<RE_tree>(RE(pattern.substr(0, i), std::vector<bool>(op_pattern.begin(), op_pattern.begin() + i)));
                right = nullptr;
            }
            else if (pattern[i] == PLUS && op_pattern[i] == OPTR && i + 1 == pattern.length())
            {
                op = PLUS;
                left = std::make_unique<RE_tree>(RE(pattern.substr(0, i), std::vector<bool>(op_pattern.begin(), op_pattern.begin() + i)));
                right = nullptr;
            }
            else if (pattern[i] == CONCAT && op_pattern[i] == OPTR)
            {
                op = CONCAT;
                left = std::make_unique<RE_tree>(RE(pattern.substr(0, i), std::vector<bool>(op_pattern.begin(), op_pattern.begin() + i)));
                right = std::make_unique<RE_tree>(RE(pattern.substr(i + 1), std::vector<bool>(op_pattern.begin() + i + 1, op_pattern.end())));
            }
            else
            {
                continue;
            }
            return;
        }
    }

}
RE_tree::operator bool() const 
{
    if(op == TERMINAL)
        return 1;
    else if(op == KLEENE_STAR || op == PLUS)
        return left != nullptr;
    else
        return left != nullptr && right != nullptr; 
}
RE_tree::~RE_tree()
{
}

NFA::NFA(const RE_tree& re_tree)
{
    assert(re_tree);
    std::unique_ptr<NFA> left, right;
    switch (re_tree.op)
    {
        case TERMINAL:
            *this = NFA(re_tree.value);
            break;
        case UNION:
            *this = NFA(*re_tree.left);
            this -> union_other(NFA(*re_tree.right));
            break;
        case CONCAT:
            *this = NFA(*re_tree.left);
            this -> concat_other(NFA(*re_tree.right));
            break;
        case KLEENE_STAR:
            *this = NFA(*re_tree.left);
            this -> kleene_star();
            break;
        case PLUS:
            *this = NFA(*re_tree.left);
            this -> plus();
            break;
        default:
            break;
    }

    this -> terminal_chars = re_tree.terminal_chars;
}
NFA::NFA(const char terminal)
{
    start_state = std::make_shared<nfa_state>();
    owned_states.push_back(start_state);

    auto final_ptr = std::make_shared<nfa_state>();
    owned_states.push_back(final_ptr);

    start_state -> is_final = false;
    start_state -> transfers.insert({terminal, final_ptr});
    final_state = final_ptr;
    final_state -> is_final = true;

    this -> terminal_chars.insert(terminal);
}
bool NFA::union_other(const NFA& other)
{
    auto copyed_other = NFA(other);
    std::shared_ptr<nfa_state> new_start_state = std::make_shared<nfa_state>(), new_final_state = std::make_shared<nfa_state>();
    owned_states.push_back(new_start_state);
    owned_states.push_back(new_final_state);
    owned_states.insert(owned_states.end(), copyed_other.owned_states.begin(), copyed_other.owned_states.end());
    new_start_state -> is_final = 0;
    new_final_state -> is_final = 1;
    new_start_state -> transfers.insert({'\0', this -> start_state});
    new_start_state -> transfers.insert({'\0', copyed_other.start_state});
    this -> final_state -> is_final = 0;
    this -> final_state -> transfers.insert({'\0', new_final_state});
    copyed_other.final_state -> is_final = 0;
    copyed_other.final_state -> transfers.insert({'\0', new_final_state});
    this -> start_state = new_start_state;
    this -> final_state = new_final_state;

    this -> terminal_chars.insert(copyed_other.terminal_chars.begin(), copyed_other.terminal_chars.end());
    return true;
}
bool NFA::concat_other(const NFA& other)
{
    auto copyed_other = NFA(other);
    owned_states.insert(owned_states.end(), copyed_other.owned_states.begin(), copyed_other.owned_states.end());
    this -> final_state -> is_final = 0;
    this -> final_state -> transfers.insert({'\0', copyed_other.start_state});
    this -> final_state = copyed_other.final_state;

    this -> terminal_chars.insert(copyed_other.terminal_chars.begin(), copyed_other.terminal_chars.end());
    return true;
}
bool NFA::kleene_star()
{
    auto new_start_state = std::make_shared<nfa_state>(), new_final_state = std::make_shared<nfa_state>();
    owned_states.push_back(new_start_state);
    owned_states.push_back(new_final_state);
    new_start_state -> is_final = 0;
    new_final_state -> is_final = 1;
    new_start_state -> transfers.insert({'\0', this -> start_state});
    new_start_state -> transfers.insert({'\0', new_final_state});
    this -> final_state -> is_final = 0;
    this -> final_state -> transfers.insert({'\0', this -> start_state});
    this -> final_state -> transfers.insert({'\0', new_final_state});
    this -> start_state = new_start_state;
    this -> final_state = new_final_state;
    return true;
}
bool NFA::plus()
{
    auto other = NFA(*this);
    kleene_star();
    concat_other(other);
    return true;
}
NFA::~NFA()
{
}
NFA::NFA(const NFA& other)
{
    std::queue<std::shared_ptr<nfa_state>> to_visit;
    std::unordered_map<std::shared_ptr<nfa_state>, std::shared_ptr<nfa_state>> copyed_state_map;

    std::shared_ptr<nfa_state> new_start_state = std::make_shared<nfa_state>();
    owned_states.push_back(new_start_state);
    new_start_state -> is_final = other.start_state -> is_final;
    copyed_state_map[other.start_state] = new_start_state;
    for (auto& transfer: other.start_state -> transfers)
    {
        auto target = transfer.second.lock();
        if (!target) continue;
        std::shared_ptr<nfa_state> new_transfer_state = std::make_shared<nfa_state>();
        owned_states.push_back(new_transfer_state);
        copyed_state_map[target] = new_transfer_state;
        new_start_state -> transfers.insert({transfer.first, new_transfer_state});
        to_visit.push(target);
        new_transfer_state -> is_final = target -> is_final;
    }

    while (!to_visit.empty()) 
    {
        auto cur_old_state = to_visit.front();
        to_visit.pop();

        auto copyed_cur_state = copyed_state_map[cur_old_state];
        copyed_cur_state -> is_final = cur_old_state -> is_final;
        for (auto& transfer: cur_old_state -> transfers)
        {
            auto target = transfer.second.lock();
            if (!target) continue;
            if (copyed_state_map.find(target) == copyed_state_map.end())
            {
                copyed_state_map[target] = std::make_shared<nfa_state>();
                owned_states.push_back(copyed_state_map[target]);
                to_visit.push(target);
            }
            auto &new_transdef_state = copyed_state_map[target];
            copyed_cur_state -> transfers.insert({transfer.first, new_transdef_state});
            new_transdef_state -> is_final = target -> is_final;            
        }
    }
    this -> start_state = new_start_state;
    this -> final_state = copyed_state_map[other.final_state];

    this -> terminal_chars = other.terminal_chars;
}
void NFA::swap(NFA& first, NFA& second)
{
    using std::swap;
    swap(first.start_state, second.start_state);
    swap(first.final_state, second.final_state);
    swap(first.terminal_chars, second.terminal_chars);
    swap(first.owned_states, second.owned_states);
}
NFA& NFA::operator=(const NFA& other)
{
    NFA copyed_other(other);
    swap(*this, copyed_other);
    return *this;
}

DFA::DFA(const NFA& nfa)
{
    this -> terminal_chars.insert(nfa.terminal_chars.begin(), nfa.terminal_chars.end());
    nfa_state_set_t closure_states;
    std::map<nfa_state_set_t, std::shared_ptr<dfa_state>> old2new_map;
    std::queue<nfa_state_set_t> unmarked_old_states;
    unmarked_old_states.push(epsilon_closure({nfa.start_state}));
    auto first_state = std::make_shared<dfa_state>();
    owned_states.push_back(first_state);
    old2new_map[unmarked_old_states.front()] = first_state;
    this -> start_state = old2new_map[unmarked_old_states.front()];
    while(!unmarked_old_states.empty())
    {
        auto cur = unmarked_old_states.front();
        unmarked_old_states.pop();
        for (const auto& ter_char: this -> terminal_chars)
        {
            auto temp_states = epsilon_closure(move(cur, ter_char));
            if (temp_states.empty())
                continue;
            if (old2new_map.find(temp_states) == old2new_map.end())
            {
                unmarked_old_states.push(temp_states);
                auto new_state_ptr = std::make_shared<dfa_state>();
                owned_states.push_back(new_state_ptr);
                old2new_map[temp_states] = new_state_ptr;
            }
            auto &new_state = old2new_map[temp_states];
            old2new_map[cur] -> transfers.insert({ter_char, new_state});
        }
    }
    for (const auto& pair: old2new_map)
    {
        auto &old_states = pair.first;
        auto &new_state = pair.second;
        for (const auto& st: old_states)
        {
            if (st -> is_final)
            {
                new_state -> is_final = 1;
                break;
            }
        }
    }
}
nfa_state_set_t DFA::move(const nfa_state_set_t& states, char input)
{
    nfa_state_set_t result;
    for (const auto& st: states)
    {
        auto valid_transfers = st -> transfers.equal_range(input);
        for (auto itr = valid_transfers.first; itr != valid_transfers.second; itr++)
        {
            auto target = itr -> second.lock();
            if (target)
                result.insert(target);
        }
    }
    return result;
}
nfa_state_set_t DFA::epsilon_closure(const nfa_state_set_t& states)
{
    nfa_state_set_t result = states;
    std::queue<std::shared_ptr<nfa_state>> q;
    for (const auto& st: states)
    {
        q.push(st);
    }
    while (!q.empty())
    {
        auto cur = q.front();
        q.pop();
        auto valid_transfers = cur -> transfers.equal_range('\0');
        for (auto itr = valid_transfers.first; itr != valid_transfers.second; itr++)
        {
            auto target = itr -> second.lock();
            if (target && result.find(target) == result.end())
            {
                result.insert(target);
                q.push(target);
            }
        }
    }
    return result;
}
size_t DFA::longest_match(const std::string& input, size_t start_pos)
{
    auto current_states = start_state;
    size_t matched_length = 0;
    size_t last_final_pos = start_pos;
    for (size_t i = start_pos; i < input.length(); i++)
    {
        bool transitioned = false;
        auto valid_transfers = current_states -> transfers.equal_range(input[i]);
        if (valid_transfers.first == valid_transfers.second)
            break;
        for (auto transfer = valid_transfers.first; transfer != valid_transfers.second; transfer++)
        {
            auto target = transfer -> second.lock();
            if (target)
            {
                current_states = target;
                transitioned = true;
                matched_length++;
                if (current_states -> is_final)
                {
                    last_final_pos = start_pos + matched_length;
                }
                break;
            }
        }

        if (!transitioned)
            break;
    }

    return last_final_pos - start_pos;
}
bool DFA::all_match(const std::string& input, size_t start_pos)
{
    auto current_states = start_state;
    bool matched = 0;

    for (size_t i = start_pos; i < input.length(); i++)
    {
        auto valid_transfers = current_states -> transfers.equal_range(input[i]);
        matched = 0;
        for (auto transfer = valid_transfers.first; transfer != valid_transfers.second; transfer++)
        {
            auto target = transfer -> second.lock();
            if (target)
            {
                current_states = target;
                matched = 1;
                break;
            }
        }
        if (matched == 0)
            return 0;
        matched = 0;
    }

    return current_states -> is_final;
}
DFA::~DFA()
{
}