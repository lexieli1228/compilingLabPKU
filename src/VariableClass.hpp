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
  
  bool operator == (const SyntaxElement &obj) const
  {
    if (ident == obj.ident)
    {
        return true;
    }
    return false;
  }
};
