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
// 当前的最大偏移数
int totalStackStride = 0;
// 计算当前value是存在寄存器里还是一个数需要li
std::string RawValueProc(const koopa_raw_value_t &value);
// 分配当前的寄存器
int RegisterAllocation();
// 通过标号得到寄存器的名称
std::string GetRegName(int regNum);
// 计算当前的函数在栈里需要偏移的位数
int GetFuncStrideNum(const koopa_raw_function_t &func);

extern int yyparse(unique_ptr<BaseAST> &ast);

void Visit(const koopa_raw_program_t &program);
void Visit(const koopa_raw_slice_t &slice);
void Visit(const koopa_raw_function_t &func);
void Visit(const koopa_raw_basic_block_t &bb);
void Visit(const koopa_raw_value_t &value);
void Visit(const koopa_raw_return_t &ret);
void Visit(const koopa_raw_integer_t &integer);
void Visit(const koopa_raw_binary_t &binary, const koopa_raw_value_t &value);
void Visit(const koopa_raw_load_t &load);
void Visit(const koopa_raw_store_t &store, const koopa_raw_value_t &value);

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
  string tempStr = "";
  tempStr += " .text\n";
  tempStr += " .globl ";
  tempStr += tempFuncName;
  tempStr += "\n";
  tempStr += tempFuncName;
  tempStr += ":\n";
  // 移动栈帧
  int funcStride = GetFuncStrideNum(func);
  tempStr += "  addi sp, sp, ";
  tempStr += to_string(funcStride * 4);
  tempStr += "\n";
  riscvOriginal += tempStr;
  // 访问所有基本块
  Visit(func->bbs);
}

