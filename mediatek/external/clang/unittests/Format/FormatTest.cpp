//===- unittest/Format/FormatTest.cpp - Formatting unit tests -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "format-test"

#include "clang/Format/Format.h"
#include "clang/Lex/Lexer.h"
#include "llvm/Support/Debug.h"
#include "gtest/gtest.h"

namespace clang {
namespace format {

class FormatTest : public ::testing::Test {
protected:
  std::string format(llvm::StringRef Code, unsigned Offset, unsigned Length,
                     const FormatStyle &Style) {
    DEBUG(llvm::errs() << "---\n");
    std::vector<tooling::Range> Ranges(1, tooling::Range(Offset, Length));
    tooling::Replacements Replaces = reformat(Style, Code, Ranges);
    ReplacementCount = Replaces.size();
    std::string Result = applyAllReplacements(Code, Replaces);
    EXPECT_NE("", Result);
    DEBUG(llvm::errs() << "\n" << Result << "\n\n");
    return Result;
  }

  std::string
  format(llvm::StringRef Code, const FormatStyle &Style = getLLVMStyle()) {
    return format(Code, 0, Code.size(), Style);
  }

  std::string messUp(llvm::StringRef Code) {
    std::string MessedUp(Code.str());
    bool InComment = false;
    bool InPreprocessorDirective = false;
    bool JustReplacedNewline = false;
    for (unsigned i = 0, e = MessedUp.size() - 1; i != e; ++i) {
      if (MessedUp[i] == '/' && MessedUp[i + 1] == '/') {
        if (JustReplacedNewline)
          MessedUp[i - 1] = '\n';
        InComment = true;
      } else if (MessedUp[i] == '#' && (JustReplacedNewline || i == 0)) {
        if (i != 0)
          MessedUp[i - 1] = '\n';
        InPreprocessorDirective = true;
      } else if (MessedUp[i] == '\\' && MessedUp[i + 1] == '\n') {
        MessedUp[i] = ' ';
        MessedUp[i + 1] = ' ';
      } else if (MessedUp[i] == '\n') {
        if (InComment) {
          InComment = false;
        } else if (InPreprocessorDirective) {
          InPreprocessorDirective = false;
        } else {
          JustReplacedNewline = true;
          MessedUp[i] = ' ';
        }
      } else if (MessedUp[i] != ' ') {
        JustReplacedNewline = false;
      }
    }
    return MessedUp;
  }

  FormatStyle getLLVMStyleWithColumns(unsigned ColumnLimit) {
    FormatStyle Style = getLLVMStyle();
    Style.ColumnLimit = ColumnLimit;
    return Style;
  }

  FormatStyle getGoogleStyleWithColumns(unsigned ColumnLimit) {
    FormatStyle Style = getGoogleStyle();
    Style.ColumnLimit = ColumnLimit;
    return Style;
  }

  void verifyFormat(llvm::StringRef Code,
                    const FormatStyle &Style = getLLVMStyle()) {
    EXPECT_EQ(Code.str(), format(messUp(Code), Style));
  }

  void verifyGoogleFormat(llvm::StringRef Code) {
    verifyFormat(Code, getGoogleStyle());
  }

  void verifyIndependentOfContext(llvm::StringRef text) {
    verifyFormat(text);
    verifyFormat(llvm::Twine("void f() { " + text + " }").str());
  }

