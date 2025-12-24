#include "DFA.h"
#include <numeric>

void trim_inplace(std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) {
        str.clear();
        return;
    }
    size_t end = str.find_last_not_of(" \t\n\r");
    str = str.substr(start, end - start + 1);
}

#ifndef DFA_ONLY
RE::RE(std::string &pattern)
{
    std::istringstream pattern_stream(pattern);
    std::string line;
    std::vector<std::pair<std::string, std::string>> defination_patterns;
    while (std::getline(pattern_stream, line))
    {
        trim_inplace(line);
        if (line.empty()) continue;
        defination_patterns.push_back(split_pattern_line(line));
    }
    merge_patterns(defination_patterns);
    parse_pattern();
}
RE::RE(std::string &pattern, std::vector<bool> &op_pattern)
{
    this -> pattern = pattern;
    this -> op_pattern = op_pattern;
}
bool RE::pattern_cmp(const std::pair<std::string, std::string>& a, const std::pair<std::string, std::string>& b)
{
    return a.first.length() > b.first.length();
}
RE::~RE() {}
std::pair<std::string, std::vector<bool>> RE::getPattern()
{
    return {pattern, op_pattern};
}
std::pair<std::string, std::string> RE::split_pattern_line(std::string &input_pattern)
{
    std::string def_name, def_pattern;
    trim_inplace(input_pattern);
    if(input_pattern.empty())
        return {"", ""};
    def_name = input_pattern.substr(0, input_pattern.find("->"));
    def_pattern = input_pattern.substr(input_pattern.find("->") + 2);
    trim_inplace(def_name);
    trim_inplace(def_pattern);
    return {def_name, def_pattern};
}
void RE::merge_patterns(std::vector<std::pair<std::string, std::string>> &defination_patterns)
{
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
    raw_pattern = defination_patterns[0].second;
    return;
}
void RE::parse_pattern()
{
    for (auto itr = raw_pattern.begin(); itr != raw_pattern.end(); itr++)
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
RE RE::postfix_form() const
{
    std::stack<std::pair<RE_char_type, RE_operator>> op_stack;
    std::string postfix_pattern;
    std::vector<bool> postfix_op_pattern;
    std::map<RE_operator, int> op_precedence = {
        {UNION, 1},
        {CONCAT, 2},
        {KLEENE_STAR, 3},
        {PLUS, 3},
        {LEFT_BRACKET, -1},
        {RIGHT_BRACKET, -1}
    };
    for (size_t i = 0; i < pattern.length(); i++)
    {
        if (op_pattern[i] == RECHAR)
        {
            postfix_pattern.push_back(pattern[i]);
            postfix_op_pattern.push_back(RECHAR);
        }
        else if (op_pattern[i] == OPTR)
        {
            if (pattern[i] == LEFT_BRACKET)
            {
                op_stack.push({OPTR, LEFT_BRACKET});
            }
            else if (pattern[i] == RIGHT_BRACKET)
            {
                while(!op_stack.empty() && op_stack.top() != std::make_pair(OPTR, LEFT_BRACKET))
                {
                    postfix_pattern.push_back(op_stack.top().second);
                    postfix_op_pattern.push_back(OPTR);
                    op_stack.pop();
                }
                op_stack.pop(); // 弹出左括号
            }
            else 
            {
                while(!op_stack.empty() && (op_stack.top() != std::make_pair(OPTR, LEFT_BRACKET)) &&
                        op_precedence[op_stack.top().second] >= op_precedence[static_cast<RE_operator>(pattern[i])])
                {
                    postfix_pattern.push_back(op_stack.top().second);
                    postfix_op_pattern.push_back(OPTR);
                    op_stack.pop();
                }
                op_stack.push({OPTR, static_cast<RE_operator>(pattern[i])});
            }
        }
    }
    while(!op_stack.empty())
    {
        postfix_pattern.push_back(op_stack.top().second);
        postfix_op_pattern.push_back(OPTR);
        op_stack.pop();
    }
    return RE(postfix_pattern, postfix_op_pattern);
}

NFA& NFA::operator=(const NFA& other)
{
    if (this != &other)
    {
        start_state = other.start_state;
        final_state = other.final_state;
        terminal_chars = other.terminal_chars;
        owned_states = other.owned_states;
    }
    return *this;
}
NFA::NFA(const NFA& other)
{
    start_state = other.start_state;
    final_state = other.final_state;
    terminal_chars = other.terminal_chars;
    owned_states = other.owned_states;
}
NFA::NFA(const RE& re)
{
    RE postfix_re = re.postfix_form();
    auto re_pattern = postfix_re.getPattern();
    auto pattern = re_pattern.first;
    auto op_pattern = re_pattern.second;
    std::stack<std::unique_ptr<NFA>> nfa_stack;

    for (size_t i = 0; i < pattern.length(); i++)
    {
        if (op_pattern[i] == RECHAR)
        {
            nfa_stack.push(std::make_unique<NFA>(pattern[i]));
        }
        else
        {
            if (pattern[i] == UNION)
            {
                auto right = std::move(nfa_stack.top());
                nfa_stack.pop();
                auto left =  std::move(nfa_stack.top());
                nfa_stack.pop();
                left -> union_other(*right);
                nfa_stack.push(std::move(left));
            }
            else if (pattern[i] == CONCAT)
            {
                auto right = std::move(nfa_stack.top());
                nfa_stack.pop();
                auto left =  std::move(nfa_stack.top());
                nfa_stack.pop();
                left -> concat_other(*right);
                nfa_stack.push(std::move(left));
            }
            else if (pattern[i] == KLEENE_STAR)
            {
                auto top = std::move(nfa_stack.top());
                nfa_stack.pop();
                top -> kleene_star();
                nfa_stack.push(std::move(top));
            }
            else if (pattern[i] == PLUS)
            {
                auto top = std::move(nfa_stack.top());
                nfa_stack.pop();
                top -> plus();
                nfa_stack.push(std::move(top));
            }
        }
    }
    *this = *(nfa_stack.top());
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
    std::shared_ptr<nfa_state> new_start_state = std::make_shared<nfa_state>(), new_final_state = std::make_shared<nfa_state>();
    owned_states.push_back(new_start_state);
    owned_states.push_back(new_final_state);
    owned_states.insert(owned_states.end(), other.owned_states.begin(), other.owned_states.end());
    new_start_state -> is_final = 0;
    new_final_state -> is_final = 1;
    new_start_state -> transfers.insert({'\0', this -> start_state});
    new_start_state -> transfers.insert({'\0', other.start_state});
    this -> final_state -> is_final = 0;
    this -> final_state -> transfers.insert({'\0', new_final_state});
    other.final_state -> is_final = 0;
    other.final_state -> transfers.insert({'\0', new_final_state});
    this -> start_state = new_start_state;
    this -> final_state = new_final_state;

    this -> terminal_chars.insert(other.terminal_chars.begin(), other.terminal_chars.end());
    return true;
}
bool NFA::concat_other(const NFA& other)
{
    owned_states.insert(owned_states.end(), other.owned_states.begin(), other.owned_states.end());
    this -> final_state -> is_final = 0;
    this -> final_state -> transfers.insert({'\0', other.start_state});
    this -> final_state = other.final_state;

    this -> terminal_chars.insert(other.terminal_chars.begin(), other.terminal_chars.end());
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
    auto new_start_state = std::make_shared<nfa_state>(), new_final_state = std::make_shared<nfa_state>();
    owned_states.push_back(new_start_state);
    owned_states.push_back(new_final_state);
    new_start_state -> is_final = 0;
    new_final_state -> is_final = 1;
    new_start_state -> transfers.insert({'\0', this -> start_state});
    this -> final_state -> is_final = 0;
    this -> final_state -> transfers.insert({'\0', this -> start_state});
    this -> final_state -> transfers.insert({'\0', new_final_state});
    this -> start_state = new_start_state;
    this -> final_state = new_final_state;
    return true;
}
NFA::~NFA()
{
}
#endif

#ifndef DFA_ONLY
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

    this -> minimize();
}
void DFA::minimize()
{
    if (!start_state || owned_states.empty())
        return;

    // Step 1: remove unreachable states so we do not keep dead nodes around.
    std::queue<std::shared_ptr<dfa_state>> q;
    std::unordered_set<std::shared_ptr<dfa_state>> reachable;
    q.push(start_state);
    reachable.insert(start_state);
    while (!q.empty())
    {
        auto cur = q.front();
        q.pop();
        for (const auto &tr : cur->transfers)
        {
            auto target = tr.second.lock();
            if (target && !reachable.count(target))
            {
                reachable.insert(target);
                q.push(target);
            }
        }
    }

    std::vector<std::shared_ptr<dfa_state>> states;
    states.reserve(reachable.size());
    for (const auto &st : owned_states)
    {
        if (reachable.count(st))
            states.push_back(st);
    }

    if (states.size() <= 1)
    {
        owned_states = std::move(states);
        return;
    }

    // Build index mapping for table-driven minimization.
    std::unordered_map<std::shared_ptr<dfa_state>, int> id_map;
    for (size_t i = 0; i < states.size(); i++)
        id_map[states[i]] = static_cast<int>(i);

    // Precompute transitions as indices; missing transitions stay at -1 (implicit sink).
    std::vector<std::unordered_map<char, int>> trans(states.size());
    for (size_t i = 0; i < states.size(); i++)
    {
        for (const auto &tr : states[i]->transfers)
        {
            auto target = tr.second.lock();
            auto itr = id_map.find(target);
            if (target && itr != id_map.end())
                trans[i][tr.first] = itr->second;
        }
    }

    const size_t n = states.size();
    std::vector<std::vector<bool>> distinguish(n, std::vector<bool>(n, false));

    // Initialize distinguishing table: final vs non-final.
    for (size_t i = 0; i < n; i++)
    {
        for (size_t j = i + 1; j < n; j++)
        {
            if (states[i]->is_final != states[j]->is_final)
            {
                distinguish[i][j] = true;
                distinguish[j][i] = true;
            }
        }
    }

    bool updated = true;
    while (updated)
    {
        updated = false;
        for (size_t i = 0; i < n; i++)
        {
            for (size_t j = i + 1; j < n; j++)
            {
                if (distinguish[i][j])
                    continue;

                for (const auto &ch : terminal_chars)
                {
                    auto it_i = trans[i].find(ch);
                    auto it_j = trans[j].find(ch);
                    int ti = (it_i == trans[i].end()) ? -1 : it_i->second;
                    int tj = (it_j == trans[j].end()) ? -1 : it_j->second;

                    if (ti == -1 && tj == -1)
                        continue;

                    if (ti == -1 || tj == -1)
                    {
                        distinguish[i][j] = distinguish[j][i] = true;
                        updated = true;
                        break;
                    }

                    size_t a = std::min(ti, tj);
                    size_t b = std::max(ti, tj);
                    if (distinguish[a][b])
                    {
                        distinguish[i][j] = distinguish[j][i] = true;
                        updated = true;
                        break;
                    }
                }
            }
        }
    }

    // Union-find to merge indistinguishable states.
    std::vector<int> parent(n);
    std::iota(parent.begin(), parent.end(), 0);

    std::function<int(int)> find = [&](int x) -> int
    {
        return parent[x] == x ? x : parent[x] = find(parent[x]);
    };

    auto unite = [&](int a, int b)
    {
        a = find(a);
        b = find(b);
        if (a != b)
            parent[b] = a;
    };

    for (size_t i = 0; i < n; i++)
    {
        for (size_t j = i + 1; j < n; j++)
        {
            if (!distinguish[i][j])
                unite(static_cast<int>(i), static_cast<int>(j));
        }
    }

    // Build new minimized DFA states.
    std::unordered_map<int, int> rep2idx;
    std::vector<std::shared_ptr<dfa_state>> new_states;

    for (size_t i = 0; i < n; i++)
    {
        int rep = find(static_cast<int>(i));
        if (rep2idx.find(rep) == rep2idx.end())
        {
            rep2idx[rep] = static_cast<int>(new_states.size());
            auto ns = std::make_shared<dfa_state>();
            ns->is_final = states[i]->is_final;
            new_states.push_back(ns);
        }
        else if (states[i]->is_final)
        {
            new_states[rep2idx[rep]]->is_final = true;
        }
    }

    for (size_t i = 0; i < n; i++)
    {
        int rep_src = find(static_cast<int>(i));
        auto src_state = new_states[rep2idx[rep_src]];
        for (const auto &tr : trans[i])
        {
            int rep_dst = find(tr.second);
            src_state->transfers[tr.first] = new_states[rep2idx[rep_dst]];
        }
    }

    // Update start and owned states.
    int start_idx = id_map[start_state];
    int start_rep = find(start_idx);
    start_state = new_states[rep2idx[start_rep]];
    owned_states = std::move(new_states);
}
#endif