// 计算当前的函数在栈里需要偏移的位数
int GetFuncStrideNum(const koopa_raw_function_t &func)
{
  int strideCnt = 0;
  for (size_t j = 0; j < func->bbs.len; ++j)
  {
    assert(func->bbs.kind == KOOPA_RSIK_BASIC_BLOCK);
    koopa_raw_basic_block_t bb = (koopa_raw_basic_block_t)func->bbs.buffer[j];
    // 进一步处理当前基本块
    // ...
    for (size_t k = 0; k < bb->insts.len; ++k)
    {
      koopa_raw_value_t rawValue = (koopa_raw_value_t)bb->insts.buffer[k];
      // 有返回值，分配栈空间
      if (rawValue->ty->tag == KOOPA_RTT_UNIT)
      {
        RiscvRegElement tempElement = RiscvRegElement();
        // 分配栈空间给这个line
        tempElement.selfMinorType = 1;
        tempElement.rawValue = rawValue;
        tempElement.stackStride = strideCnt;
        singleLineRegVec.push_back(tempElement);
        strideCnt -= 1;
      }
    }
  }
  totalStackStride = strideCnt;
  return strideCnt;
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
  case KOOPA_RVT_ALLOC:
    riscvOriginal += "allocing\n";
    break;
  case KOOPA_RVT_LOAD:
    // 访问 load 指令
    Visit(kind.data.load);
  case KOOPA_RVT_STORE:
    // 访问 store 指令
    Visit(kind.data.store, value);
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
  else if (currRetVal == "x0")
  {
    tempStr += "  li a0, 0";
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
  RiscvRegElement tempElement = RiscvRegElement();
  int currFindFlag = 0;
  int currRegister = 0;
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
    // 没分配新的寄存器，而是运用left Operand的寄存器
    for (int i = 0; i < singleLineRegVec.size(); ++ i)
    {
      if (singleLineRegVec[i].rawValue == binary.lhs)
      {
        for (int j = 0; j < singleLineRegVec.size(); ++ j)
        {
          if (singleLineRegVec[j].rawValue == valueBinary)
          {
            currFindFlag = 1;
            // 有寄存器可存
            singleLineRegVec[j].selfMinorType = 0;
            singleLineRegVec[j].reg = singleLineRegVec[i].reg;
          }
        }
        if (currFindFlag == 0)
        {
          tempElement.rawValue = valueBinary;
          tempElement.selfMinorType = 0;
          tempElement.reg = singleLineRegVec[i].reg;
          singleLineRegVec.push_back(tempElement);
        }
        singleLineRegVec[i].selfMinorType = 1;
        break;
      }
    }
    break;
  case KOOPA_RBO_NOT_EQ:
    tempStr += "  xor ";
    tempStr += lOperand;
    tempStr += ", ";
    tempStr += lOperand;
    tempStr += ", ";
    tempStr += rOperand;
    tempStr += "\n";
    tempStr += "  snez ";
    tempStr += lOperand;
    tempStr += ", ";
    tempStr += lOperand;
    tempStr += "\n";
    riscvOriginal += tempStr;
    // 没分配新的寄存器，而是运用left Operand的寄存器
    for (int i = 0; i < singleLineRegVec.size(); ++ i)
    {
      if (singleLineRegVec[i].rawValue == binary.lhs)
      {
        for (int j = 0; j < singleLineRegVec.size(); ++ j)
        {
          if (singleLineRegVec[j].rawValue == valueBinary)
          {
            currFindFlag = 1;
            // 有寄存器可存
            singleLineRegVec[j].selfMinorType = 0;
            singleLineRegVec[j].reg = singleLineRegVec[i].reg;
          }
        }
        if (currFindFlag == 0)
        {
          tempElement.rawValue = valueBinary;
          tempElement.selfMinorType = 0;
          tempElement.reg = singleLineRegVec[i].reg;
          singleLineRegVec.push_back(tempElement);
        }
        singleLineRegVec[i].selfMinorType = 1;
        break;
      }
    }
    break;
  case KOOPA_RBO_GT:
    currRegister = RegisterAllocation();
    tempStr += "  sgt ";
    tempStr += GetRegName(currRegister);
    tempStr += ", ";
    tempStr += lOperand;
    tempStr += ", ";
    tempStr += rOperand;
    tempStr += "\n";
    riscvOriginal += tempStr;
    // 存入当前的表里
    for (int i = 0; i < singleLineRegVec.size(); ++ i)
    {
      if (singleLineRegVec[i].rawValue == valueBinary)
      {
        currFindFlag = 1;
        singleLineRegVec[i].selfMinorType = 0;
        singleLineRegVec[i].reg = currRegister;
      }
    }
    if (currFindFlag == 0)
    {
      tempElement.selfMinorType = 0;
      tempElement.rawValue = valueBinary;
      tempElement.reg = currRegister;
      singleLineRegVec.push_back(tempElement);
    }
    break;
  case KOOPA_RBO_LT:
    currRegister = RegisterAllocation();
    tempStr += "  slt ";
    tempStr += GetRegName(currRegister);
    tempStr += ", ";
    tempStr += lOperand;
    tempStr += ", ";
    tempStr += rOperand;
    tempStr += "\n";
    riscvOriginal += tempStr;
    // 存入当前的表里
    for (int i = 0; i < singleLineRegVec.size(); ++ i)
    {
      if (singleLineRegVec[i].rawValue == valueBinary)
      {
        singleLineRegVec[i].selfMinorType = 0;
        singleLineRegVec[i].reg = currRegister;
      }
    }
    if (currFindFlag == 0)
    {
      tempElement.selfMinorType = 0;
      tempElement.rawValue = valueBinary;
      tempElement.reg = currRegister;
      singleLineRegVec.push_back(tempElement);
    }
    break;
  // greater or equal, 检查是否不小于
  case KOOPA_RBO_GE:
    // slt 小于是1 不小于是0
    // seqz: 0->1
    tempStr += "  slt ";
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
    // 没分配新的寄存器，而是运用left Operand的寄存器
    for (int i = 0; i < singleLineRegVec.size(); ++ i)
    {
      if (singleLineRegVec[i].rawValue == binary.lhs)
      {
        for (int j = 0; j < singleLineRegVec.size(); ++ j)
        {
          if (singleLineRegVec[j].rawValue == valueBinary)
          {
            currFindFlag = 1;
            // 有寄存器可存
            singleLineRegVec[j].selfMinorType = 0;
            singleLineRegVec[j].reg = singleLineRegVec[i].reg;
          }
        }
        if (currFindFlag == 0)
        {
          tempElement.rawValue = valueBinary;
          tempElement.selfMinorType = 0;
          tempElement.reg = singleLineRegVec[i].reg;
          singleLineRegVec.push_back(tempElement);
        }
        singleLineRegVec[i].selfMinorType = 1;
        break;
      }
    }
    break;
  // less or equal, 检查是否不大于
  case KOOPA_RBO_LE:
    // sgt 大于是1 不大于是0
    // seqz: 0->1
    tempStr += "  sgt ";
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
    // 没分配新的寄存器，而是运用left Operand的寄存器
    for (int i = 0; i < singleLineRegVec.size(); ++ i)
    {
      if (singleLineRegVec[i].rawValue == binary.lhs)
      {
        for (int j = 0; j < singleLineRegVec.size(); ++ j)
        {
          if (singleLineRegVec[j].rawValue == valueBinary)
          {
            currFindFlag = 1;
            // 有寄存器可存
            singleLineRegVec[j].selfMinorType = 0;
            singleLineRegVec[j].reg = singleLineRegVec[i].reg;
          }
        }
        if (currFindFlag == 0)
        {
          tempElement.rawValue = valueBinary;
          tempElement.selfMinorType = 0;
          tempElement.reg = singleLineRegVec[i].reg;
          singleLineRegVec.push_back(tempElement);
        }
        singleLineRegVec[i].selfMinorType = 1;
        break;
      }
    }
    break;
  case KOOPA_RBO_ADD:
    currRegister = RegisterAllocation();
    tempStr += "  add ";
    tempStr += GetRegName(currRegister);
    tempStr += ", ";
    tempStr += lOperand;
    tempStr += ", ";
    tempStr += rOperand;
    tempStr += "\n";
    riscvOriginal += tempStr;
    // 存入当前的表里
    for (int i = 0; i < singleLineRegVec.size(); ++ i)
    {
      if (singleLineRegVec[i].rawValue == valueBinary)
      {
        singleLineRegVec[i].selfMinorType = 0;
        singleLineRegVec[i].reg = currRegister;
      }
    }
    if (currFindFlag == 0)
    {
      tempElement.selfMinorType = 0;
      tempElement.rawValue = valueBinary;
      tempElement.reg = currRegister;
      singleLineRegVec.push_back(tempElement);
    }
    break;
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
    for (int i = 0; i < singleLineRegVec.size(); ++ i)
    {
      if (singleLineRegVec[i].rawValue == valueBinary)
      {
        singleLineRegVec[i].selfMinorType = 0;
        singleLineRegVec[i].reg = currRegister;
      }
    }
    if (currFindFlag == 0)
    {
      tempElement.selfMinorType = 0;
      tempElement.rawValue = valueBinary;
      tempElement.reg = currRegister;
      singleLineRegVec.push_back(tempElement);
    }
    break;
  case KOOPA_RBO_MUL:
    currRegister = RegisterAllocation();
    tempStr += "  mul ";
    tempStr += GetRegName(currRegister);
    tempStr += ", ";
    tempStr += lOperand;
    tempStr += ", ";
    tempStr += rOperand;
    tempStr += "\n";
    riscvOriginal += tempStr;
    // 存入当前的表里
    for (int i = 0; i < singleLineRegVec.size(); ++ i)
    {
      if (singleLineRegVec[i].rawValue == valueBinary)
      {
        singleLineRegVec[i].selfMinorType = 0;
        singleLineRegVec[i].reg = currRegister;
      }
    }
    if (currFindFlag == 0)
    {
      tempElement.selfMinorType = 0;
      tempElement.rawValue = valueBinary;
      tempElement.reg = currRegister;
      singleLineRegVec.push_back(tempElement);
    }
    break;
  case KOOPA_RBO_DIV:
    currRegister = RegisterAllocation();
    tempStr += "  div ";
    tempStr += GetRegName(currRegister);
    tempStr += ", ";
    tempStr += lOperand;
    tempStr += ", ";
    tempStr += rOperand;
    tempStr += "\n";
    riscvOriginal += tempStr;
    // 存入当前的表里
    for (int i = 0; i < singleLineRegVec.size(); ++ i)
    {
      if (singleLineRegVec[i].rawValue == valueBinary)
      {
        singleLineRegVec[i].selfMinorType = 0;
        singleLineRegVec[i].reg = currRegister;
      }
    }
    if (currFindFlag == 0)
    {
      tempElement.selfMinorType = 0;
      tempElement.rawValue = valueBinary;
      tempElement.reg = currRegister;
      singleLineRegVec.push_back(tempElement);
    }
    break;
  case KOOPA_RBO_MOD:
    currRegister = RegisterAllocation();
    tempStr += "  rem ";
    tempStr += GetRegName(currRegister);
    tempStr += ", ";
    tempStr += lOperand;
    tempStr += ", ";
    tempStr += rOperand;
    tempStr += "\n";
    riscvOriginal += tempStr;
    // 存入当前的表里
    for (int i = 0; i < singleLineRegVec.size(); ++ i)
    {
      if (singleLineRegVec[i].rawValue == valueBinary)
      {
        singleLineRegVec[i].selfMinorType = 0;
        singleLineRegVec[i].reg = currRegister;
      }
    }
    if (currFindFlag == 0)
    {
      tempElement.selfMinorType = 0;
      tempElement.rawValue = valueBinary;
      tempElement.reg = currRegister;
      singleLineRegVec.push_back(tempElement);
    }
    break;
  case KOOPA_RBO_AND:
    currRegister = RegisterAllocation();
    tempStr += "  and ";
    tempStr += GetRegName(currRegister);
    tempStr += ", ";
    tempStr += lOperand;
    tempStr += ", ";
    tempStr += rOperand;
    tempStr += "\n";
    riscvOriginal += tempStr;
    // 存入当前的表里
    for (int i = 0; i < singleLineRegVec.size(); ++ i)
    {
      if (singleLineRegVec[i].rawValue == valueBinary)
      {
        singleLineRegVec[i].selfMinorType = 0;
        singleLineRegVec[i].reg = currRegister;
      }
    }
    if (currFindFlag == 0)
    {
      tempElement.selfMinorType = 0;
      tempElement.rawValue = valueBinary;
      tempElement.reg = currRegister;
      singleLineRegVec.push_back(tempElement);
    }
    break;
  case KOOPA_RBO_OR:
    currRegister = RegisterAllocation();
    tempStr += "  or ";
    tempStr += GetRegName(currRegister);
    tempStr += ", ";
    tempStr += lOperand;
    tempStr += ", ";
    tempStr += rOperand;
    tempStr += "\n";
    riscvOriginal += tempStr;
    // 存入当前的表里
    for (int i = 0; i < singleLineRegVec.size(); ++ i)
    {
      if (singleLineRegVec[i].rawValue == valueBinary)
      {
        singleLineRegVec[i].selfMinorType = 0;
        singleLineRegVec[i].reg = currRegister;
      }
    }
    if (currFindFlag == 0)
    {
      tempElement.selfMinorType = 0;
      tempElement.rawValue = valueBinary;
      tempElement.reg = currRegister;
      singleLineRegVec.push_back(tempElement);
    }
    break;
  case KOOPA_RBO_XOR:
    currRegister = RegisterAllocation();
    tempStr += "  xor ";
    tempStr += GetRegName(currRegister);
    tempStr += ", ";
    tempStr += lOperand;
    tempStr += ", ";
    tempStr += rOperand;
    tempStr += "\n";
    riscvOriginal += tempStr;
    // 存入当前的表里
    for (int i = 0; i < singleLineRegVec.size(); ++ i)
    {
      if (singleLineRegVec[i].rawValue == valueBinary)
      {
        singleLineRegVec[i].selfMinorType = 0;
        singleLineRegVec[i].reg = currRegister;
      }
    }
    if (currFindFlag == 0)
    {
      tempElement.selfMinorType = 0;
      tempElement.rawValue = valueBinary;
      tempElement.reg = currRegister;
      singleLineRegVec.push_back(tempElement);
    }
    break;
  default:
    assert(false);
  }
}

