#pragma once
#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <cstring>
#include <vector>
#include <map>

class SyntaxElement
{
public:
  std::string ident;
  bool isConstant;
  bool isGivenNum;
  int num;
  int index;
  int ifFuncInitialElement;

  SyntaxElement()
  {
    ident = "";
    isConstant = false;
    isGivenNum = false;
    num = -1;
    index = 0;
    ifFuncInitialElement = 0;
  }

  bool operator==(const SyntaxElement &obj) const
  {
    if (ident == obj.ident)
    {
      return true;
    }
    return false;
  }
};

class FuncElement
{
public:
  // 函数名字
  std::string funcName;
  // 函数类别
  std::string funcType;
  // 记录当前符号表的level层数
  int currMaxSyntaxVec;
  // 当前If条件语句中then block的数又多少
  int currThenBlockCnt;
  // 当前block中有没有return
  int currRetFlag;
  // 因为return多分割了多少个block
  int currAfterRetNum;
  // 分割了多少while entry和body
  int currWhileNum;
  // 当前工作的是哪个while层级
  int currLayerWhileNum;
  // current while content;
  int currWhileContentFlag;
  // 当前是否为调用函数结果
  int currCallFunc;
  // 命名
  std::map<std::string, int> syntaxNameCnt;
  // 参数表
  std::vector<std::map<std::string, SyntaxElement>> syntaxTableVec;

  FuncElement()
  {
    funcName = "";
    funcType = "";
    currMaxSyntaxVec = -1;
    currThenBlockCnt = -1;
    currRetFlag = 0;
    currAfterRetNum = -1;
    currWhileNum = -1;
    currLayerWhileNum = -1;
    currWhileContentFlag = 0;
    currCallFunc = 0;
  }
};