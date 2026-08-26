#ifndef TOKEN_H
#define TOKEN_H

#include <string>
#include <iostream>

enum class TokenType {
    // Keywords
    ENROLL,
    GRADUATE,
    REPEAT,
    CHECK,
    OTHERWISE,
    PRINT,
    INPUT,
    RK,         // fixed

    // Datatypes
    DT,         // score, grade

    // Identifiers & Literals
    ID,
    NUM,
    CHAR_LIT,
    STRING_LIT,

    // Operators
    ASSIGN,     // =
    EQUAL,      // ==
    LT,         // <
    LTE,        // <=
    GT,         // >
    GTE,        // >=
    NOT,        // !
    NEQ,        // !=
    AND,        // &&
    OR,         // ||
    PLUS,       // +
    MINUS,      // -
    MUL,        // *
    DIV,        // /
    MOD,        // %

    // Special Symbols
    LBRACE,     // {
    RBRACE,     // }
    LPAREN,     // (
    RPAREN,     // )
    SEMI,       // ;
    COMMA,      // ,

    // Meta / Error
    TOKEN_ERROR,
    END_OF_FILE
};

enum class ErrorCategory {
    ILLEGAL_CHARACTER,
    INVALID_CHARACTER,
    UNTERMINATED_ERROR,
    UNCLOSED_ERROR,
    EXCEEDING_LIMIT
};

struct LexicalError {
    ErrorCategory category;
    std::string categoryStr;
    std::string lexeme;
    std::string message;
    int line;
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;

    std::string typeToString() const {
        switch (type) {
            case TokenType::ENROLL:      return "ENROLL";
            case TokenType::GRADUATE:    return "GRADUATE";
            case TokenType::REPEAT:      return "REPEAT";
            case TokenType::CHECK:       return "CHECK";
            case TokenType::OTHERWISE:   return "OTHERWISE";
            case TokenType::PRINT:       return "PRINT";
            case TokenType::INPUT:       return "INPUT";
            case TokenType::RK:          return "RK";
            case TokenType::DT:          return "DT";
            case TokenType::ID:          return "ID";
            case TokenType::NUM:         return "NUM";
            case TokenType::CHAR_LIT:    return "CHAR";
            case TokenType::STRING_LIT:  return "STRING";
            case TokenType::ASSIGN:      return "=";
            case TokenType::EQUAL:       return "==";
            case TokenType::LT:          return "<";
            case TokenType::LTE:         return "<=";
            case TokenType::GT:          return ">";
            case TokenType::GTE:         return ">=";
            case TokenType::NOT:         return "!";
            case TokenType::NEQ:         return "!=";
            case TokenType::AND:         return "&&";
            case TokenType::OR:          return "||";
            case TokenType::PLUS:        return "+";
            case TokenType::MINUS:       return "-";
            case TokenType::MUL:         return "*";
            case TokenType::DIV:         return "/";
            case TokenType::MOD:         return "%";
            case TokenType::LBRACE:      return "{";
            case TokenType::RBRACE:      return "}";
            case TokenType::LPAREN:      return "(";
            case TokenType::RPAREN:      return ")";
            case TokenType::SEMI:        return ";";
            case TokenType::COMMA:       return ",";
            case TokenType::TOKEN_ERROR: return "ERROR";
            case TokenType::END_OF_FILE: return "EOF";
            default:                     return "UNKNOWN";
        }
    }
};

#endif // TOKEN_H
