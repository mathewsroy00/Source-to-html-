# Source Code to HTML Converter (C)

## 🚀 Overview
This project converts C source code into syntax-highlighted HTML using a Finite State Machine (FSM) approach.

It reads a C file, parses it into tokens like keywords, comments, strings, and numbers, and generates a visually formatted HTML file with color styling.

---

## ✨ Features
- Syntax highlighting for:
  - Keywords (int, float, if, return, etc.)
  - Comments (single-line and multi-line)
  - Strings and character literals
  - Numeric constants
  - Preprocessor directives (#include, #define)
- Handles:
  - Header files (`<stdio.h>`, `"file.h"`)
  - Multi-line and single-line comments
- Generates clean HTML output using CSS styling

---

## 🧠 Concepts Used
- Finite State Machine (FSM)
- File Handling in C
- Tokenization (Lexical Analysis)
- String Processing