// load 指令
void Visit(const koopa_raw_load_t &load)
{
  string tempStr = "";
  string getValueResult = RawValueProc(load.src);
  int currReg = RegisterAllocation();
  tempStr += "  lw ";
  tempStr += GetRegName(currReg);
  tempStr += ", ";
  tempStr += getValueResult;
  tempStr += "\n";
  for (int i = 0; i < singleLineRegVec.size(); ++ i)
  {
    if (singleLineRegVec[i].rawValue == load.src)
    {
      singleLineRegVec[i].selfMinorType = 0;
      singleLineRegVec[i].reg = currReg;
    }
  }
  riscvOriginal += tempStr;
}

// store 指令
void Visit(const koopa_raw_store_t &store, const koopa_raw_value_t &value)
{
  // value dest
  // 先load value到寄存器中再存入dest
  string tempStr = "";
  string getValueResult = RawValueProc(store.value);
  string getDestResult = RawValueProc(store.dest);
  int currReg = RegisterAllocation();
  tempStr += "  lw ";
  tempStr += GetRegName(currReg);
  tempStr += ", ";
  tempStr += getValueResult;
  tempStr += "\n";
  tempStr += "  sw ";
  tempStr += GetRegName(currReg);
  tempStr += ", ";
  tempStr += getDestResult;
  tempStr += "\n";
  for (int i = 0; i < singleLineRegVec.size(); ++ i)
  {
    if (singleLineRegVec[i].rawValue == store.value)
    {
      singleLineRegVec[i].selfMinorType = 0;
      singleLineRegVec[i].reg = currReg;
    }
  }
  riscvOriginal += tempStr;
}

