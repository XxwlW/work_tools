{
  // ===================== cpp-check-lint =====================
  "cpp-check-lint.--enable": true,
  "cpp-check-lint.cppcheck.--enable=": "style",
  "cpp-check-lint.cppcheck.--executable": "/usr/bin/cppcheck",
  "cpp-check-lint.cppcheck.--inline-suppr": false,
  "cpp-check-lint.cppcheck.--language=": "c",
  "cpp-check-lint.cppcheck.--onsave": false,
  "cpp-check-lint.cppcheck.--quick_fix": false,
  "cpp-check-lint.cpplint.--enable": false,
  // ===================== highlightwords =====================
  "highlightwords.colors": [
    {
      "light": "#b3d9ff",
      "dark": "yellow"
    },
    {
      "light": "#e6ffb3",
      "dark": "Cyan"
    },
    {
      "light": "#b3b3ff",
      "dark": "lightgreen"
    },
    {
      "light": "#ffd9b3",
      "dark": "magenta"
    },
    {
      "light": "#ffb3ff",
      "dark": "cornflowerblue"
    },
    {
      "light": "#b3ffb3",
      "dark": "orange"
    },
    {
      "light": "#ffff80",
      "dark": "green"
    },
    {
      "light": "#d1e0e0",
      "dark": "red"
    },
    {
      "light": "#00bfff",
      "dark": "#ff7f50"
    },
    {
      "light": "#6b8e23",
      "dark": "#d2691e"
    }
  ],
  "highlightwords.box": {
    "light": true,
    "dark": false
  },
  "highlightwords.defaultMode": 0,
  "highlightwords.showSidebar": true,
  // ===================== editor =====================
  "editor.guides.bracketPairs": true,
  "editor.readOnly": false,
  "editor.definitionLinkOpensInPeek": true,
  "editor.selectionHighlight": false,
  // "editor.foldingMaximumRegions": 65000,
  "editor.cursorSmoothCaretAnimation": "on",
  "editor.cursorBlinking": "smooth",
  "editor.wordWrap": "on",
  "editor.minimap.autohide": "mouseover",
  "editor.minimap.maxColumn": 100,
  "editor.mouseWheelZoom": true,
  "editor.quickSuggestionsDelay": 50,
  "editor.showFoldingControls": "always",
  // ===================== terminal =====================
  "terminal.integrated.fontFamily": "monospace",
  "terminal.integrated.scrollback": 5000,
  "terminal.integrated.copyOnSelection": true,
  "terminal.integrated.env.linux": {
    "PATH": "/usr/local/bin:${env:PATH}"
  },
  // ===================== files/search =====================
  "files.autoSaveDelay": 18000,
  "files.associations": {
    ".json": "jsonc",
    // "*.log": "plaintext",
    "*.out": "plaintext",
    "*.dat": "plaintext"
  },
  "files.autoGuessEncoding": true,
  "files.exclude": {
    "**/.DS_Store": true,
    "**/.git": true,
    "**/.hg": true,
    "**/.svn": true,
    "**/arm-trusted-firmware": true,
    "**/CVS": true,
    "**/Thumbs.db": true,
    "**/node_modules/*/**": true,
    "**../.history**": true
  },
  "files.watcherExclude": {
    "**/.git/**": true,
    "**/node_modules/**": true,
    "**/.history/**": true,
    "**/build/**": true,
    "**/dist/**": true,
    "**/*.log": true
  },
  "search.exclude": {
    "**/*.code-search": true,
    "**/node_modules": true,
    "**/.git": true,
    "**/build": true,
    "**/dist": true
  },
  "search.followSymlinks": false,
  // ===================== git =====================
  "git.ignoreWindowsGit27Warning": true,
  "git.autorefresh": true,
  // "git.path": "git",
  "git.path": "/usr/bin/git",
  "git.openRepositoryInParentFolders": "never",
  "git.autoRepositoryDetection": false,
  "git.repositoryScanMaxDepth": 0,
  "git.detectSubmodules": false,
  // "git.enabled": false,
  "scm.repositories.visible": 1,
  "git.detectSubmodulesLimit": 0,
  // ===================== security =====================
  "security.workspace.trust.untrustedFiles": "open",
  "security.workspace.trust.enabled": false,
  "telemetry.telemetryLevel": "off",
  // ===================== extensions / misc =====================
  "tabnine.experimentalAutoImports": true,
  "gitlens.advanced.messages": {
    "suppressGitDisabledWarning": true,
    "suppressGitMissingWarning": true
  },
  "gitlens.hovers.enabled": true,
  "code-runner.runInTerminal": true,
  "code-runner.saveAllFilesBeforeRun": true,
  "code-runner.saveFileBeforeRun": true,
  "explorer.confirmDelete": false,
  "cmake.configureOnOpen": true,
  "cmake.showOptionsMovedNotification": false,
  "update.mode": "none",
  "deepseek.lang": "cn",
  "bookmarks.keepBookmarksOnLineDelete": true,
  "vsicons.dontShowNewVersionMessage": true,
  "bracket-pair-colorizer-2.depreciation-notice": false,
  "polacode.target": "snippet",
  "workbench.settings.openDefaultSettings": true,
  "workbench.editor.titleScrollbarSizing": "large",
  "extensions.autoCheckUpdates": false,
  "extensions.autoUpdate": false,
  "workbench.enableExperiments": false,
  "workbench.settings.enableNaturalLanguageSearch": false,
  // ===================== C/C++ =====================
  "C_Cpp.intelliSenseCachePath": "/media/disk/vs-cpptool",
  "C_Cpp.codeAnalysis.clangTidy.useBuildPath": true,
  "C_Cpp.intelliSenseCacheSize": 1024,
  "C_Cpp.workspaceParsingPriority": "medium",
  "C_Cpp.intelliSenseEngine": "disabled",
  "C_Cpp.autocompleteAddParentheses": true,
  "C_Cpp.clang_format_fallbackStyle": "Google",
  "C_Cpp.formatting": "clangFormat",
  "editor.defaultFormatter": "xaver.clang-format",
  // 2. 针对 C/C++ 文件的具体配置
  "[cpp]": {
    "editor.defaultFormatter": "xaver.clang-format",
    "editor.formatOnSave": false,
    "editor.foldingStrategy": "auto",
    "editor.defaultFoldingProvider": "llvm-vs-code-extensions.vscode-clangd"
  },
  "[c]": {
    "editor.defaultFormatter": "xaver.clang-format",
    "editor.formatOnSave": false,
    "editor.foldingStrategy": "auto",
    "editor.defaultFoldingProvider": "llvm-vs-code-extensions.vscode-clangd"
  },
  // 3. 顺便解决你可能的输入法困扰
  "editor.unicodeHighlight.nonBasicASCIICharacters": true, // 这样全角字符会被高亮标出
  // ===================== code-runner =====================
  "code-runner.executorMap": {
    "javascript": "node",
    "java": "cd $dir && javac $fileName && java $fileNameWithoutExt",
    "c": "cd $dir && gcc $fileName -o $fileNameWithoutExt && $dir$fileNameWithoutExt",
    "zig": "zig run",
    "cpp": "cd $dir && g++ -I \"D:\\\\Wxj\\\\Code\\\\Dubins_path\\\\Eigen\" $fileName -o $fileNameWithoutExt && $dir$fileNameWithoutExt",
    "objective-c": "cd $dir && gcc -framework Cocoa $fileName -o $fileNameWithoutExt && $dir$fileNameWithoutExt",
    "php": "php",
    "python": "python -u",
    "perl": "perl",
    "perl6": "perl6",
    "ruby": "ruby",
    "go": "go run",
    "lua": "lua",
    "groovy": "groovy",
    "powershell": "powershell -ExecutionPolicy ByPass -File",
    "bat": "cmd /c",
    "shellscript": "bash",
    "fsharp": "fsi",
    "csharp": "scriptcs",
    "vbscript": "cscript //Nologo",
    "typescript": "ts-node",
    "coffeescript": "coffee",
    "scala": "scala",
    "swift": "swift",
    "julia": "julia",
    "crystal": "crystal",
    "ocaml": "ocaml",
    "r": "Rscript",
    "applescript": "osascript",
    "clojure": "lein exec",
    "haxe": "haxe --cwd $dirWithoutTrailingSlash --run $fileNameWithoutExt",
    "rust": "cd $dir && rustc $fileName && $dir$fileNameWithoutExt",
    "racket": "racket",
    "scheme": "csi -script",
    "ahk": "autohotkey",
    "autoit": "autoit3",
    "dart": "dart",
    "pascal": "cd $dir && fpc $fileName && $dir$fileNameWithoutExt",
    "d": "cd $dir && dmd $fileName && $dir$fileNameWithoutExt",
    "haskell": "runghc",
    "nim": "nim compile --verbosity:0 --hints:off --run",
    "lisp": "sbcl --script",
    "kit": "kitc --run",
    "v": "v run",
    "sass": "sass --style expanded",
    "scss": "scss --style expanded",
    "less": "cd $dir && lessc $fileName $fileNameWithoutExt.css",
    "FortranFreeForm": "cd $dir && gfortran $fileName -o $fileNameWithoutExt && $dir$fileNameWithoutExt",
    "fortran-modern": "cd $dir && gfortran $fileName -o $fileNameWithoutExt && $dir$fileNameWithoutExt",
    "fortran_fixed-form": "cd $dir && gfortran $fileName -o $fileNameWithoutExt && $dir$fileNameWithoutExt",
    "fortran": "cd $dir && gfortran $fileName -o $fileNameWithoutExt && $dir$fileNameWithoutExt",
    "sml": "cd $dir && sml $fileName"
  },
  // ===================== todo-tree =====================
  "todo-tree.general.tags": [
    "todo",
    "tag",
    "done",
    "mark",
    "test",
    "update",
    "BUG",
    "HACK",
    "FIXME",
    "XXX",
    "[ ]",
    "[x]"
  ],
  "todo-tree.regex.regexCaseSensitive": false,
  "todo-tree.highlights.defaultHighlight": {
    "foreground": "black",
    "background": "yellow",
    "icon": "check",
    "rulerColour": "yellow",
    "type": "tag",
    "iconColour": "yellow"
  },
  "todo-tree.highlights.customHighlight": {
    "todo": {
      "icon": "alert",
      "background": "#c9c552",
      "rulerColour": "#c9c552",
      "iconColour": "#c9c552"
    },
    "bug": {
      "background": "#eb5c5c",
      "icon": "bug",
      "rulerColour": "#eb5c5c",
      "iconColour": "#eb5c5c"
    },
    "tag": {
      "background": "#38b2f4",
      "icon": "tag",
      "rulerColour": "#38b2f4",
      "iconColour": "#38b2f4",
      "rulerLane": "full"
    },
    "done": {
      "background": "#5eec95",
      "icon": "check",
      "rulerColour": "#5eec95",
      "iconColour": "#5eec95"
    },
    "mark": {
      "background": "#f90",
      "icon": "note",
      "rulerColour": "#f90",
      "iconColour": "#f90"
    },
    "test": {
      "background": "#df7be6",
      "icon": "flame",
      "rulerColour": "#df7be6",
      "iconColour": "#df7be6"
    },
    "update": {
      "background": "#d65d8e",
      "icon": "versions",
      "rulerColour": "#d65d8e",
      "iconColour": "#d65d8e"
    },
    "BUG": {
      "icon": "bug"
    },
    "HACK": {
      "icon": "tools"
    },
    "FIXME": {
      "icon": "flame"
    },
    "XXX": {
      "icon": "x"
    },
    "[ ]": {
      "icon": "issue-draft"
    },
    "[x]": {
      "icon": "issue-closed"
    }
  },
  // ===================== local-history =====================
  "local-history.daysLimit": 0,
  "local-history.saveDelay": 99999999,
  "local-history.maxDisplay": 200,
  "local-history.dateLocale": "fr-CH",
  "local-history.treeLocation": "localHistory",
  "local-history.exclude": [
    "**/.history/**",
    "**/node_modules/**",
    "**/typings/**",
    "**/out/**",
    "**/Code/User/**"
  ],
  "diffEditor.maxComputationTime": 0,
  "meld-diff.diffCommand": "D:/Program Files/BCompare/BCompare",
  // ===================== doxdocgen =====================
  "doxdocgen.generic.linesToGet": 2,
  "doxdocgen.generic.authorName": "xxx",
  "doxdocgen.generic.authorEmail": "xxx@xxx.com",
  "doxdocgen.generic.useGitUserEmail": true,
  "doxdocgen.generic.useGitUserName": true,
  "doxdocgen.generic.authorTag": "@author{indent:12} {author} ({email})",
  "doxdocgen.file.fileTemplate": "@file{indent:12} {name}",
  "doxdocgen.file.versionTag": "@version{indent:12} 0.1",
  "doxdocgen.file.copyrightTag": [
    "@copyright{indent:12} xxxx技有限公司"
  ],
  "doxdocgen.file.customTag": [
    "*****************************************************************************"
  ],
  "doxdocgen.generic.briefTemplate": "@brief{indent:12} {text}",
  "doxdocgen.generic.dateTemplate": "@date{indent:12} {date}",
  "doxdocgen.generic.paramTemplate": "@param{indent:12} {param} ",
  "doxdocgen.generic.returnTemplate": "@return{indent:12} {type} ",
  "doxdocgen.file.fileOrder": [
    "custom",
    "file",
    "brief",
    "author",
    "date",
    "copyright",
    "custom"
  ],
  "doxdocgen.generic.order": [
    "brief",
    "empty",
    "tparam",
    "param",
    "return",
    "custom"
  ],
  // ===================== UI / theme =====================
  "workbench.iconTheme": "office-material-icon-theme",
  "workbench.colorTheme": "Atomic Dark",
  // 合并后的最终 workbench.colorCustomizations（保持“最后生效”不变）
  // 方案一
  // "workbench.colorCustomizations": {
  //   // 你后面那段 gitGraph 颜色（原来最后出现，所以最终生效）
  //   "gitGraph.verticalLine": "#333333",
  //   "gitGraph.verticalLineHighlighted": "#666666",
  //   "gitGraph.branchLabel": "#ffffff",
  //   "gitGraph.branchLabelBackground": "#007acc",
  //   "gitGraph.commitDot": "#ffffff",
  //   "gitGraph.commitDotHighlighted": "#ffff00",
  //   // 终端主题颜色（也是最后出现的那份）
  //   "terminal.foreground": "#F1EFF8",
  //   "terminalCursor.background": "#F1EFF8",
  //   "terminalCursor.foreground": "#F1EFF8",
  //   "terminal.ansiBlack": "#292A44",
  //   "terminal.ansiBlue": "#2DE0A7",
  //   "terminal.ansiBrightBlack": "#666699",
  //   "terminal.ansiBrightBlue": "#2DE0A7",
  //   "terminal.ansiBrightCyan": "#8EAEE0",
  //   "terminal.ansiBrightGreen": "#6DFEDF",
  //   "terminal.ansiBrightMagenta": "#7AA5FF",
  //   "terminal.ansiBrightRed": "#A0A0C5",
  //   "terminal.ansiBrightWhite": "#53495D",
  //   "terminal.ansiBrightYellow": "#AE81FF",
  //   "terminal.ansiCyan": "#8EAEE0",
  //   "terminal.ansiGreen": "#6DFEDF",
  //   "terminal.ansiMagenta": "#7AA5FF",
  //   "terminal.ansiRed": "#A0A0C5",
  //   "terminal.ansiWhite": "#F1EFF8",
  //   "terminal.ansiYellow": "#AE81FF"
  // },
  // "editor.tokenColorCustomizations": {
  //   "comments": "#55aa7f",
  //   "keywords": "#ff55ff",
  //   "variables": "#5eccf8",
  //   "strings": "#00ff7f",
  //   "functions": "#ffbb00",
  //   "numbers": "#00eeff",
  //   "types": "#55bbff",
  //   "textMateRules": [
  //     {
  //       "scope": "punctuation.definition.string.begin",
  //       "settings": {
  //         "foreground": "#00ff7f",
  //         "fontStyle": "bold"
  //       }
  //     },
  //     {
  //       "scope": "punctuation.definition.string.end",
  //       "settings": {
  //         "foreground": "#00ff7f",
  //         "fontStyle": "bold"
  //       }
  //     }
  //   ]
  // },
  // 方案二
  // "workbench.colorCustomizations": {
  //   "editor.background": "#0f1117",
  //   "editor.lineHighlightBackground": "#121622",
  //   "editor.selectionBackground": "#1e2636",
  //   "editor.findMatchBackground": "#2d2a3a",
  //   "editor.findMatchHighlightBackground": "#1f1d2a",
  //   "editorCursor.foreground": "#c0caf5",
  //   "editorBracketMatch.background": "#1e2636",
  //   "editorBracketMatch.border": "#c0caf5",
  //   "tab.activeBackground": "#121622"
  // },
  // "editor.tokenColorCustomizations": {
  //   "comments": "#667085",
  //   "keywords": "#a5b4fc",
  //   "variables": "#9ca3af",
  //   "strings": "#a7f3d0",
  //   "functions": "#fbbf24",
  //   "numbers": "#93c5fd",
  //   "types": "#cbd5e1"
  // }
  // 方案三
  // "workbench.colorCustomizations": {
  //   /* ===== 全局统一背景 ===== */
  //   "editor.background": "#14161b",
  //   "sideBar.background": "#14161b",
  //   "activityBar.background": "#14161b",
  //   "statusBar.background": "#14161b",
  //   "panel.background": "#14161b",
  //   "titleBar.activeBackground": "#14161b",
  //   "titleBar.inactiveBackground": "#14161b",
  //   /* ===== 标签页 ===== */
  //   "tab.activeBackground": "#1a1d25",
  //   "tab.inactiveBackground": "#14161b",
  //   "tab.border": "#1a1d25",
  //   /* ===== 行号 & 光标 ===== */
  //   "editorLineNumber.foreground": "#5c6370",
  //   "editorLineNumber.activeForeground": "#c8d3f5",
  //   "editorCursor.foreground": "#c8d3f5",
  //   /* ===== 编辑器层次 ===== */
  //   "editor.lineHighlightBackground": "#1a1d25",
  //   "editor.selectionBackground": "#2a3550",
  //   "editor.inactiveSelectionBackground": "#202738",
  //   /* ===== 搜索 / 高亮 ===== */
  //   "editor.findMatchBackground": "#3b2f5a",
  //   "editor.findMatchHighlightBackground": "#24203a",
  //   /* ===== 括号 ===== */
  //   "editorBracketMatch.background": "#2a3550",
  //   "editorBracketMatch.border": "#c8d3f5",
  //   /* ===== 分割线 ===== */
  //   "editorGroup.border": "#1e222e",
  //   "sideBar.border": "#1e222e",
  //   "panel.border": "#1e222e",
  //   /* ===== Terminal ====*/
  //   "terminal.foreground": "#c8d3f5",
  //   "terminal.background": "#14161b",
  //   "terminalCursor.foreground": "#c8d3f5",
  //   "terminal.ansiBlack": "#14161b",
  //   "terminal.ansiRed": "#d46a6a",
  //   "terminal.ansiGreen": "#8fbf9f",
  //   "terminal.ansiYellow": "#d7ba7d",
  //   "terminal.ansiBlue": "#8aadf4",
  //   "terminal.ansiMagenta": "#b197fc",
  //   "terminal.ansiCyan": "#7dc4e4",
  //   "terminal.ansiWhite": "#c8d3f5"
  // }
  // "editor.tokenColorCustomizations": {
  //   "comments": "#6b7280",
  //   "keywords": "#b197fc",
  //   "variables": "#8bd5ff",
  //   "strings": "#9ee6a8",
  //   "functions": "#ffd28a",
  //   "numbers": "#7dd3fc",
  //   "types": "#93c5fd"
  // }
  /* 方案四*/
  // ===============================
  // A) Workbench + Editor + Terminal（统一黑底但更亮、更清晰）
  // ===============================
  "workbench.colorCustomizations": {
    "titleBar.activeBackground": "#1B1919",
    "titleBar.activeForeground": "#EDEDED",
    "titleBar.inactiveBackground": "#171515",
    "titleBar.inactiveForeground": "#BDBDBD",
    "activityBar.background": "#1B1919",
    "activityBar.foreground": "#EDEDED",
    "activityBar.inactiveForeground": "#9C9C9C",
    "activityBarBadge.background": "#3A86FF",
    "activityBarBadge.foreground": "#FFFFFF",
    "sideBar.background": "#171515",
    "sideBar.foreground": "#DADADA",
    "sideBarTitle.foreground": "#EDEDED",
    "sideBarSectionHeader.background": "#2c2a2a",
    "sideBarSectionHeader.foreground": "#EDEDED",
    "statusBar.background": "#1B1919",
    "statusBar.foreground": "#EDEDED",
    "statusBar.noFolderBackground": "#1B1919",
    "statusBar.debuggingBackground": "#2A1E1E",
    "panel.background": "#171515",
    "panel.border": "#2A2727",
    "panelTitle.activeForeground": "#EDEDED",
    "panelTitle.inactiveForeground": "#A9A9A9",
    // ---- Tab：避免“雾蒙蒙”，加强 active tab 对比 ----
    "editorGroupHeader.tabsBackground": "#171515",
    "tab.activeBackground": "#1F1C1C",
    "tab.activeForeground": "#FFFFFF",
    "tab.inactiveBackground": "#171515",
    "tab.inactiveForeground": "#B3B3B3",
    "tab.border": "#2A2727",
    // ---- Editor：以 #161414 为基准 ----
    "editor.background": "#161414",
    "editor.foreground": "#EAEAEA",
    "editorLineNumber.foreground": "#6E6A6A",
    "editorLineNumber.activeForeground": "#CFCFCF",
    "editorCursor.foreground": "#F2F2F2",
    "editor.selectionBackground": "#737470",
    "editor.inactiveSelectionBackground": "#242B3A",
    "editor.wordHighlightBackground": "#2B2430",
    "editor.wordHighlightStrongBackground": "#3A2B41",
    "editor.findMatchBackground": "#4A3B1D",
    // "editor.findMatchHighlightBackground": "#b0b1b1",
    // "editor.findRangeHighlightBackground": "#c1cad4",
    "editor.lineHighlightBackground": "#1E1B1B",
    "editorIndentGuide.background1": "#2A2626",
    "editorIndentGuide.activeBackground1": "#3A3434",
    "editorBracketMatch.background": "#2A2727",
    "editorBracketMatch.border": "#7A6F6F",
    // ---- Peek / Suggest：更亮更清楚 ----
    "editorSuggestWidget.background": "#1B1919",
    "editorSuggestWidget.foreground": "#EDEDED",
    "editorSuggestWidget.border": "#2A2727",
    "editorSuggestWidget.selectedBackground": "#2E3A5A",
    "peekViewEditor.background": "#161414",
    "peekViewResult.background": "#171515",
    "peekView.border": "#2A2727",
    // ---- Terminal：统一到同色系、提高可读性 ----
    "terminal.background": "#1a1818",
    "terminal.foreground": "#F0F0F0",
    "terminalCursor.foreground": "#F0F0F0",
    "terminalCursor.background": "#F0F0F0",
    // ---- 你原来的 gitGraph 自定义色也合并在这里（避免被覆盖）----
    "gitGraph.verticalLine": "#333333",
    "gitGraph.verticalLineHighlighted": "#666666",
    "gitGraph.branchLabel": "#ffffff",
    "gitGraph.branchLabelBackground": "#007acc",
    "gitGraph.commitDot": "#ffffff",
    "gitGraph.commitDotHighlighted": "#ffff00"
  },
  // ===============================
  // B) TextMate token（传统语法高亮）：提亮 + 强区分
  // ===============================
  "editor.tokenColorCustomizations": {
    "comments": "#7FD6A9",
    "strings": "#7CFFB2",
    "keywords": "#FF6BDE",
    "numbers": "#66F0FF",
    "types": "#68B8FF",
    "functions": "#FFD166",
    "variables": "#8DEBFF",
    "textMateRules": [
      // 取消你之前对字符串引号的 bold（避免“看起来加粗”）
      {
        "scope": "punctuation.definition.string.begin",
        "settings": {
          "foreground": "#7CFFB2",
          "fontStyle": ""
        }
      },
      {
        "scope": "punctuation.definition.string.end",
        "settings": {
          "foreground": "#7CFFB2",
          "fontStyle": ""
        }
      },
      // 参数 / 局部变量：更亮更青
      {
        "scope": [
          "variable.parameter",
          "meta.function.parameters variable"
        ],
        "settings": {
          "foreground": "#B6F7FF",
          "fontStyle": ""
        }
      },
      // 函数/方法名：更偏金色（和变量区分）
      {
        "scope": [
          "entity.name.function",
          "support.function",
          "meta.function-call"
        ],
        "settings": {
          "foreground": "#FFD166",
          "fontStyle": ""
        }
      },
      // 类/结构体/类型名：偏蓝
      {
        "scope": [
          "entity.name.type",
          "entity.name.class",
          "support.type"
        ],
        "settings": {
          "foreground": "#68B8FF",
          "fontStyle": ""
        }
      }
    ]
  },
  // ===============================
  // C) Semantic tokens（语义高亮）：让“函数名/类名/参数/成员”区分明显
  // ===============================
  "editor.semanticTokenColorCustomizations": {
    "enabled": true,
    "rules": {
      // 函数/方法：金色系
      "function": {
        "foreground": "#FFD166",
        "fontStyle": ""
      },
      "method": {
        "foreground": "#FFC857",
        "fontStyle": ""
      },
      // 类/结构体/接口/类型：更亮的蓝
      "class": {
        "foreground": "#6FB7FF",
        "fontStyle": ""
      },
      "struct": {
        "foreground": "#6FB7FF",
        "fontStyle": ""
      },
      "interface": {
        "foreground": "#78C2FF",
        "fontStyle": ""
      },
      "type": {
        "foreground": "#68B8FF",
        "fontStyle": ""
      },
      "typeParameter": {
        "foreground": "#8ACBFF",
        "fontStyle": ""
      },
      // 参数：亮青（最容易区分）
      "parameter": {
        "foreground": "#B6F7FF",
        "fontStyle": ""
      },
      // 普通变量：更淡的青；成员变量/属性：偏紫一点
      "variable": {
        "foreground": "#8DEBFF",
        "fontStyle": ""
      },
      "property": {
        "foreground": "#D5A6FF",
        "fontStyle": ""
      },
      "member": {
        "foreground": "#D5A6FF",
        "fontStyle": ""
      },
      // 枚举/常量：偏橙黄
      "enumMember": {
        "foreground": "#FFB86B",
        "fontStyle": ""
      },
      "constant": {
        "foreground": "#FFB86B",
        "fontStyle": ""
      },
      // 命名空间/宏：区分开
      "namespace": {
        "foreground": "#A6B4FF",
        "fontStyle": ""
      },
      "macro": {
        "foreground": "#FF6BDE",
        "fontStyle": ""
      }
    }
  },
  // ===================== notebooks / associations =====================
  "workbench.editorAssociations": {
    "*.ipynb": "jupyter.notebook.ipynb",
    "*.md": "default"
  },
  // ===================== leetcode =====================
  "leetcode.defaultLanguage": "cpp",
  "leetcode.nodePath": "/usr/bin/node",
  "leetcode.endpoint": "leetcode-cn",
  // ===================== git blame =====================
  "gitblame.inlineMessageEnabled": true,
  "gitblame.inlineMessageFormat": "${author.name}, (${time.ago}) · ${commit.summary}",
  // ===================== project manager =====================
  "projectManager.git.baseFolders": [
    "/media/disk/2818/",
    "/media/disk/2818_b07/",
    "/media/disk/3146/",
    "/media/disk/3097/"
  ],
  // ===================== fnMap =====================
  "fnMap.registrationCode": "c6lqhMfX4yzq57fLzIC32LeYmIbinWgINdSsNq9ihSy4LHxnYm3NbfpTArHRyTeyA4mlam7m1nolC8rG/i0kDhrl3+e9Z94oGkYLubPxslpeuHRydp814NRTUrE2YjaEDwn7mHsc9nIfA/aIqj8ftQ1VroVbs4cPBC0l4VvizjAe1/CPMwdyFqhJwL0Z+VqjdZBTWwBTQx2gRwZ8pez93SUaXROJ+yN69jGQoC+Al0LzIKBfIDZPCW0hHT/D/xWBz3xdF6+17ucyqqMHY+Lm1Xjcwwq8OA+bTuTrBBGgL4VMGtzKQNJFcmkqJ8Op+clx54ii99y9kHvUTXhex0QyDg==",
  // ===================== debug =====================
  "debug.onTaskErrors": "debugAnyway",
  "debugVisualizer.debugAdapterConfigurations": {},
  // ===================== git-worktree-manager =====================
  "git-worktree-manager.gitHistoryExtension": "mhutchie.git-graph",
  "git-worktree-manager.checkoutIgnoreOtherWorktree": true,
  "git-worktree-manager.promptDeleteBranchAfterWorktreeDeletion": true,
  "git-worktree-manager.terminalLocationInEditor": true,
  "git-worktree-manager.postCreateCmd": "/media/disk/2818_proj/dev_Loc/.git/worktrees",
  "git-worktree-manager.worktreePathTemplate": "/media/disk/2818_proj/dev_Loc/.git/worktrees",
  // ===================== git-graph（原样保留，只是整理位置/去重） =====================
  "git-graph.repository.commits.initialLoad": 300, // 初始加载的提交数量
  "git-graph.repository.commits.loadMore": 100, // 点击"加载更多"时加载的数量
  "git-graph.repository.commits.refreshOnVisible": true, // 窗口可见时自动刷新
  "git-graph.repository.commits.autoCenter": true, // 自动居中显示 HEAD
  // 提交信息显示设置
  "git-graph.repository.commits.showSignatureStatus": true, // 显示签名状态
  "git-graph.repository.commits.showTags": true, // 显示标签
  "git-graph.repository.commits.showBranches": true, // 显示分支
  "git-graph.repository.commits.showRemoteBranches": true, // 显示远程分支
  "git-graph.repository.commits.showStashes": true, // 显示储藏
  "git-graph.repository.commits.showCommitsOnlyReferencedByTags": true,
  // 图形显示设置
  "git-graph.repository.commits.graph.style": "rounded", // 图形样式：rounded/angular
  "git-graph.repository.commits.graph.colours": [
    "#0080ff",
    "#e60000",
    "#00cc00",
    "#bf00ff",
    "#ff8000",
    "#00cccc",
    "#cc00cc",
    "#cccc00",
    "#4d4d4d",
    "#ff4d4d"
  ],
  "git-graph.repository.commits.graph.grid": {
    "x": 16,
    "y": 24
  },
  // 界面布局设置
  "git-graph.repository.commits.view.style": "split", // split/unified
  "git-graph.repository.commits.view.splitDir": "horizontal", // horizontal/vertical
  "git-graph.repository.view.orientation": "vertical", // vertical/horizontal
  // 日期和时间格式
  "git-graph.repository.commits.date.format": "Date & Time",
  "git-graph.repository.commits.date.type": "Author Date",
  "git-graph.repository.commits.date.localTimezone": false,
  // 筛选和搜索设置
  "git-graph.repository.commits.showCurrentBranchOnly": false,
  "git-graph.repository.commits.includeCommitsMentionedByReflogs": true,
  "git-graph.repository.fetchAndPrune": true,
  // 右键菜单可见性（原来后面也有一次空对象，这里保留“有效那份”）
  "git-graph.contextMenuActionsVisibility": {
    "branch": true,
    "commit": true,
    "stash": true,
    "tag": true,
    "uncommittedChanges": true
  },
  // "git-graph.commitOrdering": "topological",
  "git-graph.customToolbarActions": [
    {
      "icon": "refresh",
      "text": "刷新",
      "action": "refresh"
    },
    {
      "icon": "repo-fetch",
      "text": "拉取",
      "action": "fetch"
    }
  ],
  // 文件和差异查看器设置
  "git-graph.fileView.fileTree.compactFolders": false,
  "git-graph.fileView.fileTree.show": true,
  "git-graph.diffView.openFilesInNewTab": false,
  "git-graph.diffView.wordWrap": "off",
  "git-graph.repository.commits.showMergeCommits": true,
  "git-graph.repository.onLoad.scrollToHead": true,
  "git-graph.repository.showStatusBarItem": true,
  // 对话框设置
  "git-graph.dialog.addTag.preserveFocus": true,
  "git-graph.dialog.createBranch.preserveFocus": true,
  "git-graph.dialog.merge.preserveFocus": true,
  "git-graph.dialog.rebase.preserveFocus": true,
  // 主题和颜色设置
  "git-graph.repository.commits.avatar": true,
  "git-graph.repository.commits.avatarStorage": "local",
  // ===================== other =====================
  "copyOnSelect.copyOnMouseSelection": false,
  // 1) 当前你把 occurrencesHighlight 关了，建议只关“干扰”，保留“可读性”
  "editor.occurrencesHighlight": "singleFile",
  // 2) 让光标和选区更稳定（你开了平滑动画，配合这个更不晃眼）
  "editor.cursorWidth": 2,
  "editor.renderWhitespace": "selection",
  // 3) 代码阅读更友好
  // "editor.rulers": [100],
  "editor.stickyScroll.enabled": true,
  "editor.fontWeight": "450",
  "editor.semanticHighlighting.enabled": true,
  // ===== 字体与排版 =====
  "editor.fontSize": 14,
  "editor.lineHeight": 21,
  "editor.fontFamily": "'Monaspace Neon', JetBrains Mono, Fira Code, Consolas, 'Noto Sans Mono', monospace",
  "editor.fontLigatures": false, // 不喜欢连字就关；喜欢可改 true
  "editor.fontWeight": "450",
  // ===== 视线引导：缩进 / 行 / 括号 =====
  "editor.renderLineHighlight": "gutter", // 只高亮行号区，不“糊”整行
  "editor.bracketPairColorization.enabled": true,
  "editor.guides.indentation": true,
  "editor.guides.highlightActiveIndentation": true,
  "editor.indentSize": "tabSize",
  // ===== 阅读舒适：空白/换行/滚动 =====
  "editor.wordWrapColumn": 120,
  "editor.smoothScrolling": true,
  "editor.scrollBeyondLastLine": false,
  "editor.minimap.enabled": true, // 读代码建议关，减少噪声（你也可开）
  // ===== 降低干扰：提示/悬浮/高亮 =====
  "editor.hover.delay": 250,
  "editor.parameterHints.enabled": true,
  "editor.lightbulb.enabled": "off",
  "extensions.ignoreRecommendations": true, // 少点灯泡干扰
  // === 大型文件支持 ===
  "editor.maxTokenizationLineLength": 300000,
  // 大文件不要降级为“无换行可读模式”
  "editor.largeFileOptimizations": false,
  // 提升大文件允许使用的内存
  "files.maxMemoryForLargeFilesMB": 8192,
  "[plaintext]": {
    "editor.wordWrap": "on",
    "editor.wordWrapColumn": 120,
    "editor.minimap.enabled": false,
    "editor.semanticHighlighting.enabled": false,
    "editor.smoothScrolling": false
  },
  "editor.foldingLevel": 4,
  "terminal.integrated.enableMultiLinePasteWarning": false,
  "gitlens.graph.minimap.enabled": false,
  "gitlens.graph.minimap.additionalTypes": [
    "localBranches"
  ],
  "gitlens.graph.minimap.dataType": "lines",
  "gitlens.worktrees.defaultLocation": "Editor",
  "gitlens.graph.layout": "editor",
  "gitlens.hovers.currentLine.over": "line",
  "C_Cpp.files.exclude": {
    "**/.vscode": true,
    "**/.vs": true
  },
  "gitlens.defaultDateLocale": null,
  "editor.stickyScroll.maxLineCount": 10,
  "Codegeex.License": "",
  "Codegeex.Privacy": false,
  "Codegeex.Local": {
    "apiURL": "",
    "useChatGLM": true,
    "chatGLM": {
      "apiKey": "",
      "model": ""
    },
    "chat": {
      "useDefaultSystemPrompt": true,
      "systemPrompt": "",
      "temperature": 0.2,
      "top_p": 0.95,
      "max_tokens": 1024,
      "presence_penalty": 1
    },
    "completions": {
      "useDefaultSystemPrompt": true,
      "systemPrompt": "",
      "temperature": 0.2,
      "top_p": 0.95,
      "max_tokens": 64,
      "presence_penalty": 1
    }
  },
  "Codegeex.CompletionModel": "CodeGeeX Pro[Beta]",
  "Codegeex.Comment.LanguagePreference": "中文",
  "Codegeex.SidebarUI.LanguagePreference": "中文",
  "Codegeex.CompletionDelay": 2,
  "Codegeex.OnlyKeyControl": true,
  "Codegeex.CommitMessage.LanguagePreference": "中文",
  "Codegeex.Chat.LanguagePreference": "中文",
  "diffEditor.ignoreTrimWhitespace": true,
  "editor.tabSize": 2,
  "codegeex.codeLens.enableCodeLens": "enable",
  "clang-format.executable": "/usr/bin/clang-format",
  "clang-format.style": "{ BasedOnStyle: Google, PointerAlignment: Left, SpacesBeforeTrailingComments: 2, IndentWidth: 2 }",
  "clang-format.fallbackStyle": "Google",
  "files.participants.timeout": 0,
  "clangd.path": "/home/xmtte/.config/Code/User/globalStorage/llvm-vs-code-extensions.vscode-clangd/install/22.1.0/clangd_22.1.0/bin/clangd",
  "clangd.arguments": [
    "--compile-commands-dir=${workspaceFolder}/build",
    "--pch-storage=disk",
    "-j=14",
    "--background-index",
    // "--header-insertion=never", // 封印乱加头文件的行为
    // "--clang-tidy", // 开启静态分析
    "--query-driver=/usr/bin/g++*", // 防止找不到 STL 标准库
    "--log=verbose",
    "--fallback-style=none",
  ],
  // "clangd.fallbackFlags": [
  //   "-xc++",
  //   "-std=c++17",
  //   "-I${workspaceFolder}/tools",
  //   "-I${workspaceFolder}/src",
  //   "-I${workspaceFolder}/third_party",
  //   "-I${workspaceFolder}/third_party/adol_c_2.6.3/include",
  //   "-I${workspaceFolder}/third_party/ipopt_3.11.9/include",
  //   "-I${workspaceFolder}/third_party/protobuf_3.20.0-rc-2/include",
  //   "-I/media/disk/program/Qt5.9.5/5.9.5/gcc_64/include",
  // ],
  "[jsonc]": {
    "editor.defaultFormatter": "vscode.json-language-features"
  },
  "window.newWindowDimensions": "inherit",
  "claudeCode.preferredLocation": "panel",
  "claudeCode.selectedModel": "minimax-m2.7",
  "claudeCode.environmentVariables": [
    {
      "name": "ANTHROPIC_BASE_URL",
      "value": "https://api.minimaxi.com/anthropic"
      // "value": "https://api.minimaxi.com/v1"
    },
    {
      "name": "ANTHROPIC_AUTH_TOKEN",
      "value": "sk-cp-DQbfvCPxnnX3xgZjzgJX5AshwYJ-l0XhzMdE3iGD1iUqzvC8tZPeVXTFjf9hgsrYutO5W42Nbj7waGqynxJp82uh99mV3wziqOPq-GMc_t63BhOyY8-TMDo"
    },
    {
      "name": "API_TIMEOUT_MS",
      "value": "3000000"
    },
    {
      "name": "CLAUDE_CODE_DISABLE_NONESSENTIAL_TRAFFIC",
      "value": "1"
    },
    {
      "name": "ANTHROPIC_MODEL",
      "value": "MiniMax-M2.7"
    },
    {
      "name": "ANTHROPIC_DEFAULT_SONNET_MODEL",
      "value": "MiniMax-M2.7"
    },
    {
      "name": "ANTHROPIC_DEFAULT_OPUS_MODEL",
      "value": "MiniMax-M2.7"
    },
    {
      "name": "ANTHROPIC_DEFAULT_HAIKU_MODEL",
      "value": "MiniMax-M2.7"
    }
  ],
  "permissions.default": "allow",
  "github.copilot.chat.pullRequestDescriptionGeneration.instructions": [],
  "oaicopilot.commitLanguage": "Chinese (Simplified)",
  "gitlens.ai.model": "vscode",
  "gitlens.ai.vscode.model": "copilot:deepseek",
  "terminal.integrated.gpuAcceleration": "off",
  "git.blame.editorDecoration.enabled": true,
  "git.diagnosticsCommitHook.enabled": true,
  "python.createEnvironment.trigger": "off",
  "oaicopilot.models": [
    {
      "id": "__provider__deepseek",
      "owned_by": "deepseek",
      "baseUrl": "https://api.deepseek.com",
      "apiMode": "openai"
    },
    {
      "id": "deepseek-v4-flash",
      "owned_by": "deepseek",
      "displayName": "deepseek-v4-flash",
      "configId": "thinking",
      "baseUrl": "https://api.deepseek.com",
      "family": "deepseek-v4",
      "context_length": 1000000,
      "max_tokens": 4096,
      "apiMode": "openai",
      "temperature": 0,
      "reasoning_effort": "high",
      "include_reasoning_in_request": true,
      "thinking": {
        "type": "enabled"
      }
    },
    {
      "id": "__provider__minimax",
      "owned_by": "minimax",
      "baseUrl": "https://api.minimaxi.com/v1",
      "apiMode": "openai"
    },
    {
      "id": "MiniMax-M2.7",
      "owned_by": "minimax",
      "baseUrl": "https://api.minimaxi.com/v1",
      "context_length": 128000,
      "max_tokens": 4096,
      "apiMode": "openai",
      "temperature": 0,
      "include_reasoning_in_request": true
    }
  ],
  "chat.viewSessions.orientation": "stacked",
  "claudeCode.allowDangerouslySkipPermissions": true,
  "github.copilot.chat.claudeAgent.allowDangerouslySkipPermissions": true,
  "scm.repositories.selectionMode": "single",
  "editor.copyWithSyntaxHighlighting": false,
  "github.copilot.nextEditSuggestions.enabled": false,
  "github.copilot.enable": {
    "*": false,
    "plaintext": false,
    "markdown": false,
    "scminput": false,
    "cpp": false
  },
  "Codegeex.GenerationPreference": "automatic",
  "diffEditor.renderSideBySide": true,
  "diffEditor.experimental.showMoves": true,
  
}