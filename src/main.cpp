#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <cctype>
#include <climits>
#include <iomanip>

using namespace std;

// ======================================================
// TOKEN
// ======================================================

struct Token
{
    string lexeme;
    string type;
    int line;
};

// ======================================================
// LEXICAL ERROR STRUCT
// ======================================================

struct LexicalError
{
    string category;
    string lexeme;
    string message;
    int line;
};

// ======================================================
// LEXER CLASS
// ======================================================

class Lexer
{
private:
    string source;
    int pos;
    int line;

    vector<Token> tokens;
    vector<LexicalError> errors;

    // ==================================================
    // KEYWORDS
    // ==================================================

    unordered_map<string, string> keywords =
    {
        {"enroll", "ENROLL"},
        {"graduate", "GRADUATE"},
        {"fixed", "RK"},
        {"repeat", "REPEAT"},
        {"check", "CHECK"},
        {"otherwise", "OTHERWISE"},
        {"print", "PRINT"},
        {"input", "INPUT"},
        {"score", "DT"},
        {"grade", "DT"}
    };

    // ==================================================
    // CHARACTER CHECKING
    // ==================================================

    bool isLetter(char c)
    {
        return isalpha((unsigned char)c);
    }

    bool isDigit(char c)
    {
        return isdigit((unsigned char)c);
    }

    bool isSpace(char c)
    {
        return isspace((unsigned char)c);
    }

    // ==================================================
    // ADD TOKEN
    // ==================================================

    void addToken(string lexeme, string type, int tokenLine)
    {
        tokens.push_back({
            lexeme,
            type,
            tokenLine
        });
    }

    // ==================================================
    // ERROR RECORDING & MESSAGE
    // ==================================================

    void error(string category, string lexeme, string message, int errorLine)
    {
        errors.push_back({category, lexeme, message, errorLine});
        cout << "[ERROR] Line " << errorLine << " (" << category << "): " << message << endl;
    }

    // ==================================================
    // CHECK LEGAL CHARACTER
    // ==================================================

    bool isLegalCharacter(char c)
    {
        if (isLetter(c)) return true;
        if (isDigit(c))  return true;
        if (c == '_')    return true;
        if (c == '"' || c == '\'') return true;

        // Operators
        if (c == '=' || c == '<' || c == '>' || c == '!' ||
            c == '&' || c == '|' || c == '+' || c == '-' ||
            c == '*' || c == '/' || c == '%')
        {
            return true;
        }

        // Special symbols
        if (c == '{' || c == '}' || c == '(' || c == ')' ||
            c == ';' || c == ',')
        {
            return true;
        }

        return false;
    }

    // ==================================================
    // IDENTIFIER DFA
    // [A-Za-z_][A-Za-z0-9_]*
    // ==================================================

    void identifierDFA()
    {
        string lexeme = "";
        int tokenLine = line;

        // First character
        lexeme += source[pos];
        pos++;

        // Remaining characters
        while (pos < source.length())
        {
            char c = source[pos];
            if (isLetter(c) || isDigit(c) || c == '_')
            {
                lexeme += c;
                pos++;
            }
            else
            {
                break;
            }
        }

        // Identifier length limit check (31 chars)
        if (lexeme.length() > 31)
        {
            error("Exceeding Limit", lexeme,
                  "Identifier '" + lexeme + "' exceeds maximum allowed length of 31 characters", tokenLine);
        }

        // Check keyword
        if (keywords.find(lexeme) != keywords.end())
        {
            addToken(lexeme, keywords[lexeme], tokenLine);
        }
        else
        {
            addToken(lexeme, "ID", tokenLine);
        }
    }

    // ==================================================
    // NUMBER DFA
    // [+-]?[0-9]+
    // 2 byte integer: range = -32768 to 32767
    // ==================================================

