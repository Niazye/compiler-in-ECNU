// C语言词法分析器
#include <string>
#include <regex>
#include <set>
#include <list>
#include <assert.h>
#include <iostream>

using namespace std;

enum class DataType
{
    INTNUM,
    REALNUM,
    UNKNOWN_TYPE
};
struct attribute_t
{
    enum attribute_kind {INHERIT, SYNTHESIZED} kind = SYNTHESIZED; // 属性类型：综合属性或继承属性
    union {
        long long int_value; // 整型值
        double real_value; // 浮点型值
        DataType data_type; // 数据类型
        size_t symbol_table_entry; // 符号表条目入口
    } value = {0}; // 属性值
    attribute_t(){}
    attribute_t(attribute_t::attribute_kind kind, long long x) : kind(kind) {value.int_value = x; }
    attribute_t(attribute_t::attribute_kind kind, double x) : kind(kind) {value.real_value = x; }
    attribute_t(attribute_t::attribute_kind kind, DataType x) : kind(kind) {value.data_type = x; }
    attribute_t(attribute_t::attribute_kind kind, size_t x) : kind(kind) {value.symbol_table_entry = x; }
};
enum class TokenType
{
    KEYWORD,    // 关键字
    IDENTIFIER, // IDENTIFIER
    INTNUM,   // 正整数
    REALNUM,  // 正实数
    OPERATOR,   // 运算符
    UNKNOWN
};
struct Attributes
{
    attribute_t type = {attribute_t::attribute_kind::SYNTHESIZED, DataType::UNKNOWN_TYPE};
    attribute_t value = {attribute_t::attribute_kind::SYNTHESIZED, DataType::UNKNOWN_TYPE};
    attribute_t entry = {attribute_t::attribute_kind::SYNTHESIZED, DataType::UNKNOWN_TYPE};
    attribute_t left_value = {attribute_t::attribute_kind::INHERIT, DataType::UNKNOWN_TYPE};
    attribute_t left_type = {attribute_t::attribute_kind::INHERIT, DataType::UNKNOWN_TYPE};
};
struct token_t
{
    TokenType type; // 词素类型
    Attributes attributes; // 词素属性，关键字和操作符没有属性
    string value; // 词素内容
    size_t line_no; // 词素所在行号
    token_t() : type(TokenType::UNKNOWN), line_no(0) {}
};
struct symbol_t
{
    string name; // 符号名称
    size_t line_no;
    Attributes attributes; // 词素属性，关键字和操作符没有属性
    bool operator< (const symbol_t &other) const
    {
        if(this -> attributes.type.value.data_type == DataType::INTNUM && other.attributes.type.value.data_type == DataType::INTNUM)
            return this -> attributes.value.value.int_value < other.attributes.value.value.int_value;
        else if(this -> attributes.type.value.data_type == DataType::REALNUM && other.attributes.type.value.data_type == DataType::REALNUM)
            return this -> attributes.value.value.real_value < other.attributes.value.value.real_value;
        else if(this -> attributes.type.value.data_type == DataType::INTNUM && other.attributes.type.value.data_type == DataType::REALNUM)
            return static_cast<double>(this -> attributes.value.value.int_value) < other.attributes.value.value.real_value;
        else if(this -> attributes.type.value.data_type == DataType::REALNUM && other.attributes.type.value.data_type == DataType::INTNUM)
            return this -> attributes.value.value.real_value < static_cast<double>(other.attributes.value.value.int_value);
        else
            assert(0);
    }
    bool operator> (const symbol_t &other) const
    {
        return other < *this;
    }
    bool operator== (const symbol_t &other) const
    {
        return !(*this < other) && !(other < *this);
    }
    bool operator!= (const symbol_t &other) const
    {
        return !(*this == other);
    }
    bool operator<= (const symbol_t &other) const
    {
        return (*this < other) || (*this == other);
    }
    bool operator>= (const symbol_t &other) const
    {
        return !(*this < other);
    }
};
typedef std::pair<int, std::string> resolved_token_t;

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

