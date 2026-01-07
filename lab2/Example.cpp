#include <algorithm>
#include <cctype>
#include <iostream>
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
};

struct Node {
	string sym;
	vector<Node*> children;
	explicit Node(string s) : sym(std::move(s)) {}
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

using Production = vector<string>; // empty => epsilon (E)

struct Grammar {
	string startSymbol;
	unordered_set<string> nonterminals;
	unordered_set<string> terminals; // includes "$", excludes epsilon "E"
	unordered_map<string, vector<Production>> prods; // A -> list of alternatives
	unordered_map<string, unordered_set<string>> first;
	unordered_map<string, unordered_set<string>> follow;
	unordered_map<string, unordered_map<string, Production>> table; // M[A][a]
	unordered_map<string, string> keywordCanonLower; // lowercase -> canonical terminal
};

static bool isTerminal(const Grammar& g, const string& s) {
	return g.terminals.find(s) != g.terminals.end();
}

static void printTree(const Node* node, int depth) {
	for (int i = 0; i < depth; i++) cout << '\t';
	cout << node->sym << "\n";
	for (const Node* ch : node->children) {
		printTree(ch, depth + 1);
	}
}

static void freeTree(Node* node) {
	for (Node* ch : node->children) freeTree(ch);
	delete node;
}

static void syntaxErrorMissing(const string& expected, int line) {
	cout << "语法错误,第" << line << "行,缺少\"" << expected << "\"\n";
}

static void syntaxErrorIllegal(const string& got, int line) {
	cout << "语法错误,第" << line << "行,非法符号\"" << got << "\"\n";
}

// Normalize input token (keywords case-insensitive; ID/NUM case-insensitive)
// Generic: keyword list comes from grammar terminals.
static string normalizeToken(const Grammar& g, const string& raw) {
	if (raw.empty()) return raw;

	// If it's already a known terminal (punctuation/operators), keep as-is.
	if (g.terminals.find(raw) != g.terminals.end()) return raw;

	string up = toUpperCopy(raw);
	if (up == "ID") return "ID";
	if (up == "NUM") return "NUM";

	string low = toLowerCopy(raw);
	auto it = g.keywordCanonLower.find(low);
	if (it != g.keywordCanonLower.end()) return it->second;

	// Unknown token: keep original for better error message
	return raw;
}

static bool addAll(unordered_set<string>& dst, const unordered_set<string>& src, const string& skip = "") {
	bool changed = false;
	for (const auto& x : src) {
		if (!skip.empty() && x == skip) continue;
		auto ins = dst.insert(x);
		if (ins.second) changed = true;
	}
	return changed;
}

static unordered_set<string> firstOfSequence(const Grammar& g, const Production& seq) {
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
		if (it != g.first.end()) {
			for (const auto& t : it->second) {
				if (t != "E") out.insert(t);
			}
			if (it->second.find("E") == it->second.end()) {
				allNullable = false;
				break;
			}
		} else {
			// Unknown symbol treated as terminal
			out.insert(sym);
			allNullable = false;
			break;
		}
	}
	if (allNullable) out.insert("E");
	return out;
}

static bool isNullableNonTerminal(const Grammar& g, const string& nt) {
	auto it = g.first.find(nt);
	return it != g.first.end() && it->second.find("E") != it->second.end();
}

static string joinExpected(const vector<string>& preferredOrder, const unordered_set<string>& setVals) {
	vector<string> parts;
	parts.reserve(setVals.size());
	for (const auto& tok : preferredOrder) {
		if (setVals.find(tok) != setVals.end()) parts.push_back(tok);
	}
	// Add any remaining tokens (stable-ish fallback by lexical order)
	vector<string> rest;
	for (const auto& t : setVals) {
		if (find(parts.begin(), parts.end(), t) == parts.end()) rest.push_back(t);
	}
	sort(rest.begin(), rest.end());
	for (const auto& t : rest) parts.push_back(t);
	if (parts.empty()) return "ID";
	string out = parts[0];
	for (size_t i = 1; i < parts.size(); i++) out += "或" + parts[i];
	return out;
}

