#pragma once
#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <cstring>
#include <vector>
#include <map>

extern int registerCnt;
extern int currMaxRegister;
extern std::map<std::string, int> syntaxTable;

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
    std::unique_ptr<BaseAST> blockCombinedItem;
    void Dump(std::string &strOriginal) const override
    {
        strOriginal += "%entry: ";
        strOriginal += "\n";
        blockCombinedItem->Dump(strOriginal);
        strOriginal += "\n";
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
    std::vector<BaseAST> blockItemsVec;
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
    std::unique_ptr<BaseAST> exp;
    void Dump(std::string &strOriginal) const override
    {
        // 最后一个level的寄存器或者数值
        std::string lastLevelResult = exp->ReversalDump(strOriginal);
        strOriginal += "  ret ";
        strOriginal += lastLevelResult;
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
            return lVal->ReversalDump(strOriginal);
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
    void Dump(std::string &strOriginal) const override 
    {
        constDecl->Dump(strOriginal);
    }
};

class ConstDeclAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> bType;
    std::unique_ptr<BaseAST> constCombinedDef;
    void Dump(std::string &strOriginal ) const override 
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
        std::string calConstResult = std::to_string(constInitVal->CalExpressionValue());
        if (syntaxTable.find(calConstResult) == syntaxTable.end())
        {
            syntaxTable.insert(std::make_pair(ident, atoi(calConstResult.c_str())));
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

class LValAST : public BaseAST 
{
public:
    std::string ident;
    void Dump(std::string &strOriginal) const override {}
    std::string ReversalDump(std::string &strOriginal) override 
    {
        if (syntaxTable.find(ident) == syntaxTable.end())
        {
            return "syntaxTable find ident error!";
        }
        else
        {
            return std::to_string(syntaxTable.at(ident));
        }
    }
    int CalExpressionValue() override
    {
        if (syntaxTable.find(ident) == syntaxTable.end())
        {
            return -1;
        }
        else
        {
            return syntaxTable.at(ident);
        }
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