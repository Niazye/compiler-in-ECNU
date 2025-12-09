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
using namespace std;

typedef int token_type_id;
typedef string token_type_str;
typedef pair<token_type_str, token_type_id> token_t;

/* 不要修改这个标准输入函数 */
void read_prog(string &prog)
{
    char c;
    while (scanf("%c", &c) != EOF)
    {
        prog += c;
    }
}

void print_single_token(const token_t token, int index);

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

class LexAnalyser
{
private:
    map<token_type_str, token_type_id> keys_map;
    vector<token_t> all_tokens;
    string prog;
    int pos;
    enum TokenType current_type;

private:
    /*
     * 获取下一个词法单元，并通过引用存储在传入的 token 参数中
     * 若此时已经到达末尾，直接返回 false 停止外部的 while 循环
    */
    bool get_next_token(token_t &token)
    {
        if (pos >= prog.length())
            return 0;
        // 跳过空白字符
        while (pos < prog.length() && isspace(prog[pos]))
            pos++;

        if (isdigit(prog[pos])) {
            current_type = CONSTANT;
            token = handle_constant();
            return 1;
        }
    }

    token_t handle_constant()
    {
        
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
            string key;
            int value;
            if (iss >> key >> value)
            {
                keys_map[key] = value;
            }
        }
        file.close();
    }

    vector<pair<string, int>> analyze()
    {
        token_t token;
        while(get_next_token(token))
        {
            all_tokens.push_back(token);
        }
        
    }

    void print_res()
    {
        for (int i = 0; i < all_tokens.size(); i++)
        {
            print_single_token(all_tokens[i], i + 1);
        }
    }
};

void print_single_token(const token_t token, int index)
{
    cout << index << ": <" << token.first << "." << token.second << ">" << endl;
}

class Lexer_Example
{
private:
    map<string, int> keywordMap;      // 关键字和符号映射表
    vector<pair<string, int>> tokens; // 存储词法单元
    string input;                     // 输入的源代码
    int pos;                          // 当前位置
    int lineNum;                      // 行号
    int tokenCount;                   // 词法单元计数

    // 初始化关键字和符号表
    void initKeywordMap()
    {
        ifstream file("c_keys.txt");
        string line;
        while (getline(file, line))
        {
            istringstream iss(line);
            string key;
            int value;
            if (iss >> key >> value)
            {
                keywordMap[key] = value;
            }
        }
        file.close();
    }

    // 跳过空白字符
    void skipWhitespace()
    {
        while (pos < input.length() && isspace(input[pos]))
        {
            if (input[pos] == '\n')
            {
                lineNum++;
            }
            pos++;
        }
    }

    // 处理注释
    bool handleComment()
    {
        if (pos + 1 < input.length() && input[pos] == '/' && input[pos + 1] == '/')
        {
            // 单行注释
            pos += 2;
            while (pos < input.length() && input[pos] != '\n')
            {
                pos++;
            }
            if (pos < input.length() && input[pos] == '\n')
            {
                lineNum++;
                pos++;
            }
            return true;
        }
        else if (pos + 1 < input.length() && input[pos] == '/' && input[pos + 1] == '*')
        {
            // 多行注释
            pos += 2;
            while (pos + 1 < input.length() && !(input[pos] == '*' && input[pos + 1] == '/'))
            {
                if (input[pos] == '\n')
                {
                    lineNum++;
                }
                pos++;
            }
            if (pos + 1 < input.length())
            {
                pos += 2; // 跳过 "*/"
            }
            return true;
        }
        return false;
    }

    // 处理数字（常数）
    string handleNumber()
    {
        int start = pos;
        while (pos < input.length() && isdigit(input[pos]))
        {
            pos++;
        }
        return input.substr(start, pos - start);
    }

    // 处理标识符或关键字
    string handleIdentifierOrKeyword()
    {
        int start = pos;
        while (pos < input.length() && (isalnum(input[pos]) || input[pos] == '_'))
        {
            pos++;
        }
        return input.substr(start, pos - start);
    }

    // 处理字符串内容（作为标识符处理）
    string handleStringContent()
    {
        int start = pos;
        while (pos < input.length() && input[pos] != '"')
        {
            // 字符串内容可以包含字母、数字和一些特殊字符
            pos++;
        }
        return input.substr(start, pos - start);
    }

