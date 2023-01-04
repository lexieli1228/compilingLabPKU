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
// 当前的函数个数
int currFuncCnt = -1;
// 函数们
vector<FuncElement> funcTableVec;
// 全局命名
std::map<std::string, int> globalSyntaxNameCnt;
// 全局参数表
std::map<std::string, SyntaxElement> globalSyntaxTableMap;
// 是否对全局变量进行操作
int currGlobalFlag = 0;

// 后端输出的字符串
string riscvOriginal = "";
// 当前的raw value 计算结果，存在于哪个寄存器里面
// t0: 0, t1: 1, t2: 2, t3: 3, t4: 4, t5: 5, t6: 6
// a0: 7, a1: 8, a2: 9, a3: 10, a4: 11, a5: 12, a6: 13
vector<RiscvRegElement> singleLineRegVec;
// 当前寄存器的占用情况
int registerStatus[14] = {0};
// 计算当前value是存在寄存器里还是一个数需要li
std::string RawValueProc(const koopa_raw_value_t &value);
// 分配当前的寄存器
int RegisterAllocation();
// 通过标号得到寄存器的名称
std::string GetRegName(int regNum);

extern int yyparse(unique_ptr<BaseAST> &ast);

void Visit(const koopa_raw_program_t &program);
void Visit(const koopa_raw_slice_t &slice);
void Visit(const koopa_raw_function_t &func);
void Visit(const koopa_raw_basic_block_t &bb);
void Visit(const koopa_raw_value_t &value);
void Visit(const koopa_raw_return_t &ret);
void Visit(const koopa_raw_integer_t &integer);
void Visit(const koopa_raw_binary_t &binary, const koopa_raw_value_t &value);

// 访问 raw program
void Visit(const koopa_raw_program_t &program)
{
  for (size_t i = 0; i < program.funcs.len; ++i)
  {
    assert(program.funcs.kind == KOOPA_RSIK_FUNCTION);
    koopa_raw_function_t func = (koopa_raw_function_t)program.funcs.buffer[i];
  }

  // 全局变量
  Visit(program.values);

  // 所有函数
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
  string tempFuncName = func->name;
  tempFuncName.erase(0, 1);
  riscvOriginal += " .text\n";
  riscvOriginal += " .globl ";
  riscvOriginal += tempFuncName;
  riscvOriginal += ":\n";
  riscvOriginal += tempFuncName;
  riscvOriginal += ":\n";
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
  case KOOPA_RVT_BINARY:
    // 访问 binary 指令
    Visit(kind.data.binary, value);
    break;
  default:
    // 其他类型暂时遇不到
    assert(false);
  }
}

// 访问ret
void Visit(const koopa_raw_return_t &ret)
{
  std::string tempStr = "";
  std::string currRetVal = RawValueProc(ret.value);
  if (currRetVal[0] == 'a' || currRetVal[0] == 't')
  {
    tempStr += "  mv a0, ";
    tempStr += currRetVal;
  }
  else
  {
    tempStr += "  li a0, ";
    tempStr += currRetVal;
  }
  tempStr += "\n";
  tempStr += "  ret\n";
  riscvOriginal += tempStr;
}

// 访问integer
void Visit(const koopa_raw_integer_t &integer)
{
  riscvOriginal += "Visiting integer value: ";
  riscvOriginal += integer.value;
  riscvOriginal += "\n";
}

