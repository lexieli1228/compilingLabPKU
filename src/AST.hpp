#pragma once
#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>

extern int registerCnt;

class BaseAST
{
public:
    virtual ~BaseAST() = default;

    virtual void Dump(std::string &strOriginal) const = 0;

    // 返回寄存器的编号或者一个数值
    virtual std::string ReversalDump(std::string &strOriginal)
    {
        return "";
    }
};

class CompUnitAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> func_def;

    void Dump(std::string &strOriginal) const override
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

    void Dump(std::string &strOriginal) const override
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
    void Dump(std::string &strOriginal) const override
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
    void Dump(std::string &strOriginal) const override
    {
        strOriginal += "%entry: ";
        strOriginal += "\n";
        stmt->Dump(strOriginal);
    }
};

class StmtAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> exp;
    void Dump(std::string &strOriginal) const override
    {
        // 最后一个level的寄存器
        std::string lastLevelRegister = exp->ReversalDump(strOriginal);
        strOriginal += "  ret ";
        strOriginal += lastLevelRegister;
        strOriginal += "\n";
    }
};

class ExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> unaryExp;
    void Dump(std::string &strOriginal) const override {}
    // exp所用的寄存器或者一个number
    std::string ReversalDump(std::string &strOriginal) override
    {
        return unaryExp->ReversalDump(strOriginal);
    }
};

class PrimaryExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> Number;
    std::string selfMinorType;
    void Dump(std::string &strOriginal) const override {}
    std::string ReversalDump(std::string &strOriginal) override
    {
        // exp
        if (selfMinorType[0] == '0')
        {
            return exp->ReversalDump(strOriginal);
        }
        // number
        else
        {
            return Number->ReversalDump(strOriginal);
        }
    }
};

class NumberAST : public BaseAST
{
public:
    int Number;
    void Dump(std::string &strOriginal) const override {}
    // 对于一个number，直接返回它的大小
    std::string ReversalDump(std::string &strOriginal) override
    {
        return std::to_string(Number);
    }
};

class UnaryExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> primaryExp;
    std::unique_ptr<BaseAST> unaryOp;
    std::unique_ptr<BaseAST> unaryExp;
    std::string selfMinorType;
    void Dump(std::string &strOriginal) const override {}
    // 返回一个数字或者寄存器标号
    std::string ReversalDump(std::string &strOriginal) override
    {
        // PrimaryExp
        if (selfMinorType[0] == '0')
        {
            return primaryExp->ReversalDump(strOriginal);
        }
        // UnaryOp UnaryExp
        else
        {
            std::string currOperator = unaryOp->ReversalDump(strOriginal);
            std::string lastLevelRegister = unaryExp->ReversalDump(strOriginal);
            // it is a number
            if (lastLevelRegister[0] != '%')
            {
                if (currOperator[0] == '-')
                {
                    std::string tempStr = "  %0 = sub 0, ";
                    tempStr += lastLevelRegister;
                    tempStr += "\n";
                    strOriginal += tempStr;
                }
                else if (currOperator[0] == '!')
                {
                    std::string tempStr = "  %0 = eq ";
                    tempStr += lastLevelRegister;
                    tempStr += ", 0\n";
                    strOriginal += tempStr;
                }
                return "%0";
            }
            // it is a register
            else
            {
                std::string tempLastLevelRegister = lastLevelRegister;
                tempLastLevelRegister.erase(0, 1);
                int currRegisterNum = std::atoi(tempLastLevelRegister.c_str()) + 1;
                std::string currRegister = "%" + std::to_string(currRegisterNum);
                if (currOperator[0] == '-')
                {
                    std::string tempStr = "  ";
                    tempStr += currRegister;
                    tempStr += " = sub 0, ";
                    tempStr += lastLevelRegister;
                    tempStr += "\n";
                    strOriginal += tempStr;
                }
                else if (currOperator[0] == '!')
                {
                    std::string tempStr = currRegister;
                    tempStr += " = eq ";
                    tempStr += lastLevelRegister;
                    tempStr += ", 0\n";
                    strOriginal += tempStr;
                }
                else if (currOperator[0] == '+')
                {
                    return lastLevelRegister;
                }
                return currRegister;
            }
        }
    }
};

class UnaryOpAST : public BaseAST
{
public:
    std::string unaryOp;
    void Dump(std::string &strOriginal) const override {}
    // 返回符号
    std::string ReversalDump(std::string &strOriginal) override
    {
        registerCnt += 1;
        return unaryOp;
    }
};