#include "DFA.h"
#include <algorithm>
#include <memory>
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

bool drop_global_bracket(std::string& str)
{
    if (str.front() == LEFT_BRACKET && str.back() == RIGHT_BRACKET)
    {
        int bracket_count = 0;
        for (int i = 0; i < str.length(); i++)
        {
            if (str[i] == LEFT_BRACKET)
                bracket_count++;
            else if (str[i] == RIGHT_BRACKET)
                bracket_count--;
            if (bracket_count == 0 && i != str.length() - 1)
                return 0;
        }
        str = str.substr(1, str.length() - 2);
        return 1;
    }
    return 0;
}

RE::RE(std::string pattern)
{
    this -> pattern = pattern;
}
RE::~RE() {}
std::string RE::getPattern()
{
    return pattern;
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
    std::sort(defination_patterns.begin() + 1, defination_patterns.end(), pattern_cmp);
    pattern.clear();
    for (int i = 1; i < defination_patterns.size(); i++)
    {
        for (int j = 0; j < defination_patterns.size(); j++)
        {
            if (j == i)
                continue;
            defination_patterns[j].second = std::regex_replace(
                defination_patterns[j].second,
                std::regex(defination_patterns[i].first),
                "(" + defination_patterns[i].second + ")");
        }
    }
    str_pattern = defination_patterns[0].second;
    for (auto itr = str_pattern.begin(); itr != str_pattern.end(); itr++)
    {
        if (isspace(*itr))
            continue;
        char to_push;
        if (*itr == '\\')
        {
            switch (*(itr + 1))
            {
            case 'n':
                to_push = '\n';
                itr++;
                break;
            case 't':
                to_push = '\t';
                itr++;
                break;
            case 'r':
                to_push = '\r';
                itr++;
                break;
            case '+':
                to_push = '+';
                itr++;
                break;
            case '*':
                to_push = '*';
                itr++;
                break;
            case '|':
                to_push = '|';
                itr++;
                break;
            case '(':
                to_push = '(';
                itr++;
                break;
            case ')':
                to_push = ')';
                itr++;
                break;
            case '.':
                to_push = '.';
                itr++;
                break;
            case '\\':
                to_push = '\\';
                itr++;
                break;
            default:
                to_push = *itr;
                break;
            }
        }
        else
        {
            switch (*itr)
            {
            case '+':
                to_push = PLUS;
                break;
            case '*':
                to_push = KLEENE_STAR;
                break;
            case '|':
                to_push = UNION;
                break;
            case '(':
                to_push = LEFT_BRACKET;
                break;
            case ')':
                to_push = RIGHT_BRACKET;
                break;
            case '.':
                to_push = CONCAT;
                break;
            default:
                to_push = *itr;
                break;
            }
        }
        if (!pattern.empty() && pattern.back() > ENUM_CNT && to_push > ENUM_CNT)
            pattern.push_back(CONCAT);
        else if (!pattern.empty() && pattern.back() == RIGHT_BRACKET && to_push > ENUM_CNT)
            pattern.push_back(CONCAT);
        else if (!pattern.empty() && pattern.back() > ENUM_CNT && to_push == LEFT_BRACKET)
            pattern.push_back(CONCAT);
        else if (!pattern.empty() && pattern.back() == RIGHT_BRACKET && to_push == LEFT_BRACKET)
            pattern.push_back(CONCAT);
        else if (!pattern.empty() && (pattern.back() == KLEENE_STAR || pattern.back() == PLUS) && to_push == LEFT_BRACKET)
            pattern.push_back(CONCAT);
        else if (!pattern.empty() && (pattern.back() == KLEENE_STAR || pattern.back() == PLUS) && to_push > ENUM_CNT)
            pattern.push_back(CONCAT);
        pattern.push_back(to_push);
    }
}
void RE::print_pattern()
{
    for (auto c : pattern)
    {
        if (c > ENUM_CNT)
            std::cout << c;
        else
        {
            switch (c)
            {
            case UNION:
                std::cout << "|";
                break;
            case CONCAT:
                std::cout << ".";
                break;
            case KLEENE_STAR:
                std::cout << "*";
                break;
            case PLUS:
                std::cout << "+";
                break;
            case LEFT_BRACKET:
                std::cout << "(";
                break;
            case RIGHT_BRACKET:
                std::cout << ")";
                break;
            default:
                break;
            }
        }
    }
    std::cout << std::endl;
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
    std::string pattern = pattern_obj.getPattern();
    assert(pattern.length() != 0);
    while(drop_global_bracket(pattern));
    int bracket_count = 0;
    if (pattern.length() == 1)
    {
        op = TERMINAL;
        value = pattern[0];
        left = nullptr;
        right = nullptr;
        return;
    }
    for (int i = 0; i < pattern.length(); i++)
    {
        if (pattern[i] == LEFT_BRACKET)
            bracket_count++;
        else if (pattern[i] == RIGHT_BRACKET)
            bracket_count--;
        else if (bracket_count == 0)
        {
            if (pattern[i] == UNION)
            {
                op = UNION;
                left = std::make_unique<RE_tree>(RE(pattern.substr(0, i)));
                right = std::make_unique<RE_tree>(RE(pattern.substr(i + 1)));
            }
            else if (pattern[i] == KLEENE_STAR)
            {
                op = KLEENE_STAR;
                left = std::make_unique<RE_tree>(RE(pattern.substr(0, i)));
                right = nullptr;
            }
            else if (pattern[i] == PLUS)
            {
                op = PLUS;
                left = std::make_unique<RE_tree>(RE(pattern.substr(0, i)));
                right = nullptr;
            }
            else if (pattern[i] == CONCAT)
            {
                op = CONCAT;
                left = std::make_unique<RE_tree>(RE(pattern.substr(0, i)));
                right = std::make_unique<RE_tree>(RE(pattern.substr(i + 1)));
            }
            return;
        }
    }
}
RE_tree::operator bool() const 
{
    if(op == TERMINAL)
        return value >= ENUM_CNT;
    else if(op == KLEENE_STAR || op == PLUS)
        return left != nullptr;
    else
        return left != nullptr && right != nullptr; 
}
RE_tree::~RE_tree()
{
}

