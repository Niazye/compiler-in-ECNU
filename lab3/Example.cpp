#include <algorithm>
#include <cctype>
#include <deque>
#include <cstdlib>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

using namespace std;

struct Token {
    string sym;
    int line = 1;
    bool injected = false;
};

static string toLowerCopy(string s) {
    for (char& c : s) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    return s;
}

static string toUpperCopy(string s) {
    for (char& c : s) c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
    return s;
}

static string trimCopy(const string& s) {
    size_t b = 0;
    while (b < s.size() && isspace(static_cast<unsigned char>(s[b]))) b++;
    size_t e = s.size();
    while (e > b && isspace(static_cast<unsigned char>(s[e - 1]))) e--;
    return s.substr(b, e - b);
}

struct ProductionRule {
    string lhs;
    vector<string> rhs; // empty => epsilon
};

struct Grammar {
    string startSymbol;
    string augmentedStart;

    vector<ProductionRule> rules; // rules[0] will be S' -> start
    unordered_map<string, vector<int>> rulesByLhs;

    unordered_set<string> nonterminals;
    unordered_set<string> terminals; // includes $

    unordered_map<string, unordered_set<string>> first;

    unordered_map<string, string> keywordCanonLower; // lowercase keyword -> canonical
};

static bool isNonTerminal(const Grammar& g, const string& s) {
    return g.nonterminals.find(s) != g.nonterminals.end();
}

static bool isTerminal(const Grammar& g, const string& s) {
    return g.terminals.find(s) != g.terminals.end();
}

static string normalizeToken(const Grammar& g, const string& raw) {
    if (raw.empty()) return raw;

    if (g.terminals.find(raw) != g.terminals.end()) return raw;

    string up = toUpperCopy(raw);
    if (up == "ID") return "ID";
    if (up == "NUM") return "NUM";

    string low = toLowerCopy(raw);
    auto it = g.keywordCanonLower.find(low);
    if (it != g.keywordCanonLower.end()) return it->second;

    return raw;
}

static unordered_set<string> firstOfSequence(const Grammar& g, const vector<string>& seq) {
    unordered_set<string> out;
    if (seq.empty()) {
        out.insert("E");
        return out;
    }

    bool allNullable = true;
    for (const auto& sym : seq) {
        if (sym == "E") {
            out.insert("E");
            continue;
        }
        auto it = g.first.find(sym);
        if (it == g.first.end()) {
            // Treat unknown as terminal
            out.insert(sym);
            allNullable = false;
            break;
        }
        for (const auto& t : it->second) {
            if (t != "E") out.insert(t);
        }
        if (it->second.find("E") == it->second.end()) {
            allNullable = false;
            break;
        }
    }

    if (allNullable) out.insert("E");
    return out;
}

