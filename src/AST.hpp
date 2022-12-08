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

    virtual void Dump() const = 0;
};

class CompUnitAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> func_def;

    void Dump() const override
    {
        func_def->Dump();
    }
};

class FuncDefAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;

    void Dump() const override
    {
        std::cout << "func @";
        std::cout << ident;
        std::cout << "(): ";
        func_type->Dump();
        std::cout << " {" << std::endl;
        block->Dump();
        std::cout << "}";
    }
};

class FuncTypeAST : public BaseAST
{
public:
    std::string funcType;
    void Dump() const override
    {
        if (funcType == "int")
        {
            std::cout << "i32";
        }
    }
};

class BlockAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> stmt;
    void Dump() const override
    {
        std::cout << "%entry: ";
        std::cout << std::endl;
        stmt->Dump();
        std::cout << std::endl;
    }
};

class StmtAST : public BaseAST
{
public:
    int Number;
    void Dump() const override
    {
        std::cout << "  ";
        std::cout << "ret ";
        std::cout << Number;
    }
};