class LexAnalyser
{
private:
    regex INTNUM_RE = regex(R"(\d+)");
    regex REALNUM_RE = regex(R"(\d+\.\d+)");
    regex IDENTIFIER_RE = regex(R"([a-z]+)");
    set<string> keywords = {"int", "real", "if", "else", "then"};
    set<string> operators = {"+", "-", "*", "/", "=", "==", "!=", "<", ">", "<=", ">=", ";", ",", "(", ")", "{", "}"};
    list<token_t> tokens;
    list<pair<string, size_t>> input_words;

    /*
     * 获取下一个词法单元，并通过引用存储在传入的 token 参数中
     * 若此时已经到达末尾，直接返回 false 停止外部的 while 循环
    */
    bool get_next_token(token_t &token)
    {
        if (input_words.empty())
            return false;
        string str = input_words.front().first;
        size_t line_num = input_words.front().second;
        input_words.pop_front();

        if (keywords.find(str) != keywords.end()) {
            token.value = str;
            token.type = TokenType::KEYWORD;
            token.line_no = line_num;
        }
        else if (regex_match(str, IDENTIFIER_RE)) {
            token.value = str;
            token.type = TokenType::IDENTIFIER;
            token.line_no = line_num;
        }
        else if (regex_match(str, REALNUM_RE)) {
            token.attributes.value = {attribute_t::attribute_kind::SYNTHESIZED, stod(str)};
            token.attributes.type = {attribute_t::attribute_kind::SYNTHESIZED, DataType::REALNUM};
            token.value = str;
            token.type = TokenType::REALNUM;
            token.line_no = line_num;
        }
        else if (regex_match(str, INTNUM_RE)) {
            token.attributes.value = {attribute_t::attribute_kind::SYNTHESIZED, stoll(str)};
            token.attributes.type = {attribute_t::attribute_kind::SYNTHESIZED, DataType::INTNUM};
            token.value = str;
            token.type = TokenType::INTNUM;
            token.line_no = line_num;
        }
        else if (operators.find(str) != operators.end()) {
            token.value = str;
            token.type = TokenType::OPERATOR;
            token.line_no = line_num;
        }
        else {
            token.value = str;
            token.type = TokenType::UNKNOWN;
            token.line_no = line_num;
        }

        return 1;
    }
public:
    LexAnalyser(const std::string &input_prog)
    {
        tokens.clear();
        input_words.clear();
        istringstream iss(input_prog);
        string line;
        size_t line_num = 0;
        while(getline(iss, line))
        {
            trim_inplace(line);
            if (line.empty()) continue;
            line_num++;
            istringstream line_ss(line);
            string word;
            while(line_ss >> word)
                input_words.push_back({word, line_num});
        }
    }

