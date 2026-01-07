#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace std;

struct Token {
    string type;   // keywords/operators/punctuations or ID/INTNUM/REALNUM or EOF
    string lexeme; // raw text
    int line = 1;
};

enum class ValType { Int, Real };

struct Value {
    ValType type = ValType::Int;
    long long i = 0;
    double r = 0.0;
};

struct Symbol {
    ValType type = ValType::Int;
    Value value;
};

static bool isAllDigits(const string& s) {
    if (s.empty()) return false;
    for (char c : s) {
        if (!isdigit(static_cast<unsigned char>(c))) return false;
    }
    return true;
}

static bool isRealNum(const string& s) {
    // REALNUM: INTNUM . INTNUM (positive)
    size_t dot = s.find('.');
    if (dot == string::npos) return false;
    if (dot == 0 || dot + 1 >= s.size()) return false;
    return isAllDigits(s.substr(0, dot)) && isAllDigits(s.substr(dot + 1));
}

static bool isIdentifier(const string& s) {
    if (s.empty()) return false;
    for (char c : s) {
        if (!(c >= 'a' && c <= 'z')) return false;
    }
    return true;
}

static string stripUtf8Bom(string s) {
    // UTF-8 BOM: EF BB BF
    if (s.size() >= 3 &&
        static_cast<unsigned char>(s[0]) == 0xEF &&
        static_cast<unsigned char>(s[1]) == 0xBB &&
        static_cast<unsigned char>(s[2]) == 0xBF) {
        s.erase(0, 3);
    }
    return s;
}

static bool g_hasSemanticError = false;

static void semanticError(int line, const string& msg) {
    g_hasSemanticError = true;
    cout << "error message:line " << line << "," << msg << "\n";
}

[[noreturn]] static void fatalError(int line, const string& msg) {
    cout << "error message:line " << line << "," << msg << "\n";
    exit(0);
}

class Parser {
public:
    explicit Parser(vector<Token> toks) : tokens_(std::move(toks)) {}

    void parseAndExecuteProgram() {
        parseDecls();
        parseCompoundStmt();
        expect("EOF");
    }

    void dumpVariables() {
        for (size_t idx = 0; idx < declOrder_.size(); idx++) {
            const string& name = declOrder_[idx];
            const Symbol& sym = symtab_.at(name);
            cout << name << ": " << formatValue(sym.value, sym.type) << "\n";
        }
    }

private:
    vector<Token> tokens_;
    size_t pos_ = 0;

    unordered_map<string, Symbol> symtab_;
    vector<string> declOrder_;

    const Token& peek() const {
        if (pos_ >= tokens_.size()) return tokens_.back();
        return tokens_[pos_];
    }

    const Token& consume() {
        const Token& t = peek();
        if (pos_ < tokens_.size()) pos_++;
        return t;
    }

    bool accept(const string& type) {
        if (peek().type == type) {
            consume();
            return true;
        }
        return false;
    }

    const Token& expect(const string& type) {
        if (peek().type != type) {
            // 题目只要求语义错误输出；这里把所有解析失败也当成语义错误处理。
            fatalError(peek().line, "unexpected token \"" + peek().lexeme + "\"");
        }
        return consume();
    }

    void parseDecls() {
        while (peek().type == "int" || peek().type == "real") {
            parseDecl();
            expect(";");
        }
        // epsilon
    }

    void parseDecl() {
        if (accept("int")) {
            Token idTok = expect("ID");
            expect("=");
            Token numTok = peek();
            if (numTok.type != "INTNUM") {
                // e.g. realnum assigned to int in declaration
                semanticError(numTok.line, "realnum can not be translated into int type");
            }
            consume();

            const string& name = idTok.lexeme;
            if (symtab_.find(name) != symtab_.end()) {
                semanticError(idTok.line, "identifier redeclared");
            }

            long long v = 0;
            try {
                if (numTok.type == "REALNUM") {
                    v = static_cast<long long>(stod(numTok.lexeme));
                } else {
                    v = stoll(numTok.lexeme);
                }
            } catch (...) {
                semanticError(numTok.line, "invalid int literal");
                v = 0;
            }

            Symbol sym;
            sym.type = ValType::Int;
            sym.value.type = ValType::Int;
            sym.value.i = v;
            sym.value.r = static_cast<double>(v);
            symtab_[name] = sym;
            declOrder_.push_back(name);
            return;
        }

        if (accept("real")) {
            Token idTok = expect("ID");
            expect("=");
            Token numTok = peek();
            if (numTok.type != "REALNUM") {
                // 如果给 real 用了 intnum，按题意不接受（decl -> real ID = REALNUM）
                semanticError(numTok.line, "intnum can not be translated into real type");
            }
            consume();

            const string& name = idTok.lexeme;
            if (symtab_.find(name) != symtab_.end()) {
                semanticError(idTok.line, "identifier redeclared");
            }

            double v = 0.0;
            try {
                v = stod(numTok.lexeme);
            } catch (...) {
                semanticError(numTok.line, "invalid real literal");
                v = 0.0;
            }

            Symbol sym;
            sym.type = ValType::Real;
            sym.value.type = ValType::Real;
            sym.value.r = v;
            sym.value.i = static_cast<long long>(v);
            symtab_[name] = sym;
            declOrder_.push_back(name);
            return;
        }

        semanticError(peek().line, "expected decl");
    }

