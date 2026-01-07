// C语言词法分析器
#define ecnu_course
#include <string>
#include <vector>
#include <list>
#include <sstream>
#include <set>
#include <map>
#include <algorithm>
#include <iostream>
#include <stack>
#include <memory>
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
typedef std::list<symbol_t> alter_prod_t;

class LL1;
class LL1_constructor;
class SyntaxTree;

std::set<std::string> global_sync = 
{
#ifdef ecnu_course
    "else", "}", "$", ")"
#endif
};
template<typename T>
static void join_set_except(std::set<T>& dest, const std::set<T>& src, const T& skip = T())
{
    for (const auto& s: src)
    {
        if (s != skip)
            dest.insert(s);
    }
}
static bool nullable(const std::vector<alter_prod_t>& prods)
{
    for (const auto& alt : prods)
    {
        if (alt.size() == 1 && alt.front().is_terminal && alt.front().value == "E")
        {
            return true;
        }
    }
    return false;
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

class SyntaxTree
{
    std::string value;
    std::list<std::shared_ptr<SyntaxTree>> children;
    friend class LL1;
public:
    void print(size_t depth = 0) const
    {
        if (value == "$")
            return;
        std::cout << std::string(depth, '\t') << value << "\n";
        for (const auto& child : children)
            child->print(depth + 1);
    }
    SyntaxTree(const std::string& val) : value(val){}
};
class LL1
{
    symbol_t start_symbol;
    std::map<std::string, std::vector<alter_prod_t>> productions;
    std::set<std::string> nonterminals;
    std::set<std::string> terminals;
    std::map<std::string, std::set<std::string>> first_sets;
    std::map<std::string, std::set<std::string>> follow_sets;
    std::map<std::string, std::map<std::string, alter_prod_t>> parse_table;
    friend class LL1_constructor;
    void error_missing(const std::string& expected, int line)
    {
        std::cout << "语法错误,第" << line << "行,缺少\"" << expected << "\"\n";
    }
    void error_missing(const std::set<std::string>& expected_set, int line)
    {
        if (expected_set.empty())
            return;
        std::string expected;
        for (const auto& s : expected_set)
        {
            if (!expected.empty())
                expected += "或";
            expected += "\"" + s + "\"";
        }
        std::cout << "语法错误,第" << line << "行,缺少" << expected << "\n";
    }
    void error_illegal(const std::string& got, int line)
    {
        std::cout << "语法错误,第" << line << "行,非法符号\"" << got << "\"\n";
    }
public:
    SyntaxTree match(const std::string& input)
    {
        std::list<std::pair<std::string, int>> input_tokens;
        std::istringstream input_stream(input);
        std::string line;
        int line_number = 1;
        int last_matched_line = 1;
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

        std::stack<std::pair<symbol_t, std::shared_ptr<SyntaxTree>>> parse_stack;
        auto root = std::make_shared<SyntaxTree>(start_symbol.value);
        parse_stack.push({{true, "$"}, std::make_shared<SyntaxTree>("$")});
        parse_stack.push({start_symbol, root});

        auto lookahead = input_tokens.begin();
        while(!parse_stack.empty())
        {
            auto top = parse_stack.top();
            parse_stack.pop();

            if(top.first.is_terminal)
            {
                if (top.first.value == "E")
                {
                    continue;
                }
                if(lookahead -> first == top.first.value)
                {
                    // matched
                    last_matched_line = lookahead -> second;
                    lookahead++;
                    continue;
                }
                else
                {
                    if (top.first.value == "$")
                    {
                        break; // 强行结束，剩余输入全部认作非法符合
                    }
                    if (lookahead -> first == ")")
                    {
                        // 特判多余的右括号
                        error_illegal(lookahead -> first, lookahead -> second);
                        lookahead++;
                        parse_stack.push(top); // 重新期待当前符号
                    }
                    else
                    {
                        error_missing(top.first.value, last_matched_line);
                        continue;
                    }
                }
            }
            else // 非终结符
            {
                auto row = parse_table[top.first.value];
                auto col = row.find(lookahead -> first);
                if (col == row.end())
                {
                    // 没有找到表项
                    if (follow_sets[top.first.value].find(lookahead -> first) != follow_sets[top.first.value].end() ||
                        global_sync.find(lookahead -> first) != global_sync.end())
                    {
                        // 同步符号，弹出非终结符
                        if (nullable(productions[top.first.value]))
                        {
                            top.second -> children.push_back(std::make_shared<SyntaxTree>("E"));
                        }
                        else
                        {
                            // 非终结符不可推导出E
                            error_missing(first_sets[top.first.value], last_matched_line);
                            top.second -> children.push_back(std::make_shared<SyntaxTree>("E"));
                        }
                    }
                    else
                    {
                        // 非法符号，跳过输入符号
                        error_illegal(lookahead -> first, lookahead -> second);
                        lookahead++;
                        parse_stack.push(top); // 重新期待当前非终结符
                        continue;
                    }
                    continue;
                }
                else
                {
                    // 找到表项，展开产生式
                    auto prod = col -> second;
                    for (auto it = prod.rbegin(); it != prod.rend(); it++)
                    {
                        {
                            top.second -> children.push_back(std::make_shared<SyntaxTree>(it->value));
                            parse_stack.push({*it, top.second->children.back()});
                        }
                    }
                    top.second -> children.reverse();
                }
            }
        }
        while (lookahead != input_tokens.end())
        {
            error_illegal(lookahead -> first, lookahead -> second);
            lookahead++;
        }
        return *root;
    }
};
class LL1_constructor
{
    void set_nont_and_term()
    {
        nonterminals.clear();
        terminals.clear();
        for (const auto& prod: productions)
        {
            nonterminals.insert(prod.first);
        }
        for (auto& prod: productions)
        {
            for (auto& sym: prod.second)
            {
                for (auto& s: sym)
                {
                    if (nonterminals.find(s.value) == nonterminals.end())
                    {
                        // not found in nonterminals set
                        s.is_terminal = 1;
                        terminals.insert(s.value);
                    }
                    else
                    {
                        s.is_terminal = 0;
                        nonterminals.insert(s.value);
                    }
                }
            }
        }
    }
    void left_recursion_removal()
    {
    #ifndef ecnu_course
        auto is_epsilon_alt = [](const alter_prod_t& alt) -> bool {
            return alt.size() == 1 && alt.front().is_terminal && alt.front().value == "E";
        };
        auto starts_with_nonterminal = [](const alter_prod_t& alt, const std::string& nt) -> bool {
            if (alt.empty()) return false;
            const symbol_t& first = alt.front();
            return (!first.is_terminal) && first.value == nt;
        };
        auto make_unique_nonterminal = [&](const std::string& base) -> std::string {
            std::set<std::string> existing;
            for (const auto& p : productions) existing.insert(p.first);
            if (existing.find(base) == existing.end()) return base;
            for (int k = 1;; k++) {
                std::string cand = base + "_" + std::to_string(k);
                if (existing.find(cand) == existing.end()) return cand;
            }
        };
        const symbol_t EPS{true, "E"};

        // Order nonterminals by current production order
        for (size_t i = 0; i < productions.size(); i++)
        {
            // Indirect left recursion removal by substitution:
            // For each j < i, replace Ai -> Aj γ with Ai -> δ γ for each Aj -> δ
            for (size_t j = 0; j < i; j++)
            {
                const std::string& Aj = productions[j].left;
                production_t& AiProd = productions[i];
                std::vector<alter_prod_t> newRight;
                newRight.reserve(AiProd.right.size());
                for (const auto& alt : AiProd.right)
                {
                    if (!starts_with_nonterminal(alt, Aj))
                    {
                        newRight.push_back(alt);
                        continue;
                    }

                    // tail γ
                    alter_prod_t gamma = alt;
                    gamma.pop_front();

                    for (const auto& delta : productions[j].right)
                    {
                        alter_prod_t combined;
                        if (!is_epsilon_alt(delta))
                            combined = delta;
                        // append γ
                        combined.insert(combined.end(), gamma.begin(), gamma.end());
                        if (combined.empty()) combined.push_back(EPS);
                        newRight.push_back(std::move(combined));
                    }
                }
                AiProd.right = std::move(newRight);
            }

            // Direct left recursion elimination on Ai
            production_t& AiProd = productions[i];
            std::vector<alter_prod_t> alphas;
            std::vector<alter_prod_t> betas;
            for (const auto& alt : AiProd.right)
            {
                if (starts_with_nonterminal(alt, AiProd.left))
                    alphas.push_back(alt);
                else
                    betas.push_back(alt);
            }
            if (alphas.empty())
                continue;

            std::string newNt = make_unique_nonterminal(AiProd.left + "_LR");
            symbol_t newNtSym{false, newNt};

            // Ai -> beta Ai'
            std::vector<alter_prod_t> newAiRight;
            newAiRight.reserve(std::max<size_t>(1, betas.size()));
            if (betas.empty())
            {
                // If there are no betas, allow Ai to derive Ai' (so language isn't lost)
                alter_prod_t only;
                only.push_back(newNtSym);
                newAiRight.push_back(std::move(only));
            }
            else
            {
                for (const auto& beta : betas)
                {
                    alter_prod_t b = beta;
                    if (is_epsilon_alt(b)) b.clear();
                    b.push_back(newNtSym);
                    if (b.empty()) b.push_back(newNtSym);
                    newAiRight.push_back(std::move(b));
                }
            }
            AiProd.right = std::move(newAiRight);

            // Ai' -> alpha' Ai' | E
            production_t newProd;
            newProd.left = newNt;
            for (auto alpha : alphas)
            {
                // remove leading Ai
                alpha.pop_front();
                if (alpha.empty() || is_epsilon_alt(alpha))
                {
                    // Original rule like A->A or A->A E: the recursive part adds nothing; skip it.
                    continue;
                }
                alpha.push_back(newNtSym);
                newProd.right.push_back(std::move(alpha));
            }
            newProd.right.push_back(alter_prod_t{EPS});
            productions.push_back(std::move(newProd));
        }

        // Recompute terminal/nonterminal flags after introducing new nonterminals.
        set_nont_and_term();
        for (auto& prod : productions)
        {
            sort(prod.right.begin(), prod.right.end());
        }
    #endif
    }
    void left_factoring()
    {
    #ifndef ecnu_course
        bool changed = false;
        do {
            changed = false;
            // IMPORTANT: don't push_back into productions while range-iterating (iterator invalidation).
            std::vector<production_t> toAppend;
            for (size_t idx = 0; idx < productions.size(); idx++)
            {
                auto& prod = productions[idx];
                std::map<symbol_t, std::vector<alter_prod_t>> prefix_map;
                for (const auto& alt : prod.right)
                {
                    if (!alt.empty())
                    {
                        prefix_map[alt.front()].push_back(alt);
                    }
                }
                if (prefix_map.size() < prod.right.size())
                {
                    changed = true;
                    prod.right.clear();
                    for (const auto& kv : prefix_map)
                    {
                        const auto& prefix = kv.first;
                        const auto& alts = kv.second;
                        if (alts.size() > 1)
                        {
                            std::string new_nonterminal = prod.left + "_" + prefix.value;
                            alter_prod_t new_alt = {prefix, {false, new_nonterminal}};
                            prod.right.push_back(new_alt);

                            std::vector<alter_prod_t> new_prods;
                            for (const auto& alt : alts)
                            {
                                auto it = alt.begin();
                                if (it != alt.end()) ++it;
                                alter_prod_t suffix(it, alt.end());
                                if (suffix.empty()) suffix.push_back({true, "E"});
                                new_prods.push_back(std::move(suffix));
                            }
                            toAppend.push_back({new_nonterminal, std::move(new_prods)});
                        }
                        else
                        {
                            prod.right.push_back(alts[0]);
                        }
                    }
                }
            }
            if (!toAppend.empty())
            {
                productions.insert(productions.end(), toAppend.begin(), toAppend.end());
                set_nont_and_term();
            }
        } while (changed);
        for (auto& prod : productions)
        {
            sort(prod.right.begin(), prod.right.end());
        }
    #endif
    }
    void compute_first_sets()
    {
        for (const auto& nt : nonterminals) first_sets[nt] = {};
        for (const auto& t : terminals) first_sets[t] = {t};
        first_sets["E"] = {"E"};
        bool changed = true;
        while (changed) {
            changed = false;
            for (const auto& prods : productions) {
                const std::string& A = prods.first;
                for (const auto& prod : prods.second) {
                    if (prod.front() == symbol_t{true, "E"}) {
                        auto ins = first_sets[A].insert("E");
                        if (ins.second) changed = true;
                        continue;
                    }
                    bool all_nullable = true;
                    for (const auto& X : prod) {
                        if (X == symbol_t{true, "E"}) {
                            auto ins = first_sets[A].insert("E");
                            if (ins.second) changed = true;
                            continue;
                        }
                        join_set_except<std::string>(first_sets[A], first_sets[X.value], "E");
                        if (first_sets[X.value].find("E") == first_sets[X.value].end()) {
                            all_nullable = false;
                            break;
                        }
                    }
                    if (all_nullable) {
                        auto ins = first_sets[A].insert("E");
                        if (ins.second) changed = true;
                    }
                }
            }

            bool sizeChanged = false;
            for (const auto& kv : productions) {
                const std::string& A = kv.first;
                size_t before = first_sets[A].size();
                std::set<std::string> new_first;
                for (const auto& prod : kv.second) {
                    auto f = get_first_set(prod);
                    for (const auto& t : f) new_first.insert(t);
                }
                first_sets[A] = std::move(new_first);
                size_t after = first_sets[A].size();
                if (after != before) sizeChanged = true;
            }
            if (sizeChanged) changed = true;
        }
    }
    void compute_follow_sets()
    {
        for (const auto& nt : nonterminals) follow_sets[nt] = {};
        if (!productions.empty())
            follow_sets[productions.begin()->first].insert("$");

        bool fchanged = true;
        while (fchanged) {
            fchanged = false;
            for (const auto& prods : productions) {
                const std::string& A = prods.first;
                for (const auto& prod : prods.second) {
                    for (auto it = prod.begin(); it != prod.end(); it++) {
                        const std::string& B = it->value;
                        if (nonterminals.find(B) == nonterminals.end()) continue;
                        alter_prod_t beta(it, prod.end());
                        beta.pop_front(); // remove B
                        // FIRST(beta)
                        std::set<std::string> firstBeta;
                        if (beta.empty()) {
                            firstBeta.insert("E");
                        } else {
                            bool all_nullable = true;
                            for (const auto& X : beta) {
                                join_set_except<std::string>(firstBeta, first_sets[X.value], "E");
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
    std::map<std::string, std::map<std::string, alter_prod_t>> build_parse_table()
    {
        std::map<std::string, std::map<std::string, alter_prod_t>> parse_table;
        for (const auto& prod: productions)
        {
            for (const auto& alt: prod.second)
            {
                auto first_set = get_first_set(alt);
                for (const auto& term: first_set)
                {
                    if (term != "E")
                    {
                        if (!parse_table[prod.first][term].empty() && parse_table[prod.first][term] != alt)
                        {
                            std::cerr << "这不是一个合法的 LL1 文法，在 [" << prod.first << "][" << term << "] 出现了冲突\n";
                            exit(1);
                        }
                        parse_table[prod.first][term] = alt;
                    }
                    else
                    {
                        for (const auto& fol: follow_sets[prod.first])
                        {
                            if (!parse_table[prod.first][fol].empty() && parse_table[prod.first][fol] != alt)
                            {
                                std::cerr << "这不是一个合法的 LL1 文法，在 [" << prod.first << "][" << fol << "] 出现了冲突\n";
                                exit(1);
                            }
                            parse_table[prod.first][fol] = alt;
                        }
                    }
                }
            }
        }
        return parse_table;
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
            join_set_except<std::string>(out, first_sets[sym.value], "E");
            if (first_sets[sym.value].find("E") == first_sets[sym.value].end()) {
                all_nullable = false;
                break;
            }
        }
        if (all_nullable) out.insert("E");
        return out;
    }
    symbol_t start_symbol;
    std::map<std::string, std::vector<alter_prod_t>> productions;
    std::set<std::string> nonterminals;
    std::set<std::string> terminals;
    std::map<std::string, std::set<std::string>> first_sets;
    std::map<std::string, std::set<std::string>> follow_sets;
public:
    const LL1 construct(std::string& pro)
    {
        LL1 result;
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
                start_symbol = {false, the_left};
            std::list<std::string> right_parts;
            std::istringstream right_stream(the_right);
            std::string part;
            productions[the_left].push_back({});
            while (right_stream >> part)
            {
                if (part != "|")
                {
                    productions[the_left].back().push_back({1, part});
                }
                else
                {
                    productions[the_left].push_back({});
                }
            }
        }
        set_nont_and_term();
        left_factoring();
        left_recursion_removal();
        compute_first_sets();
        compute_follow_sets();

        result.parse_table = build_parse_table();
        result.start_symbol = start_symbol;
        result.productions = productions;
        result.nonterminals = nonterminals;
        result.terminals = terminals;
        result.first_sets = first_sets;
        result.follow_sets = follow_sets;
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
	LL1_constructor constructor;
	LL1 parser = constructor.construct(GRAMMER);
	auto syntax_tree = parser.match(prog);
	syntax_tree.print();
    /********* End *********/
	
}