static Grammar buildGrammarFromString(const string& grammarText, const string& startSymbol) {
    Grammar g;
    g.startSymbol = startSymbol;

    // First pass: collect LHS as nonterminals
    {
        stringstream ss(grammarText);
        string line;
        while (getline(ss, line)) {
            string t = trimCopy(line);
            if (t.empty()) continue;
            if (!t.empty() && t[0] == '#') continue;
            size_t arrow = t.find("->");
            if (arrow == string::npos) continue;
            string lhs = trimCopy(t.substr(0, arrow));
            if (!lhs.empty()) g.nonterminals.insert(lhs);
        }
    }

    // Parse productions
    vector<pair<string, string>> rawRules;
    {
        stringstream ss(grammarText);
        string line;
        while (getline(ss, line)) {
            string t = trimCopy(line);
            if (t.empty()) continue;
            if (!t.empty() && t[0] == '#') continue;
            size_t arrow = t.find("->");
            if (arrow == string::npos) continue;
            string lhs = trimCopy(t.substr(0, arrow));
            string rhsAll = trimCopy(t.substr(arrow + 2));
            if (lhs.empty() || rhsAll.empty()) continue;
            rawRules.push_back({lhs, rhsAll});
        }
    }

    // Augmented start
    g.augmentedStart = startSymbol + "'";
    while (g.nonterminals.find(g.augmentedStart) != g.nonterminals.end()) {
        g.augmentedStart.push_back('\'');
    }
    g.nonterminals.insert(g.augmentedStart);

    // rules[0] = S' -> start
    g.rules.push_back(ProductionRule{g.augmentedStart, {g.startSymbol}});

    for (const auto& rr : rawRules) {
        const string& lhs = rr.first;
        string rhsAll = rr.second;

        // split by '|'
        vector<string> alts;
        size_t start = 0;
        while (true) {
            size_t bar = rhsAll.find('|', start);
            if (bar == string::npos) {
                alts.push_back(trimCopy(rhsAll.substr(start)));
                break;
            }
            alts.push_back(trimCopy(rhsAll.substr(start, bar - start)));
            start = bar + 1;
        }

        for (const auto& alt : alts) {
            if (alt == "E") {
                g.rules.push_back(ProductionRule{lhs, {}});
                continue;
            }
            stringstream rs(alt);
            string sym;
            vector<string> rhs;
            while (rs >> sym) rhs.push_back(sym);
            if (rhs.size() == 1 && rhs[0] == "E") rhs.clear();
            g.rules.push_back(ProductionRule{lhs, rhs});
        }
    }

    // Index rules by LHS
    for (int i = 0; i < static_cast<int>(g.rules.size()); i++) {
        g.rulesByLhs[g.rules[i].lhs].push_back(i);
    }

    // Infer terminals
    for (const auto& rule : g.rules) {
        for (const auto& sym : rule.rhs) {
            if (sym == "E") continue;
            if (!isNonTerminal(g, sym)) g.terminals.insert(sym);
        }
    }
    g.terminals.insert("$");

    // Build keyword canonical map (alpha-only terminals)
    for (const auto& t : g.terminals) {
        bool allAlpha = !t.empty();
        for (char c : t) {
            if (!isalpha(static_cast<unsigned char>(c))) {
                allAlpha = false;
                break;
            }
        }
        if (allAlpha) {
            g.keywordCanonLower[toLowerCopy(t)] = t;
        }
    }

    // FIRST init
    for (const auto& nt : g.nonterminals) g.first[nt] = {};
    for (const auto& t : g.terminals) g.first[t] = {t};

    // Compute FIRST for nonterminals
    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& rule : g.rules) {
            const string& A = rule.lhs;
            if (rule.rhs.empty()) {
                if (g.first[A].insert("E").second) changed = true;
                continue;
            }

            bool allNullable = true;
            for (const auto& X : rule.rhs) {
                if (X == "E") {
                    if (g.first[A].insert("E").second) changed = true;
                    continue;
                }

                auto it = g.first.find(X);
                if (it == g.first.end()) {
                    if (g.first[A].insert(X).second) changed = true;
                    allNullable = false;
                    break;
                }

                for (const auto& t : it->second) {
                    if (t == "E") continue;
                    if (g.first[A].insert(t).second) changed = true;
                }
                if (it->second.find("E") == it->second.end()) {
                    allNullable = false;
                    break;
                }
            }
            if (allNullable) {
                if (g.first[A].insert("E").second) changed = true;
            }
        }
    }

    return g;
}

struct Item {
    int ruleId;
    int dot;
    string la;
};

struct ItemLess {
    bool operator()(const Item& a, const Item& b) const {
        if (a.ruleId != b.ruleId) return a.ruleId < b.ruleId;
        if (a.dot != b.dot) return a.dot < b.dot;
        return a.la < b.la;
    }
};

static string itemSetKey(const set<Item, ItemLess>& I) {
    // deterministic serialization
    string out;
    for (const auto& it : I) {
        out += to_string(it.ruleId);
        out.push_back(':');
        out += to_string(it.dot);
        out.push_back(':');
        out += it.la;
        out.push_back(';');
    }
    return out;
}