DFA::DFA(const std::string &import_str)
{
    std::istringstream import_stream(import_str);
    std::string line;
    while(line.empty())
        std::getline(import_stream, line);
    owned_states.resize(std::stoi(line));
    std::getline(import_stream, line);
    for (size_t i = 0; i < line.length(); i++)
    {
        if(line[i] == '0')
        {
            owned_states[i] = std::make_shared<dfa_state>();
            owned_states[i] -> is_final = 0;
        }
        else
        {
            owned_states[i] = std::make_shared<dfa_state>();
            owned_states[i] -> is_final = 1;
        }
    }
    while (std::getline(import_stream, line))
    {
        std::istringstream line_stream(line);
        int cur_state_id;
        line_stream >> cur_state_id;
        auto cur_state = owned_states[cur_state_id];
        int ch;
        int target_state_id;
        while(line_stream >> ch >> target_state_id)
        {
            cur_state -> transfers[char(ch)] = owned_states[target_state_id];
            terminal_chars.insert(char(ch));
        }
    }
    this -> start_state = owned_states[0];
    this -> minimize();
}
size_t DFA::longest_match(const std::string& input, size_t start_pos)
{
    auto current_states = start_state;
    size_t matched_length = 0;
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
                break;
            }
        }

        if (!transitioned)
            break;
    }

    return matched_length;
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
std::string DFA::export2str()
{
    int state_cnt = owned_states.size();
    std::ostringstream export_stream;
    export_stream << state_cnt << "\n";
    std::map<std::shared_ptr<dfa_state>, int> state2id_map;
    for (size_t i = 0; i < owned_states.size(); i++)
    {
        state2id_map[owned_states[i]] = i;
    }
    std::string is_final_line;
    for (size_t i = 0; i < owned_states.size(); i++)
    {
        if (owned_states[i] -> is_final)
            is_final_line.push_back('1');
        else
            is_final_line.push_back('0');
    }
    export_stream << is_final_line << "\n";
    for (size_t i = 0; i < owned_states.size(); i++)
    {
        auto cur_state = owned_states[i];
        std::string transfer_line = std::to_string(i) + " ";
        for (const auto& transfer_pair: cur_state -> transfers)
        {
            auto target = transfer_pair.second.lock();
            if (target)
            {
                transfer_line += std::to_string(int(transfer_pair.first));
                transfer_line += " ";
                transfer_line += std::to_string(state2id_map[target]);
                transfer_line += " ";
            }
        }
        export_stream << transfer_line << "\n";
    }
    return export_stream.str();
}
