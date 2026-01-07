// C语言词法分析器
#define ecnu_course
#include <list>
#include <map>
#include <set>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <queue>
#include <stack>
struct symbol_t
{
    bool is_terminal;
    std::string value;
    bool operator<(const symbol_t& other) const
    {
        if (is_terminal != other.is_terminal)
            return is_terminal < other.is_terminal;
        return value < other.value;
    }
    bool operator==(const symbol_t& other) const
    {
        return is_terminal == other.is_terminal && value == other.value;
    }
    bool empty() const
    {
        return value.empty();
    }
    void clear()
    {
        is_terminal = false;
        value.clear();
    }
};
struct item_t
{
    size_t prod_id;
    size_t dot_pos;
    std::string lookahead;
    bool operator<(const item_t& other) const
    {
        if (prod_id != other.prod_id)
            return prod_id < other.prod_id;
        if (dot_pos != other.dot_pos)
            return dot_pos < other.dot_pos;
        return lookahead < other.lookahead;
    }
    item_t(size_t pid, size_t dpos, const std::string& la)
        : prod_id(pid), dot_pos(dpos), lookahead(la)
    {}
};
struct action
{
    enum class action_kind
    {
        nonterminal,
        shift,
        reduce,
        accept
    } kind;
    size_t target; // shift 下一个状态； reduce 产生式的 id；nonterminal 属于 goto 表，下一个状态
};
typedef std::list<symbol_t> alter_prod_t;

class LL1;
class LL1_constructor;
class SyntaxTree;

template<typename T, typename U>
static void join_set_except(std::set<T>& dest, const std::set<T>& src, const U& skip = T())
{
    for (const auto& s: src)
    {
        if (s != skip)
            dest.insert(s);
    }
}
static void trim_inplace(std::string& str)
{
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) {
        str.clear();
        return;
    }
    size_t end = str.find_last_not_of(" \t\n\r");
    str = str.substr(start, end - start + 1);
}