    void numberDFA()
    {
        string lexeme = "";
        int tokenLine = line;

        // Optional sign
        if (source[pos] == '+' || source[pos] == '-')
        {
            lexeme += source[pos];
            pos++;
        }

        // Digits
        while (pos < source.length() && isDigit(source[pos]))
        {
            lexeme += source[pos];
            pos++;
        }

        // =============================================
        // INVALID IDENTIFIER (e.g. 123abc, 1marks)
        // =============================================
        if (pos < source.length() && (isLetter(source[pos]) || source[pos] == '_'))
        {
            while (pos < source.length() && (isLetter(source[pos]) || isDigit(source[pos]) || source[pos] == '_'))
            {
                lexeme += source[pos];
                pos++;
            }

            error("Invalid Identifier", lexeme,
                  "Invalid Identifier '" + lexeme + "': identifiers cannot start with numbers/signs", tokenLine);
            return;
        }

        // =============================================
        // NUMERIC CONSTANT LIMIT (2 byte integer: -32768 to 32767)
        // =============================================
        try
        {
            long long value = stoll(lexeme);
            if (value < -32768 || value > 32767)
            {
                error("Exceeding Limit", lexeme,
                      "Numeric Constant '" + lexeme + "' exceeds 2-byte limit (-32768 to 32767)", tokenLine);
                return;
            }
        }
        catch (...)
        {
            error("Invalid Constant", lexeme,
                  "Invalid Numeric Constant '" + lexeme + "'", tokenLine);
            return;
        }

        addToken(lexeme, "NUM", tokenLine);
    }

    // ==================================================
    // STRING DFA
    // "Hello"
    // ==================================================

    void stringDFA()
    {
        string lexeme = "";
        int startLine = line;

        // Opening "
        lexeme += source[pos];
        pos++;

        while (pos < source.length())
        {
            char c = source[pos];

            // Closing "
            if (c == '"')
            {
                lexeme += c;
                pos++;
                addToken(lexeme, "STRING", startLine);
                return;
            }

            // New line before closing "
            if (c == '\n')
            {
                error("Unterminated String", lexeme,
                      "Unterminated String: missing closing double quote (\") before newline", startLine);
                line++;
                pos++;
                return;
            }

            lexeme += c;
            pos++;
        }

        // End of source
        error("Unterminated String", lexeme,
              "Unterminated String: missing closing double quote (\") at end of file", startLine);
    }

    // ==================================================
    // CHARACTER DFA
    // 'A'
    // ==================================================

    void characterDFA()
    {
        string lexeme = "";
        int startLine = line;

        // Opening '
        lexeme += source[pos];
        pos++;

        // No character
        if (pos >= source.length())
        {
            error("Unterminated Character", lexeme,
                  "Unterminated Character: missing character and closing quote", startLine);
            return;
        }

        // Check for empty character ''
        if (source[pos] == '\'')
        {
            lexeme += source[pos];
            pos++;
            error("Unclosed Error", lexeme,
                  "Empty character literal is not allowed", startLine);
            return;
        }

        // Read character content
        string content = "";
        while (pos < source.length() && source[pos] != '\'' && source[pos] != '\n')
        {
            content += source[pos];
            pos++;
        }

        lexeme += content;

        // New line or EOF without closing quote
        if (pos >= source.length() || source[pos] == '\n')
        {
            error("Unterminated Character", lexeme,
                  "Unterminated Character: missing closing single quote (')", startLine);
            return;
        }

        // Closing '
        lexeme += source[pos];
        pos++;

        if (content.length() > 1)
        {
            error("Exceeding Limit", lexeme,
                  "Character literal exceeds 1-character limit (" + to_string(content.length()) + " characters found in '" + content + "')", startLine);
            return;
        }

        addToken(lexeme, "CHAR", startLine);
    }

    // ==================================================
    // COMMENT DFA
    // // comment or /* comment */
    // ==================================================

