#pragma once
#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <cstring>
#include <vector>
#include <map>

// 符号表中的元素
class SyntaxElement 
{
public:
  std::string name;
  bool isConstant;
  bool isGivenNum;
  int num; 
  SyntaxElement()
  {
    name = "";
    isConstant = false;
    isGivenNum = false;
    num = -1;
  }
  bool operator == (const SyntaxElement &obj) const
  {
    if (name == obj.name)
    {
        return true;
    }
    return false;
  }
};