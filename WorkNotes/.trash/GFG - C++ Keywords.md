---
source: "https://www.geeksforgeeks.org/cpp/cpp-keywords/"
tags:
  - "cpp"
  - "gfg"
  - "algorithms"
created: 2026-03-16
---
# C++ Keywords

> [!abstract] 页面摘要
> Your All-in-One Learning Portal: GeeksforGeeks is a comprehensive educational platform that empowers learners across domains-spanning computer science and programming, school education, upskilling, commerce, software tools, competitive exams, and more.

---

## 💻 代码摘录 (请先在网页选中代码)
```cpp

```

---

## 📖 教程全文
Last Updated: 17 Sep, 2025

Keywords are the reserved words that have special meanings. Since their meanings are reserved, we cannot redefine them or use them for a different purpose.

  
**Output**
```
Adult
```

### How to Identify C++ Keywords

1. ****Syntax Highlighting****: Most modern IDEs (like Visual Studio, CLion, Code::Blocks) highlight keywords in a different color. This makes them stand out from variables or function names.
2. ****Compiler Errors****: If you mistakenly use a keyword as a variable name, your code won’t compile. ****Example:****

## Categorization of C++ Keywords

To make them easier to understand, let’s group C++ keywords by context:

| ****Category**** | ****Keywords**** |
| --- | --- |
| ****Data Types**** | [bool](https://www.geeksforgeeks.org/cpp/cpp-booleans/), [char](https://www.geeksforgeeks.org/cpp/cpp-char-data-types/), [char8\_t](https://www.geeksforgeeks.org/cpp/char8_t-data-type-in-cpp-20/), char16\_t, char32\_t, int, long, short, signed, unsigned, float, double, void, wchar\_t |
| ****Control Flow**** | [if](https://www.geeksforgeeks.org/cpp/c-c-if-else-statement-with-examples/), [else](https://www.geeksforgeeks.org/cpp/c-c-if-else-statement-with-examples/), [switch](https://www.geeksforgeeks.org/cpp/switch-statement-in-cpp/), case, [default](https://www.geeksforgeeks.org/cpp/default-arguments-c/), [for](https://www.geeksforgeeks.org/cpp/cpp-for-loop/), [while](https://www.geeksforgeeks.org/cpp/cpp-while-loop/), [do](https://www.geeksforgeeks.org/cpp/cpp-do-while-loop/), [break](https://www.geeksforgeeks.org/cpp/cpp-break-statement/), [continue](https://www.geeksforgeeks.org/cpp/continue-statement-cpp/), [goto](https://www.geeksforgeeks.org/cpp/goto-statement-in-cpp/) |
| ****Boolean & Null**** | true, false, [nullptr](https://www.geeksforgeeks.org/cpp/understanding-nullptr-c/) |
| ****Memory Management**** | new, [delete](https://www.geeksforgeeks.org/cpp/delete-in-c/), [sizeof](https://www.geeksforgeeks.org/cpp/cpp-sizeof-operator/), [alignas](https://www.geeksforgeeks.org/cpp/alignas-in-cpp-11/), [alignof](https://www.geeksforgeeks.org/cpp/alignof-operator-in-c/) |
| ****Classes & Structs**** | class, [struct](https://www.geeksforgeeks.org/cpp/structures-in-cpp/), [union](https://www.geeksforgeeks.org/cpp/cpp-unions/), [enum](https://www.geeksforgeeks.org/cpp/enumeration-in-cpp/), [friend](https://www.geeksforgeeks.org/cpp/friend-class-function-cpp/), [mutable](https://www.geeksforgeeks.org/cpp/c-mutable-keyword/), [this](https://www.geeksforgeeks.org/cpp/this-pointer-in-c/) |
| ****Access Specifiers**** | [public](https://www.geeksforgeeks.org/cpp/access-modifiers-in-c/), [private](https://www.geeksforgeeks.org/cpp/access-modifiers-in-c/), [protected](https://www.geeksforgeeks.org/cpp/access-modifiers-in-c/) |
| ****Functions & Modifiers**** | [inline](https://www.geeksforgeeks.org/cpp/inline-functions-cpp/), [explicit](https://www.geeksforgeeks.org/cpp/use-of-explicit-keyword-in-cpp/), [virtual](https://www.geeksforgeeks.org/cpp/virtual-function-cpp/), override, [final](https://www.geeksforgeeks.org/cpp/c-final-specifier/), [constexpr](https://www.geeksforgeeks.org/cpp/understanding-constexper-specifier-in-cpp/), consteval, [constinit](https://www.geeksforgeeks.org/cpp/constinit-specifier-in-cpp-20/), operator, [typedef](https://www.geeksforgeeks.org/cpp/typedef-in-cpp/), using, [typename](https://www.geeksforgeeks.org/cpp/how-to-use-typename-keyword-in-cpp/) |
| ****Templates & Generics**** | [template](https://www.geeksforgeeks.org/cpp/templates-cpp/), concept, requires |
| ****Exception Handling**** | [try](https://www.geeksforgeeks.org/cpp/how-to-use-the-try-and-catch-blocks-in-cpp/), [catch](https://www.geeksforgeeks.org/cpp/how-to-use-the-try-and-catch-blocks-in-cpp/), throw, [noexcept](https://www.geeksforgeeks.org/cpp/noexcept-operator-in-cpp-11/) |
| ****Casting & Type Info**** | [const\_cast](https://www.geeksforgeeks.org/cpp/const_cast-in-c-type-casting-operators/), [dynamic\_cast](https://www.geeksforgeeks.org/cpp/dynamic-_cast-in-cpp/), [reinterpret\_cast](https://www.geeksforgeeks.org/cpp/reinterpret_cast-in-c-type-casting-operators/), [static\_cast](https://www.geeksforgeeks.org/cpp/static_cast-in-cpp/), [decltype](https://www.geeksforgeeks.org/cpp/type-inference-in-c-auto-and-decltype/), [typeid](https://www.geeksforgeeks.org/cpp/typeid-operator-in-c-with-examples/) |
| ****Constants & Storage**** | [const](https://www.geeksforgeeks.org/cpp/const-keyword-in-cpp/), [static](https://www.geeksforgeeks.org/cpp/static-keyword-cpp/), [static\_assert](https://www.geeksforgeeks.org/cpp/understanding-static_assert-c-11/), [extern](https://www.geeksforgeeks.org/c/understanding-extern-keyword-in-c/), register, [thread\_local](https://www.geeksforgeeks.org/cpp/thread_local-storage-in-cpp-11/), [volatile](https://www.geeksforgeeks.org/cpp/how-to-use-volatile-keyword-in-cpp/) |
| ****Modules / Export**** | export, [namespace](https://www.geeksforgeeks.org/cpp/namespace-in-c/) |
| ****Coroutines (C++20)**** | co\_await, co\_return, co\_yield |
| ****Operators (alt spellings)**** | and, and\_eq, or, or\_eq, not, not\_eq, bitand, bitor, compl, xor, xor\_eq |
| ****Miscellaneous**** | [asm](https://www.geeksforgeeks.org/cpp/c-asm-declaration/), [auto](https://www.geeksforgeeks.org/cpp/type-inference-in-c-auto-and-decltype/), [return](https://www.geeksforgeeks.org/cpp/return-statement-in-cpp-with-examples/), [sizeof](https://www.geeksforgeeks.org/cpp/cpp-sizeof-operator/) |

****Note****: The number of keywords C++ has evolved over time as new features were added to the language. ****For example****, C++ 98 had 63 keywords, C++ 11 had 84 keywords, C++.

## Keywords vs Identifiers

So, there are some properties of keywords that [distinguish keywords from identifiers.](https://www.geeksforgeeks.org/c/difference-between-keyword-and-identifier/) They listed in the below table

| ****Keywords**** | ****Identifiers**** |
| --- | --- |
| Keywords are predefined/reserved words | identifiers are the values used to define different programming items like a variable, integers, structures, and unions. |
| It defines the type of entity. | It classifies the name of the entity. |
| A keyword contains only alphabetical characters, | an identifier can consist of alphabetical characters, digits, and underscores. |
| It should be lowercase. | It can be both upper and lowercase. |
| No special symbols or punctuations are used in keywords and identifiers. | No special symbols or punctuations are used in keywords and identifiers. The only underscore can be used in an identifier. |
| ****Example:**** int, char, while, do. | ****Example:**** geeksForGeeks, geeks\_for\_geeks, gfg, gfg12. |

6 Questions

- A
- B
- C
- D

- A
- B
- C
- D

- A
- B
- C
- D

- A
- B
- C
- D

- A
- B
- C
- D

- A
- B
- C
- D

![success](https://media.geeksforgeeks.org/auth-dashboard-uploads/sucess-img.png)

Quiz Completed Successfully

Your Score:0/6

Accuracy:0%

Article Tags:

[C++](https://www.geeksforgeeks.org/category/programming-language/cpp/)