    void commentDFA()
    {
        int startLine = line;

        // Single line comment
        if (source[pos] == '/' && pos + 1 < source.length() && source[pos + 1] == '/')
        {
            pos += 2;
            while (pos < source.length() && source[pos] != '\n')
            {
                pos++;
            }
            return;
        }

        // Multi line comment
        if (source[pos] == '/' && pos + 1 < source.length() && source[pos + 1] == '*')
        {
            pos += 2;
            while (pos < source.length())
            {
                if (source[pos] == '*' && pos + 1 < source.length() && source[pos + 1] == '/')
                {
                    pos += 2;
                    return;
                }

                if (source[pos] == '\n')
                {
                    line++;
                }
                pos++;
            }

            error("Unclosed Comment", "/*...",
                  "Unclosed Comment: missing closing '*/'", startLine);
            return;
        }
    }

    // ==================================================
    // = OR ==
    // ==================================================

    void equalDFA()
    {
        string lexeme = "";
        int tokenLine = line;

        lexeme += source[pos];
        pos++;

        if (pos < source.length() && source[pos] == '=')
        {
            lexeme += source[pos];
            pos++;
            addToken(lexeme, "==", tokenLine);
        }
        else
        {
            addToken(lexeme, "=", tokenLine);
        }
    }

    // ==================================================
    // < OR <=
    // ==================================================

    void lessDFA()
    {
        string lexeme = "";
        int tokenLine = line;

        lexeme += source[pos];
        pos++;

        if (pos < source.length() && source[pos] == '=')
        {
            lexeme += source[pos];
            pos++;
            addToken(lexeme, "<=", tokenLine);
        }
        else
        {
            addToken(lexeme, "<", tokenLine);
        }
    }

    // ==================================================
    // > OR >=
    // ==================================================

    void greaterDFA()
    {
        string lexeme = "";
        int tokenLine = line;

        lexeme += source[pos];
        pos++;

        if (pos < source.length() && source[pos] == '=')
        {
            lexeme += source[pos];
            pos++;
            addToken(lexeme, ">=", tokenLine);
        }
        else
        {
            addToken(lexeme, ">", tokenLine);
        }
    }

    // ==================================================
    // ! OR !=
    // ==================================================

    void notDFA()
    {
        string lexeme = "";
        int tokenLine = line;

        lexeme += source[pos];
        pos++;

        if (pos < source.length() && source[pos] == '=')
        {
            lexeme += source[pos];
            pos++;
            addToken(lexeme, "!=", tokenLine);
        }
        else
        {
            addToken(lexeme, "!", tokenLine);
        }
    }

    // ==================================================
    // &&
    // ==================================================

    void andDFA()
    {
        string lexeme = "";
        int tokenLine = line;

        lexeme += source[pos];
        pos++;

        if (pos < source.length() && source[pos] == '&')
        {
            lexeme += source[pos];
            pos++;
            addToken(lexeme, "&&", tokenLine);
        }
        else
        {
            error("Illegal Character", "&",
                  "Invalid '&' operator: single '&' is not allowed (did you mean '&&'?)", tokenLine);
        }
    }

    // ==================================================
    // ||
    // ==================================================

    void orDFA()
    {
        string lexeme = "";
        int tokenLine = line;

        lexeme += source[pos];
        pos++;

        if (pos < source.length() && source[pos] == '|')
        {
            lexeme += source[pos];
            pos++;
            addToken(lexeme, "||", tokenLine);
        }
        else
        {
            error("Illegal Character", "|",
                  "Invalid '|' operator: single '|' is not allowed (did you mean '||'?)", tokenLine);
        }
    }

    // ==================================================
    // + - * / %
    // ==================================================

    void arithmeticDFA()
    {
        string lexeme = "";
        int tokenLine = line;

        lexeme += source[pos];
        pos++;

        addToken(lexeme, lexeme, tokenLine);
    }

    // ==================================================
    // SPECIAL SYMBOLS
    // ==================================================

    void specialDFA()
    {
        string lexeme = "";
        int tokenLine = line;

        lexeme += source[pos];
        pos++;

        addToken(lexeme, lexeme, tokenLine);
    }

    // ==================================================
    // MASTER DFA
    // ==================================================

