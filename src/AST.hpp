#pragma once
#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <cstring>

extern int registerCnt;
extern int currMaxRegister;

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
        strOriginal += "\n";
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
    }
};

class ExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> lOrExp;
    void Dump(std::string &strOriginal) const override {}
    // exp所用的寄存器或者一个number
    std::string ReversalDump(std::string &strOriginal) override
    {
        return lOrExp->ReversalDump(strOriginal);
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

            if (currOperator[0] == '-')
            {
                currMaxRegister += 1;
                std::string currRegister = std::to_string(currMaxRegister);
                std::string tempStr = "  %";
                tempStr += currRegister;
                tempStr += " = sub 0, ";
                tempStr += lastLevelRegister;
                tempStr += "\n";
                strOriginal += tempStr;
                return "%" + currRegister;
            }
            else if (currOperator[0] == '!')
            {
                currMaxRegister += 1;
                std::string currRegister = std::to_string(currMaxRegister);
                std::string tempStr = "  %";
                tempStr += currRegister;
                tempStr += " = eq ";
                tempStr += lastLevelRegister;
                tempStr += ", 0\n";
                strOriginal += tempStr;
                return "%" + currRegister;
            }
            else if (currOperator[0] == '+')
            {
                return lastLevelRegister;
            }
            return "";
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

class MulExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> unaryExp;
    std::unique_ptr<BaseAST> mulExp;
    std::string mulOperator;
    std::string selfMinorType;
    void Dump(std::string &strOriginal) const override {}
    std::string ReversalDump(std::string &strOriginal) override 
    {
        // UnaryExp
        if (selfMinorType[0] == '0')
        {
            return unaryExp->ReversalDump(strOriginal);
        }
        // MulExp ('*' | '/' | '%') UnaryExp
        else
        {
            registerCnt ++;
            std::string mulLastRegister = mulExp->ReversalDump(strOriginal);
            std::string unaryLastRegister = unaryExp->ReversalDump(strOriginal);
            currMaxRegister ++;
            std::string currRegister = std::to_string(currMaxRegister);
            std::string tempStr = "  %";
            tempStr += currRegister;
            if (mulOperator[0] == '*')
            {
                tempStr += " = mul ";
            }
            else if (mulOperator[0] == '/')
            {
                tempStr += " = div ";
            }
            else if (mulOperator[0] == '%')
            {
                tempStr += " = mod ";
            }
            tempStr += mulLastRegister;
            tempStr += ", ";
            tempStr += unaryLastRegister;
            tempStr += "\n";
            strOriginal += tempStr;
            return "%" + currRegister;
        }
    }
};

class AddExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> mulExp;
    std::unique_ptr<BaseAST> addExp;
    std::string addOperator;
    std::string selfMinorType;
    void Dump(std::string &strOriginal) const override {}
    std::string ReversalDump(std::string &strOriginal) override 
    {
        // MulExp
        if (selfMinorType[0] == '0')
        {
            return mulExp->ReversalDump(strOriginal);
        }
        // AddExp ('+' | '-') MulExp
        else
        {
            registerCnt ++;
            std::string addLastRegister = addExp->ReversalDump(strOriginal);
            std::string mulLastRegister = mulExp->ReversalDump(strOriginal);
            currMaxRegister ++;
            std::string currRegister = std::to_string(currMaxRegister);
            std::string tempStr = "  %";
            tempStr += currRegister;
            if (addOperator[0] == '+')
            {
                tempStr += " = add ";
            }
            else if (addOperator[0] == '-')
            {
                tempStr += " = sub ";
            }
            tempStr += addLastRegister;
            tempStr += ", ";
            tempStr += mulLastRegister;
            tempStr += "\n";
            strOriginal += tempStr;
            return "%" + currRegister;
        }
    }
}; 

class RelExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> addExp;
    std::unique_ptr<BaseAST> relExp;
    std::string compOperator;
    std::string selfMinorType;
    void Dump(std::string &strOriginal) const override {}
    std::string ReversalDump(std::string &strOriginal) override 
    {
        // AddExp
        if (selfMinorType[0] == '0')
        {
            return addExp->ReversalDump(strOriginal);
        }
        // RelExp ("<" | ">" | "<=" | ">=") AddExp
        else
        {
            registerCnt ++;
            std::string relLastRegister = relExp->ReversalDump(strOriginal);
            std::string addLastRegister = addExp->ReversalDump(strOriginal);
            currMaxRegister ++;
            std::string currRegister = std::to_string(currMaxRegister);
            std::string tempStr = "  %";
            tempStr += currRegister;
            if (strcmp(compOperator.c_str(), "<") == 0)
            {
                tempStr += " = lt ";
            }
            else if (strcmp(compOperator.c_str(), ">") == 0)
            {
                tempStr += " = ge ";
            }
            else if (strcmp(compOperator.c_str(), "<=") == 0)
            {
                tempStr += " = le ";
            }
            else if (strcmp(compOperator.c_str(), ">=") == 0)
            {
                tempStr += " = ge ";
            }
            tempStr += relLastRegister;
            tempStr += ", ";
            tempStr += addLastRegister;
            tempStr += "\n";
            strOriginal += tempStr;
            return "%" + currRegister;
        }
    }
};

class EqExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> relExp;
    std::unique_ptr<BaseAST> eqExp;
    std::string eqOperator;
    std::string selfMinorType;
    void Dump(std::string &strOriginal) const override {}
    std::string ReversalDump(std::string &strOriginal) override 
    {
        // RepExp
        if (selfMinorType[0] == '0')
        {
            return relExp->ReversalDump(strOriginal);
        }
        // EqExp ("==" | "!=") RelExp;
        else
        {
            registerCnt ++;
            std::string eqLastRegister = eqExp->ReversalDump(strOriginal);
            std::string relLastRegister = relExp->ReversalDump(strOriginal);
            currMaxRegister ++;
            std::string currRegister = std::to_string(currMaxRegister);
            std::string tempStr = "  %";
            tempStr += currRegister;
            if (strcmp(eqOperator.c_str(), "==") == 0)
            {
                tempStr += " = eq ";
            }
            else if (strcmp(eqOperator.c_str(), "!=") == 0)
            {
                tempStr += " = ne ";
            }
            tempStr += eqLastRegister;
            tempStr += ", ";
            tempStr += relLastRegister;
            tempStr += "\n";
            strOriginal += tempStr;
            return "%" + currRegister;
        }
    }
};

class LAndExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> eqExp;
    std::unique_ptr<BaseAST> lAndExp;
    std::string lAndOperator;
    std::string selfMinorType;
    void Dump(std::string &strOriginal) const override {}
    std::string ReversalDump(std::string &strOriginal) override 
    {
        // EqExp
        if (selfMinorType[0] == '0')
        {
            return eqExp->ReversalDump(strOriginal);
        }
        // LAndExp "&&" EqExp
        else
        {
            registerCnt ++;
            std::string lAndLastRegister = lAndExp->ReversalDump(strOriginal);
            std::string eqLastRegister = eqExp->ReversalDump(strOriginal);

            // curr_0 = ne lAndExp, 0
            currMaxRegister ++;
            std::string currRegister_0 = std::to_string(currMaxRegister);
            std::string tempStr = "  %";
            tempStr += currRegister_0;
            tempStr += " = ne ";
            tempStr += lAndLastRegister;
            tempStr += ", 0\n";
            // curr_1 = ne eqExp, 0
            currMaxRegister ++;
            std::string currRegister_1 = std::to_string(currMaxRegister);
            tempStr += "  %";
            tempStr += currRegister_1;
            tempStr += " = ne ";
            tempStr += eqLastRegister;
            tempStr += ", 0\n";
            // curr_2 = and curr_0, curr_1
            currMaxRegister ++;
            std::string currRegister = std::to_string(currMaxRegister);
            tempStr += "  %";
            tempStr += currRegister;
            tempStr += " = and %";
            tempStr += currRegister_0;
            tempStr += ", %";
            tempStr += currRegister_1;
            tempStr += "\n";
            strOriginal += tempStr;
            return "%" + currRegister;
        }
    }
};

class LOrExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> lAndExp;
    std::unique_ptr<BaseAST> lOrExp;
    std::string lOrOperator;
    std::string selfMinorType;
    void Dump(std::string &strOriginal) const override {}
    std::string ReversalDump(std::string &strOriginal) override 
    {
        // lAndExp
        if (selfMinorType[0] == '0')
        {
            return lAndExp->ReversalDump(strOriginal);
        }
        // LOrExp "||" LAndExp
        else
        {
            registerCnt ++;
            std::string lOrLastRegister = lOrExp->ReversalDump(strOriginal);
            std::string lAndLastRegister = lAndExp->ReversalDump(strOriginal);

            // curr_0 = ne lOrExp, 0
            currMaxRegister ++;
            std::string currRegister_0 = std::to_string(currMaxRegister);
            std::string tempStr = "  %";
            tempStr += currRegister_0;
            tempStr += " = ne ";
            tempStr += lOrLastRegister;
            tempStr += ", 0\n";
            // curr_1 = ne LAndExp, 0
            currMaxRegister ++;
            std::string currRegister_1 = std::to_string(currMaxRegister);
            tempStr += "  %";
            tempStr += currRegister_1;
            tempStr += " = ne ";
            tempStr += lAndLastRegister;
            tempStr += ", 0\n";
            // curr_2 = or curr_0, curr_1
            currMaxRegister ++;
            std::string currRegister = std::to_string(currMaxRegister);
            tempStr += "  %";
            tempStr += currRegister;
            tempStr += " = or %";
            tempStr += currRegister_0;
            tempStr += ", %";
            tempStr += currRegister_1;
            tempStr += "\n";
            strOriginal += tempStr;
            return "%" + currRegister;
        }
    }
};