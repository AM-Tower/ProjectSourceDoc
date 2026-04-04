​​# Formatting rules and style guide
* Note: I do not want to have to write doxygen updates or headers because they change all the time.
1. Use Doxygen headers for the top with a footer for the bottom, and all functions.
* Top Header:
```
/*!
 * ********************************************************************************************************************************
 * @file        FileName.ext
 * @brief       File brief description.
 * @details     Provide details about file.
 * @authors     Jeffrey Scott Flesher with the help of AI: Copilot
 * @date        2026-04-02
 *********************************************************************************************************************************/
```
* Update the @date YYYY-MM-DD format
* Footer:
```
* Use tags that apply:
- `@brief` — one concise sentence
- `@details` — optional, for deeper explanation
- `@param` — for each parameter
- `@return` — only when non‑void
- `@note` — optional (ownership, side effects, caveats)
- `@throws` — if exceptions are used
- `@threadsafe`, `@reentrant`, or `@notthreadsafe` — when relevant
/*!
 * ********************************************************************************************************************************
 * @brief End of AppLogUtils.h
 *********************************************************************************************************************************/
```
* Functions. Every class, structure, and function will have doxygen headers.
```
    /*!
     * ****************************************************************************************************************************
     * @brief   Brief descrition.
     * @details Detailed description of function.
     * @param   arg1 Describe argument.
     * @return  Return type.
     *****************************************************************************************************************************/
* Classes in headers.h
- //! Describe functionName.
- void functionName();
- int varName; ///< Describe varName
- Each enum value MUST have a trailing ///< AlignTrailingComments: Always
- Every data member in a header must have ///< AlignTrailingComments: Always

```
2. Qt Brackets
```
Language: Cpp
BasedOnStyle: LLVM
IndentWidth: 4
TabWidth: 4
UseTab: Never
IndentAccessModifiers: true
AccessModifierOffset: 0
NamespaceIndentation: All
BreakBeforeBraces: Allman
# Keep short functions/constructors on one line
AllowShortFunctionsOnASingleLine: Inline
# Keep parameters on one line
BinPackParameters: true
AllowAllParametersOfDeclarationOnNextLine: false
# Keep constructor initializer list on one line
BreakConstructorInitializers: AfterColon
ConstructorInitializerIndentWidth: 0
SpaceBeforeParens: ControlStatements
SpacesInParens: Never
AlignTrailingComments: Always
ColumnLimit: 0
```
3. Do not wrap arguments, use single lines. Prefer "if" statements with 1 statement be a one-liner.
  * Not:
  * const QString varName = varName.isEmpty()
  *                       ? varName.fileName()
  *                       : varName + varName.fileName();
  * One-liner:
  * const QString varName = varName.isEmpty() ? varName.fileName() : varName + varName.fileName();
4. Do not show snippets or stubs, as they delete functions and functionality breaking the code.
  * Show complete functions with headers, with the exception of a one line or a block of code fix.
  * If showing only functions that change, do not include top header or footer to prevent deleting functions.
    * Show functions in a separate code copy box so I overwrite the correct function.
5. Ensure there are no errors or warnings in code using clazy.
6. Do not show functions, code or files that have not changed.
7. Clazy
  * for (const QFileInfo &info : dirEntries)
    * FileName.cpp:60: warning: c++11 range-loop might detach Qt container (QFileInfoList) [clazy-range-loop-detach]