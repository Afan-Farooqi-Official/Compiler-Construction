# 🎓 EduLang Lexical Analyzer (Lexer)

A fast, DFA-based **Lexical Analyzer (Lexer)** for **EduLang** — an educational programming language built for compiler design. It converts EduLang source code into a clean stream of tokens and reports detailed, categorized errors with exact line numbers.

---

## ⚡ Quick Start (Run in 10 Seconds)

### 1. Compile
```bash
g++ -std=c++17 src/main.cpp -o edulang_lexer.exe
```

### 2. Run
```powershell
# Interactive menu:
.\edulang_lexer.exe

# Or test a file directly:
.\edulang_lexer.exe tests/test_program.edu
```

---

## 📁 Project Files & Logic (What Each File Does)

| File | Purpose & Logic |
|---|---|
| **`src/main.cpp`** | **The main engine.** Contains the entire Lexer implementation: DFA state machines, comment stripping, boundary checks (`enroll`/`graduate`), interactive menu, file reader, and error reporting table. |
| **`src/Token.h`** | **Data models.** Defines `Token` struct, `TokenType` enum (keywords, operators, delimiters), and `ErrorCategory` enum. |
| **`src/Keywords.h`** | **Lookup table.** Stores reserved keywords in a fast hash map for $O(1)$ lookup (`enroll`, `graduate`, `score`, `grade`, etc.). |
| **`src/Lexer.h` & `Lexer.cpp`** | **Modular Lexer class.** Reusable class version of the lexer with state enums and lookahead methods, ready to plug into a future parser. |
| **`tests/test_program.edu`** | **Valid test program.** A student grading script using variables, if-else, chars, and print. Runs with **0 errors**. |
| **`tests/edge_cases.edu`** | **Advanced test.** Tests signed numbers (`+100`, `-20`), complex math, logical operators (`&&`, `||`, `!`), and loops. |
| **`tests/error_demo.edu`** | **Error test.** Intentionally triggers all 11 lexical errors to demonstrate error detection and recovery. |
| **`output/tokens.txt`** | **Output log.** Automatically saves the token table and error summary after each run. |

---

## 📜 EduLang Language Rules

### 1. Program Boundary (Mandatory)
- **Must START with:** `enroll`
- **Must END with:** `graduate`
- *If missing or misplaced, a `Program Structure` error is reported!*

```text
enroll
{
    // code goes here
}
graduate
```

### 2. Keywords & Data Types
| Keyword | Meaning |
|---|---|
| `enroll` | Start of program |
| `graduate` | End of program |
| `score` | Integer data type (16-bit: `-32768` to `32767`) |
| `grade` | Character data type (`'A'`, `'F'`) |
| `fixed` | Constant declaration |
| `check` | If condition |
| `otherwise` | Else condition |
| `repeat` | Loop construct |
| `input` | Read input from user |
| `print` | Print value to screen |

### 3. Identifiers & Numbers
- **Identifiers**: Start with a letter or `_`, max **31 characters** (e.g. `marks`, `total_score`). Cannot start with digits (e.g. `1student` is an error).
- **Numbers**: 2-byte signed integers (`-32768` to `32767`). Optional `+` or `-` sign allowed (e.g. `+100`, `-20`, `75`).

### 4. Operators & Symbols
- **Arithmetic**: `+`, `-`, `*`, `/`, `%`
- **Relational**: `==`, `!=`, `<`, `<=`, `>`, `>=`
- **Logical**: `&&`, `||`, `!` *(Note: Single `&` or `|` is illegal)*
- **Delimiters**: `{`, `}`, `(`, `)`, `;`, `,`

---

## 💡 How the Lexer Works (Under the Hood)

### 1. The Main Dispatcher Loop
The lexer reads the code character-by-character:
1. Skips spaces, tabs, and updates line numbers on `\n`.
2. Automatically bypasses Windows UTF-8 BOM (`0xEF, 0xBB, 0xBF`).
3. Sends characters to specialized sub-DFAs:
   - Letters $\to$ `identifierDFA` (checks 31-char limit, matches keywords)
   - Digits $\to$ `numberDFA` (checks 2-byte range, flags numbers starting identifiers)
   - Quotes $\to$ `stringDFA` / `characterDFA` (handles escapes, checks unclosed)
   - Slashes $\to$ `commentDFA` (removes comments)
   - Operators $\to$ operator DFAs (`==`, `<=`, `&&`, etc.)

---

### 2. How Comments Are Omitted (Ignored)

When the lexer sees `/`, it checks if the next character is `/` or `*`:

1. **Single-Line Comments (`// ...`)**:
   - Skips past `//`.
   - Loops forward discarding every character until `\n`.
   - **Result**: No token is created. Line numbering stays accurate.