static set<Item, ItemLess> closure(const Grammar& g, set<Item, ItemLess> I) {
    bool changed = true;
    while (changed) {
        changed = false;
        vector<Item> toAdd;

        for (const auto& item : I) {
            const auto& rule = g.rules[item.ruleId];
            if (item.dot < 0 || item.dot > static_cast<int>(rule.rhs.size())) continue;
            if (item.dot == static_cast<int>(rule.rhs.size())) continue;

            const string& B = rule.rhs[item.dot];
            if (!isNonTerminal(g, B)) continue;

            // beta a
            vector<string> beta;
            for (int k = item.dot + 1; k < static_cast<int>(rule.rhs.size()); k++) beta.push_back(rule.rhs[k]);
            beta.push_back(item.la);

            auto firstSet = firstOfSequence(g, beta);
            for (const auto& b : firstSet) {
                if (b == "E") continue;
                auto it = g.rulesByLhs.find(B);
                if (it == g.rulesByLhs.end()) continue;
                for (int rid : it->second) {
                    Item ni{rid, 0, b};
                    if (I.find(ni) == I.end()) {
                        toAdd.push_back(std::move(ni));
                    }
                }
            }
        }

        for (const auto& ni : toAdd) {
            if (I.insert(ni).second) changed = true;
        }
    }
    return I;
}

static set<Item, ItemLess> goTo(const Grammar& g, const set<Item, ItemLess>& I, const string& X) {
    set<Item, ItemLess> J;
    for (const auto& item : I) {
        const auto& rule = g.rules[item.ruleId];
        if (item.dot < static_cast<int>(rule.rhs.size()) && rule.rhs[item.dot] == X) {
            J.insert(Item{item.ruleId, item.dot + 1, item.la});
        }
    }
    if (J.empty()) return J;
    return closure(g, std::move(J));
}

enum class ActionKind { Error, Shift, Reduce, Accept };

struct Action {
    ActionKind kind = ActionKind::Error;
    int target = -1; // for shift: next state; for reduce: ruleId
};

struct LRTable {
    vector<unordered_map<string, Action>> action;
    vector<unordered_map<string, int>> goTo;
};

static LRTable buildLR1Table(const Grammar& g) {
    vector<set<Item, ItemLess>> C;
    unordered_map<string, int> index;

    // I0 = closure({ S' -> · S, $ })
    set<Item, ItemLess> I0;
    I0.insert(Item{0, 0, "$"});
    I0 = closure(g, std::move(I0));

    C.push_back(I0);
    index[itemSetKey(C[0])] = 0;

    deque<int> q;
    q.push_back(0);

    // Collect all grammar symbols that can appear in transitions
    vector<string> symbols;
    symbols.reserve(g.terminals.size() + g.nonterminals.size());
    for (const auto& t : g.terminals) {
        if (t == "E") continue;
        symbols.push_back(t);
    }
    for (const auto& nt : g.nonterminals) {
        symbols.push_back(nt);
    }

    vector<unordered_map<string, int>> trans; // state -> symbol -> next
    trans.resize(1);

    while (!q.empty()) {
        int i = q.front();
        q.pop_front();

        // Ensure trans size
        if (static_cast<int>(trans.size()) <= i) trans.resize(i + 1);

        for (const auto& X : symbols) {
            if (X == "E") continue;
            auto J = goTo(g, C[i], X);
            if (J.empty()) continue;

            string key = itemSetKey(J);
            auto it = index.find(key);
            int j;
            if (it == index.end()) {
                j = static_cast<int>(C.size());
                C.push_back(std::move(J));
                index[key] = j;
                q.push_back(j);
                trans.resize(C.size());
            } else {
                j = it->second;
            }
            trans[i][X] = j;
        }
    }

    LRTable t;
    t.action.resize(C.size());
    t.goTo.resize(C.size());

    for (int i = 0; i < static_cast<int>(C.size()); i++) {
        // SHIFT actions
        for (const auto& kv : trans[i]) {
            const string& X = kv.first;
            int j = kv.second;
            if (isTerminal(g, X) && X != "$") {
                t.action[i][X] = Action{ActionKind::Shift, j};
            }
            if (isNonTerminal(g, X)) {
                t.goTo[i][X] = j;
            }
        }

        // REDUCE / ACCEPT
        for (const auto& item : C[i]) {
            const auto& rule = g.rules[item.ruleId];
            if (item.dot != static_cast<int>(rule.rhs.size())) continue;

            if (rule.lhs == g.augmentedStart && item.la == "$") {
                t.action[i]["$"] = Action{ActionKind::Accept, -1};
                continue;
            }

            // reduce by ruleId on lookahead
            // If conflict, keep existing (grammar should be LR(1) for this task)
            auto& cell = t.action[i][item.la];
            if (cell.kind == ActionKind::Error) {
                cell = Action{ActionKind::Reduce, item.ruleId};
            }
        }

        // Also allow shift on $ if present
        auto itDollar = trans[i].find("$");
        if (itDollar != trans[i].end()) {
            if (t.action[i].find("$") == t.action[i].end() || t.action[i]["$"].kind == ActionKind::Error) {
                t.action[i]["$"] = Action{ActionKind::Shift, itDollar->second};
            }
        }
    }

    return t;
}