    void parseStmt() {
        if (peek().type == "if") {
            parseIfStmt();
            return;
        }
        if (peek().type == "{") {
            parseCompoundStmt();
            return;
        }
        if (peek().type == "ID") {
            parseAssgStmt();
            return;
        }
        semanticError(peek().line, "expected stmt");
    }

    void parseCompoundStmt() {
        expect("{");
        parseStmts();
        expect("}");
    }

    void parseStmts() {
        while (peek().type == "if" || peek().type == "{" || peek().type == "ID") {
            parseStmt();
        }
        // epsilon
    }

    void parseIfStmt() {
        expect("if");
        expect("(");
        bool cond = parseBoolExpr();
        expect(")");
        expect("then");
        if (cond) {
            parseStmt();
            expect("else");
            // 解析但不执行 else 分支
            parseStmtSkipExec();
        } else {
            // 解析但不执行 then 分支
            parseStmtSkipExec();
            expect("else");
            parseStmt();
        }
    }

    // 跳过执行：为了保持语法解析一致，需要照常解析，但不更新符号表。
    void parseStmtSkipExec() {
        // 通过“影子执行”实现：保存快照，执行后回滚。
        auto snapshot = symtab_;
        parseStmt();
        symtab_ = std::move(snapshot);
    }

    void parseAssgStmt() {
        Token idTok = expect("ID");
        expect("=");
        Value rhs = parseArithexpr();
        expect(";");

        auto it = symtab_.find(idTok.lexeme);
        if (it == symtab_.end()) {
            semanticError(idTok.line, "identifier not declared");
            return;
        }

        Symbol& sym = it->second;
        if (sym.type == ValType::Int) {
            if (rhs.type == ValType::Real) {
                semanticError(idTok.line, "realnum can not be translated into int type");
            }
            sym.value.type = ValType::Int;
            sym.value.i = (rhs.type == ValType::Real) ? static_cast<long long>(rhs.r) : rhs.i;
            sym.value.r = static_cast<double>(sym.value.i);
        } else {
            // real: allow int -> real promotion
            sym.value.type = ValType::Real;
            sym.value.r = (rhs.type == ValType::Real) ? rhs.r : static_cast<double>(rhs.i);
            sym.value.i = static_cast<long long>(sym.value.r);
        }
    }

    bool parseBoolExpr() {
        Value lhs = parseArithexpr();
        Token opTok = peek();
        if (opTok.type != "<" && opTok.type != ">" && opTok.type != "<=" && opTok.type != ">=" && opTok.type != "==") {
            semanticError(opTok.line, "expected boolop");
        }
        consume();
        Value rhs = parseArithexpr();
        return evalCompare(lhs, opTok.type, rhs);
    }

    Value parseArithexpr() {
        Value v = parseMultexpr();
        while (peek().type == "+" || peek().type == "-") {
            Token opTok = consume();
            Value r = parseMultexpr();
            v = evalBinary(v, opTok.type, r, opTok.line);
        }
        return v;
    }

    Value parseMultexpr() {
        Value v = parseSimpleexpr();
        while (peek().type == "*" || peek().type == "/") {
            Token opTok = consume();
            Value r = parseSimpleexpr();
            v = evalBinary(v, opTok.type, r, opTok.line);
        }
        return v;
    }

    Value parseSimpleexpr() {
        if (peek().type == "ID") {
            Token idTok = consume();
            auto it = symtab_.find(idTok.lexeme);
            if (it == symtab_.end()) {
                semanticError(idTok.line, "identifier not declared");
                Value v;
                v.type = ValType::Int;
                v.i = 0;
                v.r = 0.0;
                return v;
            }
            return it->second.value;
        }

        if (peek().type == "INTNUM") {
            Token t = consume();
            Value v;
            v.type = ValType::Int;
            try {
                v.i = stoll(t.lexeme);
            } catch (...) {
                semanticError(t.line, "invalid int literal");
                v.i = 0;
            }
            v.r = static_cast<double>(v.i);
            return v;
        }

        if (peek().type == "REALNUM") {
            Token t = consume();
            Value v;
            v.type = ValType::Real;
            try {
                v.r = stod(t.lexeme);
            } catch (...) {
                semanticError(t.line, "invalid real literal");
                v.r = 0.0;
            }
            v.i = static_cast<long long>(v.r);
            return v;
        }

        if (accept("(")) {
            Value v = parseArithexpr();
            expect(")");
            return v;
        }

        semanticError(peek().line, "expected simpleexpr");
        Value v;
        v.type = ValType::Int;
        v.i = 0;
        v.r = 0.0;
        return v;
    }

