# AI Formatting & Style Rules
**(Qt + Doxygen + 132‑Column Divider Standard)**

These rules define how **all C++ source files, headers, classes, functions, enums, properties, and internal code blocks** must be documented and formatted.

The goal is to maintain a **clean, Qt‑style Doxygen documentation system** with:

- **Strict structural consistency**
- **Maximum clarity**
- **Long‑term maintainability**
- **Zero ambiguity**

These rules are **mandatory** and apply uniformly across the entire codebase.

---

## 1. File Headers and Footers  
**(Required at the top of every `.cpp` and `.h` file)**

- Every file **MUST begin** with a Qt‑style **Doxygen file header**.
- The file header **MUST be wrapped** with a **132‑character divider** above and below.
- The divider format **MUST be used consistently** for all file headers and footers.
- No source or header file is permitted without a file‑level Doxygen header.

---

## 2. Required Doxygen Header Format  
**(Files, Classes, Functions, and All Declarations)**

Include **ONLY the tags that apply** to the documented item.

```cpp
/*! ******************************************************************************************************************************
 * @file filename.cpp                Required ONLY for file headers and footers
 * @brief        Short description of the file or declaration. (Required)
 * @details      Detailed description of purpose, responsibility, and behavior.
 *
 * @note         Important notes or implementation details.
 * @warning      Potential dangers or misuse scenarios.
 * @attention    Strong cautions requiring special care.
 * @remark       Additional remarks.
 * @remarks      Additional remarks (plural form allowed).
 *
 * @example      Example usage (only if it adds clarity).
 *
 * @class        Required for classes.
 * @struct       Required for structs.
 * @enum         Required for enums.
 * @typedef      Required for typedefs.
 * @namespace    Required for namespaces.
 * @interface    Required for interfaces.
 *
 * @private
 * @protected
 * @public
 *
 * @see          Related references.
 * @sa           See‑also references.
 *
 * @param        Function parameter.
 * @param[in]    Input parameter.
 * @param[out]   Output parameter.
 * @param[in,out] Input/output parameter.
 *
 * @return       Return value description (required if applicable).
 * @returns      Alternate form of @return.
 *
 * @signal       Qt signal documentation (implemented via @fn).
 * @slot         Qt slot documentation (implemented via @fn).
 *
 * @section      Major documentation section.
 * @subsection   Subsection under a section.
 *
 * @overload     Required for Qt‑style overload documentation.
 *
 * @static
 * @virtual
 *
 * Additional descriptive paragraph explaining responsibility, intent,
 * ownership, and high‑level behavior.
 ******************************************************************************************************************************* */
``` 

###### ########################################################################

# Doxygen
* @snippet
* @retval
* @reimp (Qt documentation often implies this via text, not a tag) 
* @property – Qt properties
* @fn – Explicit function documentation
* @snippetdoc
* @code / @endcode
* @mainpage
* @page
* @paragraph
* @anchor
* @internal
* @endinternal
* @cond
* @endcond
* @deprecated – Marks deprecated Qt APIs
* @module – Used by Qt to group modules
* @ingroup – Qt module grouping
* @defgroup / @addtogroup 
* @since [QtVersion] – Qt version where API was introduced
###### ########################################################################

---


###### ########################################################################

clear; tree -a -I 'Archive|.git|build'