    void masterDFA()
    {
        while (pos < source.length())
        {
            char c = source[pos];

            // ==========================================
            // WHITESPACE
            // ==========================================
            if (c == '\n')
            {
                line++;
                pos++;
                continue;
            }

            if (c == ' ' || c == '\t' || c == '\r')
            {
                pos++;
                continue;
            }

            // ==========================================
            // COMMENT
            // ==========================================
            if (c == '/' && pos + 1 < source.length() &&
                (source[pos + 1] == '/' || source[pos + 1] == '*'))
            {
                commentDFA();
                continue;
            }

            // ==========================================
            // ERROR: ILLEGAL / INVALID CHARACTER
            // ==========================================
            if (!isLegalCharacter(c))
            {
                string errLexeme(1, c);
                error("Illegal Character", errLexeme,
                      "Illegal Character '" + errLexeme + "'", line);
                pos++;
                continue;
            }

            // ==========================================
            // IDENTIFIER
            // ==========================================
            if (isLetter(c) || c == '_')
            {
                identifierDFA();
                continue;
            }

            // ==========================================
            // NUMBER (Signed or Unsigned)
            // ==========================================
            if (isDigit(c) || ((c == '+' || c == '-') && pos + 1 < source.length() && isDigit(source[pos + 1])))
            {
                numberDFA();
                continue;
            }

            // ==========================================
            // STRING
            // ==========================================
            if (c == '"')
            {
                stringDFA();
                continue;
            }

            // ==========================================
            // CHARACTER
            // ==========================================
            if (c == '\'')
            {
                characterDFA();
                continue;
            }

            // ==========================================
            // = OR ==
            // ==========================================
            if (c == '=')
            {
                equalDFA();
                continue;
            }

            // ==========================================
            // < OR <=
            // ==========================================
            if (c == '<')
            {
                lessDFA();
                continue;
            }

            // ==========================================
            // > OR >=
            // ==========================================
            if (c == '>')
            {
                greaterDFA();
                continue;
            }

            // ==========================================
            // ! OR !=
            // ==========================================
            if (c == '!')
            {
                notDFA();
                continue;
            }

            // ==========================================
            // &&
            // ==========================================
            if (c == '&')
            {
                andDFA();
                continue;
            }

            // ==========================================
            // ||
            // ==========================================
            if (c == '|')
            {
                orDFA();
                continue;
            }

            // ==========================================
            // + - * / %
            // ==========================================
            if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%')
            {
                arithmeticDFA();
                continue;
            }

            // ==========================================
            // SPECIAL SYMBOLS
            // ==========================================
            if (c == '{' || c == '}' || c == '(' || c == ')' ||
                c == ';' || c == ',')
            {
                specialDFA();
                continue;
            }

            pos++;
        }
    }

public:
    // ==================================================
    // CONSTRUCTOR
    // ==================================================
    Lexer(string input)
    {
        source = input;
        pos = 0;
        line = 1;

        // Skip UTF-8 BOM if present
        if (source.length() >= 3 &&
            (unsigned char)source[0] == 0xEF &&
            (unsigned char)source[1] == 0xBB &&
            (unsigned char)source[2] == 0xBF)
        {
            pos = 3;
        }
    }

    // ==================================================
    // PROGRAM BOUNDARY CHECK (enroll ... graduate)
    // ==================================================
    void checkProgramBoundaries()
    {
        // 1. Program must only start with 'enroll'
        if (tokens.empty() || tokens.front().type != "ENROLL")
        {
            int errLine = tokens.empty() ? 1 : tokens.front().line;
            string lex = tokens.empty() ? "-" : tokens.front().lexeme;
            error("Program Structure", lex,
                  "Program must start with 'enroll' keyword", errLine);
        }

        // 2. Program must always end with 'graduate'
        if (tokens.empty() || tokens.back().type != "GRADUATE")
        {
            int errLine = tokens.empty() ? (line > 1 ? line - 1 : 1) : tokens.back().line;
            string lex = tokens.empty() ? "-" : tokens.back().lexeme;
            error("Program Structure", lex,
                  "Program must end with 'graduate' keyword", errLine);
        }
    }