class LR1
{
    symbol_t start_symbol;
    std::vector<std::pair<std::string, alter_prod_t>> productions;
    std::vector<std::map<symbol_t, action>> action_table;
    friend class LR1_constructor;
    void error_missing(const std::string& expected, size_t line)
    {
        std::cout << "语法错误，第" << line << "行，缺少\"" << expected << "\"\n";
    }
    void error_illegal(const std::string& got, size_t line)
    {
        std::cout << "语法错误,第" << line << "行,非法符号\"" << (got == "$" ? "EOF" : got) << "\"\n";
    }
public:
    const std::list<size_t> match(const std::string& input)
    {
        std::list<std::pair<std::string, size_t>> input_tokens;
        std::istringstream input_stream(input);
        std::string line;
        std::list<size_t> reductions;
        size_t line_number = 1;
        size_t last_matched_line = 1;
        while (getline(input_stream, line))
        {
            std::istringstream line_stream(line);
            std::string token;
            std::vector<std::string> line_tokens;
            while (line_stream >> token)
            {
                line_tokens.push_back(token);
            }
            if (!line_tokens.empty())
            {
                for (const auto& t: line_tokens)
                {
                    input_tokens.push_back({t, line_number});
                }
                line_number++;
            }
        }
        input_tokens.push_back({"$", line_number});
        std::stack<std::pair<symbol_t, size_t>> parse_stack;
        parse_stack.push({{true, "$"}, 0});
        while(!parse_stack.empty())
        {
            auto in = input_tokens.front();
            input_tokens.pop_front();
            std::string lookahead = in.first;
            size_t line_num = in.second;
            auto top = parse_stack.top();
            parse_stack.pop();
            if (action_table[top.second].find({true, lookahead}) == action_table[top.second].end())
            {
                // 没有找到表项：做简单错误恢复
                std::set<std::string> expected_set;
                for (const auto& kv : action_table[top.second])
                {
                    if (kv.first.is_terminal)
                    {
                        if (kv.first.value != "$")
                            expected_set.insert(kv.first.value);
                    }
                }
                if (expected_set.find(";") != expected_set.end())
                {
                    error_missing(";", last_matched_line);
                    parse_stack.push(top); // 重新期待栈顶符号
                    input_tokens.push_front(in);
                    input_tokens.push_front({";", last_matched_line}); // 插入缺失的
                }
                else
                {
                    error_illegal(lookahead, line_num);
                    parse_stack.push(top); // 重新期待栈顶符号
                    // 丢弃当前输入符号
                    if (lookahead == "$")
                    {
                        break; // 到达输入结尾，无法继续
                    }
                }
            }
            else
            {
                auto act = action_table[top.second][{true, lookahead}];
                last_matched_line = line_num;
                if(act.kind == action::action_kind::shift)
                {
                    parse_stack.push(top);
                    parse_stack.push({{true, lookahead}, act.target});
                }
                else if(act.kind == action::action_kind::reduce)
                {
                    parse_stack.push(top);
                    for(size_t i = 0; i < productions[act.target].second.size(); i++)
                    {
                        parse_stack.pop();
                    }
                    parse_stack.push({{false, productions[act.target].first}, 
                        action_table[parse_stack.top().second][{false, productions[act.target].first}].target});
                        input_tokens.push_front(in); // 重新期待当前输入符号
                    reductions.push_back(act.target);
                }
                else if(act.kind == action::action_kind::accept)
                {
                    parse_stack.pop();
                    reductions.push_back(act.target);
                    break; 
                }
            }
        }
        return reductions;
    }
    void print_Rderivation(const std::list<size_t>& derivation) const
    {
        auto print_form = [](const std::vector<symbol_t>& form)
        {
            for (const auto& sym : form)
            {
                std::cout << sym.value << " ";
            }
        };

        // Build rightmost derivation by applying reductions in reverse order.
        std::vector<symbol_t> form;
        form.push_back(start_symbol);

        // If derivation is empty, just print the start form.
        if (derivation.empty())
        {
            return;
        }

        for (auto it = derivation.rbegin(); it != derivation.rend(); ++it)
        {
            const size_t pid = *it;

            // Print current sentential form before applying this production.
            print_form(form);
            std::cout << "=> \n";

            if (pid >= productions.size()) continue;
            const auto& prod = productions[pid];
            const std::string& A = prod.first;

            // Rightmost replacement of nonterminal A
            int pos = -1;
            for (int i = static_cast<int>(form.size()) - 1; i >= 0; --i)
            {
                if (!form[i].is_terminal && form[i].value == A)
                {
                    pos = i;
                    break;
                }
            }
            if (pos < 0) continue;

            std::vector<symbol_t> next;
            next.reserve(form.size() + prod.second.size());
            next.insert(next.end(), form.begin(), form.begin() + pos);
            for (const auto& s : prod.second) next.push_back(s);
            next.insert(next.end(), form.begin() + pos + 1, form.end());
            form = std::move(next);
        }

        // Final sentential form (no "=>")
        print_form(form);
        std::cout << "\n";
    }
};
class LR1_constructor
{
    symbol_t start_symbol;
    std::map<std::string, std::vector<size_t>> prodsID_by_left;
    std::vector<std::pair<std::string, alter_prod_t>> productions;
    std::set<std::string> nonterminals;
    std::set<std::string> terminals;
    std::map<std::string, std::set<std::string>> first_sets;
    std::map<std::string, std::set<std::string>> follow_sets;
    void remove_E_in_symbol()
    {
        for (auto& prod: productions)
            if (prod.second.size() == 1 && prod.second.front().is_terminal && prod.second.front().value == "E")
                prod.second.clear();
    }
    void set_nont_and_term()
    {
        nonterminals.clear();
        terminals.clear();
        for (const auto& prod: productions)
            nonterminals.insert(prod.first);
        for (auto& prod: productions)
            for (auto& sym: prod.second)
                if (nonterminals.find(sym.value) == nonterminals.end())
                {
                    sym.is_terminal = 1;
                    terminals.insert(sym.value);
                }
                else
                {
                    sym.is_terminal = 0;
                    nonterminals.insert(sym.value);
                }
    }
    void compute_first_sets()
    {
        terminals.erase("E");

        for (const auto& nt : nonterminals) first_sets[nt] = {};
        for (const auto& t : terminals) first_sets[t] = {t};
        first_sets["E"] = {"E"};
        first_sets["$"] = {"$"};

        bool changed = true;
        while (changed) {
            changed = false;
            for (const auto& prod_kv : productions) {
                const std::string& A = prod_kv.first;
                const alter_prod_t& rhs = prod_kv.second;

                bool all_nullable = true;
                for (const auto& sym: rhs)
                {
                    if(sym.is_terminal)
                    {
                        if (sym.value == "E")
                            continue;
                        if (first_sets[A].insert(sym.value).second)
                            changed = true;
                        all_nullable = false;
                        break;
                    }
                    // Nonterminal
                    size_t before = first_sets[A].size();
                    join_set_except(first_sets[A], first_sets[sym.value], "E");
                    if (first_sets[A].size() != before) 
                        changed = true;
                    if (first_sets[sym.value].find("E") == first_sets[sym.value].end()) {
                        all_nullable = false;
                        break;
                    }
                }
                if (all_nullable) {
                    if (first_sets[A].insert("E").second) changed = true;
                }
            }
        }
    }
    void compute_follow_sets()
    {
        for (const auto& nt : nonterminals) follow_sets[nt] = {};
        if (!productions.empty())
            follow_sets[start_symbol.value].insert("$");

        bool fchanged = true;
        while (fchanged) {
            fchanged = false;
            for (const auto& prod : productions) {
                const std::string& A = prod.first;
                for (auto it = prod.second.begin(); it != prod.second.end(); it++) {
                    const std::string& B = it->value;
                    if (nonterminals.find(B) == nonterminals.end()) continue;
                    alter_prod_t beta(it, prod.second.end());
                    beta.pop_front(); // remove B
                    // FIRST(beta)
                    std::set<std::string> firstBeta;
                    if (beta.empty()) {
                        firstBeta.insert("E");
                    } else {
                        bool all_nullable = true;
                        for (const auto& X : beta) {
                            join_set_except(firstBeta, first_sets[X.value], "E");
                            if (first_sets[X.value].find("E") == first_sets[X.value].end()) {
                                all_nullable = false;
                                break;
                            }
                        }
                        if (all_nullable) firstBeta.insert("E");
                    }
                    // FIRST(beta) - E
                    for (const auto& t : firstBeta) {
                        if (t == "E") continue;
                        auto ins = follow_sets[B].insert(t);
                        if (ins.second) fchanged = true;
                    }
                    if (beta.empty() || firstBeta.find("E") != firstBeta.end()) {
                        for (const auto& t : follow_sets[A]) {
                            auto ins = follow_sets[B].insert(t);
                            if (ins.second) fchanged = true;
                        }
                    }
                    
                }
            }
        }
    }
    void clear()
    {
        start_symbol.clear();
        productions.clear();
        nonterminals.clear();
        terminals.clear();
        first_sets.clear();
        follow_sets.clear();
    }
    std::vector<std::map<symbol_t, action>> build_action_table()
    {
        std::set<item_t> I0;
        std::map<std::string, int> state_idx;
        std::vector<std::map<symbol_t, int>> goto_table;
        std::vector<std::set<item_t>> state_set;
        item_t item_start{prodsID_by_left[start_symbol.value][0], 0, "$"};
        I0.insert(item_start);
        I0 = closure(I0);
        state_idx[item_set_name(I0)] = 0;
        state_set.push_back(std::move(I0));
        std::queue<size_t> state_q;
        state_q.push(0);
        while(!state_q.empty())
        {
            size_t i = state_q.front();
            state_q.pop();

            if (i >= goto_table.size())
                goto_table.resize(i + 1);

            for (const auto& X: nonterminals)
            {
                symbol_t sym{false, X};
                auto J = go_to(state_set[i], sym);
                if (J.empty())
                    continue;
                std::string name = item_set_name(J);
                auto it = state_idx.find(name);
                size_t j;
                if (it == state_idx.end())
                {
                    j = state_set.size();
                    state_idx[name] = j;
                    state_set.push_back(std::move(J));
                    state_q.push(j);
                }
                else
                    j = it -> second;
                goto_table[i][sym] = j;
            }
            for (const auto& X: terminals)
            {
                symbol_t sym{true, X};
                auto J = go_to(state_set[i], sym);
                if (J.empty())
                    continue;
                std::string name = item_set_name(J);
                auto it = state_idx.find(name);
                size_t j;
                if (it == state_idx.end())
                {
                    j = state_set.size();
                    state_idx[name] = j;
                    state_set.push_back(std::move(J));
                    state_q.push(j);
                }
                else
                    j = it -> second;
                goto_table[i][sym] = j;
            }
        }

        std::vector<std::map<symbol_t, action>> action_table;
        action_table.resize(state_set.size());
        for (size_t i = 0; i < state_set.size(); i++)
        {
            // 对于每一个状态，首先遍历 goto 表确定 shift（非终结符）或 goto（终结符）
            for (const auto& goto_kv: goto_table[i])
            {
                const auto& X = goto_kv.first;
                size_t j = goto_kv.second;
                if (X.is_terminal)
                    action_table[i][X] = {action::action_kind::shift, j};
                else
                    action_table[i][X] = {action::action_kind::nonterminal, j};
            }

            // 再遍历每一个状态的每一个 item 确定 reduce 或 accept
            for (const auto& item: state_set[i])
            {
                const auto& prod = productions[item.prod_id];
                if (item.dot_pos < prod.second.size())
                    // 点不在末尾，直接跳过
                    continue;
                if (prod.first == start_symbol.value && item.lookahead == "$")
                {
                    // 开始符的末尾
                    action_table[i][{true, "$"}] = {action::action_kind::accept, prodsID_by_left[start_symbol.value][0]};
                    continue;
                }
                action_table[i][{true, item.lookahead}] = {action::action_kind::reduce, item.prod_id};
            }
        }
        return action_table;
    }
    std::set<std::string> get_first_set(const alter_prod_t& seq)
    {
        std::set<std::string> out;
        if (seq.empty()) {
            out.insert("E");
            return out;
        }
        bool all_nullable = true;
        for (const auto& sym : seq) {
            join_set_except(out, first_sets[sym.value], "E");
            if (first_sets[sym.value].find("E") == first_sets[sym.value].end()) {
                all_nullable = false;
                break;
            }
        }
        if (all_nullable) out.insert("E");
        return out;
    }
    std::set<item_t> closure(const std::set<item_t>& I)
    {
        std::set<item_t> out = I;
        bool changed = true;
        while (changed) {
            changed = false;
            std::vector<item_t> to_add;
            for (const auto& item : out) {
                const auto& prod = productions[item.prod_id];
                if (item.dot_pos >= prod.second.size()) continue;

                const auto& B_sym = *std::next(prod.second.begin(), item.dot_pos);
                const std::string& B = B_sym.value;
                if (nonterminals.find(B) == nonterminals.end()) continue;

                // 向 lookahead 中补充 dot 后面的符号的 first 集合
                alter_prod_t beta;
                for (auto it = std::next(prod.second.begin(), item.dot_pos + 1);
                    it != prod.second.end(); it++) beta.push_back(*it);
                beta.push_back({true, item.lookahead});

                auto firstSet = get_first_set(beta);
                for (const auto& b : firstSet) {
                    if (b == "E") continue;
                    auto it = prodsID_by_left.find(B);
                    for (size_t rid : it->second) {
                        item_t ni{rid, 0, b};
                        if (out.find(ni) == out.end()) {
                            to_add.push_back(std::move(ni));
                        }
                    }
                }
            }

            for (const auto& ni : to_add) {
                if (out.insert(ni).second) changed = true;
            }
        }
        return out;
    }
    std::set<item_t> go_to(const std::set<item_t>& I, const symbol_t& X)
    {
        std::set<item_t> J;
        for (const auto& item : I)
        {
            const auto& prod = productions[item.prod_id];
            if (item.dot_pos < prod.second.size() && (*std::next(prod.second.begin(), item.dot_pos)) == X)
                J.insert({item.prod_id, item.dot_pos + 1, item.lookahead});
        }
        return closure(J);
    }
    std::string item_set_name(const std::set<item_t>& I)
    {
        std::string out;
        for (const auto& it : I)
        {
            out += std::to_string(it.prod_id);
            out.push_back(':');
            out += std::to_string(it.dot_pos);
            out.push_back(':');
            out += it.lookahead;
            out.push_back(';');
        }
        return out;
    }
public:
    const LR1 construct(std::string& pro)
    {
        LR1 result;
        std::istringstream pro_stream(pro);
        std::string line;
        while (std::getline(pro_stream, line))
        {
            trim_inplace(line);
            if (line.empty()) continue;
            std::string the_left, the_right;
            the_left = line.substr(0, line.find("->"));
            the_right = line.substr(line.find("->") + 2);
            trim_inplace(the_left);
            trim_inplace(the_right);
            if (start_symbol.empty())
            {
                start_symbol = {false, the_left};
            }
            std::list<std::string> right_parts;
            std::istringstream right_stream(the_right);
            std::string part;
            prodsID_by_left[the_left].push_back(static_cast<int>(productions.size()));
            productions.push_back({the_left, {}});
            nonterminals.insert(the_left);
            while (right_stream >> part)
            {
                if (part != "|")
                {
                        productions.back().second.push_back({true, part});
                }
                else
                {
                    prodsID_by_left[the_left].push_back(static_cast<int>(productions.size()));
                    productions.push_back({the_left, {}});
                }
            }
        }
        
        #ifndef ecnu_course
        std::string augmented_start = start_symbol.value;
        do
        {
            augmented_start = augmented_start + "'";
        } while (prodsID_by_left.find(augmented_start) != prodsID_by_left.end());
        for (const auto& prod: productions)
        {
            for (const auto& sym: prod.second)
            {
                if (nonterminals.find(sym.value) == nonterminals.end())
                {
                    terminals.insert(sym.value);
                }
            }
        }
        prodsID_by_left[augmented_start].push_back(static_cast<int>(productions.size()));
        productions.push_back({augmented_start, {start_symbol}});
        start_symbol = {false, augmented_start};
        #endif

        set_nont_and_term();
        compute_first_sets();
        compute_follow_sets();
        remove_E_in_symbol();
        result.action_table = build_action_table();
        result.start_symbol = start_symbol;
        result.productions = productions;
        clear();
        return result;
    }
};

