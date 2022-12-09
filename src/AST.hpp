#pragma once
#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>

class BaseAST
{
public:
    virtual ~BaseAST() = default;

    virtual void Dump(std::string& strOriginal) const = 0;
};

class CompUnitAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> func_def;

    void Dump(std::string& strOriginal) const override
    {
        func_def->Dump(strOriginal);
    }
};

class FuncDefAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;

    void Dump(std::string& strOriginal) const override
    {
        strOriginal += "fun @";
        strOriginal += ident;
        strOriginal += "(): ";
        func_type->Dump(strOriginal);
        strOriginal += " {";
        strOriginal += "\n";
        block->Dump(strOriginal);
        strOriginal += "}";
    }
};

class FuncTypeAST : public BaseAST
{
public:
    std::string funcType;
    void Dump(std::string& strOriginal) const override
    {
        if (funcType == "int")
        {
            strOriginal += "i32";
        }
    }
};

class BlockAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> stmt;
    void Dump(std::string& strOriginal) const override
    {
        strOriginal += "%entry: ";
        strOriginal += "\n";
        stmt->Dump(strOriginal);
        strOriginal += "\n";
    }
};

class StmtAST : public BaseAST
{
public:
    int Number;
    void Dump(std::string& strOriginal) const override
    {
        strOriginal += "  ret ";
        strOriginal += std::to_string(Number);
    }
};