    // ==================================================
    // TOKENIZE
    // ==================================================
    void tokenize()
    {
        masterDFA();
        checkProgramBoundaries();
    }

    // ==================================================
    // PRINT TOKENS & SAVE TO FILE
    // ==================================================
    void printTokens(const string& outputPath = "output/tokens.txt")
    {
        ofstream outFile(outputPath);

        cout << "\n=================================================================\n";
        cout << "                      EduLang Token Stream                       \n";
        cout << "=================================================================\n";
        cout << left << setw(28) << "Lexeme" 
             << left << setw(18) << "Token" 
             << left << setw(10) << "Line" << "\n";
        cout << "-----------------------------------------------------------------\n";

        if (outFile.is_open())
        {
            outFile << "Line\t\tLexeme\t\t\tToken\n";
            outFile << "--------------------------------------------------------\n";
        }

        for (const Token& t : tokens)
        {
            cout << left << setw(28) << t.lexeme 
                 << left << setw(18) << t.type 
                 << left << setw(10) << t.line << "\n";

            if (outFile.is_open())
            {
                outFile << t.line << "\t\t" << t.lexeme << "\t\t\t" << t.type << "\n";
            }
        }

        cout << "=================================================================\n";

        if (!errors.empty())
        {
            cout << "\n======================================================================================================================\n";
            cout << "                                            LEXICAL ERRORS (CATEGORY-WISE)                                            \n";
            cout << "======================================================================================================================\n";
            cout << left << setw(24) << "Error Category" 
                 << left << setw(28) << "Lexeme" 
                 << left << setw(8)  << "Line" 
                 << left << "Error Description" << "\n";
            cout << "----------------------------------------------------------------------------------------------------------------------\n";

            if (outFile.is_open())
            {
                outFile << "\n======================================================================================================================\n";
                outFile << "                                            LEXICAL ERRORS (CATEGORY-WISE)                                            \n";
                outFile << "======================================================================================================================\n";
                outFile << "Category\t\tLexeme\t\tLine\tDescription\n";
                outFile << "----------------------------------------------------------------------------------------------------------------------\n";
            }

            unordered_map<string, int> counts;
            for (const auto& err : errors)
            {
                counts[err.category]++;
                cout << left << setw(24) << err.category 
                     << left << setw(28) << err.lexeme 
                     << left << setw(8)  << err.line 
                     << left << err.message << "\n";

                if (outFile.is_open())
                {
                    outFile << err.category << "\t\t" << err.lexeme << "\t\t" << err.line << "\t" << err.message << "\n";
                }
            }

            cout << "----------------------------------------------------------------------------------------------------------------------\n";
            cout << " Error Summary:\n";
            for (const auto& pair : counts)
            {
                cout << "   - " << left << setw(24) << (pair.first + ":") << pair.second << " error(s)\n";
            }
            cout << " Total Lexical Errors: " << errors.size() << "\n";
            cout << "======================================================================================================================\n";
        }
        else
        {
            cout << "\n [SUCCESS] Lexical analysis completed with 0 errors!\n";
        }

        if (outFile.is_open())
        {
            outFile.close();
            cout << "Results saved to: " << outputPath << "\n\n";
        }
    }

    const vector<Token>& getTokens() const { return tokens; }
    const vector<LexicalError>& getErrors() const { return errors; }
};

// ======================================================
// INTERACTIVE FUNCTIONS
// ======================================================

void runFileMode()
{
    cout << "\nEnter path to .edu source file (e.g. tests/test_program.edu): ";
    string filePath;
    getline(cin, filePath);

    if (filePath.empty())
    {
        filePath = "tests/test_program.edu";
        cout << "Defaulting to: " << filePath << "\n";
    }

    ifstream inFile(filePath);
    if (!inFile.is_open())
    {
        cerr << "Error: Could not open file '" << filePath << "'\n";
        return;
    }

    stringstream buffer;
    buffer << inFile.rdbuf();
    inFile.close();

    Lexer lexer(buffer.str());
    lexer.tokenize();
    lexer.printTokens("output/tokens.txt");
}

