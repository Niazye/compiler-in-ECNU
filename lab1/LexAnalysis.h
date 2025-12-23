// C语言词法分析器
#include <cstdio>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <cctype>
#include <chrono>

#include "DFA.h"
#include "keys_patterns.h"
using namespace std;

enum TokenType
{
    KEYWORD,    // 关键字
    IDENTIFIER, // 标识符
    CONSTANT,   // 常数
    STRING,     // 字符串
    OPERATOR,   // 运算符
    COMMENT,    // 注释
    UNKNOWN
};

typedef TokenType token_type_id;
typedef pair<token_type_id, std::string> token_t;
typedef pair<int, string> resolved_token_t;

/* 不要修改这个标准输入函数 */
void read_prog(string &prog)
{
    char c;
    while (scanf("%c", &c) != EOF)
    {
        prog += c;
    }
}

void print_single_token(const resolved_token_t token, int index);

DFA constant_dfa = DFA(NFA(RE(EXPAND_CONSTANT_PATTERN)));
DFA identifier_dfa = DFA(NFA(RE(EXPAND_IDENTIFIER_PATTERN)));

class LexAnalyser
{
private:
    map<string, int> keys_map; // 关键字 -> 序号
    vector<token_t> unresolved_tokens;
    vector<resolved_token_t> resolved_tokens;
    string prog;
    size_t pos;
    bool is_at_string_token;
    TokenType current_type;
    /*
     * 获取下一个词法单元，并通过引用存储在传入的 token 参数中
     * 若此时已经到达末尾，直接返回 false 停止外部的 while 循环
    */
    bool get_next_token(token_t &token)
    {
        if(!is_at_string_token)
        {
            // 跳过空白字符
            while (pos < prog.length() && isspace(prog[pos]))
                pos++;
        }
        if (pos >= prog.length())
            return 0;

        if (prog[pos] == '\"') {
            is_at_string_token = !is_at_string_token;
            token = {OPERATOR, "\""};
            pos++;
            return 1;
        }

        if (is_at_string_token)
        {
            token.first = STRING;
            token = handle_string_literal();
            if (token.second != "")
                return 1;
        }

        if (isdigit(prog[pos]) || (prog[pos] == '.')) {
            current_type = CONSTANT;
            token = handle_constant();
            if (token.second != "")
                return 1;
        }

        if (isalpha(prog[pos]) || prog[pos] == '_') {
            token = handle_identifier_or_keyword();
            current_type = token.first;
            if (token.second != "")
                return 1;
        }

        if (pos + 1 < prog.length() && prog[pos] == '/' && prog[pos + 1] == '/') {
            current_type = COMMENT;
            token = handle_comment(0);
            if (token.second != "")
                return 1;
        }

        if (pos + 1 < prog.length() && prog[pos] == '/' && prog[pos + 1] == '*') {
            current_type = COMMENT;
            token = handle_comment(1);
            if (token.second != "")
                return 1;
        }

        current_type = OPERATOR;
        token = handle_operator();
        return token.second != "";
    }

    token_t handle_constant()
    {
        const size_t start_pos = pos;
        size_t match_length = constant_dfa.longest_match(prog, pos);
        token_t res;
        res.first = CONSTANT;
        res.second = prog.substr(start_pos, match_length);
        if (constant_dfa.all_match(res.second))
        {
            pos += match_length;
        }
        return res;
    }

    token_t handle_identifier_or_keyword()
    {
        const size_t start_pos = pos;
        size_t match_length = identifier_dfa.longest_match(prog, pos);
        token_t res;
        res.second = prog.substr(start_pos, match_length);
        if (keys_map.find(res.second) != keys_map.end()) {
            res.first = KEYWORD;
        } else {
            res.first = IDENTIFIER;
        }
        pos += match_length;
        return res;
    }

    token_t handle_comment(int type)
    {
        token_t res;
        if (type == 0) {
            // 单行注释
            size_t start_pos = pos;
            pos += 2; // 跳过 "//"
            while (pos < prog.length() && prog[pos] != '\n') {
                pos++;
            }
            res.first = COMMENT;
            res.second = prog.substr(start_pos, pos - start_pos);
        } else {
            // 多行注释
            size_t start_pos = pos;
            pos += 2; // 跳过 "/*"
            while (pos + 1 < prog.length() && !(prog[pos] == '*' && prog[pos + 1] == '/')) {
                pos++;
            }
            if (pos + 1 < prog.length()) {
                pos += 2; // 跳过 "*/"
            }
            res.first = COMMENT;
            res.second = prog.substr(start_pos, pos - start_pos);
        }
        return res;
    }

