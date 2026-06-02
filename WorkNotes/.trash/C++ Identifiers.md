---
title: "C++ Identifiers"
source: "https://www.geeksforgeeks.org/cpp/cpp-identifiers/"
author:
  - "[[GeeksforGeeks]]"
published: 2023-07-14
created: 2026-03-16
description: "Your All-in-One Learning Portal: GeeksforGeeks is a comprehensive educational platform that empowers learners across domains-spanning computer science and programming, school education, upskilling, commerce, software tools, competitive exams, and more."
tags:
  - "clippings"
---
- 课程
- 教程
- 面试准备

切换到深色模式

最后更新日期： 2025年9月15日

在 C++ 编程语言中， ****标识符**** 是分配给程序中变量、函数、类、结构体或其他实体的唯一名称。我们来看一个例子：

在上面的代码中， ****\`val\`**** 和 ****\`func\`**** 都是标识符。基本上，程序员命名的所有东西都是标识符，用于在程序后续部分引用相应的实体。

## 标识符命名规则

我们可以使用任何单词作为标识符，只要它符合以下规则：

- 标识符可以由 ****字母**** （AZ 或 az）、 ****数字**** （0-9）和 ****下划线（\_）**** 组成。不允许使用特殊字符和空格。
- 标识符只能以 ****字母或下划线开头。****
- C++ 包含一些保留 ****关键字**** ，这些关键字不能用作标识符，因为它们在语言中已有预定义的含义。例如， ****\`int\`**** 不能用作标识符，因为它在 C++ 中已经有了预定义的含义。尝试将这些关键字用作标识符会导致编译错误。
- 标识符在其命名空间内必须是 ****唯一的**** 。

此外，C++ 是一种区分大小写的语言，因此像 ****Num**** 和 ****num**** 这样的标识符会被视为不同的值。下图展示了一些有效的和无效的 C++ 标识符。

要了解有关标识符命名规则的更多信息，请参阅这篇文章 [——C++ 中的命名约定](https://www.geeksforgeeks.org/cpp/naming-convention-in-c/)

![有效和无效标识符的示例](https://media.geeksforgeeks.org/wp-content/uploads/20221202181520/Cvariables2.png)

有效/无效标识符示例

## 例子

在这个例子中，我们按照规范使用了标识符，并用标识符来命名 [类](https://www.geeksforgeeks.org/cpp/c-classes-and-objects/) 、函数、整数数据类型等等。如果您还不了解 C++ 中的 [函数](https://www.geeksforgeeks.org/cpp/functions-in-cpp/) 和 [类](https://www.geeksforgeeks.org/cpp/c-classes-and-objects/) ，不用担心，您很快就会学会。下面的代码运行成功，这意味着我们正确地命名了它们。

  
**输出**
```
总和为：12
```

## 标识符命名约定

[命名约定](https://www.geeksforgeeks.org/cpp/naming-convention-in-c/) 并非 C++ 语言强制执行的规则，而是编程社区为了便于理解而提出的变量命名建议。以下是一些命名约定：

****对于变量：****

- 请使用驼峰式命名法（常量可以使用大蛇式命名法）。
- 首先从小写字母开始。
- 使用描述性、有意义的名称。
- 例如，频率计数、人名

****对于函数：****

- 请使用驼峰式命名法。
- 命名时请使用动词或动词短语。
- 例如，getName()、countFrequency() 等 ****。****

****课程相关：****

- 使用 PascalCase
- 命名时请使用名词或名词短语。
- 例如，汽车、人等

再次强调，以上仅是一些命名标识符的建议，并非绝对规则。具体命名还取决于您正在使用的项目规范和您的个人偏好。

5个 问题

- 一个
- B
- C
- D

- 一个
- B
- C
- D

- 一个
- B
- C
- D

- 一个
- B
- C
- D

- 一个
- B
- C
- D

![成功](https://media.geeksforgeeks.org/auth-dashboard-uploads/sucess-img.png)

测验已成功完成

您的 得分 ： 0/5

准确率： 0 %

文章标签：

[C++](https://www.geeksforgeeks.org/category/programming-language/cpp/)