static void syntaxErrorMissing(const string& expected, int line) {
    // For combined expected sets like "\"(\"或\"ID\"或\"NUM\"" the expected string already
    // contains quotes and the 'or' separators. Match lab2 behavior by not wrapping it again.
    if (expected.find("或") != string::npos || (!expected.empty() && expected[0] == '"')) {
        cout << "语法错误,第" << line << "行,缺少" << expected << "\n";
        return;
    }
    cout << "语法错误,第" << line << "行,缺少\"" << expected << "\"\n";
}

static void syntaxErrorIllegal(const string& got, int line) {
    cout << "语法错误,第" << line << "行,非法符号\"" << got << "\"\n";
}

static string pickInsertionToken(const vector<string>& preferred, const unordered_set<string>& expected) {
    for (const auto& t : preferred) {
        if (expected.find(t) != expected.end()) return t;
    }
    return "";
}

static bool expectedHasAny(const unordered_set<string>& expected, const vector<string>& candidates) {
    for (const auto& c : candidates) {
        if (expected.find(c) != expected.end()) return true;
    }
    return false;
}

static string joinOrQuoted(const vector<string>& items) {
    if (items.empty()) return "";
    string out = "\"" + items[0] + "\"";
    for (size_t i = 1; i < items.size(); i++) {
        out += "或";
        out += "\"" + items[i] + "\"";
    }
    return out;
}

static string expectedAlternatives(const unordered_set<string>& expected, const vector<string>& orderedCandidates) {
    vector<string> present;
    present.reserve(orderedCandidates.size());
    for (const auto& c : orderedCandidates) {
        if (expected.find(c) != expected.end()) present.push_back(c);
    }
    return joinOrQuoted(present);
}

enum class MissingKind { Generic, ExprStart, BoolOp, StmtStart };

static string missingMessage(const unordered_set<string>& expected, MissingKind kind) {
    // Match ans order: ( ID NUM
    const vector<string> exprStart = {"(", "ID", "NUM"};
    const vector<string> boolops = {"<", ">", "<=", ">=", "=="};
    const vector<string> stmtStart = {"ID", "if", "while", "{"};

    if (kind == MissingKind::ExprStart) {
        string msg = expectedAlternatives(expected, exprStart);
        return msg.empty() ? string("\"(\"或\"ID\"或\"NUM\"") : msg;
    }
    if (kind == MissingKind::BoolOp) {
        string msg = expectedAlternatives(expected, boolops);
        return msg.empty() ? string("\"<\"或\">\"或\"<=\"或\">=\"或\"==\"") : msg;
    }

    if (kind == MissingKind::StmtStart) {
        string msg = expectedAlternatives(expected, stmtStart);
        return msg.empty() ? string("\"ID\"或\"if\"或\"while\"或\"{\"") : msg;
    }

    // Generic: keep it short; if there are too many, just show a few common ones.
    // Prefer structural tokens first.
    const vector<string> structural = {";", ")", "}", "else", "then", "{", "$"};
    string msg = expectedAlternatives(expected, structural);
    if (!msg.empty()) return msg;

    // Fall back to one arbitrary expected token if present.
    if (!expected.empty()) return string("\"") + *expected.begin() + "\"";
    return "";
}