    static Value evalBinary(const Value& a, const string& op, const Value& b, int opLine) {
        bool real = (a.type == ValType::Real) || (b.type == ValType::Real);
        if (real) {
            double x = (a.type == ValType::Real) ? a.r : static_cast<double>(a.i);
            double y = (b.type == ValType::Real) ? b.r : static_cast<double>(b.i);
            Value out;
            out.type = ValType::Real;
            if (op == "+") out.r = x + y;
            else if (op == "-") out.r = x - y;
            else if (op == "*") out.r = x * y;
            else if (op == "/") {
                if (y == 0.0) {
                    semanticError(opLine, "division by zero");
                    out.r = 0.0;
                } else {
                    out.r = x / y;
                }
            }
            else out.r = 0.0;
            out.i = static_cast<long long>(out.r);
            return out;
        }

        // int arithmetic
        Value out;
        out.type = ValType::Int;
        if (op == "+") out.i = a.i + b.i;
        else if (op == "-") out.i = a.i - b.i;
        else if (op == "*") out.i = a.i * b.i;
        else if (op == "/") {
            if (b.i == 0) {
                semanticError(opLine, "division by zero");
                out.i = 0;
            } else {
                out.i = a.i / b.i; // integer division
            }
        }
        else out.i = 0;
        out.r = static_cast<double>(out.i);
        return out;
    }

    static bool evalCompare(const Value& a, const string& op, const Value& b) {
        bool real = (a.type == ValType::Real) || (b.type == ValType::Real);
        if (real) {
            double x = (a.type == ValType::Real) ? a.r : static_cast<double>(a.i);
            double y = (b.type == ValType::Real) ? b.r : static_cast<double>(b.i);
            if (op == "<") return x < y;
            if (op == ">") return x > y;
            if (op == "<=") return x <= y;
            if (op == ">=") return x >= y;
            if (op == "==") return x == y;
            return false;
        }

        long long x = a.i;
        long long y = b.i;
        if (op == "<") return x < y;
        if (op == ">") return x > y;
        if (op == "<=") return x <= y;
        if (op == ">=") return x >= y;
        if (op == "==") return x == y;
        return false;
    }

    static string formatReal(double v) {
        // print like 1.5 instead of 1.500000
        ostringstream oss;
        oss.setf(std::ios::fixed);
        oss << setprecision(10) << v;
        string s = oss.str();
        // trim trailing zeros
        while (s.size() > 1 && s.find('.') != string::npos && s.back() == '0') s.pop_back();
        if (!s.empty() && s.back() == '.') s.pop_back();
        if (s == "-0") s = "0";
        return s;
    }

    static string formatValue(const Value& v, ValType declaredType) {
        if (declaredType == ValType::Int) {
            return to_string(v.i);
        }
        double out = (v.type == ValType::Real) ? v.r : static_cast<double>(v.i);
        return formatReal(out);
    }
};

static vector<Token> lexAllTokensFromStdin() {
    vector<Token> toks;
    string line;
    int logicalLineNo = 0;
    while (std::getline(cin, line)) {
        std::stringstream ss(line);

        vector<string> raws;
        string raw;
        while (ss >> raw) {
            raw = stripUtf8Bom(std::move(raw));
            if (!raw.empty()) raws.push_back(std::move(raw));
        }
        if (raws.empty()) continue;

        logicalLineNo++;
        for (const auto& tokRaw : raws) {
            Token t;
            t.lexeme = tokRaw;
            t.line = logicalLineNo;

            if (tokRaw == "{" || tokRaw == "}" || tokRaw == "(" || tokRaw == ")" || tokRaw == ";" || tokRaw == "=") {
                t.type = tokRaw;
            } else if (tokRaw == "+" || tokRaw == "-" || tokRaw == "*" || tokRaw == "/") {
                t.type = tokRaw;
            } else if (tokRaw == "<" || tokRaw == ">" || tokRaw == "<=" || tokRaw == ">=" || tokRaw == "==") {
                t.type = tokRaw;
            } else if (tokRaw == "if" || tokRaw == "then" || tokRaw == "else" || tokRaw == "int" || tokRaw == "real") {
                t.type = tokRaw;
            } else if (isRealNum(tokRaw)) {
                t.type = "REALNUM";
            } else if (isAllDigits(tokRaw)) {
                t.type = "INTNUM";
            } else if (isIdentifier(tokRaw)) {
                t.type = "ID";
            } else {
                t.type = tokRaw; // unknown; will trigger error when expected
            }

            toks.push_back(t);
        }
    }

    Token eof;
    eof.type = "EOF";
    eof.lexeme = "EOF";
    eof.line = (logicalLineNo == 0 ? 1 : logicalLineNo);
    toks.push_back(eof);
    return toks;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    vector<Token> toks = lexAllTokensFromStdin();
    Parser p(std::move(toks));
    p.parseAndExecuteProgram();
    if (!g_hasSemanticError) {
        p.dumpVariables();
    }
    return 0;
}
