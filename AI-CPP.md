# ✅ Scoped AI Formatting & Update Rules (Files Provided Only)
**Qt + Doxygen + 132‑Column Divider Standard — GitHub‑Ready**

These rules define how formatting and documentation standards are applied **only to the files explicitly provided in a given response**.

They are intentionally scoped to prevent accidental global refactors and to make expectations clear, predictable, and easy to follow.

---

## 1. Scope of Application (Critical)

- **Formatting and documentation rules apply ONLY to the files explicitly provided by the assistant.**
- No assumptions are made about the rest of the codebase.
- Files not shown or rewritten remain untouched and out of scope.
- These rules do **not** authorize repository‑wide cleanup, refactors, or reformatting.

**If a file is not given to you, it must not be modified or evaluated.**

---

## 2. File‑Level Documentation (Provided Files Only)

For any `.cpp` or `.h` file explicitly provided:

- The file **MUST** begin with a Qt‑style Doxygen file header.
- The header **MUST** be wrapped above and below with a **132‑character divider**.
- Existing file headers in provided files may be updated to comply.
- Files not provided are assumed correct and are not evaluated.

---

## 3. Function‑Level Documentation (Provided Files Only)

Within files explicitly provided:

- **Only functions that appear in the provided files are subject to formatting rules.**
- Each function must include a Qt‑style Doxygen header.
- Include **only the tags that apply** to the function.
- Documentation must clearly describe:
  - Responsibility
  - Intent
  - Parameters
  - Return values
  - Side effects (if applicable)

No requirement exists to document or modify functions in files that are not provided.

---

## 4. Formatting Rules (Applied Only to Provided Files)

The following formatting rules apply **only inside files explicitly given**:

- Use **Allman style braces**
- Keep **all function arguments on one line**
- Indent namespaces, classes, and functions with **4 spaces**
- Single‑statement conditionals must be written as one‑liners  
  Example:
  ```cpp
  if (path.isEmpty()) return QString();
- Do **not** reformat surrounding or unrelated code for stylistic consistency alone.
- Formatting changes must be limited strictly to the provided file.

---

## 5. Completeness Rule (Provided Files Only)

- When a function appears in a provided file:
  - The **entire function implementation must be shown**.
  - Do **not** show snippets, partial bodies, or placeholders.
- Only files explicitly provided may be included in the response.
- Additional files must not be added unless explicitly requested.

---

## 6. Behavioral Safety Rules

- Do **not** infer missing requirements.
- Do **not** introduce new behavior unless explicitly requested.
- Do **not** refactor code purely for stylistic reasons.
- Preserve existing semantics unless the task explicitly states otherwise.

---

## 7. Assumption Rule

Unless explicitly stated otherwise:

- Assume files not provided are correct.
- Assume formatting outside the provided files is intentional.
- Assume the task scope is **local to the provided files