// Pick expected starting terminals from FIRST(nt) for better error messages.
static string expectedStartTokenForNonTerminal(const Grammar& g, const string& nt) {
	static const vector<string> preferred = {
		"if", "while", "ID", "NUM", "(", "{",
		"<", ">", "<=", ">=", "==",
		"then", "else",
		"+", "-", "*", "/",
		"}", ")", ";", "="
	};
	auto it = g.first.find(nt);
	if (it == g.first.end()) return "ID";
	unordered_set<string> starters;
	for (const auto& t : it->second) {
		if (t != "E") starters.insert(t);
	}
	return joinExpected(preferred, starters);
}

static Grammar buildGrammarFromString(const string& grammarText, const string& startSymbol) {
	Grammar g;
	g.startSymbol = startSymbol;

	// Parse productions
	stringstream ss(grammarText);
	string line;
	vector<pair<string, string>> rawRules;
	while (std::getline(ss, line)) {
		string t = trimCopy(line);
		if (t.empty()) continue;
		// allow comment lines starting with #
		if (!t.empty() && t[0] == '#') continue;
		size_t arrow = t.find("->");
		if (arrow == string::npos) continue;
		string lhs = trimCopy(t.substr(0, arrow));
		string rhsAll = trimCopy(t.substr(arrow + 2));
		if (lhs.empty() || rhsAll.empty()) continue;
		rawRules.push_back({lhs, rhsAll});
		g.nonterminals.insert(lhs);
	}

	for (const auto& r : rawRules) {
		const string& lhs = r.first;
		string rhsAll = r.second;
		// split alternatives by '|'
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
				g.prods[lhs].push_back(Production{});
				continue;
			}
			stringstream rs(alt);
			string sym;
			Production prod;
			while (rs >> sym) prod.push_back(sym);
			if (prod.size() == 1 && prod[0] == "E") prod.clear();
			g.prods[lhs].push_back(prod);
		}
	}

	// Infer terminals from RHS symbols not in nonterminals, excluding epsilon.
	for (const auto& kv : g.prods) {
		for (const auto& prod : kv.second) {
			for (const auto& sym : prod) {
				if (sym == "E") continue;
				if (g.nonterminals.find(sym) == g.nonterminals.end()) {
					g.terminals.insert(sym);
				}
			}
		}
	}
	g.terminals.insert("$");

	// Build keyword canonical map (letters-only terminals, case-insensitive)
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
	// Always support ID/NUM case-insensitive
	// (handled explicitly in normalizeToken)

	// FIRST sets init
	for (const auto& nt : g.nonterminals) g.first[nt] = {};
	for (const auto& t : g.terminals) g.first[t] = {t};

	bool changed = true;
	while (changed) {
		changed = false;
		for (const auto& kv : g.prods) {
			const string& A = kv.first;
			for (const auto& prod : kv.second) {
				if (prod.empty()) {
					auto ins = g.first[A].insert("E");
					if (ins.second) changed = true;
					continue;
				}
				bool allNullable = true;
				for (const auto& X : prod) {
					if (X == "E") {
						auto ins = g.first[A].insert("E");
						if (ins.second) changed = true;
						continue;
					}
					addAll(g.first[A], g.first[X], "E");                                       
					// track changes from addAll
					// (re-check by size difference)
					// We'll do a direct check by insertion in addAll isn't exposed; use size snapshot.
					// Simplify: compute again with size snapshot.
					//
					if (g.first[X].find("E") == g.first[X].end()) {
						allNullable = false;
						break;
					}
				}
				if (allNullable) {
					auto ins = g.first[A].insert("E");
					if (ins.second) changed = true;
				}
			}
		}

		// The above used addAll without change feedback; conservatively recompute changed by another pass
		// using size snapshots for nonterminals.
		// To keep it C++14-simple and correct, we just loop until no sizes change.
		// (If no insertion happened, it stabilizes quickly.)
		//
		// This is handled by setting changed only on explicit insertions of E. We also need to detect
		// non-E insertions. We'll do a second pass with size snapshots.
		bool sizeChanged = false;
		for (const auto& kv : g.prods) {
			const string& A = kv.first;
			size_t before = g.first[A].size();
			// rebuild would be heavy; instead, we rely on addAll having inserted. Detect by comparing with
			// a fresh recomputation of FIRST(A) for this production set.
			unordered_set<string> newFirst;
			for (const auto& prod : kv.second) {
				auto f = firstOfSequence(g, prod);
				for (const auto& t : f) newFirst.insert(t);
			}
			g.first[A] = std::move(newFirst);
			size_t after = g.first[A].size();
			if (after != before) sizeChanged = true;
		}
		if (sizeChanged) changed = true;
	}

	// FOLLOW init
	for (const auto& nt : g.nonterminals) g.follow[nt] = {};
	g.follow[g.startSymbol].insert("$");

	bool fchanged = true;
	while (fchanged) {
		fchanged = false;
		for (const auto& kv : g.prods) {
			const string& A = kv.first;
			for (const auto& prod : kv.second) {
				for (size_t i = 0; i < prod.size(); i++) {
					const string& B = prod[i];
					if (g.nonterminals.find(B) == g.nonterminals.end()) continue;
					Production beta;
					for (size_t j = i + 1; j < prod.size(); j++) beta.push_back(prod[j]);
					auto firstBeta = firstOfSequence(g, beta);
					// FIRST(beta) - E
					for (const auto& t : firstBeta) {
						if (t == "E") continue;
						auto ins = g.follow[B].insert(t);
						if (ins.second) fchanged = true;
					}
					if (beta.empty() || firstBeta.find("E") != firstBeta.end()) {
						for (const auto& t : g.follow[A]) {
							auto ins = g.follow[B].insert(t);
							if (ins.second) fchanged = true;
						}
					}
				}
			}
		}
	}

	// Build LL(1) parse table
	for (const auto& nt : g.nonterminals) g.table[nt] = {};
	for (const auto& kv : g.prods) {
		const string& A = kv.first;
		for (const auto& prod : kv.second) {
			auto f = firstOfSequence(g, prod);
			for (const auto& a : f) {
				if (a == "E") continue;
				if (g.table[A].find(a) == g.table[A].end()) {
					g.table[A][a] = prod;
				}
			}
			if (f.find("E") != f.end()) {
				for (const auto& b : g.follow[A]) {
					if (g.table[A].find(b) == g.table[A].end()) {
						g.table[A][b] = Production{}; // epsilon
					}
				}
			}
		}
	}

	return g;
}