    token_t handle_string_literal() {
        token_t res;
        
        for (; pos < prog.length(); pos++) {
            if (prog[pos] == '\\' && pos + 1 < prog.length()) {
                ++pos; // 跳过反斜杠
                switch (prog[pos]) {
                    case 'n': res.second.push_back('\n'); break;
                    case 'r': res.second.push_back('\r'); break;
                    case 't': res.second.push_back('\t'); break;
                    case 'v': res.second.push_back('\v'); break;
                    case 'b': res.second.push_back('\b'); break;
                    case 'f': res.second.push_back('\f'); break;
                    case 'a': res.second.push_back('\a'); break;
                    case '\\': res.second.push_back('\\'); break;
                    case '\'': res.second.push_back('\''); break;
                    case '\"': res.second.push_back('\"'); break;
                    case '?': res.second.push_back('?'); break;
                    case '0': res.second.push_back('\0'); break;
                    default:
                        // 未知转义，保留原样
                        res.second.push_back('\\');
                        res.second.push_back(prog[pos]);
                        break;
                }
            } else if(prog[pos] == '\"') {
                break;
            } else {
                res.second.push_back(prog[pos]);
            }
        }

        res.first = STRING;
        return res;
    }

    token_t handle_operator()
    {
        token_t res;
        for (auto ele = keys_map.rbegin(); ele != keys_map.rend(); ele++)
        {
            auto pattern = ele->first;
            if (pos + pattern.length() > prog.length()) continue;
            bool match = true;
            for (size_t i = 0; i < pattern.length(); ++i) {
                if (prog[pos + i] != pattern[i]) {
                    match = false;
                    break;
                }
            }
            if (match)
            {
                pos += pattern.length();
                res = {OPERATOR, pattern};
                break;
            }
        }
        return res;
    }

    void resolve_tokens()
    {
        for (const auto &token: unresolved_tokens)
        {
            switch (token.first)
            {
                case IDENTIFIER:
                    resolved_tokens.push_back({keys_map["标识符"], token.second}); // 关键字
                    break;
                case CONSTANT:
                    resolved_tokens.push_back({keys_map["常数"], token.second}); // 常数
                    break;
                case STRING:
                    resolved_tokens.push_back({keys_map["标识符"], token.second}); // 字符串
                    break;
                case COMMENT:
                    resolved_tokens.push_back({keys_map["/*注释*/"], token.second}); // 注释
                    break;
                default:
                    resolved_tokens.push_back({keys_map[token.second], token.second}); // 关键字或运算符
                    break;
            }
        }
    }
public:
    LexAnalyser(const string &input_prog) : prog(input_prog), pos(0)
    {
        // 初始化关键字和符号映射表
        ifstream file("c_keys.txt");
        string line;
        while (getline(file, line))
        {
            istringstream iss(line);
            int key;
            string value;
            if (iss >> value >> key)
            {
                keys_map[value] = key;
            }
        }
        file.close();
        is_at_string_token = 0;
        unresolved_tokens.clear();
        resolved_tokens.clear();
    }

    vector<resolved_token_t> analyze()
    {
        token_t token;
        while(get_next_token(token))
        {
            if (token.second != "")
                unresolved_tokens.push_back(token);
        }
        resolve_tokens();
        return resolved_tokens;
    }

    void print_res()
    {
        for (size_t i = 0; i < resolved_tokens.size(); i++)
        {
            print_single_token(resolved_tokens[i], i + 1);
        }
    }
};

void print_single_token(const resolved_token_t token, int index)
{
    cout << index << ": <" << token.second << "," << token.first << ">" << endl;
}

void Analysis()
{
    string prog;
    read_prog(prog);
    const auto start_time = chrono::steady_clock::now();
    /********* Begin *********/

    LexAnalyser lexer(prog);
    lexer.analyze();

    lexer.print_res();/********* End *********/
}
