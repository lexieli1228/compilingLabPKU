#include "koopa.h"
#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <map>
#include <vector>
#include "AST.hpp"
#include "VariableClass.hpp"

using namespace std;

extern FILE *yyin;
extern FILE *yyout;

// 记录当前寄存器使用个数
int registerCnt = 0;
int currMaxRegister = -1;

// 记录当前符号表的level层数
int currMaxSyntaxVec = -1;

// 当前If条件语句中then block的数又多少
int currThenBlockCnt = -1;

// 当前block中有没有return
int currRetFlag = 0;

// 因为return多分割了多少个block
int currAfterRetNum = -1;

// 分割了多少while entry和body
int currWhileNum = -1;

// current while content;
int currWhileContentFlag = 0;

// main函数的符号表
map<std::string, int> syntaxNameCnt;
vector<map<std::string, SyntaxElement> > syntaxTableVec;

extern int yyparse(unique_ptr<BaseAST> &ast);

void Visit(const koopa_raw_program_t &program);
void Visit(const koopa_raw_slice_t &slice);
void Visit(const koopa_raw_function_t &func);
void Visit(const koopa_raw_basic_block_t &bb);
void Visit(const koopa_raw_value_t &value);
void Visit(const koopa_raw_return_t &ret);
void Visit(const koopa_raw_integer_t &integer);

/*
  .text
  .globl main
main:
  # 实现 eq 6, 0 的操作, 并把结果存入 t0
  li    t0, 6
  xor   t0, t0, x0
  seqz  t0, t0
  # 减法
  sub   t1, x0, t0
  # 减法
  sub   t2, x0, t1
  # 设置返回值并返回
  mv    a0, t2
  ret
*/

// 访问 raw program
void Visit(const koopa_raw_program_t &program)
{
  // 执行一些其他的必要操作
  // 使用 for 循环遍历函数列表
  cout << " .text\n";

  cout << " .globl ";
  for (size_t i = 0; i < program.funcs.len; ++i)
  {
    // 正常情况下, 列表中的元素就是函数, 我们只不过是在确认这个事实
    // 当然, 你也可以基于 raw slice 的 kind, 实现一个通用的处理函数
    assert(program.funcs.kind == KOOPA_RSIK_FUNCTION);
    // 获取当前函数
    koopa_raw_function_t func = (koopa_raw_function_t)program.funcs.buffer[i];
    // 进一步处理当前函数
    // ...
    string tempFuncName = func->name;
    tempFuncName.erase(0, 1);
    cout << tempFuncName << "\n";
  }

  // 访问所有全局变量
  Visit(program.values);
  // 访问所有函数
  Visit(program.funcs);
}

// 访问 raw slice
void Visit(const koopa_raw_slice_t &slice)
{
  for (size_t i = 0; i < slice.len; ++i)
  {
    auto ptr = slice.buffer[i];
    // 根据 slice 的 kind 决定将 ptr 视作何种元素
    switch (slice.kind)
    {
    case KOOPA_RSIK_FUNCTION:
      // 访问函数
      Visit(reinterpret_cast<koopa_raw_function_t>(ptr));
      break;
    case KOOPA_RSIK_BASIC_BLOCK:
      // 访问基本块
      Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
      break;
    case KOOPA_RSIK_VALUE:
      // 访问指令
      Visit(reinterpret_cast<koopa_raw_value_t>(ptr));
      break;
    default:
      // 我们暂时不会遇到其他内容, 于是不对其做任何处理
      assert(false);
    }
  }
}

// 访问函数
void Visit(const koopa_raw_function_t &func)
{
  // 执行一些其他的必要操作
  string tempFuncName = func->name;
  tempFuncName.erase(0, 1);
  cout << tempFuncName << ":\n";
  // 访问所有基本块
  Visit(func->bbs);
}

// 访问基本块
void Visit(const koopa_raw_basic_block_t &bb)
{
  // 执行一些其他的必要操作
  // ...
  // 访问所有指令
  Visit(bb->insts);
}

// 访问指令
void Visit(const koopa_raw_value_t &value)
{
  // 根据指令类型判断后续需要如何访问
  const auto &kind = value->kind;
  switch (kind.tag)
  {
  case KOOPA_RVT_RETURN:
    // 访问 return 指令
    Visit(kind.data.ret);
    break;
  case KOOPA_RVT_INTEGER:
    // 访问 integer 指令
    Visit(kind.data.integer);
    break;
  default:
    // 其他类型暂时遇不到
    assert(false);
  }
}

// 访问ret
void Visit(const koopa_raw_return_t &ret)
{
  cout << " li a0, " << ret.value->kind.data.integer.value << "\n";
  cout << " ret\n";
}

// 访问integer
void Visit(const koopa_raw_integer_t &integer)
{
  cout << integer.value << "\n";
}

int main(int argc, const char *argv[])
{
  // 解析命令行参数. 测试脚本/评测平台要求你的编译器能接收如下参数:
  // compiler 模式 输入文件 -o 输出文件
  assert(argc == 5);
  auto mode = argv[1];
  auto input = argv[2];
  auto output = argv[4];

  // 打开输入文件, 并且指定 lexer 在解析的时候读取这个文件
  yyin = fopen(input, "r");
  assert(yyin);
  yyout = freopen(output, "w", stdout);

  // parse input file
  unique_ptr<BaseAST> ast;
  auto ret0 = yyparse(ast);
  assert(!ret0);

  // original koopa program
  string strOrigin = "";

  // dump AST into string
  ast->Dump(strOrigin);

  // // 转化成内存形式
  // // 解析字符串 str, 得到 Koopa IR 程序
  // koopa_program_t program;
  // koopa_error_code_t ret = koopa_parse_from_string(strOrigin.c_str(), &program);
  // assert(ret == KOOPA_EC_SUCCESS); // 确保解析时没有出错
  // // 创建一个 raw program builder, 用来构建 raw program
  // koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
  // // 将 Koopa IR 程序转换为 raw program
  // koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
  // // 释放 Koopa IR 程序占用的内存
  // koopa_delete_program(program);

  // directly output koopa program
  if (mode[1] == 'k')
  {
    cout << strOrigin;
  }
  // // generate riscv output, deal with raw program
  // else if (mode[1] == 'r')
  // {
  //   Visit(raw);
  // }

  // koopa_delete_raw_program_builder(builder);

  return 0;
}