    list<token_t> analyze()
    {
        token_t token;
        while(get_next_token(token))
            tokens.push_back(token);
        return tokens;
    }
};
struct SyntaxTree
{
    unique_ptr<symbol_t> symbol;
    vector<shared_ptr<SyntaxTree>> children;
    SyntaxTree() : symbol(make_unique<symbol_t>()) {}
};
struct SymbolEntry
{
    string name;
    DataType type;
    union {
        long long int_value;
        double real_value;
    } value;
    SymbolEntry(const string &n, DataType t, long long v) : name(n), type(t) {value.int_value = v; }
    SymbolEntry(const string &n, DataType t, double v) : name(n), type(t) {value.real_value = v; }
};
class HardLL1Parser
{
    list<token_t>::const_iterator it;
    const list<token_t> tokens;
    vector<SymbolEntry> symbol_table;
    map<string, size_t> symbol_table_index;
    bool only_expand_literally = false;
    bool no_error_or_warning = true;
public:
    HardLL1Parser(list<token_t> input_tokens) : tokens(input_tokens)
    {
        it = tokens.begin();
        shared_ptr<SyntaxTree> root = make_shared<SyntaxTree>();
        expand_program(root);
        if (no_error_or_warning)
            print();
    }
    bool expect(TokenType token_type, const string &token_value = "")
    {
        if (it == tokens.end())
            return false;
        token_t current_token = *it;
        if (current_token.type != token_type)
            return false;
        if (!token_value.empty() && current_token.value != token_value)
            return false;
        return true;
    }
    void expand_program(shared_ptr<SyntaxTree>& root)
    {
        root -> symbol -> name = "program";
        root -> children.reserve(2);
        for (int i = 0; i < 2; i++) root -> children.emplace_back(make_shared<SyntaxTree>());

        if (expect(TokenType::KEYWORD, "int") || expect(TokenType::KEYWORD, "real"))
            expand_decls(root -> children[0]);
        else 
            error_unexpected(it -> value ,"int", "real");
        if (expect(TokenType::OPERATOR, "{"))
            expand_compoundstmt(root -> children[1]);
        else
            error_unexpected(it -> value, "{");
    }
    list<token_t>::const_iterator consume(shared_ptr<SyntaxTree>& node, TokenType token_type, const string &token_value = "")
    {
        if(expect(token_type, token_value))
        {
            node -> symbol -> name = it -> value;
            node -> symbol -> line_no = it -> line_no;
            node -> symbol -> attributes = it -> attributes;
        }
        else
            error_unexpected(it -> value, token_value.empty() ? to_string(static_cast<int>(token_type)) : token_value);
        return it++;
    }
    void expand_decls(shared_ptr<SyntaxTree>& node)
    {
        node -> symbol -> name = "decls";

        if (expect(TokenType::KEYWORD, "int") || expect(TokenType::KEYWORD, "real"))
        {
            node -> children.reserve(3);
            for (int i = 0; i < 3; i++) node -> children.emplace_back(make_shared<SyntaxTree>());

            expand_decl(node -> children[0]);
            consume(node -> children[1], TokenType::OPERATOR, ";");
            expand_decls(node -> children[2]); // 递归处理下一个声明
        }
    }
    void expand_decl(shared_ptr<SyntaxTree>& node)
    {
        node -> symbol -> name = "decl";
        node -> children.reserve(4);
        for (int i = 0; i < 4; i++) node -> children.emplace_back(make_shared<SyntaxTree>());
        
        auto &child = node -> children;
        if(expect(TokenType::KEYWORD, "int"))
        {
            consume(child[0], TokenType::KEYWORD, "int"); // 消耗 int
            consume(child[1], TokenType::IDENTIFIER); // 消耗标识符
            consume(child[2], TokenType::OPERATOR, "=");
            if (expect(TokenType::INTNUM))
                consume(child[3], TokenType::INTNUM);
            else if (expect(TokenType::REALNUM))
            {
                // 正实数不能变成整数
                consume(child[3], TokenType::REALNUM);
                warning_type_mismatch(DataType::INTNUM, DataType::REALNUM, it -> line_no);
                child[3] -> symbol -> attributes.type.value.data_type = DataType::INTNUM;
                child[3] -> symbol -> attributes.value.value.int_value = static_cast<long long>(child[3] -> symbol -> attributes.value.value.real_value);
            }
            else
                error_unexpected(it -> value, "INTNUM");

            if (symbol_table_index.find(child[1] -> symbol -> name) != symbol_table_index.end())
                error_redeclared( child[1] -> symbol -> name, child[1] -> symbol -> line_no);

            if (!only_expand_literally)
            {
                add_entry({child[1] -> symbol -> name, DataType::INTNUM, child[3] -> symbol -> attributes.value.value.int_value});
                node -> symbol -> attributes.entry.value.symbol_table_entry = symbol_table.size() - 1; // 将符号表位置存入
                node -> symbol -> attributes.value.value.int_value = symbol_table.back().value.int_value; 
                node -> symbol -> attributes.type.value.data_type = DataType::INTNUM;
                node -> symbol -> line_no = child.back() -> symbol -> line_no;
            }
        }
        else if (expect(TokenType::KEYWORD, "real"))
        {
            bool type_casted = 0;
            consume(child[0] ,TokenType::KEYWORD, "real"); // 消耗 real
            consume(child[1], TokenType::IDENTIFIER); // 消耗标识符
            consume(child[2], TokenType::OPERATOR, "=");

            if (expect(TokenType::REALNUM))
                consume(child[3], TokenType::REALNUM);
            else if (expect(TokenType::INTNUM))
            {
                consume(child[3], TokenType::INTNUM);
                child[3] -> symbol -> attributes.type.value.data_type = DataType::REALNUM;
                child[3] -> symbol -> attributes.value.value.real_value = static_cast<double>(child[3] -> symbol -> attributes.value.value.int_value);
            }
            else
                error_unexpected(it -> value, "REALNUM");
                
            if (symbol_table_index.find(child[1] -> symbol -> name) != symbol_table_index.end())
                error_redeclared(child[1] -> symbol -> name, child[1] -> symbol -> line_no);

            if (!only_expand_literally)
            {
                if (type_casted)
                {
                    child[3] -> symbol -> attributes.value.value.real_value = static_cast<double>(child[3] -> symbol -> attributes.value.value.int_value);
                    child[3] -> symbol -> attributes.type.value.data_type = DataType::REALNUM;
                }
                add_entry({child[1] -> symbol -> name, DataType::REALNUM, child[3] -> symbol -> attributes.value.value.real_value});
                node -> symbol -> attributes.entry.value.symbol_table_entry = symbol_table.size() - 1; // 将符号表位置存入
                node -> symbol -> attributes.value.value.real_value = symbol_table.back().value.real_value; 
                node -> symbol -> attributes.type.value.data_type = DataType::REALNUM;
                node -> symbol -> line_no = child.back() -> symbol -> line_no;
            }
        }
        else
            error_unexpected(it -> value, "int", "real");
    }
    void expand_compoundstmt(shared_ptr<SyntaxTree>& node)
    {
        node -> symbol -> name = "compound_stmt";
        node -> children.reserve(3);
        for (int i = 0; i < 3; i++) node -> children.emplace_back(make_shared<SyntaxTree>());

        auto &child = node -> children;
        consume(child[0], TokenType::OPERATOR, "{");
        expand_stmts(child[1]);
        consume(child[2], TokenType::OPERATOR, "}");
    }
    void expand_stmts(shared_ptr<SyntaxTree>& node)
    {
        node -> symbol -> name = "stmts";
        if (expect(TokenType::KEYWORD, "if") || expect(TokenType::OPERATOR, "{") || expect(TokenType::IDENTIFIER))
        {
            node -> children.reserve(2);
            for (int i = 0; i < 2; i++) node -> children.emplace_back(make_shared<SyntaxTree>());
            expand_stmt(node -> children[0]);
            expand_stmts(node -> children[1]); // 递归处理下一个语句
        }
    }
    void expand_stmt(shared_ptr<SyntaxTree>& node)
    {
        node -> symbol -> name = "stmts";
        node -> children.reserve(1);
        for (int i = 0; i < 1; i++) node -> children.emplace_back(make_shared<SyntaxTree>());

        if (expect(TokenType::KEYWORD, "if"))
            expand_ifstmt(node -> children[0]);
        else if (expect(TokenType::OPERATOR, "{"))
            expand_compoundstmt(node -> children[0]);
        else if (expect(TokenType::IDENTIFIER))
            expand_assgstmt(node -> children[0]);
        else
            error_unexpected(it -> value, "if", "{", "ID");
    }
    void expand_ifstmt(shared_ptr<SyntaxTree>& node)
    {
        node -> symbol -> name = "ifstmt";
        node -> children.reserve(8);
        for (int i = 0; i < 8; i++) node -> children.emplace_back(make_shared<SyntaxTree>());

        auto &child = node -> children;
        consume(child[0], TokenType::KEYWORD, "if");
        consume(child[1], TokenType::OPERATOR, "(");
        expand_boolexpr(child[2]);
        consume(child[3], TokenType::OPERATOR, ")");
        consume(child[4], TokenType::KEYWORD, "then");
        if (child[2] -> symbol -> attributes.value.value.int_value)
        {
            expand_stmt(child[5]);
            consume(child[6], TokenType::KEYWORD, "else");
            only_expand_literally = true;
            expand_stmt(child[7]);
            only_expand_literally = false;
        }
        else
        {
            only_expand_literally = true;
            expand_stmt(child[5]);
            only_expand_literally = false;
            consume(child[6], TokenType::KEYWORD, "else");
            expand_stmt(child[7]);
        }
    }
    void expand_boolexpr(shared_ptr<SyntaxTree>& node)
    {
        node -> symbol -> name = "boolexpr";
        node -> children.reserve(3);
        for (int i = 0; i < 3; i++) node -> children.emplace_back(make_shared<SyntaxTree>());

        auto &child = node -> children;
        expand_arithexpr(child[0]);
        consume(child[1], TokenType::OPERATOR);
        expand_arithexpr(child[2]);
        bool cmp_res = 0;
        string cmp_op = child[1] -> symbol -> name;

        if (cmp_op == "<")
                cmp_res = *child[0] -> symbol < *child[2] -> symbol;
        else if(cmp_op == ">")
                cmp_res = *child[0] -> symbol > *child[2] -> symbol;
        else if(cmp_op == "<=")
                cmp_res = *child[0] -> symbol <= *child[2] -> symbol;
        else if(cmp_op == ">=")
                cmp_res = *child[0] -> symbol >= *child[2] -> symbol;
        else if(cmp_op == "==")
                cmp_res = *child[0] -> symbol == *child[2] -> symbol;
        else if(cmp_op == "!=")
                cmp_res = *child[0] -> symbol != *child[2] -> symbol;
        else
            error_unexpected(child[1] -> symbol -> name, "<", ">", "<=", ">=", "==", "!=");
        
        node -> symbol -> attributes.value.value.int_value = cmp_res;
        node -> symbol -> attributes.type.value.data_type = DataType::INTNUM;
    }
    void expand_arithexpr(shared_ptr<SyntaxTree>& node)
    {
        node -> symbol -> name = "arithexpr";
        node -> children.reserve(2);
        for (int i = 0; i < 2; i++) node -> children.emplace_back(make_shared<SyntaxTree>());

        auto &child = node -> children;
        expand_multexpr(child[0]);
        expand_arithexprprime(child[1]);
        if (child[0] -> symbol -> attributes.type.value.data_type == DataType::REALNUM || child[1] -> symbol -> attributes.type.value.data_type == DataType::REALNUM)
        {
            double left = (child[0] -> symbol -> attributes.type.value.data_type == DataType::REALNUM) ? child[0] -> symbol -> attributes.value.value.real_value : static_cast<double>(child[0] -> symbol -> attributes.value.value.int_value);
            double right = (child[1] -> symbol -> attributes.type.value.data_type == DataType::REALNUM) ? child[1] -> symbol -> attributes.value.value.real_value : static_cast<double>(child[1] -> symbol -> attributes.value.value.int_value);
            node -> symbol -> attributes.type.value.data_type = DataType::REALNUM;
            node -> symbol -> attributes.value.value.real_value = left + right;
        }
        else
        {
            long long left = child[0] -> symbol -> attributes.value.value.int_value;
            long long right = child[1] -> symbol -> attributes.value.value.int_value;
            node -> symbol -> attributes.type.value.data_type = DataType::INTNUM;
            node -> symbol -> attributes.value.value.int_value = left + right;
        }
    }
    void expand_multexpr(shared_ptr<SyntaxTree>& node)
    {
        node -> symbol -> name = "multexpr";
        node -> children.reserve(2);
        for (int i = 0; i < 2; i++) node -> children.emplace_back(make_shared<SyntaxTree>());

        auto &child = node -> children;
        expand_simpleexpr(child[0]);
        child[1] -> symbol -> attributes.left_value.value = child[0] -> symbol -> attributes.value.value;
        child[1] -> symbol -> attributes.left_type.value = child[0] -> symbol -> attributes.type.value;
        expand_multexprprime(child[1]);
        node -> symbol -> attributes.type.value = child[1] -> symbol -> attributes.type.value;
        node -> symbol -> attributes.value.value = child[1] -> symbol -> attributes.value.value;
    }
    void expand_simpleexpr(shared_ptr<SyntaxTree>& node)
    {
        node -> symbol -> name = "simpleexpr";
        if (expect(TokenType::IDENTIFIER))
        {
            node -> children.reserve(1);
            for (int i = 0; i < 1; i++) node -> children.emplace_back(make_shared<SyntaxTree>());

            auto &child = node -> children;
            consume(child[0], TokenType::IDENTIFIER);
            if (symbol_table_index.find(child[0] -> symbol -> name) == symbol_table_index.end())
                error_undefined(child[0] -> symbol -> name, child[0] -> symbol -> line_no);
            size_t symbol_index = symbol_table_index[child[0] -> symbol -> name];

            node -> symbol -> attributes.type.value.data_type = symbol_table[symbol_index].type;
            if (symbol_table[symbol_index].type == DataType::INTNUM)
                node -> symbol -> attributes.value.value.int_value = symbol_table[symbol_index].value.int_value;
            else
                node -> symbol -> attributes.value.value.real_value = symbol_table[symbol_index].value.real_value;
        }
        else if (expect(TokenType::INTNUM))
        {
            node -> children.reserve(1);
            for (int i = 0; i < 1; i++) node -> children.emplace_back(make_shared<SyntaxTree>());

            auto &child = node -> children;
            consume(child[0], TokenType::INTNUM);
            node -> symbol -> attributes.value.value = child[0] -> symbol -> attributes.value.value;
            node -> symbol -> attributes.type.value = child[0] -> symbol -> attributes.type.value;
        }
        else if (expect(TokenType::REALNUM))
        {
            node -> children.reserve(1);
            for (int i = 0; i < 1; i++) node -> children.emplace_back(make_shared<SyntaxTree>());

            auto &child = node -> children;
            consume(child[0], TokenType::REALNUM);
            node -> symbol -> attributes.value.value = child[0] -> symbol -> attributes.value.value;
            node -> symbol -> attributes.type.value = child[0] -> symbol -> attributes.type.value;
        }
        else if (expect(TokenType::OPERATOR, "("))
        {
            node -> children.reserve(3);
            for (int i = 0; i < 3; i++) node -> children.emplace_back(make_shared<SyntaxTree>());

            auto &child = node -> children;
            consume(child[0], TokenType::OPERATOR, "(");
            expand_arithexpr(child[1]);
            consume(child[2], TokenType::OPERATOR, ")");
            node -> symbol -> attributes.value.value = child[1] -> symbol -> attributes.value.value;
            node -> symbol -> attributes.type.value = child[1] -> symbol -> attributes.type.value;
        }
        else
            error_unexpected(it -> value, "ID", "INTNUM", "REALNUM", "(");
    }
    void expand_multexprprime(shared_ptr<SyntaxTree>& node)
    {
        node -> symbol -> name = "multexprprime";
        if (expect(TokenType::OPERATOR, "*") || expect(TokenType::OPERATOR, "/"))
        {
            node -> children.reserve(3);
            for (int i = 0; i < 3; i++) node -> children.emplace_back(make_shared<SyntaxTree>());

            auto &child = node -> children;
            consume(child[0], TokenType::OPERATOR);
            expand_simpleexpr(child[1]);
            DataType left_type = node -> symbol -> attributes.left_type.value.data_type;
            DataType right_type = child[1] -> symbol -> attributes.type.value.data_type;
            if (left_type == DataType::REALNUM || right_type == DataType::REALNUM)
            {
                double left = (left_type == DataType::REALNUM) ? node -> symbol -> attributes.left_value.value.real_value : static_cast<double>(node -> symbol -> attributes.left_value.value.int_value);
                double right = (right_type == DataType::REALNUM) ? child[1] -> symbol -> attributes.value.value.real_value : static_cast<double>(child[1] -> symbol -> attributes.value.value.int_value);
                child[2] -> symbol -> attributes.left_type.value.data_type = DataType::REALNUM;
                if (child[0] -> symbol -> name == "*")
                    child[2] -> symbol -> attributes.left_value.value.real_value = left * right;
                else
                    if (right == 0.0)
                    {
                        warning_divide_by_zero(child[0] -> symbol -> line_no);
                        child[2] -> symbol -> attributes.left_value.value.real_value = left;
                    }
                    else
                        child[2] -> symbol -> attributes.left_value.value.real_value = left / right;
            }
            else
            {
                long long left = node -> symbol -> attributes.left_value.value.int_value;
                long long right = child[1] -> symbol -> attributes.value.value.int_value;
                if (child[0] -> symbol -> name == "*")
                    child[2] -> symbol -> attributes.left_value = {attribute_t::INHERIT, left * right};
                else
                    if (right == 0)
                    {
                        warning_divide_by_zero(child[0] -> symbol -> line_no);
                        child[2] -> symbol -> attributes.left_value.value.int_value = left;
                    }
                    else
                        child[2] -> symbol -> attributes.left_value.value.int_value = left / right;
            }
            // 上面实现计算了 child[2] 的 left_value 和 left_type，这是 child[2] 的继承属性，在进入之前计算
            expand_multexprprime(child[2]);
            // 最后计算 node 的属性，这是 node 的综合属性
            node -> symbol -> attributes.type.value = child[2] -> symbol -> attributes.type.value;
            node -> symbol -> attributes.value.value = child[2] -> symbol -> attributes.value.value;
        }
        else
        {
            node -> symbol -> attributes.value.value = node -> symbol -> attributes.left_value.value;
            node -> symbol -> attributes.type.value = node -> symbol -> attributes.left_type.value;
        }
    }
    void expand_arithexprprime(shared_ptr<SyntaxTree>& node)
    {
        node -> symbol -> name = "arithexprprime";
        if (expect(TokenType::OPERATOR, "+") || expect(TokenType::OPERATOR, "-"))
        {
            node -> children.reserve(3);
            for (int i = 0; i < 3; i++) node -> children.emplace_back(make_shared<SyntaxTree>());
            
            auto &child = node -> children;
            consume(child[0], TokenType::OPERATOR);
            expand_multexpr(child[1]);
            expand_arithexprprime(child[2]);
            // 先计算完 child[1] 和 child[2]，再计算 node 的属性，这是 node 的综合属性
            DataType left_type = child[1] -> symbol -> attributes.type.value.data_type;
            DataType right_type = child[2] -> symbol -> attributes.type.value.data_type;
            if (left_type == DataType::REALNUM || right_type == DataType::REALNUM)
            {
                double left = (left_type == DataType::REALNUM) ? child[1] -> symbol -> attributes.value.value.real_value : static_cast<double>(child[1] -> symbol -> attributes.value.value.int_value);
                double right = (right_type == DataType::REALNUM) ? child[2] -> symbol -> attributes.value.value.real_value : static_cast<double>(child[2] -> symbol -> attributes.value.value.int_value);
                child[2] -> symbol -> attributes.type.value.data_type = DataType::REALNUM;
                if (child[0] -> symbol -> name == "+")
                    child[2] -> symbol -> attributes.value.value.real_value = +left + right;
                else
                    child[2] -> symbol -> attributes.value.value.real_value = -left + right;
            }
            else
            {
                long long left = child[1] -> symbol -> attributes.value.value.int_value;
                long long right = child[2] -> symbol -> attributes.value.value.int_value;
                child[2] -> symbol -> attributes.type.value.data_type = DataType::INTNUM;
                if (child[0] -> symbol -> name == "+")
                    node -> symbol -> attributes.value.value.int_value = +left + right;
                else
                    node -> symbol -> attributes.value.value.int_value = -left + right;
            }
        }
        else
        {
            // 空产生式记录为整数 0，用于父结点的计算
            node -> symbol -> attributes.value.value.int_value = 0;
            node -> symbol -> attributes.type.value.data_type = DataType::INTNUM;
        }
    }
    void expand_assgstmt(shared_ptr<SyntaxTree>& node)
    {
        node -> symbol -> name = "assgstmt";
        node -> children.reserve(4);
        for (int i = 0; i < 4; i++) node -> children.emplace_back(make_shared<SyntaxTree>());

        auto &child = node -> children;
        consume(child[0], TokenType::IDENTIFIER);
        consume(child[1], TokenType::OPERATOR, "=");
        expand_arithexpr(child[2]);
        consume(child[3], TokenType::OPERATOR, ";");

        if (symbol_table_index.find(child[0] -> symbol -> name) == symbol_table_index.end())
            error_undefined(child[0] -> symbol -> name, child[0] -> symbol -> line_no);
        size_t symbol_index = symbol_table_index[child[0] -> symbol -> name];

        DataType var_type = symbol_table[symbol_index].type;
        DataType expr_type = child[2] -> symbol -> attributes.type.value.data_type;
        if (var_type == DataType::INTNUM && expr_type == DataType::REALNUM)
            warning_type_mismatch(DataType::INTNUM, DataType::REALNUM, child[1] -> symbol -> line_no);

        if (!only_expand_literally)
        {
            if (var_type == DataType::INTNUM)
                symbol_table[symbol_index].value.int_value = child[2] -> symbol -> attributes.value.value.int_value;
            else if(expr_type == DataType::INTNUM)
                symbol_table[symbol_index].value.real_value = static_cast<double>(child[2] -> symbol -> attributes.value.value.int_value);
            else
                symbol_table[symbol_index].value.real_value = child[2] -> symbol -> attributes.value.value.real_value;
        }
    }
    template <typename... Msgs>
    void error_unexpected(const string& got, const Msgs&... expect)
    {
        cout << "Syntax Error: expected ";
        bool first = true;
        using expander = int[];
        (void)expander{0, ((cout << (first ? "" : " or ") << expect, first = false), 0)...};
        cout << ", but got " << got;
        cout << " at line " << it -> line_no << endl;
        no_error_or_warning = false;
        exit(1);
    }
    void error_redeclared(const string &identifier, int line_no)
    {
        cout << "Semantic Error: identifier " << identifier << " redeclared at line " << line_no << endl;
        no_error_or_warning = false;
        exit(1);
    }
    void error_undefined(const string &identifier, int line_no)
    {
        cout << "Semantic Error: identifier " << identifier << " at line " << line_no << " is undefined" << endl;
        no_error_or_warning = false;
        exit(1);
    }
    void warning_type_mismatch(DataType expected, DataType got, int line_no)
    {
        cout << "error message:line " << line_no << ",";
        if (got == DataType::INTNUM)
            cout << "intnum";
        else if (got == DataType::REALNUM)
            cout << "realnum";
        else
            cout << "UNKNOWN_TYPE";
        cout << " can not be translated into ";
        if (expected == DataType::INTNUM)
            cout << "int";
        else if (expected == DataType::REALNUM)
            cout << "real";
        else
            cout << "UNKNOWN_TYPE";
        cout << " type" << endl;
        no_error_or_warning = false;
    }
    void warning_divide_by_zero(int line_no)
    {
        cout << "error message:line " << line_no << ",division by zero" << endl;
        no_error_or_warning = false;
    }
    size_t add_entry(SymbolEntry entry)
    {
        symbol_table.emplace_back(entry);
        symbol_table_index[entry.name] = symbol_table.size() - 1;
        return symbol_table.size() - 1;
    }
    void print()
    {
        for(auto &kv: symbol_table_index)
        {
            cout << kv.first << ": ";
            if (symbol_table[kv.second].type == DataType::INTNUM)
                cout << symbol_table[kv.second].value.int_value << endl;
            else if (symbol_table[kv.second].type == DataType::REALNUM)
                cout << symbol_table[kv.second].value.real_value << endl;
        }
    }
};

/* 不要修改这个标准输入函数 */
void read_prog(string& prog)
{
    char c;
    while(scanf("%c",&c)!=EOF){
        prog += c;
    }
}
/* 你可以添加其他函数 */

void Analysis()
{
    string prog;
    read_prog(prog);
    /* 骚年们 请开始你们的表演 */
    /********* Begin *********/
    LexAnalyser lexer(prog);
    list<token_t> tokens = lexer.analyze();
    HardLL1Parser parser(tokens);
    /********* End *********/
    
}