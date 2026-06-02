---
source: "https://www.geeksforgeeks.org/cpp/cpp-data-types/"
tags:
  - "cpp"
  - "gfg"
---
# Data Types in C++

Source: https://www.geeksforgeeks.org/cpp/cpp-data-types/

---

## 📖 教程正文
Last Updated: 9 Mar, 2026

Data types specify the type of data that a variable can store. Whenever a variable is defined in C++, the compiler allocates memory for that variable based on the data type with which it is declared. Please note that every may require a different amount of memory.

![Data-Type-in-C-2](https://media.geeksforgeeks.org/wp-content/uploads/20250714112214026299/Data-Type-in-C-2.webp "Click to enlarge")

Below is an example of integer data type.

  
**Output**
```
10
```

****Explanation****: In the above code, we needed to store the value ****10**** in our program, so we created a variable ****var****. But before ****var****, we have used the keyword ' ****int**** '****.**** This keyword is used to define that the variable ****var**** will store data of type ****integer****.

> ****Note****: The C++ standard does not specify the exact memory size of data types (int, float, etc.). Their size may vary depending on the compiler and system architecture. The sizes mentioned in this article are common for many modern 64-bit systems but may differ on other platforms.

Let's see how to use some primitive data types in C++ program.

## 1\. Character Data Type (char)

The [****character data type****](https://www.geeksforgeeks.org/cpp/cpp-char-data-types/) is used to store a single character. The keyword used to define a character is ****char****. Its typical size is 1 byte, and it stores characters enclosed in single quotes (' '). It can generally store upto 256 characters according to their [ASCII codes.](https://www.geeksforgeeks.org/computer-organization-architecture/what-is-ascii-a-complete-guide-to-generating-ascii-code/)

  
**Output**
```
A
```

## 2\. Integer Data Type (int)

****Integer data type**** denotes that the given variable can store the integer numbers. The keyword used to define integers is ****int.**** Its typical size is ****4-bytes**** (for 64-bit) systems and can store numbers for binary, octal, decimal and hexadecimal base systems in the range from ****\-2,147,483,648**** to ****2,147,483,647.****

  
**Output**
```
25
21
```

To know more about different base values in C++, refer to the article - [Literals in C++](https://www.geeksforgeeks.org/cpp/cpp-literals/)

## 3\. Boolean Data Type (bool)

The [****boolean data type****](https://www.geeksforgeeks.org/cpp/cpp-booleans/) is used to store logical values: ****true(1)**** or ****false(0)****. The keyword used to define a boolean variable is ****bool****. Its typical size is 1 byte.

  
**Output**
```
1
```

## 4\. Floating Point Data Type (float)

****Floating-point data type**** is used to store numbers with decimal points. The keyword used to define floating-point numbers is ****float****. Its typical size is 4 bytes (on 64-bit systems) and can store values in the range from ****1.2e-38**** to ****3.4e+38.****

  
**Output**
```
36.5
```

## 5\. Double Data Type (double)

The ****double data type**** is used to store decimal numbers with higher precision. The keyword used to define double-precision floating-point numbers is ****double****. Its typical size is 8 bytes (on 64-bit systems) and can store the values in the range from ****1.7e-308**** to ****1.7e+308****

  
**Output**
```
3.14159
```

## 6\. Void Data Type (void)

The ****void data type**** represents the absence of value. We cannot create a variable of void type. It is used for pointer and functions that do not return any value using the keyword ****void****.

## Type Safety in C++

C++ is a ****strongly typed language****. It means that all variables' data type should be specified at the declaration, and it does not change throughout the program. Moreover, we can only assign the values that are of the same type as that of the variable.

If we try to assign ****floating point**** value to a boolean variable, it may result in data corruption, runtime errors, or undefined behaviour.

  
**Output**
```
1
```

As we see, the floating-point value is not stored in the bool variable ****a.**** It just stores 1. This type checking is not only done for fundamental types, but for all data types to ensure valid operations and no data corruptions.

## Data Type Conversion

[****Type conversion****](https://www.geeksforgeeks.org/cpp/type-conversion-in-c/) refers to the process of changing one data type into another compatible one without losing its original meaning. It's an important concept for handling different data types in C++.

  
**Output**
```
67
70
```

## Size of Data Types in C++

Earlier, we mentioned that the size of the data types is according to the 64-bit systems. Does it mean that the size of C++ data types is different for different computers?

Actually, it is partially true. The size of C++ data types can vary across different systems, depending on the architecture of the computer (e.g., 32-bit vs. 64-bit systems) and the compiler being used. But if the architecture of the computer is same, then the size across different computers remains same.

We can find the size of the data type using [****sizeof****](https://www.geeksforgeeks.org/cpp/cpp-sizeof-operator/) operator. According to this type, the [range of values](https://www.geeksforgeeks.org/cpp/data-type-ranges-and-their-macros-in-c/) that a variable of given data types can store are decided.

  
**Output**
```
Size of int: 4 bytes
Size of char: 1 byte
Size of float: 4 bytes
Size of double: 8 bytes
```

## Data Type Modifiers

[****Data type modifiers****](https://www.geeksforgeeks.org/cpp/cpp-type-modifiers/) are the keywords used to change or give extra meaning to already existing data types. It is added to primitive data types as a prefix to modify their size or range of data they can store. There are 4 type modifiers in C++: ****short, long, signed**** and ****unsigned.****

****For Example,**** defining an ****int**** with ****long**** type modifier will change its size to 8 bytes:

Similarly, other type modifiers also affect the size or range of the data type.

> long double, long long int, unsigned int, etc.

8 Questions

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

What will be the output of the following code?

- A
- B
- C
- D

Identify the error that will be thrown in the following program:

- A
- B
- C
- D

What is the output?

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

Your Score:0/8

Accuracy:0%