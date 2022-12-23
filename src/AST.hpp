#pragma once
#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <cstring>
#include <vector>
#include <map>
#include "VariableClass.hpp"

extern int registerCnt;
extern int currMaxRegister;
extern int currMaxSyntaxVec;
extern std::map<std::string, int> syntaxNameCnt;
extern std::vector<std::map<std::string, SyntaxElement>> syntaxTableVec;
extern int currThenBlockCnt;

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

    // 计算表达式的值
    virtual int CalExpressionValue()
    {
        return 0;
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
        strOriginal += "%entry: ";
        strOriginal += "\n";
        block->Dump(strOriginal);
        strOriginal += "\n";
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
    std::unique_ptr<BaseAST> blockCombinedItem;
    std::map<std::string, SyntaxElement> currSyntaxTable;
    std::string selfMinorType;
    void Dump(std::string &strOriginal) const override
    {
        if (selfMinorType[0] == '0')
        {
            // 每次进入一个block 增加一个vector，然后在这个block处理过程中就会用这个最新vec 之前的vec一定对其有
            currMaxSyntaxVec += 1;
            syntaxTableVec.push_back(currSyntaxTable);
            blockCombinedItem->Dump(strOriginal);
            // erase the syntaxTable for this block when exiting
            if (syntaxTableVec.size() > 0)
            {
                syntaxTableVec.erase(syntaxTableVec.begin() + currMaxSyntaxVec);
            }
            currMaxSyntaxVec -= 1;
        }
        else
        {
            // do nothing
        }
    }
};

class BlockCombinedItemAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> blockItem;
    std::unique_ptr<BaseAST> blockCombinedItem;
    std::vector<BaseAST> blockItemsVec;
    std::string selfMinorType;
    void Dump(std::string &strOriginal) const override
    {
        // BlockItem
        if (selfMinorType[0] == '0')
        {
            blockItem->Dump(strOriginal);
        }
        // BlockCombinedItem BlockItem
        else
        {
            blockCombinedItem->Dump(strOriginal);
            blockItem->Dump(strOriginal);
        }
    }
};

class BlockItemAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> decl;
    std::unique_ptr<BaseAST> stmt;
    std::string selfMinorType;
    void Dump(std::string &strOriginal) const override
    {
        // decl
        if (selfMinorType[0] == '0')
        {
            decl->Dump(strOriginal);
        }
        // stmt
        else
        {
            stmt->Dump(strOriginal);
        }
    }
};

class StmtAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> matchedStmt;
    std::unique_ptr<BaseAST> openStmt;
    std::string selfMinorType;
    void Dump(std::string &strOriginal) const override
    {
        // matched_stmt
        if (selfMinorType[0] == '0')
        {
            matchedStmt->Dump(strOriginal);
        }
        // open_stmt
        else
        {
            openStmt->Dump(strOriginal);
        }
    }
};

class OtherStmtAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> lVal;
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> block;
    std::unique_ptr<BaseAST> stmt0;
    std::unique_ptr<BaseAST> stmt1;
    std::string selfMinorType;
    void Dump(std::string &strOriginal) const override
    {
        // LVal "=" Exp ";"
        if (selfMinorType[0] == '0')
        {
            std::string lValIdent = lVal->ReversalDump(strOriginal);
            int expVal = exp->CalExpressionValue();
            std::string expRegister = exp->ReversalDump(strOriginal);
            if (strcmp(lValIdent.c_str(), "error") != 0)
            {
                std::map<std::string, SyntaxElement>::iterator tempIter;
                for (int i = currMaxSyntaxVec; i >= 0; i--)
                {
                    tempIter = syntaxTableVec[i].find(lValIdent);
                    if (tempIter != syntaxTableVec[i].end())
                    {
                        SyntaxElement tempElement = SyntaxElement();
                        tempIter->second.isGivenNum = true;
                        tempIter->second.num = expVal;
                        std::string tempStr = "  store ";
                        tempStr += expRegister;
                        tempStr += ", @";
                        tempStr += tempIter->second.ident;
                        tempStr += "_";
                        tempStr += std::to_string(tempIter->second.index);
                        tempStr += "\n";
                        strOriginal += tempStr;
                        break;
                    }
                }
            }
        }
        // ';'
        else if (selfMinorType[0] == '1')
        {
            // do nothing
        }
        // Exp ';'
        else if (selfMinorType[0] == '2')
        {
            std::string tempStr = exp->ReversalDump(strOriginal);
        }
        // Block
        else if (selfMinorType[0] == '3')
        {
            block->Dump(strOriginal);
        }
        // "return" ';'
        else if (selfMinorType[0] == '4')
        {
            strOriginal += "  ret ";
            strOriginal += "\n";
        }
        // "return" Exp ";"
        else
        {
            // 最后一个level的寄存器或者数值
            // 如果是const或者数直接返回结果
            std::string lastLevelResult = exp->ReversalDump(strOriginal);
            strOriginal += "  ret ";
            strOriginal += lastLevelResult;
            strOriginal += "\n";
        }
    }
};

class MatchedStmtAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> matchedStmt0;
    std::unique_ptr<BaseAST> matchedStmt1;
    std::unique_ptr<BaseAST> otherStmt;
    std::string selfMinorType;
    void Dump(std::string &strOriginal) const override
    {
        // if exp then matched_stmt else matched_stmt
        if (selfMinorType[0] == '0')
        {
            std::string expResult = exp->ReversalDump(strOriginal);
            std::string currRegister;
            // register
            if (expResult[0] == '%')
            {
                currRegister = expResult;
            }
            // a variable
            else if (syntaxNameCnt.find(expResult) != syntaxNameCnt.end())
            {
                for (int i = currMaxSyntaxVec; i >= 0; i--)
                {
                    std::map<std::string, SyntaxElement>::iterator tempIter = syntaxTableVec[i].find(expResult);
                    if (tempIter != syntaxTableVec[i].end())
                    {
                        SyntaxElement tempElement = tempIter->second;
                        if (tempElement.isConstant == false)
                        {
                            currMaxRegister += 1;
                            currRegister = "%" + std::to_string(currMaxRegister);
                            std::string tempStr = "  ";
                            tempStr += currRegister;
                            tempStr += " = load @";
                            tempStr += tempElement.ident;
                            tempStr += "_";
                            tempStr += std::to_string(tempElement.index);
                            tempStr += "\n";
                            strOriginal += tempStr;
                        }
                        else
                        {
                            currRegister = tempElement.num;
                        }
                        break;
                    }
                }
            }
            // a number
            else
            {
                currRegister = expResult;
            }
            currThenBlockCnt += 1;
            int currBlock = currThenBlockCnt;

            strOriginal += "  br ";
            strOriginal += currRegister;
            strOriginal += ", %then_";
            strOriginal += std::to_string(currBlock);
            strOriginal += ", %end_";
            strOriginal += std::to_string(currBlock);
            strOriginal += "\n";
            strOriginal += "%then_";
            strOriginal += std::to_string(currBlock);
            strOriginal += ":\n";
            matchedStmt0->Dump(strOriginal);
            strOriginal += "  jump %end_";
            strOriginal += std::to_string(currBlock);
            strOriginal += "\n";

            strOriginal += "%else_";
            strOriginal += std::to_string(currBlock);
            strOriginal += ":\n";
            matchedStmt1->Dump(strOriginal);
            strOriginal += "  jump %end_";
            strOriginal += std::to_string(currBlock);
            strOriginal += "\n";

            strOriginal += "%end_";
            strOriginal += std::to_string(currBlock);
            strOriginal += ":\n";
        }
        // other
        else
        {
            otherStmt->Dump(strOriginal);
        }
    }
};

class OpenStmtAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> stmt;
    std::unique_ptr<BaseAST> matchedStmt;
    std::unique_ptr<BaseAST> openStmt;
    std::string selfMinorType;
    void Dump(std::string &strOriginal) const override
    {
        std::string expResult = exp->ReversalDump(strOriginal);
        std::string currRegister;
        // register
        if (expResult[0] == '%')
        {
            currRegister = expResult;
        }
        // a variable
        else if (syntaxNameCnt.find(expResult) != syntaxNameCnt.end())
        {
            for (int i = currMaxSyntaxVec; i >= 0; i--)
            {
                std::map<std::string, SyntaxElement>::iterator tempIter = syntaxTableVec[i].find(expResult);
                if (tempIter != syntaxTableVec[i].end())
                {
                    SyntaxElement tempElement = tempIter->second;
                    if (tempElement.isConstant == false)
                    {
                        currMaxRegister += 1;
                        currRegister = "%" + std::to_string(currMaxRegister);
                        std::string tempStr = "  ";
                        tempStr += currRegister;
                        tempStr += " = load @";
                        tempStr += tempElement.ident;
                        tempStr += "_";
                        tempStr += std::to_string(tempElement.index);
                        tempStr += "\n";
                        strOriginal += tempStr;
                    }
                    else
                    {
                        currRegister = tempElement.num;
                    }
                    break;
                }
            }
        }
        // a number
        else
        {
            currRegister = expResult;
        }
        currThenBlockCnt += 1;
        int currBlock = currThenBlockCnt;
        // if exp then stmt
        if (selfMinorType[0] == '0')
        {
            strOriginal += "  br ";
            strOriginal += currRegister;
            strOriginal += ", %then_";
            strOriginal += std::to_string(currBlock);
            strOriginal += ", %end_";
            strOriginal += std::to_string(currBlock);
            strOriginal += "\n";
            strOriginal += "%then_";
            strOriginal += std::to_string(currBlock);
            strOriginal += ":\n";
            stmt->Dump(strOriginal);
            strOriginal += "  jump %end_";
            strOriginal += std::to_string(currBlock);
            strOriginal += "\n";
        }
        // if exp then matched_stmt else open_stmt
        else
        {
            strOriginal += "  br ";
            strOriginal += currRegister;
            strOriginal += ", %then_";
            strOriginal += std::to_string(currBlock);
            strOriginal += ", %end_";
            strOriginal += std::to_string(currBlock);
            strOriginal += "\n";
            strOriginal += "%then_";
            strOriginal += std::to_string(currBlock);
            strOriginal += ":\n";
            matchedStmt->Dump(strOriginal);
            strOriginal += "  jump %end_";
            strOriginal += std::to_string(currBlock);
            strOriginal += "\n";

            strOriginal += "%else_";
            strOriginal += std::to_string(currBlock);
            strOriginal += ":\n";
            openStmt->Dump(strOriginal);
            strOriginal += "  jump %end_";
            strOriginal += std::to_string(currBlock);
            strOriginal += "\n";
        }
        strOriginal += "%end_";
        strOriginal += std::to_string(currBlock);
        strOriginal += ":\n";
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
    // 计算exp表达式的值，在lv4.1常量赋值中使用
    int CalExpressionValue() override
    {
        return lOrExp->CalExpressionValue();
    }
};

class PrimaryExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> Number;
    std::unique_ptr<BaseAST> lVal;
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
        else if (selfMinorType[0] == '1')
        {
            return Number->ReversalDump(strOriginal);
        }
        // LVal
        else
        {
            std::string lValIdent = lVal->ReversalDump(strOriginal);
            std::map<std::string, SyntaxElement>::iterator tempIter;
            for (int i = currMaxSyntaxVec; i >= 0; i--)
            {
                tempIter = syntaxTableVec[i].find(lValIdent);
                if (tempIter != syntaxTableVec[i].end())
                {
                    SyntaxElement tempElement = tempIter->second;
                    // int variable, should load it into a register
                    if (tempElement.isConstant == false)
                    {
                        currMaxRegister += 1;
                        std::string currRegister = std::to_string(currMaxRegister);
                        std::string tempStr = "  %";
                        tempStr += currRegister;
                        tempStr += " = load @";
                        tempStr += tempIter->second.ident;
                        tempStr += "_";
                        tempStr += std::to_string(tempIter->second.index);
                        tempStr += "\n";
                        strOriginal += tempStr;
                        return "%" + currRegister;
                    }
                    else
                    {
                        return std::to_string(tempElement.num);
                    }
                    break;
                }
            }
            return "error in primary exp";
        }
    }
    int CalExpressionValue() override
    {
        // exp
        if (selfMinorType[0] == '0')
        {
            return exp->CalExpressionValue();
        }
        // number
        else if (selfMinorType[0] == '1')
        {
            return Number->CalExpressionValue();
        }
        // LVal
        else
        {
            return lVal->CalExpressionValue();
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
    int CalExpressionValue() override
    {
        return Number;
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
    int CalExpressionValue() override
    {
        // PrimaryExp
        if (selfMinorType[0] == '0')
        {
            return primaryExp->CalExpressionValue();
        }
        // UnaryOp UnaryExp
        else
        {
            int currOperator = unaryOp->CalExpressionValue();
            if (currOperator == 0)
            {
                return 0 - unaryExp->CalExpressionValue();
            }
            else if (currOperator == 1)
            {
                return int(!unaryExp->CalExpressionValue());
            }
            else
            {
                return unaryExp->CalExpressionValue();
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
    int CalExpressionValue() override
    {
        if (unaryOp[0] == '-')
        {
            return 0;
        }
        else if (unaryOp[0] == '!')
        {
            return 1;
        }
        else
        {
            return 2;
        }
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
            registerCnt++;
            std::string mulLastRegister = mulExp->ReversalDump(strOriginal);
            std::string unaryLastRegister = unaryExp->ReversalDump(strOriginal);
            currMaxRegister++;
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
    int CalExpressionValue() override
    {
        // UnaryExp
        if (selfMinorType[0] == '0')
        {
            return unaryExp->CalExpressionValue();
        }
        // MulExp ('*' | '/' | '%') UnaryExp
        else
        {
            if (mulOperator[0] == '*')
            {
                return mulExp->CalExpressionValue() * unaryExp->CalExpressionValue();
            }
            else if (mulOperator[0] == '/')
            {
                return mulExp->CalExpressionValue() / unaryExp->CalExpressionValue();
            }
            else
            {
                return mulExp->CalExpressionValue() % unaryExp->CalExpressionValue();
            }
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
            registerCnt++;
            std::string addLastRegister = addExp->ReversalDump(strOriginal);
            std::string mulLastRegister = mulExp->ReversalDump(strOriginal);
            currMaxRegister++;
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
    int CalExpressionValue() override
    {
        // MulExp
        if (selfMinorType[0] == '0')
        {
            return mulExp->CalExpressionValue();
        }
        // AddExp ('+' | '-') MulExp
        else
        {
            if (addOperator[0] == '+')
            {
                return addExp->CalExpressionValue() + mulExp->CalExpressionValue();
            }
            else
            {
                return addExp->CalExpressionValue() - mulExp->CalExpressionValue();
            }
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
            registerCnt++;
            std::string relLastRegister = relExp->ReversalDump(strOriginal);
            std::string addLastRegister = addExp->ReversalDump(strOriginal);
            currMaxRegister++;
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
    int CalExpressionValue() override
    {
        // AddExp
        if (selfMinorType[0] == '0')
        {
            return addExp->CalExpressionValue();
        }
        // RelExp ("<" | ">" | "<=" | ">=") AddExp
        else
        {
            if (strcmp(compOperator.c_str(), "<") == 0)
            {
                return int(relExp->CalExpressionValue() < addExp->CalExpressionValue());
            }
            else if (strcmp(compOperator.c_str(), ">") == 0)
            {
                return int(relExp->CalExpressionValue() > addExp->CalExpressionValue());
            }
            else if (strcmp(compOperator.c_str(), "<=") == 0)
            {
                return int(relExp->CalExpressionValue() <= addExp->CalExpressionValue());
            }
            else
            {
                return int(relExp->CalExpressionValue() >= addExp->CalExpressionValue());
            }
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
            registerCnt++;
            std::string eqLastRegister = eqExp->ReversalDump(strOriginal);
            std::string relLastRegister = relExp->ReversalDump(strOriginal);
            currMaxRegister++;
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
    int CalExpressionValue() override
    {
        // RepExp
        if (selfMinorType[0] == '0')
        {
            return relExp->CalExpressionValue();
        }
        // EqExp ("==" | "!=") RelExp;
        else
        {
            if (strcmp(eqOperator.c_str(), "==") == 0)
            {
                return int(eqExp->CalExpressionValue() == relExp->CalExpressionValue());
            }
            else
            {
                return int(eqExp->CalExpressionValue() != relExp->CalExpressionValue());
            }
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
            registerCnt++;
            std::string lAndLastRegister = lAndExp->ReversalDump(strOriginal);
            std::string eqLastRegister = eqExp->ReversalDump(strOriginal);

            // curr_0 = ne lAndExp, 0
            currMaxRegister++;
            std::string currRegister_0 = std::to_string(currMaxRegister);
            std::string tempStr = "  %";
            tempStr += currRegister_0;
            tempStr += " = ne ";
            tempStr += lAndLastRegister;
            tempStr += ", 0\n";
            // curr_1 = ne eqExp, 0
            currMaxRegister++;
            std::string currRegister_1 = std::to_string(currMaxRegister);
            tempStr += "  %";
            tempStr += currRegister_1;
            tempStr += " = ne ";
            tempStr += eqLastRegister;
            tempStr += ", 0\n";
            // curr_2 = and curr_0, curr_1
            currMaxRegister++;
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
    int CalExpressionValue() override
    {
        // EqExp
        if (selfMinorType[0] == '0')
        {
            return eqExp->CalExpressionValue();
        }
        // LAndExp "&&" EqExp
        else
        {
            return int(lAndExp->CalExpressionValue() && eqExp->CalExpressionValue());
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
            registerCnt++;
            std::string lOrLastRegister = lOrExp->ReversalDump(strOriginal);
            std::string lAndLastRegister = lAndExp->ReversalDump(strOriginal);

            // curr_0 = ne lOrExp, 0
            currMaxRegister++;
            std::string currRegister_0 = std::to_string(currMaxRegister);
            std::string tempStr = "  %";
            tempStr += currRegister_0;
            tempStr += " = ne ";
            tempStr += lOrLastRegister;
            tempStr += ", 0\n";
            // curr_1 = ne LAndExp, 0
            currMaxRegister++;
            std::string currRegister_1 = std::to_string(currMaxRegister);
            tempStr += "  %";
            tempStr += currRegister_1;
            tempStr += " = ne ";
            tempStr += lAndLastRegister;
            tempStr += ", 0\n";
            // curr_2 = or curr_0, curr_1
            currMaxRegister++;
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
    int CalExpressionValue() override
    {
        // lAndExp
        if (selfMinorType[0] == '0')
        {
            return lAndExp->CalExpressionValue();
        }
        // LOrExp "||" LAndExp
        else
        {
            return int(lOrExp->CalExpressionValue() || lAndExp->CalExpressionValue());
        }
    }
};

class DeclAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> constDecl;
    std::unique_ptr<BaseAST> varDecl;
    std::string selfMinorType;
    void Dump(std::string &strOriginal) const override
    {
        if (selfMinorType[0] == '0')
        {
            constDecl->Dump(strOriginal);
        }
        else
        {
            varDecl->Dump(strOriginal);
        }
    }
};

class ConstDeclAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> bType;
    std::unique_ptr<BaseAST> constCombinedDef;
    void Dump(std::string &strOriginal) const override
    {
        constCombinedDef->Dump(strOriginal);
    }
};

class BTypeAST : public BaseAST
{
public:
    std::string bType;
    void Dump(std::string &strOriginal) const override {}
    std::string ReversalDump(std::string &strOriginal) override
    {
        return bType;
    }
};

class ConstDefAST : public BaseAST
{
public:
    std::string ident;
    std::unique_ptr<BaseAST> constInitVal;
    void Dump(std::string &strOriginal) const override
    {
        std::map<std::string, int>::iterator constInitIter = syntaxNameCnt.find(ident);
        if (constInitIter != syntaxNameCnt.end())
        {
            SyntaxElement tempElement = SyntaxElement();
            tempElement.ident = ident;
            tempElement.isConstant = true;
            tempElement.isGivenNum = true;
            tempElement.num = constInitVal->CalExpressionValue();
            tempElement.index = constInitIter->second + 1;
            constInitIter->second += 1;
            syntaxTableVec[currMaxSyntaxVec].insert(std::make_pair(ident, tempElement));
        }
        else
        {
            SyntaxElement tempElement = SyntaxElement();
            tempElement.ident = ident;
            tempElement.isConstant = true;
            tempElement.isGivenNum = true;
            tempElement.num = constInitVal->CalExpressionValue();
            tempElement.index = 0;
            syntaxNameCnt.insert(std::make_pair(ident, 0));
            syntaxTableVec[currMaxSyntaxVec].insert(std::make_pair(ident, tempElement));
        }
    }
    int CalExpressionValue() override
    {
        return constInitVal->CalExpressionValue();
    }
};

class ConstCombinedDefAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> constCombinedDef;
    std::unique_ptr<BaseAST> constDef;
    std::vector<BaseAST> constDefVec;
    std::string selfMinorType;
    void Dump(std::string &strOriginal) const override
    {
        // ConstDef
        if (selfMinorType[0] == '0')
        {
            constDef->Dump(strOriginal);
        }
        // ConstCombinedDef ',' ConstDef
        else
        {
            constCombinedDef->Dump(strOriginal);
            constDef->Dump(strOriginal);
        }
    }
};

class ConstInitValAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> constExp;
    void Dump(std::string &strOriginal) const override {}
    int CalExpressionValue() override
    {
        return constExp->CalExpressionValue();
    }
};

class VarDeclAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> bType;
    std::unique_ptr<BaseAST> varCombinedDef;
    void Dump(std::string &strOriginal) const override
    {
        varCombinedDef->Dump(strOriginal);
    }
};

class VarCombinedDefAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> varCombinedDef;
    std::unique_ptr<BaseAST> varDef;
    std::vector<BaseAST> varDefVec;
    std::string selfMinorType;
    void Dump(std::string &strOriginal) const override
    {
        // VarDef
        if (selfMinorType[0] == '0')
        {
            varDef->Dump(strOriginal);
        }
        // VarCombinedDef ',' VarDef
        else
        {
            varCombinedDef->Dump(strOriginal);
            varDef->Dump(strOriginal);
        }
    }
};

class VarDefAST : public BaseAST
{
public:
    std::string ident;
    std::unique_ptr<BaseAST> initVal;
    std::string selfMinorType;
    void Dump(std::string &strOriginal) const override
    {
        // IDENT
        if (selfMinorType[0] == '0')
        {
            std::map<std::string, int>::iterator initIter = syntaxNameCnt.find(ident);
            SyntaxElement tempElement = SyntaxElement();
            if (initIter != syntaxNameCnt.end())
            {
                tempElement.ident = ident;
                tempElement.isConstant = false;
                tempElement.isGivenNum = false;
                tempElement.index = initIter->second + 1;
                initIter->second += 1;
                syntaxTableVec[currMaxSyntaxVec].insert(std::make_pair(ident, tempElement));
            }
            else
            {
                tempElement.ident = ident;
                tempElement.isConstant = false;
                tempElement.isGivenNum = false;
                tempElement.index = 0;
                syntaxNameCnt.insert(std::make_pair(ident, 0));
                syntaxTableVec[currMaxSyntaxVec].insert(std::make_pair(ident, tempElement));
            }
            strOriginal += "@";
            strOriginal += ident;
            strOriginal += "_";
            strOriginal += std::to_string(tempElement.index);
            strOriginal += " = alloc i32\n";
        }
        // IDENT = InitVal
        else
        {
            int calInitResult = initVal->CalExpressionValue();
            std::map<std::string, int>::iterator initIter = syntaxNameCnt.find(ident);
            SyntaxElement tempElement = SyntaxElement();
            if (initIter != syntaxNameCnt.end())
            {
                tempElement.ident = ident;
                tempElement.isConstant = false;
                tempElement.isGivenNum = true;
                tempElement.num = calInitResult;
                tempElement.index = initIter->second + 1;
                initIter->second += 1;
                syntaxTableVec[currMaxSyntaxVec].insert(std::make_pair(ident, tempElement));
            }
            else
            {
                tempElement.ident = ident;
                tempElement.isConstant = false;
                tempElement.isGivenNum = true;
                tempElement.num = calInitResult;
                tempElement.index = 0;
                syntaxNameCnt.insert(std::make_pair(ident, 0));
                syntaxTableVec[currMaxSyntaxVec].insert(std::make_pair(ident, tempElement));
            }
            strOriginal += "  @";
            strOriginal += ident;
            strOriginal += "_";
            strOriginal += std::to_string(tempElement.index);
            strOriginal += " = alloc i32\n";
            strOriginal += "  store ";
            strOriginal += std::to_string(calInitResult);
            strOriginal += ", @";
            strOriginal += ident;
            strOriginal += "_";
            strOriginal += std::to_string(tempElement.index);
            strOriginal += "\n";
        }
    }
    int CalExpressionValue() override
    {
        return initVal->CalExpressionValue();
    }
};

class InitValAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> exp;
    void Dump(std::string &strOriginal) const override {}
    int CalExpressionValue() override
    {
        return exp->CalExpressionValue();
    }
};

class LValAST : public BaseAST
{
public:
    std::string ident;
    void Dump(std::string &strOriginal) const override {}
    std::string ReversalDump(std::string &strOriginal) override
    {
        if (syntaxNameCnt.find(ident) == syntaxNameCnt.end())
        {
            return "error in lval ast";
        }
        else
        {
            return ident;
        }
    }
    int CalExpressionValue() override
    {
        if (syntaxNameCnt.find(ident) == syntaxNameCnt.end())
        {
            return -1;
        }
        else
        {
            for (int i = currMaxSyntaxVec; i >= 0; i--)
            {
                std::map<std::string, SyntaxElement>::iterator tempElement = syntaxTableVec[i].find(ident);
                if (tempElement != syntaxTableVec[i].end())
                {
                    return tempElement->second.num;
                }
            }
        }
        return -1;
    }
};

class ConstExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> exp;
    void Dump(std::string &strOriginal) const override {}
    std::string ReversalDump(std::string &strOriginal) override
    {
        return exp->ReversalDump(strOriginal);
    }
    int CalExpressionValue() override
    {
        return exp->CalExpressionValue();
    }
};