void runInteractiveInputMode()
{
    cout << "\n-----------------------------------------------------------------\n";
    cout << " Enter EduLang code line by line.\n";
    cout << " Note: Program must start with 'enroll' and end with 'graduate'.\n";
    cout << " Type 'END' on a new line or end with 'graduate' to finish input.\n";
    cout << "-----------------------------------------------------------------\n";

    string sourceCode = "";
    string line;

    while (true)
    {
        if (!getline(cin, line)) break;
        if (line == "END" || line == "end") break;

        sourceCode += line + "\n";

        // Check if user entered 'graduate' to finish input
        size_t first = line.find_first_not_of(" \t\r\n");
        size_t last = line.find_last_not_of(" \t\r\n");
        if (first != string::npos)
        {
            string trimmed = line.substr(first, last - first + 1);
            if (trimmed == "graduate" || (trimmed.length() >= 8 && trimmed.rfind("graduate") == trimmed.length() - 8))
            {
                break;
            }
        }
    }

    if (sourceCode.empty())
    {
        cout << "No input provided.\n";
        return;
    }

    Lexer lexer(sourceCode);
    lexer.tokenize();
    lexer.printTokens("output/tokens.txt");
}

void runErrorDemo()
{
    string code = R"(enroll
{
    score marks = @75;
    score 1marks = 50;
    print("Hello bhai);
    score total = 50000;
    grade result = 'A';
    check(marks >= 50)
    {
        print("Pass");
    }

    /*
       This comment is NOT CLOSED
)";

    cout << "\n=======================================================\n";
    cout << "              EDULANG ERROR DEMO INPUT                 \n";
    cout << "=======================================================\n";
    cout << code << "\n";
    cout << "=======================================================\n";

    Lexer lexer(code);
    lexer.tokenize();
    lexer.printTokens("output/error_tokens.txt");
}

// ======================================================
// MAIN
// ======================================================

int main(int argc, char* argv[])
{
    if (argc > 1)
    {
        string inputPath = argv[1];
        string outputPath = (argc > 2) ? argv[2] : "output/tokens.txt";

        ifstream inFile(inputPath);
        if (!inFile.is_open())
        {
            cerr << "Error: Could not open file: " << inputPath << "\n";
            return 1;
        }

        stringstream buffer;
        buffer << inFile.rdbuf();
        inFile.close();

        Lexer lexer(buffer.str());
        lexer.tokenize();
        lexer.printTokens(outputPath);
        return 0;
    }

    while (true)
    {
        cout << "\n=======================================================\n";
        cout << "         EduLang Lexical Analyzer - Main Menu          \n";
        cout << "=======================================================\n";
        cout << " 1. Type / Paste EduLang code directly\n";
        cout << " 2. Load and analyze an EduLang file (.edu)\n";
        cout << " 3. Run default sample test program (Section 20)\n";
        cout << " 4. Run 5-Error demonstration\n";
        cout << " 5. Exit\n";
        cout << "-------------------------------------------------------\n";
        cout << "Enter your choice (1-5): ";

        string choice;
        if (!getline(cin, choice)) break;

        if (choice == "1")
        {
            runInteractiveInputMode();
        }
        else if (choice == "2")
        {
            runFileMode();
        }
        else if (choice == "3")
        {
            ifstream inFile("tests/test_program.edu");
            if (inFile.is_open())
            {
                stringstream buffer;
                buffer << inFile.rdbuf();
                inFile.close();
                Lexer lexer(buffer.str());
                lexer.tokenize();
                lexer.printTokens("output/tokens.txt");
            }
            else
            {
                cerr << "Error: tests/test_program.edu not found.\n";
            }
        }
        else if (choice == "4")
        {
            runErrorDemo();
        }
        else if (choice == "5" || choice == "exit" || choice == "q")
        {
            cout << "Exiting EduLang Lexical Analyzer. Goodbye!\n";
            break;
        }
        else
        {
            cout << "Invalid choice! Please enter a number between 1 and 5.\n";
        }
    }

    return 0;
}