static vector<string> buildPreferredInsertions(const unordered_set<string>& expected) {
    // Keep this small to avoid inserting unrelated tokens (which causes confusing cascades).
    vector<string> preferred;

    // Statement starters first.
    const vector<string> stmtStart = {"ID", "if", "while", "{"};
    for (const auto& t : stmtStart) {
        if (expected.find(t) != expected.end()) preferred.push_back(t);
    }

    // For assignment/expression holes, inserting NUM is often less misleading than inserting ID.
    const vector<string> exprStart = {"NUM", "ID", "("};
    for (const auto& t : exprStart) {
        if (expected.find(t) != expected.end()) preferred.push_back(t);
    }

    const vector<string> boolops = {"<", "==", ">", "<=", ">="};
    for (const auto& t : boolops) {
        if (expected.find(t) != expected.end()) preferred.push_back(t);
    }

    const vector<string> structural = {";", ")", "}", "else", "then", "{"};
    for (const auto& t : structural) {
        if (expected.find(t) != expected.end()) preferred.push_back(t);
    }

    return preferred;
}

static string joinSymbols(const vector<string>& syms) {
    if (syms.empty()) return "";
    string out = syms[0];
    for (size_t i = 1; i < syms.size(); i++) {
        out += ' ';
        out += syms[i];
    }
    return out;
}

static string sanitizeLine(string s) {
    // PowerShell piping to native exe can produce UTF-16LE bytes (NUL between chars).
    // Remove NULs and common BOM patterns so tokenization stays stable.
    s.erase(remove(s.begin(), s.end(), '\0'), s.end());
    if (s.size() >= 3 && static_cast<unsigned char>(s[0]) == 0xEF && static_cast<unsigned char>(s[1]) == 0xBB &&
        static_cast<unsigned char>(s[2]) == 0xBF) {
        s = s.substr(3);
    }
    if (s.size() >= 2) {
        unsigned char b0 = static_cast<unsigned char>(s[0]);
        unsigned char b1 = static_cast<unsigned char>(s[1]);
        if ((b0 == 0xFF && b1 == 0xFE) || (b0 == 0xFE && b1 == 0xFF)) {
            s = s.substr(2);
        }
    }
    return s;
}

