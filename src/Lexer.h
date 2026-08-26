#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include "Token.h"

enum class DFAState {
    q0,
    qID,
    qNUM,
    qSIGN,
    qCHAR1,
    qCHAR2,
    qCHAR_ACCEPT,
    qSTRING,
    qSTRING_ACCEPT,
    qEQ,
    qEQUAL,
    qLT,
    qLTE,
    qGT,
    qGTE,
    qNOT,
    qNE,
    qAND,
    qAND_ACCEPT,
    qOR,
    qOR_ACCEPT,
    qPLUS,
    qMINUS,
    qMUL,
    qDIV,
    qMOD,
    qLB,
    qRB,
    qLP,
    qRP,
    qSC,
    qCOM,
    qERROR
};

class Lexer {
public:
    explicit Lexer(const std::string& source);
    std::vector<Token> tokenize();
    Token nextToken();

    const std::vector<LexicalError>& getErrors() const { return errors; }
    bool hasErrors() const { return !errors.empty(); }

private:
    std::string src;
    size_t pos;
    int line;
    std::vector<LexicalError> errors;

    static const size_t MAX_ID_LENGTH = 31;
    static const size_t MAX_NUM_DIGITS = 10;

    char peek() const;
    char peekAhead(size_t offset = 1) const;
    char advance();
    bool isAtEnd() const;
    void skipWhitespace();

    void recordError(ErrorCategory cat, const std::string& catStr, const std::string& lexeme, const std::string& msg, int errLine);

    static bool isLetter(char c);
    static bool isDigit(char c);
};

#endif // LEXER_H