// process an operand and get result
std::string RawValueProc(const koopa_raw_value_t &value)
{
  if (value->kind.tag == KOOPA_RVT_INTEGER)
  {
    // 新的数 需要li
    int currRegister = RegisterAllocation();
    riscvOriginal += "  li ";
    std::string regName = GetRegName(currRegister);
    riscvOriginal += regName;
    riscvOriginal += ", ";
    riscvOriginal += to_string(value->kind.data.integer.value);
    riscvOriginal += "\n";
    // 加入寄存器map
    RiscvRegElement tempElement = RiscvRegElement();
    // 是寄存器
    tempElement.selfMinorType = 0;
    tempElement.rawValue = value;
    tempElement.reg = currRegister;
    singleLineRegVec.push_back(tempElement);
    // 返回当前寄存器的结果
    return regName;
  }
  else if (value->kind.tag == KOOPA_RVT_BINARY)
  {
    // 寻找有没有被分配
    for (int i = 0; i < singleLineRegVec.size(); ++i)
    {
      if (singleLineRegVec[i].rawValue == value)
      {
        // 返回寄存器或者一个地址
        if (singleLineRegVec[i].selfMinorType == 0)
        {
          return GetRegName(singleLineRegVec[i].reg);
        }
        else
        {
          int currStackStride = singleLineRegVec[i].stackStride - totalStackStride - 1;
          return to_string(currStackStride) + "(sp)";
        }
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
    else
    {
      for (int i = 0; i < singleLineRegVec.size(); ++i)
      {
        if (singleLineRegVec[i].reg == 0)
        {
          std::string tempStr = "";
          tempStr += "  sw ";
          tempStr += GetRegName(0);
          tempStr += ", ";
          singleLineRegVec[i].selfMinorType = 1;
          tempStr += RawValueProc(singleLineRegVec[i].rawValue);
          tempStr += "\n";
          riscvOriginal += tempStr;
          registerStatus[0] = 0;
          chosenRegister = 0;
          break;
        }
      }
      return chosenRegister;
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