int main() {
	ios::sync_with_stdio(false);
	cin.tie(nullptr);

	// ========================
	// 自定义文法：你只需要手动修改这一段字符串即可更换文法（格式与题目一致）。
	// 约定：E 表示空产生式。
	// ========================
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

	vector<Token> tokens;
	string line;
	int logicalLineNo = 0;
	while (std::getline(cin, line)) {
		std::stringstream ss(line);
		string raw;
		vector<string> rawTokens;
		while (ss >> raw) rawTokens.push_back(std::move(raw));
		if (rawTokens.empty()) continue; // ignore blank lines for line numbering (match judge)
		logicalLineNo++;
		for (const string& tok : rawTokens) {
			Token t;
			t.sym = normalizeToken(g, tok);
			t.line = logicalLineNo;
			tokens.push_back(std::move(t));
		}
	}
	if (logicalLineNo == 0) logicalLineNo = 1;
	tokens.push_back(Token{"$", logicalLineNo});

	// Predictive parsing with explicit stack + parse tree construction
	Node* root = new Node(g.startSymbol);
	struct StackItem {
		string sym;
		Node* node;
	};
	vector<StackItem> st;
	st.push_back(StackItem{"$", nullptr});
	st.push_back(StackItem{g.startSymbol, root});

	size_t pos = 0;
	auto lookaheadSym = [&]() -> string {
		if (pos >= tokens.size()) return "$";
		return tokens[pos].sym;
	};
	auto lookaheadLine = [&]() -> int {
		if (pos >= tokens.size()) return logicalLineNo;
		return tokens[pos].line;
	};

	while (!st.empty()) {
		StackItem top = st.back();
		st.pop_back();
		const string la = lookaheadSym();

		if (top.sym == "$" && la == "$") {
			break; // accept
		}

		if (isTerminal(g, top.sym) || top.sym == "$") {
			if (top.sym == la) {
				pos++;
			} else {
				// Heuristic: if we see an extra ')', discard it and retry the same expected terminal.
				// This reduces cascaded errors for inputs like: ID = ( NUM ) ) ;
				if (la == ")" && top.sym != ")" && la != "$") {
					syntaxErrorIllegal(la, lookaheadLine());
					pos++;           // discard spurious ')'
					st.push_back(top); // retry expected terminal
				} else {
					// insertion strategy: assume missing terminal
					syntaxErrorMissing(top.sym, lookaheadLine());
				}
			}
			continue;
		}

		// Non-terminal
		auto rowIt = g.table.find(top.sym);
		bool hasEntry = false;
		Production prod;
		if (rowIt != g.table.end()) {
			auto colIt = rowIt->second.find(la);
			if (colIt != rowIt->second.end()) {
				hasEntry = true;
				prod = colIt->second;
			}
		}

		if (!hasEntry) {
			// panic-mode: prefer synchronizing on FOLLOW(nt) and a small global sync set
			static const unordered_set<string> globalSync = {"}", ")", "else", "$"};
			auto folIt = g.follow.find(top.sym);
			bool inFollow = (folIt != g.follow.end() && folIt->second.find(la) != folIt->second.end());
			bool inGlobal = (globalSync.find(la) != globalSync.end());
			if (inFollow || inGlobal || la == "$") {
				// If this non-terminal can't be empty, emit a missing-start error, then pop anyway.
				if (!isNullableNonTerminal(g, top.sym)) {
					syntaxErrorMissing(expectedStartTokenForNonTerminal(g, top.sym), lookaheadLine());
				}
				if (top.node) top.node->children.push_back(new Node("E"));
			} else {
				// Discard unexpected token, retry same non-terminal.
				syntaxErrorIllegal(la, lookaheadLine());
				pos++;
				st.push_back(top);
			}
			continue;
		}

		// Expand production: attach children in order
		if (top.node == nullptr) {
			// Shouldn't happen for non-terminals, but guard anyway
		}

		if (prod.empty()) {
			if (top.node) top.node->children.push_back(new Node("E"));
			continue;
		}

		vector<Node*> childNodes;
		childNodes.reserve(prod.size());
		for (const string& s : prod) {
			Node* child = new Node(s);
			childNodes.push_back(child);
		}
		if (top.node) {
			for (Node* child : childNodes) top.node->children.push_back(child);
		}

		// Push to stack in reverse order
		for (int i = static_cast<int>(prod.size()) - 1; i >= 0; i--) {
			const string& s = prod[i];
			if (s == "E") continue;
			st.push_back(StackItem{s, childNodes[i]});
		}
	}

	// If extra tokens remain (before $), discard with errors
	while (pos < tokens.size() && tokens[pos].sym != "$") {
		syntaxErrorIllegal(tokens[pos].sym, tokens[pos].line);
		pos++;
	}

	printTree(root, 0);
	freeTree(root);
	return 0;
}