// binary calculation
void Visit(const koopa_raw_binary_t &binary, const koopa_raw_value_t &value)
{
  const auto binaryOp = binary.op;
  const auto valueBinary = value;
  std::string tempStr = "";
  std::string lOperand = RawValueProc(binary.lhs);
  std::string rOperand = RawValueProc(binary.rhs);
  RiscvRegElement tempIter = RiscvRegElement();
  int currRegister;
  switch (binaryOp)
  {
  case KOOPA_RBO_EQ:
    tempStr += "  xor ";
    tempStr += lOperand;
    tempStr += ", ";
    tempStr += lOperand;
    tempStr += ", ";
    tempStr += rOperand;
    tempStr += "\n";
    tempStr += "  seqz ";
    tempStr += lOperand;
    tempStr += ", ";
    tempStr += lOperand;
    tempStr += "\n";
    riscvOriginal += tempStr;
    // 存入当前的表里
    // 没分配新的寄存器，而是运用left Operand的寄存器
    // 删掉旧的
    for (int i = 0; i < singleLineRegVec.size(); ++ i)
    {
      if (singleLineRegVec[i].rawValue == binary.lhs)
      {
        singleLineRegVec[i].rawValue = valueBinary;
      }
    }
    break;
  case KOOPA_RBO_NOT_EQ:

  case KOOPA_RBO_GT:
  case KOOPA_RBO_LT:
  case KOOPA_RBO_GE:
  case KOOPA_RBO_LE:

  case KOOPA_RBO_ADD:
  case KOOPA_RBO_SUB:
    currRegister = RegisterAllocation();
    tempStr += "  sub ";
    tempStr += GetRegName(currRegister);
    tempStr += ", ";
    tempStr += lOperand;
    tempStr += ", ";
    tempStr += rOperand;
    tempStr += "\n";
    riscvOriginal += tempStr;
    // 存入当前的表里
    tempIter.reg = currRegister;
    tempIter.rawValue = valueBinary;
    singleLineRegVec.push_back(tempIter);
    break;
  case KOOPA_RBO_MUL:
  case KOOPA_RBO_DIV:
  case KOOPA_RBO_MOD:

  case KOOPA_RBO_AND:
  case KOOPA_RBO_OR:
    riscvOriginal += "\nhere is the op value: ";
    riscvOriginal += to_string(binaryOp);
    break;
  default:
    assert(false);
  }
}

// process an operand and get result
std::string RawValueProc(const koopa_raw_value_t &value)
{
  if (value->kind.tag == KOOPA_RVT_INTEGER)
  {
    // x0恒为0
    if (value->kind.data.integer.value == 0)
    {
      return "x0";
    }
    else
    {
      // 新的数 需要li
      riscvOriginal += "  li ";
      int currRegister = RegisterAllocation();
      std::string regName = GetRegName(currRegister);
      riscvOriginal += regName;
      riscvOriginal += ", ";
      riscvOriginal += to_string(value->kind.data.integer.value);
      riscvOriginal += "\n";
      // 加入寄存器map
      RiscvRegElement tempElement = RiscvRegElement();
      tempElement.rawValue = value;
      tempElement.reg = currRegister;
      singleLineRegVec.push_back(tempElement);
      // 返回当前寄存器的结果
      return regName;
    }
  }
  else if (value->kind.tag == KOOPA_RVT_BINARY)
  {
    // 寻找有没有被分配
    for (int i = 0; i < singleLineRegVec.size(); ++ i)
    {
      if (singleLineRegVec[i].rawValue == value)
      {
        return GetRegName(singleLineRegVec[i].reg);
      }
    }
  }
  else
  {
    assert(false);
  }
  return "";
}

// 寄存器分配函数
int RegisterAllocation()
{
  int chosenRegister = -1;
  for (int i = 0; i < 14; ++i)
  {
    // 空寄存器
    if (registerStatus[i] == 0 && i != 7)
    {
      registerStatus[i] = 1;
      chosenRegister = i;
      return chosenRegister;
    }
  }
  // 非必要不分配a0, 因为是要返回的
  if (chosenRegister == -1)
  {
    if (registerStatus[7] == 0)
    {
      registerStatus[7] = 1;
      chosenRegister = 7;
    }
  }
  return chosenRegister;
}

// 寄存器和数值的映射
std::string GetRegName(int regNum)
{
  if (regNum < 7)
  {
    return "t" + to_string(regNum);
  }
  else
  {
    return "a" + to_string(regNum - 6);
  }
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

  // 所有寄存器都没有被分配
  for (int i = 0; i < 14; ++i)
  {
    registerStatus[i] = 0;
  }

  // 转化成内存形式
  // 解析字符串 str, 得到 Koopa IR 程序
  koopa_program_t program;
  koopa_error_code_t ret = koopa_parse_from_string(strOrigin.c_str(), &program);
  assert(ret == KOOPA_EC_SUCCESS); // 确保解析时没有出错
  // 创建一个 raw program builder, 用来构建 raw program
  koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
  // 将 Koopa IR 程序转换为 raw program
  koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
  // 释放 Koopa IR 程序占用的内存
  koopa_delete_program(program);

  // directly output koopa program
  if (mode[1] == 'k')
  {
    cout << strOrigin;
  }
  // generate riscv output, deal with raw program
  else if (mode[1] == 'r')
  {
    Visit(raw);
    cout << riscvOriginal;
  }

  koopa_delete_raw_program_builder(builder);

  return 0;
}