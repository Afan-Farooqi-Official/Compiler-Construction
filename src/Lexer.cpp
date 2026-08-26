#include "Lexer.h"
#include "Keywords.h"
#include <cctype>

Lexer::Lexer(const std::string& source) : src(source), pos(0), line(1) {
    // Skip UTF-8 BOM if present
    if (src.length() >= 3 && 
        (unsigned char)src[0] == 0xEF && 
        (unsigned char)src[1] == 0xBB && 
        (unsigned char)src[2] == 0xBF) {
        pos = 3;
    }
}

void Lexer::recordError(ErrorCategory cat, const std::string& catStr, const std::string& lexeme, const std::string& msg, int errLine) {
    errors.push_back(LexicalError{cat, catStr, lexeme, msg, errLine});
}

bool Lexer::isLetter(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool Lexer::isDigit(char c) {
    return c >= '0' && c <= '9';
}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return src[pos];
}

char Lexer::peekAhead(size_t offset) const {
    if (pos + offset >= src.length()) return '\0';
    return src[pos + offset];
}

char Lexer::advance() {
    if (isAtEnd()) return '\0';
    return src[pos++];
}

bool Lexer::isAtEnd() const {
    return pos >= src.length();
}

void Lexer::skipWhitespace() {
    while (!isAtEnd()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r') {
            advance();
        } else if (c == '\n') {
            line++;
            advance();
        } else {
            break;
        }
    }
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (true) {
        Token tok = nextToken();
        tokens.push_back(tok);
        if (tok.type == TokenType::END_OF_FILE) {
            break;
        }
    }
    return tokens;
}