  int ReplacementCount;
};

TEST_F(FormatTest, MessUp) {
  EXPECT_EQ("1 2 3", messUp("1 2 3"));
  EXPECT_EQ("1 2 3\n", messUp("1\n2\n3\n"));
  EXPECT_EQ("a\n//b\nc", messUp("a\n//b\nc"));
  EXPECT_EQ("a\n#b\nc", messUp("a\n#b\nc"));
  EXPECT_EQ("a\n#b  c  d\ne", messUp("a\n#b\\\nc\\\nd\ne"));
}

//===----------------------------------------------------------------------===//
// Basic function tests.
//===----------------------------------------------------------------------===//

TEST_F(FormatTest, DoesNotChangeCorrectlyFormatedCode) {
  EXPECT_EQ(";", format(";"));
}

TEST_F(FormatTest, FormatsGlobalStatementsAt0) {
  EXPECT_EQ("int i;", format("  int i;"));
  EXPECT_EQ("\nint i;", format(" \n\t \r  int i;"));
  EXPECT_EQ("int i;\nint j;", format("    int i; int j;"));
  EXPECT_EQ("int i;\nint j;", format("    int i;\n  int j;"));
}

TEST_F(FormatTest, FormatsUnwrappedLinesAtFirstFormat) {
  EXPECT_EQ("int i;", format("int\ni;"));
}

TEST_F(FormatTest, FormatsNestedBlockStatements) {
  EXPECT_EQ("{\n  {\n    {}\n  }\n}", format("{{{}}}"));
}

TEST_F(FormatTest, FormatsNestedCall) {
  verifyFormat("Method(f1, f2(f3));");
  verifyFormat("Method(f1(f2, f3()));");
  verifyFormat("Method(f1(f2, (f3())));");
}

TEST_F(FormatTest, NestedNameSpecifiers) {
  verifyFormat("vector< ::Type> v;");
  verifyFormat("::ns::SomeFunction(::ns::SomeOtherFunction())");
}

TEST_F(FormatTest, OnlyGeneratesNecessaryReplacements) {
  EXPECT_EQ("if (a) {\n"
            "  f();\n"
            "}",
            format("if(a){f();}"));
  EXPECT_EQ(4, ReplacementCount);
  EXPECT_EQ("if (a) {\n"
            "  f();\n"
            "}",
            format("if (a) {\n"
                   "  f();\n"
                   "}"));
  EXPECT_EQ(0, ReplacementCount);
}

TEST_F(FormatTest, RemovesTrailingWhitespaceOfFormattedLine) {
  EXPECT_EQ("int a;\nint b;", format("int a; \nint b;", 0, 0, getLLVMStyle()));
  EXPECT_EQ("int a;", format("int a;         "));
  EXPECT_EQ("int a;\n", format("int a;  \n   \n   \n "));
  EXPECT_EQ("int a;\nint b;    ",
            format("int a;  \nint b;    ", 0, 0, getLLVMStyle()));
}

TEST_F(FormatTest, FormatsCorrectRegionForLeadingWhitespace) {
  EXPECT_EQ("int b;\nint a;",
            format("int b;\n   int a;", 7, 0, getLLVMStyle()));
  EXPECT_EQ("int b;\n   int a;",
            format("int b;\n   int a;", 6, 0, getLLVMStyle()));

  EXPECT_EQ("#define A  \\\n"
            "  int a;   \\\n"
            "  int b;",
            format("#define A  \\\n"
                   "  int a;   \\\n"
                   "    int b;",
                   26, 0, getLLVMStyleWithColumns(12)));
  EXPECT_EQ("#define A  \\\n"
            "  int a;   \\\n"
            "    int b;",
            format("#define A  \\\n"
                   "  int a;   \\\n"
                   "    int b;",
                   25, 0, getLLVMStyleWithColumns(12)));
}

TEST_F(FormatTest, RemovesWhitespaceWhenTriggeredOnEmptyLine) {
  EXPECT_EQ("int  a;\n\n int b;",
            format("int  a;\n  \n\n int b;", 7, 0, getLLVMStyle()));
  EXPECT_EQ("int  a;\n\n int b;",
            format("int  a;\n  \n\n int b;", 9, 0, getLLVMStyle()));
}

TEST_F(FormatTest, RemovesEmptyLines) {
  EXPECT_EQ("class C {\n"
            "  int i;\n"
            "};",
            format("class C {\n"
                   " int i;\n"
                   "\n"
                   "};"));

  // Don't remove empty lines in more complex control statements.
  EXPECT_EQ("void f() {\n"
            "  if (a) {\n"
            "    f();\n"
            "\n"
            "  } else if (b) {\n"
            "    f();\n"
            "  }\n"
            "}",
            format("void f() {\n"
                   "  if (a) {\n"
                   "    f();\n"
                   "\n"
                   "  } else if (b) {\n"
                   "    f();\n"
                   "\n"
                   "  }\n"
                   "\n"
                   "}"));

  // FIXME: This is slightly inconsistent.
  EXPECT_EQ("namespace {\n"
            "int i;\n"
            "}",
            format("namespace {\n"
                   "int i;\n"
                   "\n"
                   "}"));
  EXPECT_EQ("namespace {\n"
            "int i;\n"
            "\n"
            "} // namespace",
            format("namespace {\n"
                   "int i;\n"
                   "\n"
                   "}  // namespace"));
}

TEST_F(FormatTest, ReformatsMovedLines) {
  EXPECT_EQ(
      "template <typename T> T *getFETokenInfo() const {\n"
      "  return static_cast<T *>(FETokenInfo);\n"
      "}\n"
      "  int a; // <- Should not be formatted",
      format(
          "template<typename T>\n"
          "T *getFETokenInfo() const { return static_cast<T*>(FETokenInfo); }\n"
          "  int a; // <- Should not be formatted",
          9, 5, getLLVMStyle()));
}

//===----------------------------------------------------------------------===//
// Tests for control statements.
//===----------------------------------------------------------------------===//

TEST_F(FormatTest, FormatIfWithoutCompoundStatement) {
  verifyFormat("if (true)\n  f();\ng();");
  verifyFormat("if (a)\n  if (b)\n    if (c)\n      g();\nh();");
  verifyFormat("if (a)\n  if (b) {\n    f();\n  }\ng();");

  FormatStyle AllowsMergedIf = getLLVMStyle();
  AllowsMergedIf.AllowShortIfStatementsOnASingleLine = true;
  verifyFormat("if (a)\n"
               "  // comment\n"
               "  f();",
               AllowsMergedIf);
  verifyFormat("if (a)\n"
               "  ;",
               AllowsMergedIf);
  verifyFormat("if (a)\n"
               "  if (b) return;",
               AllowsMergedIf);

  verifyFormat("if (a) // Can't merge this\n"
               "  f();\n",
               AllowsMergedIf);
  verifyFormat("if (a) /* still don't merge */\n"
               "  f();",
               AllowsMergedIf);
  verifyFormat("if (a) { // Never merge this\n"
               "  f();\n"
               "}",
               AllowsMergedIf);
  verifyFormat("if (a) { /* Never merge this */\n"
               "  f();\n"
               "}",
               AllowsMergedIf);

  EXPECT_EQ("if (a) return;", format("if(a)\nreturn;", 7, 1, AllowsMergedIf));
  EXPECT_EQ("if (a) return; // comment",
            format("if(a)\nreturn; // comment", 20, 1, AllowsMergedIf));

  AllowsMergedIf.ColumnLimit = 14;
  verifyFormat("if (a) return;", AllowsMergedIf);
  verifyFormat("if (aaaaaaaaa)\n"
               "  return;",
               AllowsMergedIf);

  AllowsMergedIf.ColumnLimit = 13;
  verifyFormat("if (a)\n  return;", AllowsMergedIf);
}

TEST_F(FormatTest, FormatLoopsWithoutCompoundStatement) {
  FormatStyle AllowsMergedLoops = getLLVMStyle();
  AllowsMergedLoops.AllowShortLoopsOnASingleLine = true;
  verifyFormat("while (true) continue;", AllowsMergedLoops);
  verifyFormat("for (;;) continue;", AllowsMergedLoops);
  verifyFormat("for (int &v : vec) v *= 2;", AllowsMergedLoops);
  verifyFormat("while (true)\n"
               "  ;",
               AllowsMergedLoops);
  verifyFormat("for (;;)\n"
               "  ;",
               AllowsMergedLoops);
  verifyFormat("for (;;)\n"
               "  for (;;) continue;",
               AllowsMergedLoops);
  verifyFormat("for (;;) // Can't merge this\n"
               "  continue;",
               AllowsMergedLoops);
  verifyFormat("for (;;) /* still don't merge */\n"
               "  continue;",
               AllowsMergedLoops);
}

TEST_F(FormatTest, ParseIfElse) {
  verifyFormat("if (true)\n"
               "  if (true)\n"
               "    if (true)\n"
               "      f();\n"
               "    else\n"
               "      g();\n"
               "  else\n"
               "    h();\n"
               "else\n"
               "  i();");
  verifyFormat("if (true)\n"
               "  if (true)\n"
               "    if (true) {\n"
               "      if (true)\n"
               "        f();\n"
               "    } else {\n"
               "      g();\n"
               "    }\n"
               "  else\n"
               "    h();\n"
               "else {\n"
               "  i();\n"
               "}");
}

TEST_F(FormatTest, ElseIf) {
  verifyFormat("if (a) {\n} else if (b) {\n}");
  verifyFormat("if (a)\n"
               "  f();\n"
               "else if (b)\n"
               "  g();\n"
               "else\n"
               "  h();");
}

TEST_F(FormatTest, FormatsForLoop) {
  verifyFormat(
      "for (int VeryVeryLongLoopVariable = 0; VeryVeryLongLoopVariable < 10;\n"
      "     ++VeryVeryLongLoopVariable)\n"
      "  ;");
  verifyFormat("for (;;)\n"
               "  f();");
  verifyFormat("for (;;) {\n}");
  verifyFormat("for (;;) {\n"
               "  f();\n"
               "}");
  verifyFormat("for (int i = 0; (i < 10); ++i) {\n}");

  verifyFormat(
      "for (std::vector<UnwrappedLine>::iterator I = UnwrappedLines.begin(),\n"
      "                                          E = UnwrappedLines.end();\n"
      "     I != E; ++I) {\n}");

  verifyFormat(
      "for (MachineFun::iterator IIII = PrevIt, EEEE = F.end(); IIII != EEEE;\n"
      "     ++IIIII) {\n}");
  verifyFormat("for (aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa aaaaaaaaaaa =\n"
               "         aaaaaaaaaaaaaaaa.aaaaaaaaaaaaaaa;\n"
               "     aaaaaaaaaaa != aaaaaaaaaaaaaaaaaaa; ++aaaaaaaaaaa) {\n}");
  verifyFormat("for (llvm::ArrayRef<NamedDecl *>::iterator\n"
               "         I = FD->getDeclsInPrototypeScope().begin(),\n"
               "         E = FD->getDeclsInPrototypeScope().end();\n"
               "     I != E; ++I) {\n}");

  // FIXME: Not sure whether we want extra identation in line 3 here:
  verifyFormat(
      "for (aaaaaaaaaaaaaaaaa aaaaaaaaaaa = aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa;\n"
      "     aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa !=\n"
      "         aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(\n"
      "             aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa);\n"
      "     ++aaaaaaaaaaa) {\n}");
  verifyFormat("for (int aaaaaaaaaaa = 1; aaaaaaaaaaa <= bbbbbbbbbbbbbbb;\n"
               "     aaaaaaaaaaa++, bbbbbbbbbbbbbbbbb++) {\n"
               "}");
  verifyFormat("for (some_namespace::SomeIterator iter( // force break\n"
               "         aaaaaaaaaa);\n"
               "     iter; ++iter) {\n"
               "}");

  FormatStyle NoBinPacking = getLLVMStyle();
  NoBinPacking.BinPackParameters = false;
  verifyFormat("for (int aaaaaaaaaaa = 1;\n"
               "     aaaaaaaaaaa <= aaaaaaaaaaaaaaaaaaaaaa(aaaaaaaaaaaaaaaa,\n"
               "                                           aaaaaaaaaaaaaaaa,\n"
               "                                           aaaaaaaaaaaaaaaa,\n"
               "                                           aaaaaaaaaaaaaaaa);\n"
               "     aaaaaaaaaaa++, bbbbbbbbbbbbbbbbb++) {\n"
               "}",
               NoBinPacking);
  verifyFormat(
      "for (std::vector<UnwrappedLine>::iterator I = UnwrappedLines.begin(),\n"
      "                                          E = UnwrappedLines.end();\n"
      "     I != E;\n"
      "     ++I) {\n}",
      NoBinPacking);
}

TEST_F(FormatTest, RangeBasedForLoops) {
  verifyFormat("for (auto aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa :\n"
               "     aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa) {\n}");
  verifyFormat("for (auto aaaaaaaaaaaaaaaaaaaaa :\n"
               "     aaaaaaaaaaaaaaaaa(aaaaaaaaaaaaaaaa, aaaaaaaaaaaaa)) {\n}");
  verifyFormat("for (const aaaaaaaaaaaaaaaaaaaaa &aaaaaaaaa :\n"
               "     aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa) {\n}");
}

TEST_F(FormatTest, FormatsWhileLoop) {
  verifyFormat("while (true) {\n}");
  verifyFormat("while (true)\n"
               "  f();");
  verifyFormat("while () {\n}");
  verifyFormat("while () {\n"
               "  f();\n"
               "}");
}

TEST_F(FormatTest, FormatsDoWhile) {
  verifyFormat("do {\n"
               "  do_something();\n"
               "} while (something());");
  verifyFormat("do\n"
               "  do_something();\n"
               "while (something());");
}

TEST_F(FormatTest, FormatsSwitchStatement) {
  verifyFormat("switch (x) {\n"
               "case 1:\n"
               "  f();\n"
               "  break;\n"
               "case kFoo:\n"
               "case ns::kBar:\n"
               "case kBaz:\n"
               "  break;\n"
               "default:\n"
               "  g();\n"
               "  break;\n"
               "}");
  verifyFormat("switch (x) {\n"
               "case 1: {\n"
               "  f();\n"
               "  break;\n"
               "}\n"
               "}");
  verifyFormat("switch (x) {\n"
               "case 1: {\n"
               "  f();\n"
               "  {\n"
               "    g();\n"
               "    h();\n"
               "  }\n"
               "  break;\n"
               "}\n"
               "}");
  verifyFormat("switch (x) {\n"
               "case 1: {\n"
               "  f();\n"
               "  if (foo) {\n"
               "    g();\n"
               "    h();\n"
               "  }\n"
               "  break;\n"
               "}\n"
               "}");
  verifyFormat("switch (x) {\n"
               "case 1: {\n"
               "  f();\n"
               "  g();\n"
               "} break;\n"
               "}");
  verifyFormat("switch (test)\n"
               "  ;");
  verifyFormat("switch (x) {\n"
               "default: {\n"
               "  // Do nothing.\n"
               "}\n"
               "}");
  verifyFormat("switch (x) {\n"
               "// comment\n"
               "// if 1, do f()\n"
               "case 1:\n"
               "  f();\n"
               "}");
  verifyFormat("switch (x) {\n"
               "case 1:\n"
               "  // Do amazing stuff\n"
               "  {\n"
               "    f();\n"
               "    g();\n"
               "  }\n"
               "  break;\n"
               "}");
  verifyFormat("#define A          \\\n"
               "  switch (x) {     \\\n"
               "  case a:          \\\n"
               "    foo = b;       \\\n"
               "  }", getLLVMStyleWithColumns(20));

  verifyGoogleFormat("switch (x) {\n"
                     "  case 1:\n"
                     "    f();\n"
                     "    break;\n"
                     "  case kFoo:\n"
                     "  case ns::kBar:\n"
                     "  case kBaz:\n"
                     "    break;\n"
                     "  default:\n"
                     "    g();\n"
                     "    break;\n"
                     "}");
  verifyGoogleFormat("switch (x) {\n"
                     "  case 1: {\n"
                     "    f();\n"
                     "    break;\n"
                     "  }\n"
                     "}");
  verifyGoogleFormat("switch (test)\n"
                     "  ;");

  verifyGoogleFormat("#define OPERATION_CASE(name) \\\n"
                     "  case OP_name:              \\\n"
                     "    return operations::Operation##name\n");
  verifyGoogleFormat("Operation codeToOperation(OperationCode OpCode) {\n"
                     "  // Get the correction operation class.\n"
                     "  switch (OpCode) {\n"
                     "    CASE(Add);\n"
                     "    CASE(Subtract);\n"
                     "    default:\n"
                     "      return operations::Unknown;\n"
                     "  }\n"
                     "#undef OPERATION_CASE\n"
                     "}");
}

TEST_F(FormatTest, FormatsLabels) {
  verifyFormat("void f() {\n"
               "  some_code();\n"
               "test_label:\n"
               "  some_other_code();\n"
               "  {\n"
               "    some_more_code();\n"
               "  another_label:\n"
               "    some_more_code();\n"
               "  }\n"
               "}");
  verifyFormat("some_code();\n"
               "test_label:\n"
               "some_other_code();");
}

//===----------------------------------------------------------------------===//
// Tests for comments.
//===----------------------------------------------------------------------===//

TEST_F(FormatTest, UnderstandsSingleLineComments) {
  verifyFormat("//* */");
  verifyFormat("// line 1\n"
               "// line 2\n"
               "void f() {}\n");

  verifyFormat("void f() {\n"
               "  // Doesn't do anything\n"
               "}");
  verifyFormat("SomeObject\n"
               "    // Calling someFunction on SomeObject\n"
               "    .someFunction();");
  verifyFormat("void f(int i,  // some comment (probably for i)\n"
               "       int j,  // some comment (probably for j)\n"
               "       int k); // some comment (probably for k)");
  verifyFormat("void f(int i,\n"
               "       // some comment (probably for j)\n"
               "       int j,\n"
               "       // some comment (probably for k)\n"
               "       int k);");

  verifyFormat("int i    // This is a fancy variable\n"
               "    = 5; // with nicely aligned comment.");

  verifyFormat("// Leading comment.\n"
               "int a; // Trailing comment.");
  verifyFormat("int a; // Trailing comment\n"
               "       // on 2\n"
               "       // or 3 lines.\n"
               "int b;");
  verifyFormat("int a; // Trailing comment\n"
               "\n"
               "// Leading comment.\n"
               "int b;");
  verifyFormat("int a;    // Comment.\n"
               "          // More details.\n"
               "int bbbb; // Another comment.");
  verifyFormat(
      "int aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa; // comment\n"
      "int bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb;   // comment\n"
      "int cccccccccccccccccccccccccccccc;       // comment\n"
      "int ddd;                     // looooooooooooooooooooooooong comment\n"
      "int aaaaaaaaaaaaaaaaaaaaaaa; // comment\n"
      "int bbbbbbbbbbbbbbbbbbbbb;   // comment\n"
      "int ccccccccccccccccccc;     // comment");

  verifyFormat("#include \"a\"     // comment\n"
               "#include \"a/b/c\" // comment");
  verifyFormat("#include <a>     // comment\n"
               "#include <a/b/c> // comment");
  EXPECT_EQ("#include \\\n"
            "  \"a\"            // comment\n"
            "#include \"a/b/c\" // comment",
            format("#include \\\n"
                   "  \"a\" // comment\n"
                   "#include \"a/b/c\" // comment"));

  verifyFormat("enum E {\n"
               "  // comment\n"
               "  VAL_A, // comment\n"
               "  VAL_B\n"
               "};");

  verifyFormat(
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa =\n"
      "    bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb; // Trailing comment");
  verifyFormat("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa =\n"
               "    // Comment inside a statement.\n"
               "    bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb;");
  verifyFormat(
      "bool aaaaaaaaaaaaa = // comment\n"
      "    aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa || aaaaaaaaaaaaaaaaaaaaaaaaaaaa ||\n"
      "    aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa || aaaaaaaaaaaaaaaaaaaaaaaaaaaaa;");

  verifyFormat("int aaaa; // aaaaa\n"
               "int aa;   // aaaaaaa",
               getLLVMStyleWithColumns(20));

  EXPECT_EQ("void f() { // This does something ..\n"
            "}\n"
            "int a; // This is unrelated",
            format("void f()    {     // This does something ..\n"
                   "  }\n"
                   "int   a;     // This is unrelated"));
  EXPECT_EQ("class C {\n"
            "  void f() { // This does something ..\n"
            "  }          // awesome..\n"
            "\n"
            "  int a; // This is unrelated\n"
            "};",
            format("class C{void f()    { // This does something ..\n"
                   "      } // awesome..\n"
                   " \n"
                   "int a;    // This is unrelated\n"
                   "};"));

  EXPECT_EQ("int i; // single line trailing comment",
            format("int i;\\\n// single line trailing comment"));

  verifyGoogleFormat("int a;  // Trailing comment.");

  verifyFormat("someFunction(anotherFunction( // Force break.\n"
               "    parameter));");

  verifyGoogleFormat("#endif  // HEADER_GUARD");

  verifyFormat("const char *test[] = {\n"
               "  // A\n"
               "  \"aaaa\",\n"
               "  // B\n"
               "  \"aaaaa\",\n"
               "};");
  verifyGoogleFormat(
      "aaaaaaaaaaaaaaaaaaaaaaaaaa(\n"
      "    aaaaaaaaaaaaaaaaaaaaaa);  // 81_cols_with_this_comment");
  EXPECT_EQ("D(a, {\n"
            "  // test\n"
            "  int a;\n"
            "});",
            format("D(a, {\n"
                   "// test\n"
                   "int a;\n"
                   "});"));

  EXPECT_EQ("lineWith(); // comment\n"
            "// at start\n"
            "otherLine();",
            format("lineWith();   // comment\n"
                   "// at start\n"
                   "otherLine();"));
  EXPECT_EQ("lineWith(); // comment\n"
            "            // at start\n"
            "otherLine();",
            format("lineWith();   // comment\n"
                   " // at start\n"
                   "otherLine();"));

  EXPECT_EQ("lineWith(); // comment\n"
            "// at start\n"
            "otherLine(); // comment",
            format("lineWith();   // comment\n"
                   "// at start\n"
                   "otherLine();   // comment"));
  EXPECT_EQ("lineWith();\n"
            "// at start\n"
            "otherLine(); // comment",
            format("lineWith();\n"
                   " // at start\n"
                   "otherLine();   // comment"));
  EXPECT_EQ("// first\n"
            "// at start\n"
            "otherLine(); // comment",
            format("// first\n"
                   " // at start\n"
                   "otherLine();   // comment"));
  EXPECT_EQ("f();\n"
            "// first\n"
            "// at start\n"
            "otherLine(); // comment",
            format("f();\n"
                   "// first\n"
                   " // at start\n"
                   "otherLine();   // comment"));
}

TEST_F(FormatTest, CanFormatCommentsLocally) {
  EXPECT_EQ("int a;    // comment\n"
            "int    b; // comment",
            format("int   a; // comment\n"
                   "int    b; // comment",
                   0, 0, getLLVMStyle()));
  EXPECT_EQ("int   a; // comment\n"
            "         // line 2\n"
            "int b;",
            format("int   a; // comment\n"
                   "            // line 2\n"
                   "int b;",
                   28, 0, getLLVMStyle()));
  EXPECT_EQ("int aaaaaa; // comment\n"
            "int b;\n"
            "int c; // unrelated comment",
            format("int aaaaaa; // comment\n"
                   "int b;\n"
                   "int   c; // unrelated comment",
                   31, 0, getLLVMStyle()));
}

TEST_F(FormatTest, RemovesTrailingWhitespaceOfComments) {
  EXPECT_EQ("// comment", format("// comment  "));
  EXPECT_EQ("int aaaaaaa, bbbbbbb; // comment",
            format("int aaaaaaa, bbbbbbb; // comment                   ",
                   getLLVMStyleWithColumns(33)));
}

TEST_F(FormatTest, UnderstandsBlockComments) {
  verifyFormat("f(/*noSpaceAfterParameterNamingComment=*/true);");
  EXPECT_EQ(
      "f(aaaaaaaaaaaaaaaaaaaaaaaaa, /* Trailing comment for aa... */\n"
      "  bbbbbbbbbbbbbbbbbbbbbbbbb);",
      format("f(aaaaaaaaaaaaaaaaaaaaaaaaa ,   \\\n/* Trailing comment for aa... */\n"
             "  bbbbbbbbbbbbbbbbbbbbbbbbb);"));
  EXPECT_EQ(
      "f(aaaaaaaaaaaaaaaaaaaaaaaaa,\n"
      "  /* Leading comment for bb... */ bbbbbbbbbbbbbbbbbbbbbbbbb);",
      format("f(aaaaaaaaaaaaaaaaaaaaaaaaa    ,   \n"
             "/* Leading comment for bb... */   bbbbbbbbbbbbbbbbbbbbbbbbb);"));
  EXPECT_EQ(
      "void aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(\n"
      "    aaaaaaaaaaaaaaaaaa,\n"
      "    aaaaaaaaaaaaaaaaaa) { /*aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa*/\n"
      "}",
      format("void      aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(\n"
             "                      aaaaaaaaaaaaaaaaaa  ,\n"
             "    aaaaaaaaaaaaaaaaaa) {   /*aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa*/\n"
             "}"));

  FormatStyle NoBinPacking = getLLVMStyle();
  NoBinPacking.BinPackParameters = false;
  verifyFormat("aaaaaaaa(/* parameter 1 */ aaaaaa,\n"
               "         /* parameter 2 */ aaaaaa,\n"
               "         /* parameter 3 */ aaaaaa,\n"
               "         /* parameter 4 */ aaaaaa);",
               NoBinPacking);
}

TEST_F(FormatTest, AlignsBlockComments) {
  EXPECT_EQ("/*\n"
            " * Really multi-line\n"
            " * comment.\n"
            " */\n"
            "void f() {}",
            format("  /*\n"
                   "   * Really multi-line\n"
                   "   * comment.\n"
                   "   */\n"
                   "  void f() {}"));
  EXPECT_EQ("class C {\n"
            "  /*\n"
            "   * Another multi-line\n"
            "   * comment.\n"
            "   */\n"
            "  void f() {}\n"
            "};",
            format("class C {\n"
                   "/*\n"
                   " * Another multi-line\n"
                   " * comment.\n"
                   " */\n"
                   "void f() {}\n"
                   "};"));
  EXPECT_EQ("/*\n"
            "  1. This is a comment with non-trivial formatting.\n"
            "     1.1. We have to indent/outdent all lines equally\n"
            "         1.1.1. to keep the formatting.\n"
            " */",
            format("  /*\n"
                   "    1. This is a comment with non-trivial formatting.\n"
                   "       1.1. We have to indent/outdent all lines equally\n"
                   "           1.1.1. to keep the formatting.\n"
                   "   */"));
  EXPECT_EQ("/*\n"
            "Don't try to outdent if there's not enough indentation.\n"
            "*/",
            format("  /*\n"
                   " Don't try to outdent if there's not enough indentation.\n"
                   " */"));

  EXPECT_EQ("int i; /* Comment with empty...\n"
            "        *\n"
            "        * line. */",
            format("int i; /* Comment with empty...\n"
                   "        *\n"
                   "        * line. */"));
}

TEST_F(FormatTest, CorrectlyHandlesLengthOfBlockComments) {
  EXPECT_EQ("double *x; /* aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
            "              aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa */",
            format("double *x; /* aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
                   "              aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa */"));
  EXPECT_EQ(
      "void ffffffffffff(\n"
      "    int aaaaaaaa, int bbbbbbbb,\n"
      "    int cccccccccccc) { /*\n"
      "                           aaaaaaaaaa\n"
      "                           aaaaaaaaaaaaa\n"
      "                           bbbbbbbbbbbbbb\n"
      "                           bbbbbbbbbb\n"
      "                         */\n"
      "}",
      format("void ffffffffffff(int aaaaaaaa, int bbbbbbbb, int cccccccccccc)\n"
             "{ /*\n"
             "     aaaaaaaaaa aaaaaaaaaaaaa\n"
             "     bbbbbbbbbbbbbb bbbbbbbbbb\n"
             "   */\n"
             "}",
             getLLVMStyleWithColumns(40)));
}

TEST_F(FormatTest, DontBreakNonTrailingBlockComments) {
  EXPECT_EQ("void\n"
            "ffffffffff(int aaaaa /* test */);",
            format("void ffffffffff(int aaaaa /* test */);",
                   getLLVMStyleWithColumns(35)));
}

TEST_F(FormatTest, SplitsLongCxxComments) {
  EXPECT_EQ("// A comment that\n"
            "// doesn't fit on\n"
            "// one line",
            format("// A comment that doesn't fit on one line",
                   getLLVMStyleWithColumns(20)));
  EXPECT_EQ("// a b c d\n"
            "// e f  g\n"
            "// h i j k",
            format("// a b c d e f  g h i j k",
                   getLLVMStyleWithColumns(10)));
  EXPECT_EQ("// a b c d\n"
            "// e f  g\n"
            "// h i j k",
            format("\\\n// a b c d e f  g h i j k",
                   getLLVMStyleWithColumns(10)));
  EXPECT_EQ("if (true) // A comment that\n"
            "          // doesn't fit on\n"
            "          // one line",
            format("if (true) // A comment that doesn't fit on one line   ",
                   getLLVMStyleWithColumns(30)));
  EXPECT_EQ("//    Don't_touch_leading_whitespace",
            format("//    Don't_touch_leading_whitespace",
                   getLLVMStyleWithColumns(20)));
  EXPECT_EQ("// Add leading\n"
            "// whitespace",
            format("//Add leading whitespace", getLLVMStyleWithColumns(20)));
  EXPECT_EQ("// whitespace", format("//whitespace", getLLVMStyle()));
  EXPECT_EQ("// Even if it makes the line exceed the column\n"
            "// limit",
            format("//Even if it makes the line exceed the column limit",
                   getLLVMStyleWithColumns(51)));
  EXPECT_EQ("//--But not here", format("//--But not here", getLLVMStyle()));
  EXPECT_EQ("// A comment before\n"
            "// a macro\n"
            "// definition\n"
            "#define a b",
            format("// A comment before a macro definition\n"
                   "#define a b",
                   getLLVMStyleWithColumns(20)));
  EXPECT_EQ("void ffffff(int aaaaaaaaa,  // wwww\n"
            "            int a, int bbb, // xxxxxxx\n"
            "                            // yyyyyyyyy\n"
            "            int c, int d, int e) {}",
            format("void ffffff(\n"
                   "    int aaaaaaaaa, // wwww\n"
                   "    int a,\n"
                   "    int bbb, // xxxxxxx yyyyyyyyy\n"
                   "    int c, int d, int e) {}",
                   getLLVMStyleWithColumns(40)));
  EXPECT_EQ("//\t aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
            format("//\t aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
                   getLLVMStyleWithColumns(20)));
}

TEST_F(FormatTest, DontSplitLineCommentsWithEscapedNewlines) {
  EXPECT_EQ("// aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\\\n"
            "// aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\\\n"
            "// aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
            format("// aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\\\n"
                   "// aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\\\n"
                   "// aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
  EXPECT_EQ("int a; // AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\\\n"
            "       // AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\\\n"
            "       // AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
            format("int a; // AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\\\n"
                   "       // AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\\\n"
                   "       // AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
                   getLLVMStyleWithColumns(50)));
  // FIXME: One day we might want to implement adjustment of leading whitespace
  // of the consecutive lines in this kind of comment:
  EXPECT_EQ("int\n"
            "a; // AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\\\n"
            "       // AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\\\n"
            "       // AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
            format("int a; // AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\\\n"
                   "       // AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\\\n"
                   "       // AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
                   getLLVMStyleWithColumns(49)));
}

TEST_F(FormatTest, PriorityOfCommentBreaking) {
  EXPECT_EQ("if (xxx == yyy && // aaaaaaaaaaaa\n"
            "                  // bbbbbbbbb\n"
            "    zzz)\n"
            "  q();",
            format("if (xxx == yyy && // aaaaaaaaaaaa bbbbbbbbb\n"
                   "    zzz) q();",
                   getLLVMStyleWithColumns(40)));
  EXPECT_EQ("if (xxxxxxxxxx ==\n"
            "        yyy && // aaaaaa bbbbbbbb cccc\n"
            "    zzz)\n"
            "  q();",
            format("if (xxxxxxxxxx == yyy && // aaaaaa bbbbbbbb cccc\n"
                   "    zzz) q();",
                   getLLVMStyleWithColumns(40)));
  EXPECT_EQ("if (xxxxxxxxxx &&\n"
            "        yyy || // aaaaaa bbbbbbbb cccc\n"
            "    zzz)\n"
            "  q();",
            format("if (xxxxxxxxxx && yyy || // aaaaaa bbbbbbbb cccc\n"
                   "    zzz) q();",
                   getLLVMStyleWithColumns(40)));
  EXPECT_EQ("fffffffff(&xxx, // aaaaaaaaaaaa\n"
            "                // bbbbbbbbbbb\n"
            "          zzz);",
            format("fffffffff(&xxx, // aaaaaaaaaaaa bbbbbbbbbbb\n"
                   " zzz);",
                   getLLVMStyleWithColumns(40)));
}

TEST_F(FormatTest, MultiLineCommentsInDefines) {
  EXPECT_EQ("#define A(x) /* \\\n"
            "  a comment     \\\n"
            "  inside */     \\\n"
            "  f();",
            format("#define A(x) /* \\\n"
                   "  a comment     \\\n"
                   "  inside */     \\\n"
                   "  f();",
                   getLLVMStyleWithColumns(17)));
  EXPECT_EQ("#define A(      \\\n"
            "    x) /*       \\\n"
            "  a comment     \\\n"
            "  inside */     \\\n"
            "  f();",
            format("#define A(      \\\n"
                   "    x) /*       \\\n"
                   "  a comment     \\\n"
                   "  inside */     \\\n"
                   "  f();",
                   getLLVMStyleWithColumns(17)));
}

TEST_F(FormatTest, ParsesCommentsAdjacentToPPDirectives) {
  EXPECT_EQ("namespace {}\n// Test\n#define A",
            format("namespace {}\n   // Test\n#define A"));
  EXPECT_EQ("namespace {}\n/* Test */\n#define A",
            format("namespace {}\n   /* Test */\n#define A"));
  EXPECT_EQ("namespace {}\n/* Test */ #define A",
            format("namespace {}\n   /* Test */    #define A"));
}

TEST_F(FormatTest, SplitsLongLinesInComments) {
  EXPECT_EQ("/* This is a long\n"
            " * comment that\n"
            " * doesn't\n"
            " * fit on one line.\n"
            " */",
            format("/* "
                   "This is a long                                         "
                   "comment that "
                   "doesn't                                    "
                   "fit on one line.  */",
                   getLLVMStyleWithColumns(20)));
  EXPECT_EQ("/* a b c d\n"
            " * e f  g\n"
            " * h i j k\n"
            " */",
            format("/* a b c d e f  g h i j k */",
                   getLLVMStyleWithColumns(10)));
  EXPECT_EQ("/* a b c d\n"
            " * e f  g\n"
            " * h i j k\n"
            " */",
            format("\\\n/* a b c d e f  g h i j k */",
                   getLLVMStyleWithColumns(10)));
  EXPECT_EQ("/*\n"
            "This is a long\n"
            "comment that doesn't\n"
            "fit on one line.\n"
            "*/",
            format("/*\n"
                   "This is a long                                         "
                   "comment that doesn't                                    "
                   "fit on one line.                                      \n"
                   "*/", getLLVMStyleWithColumns(20)));
  EXPECT_EQ("/*\n"
            " * This is a long\n"
            " * comment that\n"
            " * doesn't fit on\n"
            " * one line.\n"
            " */",
            format("/*      \n"
                   " * This is a long "
                   "   comment that     "
                   "   doesn't fit on   "
                   "   one line.                                            \n"
                   " */", getLLVMStyleWithColumns(20)));
  EXPECT_EQ("/*\n"
            " * This_is_a_comment_with_words_that_dont_fit_on_one_line\n"
            " * so_it_should_be_broken\n"
            " * wherever_a_space_occurs\n"
            " */",
            format("/*\n"
                   " * This_is_a_comment_with_words_that_dont_fit_on_one_line "
                   "   so_it_should_be_broken "
                   "   wherever_a_space_occurs                             \n"
                   " */",
                   getLLVMStyleWithColumns(20)));
  EXPECT_EQ("/*\n"
            " *    This_comment_can_not_be_broken_into_lines\n"
            " */",
            format("/*\n"
                   " *    This_comment_can_not_be_broken_into_lines\n"
                   " */",
                   getLLVMStyleWithColumns(20)));
  EXPECT_EQ("{\n"
            "  /*\n"
            "  This is another\n"
            "  long comment that\n"
            "  doesn't fit on one\n"
            "  line    1234567890\n"
            "  */\n"
            "}",
            format("{\n"
                   "/*\n"
                   "This is another     "
                   "  long comment that "
                   "  doesn't fit on one"
                   "  line    1234567890\n"
                   "*/\n"
                   "}", getLLVMStyleWithColumns(20)));
  EXPECT_EQ("{\n"
            "  /*\n"
            "   * This        i s\n"
            "   * another comment\n"
            "   * t hat  doesn' t\n"
            "   * fit on one l i\n"
            "   * n e\n"
            "   */\n"
            "}",
            format("{\n"
                   "/*\n"
                   " * This        i s"
                   "   another comment"
                   "   t hat  doesn' t"
                   "   fit on one l i"
                   "   n e\n"
                   " */\n"
                   "}", getLLVMStyleWithColumns(20)));
  EXPECT_EQ("/*\n"
            " * This is a long\n"
            " * comment that\n"
            " * doesn't fit on\n"
            " * one line\n"
            " */",
            format("   /*\n"
                   "    * This is a long comment that doesn't fit on one line\n"
                   "    */", getLLVMStyleWithColumns(20)));
  EXPECT_EQ("{\n"
            "  if (something) /* This is a\n"
            "                    long\n"
            "                    comment */\n"
            "    ;\n"
            "}",
            format("{\n"
                   "  if (something) /* This is a long comment */\n"
                   "    ;\n"
                   "}",
                   getLLVMStyleWithColumns(30)));

  EXPECT_EQ("/* A comment before\n"
            " * a macro\n"
            " * definition */\n"
            "#define a b",
            format("/* A comment before a macro definition */\n"
                   "#define a b",
                   getLLVMStyleWithColumns(20)));

  EXPECT_EQ("/* some comment\n"
            "     *   a comment\n"
            "* that we break\n"
            " * another comment\n"
            "* we have to break\n"
            "* a left comment\n"
            " */",
            format("  /* some comment\n"
                   "       *   a comment that we break\n"
                   "   * another comment we have to break\n"
                   "* a left comment\n"
                   "   */",
                   getLLVMStyleWithColumns(20)));

  EXPECT_EQ("/*\n"
            "\n"
            "\n"
            "    */\n",
            format("  /*       \n"
                   "      \n"
                   "               \n"
                   "      */\n"));
}

TEST_F(FormatTest, SplitsLongLinesInCommentsInPreprocessor) {
  EXPECT_EQ("#define X          \\\n"
            "  /*               \\\n"
            "   Test            \\\n"
            "   Macro comment   \\\n"
            "   with a long     \\\n"
            "   line            \\\n"
            "   */              \\\n"
            "  A + B",
            format("#define X \\\n"
                   "  /*\n"
                   "   Test\n"
                   "   Macro comment with a long  line\n"
                   "   */ \\\n"
                   "  A + B",
                   getLLVMStyleWithColumns(20)));
  EXPECT_EQ("#define X          \\\n"
            "  /* Macro comment \\\n"
            "     with a long   \\\n"
            "     line */       \\\n"
            "  A + B",
            format("#define X \\\n"
                   "  /* Macro comment with a long\n"
                   "     line */ \\\n"
                   "  A + B",
                   getLLVMStyleWithColumns(20)));
  EXPECT_EQ("#define X          \\\n"
            "  /* Macro comment \\\n"
            "   * with a long   \\\n"
            "   * line */       \\\n"
            "  A + B",
            format("#define X \\\n"
                   "  /* Macro comment with a long  line */ \\\n"
                   "  A + B",
                   getLLVMStyleWithColumns(20)));
}

TEST_F(FormatTest, CommentsInStaticInitializers) {
  EXPECT_EQ(
      "static SomeType type = { aaaaaaaaaaaaaaaaaaaa, /* comment */\n"
      "                         aaaaaaaaaaaaaaaaaaaa /* comment */,\n"
      "                         /* comment */ aaaaaaaaaaaaaaaaaaaa,\n"
      "                         aaaaaaaaaaaaaaaaaaaa, // comment\n"
      "                         aaaaaaaaaaaaaaaaaaaa };",
      format("static SomeType type = { aaaaaaaaaaaaaaaaaaaa  ,  /* comment */\n"
             "                   aaaaaaaaaaaaaaaaaaaa   /* comment */ ,\n"
             "                     /* comment */   aaaaaaaaaaaaaaaaaaaa ,\n"
             "              aaaaaaaaaaaaaaaaaaaa ,   // comment\n"
             "                  aaaaaaaaaaaaaaaaaaaa };"));
  verifyFormat("static SomeType type = { aaaaaaaaaaa, // comment for aa...\n"
               "                         bbbbbbbbbbb, ccccccccccc };");
  verifyFormat("static SomeType type = { aaaaaaaaaaa,\n"
               "                         // comment for bb....\n"
               "                         bbbbbbbbbbb, ccccccccccc };");
  verifyGoogleFormat(
      "static SomeType type = {aaaaaaaaaaa,  // comment for aa...\n"
      "                        bbbbbbbbbbb, ccccccccccc};");
  verifyGoogleFormat("static SomeType type = {aaaaaaaaaaa,\n"
                     "                        // comment for bb....\n"
                     "                        bbbbbbbbbbb, ccccccccccc};");

  verifyFormat("S s = { { a, b, c },   // Group #1\n"
               "        { d, e, f },   // Group #2\n"
               "        { g, h, i } }; // Group #3");
  verifyFormat("S s = { { // Group #1\n"
               "          a, b, c },\n"
               "        { // Group #2\n"
               "          d, e, f },\n"
               "        { // Group #3\n"
               "          g, h, i } };");

  EXPECT_EQ("S s = {\n"
            "  // Some comment\n"
            "  a,\n"
            "\n"
            "  // Comment after empty line\n"
            "  b\n"
            "}",
            format("S s =    {\n"
                   "      // Some comment\n"
                   "  a,\n"
                   "  \n"
                   "     // Comment after empty line\n"
                   "      b\n"
                   "}"));
  EXPECT_EQ("S s = {\n"
            "  /* Some comment */\n"
            "  a,\n"
            "\n"
            "  /* Comment after empty line */\n"
            "  b\n"
            "}",
            format("S s =    {\n"
                   "      /* Some comment */\n"
                   "  a,\n"
                   "  \n"
                   "     /* Comment after empty line */\n"
                   "      b\n"
                   "}"));
  verifyFormat("const uint8_t aaaaaaaaaaaaaaaaaaaaaa[0] = {\n"
               "  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // comment\n"
               "  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // comment\n"
               "  0x00, 0x00, 0x00, 0x00              // comment\n"
               "};");
}

TEST_F(FormatTest, IgnoresIf0Contents) {
  EXPECT_EQ("#if 0\n"
            "}{)(&*(^%%#%@! fsadj f;ldjs ,:;| <<<>>>][)(][\n"
            "#endif\n"
            "void f() {}",
            format("#if 0\n"
                   "}{)(&*(^%%#%@! fsadj f;ldjs ,:;| <<<>>>][)(][\n"
                   "#endif\n"
                   "void f(  ) {  }"));
  EXPECT_EQ("#if false\n"
            "void f(  ) {  }\n"
            "#endif\n"
            "void g() {}\n",
            format("#if false\n"
                   "void f(  ) {  }\n"
                   "#endif\n"
                   "void g(  ) {  }\n"));
  EXPECT_EQ("enum E {\n"
            "  One,\n"
            "  Two,\n"
            "#if 0\n"
            "Three,\n"
            "      Four,\n"
            "#endif\n"
            "  Five\n"
            "};",
            format("enum E {\n"
                   "  One,Two,\n"
                   "#if 0\n"
                   "Three,\n"
                   "      Four,\n"
                   "#endif\n"
                   "  Five};"));
  EXPECT_EQ("enum F {\n"
            "  One,\n"
            "#if 1\n"
            "  Two,\n"
            "#if 0\n"
            "Three,\n"
            "      Four,\n"
            "#endif\n"
            "  Five\n"
            "#endif\n"
            "};",
            format("enum F {\n"
                   "One,\n"
                   "#if 1\n"
                   "Two,\n"
                   "#if 0\n"
                   "Three,\n"
                   "      Four,\n"
                   "#endif\n"
                   "Five\n"
                   "#endif\n"
                   "};"));
  EXPECT_EQ("enum G {\n"
            "  One,\n"
            "#if 0\n"
            "Two,\n"
            "#else\n"
            "  Three,\n"
            "#endif\n"
            "  Four\n"
            "};",
            format("enum G {\n"
                   "One,\n"
                   "#if 0\n"
                   "Two,\n"
                   "#else\n"
                   "Three,\n"
                   "#endif\n"
                   "Four\n"
                   "};"));
  EXPECT_EQ("enum H {\n"
            "  One,\n"
            "#if 0\n"
            "#ifdef Q\n"
            "Two,\n"
            "#else\n"
            "Three,\n"
            "#endif\n"
            "#endif\n"
            "  Four\n"
            "};",
            format("enum H {\n"
                   "One,\n"
                   "#if 0\n"
                   "#ifdef Q\n"
                   "Two,\n"
                   "#else\n"
                   "Three,\n"
                   "#endif\n"
                   "#endif\n"
                   "Four\n"
                   "};"));
  EXPECT_EQ("enum I {\n"
            "  One,\n"
            "#if /* test */ 0 || 1\n"
            "Two,\n"
            "Three,\n"
            "#endif\n"
            "  Four\n"
            "};",
            format("enum I {\n"
                   "One,\n"
                   "#if /* test */ 0 || 1\n"
                   "Two,\n"
                   "Three,\n"
                   "#endif\n"
                   "Four\n"
                   "};"));
  EXPECT_EQ("enum J {\n"
            "  One,\n"
            "#if 0\n"
            "#if 0\n"
            "Two,\n"
            "#else\n"
            "Three,\n"
            "#endif\n"
            "Four,\n"
            "#endif\n"
            "  Five\n"
            "};",
            format("enum J {\n"
                   "One,\n"
                   "#if 0\n"
                   "#if 0\n"
                   "Two,\n"
                   "#else\n"
                   "Three,\n"
                   "#endif\n"
                   "Four,\n"
                   "#endif\n"
                   "Five\n"
                   "};"));

}

//===----------------------------------------------------------------------===//
// Tests for classes, namespaces, etc.
//===----------------------------------------------------------------------===//

TEST_F(FormatTest, DoesNotBreakSemiAfterClassDecl) {
  verifyFormat("class A {};");
}

TEST_F(FormatTest, UnderstandsAccessSpecifiers) {
  verifyFormat("class A {\n"
               "public:\n"
               "public: // comment\n"
               "protected:\n"
               "private:\n"
               "  void f() {}\n"
               "};");
  verifyGoogleFormat("class A {\n"
                     " public:\n"
                     " protected:\n"
                     " private:\n"
                     "  void f() {}\n"
                     "};");
}

TEST_F(FormatTest, SeparatesLogicalBlocks) {
  EXPECT_EQ("class A {\n"
            "public:\n"
            "  void f();\n"
            "\n"
            "private:\n"
            "  void g() {}\n"
            "  // test\n"
            "protected:\n"
            "  int h;\n"
            "};",
            format("class A {\n"
                   "public:\n"
                   "void f();\n"
                   "private:\n"
                   "void g() {}\n"
                   "// test\n"
                   "protected:\n"
                   "int h;\n"
                   "};"));
}

TEST_F(FormatTest, FormatsClasses) {
  verifyFormat("class A : public B {};");
  verifyFormat("class A : public ::B {};");

  verifyFormat(
      "class AAAAAAAAAAAAAAAAAAAA : public BBBBBBBBBBBBBBBBBBBBBBBBBBBBBB,\n"
      "                             public CCCCCCCCCCCCCCCCCCCCCCCCCCCCCC {};");
  verifyFormat("class AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n"
               "    : public BBBBBBBBBBBBBBBBBBBBBBBBBBBBBB,\n"
               "      public CCCCCCCCCCCCCCCCCCCCCCCCCCCCCC {};");
  verifyFormat(
      "class A : public B, public C, public D, public E, public F {};");
  verifyFormat("class AAAAAAAAAAAA : public B,\n"
               "                     public C,\n"
               "                     public D,\n"
               "                     public E,\n"
               "                     public F,\n"
               "                     public G {};");

  verifyFormat("class\n"
               "    ReallyReallyLongClassName {\n};",
               getLLVMStyleWithColumns(32));
}

TEST_F(FormatTest, FormatsVariableDeclarationsAfterStructOrClass) {
  verifyFormat("class A {\n} a, b;");
  verifyFormat("struct A {\n} a, b;");
  verifyFormat("union A {\n} a;");
}

TEST_F(FormatTest, FormatsEnum) {
  verifyFormat("enum {\n"
               "  Zero,\n"
               "  One = 1,\n"
               "  Two = One + 1,\n"
               "  Three = (One + Two),\n"
               "  Four = (Zero && (One ^ Two)) | (One << Two),\n"
               "  Five = (One, Two, Three, Four, 5)\n"
               "};");
  verifyFormat("enum Enum {};");
  verifyFormat("enum {};");
  verifyFormat("enum X E {\n} d;");
  verifyFormat("enum __attribute__((...)) E {\n} d;");
  verifyFormat("enum __declspec__((...)) E {\n} d;");
  verifyFormat("enum X f() {\n  a();\n  return 42;\n}");
}

TEST_F(FormatTest, FormatsBitfields) {
  verifyFormat("struct Bitfields {\n"
               "  unsigned sClass : 8;\n"
               "  unsigned ValueKind : 2;\n"
               "};");
}

TEST_F(FormatTest, FormatsNamespaces) {
  verifyFormat("namespace some_namespace {\n"
               "class A {};\n"
               "void f() { f(); }\n"
               "}");
  verifyFormat("namespace {\n"
               "class A {};\n"
               "void f() { f(); }\n"
               "}");
  verifyFormat("inline namespace X {\n"
               "class A {};\n"
               "void f() { f(); }\n"
               "}");
  verifyFormat("using namespace some_namespace;\n"
               "class A {};\n"
               "void f() { f(); }");

  // This code is more common than we thought; if we
  // layout this correctly the semicolon will go into
  // its own line, which is undesireable.
  verifyFormat("namespace {};");
  verifyFormat("namespace {\n"
               "class A {};\n"
               "};");

  verifyFormat("namespace {\n"
               "int SomeVariable = 0; // comment\n"
               "} // namespace");
  EXPECT_EQ("#ifndef HEADER_GUARD\n"
            "#define HEADER_GUARD\n"
            "namespace my_namespace {\n"
            "int i;\n"
            "} // my_namespace\n"
            "#endif // HEADER_GUARD",
            format("#ifndef HEADER_GUARD\n"
                   " #define HEADER_GUARD\n"
                   "   namespace my_namespace {\n"
                   "int i;\n"
                   "}    // my_namespace\n"
                   "#endif    // HEADER_GUARD"));

  FormatStyle Style = getLLVMStyle();
  Style.NamespaceIndentation = FormatStyle::NI_All;
  EXPECT_EQ("namespace out {\n"
            "  int i;\n"
            "  namespace in {\n"
            "    int i;\n"
            "  } // namespace\n"
            "} // namespace",
            format("namespace out {\n"
                   "int i;\n"
                   "namespace in {\n"
                   "int i;\n"
                   "} // namespace\n"
                   "} // namespace",
                   Style));

  Style.NamespaceIndentation = FormatStyle::NI_Inner;
  EXPECT_EQ("namespace out {\n"
            "int i;\n"
            "namespace in {\n"
            "  int i;\n"
            "} // namespace\n"
            "} // namespace",
            format("namespace out {\n"
                   "int i;\n"
                   "namespace in {\n"
                   "int i;\n"
                   "} // namespace\n"
                   "} // namespace",
                   Style));
}

TEST_F(FormatTest, FormatsExternC) { verifyFormat("extern \"C\" {\nint a;"); }

TEST_F(FormatTest, FormatsInlineASM) {
  verifyFormat("asm(\"xyz\" : \"=a\"(a), \"=d\"(b) : \"a\"(data));");
  verifyFormat(
      "asm(\"movq\\t%%rbx, %%rsi\\n\\t\"\n"
      "    \"cpuid\\n\\t\"\n"
      "    \"xchgq\\t%%rbx, %%rsi\\n\\t\"\n"
      "    : \"=a\"(*rEAX), \"=S\"(*rEBX), \"=c\"(*rECX), \"=d\"(*rEDX)\n"
      "    : \"a\"(value));");
}

TEST_F(FormatTest, FormatTryCatch) {
  // FIXME: Handle try-catch explicitly in the UnwrappedLineParser, then we'll
  // also not create single-line-blocks.
  verifyFormat("try {\n"
               "  throw a * b;\n"
               "}\n"
               "catch (int a) {\n"
               "  // Do nothing.\n"
               "}\n"
               "catch (...) {\n"
               "  exit(42);\n"
               "}");

  // Function-level try statements.
  verifyFormat("int f() try { return 4; }\n"
               "catch (...) {\n"
               "  return 5;\n"
               "}");
  verifyFormat("class A {\n"
               "  int a;\n"
               "  A() try : a(0) {}\n"
               "  catch (...) {\n"
               "    throw;\n"
               "  }\n"
               "};\n");
}

TEST_F(FormatTest, FormatObjCTryCatch) {
  verifyFormat("@try {\n"
               "  f();\n"
               "}\n"
               "@catch (NSException e) {\n"
               "  @throw;\n"
               "}\n"
               "@finally {\n"
               "  exit(42);\n"
               "}");
}

TEST_F(FormatTest, StaticInitializers) {
  verifyFormat("static SomeClass SC = { 1, 'a' };");

  verifyFormat(
      "static SomeClass WithALoooooooooooooooooooongName = {\n"
      "  100000000, \"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\"\n"
      "};");

  verifyFormat(
      "static SomeClass = { a, b, c, d, e, f, g, h, i, j,\n"
      "                     looooooooooooooooooooooooooooooooooongname,\n"
      "                     looooooooooooooooooooooooooooooong };");
  // Allow bin-packing in static initializers as this would often lead to
  // terrible results, e.g.:
  verifyGoogleFormat(
      "static SomeClass = {a, b, c, d, e, f, g, h, i, j,\n"
      "                    looooooooooooooooooooooooooooooooooongname,\n"
      "                    looooooooooooooooooooooooooooooong};");
  // Here, everything other than the "}" would fit on a line.
  verifyFormat("static int LooooooooooooooooooooooooongVariable[1] = {\n"
               "  100000000000000000000000\n"
               "};");
  EXPECT_EQ("S s = { a, b };", format("S s = {\n"
                                      "  a,\n"
                                      "\n"
                                      "  b\n"
                                      "};"));

  // FIXME: This would fit into the column limit if we'd fit "{ {" on the first
  // line. However, the formatting looks a bit off and this probably doesn't
  // happen often in practice.
  verifyFormat("static int Variable[1] = {\n"
               "  { 1000000000000000000000000000000000000 }\n"
               "};",
               getLLVMStyleWithColumns(40));
}

TEST_F(FormatTest, DesignatedInitializers) {
  verifyFormat("const struct A a = { .a = 1, .b = 2 };");
  verifyFormat("const struct A a = { .aaaaaaaaaa = 1,\n"
               "                     .bbbbbbbbbb = 2,\n"
               "                     .cccccccccc = 3,\n"
               "                     .dddddddddd = 4,\n"
               "                     .eeeeeeeeee = 5 };");
  verifyFormat("const struct Aaaaaaaaaaaaaaaa aaaaaaaaaaaaaaaaaa = {\n"
               "  .aaaaaaaaaaaaaaaaaaaaaaaaaaa = 1,\n"
               "  .bbbbbbbbbbbbbbbbbbbbbbbbbbb = 2,\n"
               "  .ccccccccccccccccccccccccccc = 3,\n"
               "  .ddddddddddddddddddddddddddd = 4,\n"
               "  .eeeeeeeeeeeeeeeeeeeeeeeeeee = 5\n"
               "};");

  verifyGoogleFormat("const struct A a = {.a = 1, .b = 2};");
}

TEST_F(FormatTest, NestedStaticInitializers) {
  verifyFormat("static A x = { { {} } };\n");
  verifyFormat("static A x = { { { init1, init2, init3, init4 },\n"
               "                 { init1, init2, init3, init4 } } };");

  verifyFormat("somes Status::global_reps[3] = {\n"
               "  { kGlobalRef, OK_CODE, NULL, NULL, NULL },\n"
               "  { kGlobalRef, CANCELLED_CODE, NULL, NULL, NULL },\n"
               "  { kGlobalRef, UNKNOWN_CODE, NULL, NULL, NULL }\n"
               "};");
  verifyGoogleFormat("SomeType Status::global_reps[3] = {\n"
                     "    {kGlobalRef, OK_CODE, NULL, NULL, NULL},\n"
                     "    {kGlobalRef, CANCELLED_CODE, NULL, NULL, NULL},\n"
                     "    {kGlobalRef, UNKNOWN_CODE, NULL, NULL, NULL}};");
  verifyFormat(
      "CGRect cg_rect = { { rect.fLeft, rect.fTop },\n"
      "                   { rect.fRight - rect.fLeft, rect.fBottom - rect.fTop"
      " } };");

  verifyFormat(
      "SomeArrayOfSomeType a = { { { 1, 2, 3 }, { 1, 2, 3 },\n"
      "                            { 111111111111111111111111111111,\n"
      "                              222222222222222222222222222222,\n"
      "                              333333333333333333333333333333 },\n"
      "                            { 1, 2, 3 }, { 1, 2, 3 } } };");
  verifyFormat(
      "SomeArrayOfSomeType a = { { { 1, 2, 3 } }, { { 1, 2, 3 } },\n"
      "                          { { 111111111111111111111111111111,\n"
      "                              222222222222222222222222222222,\n"
      "                              333333333333333333333333333333 } },\n"
      "                          { { 1, 2, 3 } }, { { 1, 2, 3 } } };");
  verifyGoogleFormat(
      "SomeArrayOfSomeType a = {\n"
      "    {{1, 2, 3}}, {{1, 2, 3}},\n"
      "    {{111111111111111111111111111111, 222222222222222222222222222222,\n"
      "      333333333333333333333333333333}},\n"
      "    {{1, 2, 3}}, {{1, 2, 3}}};");

  // FIXME: We might at some point want to handle this similar to parameter
  // lists, where we have an option to put each on a single line.
  verifyFormat(
      "struct {\n"
      "  unsigned bit;\n"
      "  const char *const name;\n"
      "} kBitsToOs[] = { { kOsMac, \"Mac\" }, { kOsWin, \"Windows\" },\n"
      "                  { kOsLinux, \"Linux\" }, { kOsCrOS, \"Chrome OS\" } };");
}

TEST_F(FormatTest, FormatsSmallMacroDefinitionsInSingleLine) {
  verifyFormat("#define ALooooooooooooooooooooooooooooooooooooooongMacro("
               "                      \\\n"
               "    aLoooooooooooooooooooooooongFuuuuuuuuuuuuuunctiooooooooo)");
}

TEST_F(FormatTest, DoesNotBreakPureVirtualFunctionDefinition) {
  verifyFormat(
      "virtual void write(ELFWriter *writerrr,\n"
      "                   OwningPtr<FileOutputBuffer> &buffer) = 0;");
}

TEST_F(FormatTest, LayoutUnknownPPDirective) {
  EXPECT_EQ("#123 \"A string literal\"",
            format("   #     123    \"A string literal\""));
  EXPECT_EQ("#;", format("#;"));
  verifyFormat("#\n;\n;\n;");
}

TEST_F(FormatTest, UnescapedEndOfLineEndsPPDirective) {
  EXPECT_EQ("#line 42 \"test\"\n",
            format("#  \\\n  line  \\\n  42  \\\n  \"test\"\n"));
  EXPECT_EQ("#define A B\n", format("#  \\\n define  \\\n    A  \\\n       B\n",
                                    getLLVMStyleWithColumns(12)));
}

TEST_F(FormatTest, EndOfFileEndsPPDirective) {
  EXPECT_EQ("#line 42 \"test\"",
            format("#  \\\n  line  \\\n  42  \\\n  \"test\""));
  EXPECT_EQ("#define A B", format("#  \\\n define  \\\n    A  \\\n       B"));
}

TEST_F(FormatTest, IndentsPPDirectiveInReducedSpace) {
  verifyFormat("#define A(BB)", getLLVMStyleWithColumns(13));
  verifyFormat("#define A( \\\n    BB)", getLLVMStyleWithColumns(12));
  verifyFormat("#define A( \\\n    A, B)", getLLVMStyleWithColumns(12));
  // FIXME: We never break before the macro name.
  verifyFormat("#define AA(\\\n    B)", getLLVMStyleWithColumns(12));

  verifyFormat("#define A A\n#define A A");
  verifyFormat("#define A(X) A\n#define A A");

  verifyFormat("#define Something Other", getLLVMStyleWithColumns(23));
  verifyFormat("#define Something    \\\n  Other", getLLVMStyleWithColumns(22));
}

TEST_F(FormatTest, HandlePreprocessorDirectiveContext) {
  EXPECT_EQ("// somecomment\n"
            "#include \"a.h\"\n"
            "#define A(  \\\n"
            "    A, B)\n"
            "#include \"b.h\"\n"
            "// somecomment\n",
            format("  // somecomment\n"
                   "  #include \"a.h\"\n"
                   "#define A(A,\\\n"
                   "    B)\n"
                   "    #include \"b.h\"\n"
                   " // somecomment\n",
                   getLLVMStyleWithColumns(13)));
}

TEST_F(FormatTest, LayoutSingleHash) { EXPECT_EQ("#\na;", format("#\na;")); }

TEST_F(FormatTest, LayoutCodeInMacroDefinitions) {
  EXPECT_EQ("#define A    \\\n"
            "  c;         \\\n"
            "  e;\n"
            "f;",
            format("#define A c; e;\n"
                   "f;",
                   getLLVMStyleWithColumns(14)));
}

TEST_F(FormatTest, LayoutRemainingTokens) { EXPECT_EQ("{}", format("{}")); }

TEST_F(FormatTest, AlwaysFormatsEntireMacroDefinitions) {
  EXPECT_EQ("int  i;\n"
            "#define A \\\n"
            "  int i;  \\\n"
            "  int j\n"
            "int  k;",
            format("int  i;\n"
                   "#define A  \\\n"
                   " int   i    ;  \\\n"
                   " int   j\n"
                   "int  k;",
                   8, 0, getGoogleStyle())); // 8: position of "#define".
  EXPECT_EQ("int  i;\n"
            "#define A \\\n"
            "  int i;  \\\n"
            "  int j\n"
            "int  k;",
            format("int  i;\n"
                   "#define A  \\\n"
                   " int   i    ;  \\\n"
                   " int   j\n"
                   "int  k;",
                   45, 0, getGoogleStyle())); // 45: position of "j".
}

TEST_F(FormatTest, MacroDefinitionInsideStatement) {
  EXPECT_EQ("int x,\n"
            "#define A\n"
            "    y;",
            format("int x,\n#define A\ny;"));
}

TEST_F(FormatTest, HashInMacroDefinition) {
  verifyFormat("#define A \\\n  b #c;", getLLVMStyleWithColumns(11));
  verifyFormat("#define A \\\n"
               "  {       \\\n"
               "    f(#c);\\\n"
               "  }",
               getLLVMStyleWithColumns(11));

  verifyFormat("#define A(X)         \\\n"
               "  void function##X()",
               getLLVMStyleWithColumns(22));

  verifyFormat("#define A(a, b, c)   \\\n"
               "  void a##b##c()",
               getLLVMStyleWithColumns(22));

  verifyFormat("#define A void # ## #", getLLVMStyleWithColumns(22));
}

TEST_F(FormatTest, RespectWhitespaceInMacroDefinitions) {
  verifyFormat("#define A (1)");
}

TEST_F(FormatTest, EmptyLinesInMacroDefinitions) {
  EXPECT_EQ("#define A b;", format("#define A \\\n"
                                   "          \\\n"
                                   "  b;",
                                   getLLVMStyleWithColumns(25)));
  EXPECT_EQ("#define A \\\n"
            "          \\\n"
            "  a;      \\\n"
            "  b;",
            format("#define A \\\n"
                   "          \\\n"
                   "  a;      \\\n"
                   "  b;",
                   getLLVMStyleWithColumns(11)));
  EXPECT_EQ("#define A \\\n"
            "  a;      \\\n"
            "          \\\n"
            "  b;",
            format("#define A \\\n"
                   "  a;      \\\n"
                   "          \\\n"
                   "  b;",
                   getLLVMStyleWithColumns(11)));
}

TEST_F(FormatTest, MacroDefinitionsWithIncompleteCode) {
  verifyFormat("#define A :");
  verifyFormat("#define SOMECASES  \\\n"
               "  case 1:          \\\n"
               "  case 2\n",
               getLLVMStyleWithColumns(20));
  verifyFormat("#define A template <typename T>");
  verifyFormat("#define STR(x) #x\n"
               "f(STR(this_is_a_string_literal{));");
}

TEST_F(FormatTest, MacrosWithoutTrailingSemicolon) {
  verifyFormat("SOME_TYPE_NAME abc;"); // Gated on the newline.
  EXPECT_EQ("class A : public QObject {\n"
            "  Q_OBJECT\n"
            "\n"
            "  A() {}\n"
            "};",
            format("class A  :  public QObject {\n"
                   "     Q_OBJECT\n"
                   "\n"
                   "  A() {\n}\n"
                   "}  ;"));
  EXPECT_EQ("SOME_MACRO\n"
            "namespace {\n"
            "void f();\n"
            "}",
            format("SOME_MACRO\n"
                   "  namespace    {\n"
                   "void   f(  );\n"
                   "}"));
  // Only if the identifier contains at least 5 characters.
  EXPECT_EQ("HTTP f();",
            format("HTTP\nf();"));
  EXPECT_EQ("MACRO\nf();",
            format("MACRO\nf();"));
  // Only if everything is upper case.
  EXPECT_EQ("class A : public QObject {\n"
            "  Q_Object A() {}\n"
            "};",
            format("class A  :  public QObject {\n"
                   "     Q_Object\n"
                   "\n"
                   "  A() {\n}\n"
                   "}  ;"));
}

TEST_F(FormatTest, MacroCallsWithoutTrailingSemicolon) {
  EXPECT_EQ("INITIALIZE_PASS_BEGIN(ScopDetection, \"polly-detect\")\n"
            "INITIALIZE_AG_DEPENDENCY(AliasAnalysis)\n"
            "INITIALIZE_PASS_DEPENDENCY(DominatorTree)\n"
            "class X {};\n"
            "INITIALIZE_PASS_END(ScopDetection, \"polly-detect\")\n"
            "int *createScopDetectionPass() { return 0; }",
            format("  INITIALIZE_PASS_BEGIN(ScopDetection, \"polly-detect\")\n"
                   "  INITIALIZE_AG_DEPENDENCY(AliasAnalysis)\n"
                   "  INITIALIZE_PASS_DEPENDENCY(DominatorTree)\n"
                   "  class X {};\n"
                   "  INITIALIZE_PASS_END(ScopDetection, \"polly-detect\")\n"
                   "  int *createScopDetectionPass() { return 0; }"));
  // FIXME: We could probably treat IPC_BEGIN_MESSAGE_MAP/IPC_END_MESSAGE_MAP as
  // braces, so that inner block is indented one level more.
  EXPECT_EQ("int q() {\n"
            "  IPC_BEGIN_MESSAGE_MAP(WebKitTestController, message)\n"
            "  IPC_MESSAGE_HANDLER(xxx, qqq)\n"
            "  IPC_END_MESSAGE_MAP()\n"
            "}",
            format("int q() {\n"
                   "  IPC_BEGIN_MESSAGE_MAP(WebKitTestController, message)\n"
                   "    IPC_MESSAGE_HANDLER(xxx, qqq)\n"
                   "  IPC_END_MESSAGE_MAP()\n"
                   "}"));
  EXPECT_EQ("int q() {\n"
            "  f(x);\n"
            "  f(x) {}\n"
            "  f(x)->g();\n"
            "  f(x)->*g();\n"
            "  f(x).g();\n"
            "  f(x) = x;\n"
            "  f(x) += x;\n"
            "  f(x) -= x;\n"
            "  f(x) *= x;\n"
            "  f(x) /= x;\n"
            "  f(x) %= x;\n"
            "  f(x) &= x;\n"
            "  f(x) |= x;\n"
            "  f(x) ^= x;\n"
            "  f(x) >>= x;\n"
            "  f(x) <<= x;\n"
            "  f(x)[y].z();\n"
            "  LOG(INFO) << x;\n"
            "  ifstream(x) >> x;\n"
            "}\n",
            format("int q() {\n"
                   "  f(x)\n;\n"
                   "  f(x)\n {}\n"
                   "  f(x)\n->g();\n"
                   "  f(x)\n->*g();\n"
                   "  f(x)\n.g();\n"
                   "  f(x)\n = x;\n"
                   "  f(x)\n += x;\n"
                   "  f(x)\n -= x;\n"
                   "  f(x)\n *= x;\n"
                   "  f(x)\n /= x;\n"
                   "  f(x)\n %= x;\n"
                   "  f(x)\n &= x;\n"
                   "  f(x)\n |= x;\n"
                   "  f(x)\n ^= x;\n"
                   "  f(x)\n >>= x;\n"
                   "  f(x)\n <<= x;\n"
                   "  f(x)\n[y].z();\n"
                   "  LOG(INFO)\n << x;\n"
                   "  ifstream(x)\n >> x;\n"
                   "}\n"));
  EXPECT_EQ("int q() {\n"
            "  f(x)\n"
            "  if (1) {\n"
            "  }\n"
            "  f(x)\n"
            "  while (1) {\n"
            "  }\n"
            "  f(x)\n"
            "  g(x);\n"
            "  f(x)\n"
            "  try {\n"
            "    q();\n"
            "  }\n"
            "  catch (...) {\n"
            "  }\n"
            "}\n",
            format("int q() {\n"
                   "f(x)\n"
                   "if (1) {}\n"
                   "f(x)\n"
                   "while (1) {}\n"
                   "f(x)\n"
                   "g(x);\n"
                   "f(x)\n"
                   "try { q(); } catch (...) {}\n"
                   "}\n"));
  EXPECT_EQ("class A {\n"
            "  A() : t(0) {}\n"
            "  A(X x)\n" // FIXME: function-level try blocks are broken.
            "  try : t(0) {\n"
            "  }\n"
            "  catch (...) {\n"
            "  }\n"
            "};",
            format("class A {\n"
                   "  A()\n : t(0) {}\n"
                   "  A(X x)\n"
                   "  try : t(0) {} catch (...) {}\n"
                   "};"));
}

TEST_F(FormatTest, LayoutMacroDefinitionsStatementsSpanningBlocks) {
  verifyFormat("#define A \\\n"
               "  f({     \\\n"
               "    g();  \\\n"
               "  });", getLLVMStyleWithColumns(11));
}

TEST_F(FormatTest, IndentPreprocessorDirectivesAtZero) {
  EXPECT_EQ("{\n  {\n#define A\n  }\n}", format("{{\n#define A\n}}"));
}

TEST_F(FormatTest, FormatHashIfNotAtStartOfLine) {
  verifyFormat("{\n  { a #c; }\n}");
}

TEST_F(FormatTest, FormatUnbalancedStructuralElements) {
  EXPECT_EQ("#define A \\\n  {       \\\n    {\nint i;",
            format("#define A { {\nint i;", getLLVMStyleWithColumns(11)));
  EXPECT_EQ("#define A \\\n  }       \\\n  }\nint i;",
            format("#define A } }\nint i;", getLLVMStyleWithColumns(11)));
}

TEST_F(FormatTest, EscapedNewlineAtStartOfToken) {
  EXPECT_EQ(
      "#define A \\\n  int i;  \\\n  int j;",
      format("#define A \\\nint i;\\\n  int j;", getLLVMStyleWithColumns(11)));
  EXPECT_EQ("template <class T> f();", format("\\\ntemplate <class T> f();"));
}

TEST_F(FormatTest, NoEscapedNewlineHandlingInBlockComments) {
  EXPECT_EQ("/* \\  \\  \\\n*/", format("\\\n/* \\  \\  \\\n*/"));
}

TEST_F(FormatTest, CalculateSpaceOnConsecutiveLinesInMacro) {
  verifyFormat("#define A \\\n"
               "  int v(  \\\n"
               "      a); \\\n"
               "  int i;",
               getLLVMStyleWithColumns(11));
}

TEST_F(FormatTest, MixingPreprocessorDirectivesAndNormalCode) {
  EXPECT_EQ(
      "#define ALooooooooooooooooooooooooooooooooooooooongMacro("
      "                      \\\n"
      "    aLoooooooooooooooooooooooongFuuuuuuuuuuuuuunctiooooooooo)\n"
      "\n"
      "AlooooooooooooooooooooooooooooooooooooooongCaaaaaaaaaal(\n"
      "    aLooooooooooooooooooooooonPaaaaaaaaaaaaaaaaaaaaarmmmm);\n",
      format("  #define   ALooooooooooooooooooooooooooooooooooooooongMacro("
             "\\\n"
             "aLoooooooooooooooooooooooongFuuuuuuuuuuuuuunctiooooooooo)\n"
             "  \n"
             "   AlooooooooooooooooooooooooooooooooooooooongCaaaaaaaaaal(\n"
             "  aLooooooooooooooooooooooonPaaaaaaaaaaaaaaaaaaaaarmmmm);\n"));
}

TEST_F(FormatTest, LayoutStatementsAroundPreprocessorDirectives) {
  EXPECT_EQ("int\n"
            "#define A\n"
            "    a;",
            format("int\n#define A\na;", getGoogleStyle()));
  verifyFormat("functionCallTo(\n"
               "    someOtherFunction(\n"
               "        withSomeParameters, whichInSequence,\n"
               "        areLongerThanALine(andAnotherCall,\n"
               "#define A B\n"
               "                           withMoreParamters,\n"
               "                           whichStronglyInfluenceTheLayout),\n"
               "        andMoreParameters),\n"
               "    trailing);",
               getLLVMStyleWithColumns(69));
}

TEST_F(FormatTest, LayoutBlockInsideParens) {
  EXPECT_EQ("functionCall({\n"
            "  int i;\n"
            "});",
            format(" functionCall ( {int i;} );"));

  // FIXME: This is bad, find a better and more generic solution.
  EXPECT_EQ("functionCall({\n"
            "  int i;\n"
            "},\n"
            "             aaaa, bbbb, cccc);",
            format(" functionCall ( {int i;},  aaaa,   bbbb, cccc);"));
  verifyFormat(
      "Aaa({\n"
      "  int i;\n"
      "},\n"
      "    aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb,\n"
      "                                     ccccccccccccccccc));");
}

TEST_F(FormatTest, LayoutBlockInsideStatement) {
  EXPECT_EQ("SOME_MACRO { int i; }\n"
            "int i;",
            format("  SOME_MACRO  {int i;}  int i;"));
}

TEST_F(FormatTest, LayoutNestedBlocks) {
  verifyFormat("void AddOsStrings(unsigned bitmask) {\n"
               "  struct s {\n"
               "    int i;\n"
               "  };\n"
               "  s kBitsToOs[] = { { 10 } };\n"
               "  for (int i = 0; i < 10; ++i)\n"
               "    return;\n"
               "}");
}

TEST_F(FormatTest, PutEmptyBlocksIntoOneLine) {
  EXPECT_EQ("{}", format("{}"));
  verifyFormat("enum E {};");
  verifyFormat("enum E {}");
}

//===----------------------------------------------------------------------===//
// Line break tests.
//===----------------------------------------------------------------------===//

TEST_F(FormatTest, FormatsAwesomeMethodCall) {
  verifyFormat(
      "SomeLongMethodName(SomeReallyLongMethod(CallOtherReallyLongMethod(\n"
      "                       parameter, parameter, parameter)),\n"
      "                   SecondLongCall(parameter));");
}

TEST_F(FormatTest, PreventConfusingIndents) {
  verifyFormat(
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(\n"
      "    aaaaaaaaaaaaaaaaaaaaaaaa(\n"
      "        aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa),\n"
      "    aaaaaaaaaaaaaaaaaaaaaaaa);");
  verifyFormat(
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
      "    [aaaaaaaaaaaaaaaaaaaaaaaa\n"
      "         [aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa]\n"
      "         [aaaaaaaaaaaaaaaaaaaaaaaa]];");
  verifyFormat(
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa<\n"
      "    aaaaaaaaaaaaaaaaaaaaaaaa<\n"
      "        aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa>,\n"
      "    aaaaaaaaaaaaaaaaaaaaaaaa>;");
  verifyFormat("int a = bbbb && ccc && fffff(\n"
               "#define A Just forcing a new line\n"
               "                           ddd);");
}

TEST_F(FormatTest, LineBreakingInBinaryExpressions) {
  verifyFormat(
      "bool aaaaaaa =\n"
      "    aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(aaa).aaaaaaaaaaaaaaaaaaa() ||\n"
      "    bbbbbbbb();");
  verifyFormat("bool aaaaaaaaaaaaaaaaaaaaa =\n"
               "    aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa != bbbbbbbbbbbbbbbbbb &&\n"
               "    ccccccccc == ddddddddddd;");

  verifyFormat("aaaaaa = aaaaaaa(aaaaaaa, // break\n"
               "                 aaaaaa) &&\n"
               "         bbbbbb && cccccc;");
  verifyFormat("aaaaaa = aaaaaaa(aaaaaaa, // break\n"
               "                 aaaaaa) >>\n"
               "         bbbbbb;");
  verifyFormat("Whitespaces.addUntouchableComment(\n"
               "    SourceMgr.getSpellingColumnNumber(\n"
               "        TheLine.Last->FormatTok.Tok.getLocation()) -\n"
               "    1);");

  verifyFormat("if ((aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa ||\n"
               "     bbbbbbbbbbbbbbbbbb) && // aaaaaaaaaaaaaaaa\n"
               "    cccccc) {\n}");

  // If the LHS of a comparison is not a binary expression itself, the
  // additional linebreak confuses many people.
  verifyFormat(
      "if (aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(\n"
      "        aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa) > 5) {\n"
      "}");
  verifyFormat(
      "if (aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(\n"
      "        aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa) == 5) {\n"
      "}");
  // Even explicit parentheses stress the precedence enough to make the
  // additional break unnecessary.
  verifyFormat(
      "if ((aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa +\n"
      "     aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa) == 5) {\n"
      "}");
  // This cases is borderline, but with the indentation it is still readable.
  verifyFormat(
      "if (aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(\n"
      "        aaaaaaaaaaaaaaa) > aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa +\n"
      "                               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa) {\n"
      "}",
      getLLVMStyleWithColumns(75));

  // If the LHS is a binary expression, we should still use the additional break
  // as otherwise the formatting hides the operator precedence.
  verifyFormat(
      "if (aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa +\n"
      "        aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa ==\n"
      "    5) {\n"
      "}");

  FormatStyle OnePerLine = getLLVMStyle();
  OnePerLine.BinPackParameters = false;
  verifyFormat(
      "if (aaaaaaaaaaaaaaaaaaaaaaaaaaaa || aaaaaaaaaaaaaaaaaaaaaaaaaaaa ||\n"
      "    aaaaaaaaaaaaaaaaaaaaaaaaaaaa || aaaaaaaaaaaaaaaaaaaaaaaaaaaa ||\n"
      "    aaaaaaaaaaaaaaaaaaaaaaaaaaaa) {\n}",
      OnePerLine);
}

TEST_F(FormatTest, ExpressionIndentation) {
  verifyFormat("bool value = aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa +\n"
               "                     aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa +\n"
               "                     aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa ==\n"
               "                 aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa *\n"
               "                         bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb +\n"
               "                     bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb &&\n"
               "             aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa *\n"
               "                     aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa >\n"
               "                 ccccccccccccccccccccccccccccccccccccccccc;");
  verifyFormat("if (aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa *\n"
               "            aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa +\n"
               "        aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa ==\n"
               "    bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb) {\n}");
  verifyFormat("if (aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa +\n"
               "        aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa *\n"
               "            aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa ==\n"
               "    bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb) {\n}");
  verifyFormat("if (aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa ==\n"
               "    aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa *\n"
               "            aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa +\n"
               "        bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb) {\n}");
  verifyFormat("if () {\n"
               "} else if (aaaaa && bbbbb > // break\n"
               "                        ccccc) {\n"
               "}");

  // Presence of a trailing comment used to change indentation of b.
  verifyFormat("return aaaaaaaaaaaaaaaaaaa +\n"
               "       b;\n"
               "return aaaaaaaaaaaaaaaaaaa +\n"
               "       b; //",
               getLLVMStyleWithColumns(30));
}

TEST_F(FormatTest, ExpressionIndentationBreakingBeforeOperators) {
  // Not sure what the best system is here. Like this, the LHS can be found
  // immediately above an operator (everything with the same or a higher
  // indent). The RHS is aligned right of the operator and so compasses
  // everything until something with the same indent as the operator is found.
  // FIXME: Is this a good system?
  FormatStyle Style = getLLVMStyle();
  Style.BreakBeforeBinaryOperators = true;
  verifyFormat(
      "bool value = aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
      "             + aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
      "             + aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
      "             == aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
      "                * bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\n"
      "                + bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\n"
      "             && aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
      "                * aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
      "                > ccccccccccccccccccccccccccccccccccccccccc;",
      Style);
  verifyFormat("if (aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
               "    * aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
               "    + aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
               "    == bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb) {\n}",
               Style);
  verifyFormat("if (aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
               "    + aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
               "      * aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
               "    == bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb) {\n}",
               Style);
  verifyFormat("if (aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
               "    == aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
               "       * aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
               "       + bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb) {\n}",
               Style);
  verifyFormat("if () {\n"
               "} else if (aaaaa && bbbbb // break\n"
               "                    > ccccc) {\n"
               "}",
               Style);
}

TEST_F(FormatTest, ConstructorInitializers) {
  verifyFormat("Constructor() : Initializer(FitsOnTheLine) {}");
  verifyFormat("Constructor() : Inttializer(FitsOnTheLine) {}",
               getLLVMStyleWithColumns(45));
  verifyFormat("Constructor()\n"
               "    : Inttializer(FitsOnTheLine) {}",
               getLLVMStyleWithColumns(44));
  verifyFormat("Constructor()\n"
               "    : Inttializer(FitsOnTheLine) {}",
               getLLVMStyleWithColumns(43));

  verifyFormat(
      "SomeClass::Constructor()\n"
      "    : aaaaaaaaaaaaa(aaaaaaaaaaaaaa), aaaaaaaaaaaaaaa(aaaaaaaaaaaa) {}");

  verifyFormat(
      "SomeClass::Constructor()\n"
      "    : aaaaaaaaaaaaa(aaaaaaaaaaaaaa), aaaaaaaaaaaaa(aaaaaaaaaaaaaa),\n"
      "      aaaaaaaaaaaaa(aaaaaaaaaaaaaa) {}");
  verifyFormat(
      "SomeClass::Constructor()\n"
      "    : aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa),\n"
      "      aaaaaaaaaaaaaaa(aaaaaaaaaaaa) {}");

  verifyFormat("Constructor()\n"
               "    : aaaaaaaaaaaaaaaaaaaaaaaa(aaaaaaaaaaaaaaaaaaaaaaaaaaa),\n"
               "      aaaaaaaaaaaaaaaaaaaaaaaa(aaaaaaaaaaaaaaaaaaaaaaaaaaa,\n"
               "                               aaaaaaaaaaaaaaaaaaaaaaaaaaa),\n"
               "      aaaaaaaaaaaaaaaaaaaaaaa() {}");

  verifyFormat("Constructor()\n"
               "    : aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(\n"
               "          aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa) {}");

  verifyFormat("Constructor(int Parameter = 0)\n"
               "    : aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(aaaaaaaaaaaaaaaaa),\n"
               "      aaaaaaaaaaaa(aaaaaaaaaaaaaaaaa) {}");

  // Here a line could be saved by splitting the second initializer onto two
  // lines, but that is not desireable.
  verifyFormat("Constructor()\n"
               "    : aaaaaaaaaaaaaaaaaaaaaaaa(aaaaaaaaaaaaaaaaaaaaaaaa),\n"
               "      aaaaaaaaaaa(aaaaaaaaaaa),\n"
               "      aaaaaaaaaaaaaaaaaaaaat(aaaaaaaaaaaaaaaaaaaaaaaaaaaa) {}");

  FormatStyle OnePerLine = getLLVMStyle();
  OnePerLine.ConstructorInitializerAllOnOneLineOrOnePerLine = true;
  verifyFormat("SomeClass::Constructor()\n"
               "    : aaaaaaaaaaaaa(aaaaaaaaaaaaaa),\n"
               "      aaaaaaaaaaaaa(aaaaaaaaaaaaaa),\n"
               "      aaaaaaaaaaaaa(aaaaaaaaaaaaaa) {}",
               OnePerLine);
  verifyFormat("SomeClass::Constructor()\n"
               "    : aaaaaaaaaaaaa(aaaaaaaaaaaaaa), // Some comment\n"
               "      aaaaaaaaaaaaa(aaaaaaaaaaaaaa),\n"
               "      aaaaaaaaaaaaa(aaaaaaaaaaaaaa) {}",
               OnePerLine);
  verifyFormat("MyClass::MyClass(int var)\n"
               "    : some_var_(var),            // 4 space indent\n"
               "      some_other_var_(var + 1) { // lined up\n"
               "}",
               OnePerLine);
  verifyFormat("Constructor()\n"
               "    : aaaaa(aaaaaa),\n"
               "      aaaaa(aaaaaa),\n"
               "      aaaaa(aaaaaa),\n"
               "      aaaaa(aaaaaa),\n"
               "      aaaaa(aaaaaa) {}",
               OnePerLine);
  verifyFormat("Constructor()\n"
               "    : aaaaa(aaaaaaaaaaaaaaaaaaaaaa, aaaaaaaaaaaaaaaaaaaaaa,\n"
               "            aaaaaaaaaaaaaaaaaaaaaa) {}",
               OnePerLine);
}

TEST_F(FormatTest, MemoizationTests) {
  // This breaks if the memoization lookup does not take \c Indent and
  // \c LastSpace into account.
  verifyFormat(
      "extern CFRunLoopTimerRef\n"
      "CFRunLoopTimerCreate(CFAllocatorRef allocato, CFAbsoluteTime fireDate,\n"
      "                     CFTimeInterval interval, CFOptionFlags flags,\n"
      "                     CFIndex order, CFRunLoopTimerCallBack callout,\n"
      "                     CFRunLoopTimerContext *context) {}");

  // Deep nesting somewhat works around our memoization.
  verifyFormat(
      "aaaaa(aaaaa(aaaaa(aaaaa(aaaaa(aaaaa(aaaaa(aaaaa(aaaaa(aaaaa(aaaaa(\n"
      "    aaaaa(aaaaa(aaaaa(aaaaa(aaaaa(aaaaa(aaaaa(aaaaa(aaaaa(aaaaa(\n"
      "        aaaaa(aaaaa(aaaaa(aaaaa(aaaaa(aaaaa(aaaaa(aaaaa(aaaaa(\n"
      "            aaaaa(aaaaa(aaaaa(aaaaa(aaaaa(aaaaa(aaaaa(aaaaa(aaaaa(\n"
      "                aaaaa())))))))))))))))))))))))))))))))))))))));",
      getLLVMStyleWithColumns(65));
  verifyFormat(
      "aaaaa(\n"
      "    aaaaa,\n"
      "    aaaaa(\n"
      "        aaaaa,\n"
      "        aaaaa(\n"
      "            aaaaa,\n"
      "            aaaaa(\n"
      "                aaaaa,\n"
      "                aaaaa(\n"
      "                    aaaaa,\n"
      "                    aaaaa(\n"
      "                        aaaaa,\n"
      "                        aaaaa(\n"
      "                            aaaaa,\n"
      "                            aaaaa(\n"
      "                                aaaaa,\n"
      "                                aaaaa(\n"
      "                                    aaaaa,\n"
      "                                    aaaaa(\n"
      "                                        aaaaa,\n"
      "                                        aaaaa(\n"
      "                                            aaaaa,\n"
      "                                            aaaaa(\n"
      "                                                aaaaa,\n"
      "                                                aaaaa))))))))))));",
      getLLVMStyleWithColumns(65));
  verifyFormat(
      "a(a(a(a(a(a(a(a(a(a(a(a(a(a(a(a(a(a(a(a(a(a(), a), a), a), a),\n"
      "                                  a),\n"
      "                                a),\n"
      "                              a),\n"
      "                            a),\n"
      "                          a),\n"
      "                        a),\n"
      "                      a),\n"
      "                    a),\n"
      "                  a),\n"
      "                a),\n"
      "              a),\n"
      "            a),\n"
      "          a),\n"
      "        a),\n"
      "      a),\n"
      "    a),\n"
      "  a)",
      getLLVMStyleWithColumns(65));

  // This test takes VERY long when memoization is broken.
  FormatStyle OnePerLine = getLLVMStyle();
  OnePerLine.ConstructorInitializerAllOnOneLineOrOnePerLine = true;
  OnePerLine.BinPackParameters = false;
  std::string input = "Constructor()\n"
                      "    : aaaa(a,\n";
  for (unsigned i = 0, e = 80; i != e; ++i) {
    input += "           a,\n";
  }
  input += "           a) {}";
  verifyFormat(input, OnePerLine);
}

TEST_F(FormatTest, BreaksAsHighAsPossible) {
  verifyFormat(
      "void f() {\n"
      "  if ((aaaaaaaaaaaaaaaaaaaaaaaaaaaaa && aaaaaaaaaaaaaaaaaaaaaaaaaa) ||\n"
      "      (bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb && bbbbbbbbbbbbbbbbbbbbbbbbbb))\n"
      "    f();\n"
      "}");
  verifyFormat("if (Intervals[i].getRange().getFirst() <\n"
               "    Intervals[i - 1].getRange().getLast()) {\n}");
}

TEST_F(FormatTest, BreaksFunctionDeclarations) {
  // Principially, we break function declarations in a certain order:
  // 1) break amongst arguments.
  verifyFormat("Aaaaaaaaaaaaaa bbbbbbbbbbbbbb(Cccccccccccccc cccccccccccccc,\n"
               "                              Cccccccccccccc cccccccccccccc);");
  verifyFormat(
      "template <class TemplateIt>\n"
      "SomeReturnType SomeFunction(TemplateIt begin, TemplateIt end,\n"
      "                            TemplateIt *stop) {}");

  // 2) break after return type.
  verifyFormat(
      "Aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
      "    bbbbbbbbbbbbbb(Cccccccccccccc cccccccccccccccccccccccccc);",
      getGoogleStyle());

  // 3) break after (.
  verifyFormat(
      "Aaaaaaaaaaaaaaaaaaaaaaaa bbbbbbbbbbbbbbbbbb(\n"
      "    Cccccccccccccccccccccccccccccc cccccccccccccccccccccccccccccccc);",
      getGoogleStyle());

  // 4) break before after nested name specifiers.
  verifyFormat(
      "Aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
      "    SomeClasssssssssssssssssssssssssssssssssssssss::\n"
      "        bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb(Cccccccccccccc cccccccccc);",
      getGoogleStyle());

  // However, there are exceptions, if a sufficient amount of lines can be
  // saved.
  // FIXME: The precise cut-offs wrt. the number of saved lines might need some
  // more adjusting.
  verifyFormat("Aaaaaaaaaaaaaaaaaa bbbbbbbbbbbbbb(Cccccccccccccc cccccccccc,\n"
               "                                  Cccccccccccccc cccccccccc,\n"
               "                                  Cccccccccccccc cccccccccc,\n"
               "                                  Cccccccccccccc cccccccccc,\n"
               "                                  Cccccccccccccc cccccccccc);");
  verifyFormat(
      "Aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
      "    bbbbbbbbbbb(Cccccccccccccc cccccccccc, Cccccccccccccc cccccccccc,\n"
      "                Cccccccccccccc cccccccccc, Cccccccccccccc cccccccccc,\n"
      "                Cccccccccccccc cccccccccc, Cccccccccccccc cccccccccc);",
      getGoogleStyle());
  verifyFormat(
      "Aaaaaaaaaa bbbbbbbbbbbbbbbbbbbbbbbbbbbbbb(Cccccccccccccc cccccccccc,\n"
      "                                          Cccccccccccccc cccccccccc,\n"
      "                                          Cccccccccccccc cccccccccc,\n"
      "                                          Cccccccccccccc cccccccccc,\n"
      "                                          Cccccccccccccc cccccccccc,\n"
      "                                          Cccccccccccccc cccccccccc,\n"
      "                                          Cccccccccccccc cccccccccc);");
  verifyFormat("Aaaaaaaaaa bbbbbbbbbbbbbbbbbbbbbbbbbbbbbb(\n"
               "    Cccccccccccccc cccccccccc, Cccccccccccccc cccccccccc,\n"
               "    Cccccccccccccc cccccccccc, Cccccccccccccc cccccccccc,\n"
               "    Cccccccccccccc cccccccccc, Cccccccccccccc cccccccccc,\n"
               "    Cccccccccccccc cccccccccc, Cccccccccccccc cccccccccc);");

  // Break after multi-line parameters.
  verifyFormat("void aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(\n"
               "    aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
               "        aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,\n"
               "    bbbb bbbb);");

  // Treat overloaded operators like other functions.
  verifyFormat("SomeLoooooooooooooooooooooooooogType\n"
               "operator>(const SomeLoooooooooooooooooooooooooogType &other);");
  verifyGoogleFormat(
      "SomeLoooooooooooooooooooooooooooooogType operator<<(\n"
      "    const SomeLooooooooogType &a, const SomeLooooooooogType &b);");
}

TEST_F(FormatTest, TrailingReturnType) {
  verifyFormat("auto foo() -> int;\n");
  verifyFormat("struct S {\n"
               "  auto bar() const -> int;\n"
               "};");
  verifyFormat("template <size_t Order, typename T>\n"
               "auto load_img(const std::string &filename)\n"
               "    -> alias::tensor<Order, T, mem::tag::cpu> {}");

  // Not trailing return types.
  verifyFormat("void f() { auto a = b->c(); }");
}

TEST_F(FormatTest, BreaksFunctionDeclarationsWithTrailingTokens) {
  // Avoid breaking before trailing 'const'.
  verifyFormat("void someLongFunction(\n"
               "    int someLongParameter) const {}",
               getLLVMStyleWithColumns(46));
  FormatStyle Style = getGoogleStyle();
  Style.ColumnLimit = 47;
  verifyFormat("void\n"
               "someLongFunction(int someLongParameter) const {\n}",
               getLLVMStyleWithColumns(47));
  verifyFormat("void someLongFunction(\n"
               "    int someLongParameter) const {}",
               Style);
  verifyFormat("LoooooongReturnType\n"
               "someLoooooooongFunction() const {}",
               getLLVMStyleWithColumns(47));
  verifyFormat("LoooooongReturnType someLoooooooongFunction()\n"
               "    const {}",
               Style);

  // Avoid breaking before other trailing annotations, if they are not
  // function-like.
  verifyFormat("void SomeFunction(aaaaaaaaaaaaaaaaaaaaaaaaaa,\n"
               "                  aaaaaaaaaaaaaaaaaaaaaaaaaa) OVERRIDE;");

  // Breaking before function-like trailing annotations is fine to keep them
  // close to their arguments.
  verifyFormat("void aaaaaaaaaaaa(int aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa)\n"
               "    LOCKS_EXCLUDED(aaaaaaaaaaaaa);");
  verifyFormat("void aaaaaaaaaaaa(int aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa) const\n"
               "    LOCKS_EXCLUDED(aaaaaaaaaaaaa);");
  verifyFormat("void aaaaaaaaaaaa(int aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa) const\n"
               "    LOCKS_EXCLUDED(aaaaaaaaaaaaa) {}");

  verifyFormat(
      "void aaaaaaaaaaaaaaaaaa()\n"
      "    __attribute__((aaaaaaaaaaaaaaaaaaaaaaaaa, aaaaaaaaaaaaaaaaaaaaaaa,\n"
      "                   aaaaaaaaaaaaaaaaaaaaaaaaa));");
  verifyFormat("bool aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
               "    __attribute__((unused));");
  verifyFormat(
      "bool aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
      "    GUARDED_BY(aaaaaaaaaaaa);",
      getGoogleStyle());
  verifyFormat(
      "bool aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
      "    GUARDED_BY(aaaaaaaaaaaa);",
      getGoogleStyle());
}

TEST_F(FormatTest, BreaksDesireably) {
  verifyFormat("if (aaaaaaaaaaaaaaaaaaa(aaaaaaaaaaaaaaa) ||\n"
               "    aaaaaaaaaaaaaaaaaaa(aaaaaaaaaaaaaaa) ||\n"
               "    aaaaaaaaaaaaaaaaaaa(aaaaaaaaaaaaaaa)) {\n}");
  verifyFormat("if (aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(\n"
               "        aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa)) {\n"
               "}");

  verifyFormat(
      "aaaaaaaaaaaaaaaaaaaaa(aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,\n"
      "                      aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa) {}");

  verifyFormat("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(\n"
               "    aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(\n"
               "        aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa));");

  verifyFormat(
      "aaaaaaaa(aaaaaaaaaaaaa, aaaaaaaaaaaaaaa(aaaaaaaaaaaaaaaaaaaaaaaaaaaaa(\n"
      "                            aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa)),\n"
      "         aaaaaaaa(aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(\n"
      "             aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa)));");

  verifyFormat("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa ||\n"
               "    (aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa);");

  verifyFormat(
      "void f() {\n"
      "  aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa &&\n"
      "                                 aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa);\n"
      "}");
  verifyFormat(
      "aaaaaa(new Aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(\n"
      "    aaaaaaaaaaaaaaaaaaaaaaaaaaaaa));");
  verifyFormat(
      "aaaaaa(aaa, new Aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(\n"
      "                aaaaaaaaaaaaaaaaaaaaaaaaaaaaa));");
  verifyFormat(
      "aaaaaaaaaaaaaaaaa(\n"
      "    aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa + aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,\n"
      "    aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa);");

  // This test case breaks on an incorrect memoization, i.e. an optimization not
  // taking into account the StopAt value.
  verifyFormat(
      "return aaaaaaaaaaaaaaaaaaaaaaaa || aaaaaaaaaaaaaaaaaaaaaaa ||\n"
      "       aaaaaaaaaaa(aaaaaaaaa) || aaaaaaaaaaaaaaaaaaaaaaa ||\n"
      "       aaaaaaaaaaaaaaaaaaaaaaaaa || aaaaaaaaaaaaaaaaaaaaaaa ||\n"
      "       (aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa);");

  verifyFormat("{\n  {\n    {\n"
               "      Annotation.SpaceRequiredBefore =\n"
               "          Line.Tokens[i - 1].Tok.isNot(tok::l_paren) &&\n"
               "          Line.Tokens[i - 1].Tok.isNot(tok::l_square);\n"
               "    }\n  }\n}");

  // Break on an outer level if there was a break on an inner level.
  EXPECT_EQ("f(g(h(a, // comment\n"
            "      b, c),\n"
            "    d, e),\n"
            "  x, y);",
            format("f(g(h(a, // comment\n"
                   "    b, c), d, e), x, y);"));

  // Prefer breaking similar line breaks.
  verifyFormat(
      "const int kTrackingOptions = NSTrackingMouseMoved |\n"
      "                             NSTrackingMouseEnteredAndExited |\n"
      "                             NSTrackingActiveAlways;");
}

TEST_F(FormatTest, FormatsOneParameterPerLineIfNecessary) {
  FormatStyle NoBinPacking = getGoogleStyle();
  NoBinPacking.BinPackParameters = false;
  verifyFormat("f(aaaaaaaaaaaaaaaaaaaa,\n"
               "  aaaaaaaaaaaaaaaaaaaa,\n"
               "  aaaaaaaaaaaaaaaaaaaa + aaaaaaaaaaaaaaaaaaaa);",
               NoBinPacking);
  verifyFormat("aaaaaaa(aaaaaaaaaaaaa,\n"
               "        aaaaaaaaaaaaa,\n"
               "        aaaaaaaaaaaaa(aaaaaaaaaaaaaaaaa, aaaaaaaaaaaaaaaa));",
               NoBinPacking);
  verifyFormat(
      "aaaaaaaa(aaaaaaaaaaaaa,\n"
      "         aaaaaaaaaaaaaaa(aaaaaaaaaaaaaaaaaaaaaaaaaaaaa(\n"
      "             aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa)),\n"
      "         aaaaaaaa(aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(\n"
      "             aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa)));",
      NoBinPacking);
  verifyFormat("aaaaaaaaaaaaaaa(aaaaaaaaa, aaaaaaaaa, aaaaaaaaaaaaaaaaaaaaa)\n"
               "    .aaaaaaaaaaaaaaaaaa();",
               NoBinPacking);
  verifyFormat("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(\n"
               "    aaaaaaaaaa, aaaaaaaaaa, aaaaaaaaaa, aaaaaaaaaaa);",
               NoBinPacking);

  verifyFormat(
      "aaaaaaaaaaaa(aaaaaaaaaaaaaaaaaaaaaaaaaaaa + aaaaaaaaaaaaaaaaaaaaaaaaa,\n"
      "             aaaaaaaaaaaa,\n"
      "             aaaaaaaaaaaa);",
      NoBinPacking);
  verifyFormat(
      "somefunction(someotherFunction(ddddddddddddddddddddddddddddddddddd,\n"
      "                               ddddddddddddddddddddddddddddd),\n"
      "             test);",
      NoBinPacking);

  verifyFormat("std::vector<aaaaaaaaaaaaaaaaaaaaaaa,\n"
               "            aaaaaaaaaaaaaaaaaaaaaaa,\n"
               "            aaaaaaaaaaaaaaaaaaaaaaa> aaaaaaaaaaaaaaaaaa;",
               NoBinPacking);
  verifyFormat("a(\"a\"\n"
               "  \"a\",\n"
               "  a);");

  NoBinPacking.AllowAllParametersOfDeclarationOnNextLine = false;
  verifyFormat("void aaaaaaaaaa(aaaaaaaaa,\n"
               "                aaaaaaaaa,\n"
               "                aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa);",
               NoBinPacking);
  verifyFormat(
      "void f() {\n"
      "  aaaaaaaaaaaaaaaaaaaaaaa(aaaaaaaaa, aaaaaaaaa, aaaaaaaaaaaaaaaaaaaaa)\n"
      "      .aaaaaaa();\n"
      "}",
      NoBinPacking);
  verifyFormat(
      "template <class SomeType, class SomeOtherType>\n"
      "SomeType SomeFunction(SomeType Type, SomeOtherType OtherType) {}",
      NoBinPacking);
}

TEST_F(FormatTest, AdaptiveOnePerLineFormatting) {
  FormatStyle Style = getLLVMStyleWithColumns(15);
  Style.ExperimentalAutoDetectBinPacking = true;
  EXPECT_EQ("aaa(aaaa,\n"
            "    aaaa,\n"
            "    aaaa);\n"
            "aaa(aaaa,\n"
            "    aaaa,\n"
            "    aaaa);",
            format("aaa(aaaa,\n" // one-per-line
                   "  aaaa,\n"
                   "    aaaa  );\n"
                   "aaa(aaaa,  aaaa,  aaaa);", // inconclusive
                   Style));
  EXPECT_EQ("aaa(aaaa, aaaa,\n"
            "    aaaa);\n"
            "aaa(aaaa, aaaa,\n"
            "    aaaa);",
            format("aaa(aaaa,  aaaa,\n" // bin-packed
                   "    aaaa  );\n"
                   "aaa(aaaa,  aaaa,  aaaa);", // inconclusive
                   Style));
}

TEST_F(FormatTest, FormatsBuilderPattern) {
  verifyFormat(
      "return llvm::StringSwitch<Reference::Kind>(name)\n"
      "    .StartsWith(\".eh_frame_hdr\", ORDER_EH_FRAMEHDR)\n"
      "    .StartsWith(\".eh_frame\", ORDER_EH_FRAME).StartsWith(\".init\", ORDER_INIT)\n"
      "    .StartsWith(\".fini\", ORDER_FINI).StartsWith(\".hash\", ORDER_HASH)\n"
      "    .Default(ORDER_TEXT);\n");

  verifyFormat("return aaaaaaaaaaaaaaaaa->aaaaa().aaaaaaaaaaaaa().aaaaaa() <\n"
               "       aaaaaaaaaaaaaaa->aaaaa().aaaaaaaaaaaaa().aaaaaa();");
  verifyFormat(
      "aaaaaaa->aaaaaaa->aaaaaaaaaaaaaaaa(\n"
      "                      aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa)\n"
      "    ->aaaaaaaa(aaaaaaaaaaaaaaa);");
  verifyFormat(
      "aaaaaaaaaaaaaaaaaaa()->aaaaaa(bbbbb)->aaaaaaaaaaaaaaaaaaa( // break\n"
      "    aaaaaaaaaaaaaa);");
  verifyFormat(
      "aaaaaaaaaaaaaaaaaaaaaaa *aaaaaaaaa = aaaaaa->aaaaaaaaaaaa()\n"
      "    ->aaaaaaaaaaaaaaaa(\n"
      "          aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa)\n"
      "    ->aaaaaaaaaaaaaaaaa();");
}

TEST_F(FormatTest, BreaksAccordingToOperatorPrecedence) {
  verifyFormat(
      "if (aaaaaaaaaaaaaaaaaaaaaaaaa ||\n"
      "    bbbbbbbbbbbbbbbbbbbbbbbbb && ccccccccccccccccccccccccc) {\n}");
  verifyFormat("if (aaaaaaaaaaaaaaaaaaaaaaaaa && bbbbbbbbbbbbbbbbbbbbbbbbb ||\n"
               "    ccccccccccccccccccccccccc) {\n}");
  verifyFormat("if (aaaaaaaaaaaaaaaaaaaaaaaaa || bbbbbbbbbbbbbbbbbbbbbbbbb ||\n"
               "    ccccccccccccccccccccccccc) {\n}");
  verifyFormat(
      "if ((aaaaaaaaaaaaaaaaaaaaaaaaa || bbbbbbbbbbbbbbbbbbbbbbbbb) &&\n"
      "    ccccccccccccccccccccccccc) {\n}");
  verifyFormat("return aaaa & AAAAAAAAAAAAAAAAAAAAAAAAAAAAA ||\n"
               "       bbbb & BBBBBBBBBBBBBBBBBBBBBBBBBBBBB ||\n"
               "       cccc & CCCCCCCCCCCCCCCCCCCCCCCCCC ||\n"
               "       dddd & DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD;");
  verifyFormat("if ((aaaaaaaaaa != aaaaaaaaaaaaaaa ||\n"
               "     aaaaaaaaaaaaaaaaaaaaaaaa() >= aaaaaaaaaaaaaaaaaaaa) &&\n"
               "    aaaaaaaaaaaaaaa != aa) {\n}");
}

TEST_F(FormatTest, BreaksAfterAssignments) {
  verifyFormat(
      "unsigned Cost =\n"
      "    TTI.getMemoryOpCost(I->getOpcode(), VectorTy, SI->getAlignment(),\n"
      "                        SI->getPointerAddressSpaceee());\n");
  verifyFormat(
      "CharSourceRange LineRange = CharSourceRange::getTokenRange(\n"
      "    Line.Tokens.front().Tok.getLo(), Line.Tokens.back().Tok.getLoc());");

  verifyFormat(
      "aaaaaaaaaaaaaaaaaaaaaaaaaa aaaa = aaaaaaaaaaaaaa(0)\n"
      "    .aaaa().aaaaaaaaaaaa(aaaaaaaaaaaaaaaaaaa::aaaaaaaaaaaaaaaaaaaaa);");
  verifyFormat("unsigned OriginalStartColumn =\n"
               "    SourceMgr.getSpellingColumnNumber(\n"
               "        Current.FormatTok.getStartOfNonWhitespace()) -\n"
               "    1;");
}

TEST_F(FormatTest, AlignsAfterAssignments) {
  verifyFormat(
      "int Result = aaaaaaaaaaaaaaaaaaaaaaaaa + aaaaaaaaaaaaaaaaaaaaaaaaa +\n"
      "             aaaaaaaaaaaaaaaaaaaaaaaaa;");
  verifyFormat(
      "Result += aaaaaaaaaaaaaaaaaaaaaaaaa + aaaaaaaaaaaaaaaaaaaaaaaaa +\n"
      "          aaaaaaaaaaaaaaaaaaaaaaaaa;");
  verifyFormat(
      "Result >>= aaaaaaaaaaaaaaaaaaaaaaaaa + aaaaaaaaaaaaaaaaaaaaaaaaa +\n"
      "           aaaaaaaaaaaaaaaaaaaaaaaaa;");
  verifyFormat(
      "int Result = (aaaaaaaaaaaaaaaaaaaaaaaaa + aaaaaaaaaaaaaaaaaaaaaaaaa +\n"
      "              aaaaaaaaaaaaaaaaaaaaaaaaa);");
  verifyFormat(
      "double LooooooooooooooooooooooooongResult = aaaaaaaaaaaaaaaaaaaaaaaa +\n"
      "                                            aaaaaaaaaaaaaaaaaaaaaaaa +\n"
      "                                            aaaaaaaaaaaaaaaaaaaaaaaa;");
}

TEST_F(FormatTest, AlignsAfterReturn) {
  verifyFormat(
      "return aaaaaaaaaaaaaaaaaaaaaaaaa + aaaaaaaaaaaaaaaaaaaaaaaaa +\n"
      "       aaaaaaaaaaaaaaaaaaaaaaaaa;");
  verifyFormat(
      "return (aaaaaaaaaaaaaaaaaaaaaaaaa + aaaaaaaaaaaaaaaaaaaaaaaaa +\n"
      "        aaaaaaaaaaaaaaaaaaaaaaaaa);");
  verifyFormat(
      "return aaaaaaaaaaaaaaaaaaaaaa + aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa >=\n"
      "       aaaaaaaaaaaaaaaaaaaaaa();");
  verifyFormat(
      "return (aaaaaaaaaaaaaaaaaaaaaa + aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa >=\n"
      "        aaaaaaaaaaaaaaaaaaaaaa());");
  verifyFormat("return aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(\n"
               "    aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa);");
  verifyFormat("return aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(\n"
               "           aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa) &&\n"
               "       aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa;");
}

TEST_F(FormatTest, BreaksConditionalExpressions) {
  verifyFormat(
      "aaaa(aaaaaaaaaaaaaaaaaaaa, aaaaaaaaaaaaaaaaaaaaaaaaaa\n"
      "                               ? aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
      "                               : aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa);");
  verifyFormat(
      "aaaa(aaaaaaaaaaaaaaaaaaaa, aaaaaaa ? aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
      "                                   : aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa);");
  verifyFormat(
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(aaaaaaaaaaaaaaaaaaaa ? aaaa(aaaaaa)\n"
      "                                                    : aaaaaaaaaaaaa);");
  verifyFormat(
      "aaaaaaaaaaaaaaaaaa(aaaaaaaaaaaaaaaaaaaaaaaaa,\n"
      "                   aaaaaaaaaaaaaaaa ? aaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
      "                                    : aaaaaaaaaaaaaaaaaaaaaaaaaaaa,\n"
      "                   aaaaaaaaaaaaa);");
  verifyFormat(
      "aaaaaaaaaaaaaaaaaa(aaaaaaaaaaaaaaaaaaaaaaaaa,\n"
      "                   aaaaaaaaaaaaaaaa ?: aaaaaaaaaaaaaaaaaaaaaaaaaaaa,\n"
      "                   aaaaaaaaaaaaa);");
  verifyFormat("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
               "    ? aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(\n"
               "          aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa)\n"
               "    : aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(\n"
               "          aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa);");
  verifyFormat("aaaaaa(aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,\n"
               "       aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
               "           ? aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(\n"
               "                 aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa)\n"
               "           : aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(\n"
               "                 aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa),\n"
               "       aaaaaaaaaaaaaaaaaaaaaaaaaaa);");
  verifyFormat("aaaaaa(aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,\n"
               "       aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
               "           ?: aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(\n"
               "                  aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa),\n"
               "       aaaaaaaaaaaaaaaaaaaaaaaaaaa);");
  verifyFormat("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
               "    ? aaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
               "    : aaaaaaaaaaaaaaaaaaaaaaaaaaa;");
  verifyFormat("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa aaaaaa =\n"
               "    aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
               "        ? aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
               "        : aaaaaaaaaaaaaaaa;");
  verifyFormat(
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa == aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
      "    ? aaaaaaaaaaaaaaa\n"
      "    : aaaaaaaaaaaaaaa;");
  verifyFormat("f(aaaaaaaaaaaaaaaa == // force break\n"
               "          aaaaaaaaa\n"
               "      ? b\n"
               "      : c);");
  verifyFormat(
      "unsigned Indent =\n"
      "    format(TheLine.First, IndentForLevel[TheLine.Level] >= 0\n"
      "                              ? IndentForLevel[TheLine.Level]\n"
      "                              : TheLine * 2,\n"
      "           TheLine.InPPDirective, PreviousEndOfLineColumn);",
      getLLVMStyleWithColumns(70));
  verifyFormat("bool aaaaaa = aaaaaaaaaaaaa //\n"
               "                  ? aaaaaaaaaaaaaaa\n"
               "                  : bbbbbbbbbbbbbbb //\n"
               "                        ? ccccccccccccccc\n"
               "                        : ddddddddddddddd;");
  verifyFormat("bool aaaaaa = aaaaaaaaaaaaa //\n"
               "                  ? aaaaaaaaaaaaaaa\n"
               "                  : (bbbbbbbbbbbbbbb //\n"
               "                         ? ccccccccccccccc\n"
               "                         : ddddddddddddddd);");

  FormatStyle NoBinPacking = getLLVMStyle();
  NoBinPacking.BinPackParameters = false;
  verifyFormat(
      "void f() {\n"
      "  g(aaa,\n"
      "    aaaaaaaaaa == aaaaaaaaaa ? aaaa : aaaaa,\n"
      "    aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa == aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
      "        ? aaaaaaaaaaaaaaa\n"
      "        : aaaaaaaaaaaaaaa);\n"
      "}",
      NoBinPacking);
  verifyFormat(
      "void f() {\n"
      "  g(aaa,\n"
      "    aaaaaaaaaa == aaaaaaaaaa ? aaaa : aaaaa,\n"
      "    aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa == aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
      "        ?: aaaaaaaaaaaaaaa);\n"
      "}",
      NoBinPacking);
}

TEST_F(FormatTest, DeclarationsOfMultipleVariables) {
  verifyFormat("bool aaaaaaaaaaaaaaaaa = aaaaaa->aaaaaaaaaaaaaaaaa(),\n"
               "     aaaaaaaaaaa = aaaaaa->aaaaaaaaaaa();");
  verifyFormat("bool a = true, b = false;");

  verifyFormat("bool aaaaaaaaaaaaaaaaaaaaaaaaa =\n"
               "         aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(aaaaaaaaaaaaaaaa),\n"
               "     bbbbbbbbbbbbbbbbbbbbbbbbb =\n"
               "         bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb(bbbbbbbbbbbbbbbb);");
  verifyFormat(
      "bool aaaaaaaaaaaaaaaaaaaaa =\n"
      "         bbbbbbbbbbbbbbbbbbbbbbbbbbbb && cccccccccccccccccccccccccccc,\n"
      "     d = e && f;");
  verifyFormat("aaaaaaaaa a = aaaaaaaaaaaaaaaaaaaa, b = bbbbbbbbbbbbbbbbbbbb,\n"
               "          c = cccccccccccccccccccc, d = dddddddddddddddddddd;");
  verifyFormat("aaaaaaaaa *a = aaaaaaaaaaaaaaaaaaa, *b = bbbbbbbbbbbbbbbbbbb,\n"
               "          *c = ccccccccccccccccccc, *d = ddddddddddddddddddd;");
  verifyFormat("aaaaaaaaa ***a = aaaaaaaaaaaaaaaaaaa, ***b = bbbbbbbbbbbbbbb,\n"
               "          ***c = ccccccccccccccccccc, ***d = ddddddddddddddd;");
  // FIXME: If multiple variables are defined, the "*" needs to move to the new
  // line. Also fix indent for breaking after the type, this looks bad.
  verifyFormat("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa*\n"
               "    aaaaaaaaaaaaaaaaaaaaaaaaaaaaa = aaaaaaaaaaaaaaaaaaa,\n"
               "    *b = bbbbbbbbbbbbbbbbbbb;",
               getGoogleStyle());

  // Not ideal, but pointer-with-type does not allow much here.
  verifyGoogleFormat(
      "aaaaaaaaa* a = aaaaaaaaaaaaaaaaaaa, *b = bbbbbbbbbbbbbbbbbbb,\n"
      "           *b = bbbbbbbbbbbbbbbbbbb, *d = ddddddddddddddddddd;");
}

TEST_F(FormatTest, ConditionalExpressionsInBrackets) {
  verifyFormat("arr[foo ? bar : baz];");
  verifyFormat("f()[foo ? bar : baz];");
  verifyFormat("(a + b)[foo ? bar : baz];");
  verifyFormat("arr[foo ? (4 > 5 ? 4 : 5) : 5 < 5 ? 5 : 7];");
}

TEST_F(FormatTest, AlignsStringLiterals) {
  verifyFormat("loooooooooooooooooooooooooongFunction(\"short literal \"\n"
               "                                      \"short literal\");");
  verifyFormat(
      "looooooooooooooooooooooooongFunction(\n"
      "    \"short literal\"\n"
      "    \"looooooooooooooooooooooooooooooooooooooooooooooooong literal\");");
  verifyFormat("someFunction(\"Always break between multi-line\"\n"
               "             \" string literals\",\n"
               "             and, other, parameters);");
  EXPECT_EQ("fun + \"1243\" /* comment */\n"
            "      \"5678\";",
            format("fun + \"1243\" /* comment */\n"
                   "      \"5678\";",
                   getLLVMStyleWithColumns(28)));
  EXPECT_EQ(
      "aaaaaa = \"aaaaaaaaaaaaaaaaaaaaaaaaaa aaaaaaaaaaaaaaaaaaaaaaaaaaaa \"\n"
      "         \"aaaaaaaaaaaaaaaaaaaaa\"\n"
      "         \"aaaaaaaaaaaaaaaa\";",
      format("aaaaaa ="
             "\"aaaaaaaaaaaaaaaaaaaaaaaaaa aaaaaaaaaaaaaaaaaaaaaaaaaaaa "
             "aaaaaaaaaaaaaaaaaaaaa\" "
             "\"aaaaaaaaaaaaaaaa\";"));
  verifyFormat("a = a + \"a\"\n"
               "        \"a\"\n"
               "        \"a\";");
  verifyFormat("f(\"a\", \"b\"\n"
               "       \"c\");");

  verifyFormat(
      "#define LL_FORMAT \"ll\"\n"
      "printf(\"aaaaa: %d, bbbbbb: %\" LL_FORMAT \"d, cccccccc: %\" LL_FORMAT\n"
      "       \"d, ddddddddd: %\" LL_FORMAT \"d\");");

  verifyFormat("#define A(X)          \\\n"
               "  \"aaaaa\" #X \"bbbbbb\" \\\n"
               "  \"ccccc\"",
               getLLVMStyleWithColumns(23));
  verifyFormat("#define A \"def\"\n"
               "f(\"abc\" A \"ghi\"\n"
               "  \"jkl\");");
}

TEST_F(FormatTest, AlwaysBreakBeforeMultilineStrings) {
  FormatStyle NoBreak = getLLVMStyle();
  NoBreak.AlwaysBreakBeforeMultilineStrings = false;
  FormatStyle Break = getLLVMStyle();
  Break.AlwaysBreakBeforeMultilineStrings = true;
  verifyFormat("aaaa = \"bbbb\"\n"
               "       \"cccc\";",
               NoBreak);
  verifyFormat("aaaa =\n"
               "    \"bbbb\"\n"
               "    \"cccc\";",
               Break);
  verifyFormat("aaaa(\"bbbb\"\n"
               "     \"cccc\");",
               NoBreak);
  verifyFormat("aaaa(\n"
               "    \"bbbb\"\n"
               "    \"cccc\");",
               Break);
  verifyFormat("aaaa(qqq, \"bbbb\"\n"
               "          \"cccc\");",
               NoBreak);
  verifyFormat("aaaa(qqq,\n"
               "     \"bbbb\"\n"
               "     \"cccc\");",
               Break);

  // Don't break if there is no column gain.
  verifyFormat("f(\"aaaa\"\n"
               "  \"bbbb\");",
               Break);

  // Treat literals with escaped newlines like multi-line string literals.
  EXPECT_EQ("x = \"a\\\n"
            "b\\\n"
            "c\";",
            format("x = \"a\\\n"
                   "b\\\n"
                   "c\";",
                   NoBreak));
  EXPECT_EQ("x =\n"
            "    \"a\\\n"
            "b\\\n"
            "c\";",
            format("x = \"a\\\n"
                   "b\\\n"
                   "c\";",
                   Break));
}

TEST_F(FormatTest, AlignsPipes) {
  verifyFormat(
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
      "    << aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
      "    << aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa;");
  verifyFormat(
      "aaaaaaaaaaaaaaaaaaaa << aaaaaaaaaaaaaaaaaaaa << aaaaaaaaaaaaaaaaaaaa\n"
      "                     << aaaaaaaaaaaaaaaaaaaa;");
  verifyFormat(
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa << aaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
      "                                 << aaaaaaaaaaaaaaaaaaaaaaaaaaaa;");
  verifyFormat(
      "llvm::outs() << \"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\"\n"
      "                \"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\"\n"
      "             << \"ccccccccccccccccccccccccccccccccccccccccccccccccc\";");
  verifyFormat(
      "aaaaaaaa << (aaaaaaaaaaaaaaaaaaa << aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
      "                                 << aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa)\n"
      "         << aaaaaaaaaaaaaaaaaaaaaaaaaaaaa;");

  verifyFormat("return out << \"somepacket = {\\n\"\n"
               "           << \"  aaaaaa = \" << pkt.aaaaaa << \"\\n\"\n"
               "           << \"  bbbb = \" << pkt.bbbb << \"\\n\"\n"
               "           << \"  cccccc = \" << pkt.cccccc << \"\\n\"\n"
               "           << \"  ddd = [\" << pkt.ddd << \"]\\n\"\n"
               "           << \"}\";");

  verifyFormat("llvm::outs() << \"aaaaaaaaaaaaaaaa: \" << aaaaaaaaaaaaaaaa\n"
               "             << \"aaaaaaaaaaaaaaaa: \" << aaaaaaaaaaaaaaaa\n"
               "             << \"aaaaaaaaaaaaaaaa: \" << aaaaaaaaaaaaaaaa;");
  verifyFormat(
      "llvm::outs() << \"aaaaaaaaaaaaaaaaa = \" << aaaaaaaaaaaaaaaaa\n"
      "             << \"bbbbbbbbbbbbbbbbb = \" << bbbbbbbbbbbbbbbbb\n"
      "             << \"ccccccccccccccccc = \" << ccccccccccccccccc\n"
      "             << \"ddddddddddddddddd = \" << ddddddddddddddddd\n"
      "             << \"eeeeeeeeeeeeeeeee = \" << eeeeeeeeeeeeeeeee;");
  verifyFormat("llvm::outs() << aaaaaaaaaaaaaaaaaaaaaaaa << \"=\"\n"
               "             << bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb;");
  verifyFormat(
      "void f() {\n"
      "  llvm::outs() << \"aaaaaaaaaaaaaaaaaaaa: \"\n"
      "               << aaaaaaaaaaaaa(aaaaaaaaaaaaaaaaaaaaaaaaaaaa);\n"
      "}");

  // Breaking before the first "<<" is generally not desirable.
  verifyFormat(
      "llvm::errs()\n"
      "    << \"aaaaaaaaaaaaaaaaaaa: \" << aaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
      "    << \"aaaaaaaaaaaaaaaaaaa: \" << aaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
      "    << \"aaaaaaaaaaaaaaaaaaa: \" << aaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
      "    << \"aaaaaaaaaaaaaaaaaaa: \" << aaaaaaaaaaaaaaaaaaaaaaaaaaaa;",
      getLLVMStyleWithColumns(70));
  verifyFormat("llvm::errs() << \"aaaaaaaaaaaaaaaaaaa: \"\n"
               "             << aaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
               "             << \"aaaaaaaaaaaaaaaaaaa: \"\n"
               "             << aaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
               "             << \"aaaaaaaaaaaaaaaaaaa: \"\n"
               "             << aaaaaaaaaaaaaaaaaaaaaaaaaaaa;",
               getLLVMStyleWithColumns(70));

  // But sometimes, breaking before the first "<<" is desirable.
  verifyFormat("Diag(aaaaaaaaaaaaaaaaaaaaaaaaaaaaa, bbbbbbbbb)\n"
               "    << aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
               "    << aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa;");
  verifyFormat("SemaRef.Diag(Loc, diag::note_for_range_begin_end)\n"
               "    << BEF << IsTemplate << Description << E->getType();");

  verifyFormat(
      "llvm::errs() << aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
      "                    .aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa();");
}

TEST_F(FormatTest, UnderstandsEquals) {
  verifyFormat(
      "aaaaaaaaaaaaaaaaa =\n"
      "    aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa;");
  verifyFormat(
      "if (aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa =\n"
      "        aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa) {\n}");
  verifyFormat(
      "if (a) {\n"
      "  f();\n"
      "} else if (aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa =\n"
      "               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa) {\n"
      "}");

  verifyFormat("if (int aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa =\n"
               "        100000000 + 10000000) {\n}");
}

TEST_F(FormatTest, WrapsAtFunctionCallsIfNecessary) {
  verifyFormat("LoooooooooooooooooooooooooooooooooooooongObject\n"
               "    .looooooooooooooooooooooooooooooooooooooongFunction();");

  verifyFormat("LoooooooooooooooooooooooooooooooooooooongObject\n"
               "    ->looooooooooooooooooooooooooooooooooooooongFunction();");

  verifyFormat(
      "LooooooooooooooooooooooooooooooooongObject->shortFunction(Parameter1,\n"
      "                                                          Parameter2);");

  verifyFormat(
      "ShortObject->shortFunction(\n"
      "    LooooooooooooooooooooooooooooooooooooooooooooooongParameter1,\n"
      "    LooooooooooooooooooooooooooooooooooooooooooooooongParameter2);");

  verifyFormat("loooooooooooooongFunction(\n"
               "    LoooooooooooooongObject->looooooooooooooooongFunction());");

  verifyFormat(
      "function(LoooooooooooooooooooooooooooooooooooongObject\n"
      "             ->loooooooooooooooooooooooooooooooooooooooongFunction());");

  verifyFormat("EXPECT_CALL(SomeObject, SomeFunction(Parameter))\n"
               "    .WillRepeatedly(Return(SomeValue));");
  verifyFormat("SomeMap[std::pair(aaaaaaaaaaaa, bbbbbbbbbbbbbbb)]\n"
               "    .insert(ccccccccccccccccccccccc);");
  verifyFormat("aaaaa(aaaaa(aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,\n"
               "            aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa).aaaaa(aaaaa),\n"
               "      aaaaaaaaaaaaaaaaaaaaa);");
  verifyFormat("aaaaaaaaaaaaaaaaaaaaaaaaa(\n"
               "    aaaaaaaaaaaaa(aaaaaaaaaaaaaaaaaaaaaaaaa)->aaaaaaaaa());");
  verifyFormat(
      "aaaaa(aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,\n"
      "      aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa)\n"
      "    .aaaaaaaaaaaaaaa(aa(aaaaaaaaaaaaaaaaaaaaaaaaaaa,\n"
      "                        aaaaaaaaaaaaaaaaaaaaaaaaaaa,\n"
      "                        aaaaaaaaaaaaaaaaaaaaaaaaaaa));");
  verifyFormat("if (aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
               "        .aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
               "        .aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
               "        .aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa()) {\n"
               "}");

  // Here, it is not necessary to wrap at "." or "->".
  verifyFormat("if (aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(aaaaaaaaaaaa) ||\n"
               "    aaaa.aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa) {\n}");
  verifyFormat(
      "aaaaaaaaaaa->aaaaaaaaa(\n"
      "    aaaaaaaaaaaaaaaaaaaaaaaaa,\n"
      "    aaaaaaaaaaaaaaaaaa->aaaaaaaaa(aaaaaaaaaaaaaaaaaaaaaaaaa));\n");

  verifyFormat(
      "aaaaaaaaaaaaaaaaaaaaaaaaa(\n"
      "    aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa().aaaaaaaaaaaaaaaaa());");
  verifyFormat("a->aaaaaa()->aaaaaaaaaaa(aaaaaaaa()->aaaaaa()->aaaaa() *\n"
               "                         aaaaaaaaa()->aaaaaa()->aaaaa());");
  verifyFormat("a->aaaaaa()->aaaaaaaaaaa(aaaaaaaa()->aaaaaa()->aaaaa() ||\n"
               "                         aaaaaaaaa()->aaaaaa()->aaaaa());");

  // FIXME: Should we break before .a()?
  verifyFormat("aaaaa(aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,\n"
               "      aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa).a();");

  FormatStyle NoBinPacking = getLLVMStyle();
  NoBinPacking.BinPackParameters = false;
  verifyFormat("aaaaaaaaaaaaaaaaaaaaaa(aaaaaaaaaaaaaaaaaaaaa)\n"
               "    .aaaaaaaaaaaaaaaaa(aaaaaaaaaaaaaaaaaaaaa)\n"
               "    .aaaaaaaaaaaaaaaaaaa(aaaaaaaaaaaaaaaaaaa,\n"
               "                         aaaaaaaaaaaaaaaaaaa,\n"
               "                         aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa);",
               NoBinPacking);

  // If there is a subsequent call, change to hanging indentation.
  verifyFormat(
      "aaaaaaaaaaaaaaaaaaaa(aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(\n"
      "                         aaaaaaaaaaaaaaaaaa, aaaaaaaaaaaaaaaaa))\n"
      "    .aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa();");
  verifyFormat(
      "aaaaaaaaaaaaaaaaaaaa(aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(\n"
      "    aaaaaaaaaaaaaaaaaa, aaaaaaaaaaaaaaaaa));");
}

TEST_F(FormatTest, WrapsTemplateDeclarations) {
  verifyFormat("template <typename T>\n"
               "virtual void loooooooooooongFunction(int Param1, int Param2);");
  verifyFormat(
      "template <typename T>\n"
      "using comment_to_xml_conversion = comment_to_xml_conversion<T, int>;");
  verifyFormat("template <typename T>\n"
               "void f(int Paaaaaaaaaaaaaaaaaaaaaaaaaaaaaaram1,\n"
               "       int Paaaaaaaaaaaaaaaaaaaaaaaaaaaaaaram2);");
  verifyFormat(
      "template <typename T>\n"
      "void looooooooooooooooooooongFunction(int Paaaaaaaaaaaaaaaaaaaaram1,\n"
      "                                      int Paaaaaaaaaaaaaaaaaaaaram2);");
  verifyFormat(
      "template <typename T>\n"
      "aaaaaaaaaaaaaaaaaaa(aaaaaaaaaaaaaaaaaa,\n"
      "                    aaaaaaaaaaaaaaaaaaaaaaaaaa<T>::aaaaaaaaaa,\n"
      "                    aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa);");
  verifyFormat("template <typename T>\n"
               "void aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(\n"
               "    int aaaaaaaaaaaaaaaaaaaaaa);");
  verifyFormat(
      "template <typename T1, typename T2 = char, typename T3 = char,\n"
      "          typename T4 = char>\n"
      "void f();");
  verifyFormat(
      "aaaaaaaaaaaaaaaaaaaaaaaa<aaaaaaaaaaaaaaaaa, aaaaaaaaaaaaaaaaa>(\n"
      "    aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa);");

  verifyFormat("a<aaaaaaaaaaaaaaaaaaaaa, aaaaaaaaaaaaaaaaaaa>(\n"
               "    a(aaaaaaaaaaaaaaaaaa, aaaaaaaaaaaaaaaa));");

  verifyFormat("template <typename T> class C {};");
  verifyFormat("template <typename T> void f();");
  verifyFormat("template <typename T> void f() {}");

  FormatStyle AlwaysBreak = getLLVMStyle();
  AlwaysBreak.AlwaysBreakTemplateDeclarations = true;
  verifyFormat("template <typename T>\nclass C {};", AlwaysBreak);
  verifyFormat("template <typename T>\nvoid f();", AlwaysBreak);
  verifyFormat("template <typename T>\nvoid f() {}", AlwaysBreak);
  verifyFormat("void aaaaaaaaaaaaaaaaaaa<aaaaaaaaaaaaaaaaaaaaaaaaaaaaa,\n"
               "                         bbbbbbbbbbbbbbbbbbbbbbbbbbbb>(\n"
               "    ccccccccccccccccccccccccccccccccccccccccccccccc);");
  verifyFormat(
      "aaaaaaaaaaaaa<aaaaaaaaaa, aaaaaaaaaaa,\n"
      "              aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,\n"
      "              aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa> *aaaa =\n"
      "    new aaaaaaaaaaaaa<aaaaaaaaaa, aaaaaaaaaaa,\n"
      "                      aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,\n"
      "                      aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa>(\n"
      "        bbbbbbbbbbbbbbbbbbbbbbbb);",
      getLLVMStyleWithColumns(72));
}

TEST_F(FormatTest, WrapsAtNestedNameSpecifiers) {
  verifyFormat(
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa::\n"
      "    aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa();");
  verifyFormat(
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa::\n"
      "    aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(\n"
      "        aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa());");

  // FIXME: Should we have the extra indent after the second break?
  verifyFormat(
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa::\n"
      "    aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa::\n"
      "        aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa();");

  verifyFormat(
      "aaaaaaaaaaaaaaa(bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb::\n"
      "                    cccccccccccccccccccccccccccccccccccccccccccccc());");

  // Breaking at nested name specifiers is generally not desirable.
  verifyFormat(
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa::aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(\n"
      "    aaaaaaaaaaaaaaaaaaaaaaa);");

  verifyFormat(
      "aaaaaaaaaaaaaaaaaa(aaaaaaaa, aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa::\n"
      "                                 aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,\n"
      "                   aaaaaaaaaaaaaaaaaaaaa);",
      getLLVMStyleWithColumns(74));

  verifyFormat("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa::\n"
               "    aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
               "        .aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa();");
}

TEST_F(FormatTest, UnderstandsTemplateParameters) {
  verifyFormat("A<int> a;");
  verifyFormat("A<A<A<int> > > a;");
  verifyFormat("A<A<A<int, 2>, 3>, 4> a;");
  verifyFormat("bool x = a < 1 || 2 > a;");
  verifyFormat("bool x = 5 < f<int>();");
  verifyFormat("bool x = f<int>() > 5;");
  verifyFormat("bool x = 5 < a<int>::x;");
  verifyFormat("bool x = a < 4 ? a > 2 : false;");
  verifyFormat("bool x = f() ? a < 2 : a > 2;");

  verifyGoogleFormat("A<A<int>> a;");
  verifyGoogleFormat("A<A<A<int>>> a;");
  verifyGoogleFormat("A<A<A<A<int>>>> a;");
  verifyGoogleFormat("A<A<int> > a;");
  verifyGoogleFormat("A<A<A<int> > > a;");
  verifyGoogleFormat("A<A<A<A<int> > > > a;");
  EXPECT_EQ("A<A<A<A>>> a;", format("A<A<A<A> >> a;", getGoogleStyle()));
  EXPECT_EQ("A<A<A<A>>> a;", format("A<A<A<A>> > a;", getGoogleStyle()));

  verifyFormat("test >> a >> b;");
  verifyFormat("test << a >> b;");

  verifyFormat("f<int>();");
  verifyFormat("template <typename T> void f() {}");

  // Not template parameters.
  verifyFormat("return a < b && c > d;");
  verifyFormat("void f() {\n"
               "  while (a < b && c > d) {\n"
               "  }\n"
               "}");
  verifyFormat("template <typename... Types>\n"
               "typename enable_if<0 < sizeof...(Types)>::type Foo() {}");
}

TEST_F(FormatTest, UnderstandsBinaryOperators) {
  verifyFormat("COMPARE(a, ==, b);");
}

TEST_F(FormatTest, UnderstandsPointersToMembers) {
  verifyFormat("int A::*x;");
  verifyFormat("int (S::*func)(void *);");
  verifyFormat("void f() { int (S::*func)(void *); }");
  verifyFormat("typedef bool *(Class::*Member)() const;");
  verifyFormat("void f() {\n"
               "  (a->*f)();\n"
               "  a->*x;\n"
               "  (a.*f)();\n"
               "  ((*a).*f)();\n"
               "  a.*x;\n"
               "}");
  verifyFormat("void f() {\n"
               "  (a->*aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa)(\n"
               "      aaaa, bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb);\n"
               "}");
  FormatStyle Style = getLLVMStyle();
  Style.PointerBindsToType = true;
  verifyFormat("typedef bool* (Class::*Member)() const;", Style);
}

TEST_F(FormatTest, UnderstandsUnaryOperators) {
  verifyFormat("int a = -2;");
  verifyFormat("f(-1, -2, -3);");
  verifyFormat("a[-1] = 5;");
  verifyFormat("int a = 5 + -2;");
  verifyFormat("if (i == -1) {\n}");
  verifyFormat("if (i != -1) {\n}");
  verifyFormat("if (i > -1) {\n}");
  verifyFormat("if (i < -1) {\n}");
  verifyFormat("++(a->f());");
  verifyFormat("--(a->f());");
  verifyFormat("(a->f())++;");
  verifyFormat("a[42]++;");
  verifyFormat("if (!(a->f())) {\n}");

  verifyFormat("a-- > b;");
  verifyFormat("b ? -a : c;");
  verifyFormat("n * sizeof char16;");
  verifyFormat("n * alignof char16;", getGoogleStyle());
  verifyFormat("sizeof(char);");
  verifyFormat("alignof(char);", getGoogleStyle());

  verifyFormat("return -1;");
  verifyFormat("switch (a) {\n"
               "case -1:\n"
               "  break;\n"
               "}");
  verifyFormat("#define X -1");
  verifyFormat("#define X -kConstant");

  verifyFormat("const NSPoint kBrowserFrameViewPatternOffset = { -5, +3 };");
  verifyFormat("const NSPoint kBrowserFrameViewPatternOffset = { +5, -3 };");

  verifyFormat("int a = /* confusing comment */ -1;");
  // FIXME: The space after 'i' is wrong, but hopefully, this is a rare case.
  verifyFormat("int a = i /* confusing comment */++;");
}

TEST_F(FormatTest, UndestandsOverloadedOperators) {
  verifyFormat("bool operator<();");
  verifyFormat("bool operator>();");
  verifyFormat("bool operator=();");
  verifyFormat("bool operator==();");
  verifyFormat("bool operator!=();");
  verifyFormat("int operator+();");
  verifyFormat("int operator++();");
  verifyFormat("bool operator();");
  verifyFormat("bool operator()();");
  verifyFormat("bool operator[]();");
  verifyFormat("operator bool();");
  verifyFormat("operator int();");
  verifyFormat("operator void *();");
  verifyFormat("operator SomeType<int>();");
  verifyFormat("operator SomeType<int, int>();");
  verifyFormat("operator SomeType<SomeType<int> >();");
  verifyFormat("void *operator new(std::size_t size);");
  verifyFormat("void *operator new[](std::size_t size);");
  verifyFormat("void operator delete(void *ptr);");
  verifyFormat("void operator delete[](void *ptr);");
  verifyFormat("template <typename AAAAAAA, typename BBBBBBB>\n"
               "AAAAAAA operator/(const AAAAAAA &a, BBBBBBB &b);");

  verifyFormat(
      "ostream &operator<<(ostream &OutputStream,\n"
      "                    SomeReallyLongType WithSomeReallyLongValue);");
  verifyFormat("bool operator<(const aaaaaaaaaaaaaaaaaaaaa &left,\n"
               "               const aaaaaaaaaaaaaaaaaaaaa &right) {\n"
               "  return left.group < right.group;\n"
               "}");
  verifyFormat("SomeType &operator=(const SomeType &S);");

  verifyGoogleFormat("operator void*();");
  verifyGoogleFormat("operator SomeType<SomeType<int>>();");
}

TEST_F(FormatTest, UnderstandsNewAndDelete) {
  verifyFormat("void f() {\n"
               "  A *a = new A;\n"
               "  A *a = new (placement) A;\n"
               "  delete a;\n"
               "  delete (A *)a;\n"
               "}");
}

TEST_F(FormatTest, UnderstandsUsesOfStarAndAmp) {
  verifyFormat("int *f(int *a) {}");
  verifyFormat("int main(int argc, char **argv) {}");
  verifyFormat("Test::Test(int b) : a(b * b) {}");
  verifyIndependentOfContext("f(a, *a);");
  verifyFormat("void g() { f(*a); }");
  verifyIndependentOfContext("int a = b * 10;");
  verifyIndependentOfContext("int a = 10 * b;");
  verifyIndependentOfContext("int a = b * c;");
  verifyIndependentOfContext("int a += b * c;");
  verifyIndependentOfContext("int a -= b * c;");
  verifyIndependentOfContext("int a *= b * c;");
  verifyIndependentOfContext("int a /= b * c;");
  verifyIndependentOfContext("int a = *b;");
  verifyIndependentOfContext("int a = *b * c;");
  verifyIndependentOfContext("int a = b * *c;");
  verifyIndependentOfContext("return 10 * b;");
  verifyIndependentOfContext("return *b * *c;");
  verifyIndependentOfContext("return a & ~b;");
  verifyIndependentOfContext("f(b ? *c : *d);");
  verifyIndependentOfContext("int a = b ? *c : *d;");
  verifyIndependentOfContext("*b = a;");
  verifyIndependentOfContext("a * ~b;");
  verifyIndependentOfContext("a * !b;");
  verifyIndependentOfContext("a * +b;");
  verifyIndependentOfContext("a * -b;");
  verifyIndependentOfContext("a * ++b;");
  verifyIndependentOfContext("a * --b;");
  verifyIndependentOfContext("a[4] * b;");
  verifyIndependentOfContext("a[a * a] = 1;");
  verifyIndependentOfContext("f() * b;");
  verifyIndependentOfContext("a * [self dostuff];");
  verifyIndependentOfContext("int x = a * (a + b);");
  verifyIndependentOfContext("(a *)(a + b);");
  verifyIndependentOfContext("int *pa = (int *)&a;");
  verifyIndependentOfContext("return sizeof(int **);");
  verifyIndependentOfContext("return sizeof(int ******);");
  verifyIndependentOfContext("return (int **&)a;");
  verifyIndependentOfContext("f((*PointerToArray)[10]);");
  verifyFormat("void f(Type (*parameter)[10]) {}");
  verifyGoogleFormat("return sizeof(int**);");
  verifyIndependentOfContext("Type **A = static_cast<Type **>(P);");
  verifyGoogleFormat("Type** A = static_cast<Type**>(P);");
  verifyFormat("auto a = [](int **&, int ***) {};");

  verifyIndependentOfContext("InvalidRegions[*R] = 0;");

  verifyIndependentOfContext("A<int *> a;");
  verifyIndependentOfContext("A<int **> a;");
  verifyIndependentOfContext("A<int *, int *> a;");
  verifyIndependentOfContext(
      "const char *const p = reinterpret_cast<const char *const>(q);");
  verifyIndependentOfContext("A<int **, int **> a;");
  verifyIndependentOfContext("void f(int *a = d * e, int *b = c * d);");
  verifyFormat("for (char **a = b; *a; ++a) {\n}");
  verifyFormat("for (; a && b;) {\n}");

  verifyFormat(
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(\n"
      "    aaaaaaaaaaaaaaaaaaaaaaaaaaaa, *aaaaaaaaaaaaaaaaaaaaaaaaaaaaa);");

  verifyGoogleFormat("int main(int argc, char** argv) {}");
  verifyGoogleFormat("A<int*> a;");
  verifyGoogleFormat("A<int**> a;");
  verifyGoogleFormat("A<int*, int*> a;");
  verifyGoogleFormat("A<int**, int**> a;");
  verifyGoogleFormat("f(b ? *c : *d);");
  verifyGoogleFormat("int a = b ? *c : *d;");
  verifyGoogleFormat("Type* t = **x;");
  verifyGoogleFormat("Type* t = *++*x;");
  verifyGoogleFormat("*++*x;");
  verifyGoogleFormat("Type* t = const_cast<T*>(&*x);");
  verifyGoogleFormat("Type* t = x++ * y;");
  verifyGoogleFormat(
      "const char* const p = reinterpret_cast<const char* const>(q);");

  verifyIndependentOfContext("a = *(x + y);");
  verifyIndependentOfContext("a = &(x + y);");
  verifyIndependentOfContext("*(x + y).call();");
  verifyIndependentOfContext("&(x + y)->call();");
  verifyFormat("void f() { &(*I).first; }");

  verifyIndependentOfContext("f(b * /* confusing comment */ ++c);");
  verifyFormat(
      "int *MyValues = {\n"
      "  *A, // Operator detection might be confused by the '{'\n"
      "  *BB // Operator detection might be confused by previous comment\n"
      "};");

  verifyIndependentOfContext("if (int *a = &b)");
  verifyIndependentOfContext("if (int &a = *b)");
  verifyIndependentOfContext("if (a & b[i])");
  verifyIndependentOfContext("if (a::b::c::d & b[i])");
  verifyIndependentOfContext("if (*b[i])");
  verifyIndependentOfContext("if (int *a = (&b))");
  verifyIndependentOfContext("while (int *a = &b)");
  verifyFormat("void f() {\n"
               "  for (const int &v : Values) {\n"
               "  }\n"
               "}");
  verifyFormat("for (int i = a * a; i < 10; ++i) {\n}");
  verifyFormat("for (int i = 0; i < a * a; ++i) {\n}");

  verifyFormat("#define MACRO     \\\n"
               "  int *i = a * b; \\\n"
               "  void f(a *b);",
               getLLVMStyleWithColumns(19));

  verifyIndependentOfContext("A = new SomeType *[Length];");
  verifyIndependentOfContext("A = new SomeType *[Length]();");
  verifyIndependentOfContext("T **t = new T *;");
  verifyIndependentOfContext("T **t = new T *();");
  verifyGoogleFormat("A = new SomeType* [Length]();");
  verifyGoogleFormat("A = new SomeType* [Length];");
  verifyGoogleFormat("T** t = new T*;");
  verifyGoogleFormat("T** t = new T*();");

  FormatStyle PointerLeft = getLLVMStyle();
  PointerLeft.PointerBindsToType = true;
  verifyFormat("delete *x;", PointerLeft);
}

TEST_F(FormatTest, UnderstandsAttributes) {
  verifyFormat("SomeType s __attribute__((unused)) (InitValue);");
}

TEST_F(FormatTest, UnderstandsEllipsis) {
  verifyFormat("int printf(const char *fmt, ...);");
  verifyFormat("template <class... Ts> void Foo(Ts... ts) { Foo(ts...); }");
  verifyFormat("template <class... Ts> void Foo(Ts *... ts) {}");

  FormatStyle PointersLeft = getLLVMStyle();
  PointersLeft.PointerBindsToType = true;
  verifyFormat("template <class... Ts> void Foo(Ts*... ts) {}", PointersLeft);
}

TEST_F(FormatTest, AdaptivelyFormatsPointersAndReferences) {
  EXPECT_EQ("int *a;\n"
            "int *a;\n"
            "int *a;",
            format("int *a;\n"
                   "int* a;\n"
                   "int *a;",
                   getGoogleStyle()));
  EXPECT_EQ("int* a;\n"
            "int* a;\n"
            "int* a;",
            format("int* a;\n"
                   "int* a;\n"
                   "int *a;",
                   getGoogleStyle()));
  EXPECT_EQ("int *a;\n"
            "int *a;\n"
            "int *a;",
            format("int *a;\n"
                   "int * a;\n"
                   "int *  a;",
                   getGoogleStyle()));
}

TEST_F(FormatTest, UnderstandsRvalueReferences) {
  verifyFormat("int f(int &&a) {}");
  verifyFormat("int f(int a, char &&b) {}");
  verifyFormat("void f() { int &&a = b; }");
  verifyGoogleFormat("int f(int a, char&& b) {}");
  verifyGoogleFormat("void f() { int&& a = b; }");

  verifyIndependentOfContext("A<int &&> a;");
  verifyIndependentOfContext("A<int &&, int &&> a;");
  verifyGoogleFormat("A<int&&> a;");
  verifyGoogleFormat("A<int&&, int&&> a;");

  // Not rvalue references:
  verifyFormat("template <bool B, bool C> class A {\n"
               "  static_assert(B && C, \"Something is wrong\");\n"
               "};");
  verifyGoogleFormat("#define IF(a, b, c) if (a && (b == c))");
  verifyGoogleFormat("#define WHILE(a, b, c) while (a && (b == c))");
}

TEST_F(FormatTest, FormatsBinaryOperatorsPrecedingEquals) {
  verifyFormat("void f() {\n"
               "  x[aaaaaaaaa -\n"
               "    b] = 23;\n"
               "}",
               getLLVMStyleWithColumns(15));
}

TEST_F(FormatTest, FormatsCasts) {
  verifyFormat("Type *A = static_cast<Type *>(P);");
  verifyFormat("Type *A = (Type *)P;");
  verifyFormat("Type *A = (vector<Type *, int *>)P;");
  verifyFormat("int a = (int)(2.0f);");
  verifyFormat("int a = (int)2.0f;");
  verifyFormat("x[(int32)y];");
  verifyFormat("x = (int32)y;");
  verifyFormat("#define AA(X) sizeof(((X *)NULL)->a)");
  verifyFormat("int a = (int)*b;");
  verifyFormat("int a = (int)2.0f;");
  verifyFormat("int a = (int)~0;");
  verifyFormat("int a = (int)++a;");
  verifyFormat("int a = (int)sizeof(int);");
  verifyFormat("int a = (int)+2;");
  verifyFormat("my_int a = (my_int)2.0f;");
  verifyFormat("my_int a = (my_int)sizeof(int);");
  verifyFormat("return (my_int)aaa;");
  verifyFormat("#define x ((int)-1)");
  verifyFormat("#define p(q) ((int *)&q)");

  // FIXME: Without type knowledge, this can still fall apart miserably.
  verifyFormat("void f() { my_int a = (my_int) * b; }");
  verifyFormat("void f() { return P ? (my_int) * P : (my_int)0; }");
  verifyFormat("my_int a = (my_int) ~0;");
  verifyFormat("my_int a = (my_int)++ a;");
  verifyFormat("my_int a = (my_int) + 2;");

  // Don't break after a cast's
  verifyFormat("int aaaaaaaaaaaaaaaaaaaaaaaaaaa =\n"
               "    (aaaaaaaaaaaaaaaaaaaaaaaaaa *)(aaaaaaaaaaaaaaaaaaaaaa +\n"
               "                                   bbbbbbbbbbbbbbbbbbbbbb);");

  // These are not casts.
  verifyFormat("void f(int *) {}");
  verifyFormat("f(foo)->b;");
  verifyFormat("f(foo).b;");
  verifyFormat("f(foo)(b);");
  verifyFormat("f(foo)[b];");
  verifyFormat("[](foo) { return 4; }(bar)];");
  verifyFormat("(*funptr)(foo)[4];");
  verifyFormat("funptrs[4](foo)[4];");
  verifyFormat("void f(int *);");
  verifyFormat("void f(int *) = 0;");
  verifyFormat("void f(SmallVector<int>) {}");
  verifyFormat("void f(SmallVector<int>);");
  verifyFormat("void f(SmallVector<int>) = 0;");
  verifyFormat("void f(int i = (kValue) * kMask) {}");
  verifyFormat("void f(int i = (kA * kB) & kMask) {}");
  verifyFormat("int a = sizeof(int) * b;");
  verifyFormat("int a = alignof(int) * b;", getGoogleStyle());
  verifyFormat("template <> void f<int>(int i) SOME_ANNOTATION;");
  verifyFormat("f(\"%\" SOME_MACRO(ll) \"d\");");
  verifyFormat("aaaaa &operator=(const aaaaa &) LLVM_DELETED_FUNCTION;");

  // These are not casts, but at some point were confused with casts.
  verifyFormat("virtual void foo(int *) override;");
  verifyFormat("virtual void foo(char &) const;");
  verifyFormat("virtual void foo(int *a, char *) const;");
  verifyFormat("int a = sizeof(int *) + b;");
  verifyFormat("int a = alignof(int *) + b;", getGoogleStyle());

  verifyFormat("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa *foo = (aaaaaaaaaaaaaaaaa *)\n"
               "    bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb;");
  // FIXME: The indentation here is not ideal.
  verifyFormat(
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
      "    [bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb] = (*cccccccccccccccc)\n"
      "        [dddddddddddddddddddddddddddddddddddddddddddddddddddddddd];");
}

TEST_F(FormatTest, FormatsFunctionTypes) {
  verifyFormat("A<bool()> a;");
  verifyFormat("A<SomeType()> a;");
  verifyFormat("A<void (*)(int, std::string)> a;");
  verifyFormat("A<void *(int)>;");
  verifyFormat("void *(*a)(int *, SomeType *);");
  verifyFormat("int (*func)(void *);");
  verifyFormat("void f() { int (*func)(void *); }");
  verifyFormat("template <class CallbackClass>\n"
               "using MyCallback = void (CallbackClass::*)(SomeObject *Data);");

  verifyGoogleFormat("A<void*(int*, SomeType*)>;");
  verifyGoogleFormat("void* (*a)(int);");
  verifyGoogleFormat(
      "template <class CallbackClass>\n"
      "using MyCallback = void (CallbackClass::*)(SomeObject* Data);");

  // Other constructs can look somewhat like function types:
  verifyFormat("A<sizeof(*x)> a;");
  verifyFormat("#define DEREF_AND_CALL_F(x) f(*x)");
  verifyFormat("some_var = function(*some_pointer_var)[0];");
  verifyFormat("void f() { function(*some_pointer_var)[0] = 10; }");
}

TEST_F(FormatTest, BreaksLongDeclarations) {
  verifyFormat("typedef LoooooooooooooooooooooooooooooooooooooooongType\n"
               "    AnotherNameForTheLongType;",
               getGoogleStyle());
  verifyFormat("LoooooooooooooooooooooooooooooooooooooooongType\n"
               "    LoooooooooooooooooooooooooooooooooooooooongVariable;",
               getGoogleStyle());
  verifyFormat("LoooooooooooooooooooooooooooooooooooooooongType const\n"
               "    LoooooooooooooooooooooooooooooooooooooooongVariable;",
               getGoogleStyle());
  verifyFormat("LoooooooooooooooooooooooooooooooooooooooongReturnType\n"
               "    LoooooooooooooooooooooooooooooooongFunctionDeclaration();",
               getGoogleStyle());
  verifyFormat("LoooooooooooooooooooooooooooooooooooooooongReturnType\n"
               "LooooooooooooooooooooooooooooooooooongFunctionDefinition() {}");
  verifyFormat("LoooooooooooooooooooooooooooooooooooooooongReturnType const\n"
               "LooooooooooooooooooooooooooooooooooongFunctionDefinition() {}");

  // FIXME: Without the comment, this breaks after "(".
  verifyFormat("LoooooooooooooooooooooooooooooooooooooooongType  // break\n"
               "    (*LoooooooooooooooooooooooooooongFunctionTypeVarialbe)();",
               getGoogleStyle());

  verifyFormat("int *someFunction(int LoooooooooooooooooooongParam1,\n"
               "                  int LoooooooooooooooooooongParam2) {}");
  verifyFormat(
      "TypeSpecDecl *TypeSpecDecl::Create(ASTContext &C, DeclContext *DC,\n"
      "                                   SourceLocation L, IdentifierIn *II,\n"
      "                                   Type *T) {}");
  verifyFormat("ReallyLongReturnType<TemplateParam1, TemplateParam2>\n"
               "ReallyReallyLongFunctionName(\n"
               "    const std::string &SomeParameter,\n"
               "    const SomeType<string, SomeOtherTemplateParameter> &\n"
               "        ReallyReallyLongParameterName,\n"
               "    const SomeType<string, SomeOtherTemplateParameter> &\n"
               "        AnotherLongParameterName) {}");
  verifyFormat("template <typename A>\n"
               "SomeLoooooooooooooooooooooongType<\n"
               "    typename some_namespace::SomeOtherType<A>::Type>\n"
               "Function() {}");

  verifyGoogleFormat(
      "aaaaaaaaaaaaaaaa::aaaaaaaaaaaaaaaa<aaaaaaaaaaaaa, aaaaaaaaaaaa>\n"
      "    aaaaaaaaaaaaaaaaaaaaaaa;");
  verifyGoogleFormat(
      "TypeSpecDecl* TypeSpecDecl::Create(ASTContext& C, DeclContext* DC,\n"
      "                                   SourceLocation L) {}");
  verifyGoogleFormat(
      "some_namespace::LongReturnType\n"
      "long_namespace::SomeVeryLongClass::SomeVeryLongFunction(\n"
      "    int first_long_parameter, int second_parameter) {}");

  verifyGoogleFormat("template <typename T>\n"
                     "aaaaaaaa::aaaaa::aaaaaa<T, aaaaaaaaaaaaaaaaaaaaaaaaa>\n"
                     "aaaaaaaaaaaaaaaaaaaaaaaa<T>::aaaaaaa() {}");
  verifyGoogleFormat("A<A<A>> aaaaaaaaaa(int aaaaaaaaaaaaaaaaaaaaaaaaaaa,\n"
                     "                   int aaaaaaaaaaaaaaaaaaaaaaa);");
}

TEST_F(FormatTest, FormatsArrays) {
  verifyFormat("aaaaaaaaaaaaaaaaaaaaaaaaa[aaaaaaaaaaaaaaaaaaaaaaaaa]\n"
               "                         [bbbbbbbbbbbbbbbbbbbbbbbbb] = c;");
  verifyFormat("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
               "    [bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb] = ccccccccccc;");
  verifyFormat("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
               "    [a][bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb] = cccccccc;");
  verifyFormat("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
               "    [aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa]\n"
               "    [bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb] = ccccccccccc;");
  verifyFormat(
      "llvm::outs() << \"aaaaaaaaaaaa: \"\n"
      "             << (*aaaaaaaiaaaaaaa)[aaaaaaaaaaaaaaaaaaaaaaaaa]\n"
      "                                  [aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa];");
}

TEST_F(FormatTest, LineStartsWithSpecialCharacter) {
  verifyFormat("(a)->b();");
  verifyFormat("--a;");
}

TEST_F(FormatTest, HandlesIncludeDirectives) {
  verifyFormat("#include <string>\n"
               "#include <a/b/c.h>\n"
               "#include \"a/b/string\"\n"
               "#include \"string.h\"\n"
               "#include \"string.h\"\n"
               "#include <a-a>\n"
               "#include < path with space >\n"
               "#include \"abc.h\" // this is included for ABC\n"
               "#include \"some long include\" // with a comment\n"
               "#include \"some very long include paaaaaaaaaaaaaaaaaaaaaaath\"",
               getLLVMStyleWithColumns(35));

  verifyFormat("#import <string>");
  verifyFormat("#import <a/b/c.h>");
  verifyFormat("#import \"a/b/string\"");
  verifyFormat("#import \"string.h\"");
  verifyFormat("#import \"string.h\"");
}

//===----------------------------------------------------------------------===//
// Error recovery tests.
//===----------------------------------------------------------------------===//

TEST_F(FormatTest, IncompleteParameterLists) {
  FormatStyle NoBinPacking = getLLVMStyle();
  NoBinPacking.BinPackParameters = false;
  verifyFormat("void aaaaaaaaaaaaaaaaaa(int level,\n"
               "                        double *min_x,\n"
               "                        double *max_x,\n"
               "                        double *min_y,\n"
               "                        double *max_y,\n"
               "                        double *min_z,\n"
               "                        double *max_z, ) {}",
               NoBinPacking);
}

TEST_F(FormatTest, IncorrectCodeTrailingStuff) {
  verifyFormat("void f() { return; }\n42");
  verifyFormat("void f() {\n"
               "  if (0)\n"
               "    return;\n"
               "}\n"
               "42");
  verifyFormat("void f() { return }\n42");
  verifyFormat("void f() {\n"
               "  if (0)\n"
               "    return\n"
               "}\n"
               "42");
}

TEST_F(FormatTest, IncorrectCodeMissingSemicolon) {
  EXPECT_EQ("void f() { return }", format("void  f ( )  {  return  }"));
  EXPECT_EQ("void f() {\n"
            "  if (a)\n"
            "    return\n"
            "}",
            format("void  f  (  )  {  if  ( a )  return  }"));
  EXPECT_EQ("namespace N {\n"
            "void f()\n"
            "}",
            format("namespace  N  {  void f()  }"));
  EXPECT_EQ("namespace N {\n"
            "void f() {}\n"
            "void g()\n"
            "}",
            format("namespace N  { void f( ) { } void g( ) }"));
}

TEST_F(FormatTest, IndentationWithinColumnLimitNotPossible) {
  verifyFormat("int aaaaaaaa =\n"
               "    // Overlylongcomment\n"
               "    b;",
               getLLVMStyleWithColumns(20));
  verifyFormat("function(\n"
               "    ShortArgument,\n"
               "    LoooooooooooongArgument);\n",
               getLLVMStyleWithColumns(20));
}

TEST_F(FormatTest, IncorrectAccessSpecifier) {
  verifyFormat("public:");
  verifyFormat("class A {\n"
               "public\n"
               "  void f() {}\n"
               "};");
  verifyFormat("public\n"
               "int qwerty;");
  verifyFormat("public\n"
               "B {}");
  verifyFormat("public\n"
               "{}");
  verifyFormat("public\n"
               "B { int x; }");
}

TEST_F(FormatTest, IncorrectCodeUnbalancedBraces) {
  verifyFormat("{");
  verifyFormat("#})");
}

TEST_F(FormatTest, IncorrectCodeDoNoWhile) {
  verifyFormat("do {\n}");
  verifyFormat("do {\n}\n"
               "f();");
  verifyFormat("do {\n}\n"
               "wheeee(fun);");
  verifyFormat("do {\n"
               "  f();\n"
               "}");
}

TEST_F(FormatTest, IncorrectCodeMissingParens) {
  verifyFormat("if {\n  foo;\n  foo();\n}");
  verifyFormat("switch {\n  foo;\n  foo();\n}");
  verifyFormat("for {\n  foo;\n  foo();\n}");
  verifyFormat("while {\n  foo;\n  foo();\n}");
  verifyFormat("do {\n  foo;\n  foo();\n} while;");
}

TEST_F(FormatTest, DoesNotTouchUnwrappedLinesWithErrors) {
  verifyFormat("namespace {\n"
               "class Foo {  Foo  (\n"
               "};\n"
               "} // comment");
}

TEST_F(FormatTest, IncorrectCodeErrorDetection) {
  EXPECT_EQ("{\n{}\n", format("{\n{\n}\n"));
  EXPECT_EQ("{\n  {}\n", format("{\n  {\n}\n"));
  EXPECT_EQ("{\n  {}\n", format("{\n  {\n  }\n"));
  EXPECT_EQ("{\n  {}\n  }\n}\n", format("{\n  {\n    }\n  }\n}\n"));

  EXPECT_EQ("{\n"
            "    {\n"
            " breakme(\n"
            "     qwe);\n"
            "}\n",
            format("{\n"
                   "    {\n"
                   " breakme(qwe);\n"
                   "}\n",
                   getLLVMStyleWithColumns(10)));
}

TEST_F(FormatTest, LayoutCallsInsideBraceInitializers) {
  verifyFormat("int x = {\n"
               "  avariable,\n"
               "  b(alongervariable)\n"
               "};",
               getLLVMStyleWithColumns(25));
}

TEST_F(FormatTest, LayoutBraceInitializersInReturnStatement) {
  verifyFormat("return (a)(b) { 1, 2, 3 };");
}

TEST_F(FormatTest, LayoutCxx11ConstructorBraceInitializers) {
    verifyFormat("vector<int> x{ 1, 2, 3, 4 };");
    verifyFormat("vector<T> x{ {}, {}, {}, {} };");
    verifyFormat("f({ 1, 2 });");
    verifyFormat("auto v = Foo{ 1 };");
    verifyFormat("f({ 1, 2 }, { { 2, 3 }, { 4, 5 } }, c, { d });");
    verifyFormat("Class::Class : member{ 1, 2, 3 } {}");
    verifyFormat("new vector<int>{ 1, 2, 3 };");
    verifyFormat("new int[3]{ 1, 2, 3 };");
    verifyFormat("return { arg1, arg2 };");
    verifyFormat("return { arg1, SomeType{ parameter } };");
    verifyFormat("new T{ arg1, arg2 };");
    verifyFormat("class Class {\n"
                 "  T member = { arg1, arg2 };\n"
                 "};");
    verifyFormat(
        "foo = aaaaaaaaaaa ? vector<int>{ aaaaaaaaaaaaaaaaaaaaaaaaaaa,\n"
        "                                 aaaaaaaaaaaaaaaaaaaa, aaaaa }\n"
        "                  : vector<int>{ bbbbbbbbbbbbbbbbbbbbbbbbbbb,\n"
        "                                 bbbbbbbbbbbbbbbbbbbb, bbbbb };");
    verifyFormat("DoSomethingWithVector({} /* No data */);");
    verifyFormat("DoSomethingWithVector({ {} /* No data */ }, { { 1, 2 } });");
    verifyFormat(
        "someFunction(OtherParam, BracedList{\n"
        "                           // comment 1 (Forcing interesting break)\n"
        "                           param1, param2,\n"
        "                           // comment 2\n"
        "                           param3, param4\n"
        "                         });");

    FormatStyle NoSpaces = getLLVMStyle();
    NoSpaces.Cpp11BracedListStyle = true;
    verifyFormat("vector<int> x{1, 2, 3, 4};", NoSpaces);
    verifyFormat("vector<T> x{{}, {}, {}, {}};", NoSpaces);
    verifyFormat("f({1, 2});", NoSpaces);
    verifyFormat("auto v = Foo{-1};", NoSpaces);
    verifyFormat("f({1, 2}, {{2, 3}, {4, 5}}, c, {d});", NoSpaces);
    verifyFormat("Class::Class : member{1, 2, 3} {}", NoSpaces);
    verifyFormat("new vector<int>{1, 2, 3};", NoSpaces);
    verifyFormat("new int[3]{1, 2, 3};", NoSpaces);
    verifyFormat("return {arg1, arg2};", NoSpaces);
    verifyFormat("return {arg1, SomeType{parameter}};", NoSpaces);
    verifyFormat("new T{arg1, arg2};", NoSpaces);
    verifyFormat("class Class {\n"
                 "  T member = {arg1, arg2};\n"
                 "};",
                 NoSpaces);
}

TEST_F(FormatTest, PullTrivialFunctionDefinitionsIntoSingleLine) {
  verifyFormat("void f() { return 42; }");
  verifyFormat("void f() {\n"
               "  // Comment\n"
               "}");
  verifyFormat("{\n"
               "#error {\n"
               "  int a;\n"
               "}");
  verifyFormat("{\n"
               "  int a;\n"
               "#error {\n"
               "}");
  verifyFormat("void f() {} // comment");
  verifyFormat("void f() { int a; } // comment");
  verifyFormat("void f() {\n"
               "} // comment",
               getLLVMStyleWithColumns(15));

  verifyFormat("void f() { return 42; }", getLLVMStyleWithColumns(23));
  verifyFormat("void f() {\n  return 42;\n}", getLLVMStyleWithColumns(22));

  verifyFormat("void f() {}", getLLVMStyleWithColumns(11));
  verifyFormat("void f() {\n}", getLLVMStyleWithColumns(10));
}

TEST_F(FormatTest, UnderstandContextOfRecordTypeKeywords) {
  // Elaborate type variable declarations.
  verifyFormat("struct foo a = { bar };\nint n;");
  verifyFormat("class foo a = { bar };\nint n;");
  verifyFormat("union foo a = { bar };\nint n;");

  // Elaborate types inside function definitions.
  verifyFormat("struct foo f() {}\nint n;");
  verifyFormat("class foo f() {}\nint n;");
  verifyFormat("union foo f() {}\nint n;");

  // Templates.
  verifyFormat("template <class X> void f() {}\nint n;");
  verifyFormat("template <struct X> void f() {}\nint n;");
  verifyFormat("template <union X> void f() {}\nint n;");

  // Actual definitions...
  verifyFormat("struct {\n} n;");
  verifyFormat(
      "template <template <class T, class Y>, class Z> class X {\n} n;");
  verifyFormat("union Z {\n  int n;\n} x;");
  verifyFormat("class MACRO Z {\n} n;");
  verifyFormat("class MACRO(X) Z {\n} n;");
  verifyFormat("class __attribute__(X) Z {\n} n;");
  verifyFormat("class __declspec(X) Z {\n} n;");
  verifyFormat("class A##B##C {\n} n;");

  // Redefinition from nested context:
  verifyFormat("class A::B::C {\n} n;");

  // Template definitions.
  verifyFormat(
      "template <typename F>\n"
      "Matcher(const Matcher<F> &Other,\n"
      "        typename enable_if_c<is_base_of<F, T>::value &&\n"
      "                             !is_same<F, T>::value>::type * = 0)\n"
      "    : Implementation(new ImplicitCastMatcher<F>(Other)) {}");

  // FIXME: This is still incorrectly handled at the formatter side.
  verifyFormat("template <> struct X < 15, i<3 && 42 < 50 && 33 < 28> {};");

  // FIXME:
  // This now gets parsed incorrectly as class definition.
  // verifyFormat("class A<int> f() {\n}\nint n;");

  // Elaborate types where incorrectly parsing the structural element would
  // break the indent.
  verifyFormat("if (true)\n"
               "  class X x;\n"
               "else\n"
               "  f();\n");

  // This is simply incomplete. Formatting is not important, but must not crash.
  verifyFormat("class A:");
}

TEST_F(FormatTest, DoNotInterfereWithErrorAndWarning) {
  verifyFormat("#error Leave     all         white!!!!! space* alone!\n");
  verifyFormat("#warning Leave     all         white!!!!! space* alone!\n");
  EXPECT_EQ("#error 1", format("  #  error   1"));
  EXPECT_EQ("#warning 1", format("  #  warning 1"));
}

TEST_F(FormatTest, FormatHashIfExpressions) {
  // FIXME: Come up with a better indentation for #elif.
  verifyFormat(
      "#if !defined(AAAAAAA) && (defined CCCCCC || defined DDDDDD) &&  \\\n"
      "    defined(BBBBBBBB)\n"
      "#elif !defined(AAAAAA) && (defined CCCCC || defined DDDDDD) &&  \\\n"
      "    defined(BBBBBBBB)\n"
      "#endif",
      getLLVMStyleWithColumns(65));
}

TEST_F(FormatTest, MergeHandlingInTheFaceOfPreprocessorDirectives) {
  FormatStyle AllowsMergedIf = getGoogleStyle();
  AllowsMergedIf.AllowShortIfStatementsOnASingleLine = true;
  verifyFormat("void f() { f(); }\n#error E", AllowsMergedIf);
  verifyFormat("if (true) return 42;\n#error E", AllowsMergedIf);
  verifyFormat("if (true)\n#error E\n  return 42;", AllowsMergedIf);
  EXPECT_EQ("if (true) return 42;",
            format("if (true)\nreturn 42;", AllowsMergedIf));
  FormatStyle ShortMergedIf = AllowsMergedIf;
  ShortMergedIf.ColumnLimit = 25;
  verifyFormat("#define A \\\n"
               "  if (true) return 42;",
               ShortMergedIf);
  verifyFormat("#define A \\\n"
               "  f();    \\\n"
               "  if (true)\n"
               "#define B",
               ShortMergedIf);
  verifyFormat("#define A \\\n"
               "  f();    \\\n"
               "  if (true)\n"
               "g();",
               ShortMergedIf);
  verifyFormat("{\n"
               "#ifdef A\n"
               "  // Comment\n"
               "  if (true) continue;\n"
               "#endif\n"
               "  // Comment\n"
               "  if (true) continue;",
               ShortMergedIf);
}

TEST_F(FormatTest, BlockCommentsInControlLoops) {
  verifyFormat("if (0) /* a comment in a strange place */ {\n"
               "  f();\n"
               "}");
  verifyFormat("if (0) /* a comment in a strange place */ {\n"
               "  f();\n"
               "} /* another comment */ else /* comment #3 */ {\n"
               "  g();\n"
               "}");
  verifyFormat("while (0) /* a comment in a strange place */ {\n"
               "  f();\n"
               "}");
  verifyFormat("for (;;) /* a comment in a strange place */ {\n"
               "  f();\n"
               "}");
  verifyFormat("do /* a comment in a strange place */ {\n"
               "  f();\n"
               "} /* another comment */ while (0);");
}

TEST_F(FormatTest, BlockComments) {
  EXPECT_EQ("/* */ /* */ /* */\n/* */ /* */ /* */",
            format("/* *//* */  /* */\n/* *//* */  /* */"));
  EXPECT_EQ("/* */ a /* */ b;", format("  /* */  a/* */  b;"));
  EXPECT_EQ("#define A /*123*/\\\n"
            "  b\n"
            "/* */\n"
            "someCall(\n"
            "    parameter);",
            format("#define A /*123*/ b\n"
                   "/* */\n"
                   "someCall(parameter);",
                   getLLVMStyleWithColumns(15)));

  EXPECT_EQ("#define A\n"
            "/* */ someCall(\n"
            "    parameter);",
            format("#define A\n"
                   "/* */someCall(parameter);",
                   getLLVMStyleWithColumns(15)));
  EXPECT_EQ("/*\n**\n*/", format("/*\n**\n*/"));
  EXPECT_EQ("/*\n"
            "*\n"
            " * aaaaaa\n"
            "*aaaaaa\n"
            "*/",
            format("/*\n"
                   "*\n"
                   " * aaaaaa aaaaaa\n"
                   "*/",
                   getLLVMStyleWithColumns(10)));
  EXPECT_EQ("/*\n"
            "**\n"
            "* aaaaaa\n"
            "*aaaaaa\n"
            "*/",
            format("/*\n"
                   "**\n"
                   "* aaaaaa aaaaaa\n"
                   "*/",
                   getLLVMStyleWithColumns(10)));

  FormatStyle NoBinPacking = getLLVMStyle();
  NoBinPacking.BinPackParameters = false;
  EXPECT_EQ("someFunction(1, /* comment 1 */\n"
            "             2, /* comment 2 */\n"
            "             3, /* comment 3 */\n"
            "             aaaa,\n"
            "             bbbb);",
            format("someFunction (1,   /* comment 1 */\n"
                   "                2, /* comment 2 */  \n"
                   "               3,   /* comment 3 */\n"
                   "aaaa, bbbb );",
                   NoBinPacking));
  verifyFormat(
      "bool aaaaaaaaaaaaa = /* comment: */ aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa ||\n"
      "                     aaaaaaaaaaaaaaaaaaaaaaaaaaaa;");
  EXPECT_EQ(
      "bool aaaaaaaaaaaaa = /* trailing comment */\n"
      "    aaaaaaaaaaaaaaaaaaaaaaaaaaa || aaaaaaaaaaaaaaaaaaaaaaaaa ||\n"
      "    aaaaaaaaaaaaaaaaaaaaaaaaaaaa || aaaaaaaaaaaaaaaaaaaaaaaaaa;",
      format(
          "bool       aaaaaaaaaaaaa =       /* trailing comment */\n"
          "    aaaaaaaaaaaaaaaaaaaaaaaaaaa||aaaaaaaaaaaaaaaaaaaaaaaaa    ||\n"
          "    aaaaaaaaaaaaaaaaaaaaaaaaaaaa   || aaaaaaaaaaaaaaaaaaaaaaaaaa;"));
  EXPECT_EQ(
      "int aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa; /* comment */\n"
      "int bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb;   /* comment */\n"
      "int cccccccccccccccccccccccccccccc;       /* comment */\n",
      format("int aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa; /* comment */\n"
             "int      bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb; /* comment */\n"
             "int    cccccccccccccccccccccccccccccc;  /* comment */\n"));

  verifyFormat("void f(int * /* unused */) {}");

  EXPECT_EQ("/*\n"
            " **\n"
            " */",
            format("/*\n"
                   " **\n"
                   " */"));
  EXPECT_EQ("/*\n"
            " *q\n"
            " */",
            format("/*\n"
                   " *q\n"
                   " */"));
  EXPECT_EQ("/*\n"
            " * q\n"
            " */",
            format("/*\n"
                   " * q\n"
                   " */"));
  EXPECT_EQ("/*\n"
            " **/",
            format("/*\n"
                   " **/"));
  EXPECT_EQ("/*\n"
            " ***/",
            format("/*\n"
                   " ***/"));
}

TEST_F(FormatTest, BlockCommentsInMacros) {
  EXPECT_EQ("#define A          \\\n"
            "  {                \\\n"
            "    /* one line */ \\\n"
            "    someCall();",
            format("#define A {        \\\n"
                   "  /* one line */   \\\n"
                   "  someCall();",
                   getLLVMStyleWithColumns(20)));
  EXPECT_EQ("#define A          \\\n"
            "  {                \\\n"
            "    /* previous */ \\\n"
            "    /* one line */ \\\n"
            "    someCall();",
            format("#define A {        \\\n"
                   "  /* previous */   \\\n"
                   "  /* one line */   \\\n"
                   "  someCall();",
                   getLLVMStyleWithColumns(20)));
}

TEST_F(FormatTest, BlockCommentsAtEndOfLine) {
  EXPECT_EQ("a = {\n"
            "  1111 /*    */\n"
            "};",
            format("a = {1111 /*    */\n"
                   "};",
                   getLLVMStyleWithColumns(15)));
  EXPECT_EQ("a = {\n"
            "  1111 /*      */\n"
            "};",
            format("a = {1111 /*      */\n"
                   "};",
                   getLLVMStyleWithColumns(15)));

  // FIXME: The formatting is still wrong here.
  EXPECT_EQ("a = {\n"
            "  1111 /*      a\n"
            "          */\n"
            "};",
            format("a = {1111 /*      a */\n"
                   "};",
                   getLLVMStyleWithColumns(15)));
}

TEST_F(FormatTest, IndentLineCommentsInStartOfBlockAtEndOfFile) {
  // FIXME: This is not what we want...
  verifyFormat("{\n"
               "// a"
               "// b");
}

TEST_F(FormatTest, FormatStarDependingOnContext) {
  verifyFormat("void f(int *a);");
  verifyFormat("void f() { f(fint * b); }");
  verifyFormat("class A {\n  void f(int *a);\n};");
  verifyFormat("class A {\n  int *a;\n};");
  verifyFormat("namespace a {\n"
               "namespace b {\n"
               "class A {\n"
               "  void f() {}\n"
               "  int *a;\n"
               "};\n"
               "}\n"
               "}");
}

TEST_F(FormatTest, SpecialTokensAtEndOfLine) {
  verifyFormat("while");
  verifyFormat("operator");
}

//===----------------------------------------------------------------------===//
// Objective-C tests.
//===----------------------------------------------------------------------===//

TEST_F(FormatTest, FormatForObjectiveCMethodDecls) {
  verifyFormat("- (void)sendAction:(SEL)aSelector to:(BOOL)anObject;");
  EXPECT_EQ("- (NSUInteger)indexOfObject:(id)anObject;",
            format("-(NSUInteger)indexOfObject:(id)anObject;"));
  EXPECT_EQ("- (NSInteger)Mthod1;", format("-(NSInteger)Mthod1;"));
  EXPECT_EQ("+ (id)Mthod2;", format("+(id)Mthod2;"));
  EXPECT_EQ("- (NSInteger)Method3:(id)anObject;",
            format("-(NSInteger)Method3:(id)anObject;"));
  EXPECT_EQ("- (NSInteger)Method4:(id)anObject;",
            format("-(NSInteger)Method4:(id)anObject;"));
  EXPECT_EQ("- (NSInteger)Method5:(id)anObject:(id)AnotherObject;",
            format("-(NSInteger)Method5:(id)anObject:(id)AnotherObject;"));
  EXPECT_EQ("- (id)Method6:(id)A:(id)B:(id)C:(id)D;",
            format("- (id)Method6:(id)A:(id)B:(id)C:(id)D;"));
  EXPECT_EQ(
      "- (void)sendAction:(SEL)aSelector to:(id)anObject forAllCells:(BOOL)flag;",
      format(
          "- (void)sendAction:(SEL)aSelector to:(id)anObject forAllCells:(BOOL)flag;"));

  // Very long objectiveC method declaration.
  verifyFormat("- (NSUInteger)indexOfObject:(id)anObject\n"
               "                    inRange:(NSRange)range\n"
               "                   outRange:(NSRange)out_range\n"
               "                  outRange1:(NSRange)out_range1\n"
               "                  outRange2:(NSRange)out_range2\n"
               "                  outRange3:(NSRange)out_range3\n"
               "                  outRange4:(NSRange)out_range4\n"
               "                  outRange5:(NSRange)out_range5\n"
               "                  outRange6:(NSRange)out_range6\n"
               "                  outRange7:(NSRange)out_range7\n"
               "                  outRange8:(NSRange)out_range8\n"
               "                  outRange9:(NSRange)out_range9;");

  verifyFormat("- (int)sum:(vector<int>)numbers;");
  verifyGoogleFormat("- (void)setDelegate:(id<Protocol>)delegate;");
  // FIXME: In LLVM style, there should be a space in front of a '<' for ObjC
  // protocol lists (but not for template classes):
  //verifyFormat("- (void)setDelegate:(id <Protocol>)delegate;");

  verifyFormat("- (int (*)())foo:(int (*)())f;");
  verifyGoogleFormat("- (int (*)())foo:(int (*)())foo;");

  // If there's no return type (very rare in practice!), LLVM and Google style
  // agree.
  verifyFormat("- foo;");
  verifyFormat("- foo:(int)f;");
  verifyGoogleFormat("- foo:(int)foo;");
}

TEST_F(FormatTest, FormatObjCBlocks) {
  verifyFormat("int (^Block)(int, int);");
  verifyFormat("int (^Block1)(int, int) = ^(int i, int j)");
}

TEST_F(FormatTest, FormatObjCInterface) {
  verifyFormat("@interface Foo : NSObject <NSSomeDelegate> {\n"
               "@public\n"
               "  int field1;\n"
               "@protected\n"
               "  int field2;\n"
               "@private\n"
               "  int field3;\n"
               "@package\n"
               "  int field4;\n"
               "}\n"
               "+ (id)init;\n"
               "@end");

  verifyGoogleFormat("@interface Foo : NSObject<NSSomeDelegate> {\n"
                     " @public\n"
                     "  int field1;\n"
                     " @protected\n"
                     "  int field2;\n"
                     " @private\n"
                     "  int field3;\n"
                     " @package\n"
                     "  int field4;\n"
                     "}\n"
                     "+ (id)init;\n"
                     "@end");

  verifyFormat("@interface /* wait for it */ Foo\n"
               "+ (id)init;\n"
               "// Look, a comment!\n"
               "- (int)answerWith:(int)i;\n"
               "@end");

  verifyFormat("@interface Foo\n"
               "@end\n"
               "@interface Bar\n"
               "@end");

  verifyFormat("@interface Foo : Bar\n"
               "+ (id)init;\n"
               "@end");

  verifyFormat("@interface Foo : /**/ Bar /**/ <Baz, /**/ Quux>\n"
               "+ (id)init;\n"
               "@end");

  verifyGoogleFormat("@interface Foo : Bar<Baz, Quux>\n"
                     "+ (id)init;\n"
                     "@end");

  verifyFormat("@interface Foo (HackStuff)\n"
               "+ (id)init;\n"
               "@end");

  verifyFormat("@interface Foo ()\n"
               "+ (id)init;\n"
               "@end");

  verifyFormat("@interface Foo (HackStuff) <MyProtocol>\n"
               "+ (id)init;\n"
               "@end");

  verifyGoogleFormat("@interface Foo (HackStuff)<MyProtocol>\n"
                     "+ (id)init;\n"
                     "@end");

  verifyFormat("@interface Foo {\n"
               "  int _i;\n"
               "}\n"
               "+ (id)init;\n"
               "@end");

  verifyFormat("@interface Foo : Bar {\n"
               "  int _i;\n"
               "}\n"
               "+ (id)init;\n"
               "@end");

  verifyFormat("@interface Foo : Bar <Baz, Quux> {\n"
               "  int _i;\n"
               "}\n"
               "+ (id)init;\n"
               "@end");

  verifyFormat("@interface Foo (HackStuff) {\n"
               "  int _i;\n"
               "}\n"
               "+ (id)init;\n"
               "@end");

  verifyFormat("@interface Foo () {\n"
               "  int _i;\n"
               "}\n"
               "+ (id)init;\n"
               "@end");

  verifyFormat("@interface Foo (HackStuff) <MyProtocol> {\n"
               "  int _i;\n"
               "}\n"
               "+ (id)init;\n"
               "@end");
}

TEST_F(FormatTest, FormatObjCImplementation) {
  verifyFormat("@implementation Foo : NSObject {\n"
               "@public\n"
               "  int field1;\n"
               "@protected\n"
               "  int field2;\n"
               "@private\n"
               "  int field3;\n"
               "@package\n"
               "  int field4;\n"
               "}\n"
               "+ (id)init {\n}\n"
               "@end");

  verifyGoogleFormat("@implementation Foo : NSObject {\n"
                     " @public\n"
                     "  int field1;\n"
                     " @protected\n"
                     "  int field2;\n"
                     " @private\n"
                     "  int field3;\n"
                     " @package\n"
                     "  int field4;\n"
                     "}\n"
                     "+ (id)init {\n}\n"
                     "@end");

  verifyFormat("@implementation Foo\n"
               "+ (id)init {\n"
               "  if (true)\n"
               "    return nil;\n"
               "}\n"
               "// Look, a comment!\n"
               "- (int)answerWith:(int)i {\n"
               "  return i;\n"
               "}\n"
               "+ (int)answerWith:(int)i {\n"
               "  return i;\n"
               "}\n"
               "@end");

  verifyFormat("@implementation Foo\n"
               "@end\n"
               "@implementation Bar\n"
               "@end");

  verifyFormat("@implementation Foo : Bar\n"
               "+ (id)init {\n}\n"
               "- (void)foo {\n}\n"
               "@end");

  verifyFormat("@implementation Foo {\n"
               "  int _i;\n"
               "}\n"
               "+ (id)init {\n}\n"
               "@end");

  verifyFormat("@implementation Foo : Bar {\n"
               "  int _i;\n"
               "}\n"
               "+ (id)init {\n}\n"
               "@end");

  verifyFormat("@implementation Foo (HackStuff)\n"
               "+ (id)init {\n}\n"
               "@end");
}

TEST_F(FormatTest, FormatObjCProtocol) {
  verifyFormat("@protocol Foo\n"
               "@property(weak) id delegate;\n"
               "- (NSUInteger)numberOfThings;\n"
               "@end");

  verifyFormat("@protocol MyProtocol <NSObject>\n"
               "- (NSUInteger)numberOfThings;\n"
               "@end");

  verifyGoogleFormat("@protocol MyProtocol<NSObject>\n"
                     "- (NSUInteger)numberOfThings;\n"
                     "@end");

  verifyFormat("@protocol Foo;\n"
               "@protocol Bar;\n");

  verifyFormat("@protocol Foo\n"
               "@end\n"
               "@protocol Bar\n"
               "@end");

  verifyFormat("@protocol myProtocol\n"
               "- (void)mandatoryWithInt:(int)i;\n"
               "@optional\n"
               "- (void)optional;\n"
               "@required\n"
               "- (void)required;\n"
               "@optional\n"
               "@property(assign) int madProp;\n"
               "@end\n");

  verifyFormat("@property(nonatomic, assign, readonly)\n"
               "    int *looooooooooooooooooooooooooooongNumber;\n"
               "@property(nonatomic, assign, readonly)\n"
               "    NSString *looooooooooooooooooooooooooooongName;");
}

TEST_F(FormatTest, FormatObjCMethodDeclarations) {
  verifyFormat("- (void)doSomethingWith:(GTMFoo *)theFoo\n"
               "                   rect:(NSRect)theRect\n"
               "               interval:(float)theInterval {\n"
               "}");
  verifyFormat("- (void)shortf:(GTMFoo *)theFoo\n"
               "          longKeyword:(NSRect)theRect\n"
               "    evenLongerKeyword:(float)theInterval\n"
               "                error:(NSError **)theError {\n"
               "}");
}

TEST_F(FormatTest, FormatObjCMethodExpr) {
  verifyFormat("[foo bar:baz];");
  verifyFormat("return [foo bar:baz];");
  verifyFormat("f([foo bar:baz]);");
  verifyFormat("f(2, [foo bar:baz]);");
  verifyFormat("f(2, a ? b : c);");
  verifyFormat("[[self initWithInt:4] bar:[baz quux:arrrr]];");

  // Unary operators.
  verifyFormat("int a = +[foo bar:baz];");
  verifyFormat("int a = -[foo bar:baz];");
  verifyFormat("int a = ![foo bar:baz];");
  verifyFormat("int a = ~[foo bar:baz];");
  verifyFormat("int a = ++[foo bar:baz];");
  verifyFormat("int a = --[foo bar:baz];");
  verifyFormat("int a = sizeof [foo bar:baz];");
  verifyFormat("int a = alignof [foo bar:baz];", getGoogleStyle());
  verifyFormat("int a = &[foo bar:baz];");
  verifyFormat("int a = *[foo bar:baz];");
  // FIXME: Make casts work, without breaking f()[4].
  //verifyFormat("int a = (int)[foo bar:baz];");
  //verifyFormat("return (int)[foo bar:baz];");
  //verifyFormat("(void)[foo bar:baz];");
  verifyFormat("return (MyType *)[self.tableView cellForRowAtIndexPath:cell];");

  // Binary operators.
  verifyFormat("[foo bar:baz], [foo bar:baz];");
  verifyFormat("[foo bar:baz] = [foo bar:baz];");
  verifyFormat("[foo bar:baz] *= [foo bar:baz];");
  verifyFormat("[foo bar:baz] /= [foo bar:baz];");
  verifyFormat("[foo bar:baz] %= [foo bar:baz];");
  verifyFormat("[foo bar:baz] += [foo bar:baz];");
  verifyFormat("[foo bar:baz] -= [foo bar:baz];");
  verifyFormat("[foo bar:baz] <<= [foo bar:baz];");
  verifyFormat("[foo bar:baz] >>= [foo bar:baz];");
  verifyFormat("[foo bar:baz] &= [foo bar:baz];");
  verifyFormat("[foo bar:baz] ^= [foo bar:baz];");
  verifyFormat("[foo bar:baz] |= [foo bar:baz];");
  verifyFormat("[foo bar:baz] ? [foo bar:baz] : [foo bar:baz];");
  verifyFormat("[foo bar:baz] || [foo bar:baz];");
  verifyFormat("[foo bar:baz] && [foo bar:baz];");
  verifyFormat("[foo bar:baz] | [foo bar:baz];");
  verifyFormat("[foo bar:baz] ^ [foo bar:baz];");
  verifyFormat("[foo bar:baz] & [foo bar:baz];");
  verifyFormat("[foo bar:baz] == [foo bar:baz];");
  verifyFormat("[foo bar:baz] != [foo bar:baz];");
  verifyFormat("[foo bar:baz] >= [foo bar:baz];");
  verifyFormat("[foo bar:baz] <= [foo bar:baz];");
  verifyFormat("[foo bar:baz] > [foo bar:baz];");
  verifyFormat("[foo bar:baz] < [foo bar:baz];");
  verifyFormat("[foo bar:baz] >> [foo bar:baz];");
  verifyFormat("[foo bar:baz] << [foo bar:baz];");
  verifyFormat("[foo bar:baz] - [foo bar:baz];");
  verifyFormat("[foo bar:baz] + [foo bar:baz];");
  verifyFormat("[foo bar:baz] * [foo bar:baz];");
  verifyFormat("[foo bar:baz] / [foo bar:baz];");
  verifyFormat("[foo bar:baz] % [foo bar:baz];");
  // Whew!

  verifyFormat("return in[42];");
  verifyFormat("for (id foo in [self getStuffFor:bla]) {\n"
               "}");

  verifyFormat("[self stuffWithInt:(4 + 2) float:4.5];");
  verifyFormat("[self stuffWithInt:a ? b : c float:4.5];");
  verifyFormat("[self stuffWithInt:a ? [self foo:bar] : c];");
  verifyFormat("[self stuffWithInt:a ? (e ? f : g) : c];");
  verifyFormat("[cond ? obj1 : obj2 methodWithParam:param]");
  verifyFormat("[button setAction:@selector(zoomOut:)];");
  verifyFormat("[color getRed:&r green:&g blue:&b alpha:&a];");

  verifyFormat("arr[[self indexForFoo:a]];");
  verifyFormat("throw [self errorFor:a];");
  verifyFormat("@throw [self errorFor:a];");

  verifyFormat("[(id)foo bar:(id)baz quux:(id)snorf];");
  verifyFormat("[(id)foo bar:(id) ? baz : quux];");
  verifyFormat("4 > 4 ? (id)a : (id)baz;");

  // This tests that the formatter doesn't break after "backing" but before ":",
  // which would be at 80 columns.
  verifyFormat(
      "void f() {\n"
      "  if ((self = [super initWithContentRect:contentRect\n"
      "                               styleMask:styleMask ?: otherMask\n"
      "                                 backing:NSBackingStoreBuffered\n"
      "                                   defer:YES]))");

  verifyFormat(
      "[foo checkThatBreakingAfterColonWorksOk:\n"
      "         [bar ifItDoes:reduceOverallLineLengthLikeInThisCase]];");

  verifyFormat("[myObj short:arg1 // Force line break\n"
               "          longKeyword:arg2 != nil ? arg2 : @\"longKeyword\"\n"
               "    evenLongerKeyword:arg3 ?: @\"evenLongerKeyword\"\n"
               "                error:arg4];");
  verifyFormat(
      "void f() {\n"
      "  popup_window_.reset([[RenderWidgetPopupWindow alloc]\n"
      "      initWithContentRect:NSMakeRect(origin_global.x, origin_global.y,\n"
      "                                     pos.width(), pos.height())\n"
      "                styleMask:NSBorderlessWindowMask\n"
      "                  backing:NSBackingStoreBuffered\n"
      "                    defer:NO]);\n"
      "}");
  verifyFormat("[contentsContainer replaceSubview:[subviews objectAtIndex:0]\n"
               "                             with:contentsNativeView];");

  verifyFormat(
      "[pboard addTypes:[NSArray arrayWithObject:kBookmarkButtonDragType]\n"
      "           owner:nillllll];");

  verifyFormat(
      "[pboard setData:[NSData dataWithBytes:&button length:sizeof(button)]\n"
      "        forType:kBookmarkButtonDragType];");

  verifyFormat("[defaultCenter addObserver:self\n"
               "                  selector:@selector(willEnterFullscreen)\n"
               "                      name:kWillEnterFullscreenNotification\n"
               "                    object:nil];");
  verifyFormat("[image_rep drawInRect:drawRect\n"
               "             fromRect:NSZeroRect\n"
               "            operation:NSCompositeCopy\n"
               "             fraction:1.0\n"
               "       respectFlipped:NO\n"
               "                hints:nil];");

  verifyFormat(
      "scoped_nsobject<NSTextField> message(\n"
      "    // The frame will be fixed up when |-setMessageText:| is called.\n"
      "    [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 0, 0)]);");
  verifyFormat("[self aaaaaa:bbbbbbbbbbbbb\n"
               "    aaaaaaaaaa:bbbbbbbbbbbbbbbbb\n"
               "         aaaaa:bbbbbbbbbbb + bbbbbbbbbbbb\n"
               "          aaaa:bbb];");
}

TEST_F(FormatTest, ObjCAt) {
  verifyFormat("@autoreleasepool");
  verifyFormat("@catch");
  verifyFormat("@class");
  verifyFormat("@compatibility_alias");
  verifyFormat("@defs");
  verifyFormat("@dynamic");
  verifyFormat("@encode");
  verifyFormat("@end");
  verifyFormat("@finally");
  verifyFormat("@implementation");
  verifyFormat("@import");
  verifyFormat("@interface");
  verifyFormat("@optional");
  verifyFormat("@package");
  verifyFormat("@private");
  verifyFormat("@property");
  verifyFormat("@protected");
  verifyFormat("@protocol");
  verifyFormat("@public");
  verifyFormat("@required");
  verifyFormat("@selector");
  verifyFormat("@synchronized");
  verifyFormat("@synthesize");
  verifyFormat("@throw");
  verifyFormat("@try");

  EXPECT_EQ("@interface", format("@ interface"));

  // The precise formatting of this doesn't matter, nobody writes code like
  // this.
  verifyFormat("@ /*foo*/ interface");
}

TEST_F(FormatTest, ObjCSnippets) {
  verifyFormat("@autoreleasepool {\n"
               "  foo();\n"
               "}");
  verifyFormat("@class Foo, Bar;");
  verifyFormat("@compatibility_alias AliasName ExistingClass;");
  verifyFormat("@dynamic textColor;");
  verifyFormat("char *buf1 = @encode(int *);");
  verifyFormat("char *buf1 = @encode(typeof(4 * 5));");
  verifyFormat("char *buf1 = @encode(int **);");
  verifyFormat("Protocol *proto = @protocol(p1);");
  verifyFormat("SEL s = @selector(foo:);");
  verifyFormat("@synchronized(self) {\n"
               "  f();\n"
               "}");

  verifyFormat("@synthesize dropArrowPosition = dropArrowPosition_;");
  verifyGoogleFormat("@synthesize dropArrowPosition = dropArrowPosition_;");

  verifyFormat("@property(assign, nonatomic) CGFloat hoverAlpha;");
  verifyFormat("@property(assign, getter=isEditable) BOOL editable;");
  verifyGoogleFormat("@property(assign, getter=isEditable) BOOL editable;");

  verifyFormat("@import foo.bar;\n"
               "@import baz;");
}

TEST_F(FormatTest, ObjCLiterals) {
  verifyFormat("@\"String\"");
  verifyFormat("@1");
  verifyFormat("@+4.8");
  verifyFormat("@-4");
  verifyFormat("@1LL");
  verifyFormat("@.5");
  verifyFormat("@'c'");
  verifyFormat("@true");

  verifyFormat("NSNumber *smallestInt = @(-INT_MAX - 1);");
  verifyFormat("NSNumber *piOverTwo = @(M_PI / 2);");
  verifyFormat("NSNumber *favoriteColor = @(Green);");
  verifyFormat("NSString *path = @(getenv(\"PATH\"));");

  verifyFormat("@[");
  verifyFormat("@[]");
  verifyFormat(
      "NSArray *array = @[ @\" Hey \", NSApp, [NSNumber numberWithInt:42] ];");
  verifyFormat("return @[ @3, @[], @[ @4, @5 ] ];");

  verifyFormat("@{");
  verifyFormat("@{}");
  verifyFormat("@{ @\"one\" : @1 }");
  verifyFormat("return @{ @\"one\" : @1 };");
  verifyFormat("@{ @\"one\" : @1, }");

  verifyFormat("@{ @\"one\" : @{ @2 : @1 } }");
  verifyFormat("@{ @\"one\" : @{ @2 : @1 }, }");

  verifyFormat("@{ 1 > 2 ? @\"one\" : @\"two\" : 1 > 2 ? @1 : @2 }");
  verifyFormat("[self setDict:@{}");
  verifyFormat("[self setDict:@{ @1 : @2 }");
  verifyFormat("NSLog(@\"%@\", @{ @1 : @2, @2 : @3 }[@1]);");
  verifyFormat(
      "NSDictionary *masses = @{ @\"H\" : @1.0078, @\"He\" : @4.0026 };");
  verifyFormat(
      "NSDictionary *settings = @{ AVEncoderKey : @(AVAudioQualityMax) };");

  // FIXME: Nested and multi-line array and dictionary literals need more work.
  verifyFormat(
      "NSDictionary *d = @{ @\"nam\" : NSUserNam(), @\"dte\" : [NSDate date],\n"
      "                     @\"processInfo\" : [NSProcessInfo processInfo] };");
  verifyFormat(
      "@{ NSFontAttributeNameeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee :\n"
      "   regularFont, };");

}

TEST_F(FormatTest, ReformatRegionAdjustsIndent) {
  EXPECT_EQ("{\n"
            "{\n"
            "a;\n"
            "b;\n"
            "}\n"
            "}",
            format("{\n"
                   "{\n"
                   "a;\n"
                   "     b;\n"
                   "}\n"
                   "}",
                   13, 2, getLLVMStyle()));
  EXPECT_EQ("{\n"
            "{\n"
            "  a;\n"
            "b;\n"
            "}\n"
            "}",
            format("{\n"
                   "{\n"
                   "     a;\n"
                   "b;\n"
                   "}\n"
                   "}",
                   9, 2, getLLVMStyle()));
  EXPECT_EQ("{\n"
            "{\n"
            "public:\n"
            "  b;\n"
            "}\n"
            "}",
            format("{\n"
                   "{\n"
                   "public:\n"
                   "     b;\n"
                   "}\n"
                   "}",
                   17, 2, getLLVMStyle()));
  EXPECT_EQ("{\n"
            "{\n"
            "a;\n"
            "}\n"
            "{\n"
            "  b; //\n"
            "}\n"
            "}",
            format("{\n"
                   "{\n"
                   "a;\n"
                   "}\n"
                   "{\n"
                   "           b; //\n"
                   "}\n"
                   "}",
                   22, 2, getLLVMStyle()));
  EXPECT_EQ("  {\n"
            "    a; //\n"
            "  }",
            format("  {\n"
                   "a; //\n"
                   "  }",
                   4, 2, getLLVMStyle()));
  EXPECT_EQ("void f() {}\n"
            "void g() {}",
            format("void f() {}\n"
                   "void g() {}",
                   13, 0, getLLVMStyle()));
  EXPECT_EQ("int a; // comment\n"
            "       // line 2\n"
            "int b;",
            format("int a; // comment\n"
                   "       // line 2\n"
                   "  int b;",
                   35, 0, getLLVMStyle()));
  EXPECT_EQ("  int a;\n"
            "  void\n"
            "  ffffff() {\n"
            "  }",
            format("  int a;\n"
                   "void ffffff() {}",
                   11, 0, getLLVMStyleWithColumns(11)));
}

TEST_F(FormatTest, BreakStringLiterals) {
  EXPECT_EQ("\"some text \"\n"
            "\"other\";",
            format("\"some text other\";", getLLVMStyleWithColumns(12)));
  EXPECT_EQ("\"some text \"\n"
            "\"other\";",
            format("\\\n\"some text other\";", getLLVMStyleWithColumns(12)));
  EXPECT_EQ(
      "#define A  \\\n"
      "  \"some \"  \\\n"
      "  \"text \"  \\\n"
      "  \"other\";",
      format("#define A \"some text other\";", getLLVMStyleWithColumns(12)));
  EXPECT_EQ(
      "#define A  \\\n"
      "  \"so \"    \\\n"
      "  \"text \"  \\\n"
      "  \"other\";",
      format("#define A \"so text other\";", getLLVMStyleWithColumns(12)));

  EXPECT_EQ("\"some text\"",
            format("\"some text\"", getLLVMStyleWithColumns(1)));
  EXPECT_EQ("\"some text\"",
            format("\"some text\"", getLLVMStyleWithColumns(11)));
  EXPECT_EQ("\"some \"\n"
            "\"text\"",
            format("\"some text\"", getLLVMStyleWithColumns(10)));
  EXPECT_EQ("\"some \"\n"
            "\"text\"",
            format("\"some text\"", getLLVMStyleWithColumns(7)));
  EXPECT_EQ("\"some\"\n"
            "\" tex\"\n"
            "\"t\"",
            format("\"some text\"", getLLVMStyleWithColumns(6)));
  EXPECT_EQ("\"some\"\n"
            "\" tex\"\n"
            "\" and\"",
            format("\"some tex and\"", getLLVMStyleWithColumns(6)));
  EXPECT_EQ("\"some\"\n"
            "\"/tex\"\n"
            "\"/and\"",
            format("\"some/tex/and\"", getLLVMStyleWithColumns(6)));

  EXPECT_EQ("variable =\n"
            "    \"long string \"\n"
            "    \"literal\";",
            format("variable = \"long string literal\";",
                   getLLVMStyleWithColumns(20)));

  EXPECT_EQ("variable = f(\n"
            "    \"long string \"\n"
            "    \"literal\",\n"
            "    short,\n"
            "    loooooooooooooooooooong);",
            format("variable = f(\"long string literal\", short, "
                   "loooooooooooooooooooong);",
                   getLLVMStyleWithColumns(20)));

  EXPECT_EQ("f(g(\"long string \"\n"
            "    \"literal\"),\n"
            "  b);",
            format("f(g(\"long string literal\"), b);",
                   getLLVMStyleWithColumns(20)));
  EXPECT_EQ("f(g(\"long string \"\n"
            "    \"literal\",\n"
            "    a),\n"
            "  b);",
            format("f(g(\"long string literal\", a), b);",
                   getLLVMStyleWithColumns(20)));
  EXPECT_EQ(
      "f(\"one two\".split(\n"
      "    variable));",
      format("f(\"one two\".split(variable));", getLLVMStyleWithColumns(20)));
  EXPECT_EQ("f(\"one two three four five six \"\n"
            "  \"seven\".split(\n"
            "      really_looooong_variable));",
            format("f(\"one two three four five six seven\"."
                   "split(really_looooong_variable));",
                   getLLVMStyleWithColumns(33)));

  EXPECT_EQ("f(\"some \"\n"
            "  \"text\",\n"
            "  other);",
            format("f(\"some text\", other);", getLLVMStyleWithColumns(10)));

  // Only break as a last resort.
  verifyFormat(
      "aaaaaaaaaaaaaaaaaaaa(\n"
      "    aaaaaaaaaaaaaaaaaaaa,\n"
      "    aaaaaa(\"aaa aaaaa aaa aaa aaaaa aaa aaaaa aaa aaa aaaaaa\"));");

  EXPECT_EQ(
      "\"splitmea\"\n"
      "\"trandomp\"\n"
      "\"oint\"",
      format("\"splitmeatrandompoint\"", getLLVMStyleWithColumns(10)));

  EXPECT_EQ(
      "\"split/\"\n"
      "\"pathat/\"\n"
      "\"slashes\"",
      format("\"split/pathat/slashes\"", getLLVMStyleWithColumns(10)));

  EXPECT_EQ(
      "\"split/\"\n"
      "\"pathat/\"\n"
      "\"slashes\"",
      format("\"split/pathat/slashes\"", getLLVMStyleWithColumns(10)));
  EXPECT_EQ("\"split at \"\n"
            "\"spaces/at/\"\n"
            "\"slashes.at.any$\"\n"
            "\"non-alphanumeric%\"\n"
            "\"1111111111characte\"\n"
            "\"rs\"",
            format("\"split at "
                   "spaces/at/"
                   "slashes.at."
                   "any$non-"
                   "alphanumeric%"
                   "1111111111characte"
                   "rs\"",
                   getLLVMStyleWithColumns(20)));

  // Verify that splitting the strings understands
  // Style::AlwaysBreakBeforeMultilineStrings.
  EXPECT_EQ("aaaaaaaaaaaa(aaaaaaaaaaaaa,\n"
            "             \"aaaaaaaaaaaaaaaaaaaaaa aaaaaaaaaaaaaaaaaaaaaa \"\n"
            "             \"aaaaaaaaaaaaaaaaaaaaaa aaaaaaaaaaaaaaaaaaaaaa\");",
            format("aaaaaaaaaaaa(aaaaaaaaaaaaa, \"aaaaaaaaaaaaaaaaaaaaaa "
                   "aaaaaaaaaaaaaaaaaaaaaa aaaaaaaaaaaaaaaaaaaaaa "
                   "aaaaaaaaaaaaaaaaaaaaaa\");",
                   getGoogleStyle()));
  EXPECT_EQ("llvm::outs() << \"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa \"\n"
            "                \"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\";",
            format("llvm::outs() << "
                   "\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa aaaaaaaaaaaaaaaaaa"
                   "aaaaaaaaaaaaaaaaaaa\";"));

  FormatStyle AlignLeft = getLLVMStyleWithColumns(12);
  AlignLeft.AlignEscapedNewlinesLeft = true;
  EXPECT_EQ(
      "#define A \\\n"
      "  \"some \" \\\n"
      "  \"text \" \\\n"
      "  \"other\";",
      format("#define A \"some text other\";", AlignLeft));
}

TEST_F(FormatTest, DontSplitStringLiteralsWithEscapedNewlines) {
  EXPECT_EQ("aaaaaaaaaaa =\n"
            "    \"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\\\n"
            "  aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\\\n"
            "  aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\";",
            format("aaaaaaaaaaa = \"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\\\n"
                   "  aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\\\n"
                   "  aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\";"));
}

TEST_F(FormatTest, SkipsUnknownStringLiterals) {
  EXPECT_EQ(
      "u8\"unsupported literal\";",
      format("u8\"unsupported literal\";", getGoogleStyleWithColumns(15)));
  EXPECT_EQ("u\"unsupported literal\";",
            format("u\"unsupported literal\";", getGoogleStyleWithColumns(15)));
  EXPECT_EQ("U\"unsupported literal\";",
            format("U\"unsupported literal\";", getGoogleStyleWithColumns(15)));
  EXPECT_EQ("L\"unsupported literal\";",
            format("L\"unsupported literal\";", getGoogleStyleWithColumns(15)));
  EXPECT_EQ("R\"x(raw literal)x\";",
            format("R\"x(raw literal)x\";", getGoogleStyleWithColumns(15)));
  verifyFormat("string a = \"unterminated;");
  EXPECT_EQ("function(\"unterminated,\n"
            "         OtherParameter);",
            format("function(  \"unterminated,\n"
                   "    OtherParameter);"));
}

TEST_F(FormatTest, DoesNotTryToParseUDLiteralsInPreCpp11Code) {
  EXPECT_EQ("#define x(_a) printf(\"foo\" _a);",
            format("#define x(_a) printf(\"foo\"_a);", getLLVMStyle()));
}

TEST_F(FormatTest, BreakStringLiteralsBeforeUnbreakableTokenSequence) {
  EXPECT_EQ("someFunction(\"aaabbbcccd\"\n"
            "             \"ddeeefff\");",
            format("someFunction(\"aaabbbcccdddeeefff\");",
                   getLLVMStyleWithColumns(25)));
  EXPECT_EQ("someFunction1234567890(\n"
            "    \"aaabbbcccdddeeefff\");",
            format("someFunction1234567890(\"aaabbbcccdddeeefff\");",
                   getLLVMStyleWithColumns(26)));
  EXPECT_EQ("someFunction1234567890(\n"
            "    \"aaabbbcccdddeeeff\"\n"
            "    \"f\");",
            format("someFunction1234567890(\"aaabbbcccdddeeefff\");",
                   getLLVMStyleWithColumns(25)));
  EXPECT_EQ("someFunction1234567890(\n"
            "    \"aaabbbcccdddeeeff\"\n"
            "    \"f\");",
            format("someFunction1234567890(\"aaabbbcccdddeeefff\");",
                   getLLVMStyleWithColumns(24)));
  EXPECT_EQ("someFunction(\n"
            "    \"aaabbbcc \"\n"
            "    \"dddeeefff\");",
            format("someFunction(\"aaabbbcc dddeeefff\");",
                   getLLVMStyleWithColumns(25)));
  EXPECT_EQ("someFunction(\"aaabbbccc \"\n"
            "             \"ddeeefff\");",
            format("someFunction(\"aaabbbccc ddeeefff\");",
                   getLLVMStyleWithColumns(25)));
  EXPECT_EQ("someFunction1234567890(\n"
            "    \"aaabb \"\n"
            "    \"cccdddeeefff\");",
            format("someFunction1234567890(\"aaabb cccdddeeefff\");",
                   getLLVMStyleWithColumns(25)));
  EXPECT_EQ("#define A          \\\n"
            "  string s =       \\\n"
            "      \"123456789\"  \\\n"
            "      \"0\";         \\\n"
            "  int i;",
            format("#define A string s = \"1234567890\"; int i;",
                   getLLVMStyleWithColumns(20)));
}

TEST_F(FormatTest, DoNotBreakStringLiteralsInEscapeSequence) {
  EXPECT_EQ("\"\\a\"",
            format("\"\\a\"", getLLVMStyleWithColumns(3)));
  EXPECT_EQ("\"\\\"",
            format("\"\\\"", getLLVMStyleWithColumns(2)));
  EXPECT_EQ("\"test\"\n"
            "\"\\n\"",
            format("\"test\\n\"", getLLVMStyleWithColumns(7)));
  EXPECT_EQ("\"tes\\\\\"\n"
            "\"n\"",
            format("\"tes\\\\n\"", getLLVMStyleWithColumns(7)));
  EXPECT_EQ("\"\\\\\\\\\"\n"
            "\"\\n\"",
            format("\"\\\\\\\\\\n\"", getLLVMStyleWithColumns(7)));
  EXPECT_EQ("\"\\uff01\"",
            format("\"\\uff01\"", getLLVMStyleWithColumns(7)));
  EXPECT_EQ("\"\\uff01\"\n"
            "\"test\"",
            format("\"\\uff01test\"", getLLVMStyleWithColumns(8)));
  EXPECT_EQ("\"\\Uff01ff02\"",
            format("\"\\Uff01ff02\"", getLLVMStyleWithColumns(11)));
  EXPECT_EQ("\"\\x000000000001\"\n"
            "\"next\"",
            format("\"\\x000000000001next\"", getLLVMStyleWithColumns(16)));
  EXPECT_EQ("\"\\x000000000001next\"",
            format("\"\\x000000000001next\"", getLLVMStyleWithColumns(15)));
  EXPECT_EQ("\"\\x000000000001\"",
            format("\"\\x000000000001\"", getLLVMStyleWithColumns(7)));
  EXPECT_EQ("\"test\"\n"
            "\"\\000000\"\n"
            "\"000001\"",
            format("\"test\\000000000001\"", getLLVMStyleWithColumns(9)));
  EXPECT_EQ("\"test\\000\"\n"
            "\"00000000\"\n"
            "\"1\"",
            format("\"test\\000000000001\"", getLLVMStyleWithColumns(10)));
  EXPECT_EQ("R\"(\\x\\x00)\"\n",
            format("R\"(\\x\\x00)\"\n", getGoogleStyleWithColumns(7)));
}

TEST_F(FormatTest, DoNotCreateUnreasonableUnwrappedLines) {
  verifyFormat("void f() {\n"
               "  return g() {}\n"
               "  void h() {}");
  verifyFormat("if (foo)\n"
               "  return { forgot_closing_brace();\n"
               "test();");
  verifyFormat("int a[] = { void forgot_closing_brace() { f();\n"
               "g();\n"
               "}");
}

TEST_F(FormatTest, FormatsClosingBracesInEmptyNestedBlocks) {
  verifyFormat("class X {\n"
               "  void f() {\n"
               "  }\n"
               "};",
               getLLVMStyleWithColumns(12));
}

TEST_F(FormatTest, ConfigurableIndentWidth) {
  FormatStyle EightIndent = getLLVMStyleWithColumns(18);
  EightIndent.IndentWidth = 8;
  verifyFormat("void f() {\n"
               "        someFunction();\n"
               "        if (true) {\n"
               "                f();\n"
               "        }\n"
               "}",
               EightIndent);
  verifyFormat("class X {\n"
               "        void f() {\n"
               "        }\n"
               "};",
               EightIndent);
  verifyFormat("int x[] = {\n"
               "        call(),\n"
               "        call(),\n"
               "};",
               EightIndent);
}

TEST_F(FormatTest, ConfigurableFunctionDeclarationIndentAfterType) {
  verifyFormat("void\n"
               "f();",
               getLLVMStyleWithColumns(8));
}

TEST_F(FormatTest, ConfigurableUseOfTab) {
  FormatStyle Tab = getLLVMStyleWithColumns(42);
  Tab.IndentWidth = 8;
  Tab.UseTab = true;
  Tab.AlignEscapedNewlinesLeft = true;
  verifyFormat("class X {\n"
               "\tvoid f() {\n"
               "\t\tsomeFunction(parameter1,\n"
               "\t\t\t     parameter2);\n"
               "\t}\n"
               "};",
               Tab);
  verifyFormat("#define A                        \\\n"
               "\tvoid f() {               \\\n"
               "\t\tsomeFunction(    \\\n"
               "\t\t    parameter1,  \\\n"
               "\t\t    parameter2); \\\n"
               "\t}",
               Tab);


  // FIXME: To correctly count mixed whitespace we need to
  // also correctly count mixed whitespace in front of the comment.
  //
  // EXPECT_EQ("/*\n"
  //           "\t      a\t\tcomment\n"
  //           "\t      in multiple lines\n"
  //           "       */",
  //           format("   /*\t \t \n"
  //                  " \t \t a\t\tcomment\t \t\n"
  //                  " \t \t in multiple lines\t\n"
  //                  " \t  */",
  //                  Tab));
  // Tab.UseTab = false;
  // EXPECT_EQ("/*\n"
  //           "              a\t\tcomment\n"
  //           "              in multiple lines\n"
  //           "       */",
  //           format("   /*\t \t \n"
  //                  " \t \t a\t\tcomment\t \t\n"
  //                  " \t \t in multiple lines\t\n"
  //                  " \t  */",
  //                  Tab));
  // EXPECT_EQ("/* some\n"
  //           "   comment */",
  //          format(" \t \t /* some\n"
  //                 " \t \t    comment */",
  //                 Tab));

  EXPECT_EQ("{\n"
            "  /*\n"
            "   * Comment\n"
            "   */\n"
            "  int i;\n"
            "}",
            format("{\n"
                   "\t/*\n"
                   "\t * Comment\n"
                   "\t */\n"
                   "\t int i;\n"
                   "}"));
}

TEST_F(FormatTest, LinuxBraceBreaking) {
  FormatStyle BreakBeforeBrace = getLLVMStyle();
  BreakBeforeBrace.BreakBeforeBraces = FormatStyle::BS_Linux;
  verifyFormat("namespace a\n"
               "{\n"
               "class A\n"
               "{\n"
               "  void f()\n"
               "  {\n"
               "    if (true) {\n"
               "      a();\n"
               "      b();\n"
               "    }\n"
               "  }\n"
               "  void g()\n"
               "  {\n"
               "    return;\n"
               "  }\n"
               "}\n"
               "}",
               BreakBeforeBrace);
}

TEST_F(FormatTest, StroustrupBraceBreaking) {
  FormatStyle BreakBeforeBrace = getLLVMStyle();
  BreakBeforeBrace.BreakBeforeBraces = FormatStyle::BS_Stroustrup;
  verifyFormat("namespace a {\n"
               "class A {\n"
               "  void f()\n"
               "  {\n"
               "    if (true) {\n"
               "      a();\n"
               "      b();\n"
               "    }\n"
               "  }\n"
               "  void g()\n"
               "  {\n"
               "    return;\n"
               "  }\n"
               "}\n"
               "}",
               BreakBeforeBrace);
}

TEST_F(FormatTest, AllmanBraceBreaking) {
  FormatStyle BreakBeforeBrace = getLLVMStyle();
  BreakBeforeBrace.BreakBeforeBraces = FormatStyle::BS_Allman;
  verifyFormat("namespace a\n"
               "{\n"
               "class A\n"
               "{\n"
               "  void f()\n"
               "  {\n"
               "    if (true)\n"
               "    {\n"
               "      a();\n"
               "      b();\n"
               "    }\n"
               "  }\n"
               "  void g()\n"
               "  {\n"
               "    return;\n"
               "  }\n"
               "}\n"
               "}",
               BreakBeforeBrace);

  verifyFormat("void f()\n"
               "{\n"
               "  if (true)\n"
               "  {\n"
               "    a();\n"
               "  }\n"
               "  else if (false)\n"
               "  {\n"
               "    b();\n"
               "  }\n"
               "  else\n"
               "  {\n"
               "    c();\n"
               "  }\n"
               "}\n",
               BreakBeforeBrace);

  verifyFormat("void f()\n"
               "{\n"
               "  for (int i = 0; i < 10; ++i)\n"
               "  {\n"
               "    a();\n"
               "  }\n"
               "  while (false)\n"
               "  {\n"
               "    b();\n"
               "  }\n"
               "  do\n"
               "  {\n"
               "    c();\n"
               "  } while (false)\n"
               "}\n",
               BreakBeforeBrace);

  verifyFormat("void f(int a)\n"
               "{\n"
               "  switch (a)\n"
               "  {\n"
               "  case 0:\n"
               "    break;\n"
               "  case 1:\n"
               "  {\n"
               "    break;\n"
               "  }\n"
               "  case 2:\n"
               "  {\n"
               "  }\n"
               "  break;\n"
               "  default:\n"
               "    break;\n"
               "  }\n"
               "}\n",
               BreakBeforeBrace);

  verifyFormat("enum X\n"
               "{\n"
               "  Y = 0,\n"
               "}\n",
               BreakBeforeBrace);

  FormatStyle BreakBeforeBraceShortIfs = BreakBeforeBrace;
  BreakBeforeBraceShortIfs.AllowShortIfStatementsOnASingleLine = true;
  BreakBeforeBraceShortIfs.AllowShortLoopsOnASingleLine = true;
  verifyFormat("void f(bool b)\n"
               "{\n"
               "  if (b)\n"
               "  {\n"
               "    return;\n"
               "  }\n"
               "}\n",
               BreakBeforeBraceShortIfs);
  verifyFormat("void f(bool b)\n"
               "{\n"
               "  if (b) return;\n"
               "}\n",
               BreakBeforeBraceShortIfs);
  verifyFormat("void f(bool b)\n"
               "{\n"
               "  while (b)\n"
               "  {\n"
               "    return;\n"
               "  }\n"
               "}\n",
               BreakBeforeBraceShortIfs);
}

TEST_F(FormatTest, CatchExceptionReferenceBinding) {
  verifyFormat("void f() {\n"
               "  try {\n"
               "  }\n"
               "  catch (const Exception &e) {\n"
               "  }\n"
               "}\n",
               getLLVMStyle());
}

TEST_F(FormatTest, UnderstandsPragmas) {
  verifyFormat("#pragma omp reduction(| : var)");
  verifyFormat("#pragma omp reduction(+ : var)");
}

bool allStylesEqual(ArrayRef<FormatStyle> Styles) {
  for (size_t i = 1; i < Styles.size(); ++i)
    if (!(Styles[0] == Styles[i]))
      return false;
  return true;
}

TEST_F(FormatTest, GetsPredefinedStyleByName) {
  FormatStyle Styles[3];

  Styles[0] = getLLVMStyle();
  EXPECT_TRUE(getPredefinedStyle("LLVM", &Styles[1]));
  EXPECT_TRUE(getPredefinedStyle("lLvM", &Styles[2]));
  EXPECT_TRUE(allStylesEqual(Styles));

  Styles[0] = getGoogleStyle();
  EXPECT_TRUE(getPredefinedStyle("Google", &Styles[1]));
  EXPECT_TRUE(getPredefinedStyle("gOOgle", &Styles[2]));
  EXPECT_TRUE(allStylesEqual(Styles));

  Styles[0] = getChromiumStyle();
  EXPECT_TRUE(getPredefinedStyle("Chromium", &Styles[1]));
  EXPECT_TRUE(getPredefinedStyle("cHRoMiUM", &Styles[2]));
  EXPECT_TRUE(allStylesEqual(Styles));

  Styles[0] = getMozillaStyle();
  EXPECT_TRUE(getPredefinedStyle("Mozilla", &Styles[1]));
  EXPECT_TRUE(getPredefinedStyle("moZILla", &Styles[2]));
  EXPECT_TRUE(allStylesEqual(Styles));

  Styles[0] = getWebKitStyle();
  EXPECT_TRUE(getPredefinedStyle("WebKit", &Styles[1]));
  EXPECT_TRUE(getPredefinedStyle("wEbKit", &Styles[2]));
  EXPECT_TRUE(allStylesEqual(Styles));

  EXPECT_FALSE(getPredefinedStyle("qwerty", &Styles[0]));
}

TEST_F(FormatTest, ParsesConfiguration) {
  FormatStyle Style = {};
#define CHECK_PARSE(TEXT, FIELD, VALUE)                                        \
  EXPECT_NE(VALUE, Style.FIELD);                                               \
  EXPECT_EQ(0, parseConfiguration(TEXT, &Style).value());                      \
  EXPECT_EQ(VALUE, Style.FIELD)

#define CHECK_PARSE_BOOL(FIELD)                                                \
  Style.FIELD = false;                                                         \
  EXPECT_EQ(0, parseConfiguration(#FIELD ": true", &Style).value());           \
  EXPECT_TRUE(Style.FIELD);                                                    \
  EXPECT_EQ(0, parseConfiguration(#FIELD ": false", &Style).value());          \
  EXPECT_FALSE(Style.FIELD);

  CHECK_PARSE_BOOL(AlignEscapedNewlinesLeft);
  CHECK_PARSE_BOOL(AlignTrailingComments);
  CHECK_PARSE_BOOL(AllowAllParametersOfDeclarationOnNextLine);
  CHECK_PARSE_BOOL(AllowShortIfStatementsOnASingleLine);
  CHECK_PARSE_BOOL(AllowShortLoopsOnASingleLine);
  CHECK_PARSE_BOOL(AlwaysBreakTemplateDeclarations);
  CHECK_PARSE_BOOL(BinPackParameters);
  CHECK_PARSE_BOOL(BreakBeforeBinaryOperators);
  CHECK_PARSE_BOOL(BreakConstructorInitializersBeforeComma);
  CHECK_PARSE_BOOL(ConstructorInitializerAllOnOneLineOrOnePerLine);
  CHECK_PARSE_BOOL(DerivePointerBinding);
  CHECK_PARSE_BOOL(IndentCaseLabels);
  CHECK_PARSE_BOOL(ObjCSpaceBeforeProtocolList);
  CHECK_PARSE_BOOL(PointerBindsToType);
  CHECK_PARSE_BOOL(Cpp11BracedListStyle);
  CHECK_PARSE_BOOL(UseTab);
  CHECK_PARSE_BOOL(IndentFunctionDeclarationAfterType);

  CHECK_PARSE("AccessModifierOffset: -1234", AccessModifierOffset, -1234);
  CHECK_PARSE("ConstructorInitializerIndentWidth: 1234",
              ConstructorInitializerIndentWidth, 1234u);
  CHECK_PARSE("ColumnLimit: 1234", ColumnLimit, 1234u);
  CHECK_PARSE("MaxEmptyLinesToKeep: 1234", MaxEmptyLinesToKeep, 1234u);
  CHECK_PARSE("PenaltyExcessCharacter: 1234", PenaltyExcessCharacter, 1234u);
  CHECK_PARSE("PenaltyReturnTypeOnItsOwnLine: 1234",
              PenaltyReturnTypeOnItsOwnLine, 1234u);
  CHECK_PARSE("SpacesBeforeTrailingComments: 1234",
              SpacesBeforeTrailingComments, 1234u);
  CHECK_PARSE("IndentWidth: 32", IndentWidth, 32u);

  Style.Standard = FormatStyle::LS_Auto;
  CHECK_PARSE("Standard: C++03", Standard, FormatStyle::LS_Cpp03);
  CHECK_PARSE("Standard: C++11", Standard, FormatStyle::LS_Cpp11);
  CHECK_PARSE("Standard: Auto", Standard, FormatStyle::LS_Auto);

  Style.ColumnLimit = 123;
  FormatStyle BaseStyle = getLLVMStyle();
  CHECK_PARSE("BasedOnStyle: LLVM", ColumnLimit, BaseStyle.ColumnLimit);
  CHECK_PARSE("BasedOnStyle: LLVM\nColumnLimit: 1234", ColumnLimit, 1234u);

  Style.BreakBeforeBraces = FormatStyle::BS_Stroustrup;
  CHECK_PARSE("BreakBeforeBraces: Attach", BreakBeforeBraces,
              FormatStyle::BS_Attach);
  CHECK_PARSE("BreakBeforeBraces: Linux", BreakBeforeBraces,
              FormatStyle::BS_Linux);
  CHECK_PARSE("BreakBeforeBraces: Stroustrup", BreakBeforeBraces,
              FormatStyle::BS_Stroustrup);

  Style.NamespaceIndentation = FormatStyle::NI_All;
  CHECK_PARSE("NamespaceIndentation: None", NamespaceIndentation,
              FormatStyle::NI_None);
  CHECK_PARSE("NamespaceIndentation: Inner", NamespaceIndentation,
              FormatStyle::NI_Inner);
  CHECK_PARSE("NamespaceIndentation: All", NamespaceIndentation,
              FormatStyle::NI_All);

#undef CHECK_PARSE
#undef CHECK_PARSE_BOOL
}

TEST_F(FormatTest, ConfigurationRoundTripTest) {
  FormatStyle Style = getLLVMStyle();
  std::string YAML = configurationAsText(Style);
  FormatStyle ParsedStyle = {};
  EXPECT_EQ(0, parseConfiguration(YAML, &ParsedStyle).value());
  EXPECT_EQ(Style, ParsedStyle);
}

TEST_F(FormatTest, WorksFor8bitEncodings) {
  EXPECT_EQ("\"\xce\xe4\xed\xe0\xe6\xe4\xfb \xe2 \"\n"
            "\"\xf1\xf2\xf3\xe4\xb8\xed\xf3\xfe \"\n"
            "\"\xe7\xe8\xec\xed\xfe\xfe \"\n"
            "\"\xef\xee\xf0\xf3...\"",
            format("\"\xce\xe4\xed\xe0\xe6\xe4\xfb \xe2 "
                   "\xf1\xf2\xf3\xe4\xb8\xed\xf3\xfe \xe7\xe8\xec\xed\xfe\xfe "
                   "\xef\xee\xf0\xf3...\"",
                   getLLVMStyleWithColumns(12)));
}

// FIXME: Encode Cyrillic and CJK characters below to appease MS compilers.
#if !defined(_MSC_VER)

TEST_F(FormatTest, CountsUTF8CharactersProperly) {
  verifyFormat("\"Однажды в студёную зимнюю пору...\"",
               getLLVMStyleWithColumns(35));
  verifyFormat("\"一 二 三 四 五 六 七 八 九 十\"",
               getLLVMStyleWithColumns(21));
  verifyFormat("// Однажды в студёную зимнюю пору...",
               getLLVMStyleWithColumns(36));
  verifyFormat("// 一 二 三 四 五 六 七 八 九 十",
               getLLVMStyleWithColumns(22));
  verifyFormat("/* Однажды в студёную зимнюю пору... */",
               getLLVMStyleWithColumns(39));
  verifyFormat("/* 一 二 三 四 五 六 七 八 九 十 */",
               getLLVMStyleWithColumns(25));
}

TEST_F(FormatTest, SplitsUTF8Strings) {
  EXPECT_EQ(
      "\"Однажды, в \"\n"
      "\"студёную \"\n"
      "\"зимнюю \"\n"
      "\"пору,\"",
      format("\"Однажды, в студёную зимнюю пору,\"",
             getLLVMStyleWithColumns(13)));
  EXPECT_EQ("\"一 二 三 四 \"\n"
            "\"五 六 七 八 \"\n"
            "\"九 十\"",
            format("\"一 二 三 四 五 六 七 八 九 十\"",
                   getLLVMStyleWithColumns(10)));
}

TEST_F(FormatTest, SplitsUTF8LineComments) {
  EXPECT_EQ("// Я из лесу\n"
            "// вышел; был\n"
            "// сильный\n"
            "// мороз.",
            format("// Я из лесу вышел; был сильный мороз.",
                   getLLVMStyleWithColumns(13)));
  EXPECT_EQ("// 一二三\n"
            "// 四五六七\n"
            "// 八\n"
            "// 九 十",
            format("// 一二三 四五六七 八  九 十", getLLVMStyleWithColumns(6)));
}

TEST_F(FormatTest, SplitsUTF8BlockComments) {
  EXPECT_EQ("/* Гляжу,\n"
            " * поднимается\n"
            " * медленно в\n"
            " * гору\n"
            " * Лошадка,\n"
            " * везущая\n"
            " * хворосту\n"
            " * воз. */",
            format("/* Гляжу, поднимается медленно в гору\n"
                   " * Лошадка, везущая хворосту воз. */",
                   getLLVMStyleWithColumns(13)));
  EXPECT_EQ("/* 一二三\n"
            " * 四五六七\n"
            " * 八\n"
            " * 九 十\n"
            " */",
            format("/* 一二三 四五六七 八  九 十 */", getLLVMStyleWithColumns(6)));
  EXPECT_EQ("/* 𝓣𝓮𝓼𝓽 𝔣𝔬𝔲𝔯\n"
            " * 𝕓𝕪𝕥𝕖\n"
            " * 𝖀𝕿𝕱-𝟠 */",
            format("/* 𝓣𝓮𝓼𝓽 𝔣𝔬𝔲𝔯 𝕓𝕪𝕥𝕖 𝖀𝕿𝕱-𝟠 */", getLLVMStyleWithColumns(12)));
}

TEST_F(FormatTest, ConstructorInitializerIndentWidth) {
  FormatStyle Style = getLLVMStyle();

  Style.ConstructorInitializerIndentWidth = 4;
  verifyFormat(
      "SomeClass::Constructor()\n"
      "    : aaaaaaaaaaaaa(aaaaaaaaaaaaaa), aaaaaaaaaaaaa(aaaaaaaaaaaaaa),\n"
      "      aaaaaaaaaaaaa(aaaaaaaaaaaaaa) {}",
      Style);

  Style.ConstructorInitializerIndentWidth = 2;
  verifyFormat(
      "SomeClass::Constructor()\n"
      "  : aaaaaaaaaaaaa(aaaaaaaaaaaaaa), aaaaaaaaaaaaa(aaaaaaaaaaaaaa),\n"
      "    aaaaaaaaaaaaa(aaaaaaaaaaaaaa) {}",
      Style);

  Style.ConstructorInitializerIndentWidth = 0;
  verifyFormat(
      "SomeClass::Constructor()\n"
      ": aaaaaaaaaaaaa(aaaaaaaaaaaaaa), aaaaaaaaaaaaa(aaaaaaaaaaaaaa),\n"
      "  aaaaaaaaaaaaa(aaaaaaaaaaaaaa) {}",
      Style);

  Style.BreakConstructorInitializersBeforeComma = true;
  Style.ConstructorInitializerIndentWidth = 4;
  verifyFormat("SomeClass::Constructor()\n"
               "    : a(a)\n"
               "    , b(b)\n"
               "    , c(c) {}",
               Style);

  Style.ConstructorInitializerIndentWidth = 2;
  verifyFormat("SomeClass::Constructor()\n"
               "  : a(a)\n"
               "  , b(b)\n"
               "  , c(c) {}",
               Style);

  Style.ConstructorInitializerIndentWidth = 0;
  verifyFormat("SomeClass::Constructor()\n"
               ": a(a)\n"
               ", b(b)\n"
               ", c(c) {}",
               Style);
}

#endif

TEST_F(FormatTest, FormatsWithWebKitStyle) {
  FormatStyle Style = getWebKitStyle();

  // Don't indent in outer namespaces.
  verifyFormat("namespace outer {\n"
               "int i;\n"
               "namespace inner {\n"
               "    int i;\n"
               "} // namespace inner\n"
               "} // namespace outer\n"
               "namespace other_outer {\n"
               "int i;\n"
               "}",
               Style);

  // Don't indent case labels.
  verifyFormat("switch (variable) {\n"
               "case 1:\n"
               "case 2:\n"
               "    doSomething();\n"
               "    break;\n"
               "default:\n"
               "    ++variable;\n"
               "}",
               Style);

  // Wrap before binary operators.
  EXPECT_EQ(
      "void f()\n"
      "{\n"
      "    if (aaaaaaaaaaaaaaaa\n"
      "        && bbbbbbbbbbbbbbbbbbbbbbbb\n"
      "        && (cccccccccccccccccccccccccc || dddddddddddddddddddd))\n"
      "        return;\n"
      "}",
      format(
          "void f() {\n"
          "if (aaaaaaaaaaaaaaaa\n"
          "&& bbbbbbbbbbbbbbbbbbbbbbbb\n"
          "&& (cccccccccccccccccccccccccc || dddddddddddddddddddd))\n"
          "return;\n"
          "}",
          Style));

  // Constructor initializers are formatted one per line with the "," on the
  // new line.
  verifyFormat("Constructor()\n"
               "    : aaaaaaaaaaaaaaaaaaaaaaaa(aaaaaaaaaaaaaaaaaaaaaaaaaaa)\n"
               "    , aaaaaaaaaaaaaaaaaaaaaaaa(aaaaaaaaaaaaaa, // break\n"
               "                               aaaaaaaaaaaaaa)\n"
               "    , aaaaaaaaaaaaaaaaaaaaaaa()\n{\n}",
               Style);

  // Access specifiers should be aligned left.
  verifyFormat("class C {\n"
               "public:\n"
               "    int i;\n"
               "};",
               Style);

  // Do not align comments.
  verifyFormat("int a; // Do not\n"
               "double b; // align comments.",
               Style);

  // Accept input's line breaks.
  EXPECT_EQ("if (aaaaaaaaaaaaaaa\n"
            "    || bbbbbbbbbbbbbbb) {\n"
            "    i++;\n"
            "}",
            format("if (aaaaaaaaaaaaaaa\n"
                   "|| bbbbbbbbbbbbbbb) { i++; }",
                   Style));
  EXPECT_EQ("if (aaaaaaaaaaaaaaa || bbbbbbbbbbbbbbb) {\n"
            "    i++;\n"
            "}",
            format("if (aaaaaaaaaaaaaaa || bbbbbbbbbbbbbbb) { i++; }", Style));
}

} // end namespace tooling
} // end namespace clang
