#ifndef KEYWORDS_H
#define KEYWORDS_H

#include <string>
#include <unordered_map>
#include "Token.h"

inline bool isKeyword(const std::string& lexeme, TokenType& outType) {
    static const std::unordered_map<std::string, TokenType> KEYWORDS = {
        {"enroll",    TokenType::ENROLL},
        {"graduate",  TokenType::GRADUATE},
        {"fixed",     TokenType::RK},
        {"repeat",    TokenType::REPEAT},
        {"check",     TokenType::CHECK},
        {"otherwise", TokenType::OTHERWISE},
        {"print",     TokenType::PRINT},
        {"input",     TokenType::INPUT},
        {"score",     TokenType::DT},
        {"grade",     TokenType::DT}
    };

    auto it = KEYWORDS.find(lexeme);
    if (it != KEYWORDS.end()) {
        outType = it->second;
        return true;
    }
    return false;
}

#endif // KEYWORDS_H