Token Lexer::nextToken() {
    skipWhitespace();

    if (isAtEnd()) {
        return Token{TokenType::END_OF_FILE, "", line};
    }

    int tokenLine = line;
    char c = peek();

    // 1. Identifier / Keyword DFA (q0 -> qID)
    if (isLetter(c) || c == '_') {
        std::string lexeme = "";
        lexeme += advance();

        while (!isAtEnd()) {
            char next = peek();
            if (isLetter(next) || isDigit(next) || next == '_') {
                lexeme += advance();
            } else {
                break;
            }
        }

        // Check exceeding limit error (Identifier max length)
        if (lexeme.length() > MAX_ID_LENGTH) {
            recordError(ErrorCategory::EXCEEDING_LIMIT, "Exceeding limit", lexeme,
                        "Identifier exceeds maximum allowed length of " + std::to_string(MAX_ID_LENGTH) + " characters", tokenLine);
        }

        TokenType kwType;
        if (isKeyword(lexeme, kwType)) {
            return Token{kwType, lexeme, tokenLine};
        }
        return Token{TokenType::ID, lexeme, tokenLine};
    }

    // 2. Number DFA with optional Sign (q0 -> qSIGN -> qNUM or q0 -> qNUM)
    if ((c == '+' || c == '-') && isDigit(peekAhead(1))) {
        std::string lexeme = "";
        lexeme += advance(); // consume sign

        size_t digitCount = 0;
        while (!isAtEnd() && isDigit(peek())) {
            lexeme += advance();
            digitCount++;
        }

        // Check if immediately followed by an invalid character (e.g. 1student)
        if (!isAtEnd() && (isLetter(peek()) || peek() == '_')) {
            while (!isAtEnd() && (isLetter(peek()) || isDigit(peek()) || peek() == '_')) {
                lexeme += advance();
            }
            recordError(ErrorCategory::INVALID_CHARACTER, "Invalid character", lexeme,
                        "Malformed identifier/number: identifier cannot start with digits/sign", tokenLine);
            return Token{TokenType::TOKEN_ERROR, lexeme, tokenLine};
        }

        if (digitCount > MAX_NUM_DIGITS) {
            recordError(ErrorCategory::EXCEEDING_LIMIT, "Exceeding limit", lexeme,
                        "Numeric literal exceeds maximum allowed limit of " + std::to_string(MAX_NUM_DIGITS) + " digits", tokenLine);
        }

        return Token{TokenType::NUM, lexeme, tokenLine};
    }

    if (isDigit(c)) {
        std::string lexeme = "";
        size_t digitCount = 0;
        while (!isAtEnd() && isDigit(peek())) {
            lexeme += advance();
            digitCount++;
        }

        // Check if immediately followed by a letter (e.g., 1student)
        if (!isAtEnd() && (isLetter(peek()) || peek() == '_')) {
            while (!isAtEnd() && (isLetter(peek()) || isDigit(peek()) || peek() == '_')) {
                lexeme += advance();
            }
            recordError(ErrorCategory::INVALID_CHARACTER, "Invalid character", lexeme,
                        "Malformed identifier: identifiers cannot start with a digit", tokenLine);
            return Token{TokenType::TOKEN_ERROR, lexeme, tokenLine};
        }

        if (digitCount > MAX_NUM_DIGITS) {
            recordError(ErrorCategory::EXCEEDING_LIMIT, "Exceeding limit", lexeme,
                        "Numeric literal exceeds maximum allowed limit of " + std::to_string(MAX_NUM_DIGITS) + " digits", tokenLine);
        }

        return Token{TokenType::NUM, lexeme, tokenLine};
    }

    // 3. Character DFA (q0 -> qCHAR1 -> qCHAR2 -> qCHAR_ACCEPT)
    if (c == '\'') {
        std::string lexeme = "";
        lexeme += advance(); // consume opening '

        // Check for empty character ''
        if (!isAtEnd() && peek() == '\'') {
            lexeme += advance(); // consume ''
            recordError(ErrorCategory::UNCLOSED_ERROR, "Unclosed error", lexeme,
                        "Empty character literal is not allowed", tokenLine);
            return Token{TokenType::TOKEN_ERROR, lexeme, tokenLine};
        }

        std::string content = "";
        while (!isAtEnd() && peek() != '\'' && peek() != '\n') {
            content += advance();
        }

        lexeme += content;

        if (isAtEnd() || peek() == '\n') {
            // Missing closing quote
            recordError(ErrorCategory::UNCLOSED_ERROR, "Unclosed error", lexeme,
                        "Unclosed character literal: missing closing quote ('')", tokenLine);
            return Token{TokenType::TOKEN_ERROR, lexeme, tokenLine};
        }

        // We found closing quote
        lexeme += advance(); // consume closing '

        if (content.length() > 1) {
            // Exceeding limit error (more than 1 char)
            recordError(ErrorCategory::EXCEEDING_LIMIT, "Exceeding limit", lexeme,
                        "Character literal exceeds 1-character limit (" + std::to_string(content.length()) + " characters found)", tokenLine);
            return Token{TokenType::TOKEN_ERROR, lexeme, tokenLine};
        }

        return Token{TokenType::CHAR_LIT, lexeme, tokenLine};
    }

    // 4. String DFA (q0 -> qSTRING -> qSTRING_ACCEPT)
    if (c == '"') {
        std::string lexeme = "";
        lexeme += advance(); // consume opening "

        while (!isAtEnd() && peek() != '"' && peek() != '\n') {
            lexeme += advance();
        }

        if (isAtEnd() || peek() == '\n') {
            // Unterminated string error
            recordError(ErrorCategory::UNTERMINATED_ERROR, "Unterminated error", lexeme,
                        "Unterminated string literal: missing closing double quote (\")", tokenLine);
            return Token{TokenType::TOKEN_ERROR, lexeme, tokenLine};
        }

        lexeme += advance(); // consume closing "
        return Token{TokenType::STRING_LIT, lexeme, tokenLine};
    }

    // 5. Assignment & Equality Operators (q0 -> qEQ / qEQUAL)
    if (c == '=') {
        advance();
        if (!isAtEnd() && peek() == '=') {
            advance();
            return Token{TokenType::EQUAL, "==", tokenLine};
        }
        return Token{TokenType::ASSIGN, "=", tokenLine};
    }

    // 6. Less Than Operators (q0 -> qLT / qLTE)
    if (c == '<') {
        advance();
        if (!isAtEnd() && peek() == '=') {
            advance();
            return Token{TokenType::LTE, "<=", tokenLine};
        }
        return Token{TokenType::LT, "<", tokenLine};
    }

    // 7. Greater Than Operators (q0 -> qGT / qGTE)
    if (c == '>') {
        advance();
        if (!isAtEnd() && peek() == '=') {
            advance();
            return Token{TokenType::GTE, ">=", tokenLine};
        }
        return Token{TokenType::GT, ">", tokenLine};
    }

    // 8. NOT Operators (q0 -> qNOT / qNE)
    if (c == '!') {
        advance();
        if (!isAtEnd() && peek() == '=') {
            advance();
            return Token{TokenType::NEQ, "!=", tokenLine};
        }
        return Token{TokenType::NOT, "!", tokenLine};
    }

    // 9. Logical AND (q0 -> qAND -> qAND_ACCEPT)
    if (c == '&') {
        advance();
        if (!isAtEnd() && peek() == '&') {
            advance();
            return Token{TokenType::AND, "&&", tokenLine};
        }
        recordError(ErrorCategory::ILLEGAL_CHARACTER, "Illegal character", "&",
                    "Illegal character: single '&' is not allowed in EduLang (did you mean '&&'?)", tokenLine);
        return Token{TokenType::TOKEN_ERROR, "&", tokenLine};
    }

    // 10. Logical OR (q0 -> qOR -> qOR_ACCEPT)
    if (c == '|') {
        advance();
        if (!isAtEnd() && peek() == '|') {
            advance();
            return Token{TokenType::OR, "||", tokenLine};
        }
        recordError(ErrorCategory::ILLEGAL_CHARACTER, "Illegal character", "|",
                    "Illegal character: single '|' is not allowed in EduLang (did you mean '||'?)", tokenLine);
        return Token{TokenType::TOKEN_ERROR, "|", tokenLine};
    }

    // 11. Single-character Arithmetic Operators
    if (c == '+') {
        advance();
        return Token{TokenType::PLUS, "+", tokenLine};
    }
    if (c == '-') {
        advance();
        return Token{TokenType::MINUS, "-", tokenLine};
    }
    if (c == '*') {
        advance();
        return Token{TokenType::MUL, "*", tokenLine};
    }
    if (c == '/') {
        advance();
        return Token{TokenType::DIV, "/", tokenLine};
    }
    if (c == '%') {
        advance();
        return Token{TokenType::MOD, "%", tokenLine};
    }

    // 12. Special Symbols
    if (c == '{') {
        advance();
        return Token{TokenType::LBRACE, "{", tokenLine};
    }
    if (c == '}') {
        advance();
        return Token{TokenType::RBRACE, "}", tokenLine};
    }
    if (c == '(') {
        advance();
        return Token{TokenType::LPAREN, "(", tokenLine};
    }
    if (c == ')') {
        advance();
        return Token{TokenType::RPAREN, ")", tokenLine};
    }
    if (c == ';') {
        advance();
        return Token{TokenType::SEMI, ";", tokenLine};
    }
    if (c == ',') {
        advance();
        return Token{TokenType::COMMA, ",", tokenLine};
    }

    // Invalid character not part of EduLang language
    std::string errLexeme(1, advance());
    recordError(ErrorCategory::INVALID_CHARACTER, "Invalid character", errLexeme,
                "Invalid character '" + errLexeme + "' is not recognized in EduLang", tokenLine);
    return Token{TokenType::TOKEN_ERROR, errLexeme, tokenLine};
}