using namespace std;
/* 不要修改这个标准输入函数 */
void read_prog(string& prog)
{
	char c;
	while(scanf("%c",&c)!=EOF){
		prog += c;
	}
}
/* 你可以添加其他函数 */

std::string GRAMMER =
"program -> compoundstmt\n"
"stmt ->  ifstmt  |  whilestmt  |  assgstmt  |  compoundstmt\n"
"compoundstmt ->  { stmts }\n"
"stmts ->  stmt stmts   |   E\n"
"ifstmt ->  if ( boolexpr ) then stmt else stmt\n"
"whilestmt ->  while ( boolexpr ) stmt\n"
"assgstmt ->  ID = arithexpr ;\n"
"boolexpr  ->  arithexpr boolop arithexpr\n"
"boolop ->   <  |  >  |  <=  |  >=  | ==\n"
"arithexpr  ->  multexpr arithexprprime\n"
"arithexprprime ->  + multexpr arithexprprime  |  - multexpr arithexprprime  |   E\n"
"multexpr ->  simpleexpr  multexprprime\n"
"multexprprime ->  * simpleexpr multexprprime  |  / simpleexpr multexprprime  |   E\n"
"simpleexpr ->  ID  |  NUM  |  ( arithexpr )\n";

void Analysis()
{
	string prog;
	read_prog(prog);
	/* 骚年们 请开始你们的表演 */
    /********* Begin *********/
	LR1_constructor constructor;
	LR1 parser = constructor.construct(GRAMMER);
    parser.print_Rderivation(parser.match(prog));

    /********* End *********/
	
}