static void printRightmostDerivation(const Grammar& g, const vector<int>& reductions) {
    // reductions are in reduce-time order; reverse gives rightmost derivation production order
    vector<int> order = reductions;
    reverse(order.begin(), order.end());

    vector<vector<string>> forms;
    forms.push_back(vector<string>{g.startSymbol});

    for (int rid : order) {
        const auto& rule = g.rules[rid];
        const auto& sentential = forms.back();

        int pos = -1;
        for (int i = static_cast<int>(sentential.size()) - 1; i >= 0; i--) {
            if (sentential[i] == rule.lhs) {
                pos = i;
                break;
            }
        }
        if (pos == -1) {
            // Should not happen for successful parse; keep current form
            continue;
        }

        vector<string> next;
        next.reserve(sentential.size() - 1 + rule.rhs.size());
        for (int i = 0; i < pos; i++) next.push_back(sentential[i]);
        for (const auto& s : rule.rhs) {
            if (s != "E") next.push_back(s);
        }
        for (int i = pos + 1; i < static_cast<int>(sentential.size()); i++) next.push_back(sentential[i]);
        forms.push_back(std::move(next));
    }

    // Output format per requirement: each line prints the current sentential form,
    // followed by "=>" (with a space after it). The last line has no "=>".
    for (size_t i = 0; i < forms.size(); i++) {
        cout << joinSymbols(forms[i]);
        if (i + 1 < forms.size()) {
            cout << " => \n";
        } else {
            cout << "\n";
        }
    }
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    const bool debug = (getenv("LR_DEBUG") != nullptr);

    // Grammar (E = epsilon)
    const string grammarText =
        "program -> compoundstmt\n"
        "stmt -> ifstmt | whilestmt | assgstmt | compoundstmt\n"
        "compoundstmt -> { stmts }\n"
        "stmts -> stmt stmts | E\n"
        "ifstmt -> if ( boolexpr ) then stmt else stmt\n"
        "whilestmt -> while ( boolexpr ) stmt\n"
        "assgstmt -> ID = arithexpr ;\n"
        "boolexpr -> arithexpr boolop arithexpr\n"
        "boolop -> < | > | <= | >= | ==\n"
        "arithexpr -> multexpr arithexprprime\n"
        "arithexprprime -> + multexpr arithexprprime | - multexpr arithexprprime | E\n"
        "multexpr -> simpleexpr multexprprime\n"
        "multexprprime -> * simpleexpr multexprprime | / simpleexpr multexprprime | E\n"
        "simpleexpr -> ID | NUM | ( arithexpr )\n";

    Grammar g = buildGrammarFromString(grammarText, "program");
    LRTable table = buildLR1Table(g);

    if (debug) {
        cerr << "[debug] terminals size=" << g.terminals.size() << "\n";
        cerr << "[debug] has '{'=" << (g.terminals.find("{") != g.terminals.end())
             << ", '}'=" << (g.terminals.find("}") != g.terminals.end())
             << ", '='=" << (g.terminals.find("=") != g.terminals.end())
             << ", ';'=" << (g.terminals.find(";") != g.terminals.end())
             << ", 'if'=" << (g.terminals.find("if") != g.terminals.end())
             << ", 'ID'=" << (g.terminals.find("ID") != g.terminals.end())
             << "\n";
        int acceptStates = 0;
        for (size_t i = 0; i < table.action.size(); i++) {
            auto it = table.action[i].find("$");
            if (it != table.action[i].end() && it->second.kind == ActionKind::Accept) acceptStates++;
        }
        cerr << "[debug] states=" << table.action.size() << ", acceptStates=" << acceptStates << "\n";
    }

    // Read tokens: each line's tokens separated by spaces; line number = non-empty input line index
    vector<Token> rawTokens;
    string line;
    int logicalLineNo = 0;
    while (getline(cin, line)) {
        line = sanitizeLine(std::move(line));
        stringstream ss(line);
        string raw;
        vector<string> toks;
        while (ss >> raw) toks.push_back(raw);
        if (toks.empty()) continue;
        logicalLineNo++;
        for (const auto& tk : toks) {
            rawTokens.push_back(Token{normalizeToken(g, tk), logicalLineNo, false});
        }
    }
    if (logicalLineNo == 0) logicalLineNo = 1;
    rawTokens.push_back(Token{"$", logicalLineNo, false});

    // Injected token queue for error recovery insertions
    deque<Token> injected;

    int lastRealLine = 1;

    auto peekToken = [&](size_t idx) -> Token {
        if (!injected.empty()) return injected.front();
        if (idx < rawTokens.size()) return rawTokens[idx];
        return Token{"$", logicalLineNo, false};
    };

    auto consumeToken = [&](size_t& idx) {
        if (!injected.empty()) {
            injected.pop_front();
        } else {
            if (idx < rawTokens.size()) {
                lastRealLine = rawTokens[idx].line;
                idx++;
            }
        }
    };

    // LR(1) parse
    vector<int> stateStack;
    vector<string> symStack;
    stateStack.push_back(0);

    vector<int> reductions;

    size_t pos = 0;

    // Prevent infinite "insert expected token" loops on LR error recovery.
    // Keyed by (state, insertedToken, lookaheadToken, lookaheadLine)
    map<tuple<int, string, string, int>, int> insertAttempts;

    // Avoid infinite loops on repeated insertion at same state
    map<pair<int, string>, int> insertionCount;

    // Avoid spamming the same kind of "missing" on the same line.
    map<pair<int, int>, int> missingReportCount; // (line, MissingKind)

    while (true) {
        int st = stateStack.back();
        Token laTok = peekToken(pos);
        string la = laTok.sym;

        // Unknown token not in grammar terminals: treat as illegal
        if (!isTerminal(g, la) && la != "$") {
            syntaxErrorIllegal(la, laTok.line);
            consumeToken(pos);
            continue;
        }

        Action act;
        auto it = table.action[st].find(la);
        if (it != table.action[st].end()) act = it->second;

        if (debug) {
            const char* k = "ERR";
            if (act.kind == ActionKind::Shift) k = "SHIFT";
            else if (act.kind == ActionKind::Reduce) k = "REDUCE";
            else if (act.kind == ActionKind::Accept) k = "ACCEPT";
            cerr << "[debug] st=" << st << " la=" << la << " act=" << k;
            if (act.kind == ActionKind::Shift || act.kind == ActionKind::Reduce) cerr << "(" << act.target << ")";
            cerr << "\n";
        }

        if (act.kind == ActionKind::Shift) {
            symStack.push_back(la);
            stateStack.push_back(act.target);
            consumeToken(pos);
            continue;
        }

        if (act.kind == ActionKind::Reduce) {
            const auto& rule = g.rules[act.target];
            int popCount = static_cast<int>(rule.rhs.size());
            // epsilon production => pop 0
            if (rule.rhs.empty()) popCount = 0;

            for (int i = 0; i < popCount; i++) {
                if (!symStack.empty()) symStack.pop_back();
                if (stateStack.size() > 1) stateStack.pop_back();
            }

            int st2 = stateStack.back();
            auto gtIt = table.goTo[st2].find(rule.lhs);
            if (gtIt == table.goTo[st2].end()) {
                // Should not happen; try to recover by discarding lookahead
                if (la == "$") break;
                syntaxErrorIllegal(la, laTok.line);
                consumeToken(pos);
                continue;
            }

            symStack.push_back(rule.lhs);
            stateStack.push_back(gtIt->second);
            reductions.push_back(act.target);
            continue;
        }

        if (act.kind == ActionKind::Accept) {
            break;
        }

        // Error recovery (align with lab2 style: prefer high-level starters; sync on structural tokens)
        unordered_set<string> expectedSet;
        for (const auto& kv : table.action[st]) {
            if (kv.second.kind != ActionKind::Error) expectedSet.insert(kv.first);
        }

        if (expectedSet.empty()) {
            if (la == "$") break;
            syntaxErrorIllegal(la, laTok.line);
            consumeToken(pos);
            continue;
        }

        // If we only expect end-of-input but still have tokens, they are illegal (do not insert '$').
        if (expectedSet.size() == 1 && expectedSet.find("$") != expectedSet.end() && la != "$") {
            syntaxErrorIllegal(la, laTok.line);
            consumeToken(pos);
            continue;
        }

        const vector<string> exprStart = {"(", "ID", "NUM"};
        const vector<string> stmtStart = {"ID", "if", "while", "{"};
        const unordered_set<string> syncTokens = {")", ";", "}", "else", "then", "$"};

        const bool expectExprStart = expectedHasAny(expectedSet, exprStart);
        const bool expectStmtStart = expectedHasAny(expectedSet, stmtStart);

        // Special: after 'else', match lab2 by skipping junk tokens until '}'
        // and then reporting a missing statement starter.
        if (!symStack.empty() && symStack.back() == "else") {
            if (la == "}") {
                int reportLine = laTok.line;
                int& rep = missingReportCount[{reportLine, static_cast<int>(MissingKind::StmtStart)}];
                if (rep == 0) {
                    syntaxErrorMissing(missingMessage(expectedSet, MissingKind::StmtStart), reportLine);
                }
                rep++;

                // Prefer skipping the whole missing stmt and syncing on '}' (like lab2 output uses E).
                while (stateStack.size() > 1) {
                    int cur = stateStack.back();
                    auto it2 = table.action[cur].find(la);
                    if (it2 != table.action[cur].end() && it2->second.kind != ActionKind::Error) break;
                    stateStack.pop_back();
                    if (!symStack.empty()) symStack.pop_back();
                }

                int cur = stateStack.back();
                auto it2 = table.action[cur].find(la);
                if (it2 != table.action[cur].end() && it2->second.kind != ActionKind::Error) {
                    continue;
                }

                // Fallback: insert '{' so an empty compoundstmt can close immediately on the real '}'.
                if (expectedSet.find("{") != expectedSet.end()) {
                    injected.push_front(Token{"{", laTok.line, true});
                }
                continue;
            }
            if (la == "$") {
                int reportLine = laTok.line;
                int& rep = missingReportCount[{reportLine, static_cast<int>(MissingKind::StmtStart)}];
                if (rep == 0) {
                    syntaxErrorMissing(missingMessage(expectedSet, MissingKind::StmtStart), reportLine);
                }
                rep++;
                break;
            }
            syntaxErrorIllegal(la, laTok.line);
            consumeToken(pos);
            continue;
        }

        // Case A: Expression missing (e.g., "if ( )" or "ID = ;")
        // Match lab2 by reporting the combined expected-set message.
        if (expectExprStart) {
            int reportLine = laTok.line;
            int& rep = missingReportCount[{reportLine, static_cast<int>(MissingKind::ExprStart)}];
            if (rep == 0) {
                syntaxErrorMissing(missingMessage(expectedSet, MissingKind::ExprStart), reportLine);
            }
            rep++;

            // Special cases to closely match lab2 behavior:
            // - if ( ) : treat boolexpr as a fabricated minimal pattern so ')' can be consumed without extra errors
            // - ID = ; : fabricate a minimal arithexpr before ';'
            if (la == ")") {
                // Inject: ID < ID
                injected.push_front(Token{"ID", laTok.line, true});
                injected.push_front(Token{"<", laTok.line, true});
                injected.push_front(Token{"ID", laTok.line, true});
                continue;
            }
            if (la == ";") {
                // Inject: NUM
                injected.push_front(Token{"NUM", laTok.line, true});
                continue;
            }

            // Fallback: insert one expression starter.
            string insTok = pickInsertionToken({"ID", "NUM", "("}, expectedSet);
            if (insTok.empty()) insTok = "ID";
            auto key = make_tuple(st, insTok, la, laTok.line);
            int& tries = insertAttempts[key];
            tries++;
            if (tries <= 2) {
                injected.push_front(Token{insTok, laTok.line, true});
                continue;
            }

            if (la == "$") break;
            syntaxErrorIllegal(la, laTok.line);
            consumeToken(pos);
            continue;
        }

        // Case B: Statement missing. For input6, match lab2 by skipping illegal tokens until '}',
        // then reporting the combined stmtStart and inserting '{' so an empty compoundstmt can close on '}'.
        if (expectStmtStart && expectedSet.find(la) == expectedSet.end()) {
            if (la == "}") {
                int reportLine = laTok.line;
                int& rep = missingReportCount[{reportLine, static_cast<int>(MissingKind::StmtStart)}];
                if (rep == 0) {
                    syntaxErrorMissing(missingMessage(expectedSet, MissingKind::StmtStart), reportLine);
                }
                rep++;

                string insTok = (expectedSet.find("{") != expectedSet.end()) ? string("{") : string("ID");
                injected.push_front(Token{insTok, laTok.line, true});
                continue;
            }

            if (la == "$") {
                int reportLine = laTok.line;
                int& rep = missingReportCount[{reportLine, static_cast<int>(MissingKind::StmtStart)}];
                if (rep == 0) {
                    syntaxErrorMissing(missingMessage(expectedSet, MissingKind::StmtStart), reportLine);
                }
                rep++;
                break;
            }

            // Otherwise: illegal token in statement position, skip it.
            syntaxErrorIllegal(la, laTok.line);
            consumeToken(pos);
            continue;
        }

        // Extra ')' -> discard it (keep this after expr/stmt special cases).
        if (la == ")" && expectedSet.find(")") == expectedSet.end()) {
            syntaxErrorIllegal(la, laTok.line);
            consumeToken(pos);
            continue;
        }

        // Generic fallback: pick a conservative insertion to avoid cascades.
        vector<string> preferred = buildPreferredInsertions(expectedSet);
        string insTok = pickInsertionToken(preferred, expectedSet);
        if (insTok.empty()) insTok = *expectedSet.begin();

        int reportLine = laTok.line;
        if (insTok == ";" && lastRealLine > 0) reportLine = lastRealLine;

        auto key = make_tuple(st, insTok, la, laTok.line);
        int& tries = insertAttempts[key];
        tries++;
        if (tries <= 2) {
            syntaxErrorMissing(insTok, reportLine);
            injected.push_front(Token{insTok, laTok.line, true});
            continue;
        }

        // Too many failed insertions at the same point: fall back to discarding the lookahead.
        if (la == "$") break;
        syntaxErrorIllegal(la, laTok.line);
        consumeToken(pos);
        continue;
    }

    // Output rightmost derivation for the (recovered) program
    printRightmostDerivation(g, reductions);
    return 0;
}