    // 处理运算符和分隔符
    string handleOperator()
    {
        // 检查3字符运算符
        if (pos + 2 < input.length())
        {
            string threeChar = input.substr(pos, 3);
            if (threeChar == "<<=" || threeChar == ">>=")
            {
                pos += 3;
                return threeChar;
            }
        }

        // 检查2字符运算符
        if (pos + 1 < input.length())
        {
            string twoChar = input.substr(pos, 2);
            if (twoChar == "++" || twoChar == "--" || twoChar == "->" ||
                twoChar == "+=" || twoChar == "-=" || twoChar == "*=" ||
                twoChar == "/=" || twoChar == "%=" || twoChar == "&=" ||
                twoChar == "|=" || twoChar == "^=" || twoChar == "==" ||
                twoChar == "!=" || twoChar == "<=" || twoChar == ">=" ||
                twoChar == "&&" || twoChar == "||" || twoChar == "<<" ||
                twoChar == ">>" || twoChar == "//")
            {
                pos += 2;
                return twoChar;
            }
        }

        // 单字符运算符
        string singleChar = input.substr(pos, 1);
        pos++;
        return singleChar;
    }

public:
    Lexer_Example(const string &prog) : input(prog), pos(0), lineNum(1), tokenCount(0)
    {
        initKeywordMap();
    }

    // 获取下一个词法单元
    bool getNextToken(string &token, int &type)
    {
        // 跳过空白字符
        skipWhitespace();

        // 检查是否到达文件末尾
        if (pos >= input.length())
        {
            return false;
        }

        // 处理注释
        if (handleComment())
        {
            token = "/*注释*/";
            type = 79;
            tokenCount++;
            cout << tokenCount << ": <" << token << "," << type << ">" << endl;
            return true;
        }

        // 处理数字
        if (isdigit(input[pos]))
        {
            token = handleNumber();
            type = 80; // 常数
            tokenCount++;
            cout << tokenCount << ": <" << token << "," << type << ">" << endl;
            return true;
        }

        // 处理标识符或关键字
        if (isalpha(input[pos]) || input[pos] == '_')
        {
            token = handleIdentifierOrKeyword();

            // 检查是否是关键字
            if (keywordMap.find(token) != keywordMap.end())
            {
                type = keywordMap[token];
            }
            else
            {
                type = 81; // 标识符
            }

            tokenCount++;
            cout << tokenCount << ": <" << token << "," << type << ">" << endl;
            return true;
        }

        // 处理字符串（分成三部分：引号、内容、引号）
        if (input[pos] == '"')
        {
            // 第一部分：左引号
            token = "\"";
            type = 78;
            tokenCount++;
            cout << tokenCount << ": <" << token << "," << type << ">" << endl;
            pos++; // 跳过左引号

            // 第二部分：字符串内容（作为标识符处理）
            if (pos < input.length() && input[pos] != '"')
            {
                token = handleStringContent();
                type = 81; // 标识符
                tokenCount++;
                cout << tokenCount << ": <" << token << "," << type << ">" << endl;
            }

            // 第三部分：右引号（如果有）
            if (pos < input.length() && input[pos] == '"')
            {
                token = "\"";
                type = 78;
                tokenCount++;
                cout << tokenCount << ": <" << token << "," << type << ">" << endl;
                pos++; // 跳过右引号
            }

            return getNextToken(token, type); // 继续获取下一个token
        }

        // 处理字符字面量
        if (input[pos] == '\'')
        {
            int start = pos;
            pos++; // 跳过开始的单引号

            // 处理转义字符
            if (pos < input.length() && input[pos] == '\\')
            {
                pos++;
            }

            if (pos < input.length())
            {
                pos++; // 跳过字符
            }

            if (pos < input.length() && input[pos] == '\'')
            {
                pos++; // 跳过结束的单引号
            }

            token = input.substr(start, pos - start);
            type = 80; // 常数（字符常量）
            tokenCount++;
            cout << tokenCount << ": <" << token << "," << type << ">" << endl;
            return true;
        }

        // 处理运算符和分隔符
        token = handleOperator();

        // 检查是否是已知的运算符
        if (keywordMap.find(token) != keywordMap.end())
        {
            type = keywordMap[token];
        }
        else
        {
            // 未知符号
            type = 0;
        }

        tokenCount++;
        cout << tokenCount << ": <" << token << "," << type << ">" << endl;
        return true;
    }

    // 分析整个程序
    void analyze()
    {
        string token;
        int type;

        while (getNextToken(token, type))
        {
            // 已经在getNextToken中输出
        }
    }
};

void Analysis()
{
    string prog;
    read_prog(prog);
    /********* Begin *********/

    LexAnalyser lexer(prog);
    lexer.analyze();

    /********* End *********/
}