2. **Multi-Line Comments (`/* ... */`)**:
   - Skips past `/*`.
   - Loops until `*/` is found.
   - Increments `line++` on every `\n` inside the comment block so following lines have correct line numbers.
   - **If closing `*/` is missing before the file ends**, it reports an `Unclosed Comment` error.
   - **Result**: No token is created.

> **Why this is important:** Because comments produce zero tokens, you can freely place comments before `enroll` or after `graduate` without causing boundary errors!

---

## 🚨 Error Detection (All Categories Explained)

The lexer detects and categorizes all errors into clean tables:

| Error Category | What Triggers It | Example | Message Shown |
|---|---|---|---|
| **Program Structure** | Missing `enroll` at start | `score x = 5;` | `Program must start with 'enroll' keyword` |
| **Program Structure** | Missing `graduate` at end | `enroll { ... }` | `Program must end with 'graduate' keyword` |
| **Illegal Character** | Unrecognized symbol | `@score`, `$total` | `Illegal Character '@'` |
| **Illegal Character** | Standalone `&` or `|` | `a & b` | `Invalid '&' operator (did you mean '&&'?)` |
| **Invalid Identifier** | Identifier starts with number | `1student` | `Invalid Identifier '1student': cannot start with numbers` |
| **Exceeding Limit** | Identifier $> 31$ chars | `very_long_variable_name_...` | `Identifier exceeds maximum allowed length of 31 characters` |
| **Exceeding Limit** | Number out of 2-byte range | `999999999` | `Numeric Constant exceeds 2-byte limit (-32768 to 32767)` |
| **Exceeding Limit** | Char literal $> 1$ char | `'ABC'` | `Character literal exceeds 1-character limit` |
| **Unclosed Error** | Empty character literal | `''` | `Empty character literal is not allowed` |
| **Unclosed Error** | Unclosed block comment | `/* comment` | `Unclosed Comment: missing closing '*/'` |
| **Unterminated Char** | Char missing closing quote | `'A;` | `Unterminated Character: missing closing single quote (')` |
| **Unterminated String** | String missing closing quote | `"Hello;` | `Unterminated String: missing closing double quote (")` |

---

## 🖥️ Interactive Menu Guide

When you run `.\edulang_lexer.exe` without arguments, you get this menu:

```text
=======================================================
         EduLang Lexical Analyzer - Main Menu          
=======================================================
 1. Type / Paste EduLang code directly
 2. Load and analyze an EduLang file (.edu)
 3. Run default sample test program (Section 20)
 4. Run 5-Error demonstration
 5. Exit
-------------------------------------------------------
Enter your choice (1-5):
```

- **Option 1**: Type or paste code line-by-line. Finish by typing `graduate` on the final line or `END`.
- **Option 2**: Type the path to any `.edu` file (e.g., `tests/edge_cases.edu`).
- **Option 3**: Immediately runs the standard student grading program (`tests/test_program.edu`).
- **Option 4**: Runs the built-in error demo showing all error messages and tables.
- **Option 5**: Exits the program.

---

## 📊 Sample Program & Output

### Input Code ([`tests/test_program.edu`](tests/test_program.edu))
```text
enroll
{
    score marks = 75;
    grade result;
    check (marks >= 50)
    {
        result = 'P';
    }
    otherwise
    {
        result = 'F';
    }
    print(result);
}
graduate
```

### Output Table
```text
=================================================================
                      EduLang Token Stream                       
=================================================================
Lexeme                      Token             Line      
-----------------------------------------------------------------
enroll                      ENROLL            1         
{                           {                 2         
score                       DT                3         
marks                       ID                3         
=                           =                 3         
75                          NUM               3         
;                           ;                 3         
grade                       DT                4         
result                      ID                4         
;                           ;                 4         
check                       CHECK             5         
(                           (                 5         
marks                       ID                5         
>=                          >=                5         
50                          NUM               5         
)                           )                 5         
{                           {                 6         
result                      ID                7         
=                           =                 7         
'P'                         CHAR              7         
;                           ;                 7         
}                           }                 8         
otherwise                   OTHERWISE         9         
{                           {                 10        
result                      ID                11        
=                           =                 11        
'F'                         CHAR              11        
;                           ;                 11        
}                           }                 12        
print                       PRINT             13        
(                           (                 13        
result                      ID                13        
)                           )                 13        
;                           ;                 13        
}                           }                 14        
graduate                    GRADUATE          15        
=================================================================

 [SUCCESS] Lexical analysis completed with 0 errors!
Results saved to: output/tokens.txt
```

---

## 🛠️ Requirements & Build

- **Compiler**: Any modern C++ compiler supporting C++17 (`g++`, `clang++`, or Visual Studio MSVC).
- **Zero Dependencies**: Standard C++ libraries only (`<iostream>`, `<fstream>`, `<vector>`, `<string>`, `<unordered_map>`).
- **Operating System**: Windows / Linux / macOS.
