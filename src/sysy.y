%code requires {
  #include <memory>
  #include <string>
}

%{

#include <iostream>
#include <memory>
#include <string>
#include "AST.hpp"

// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}

%parse-param { std::unique_ptr<BaseAST> &ast }

%union {
  std::string *str_val;
  int int_val;
  class BaseAST *ast_val;
}

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT RETURN
%token <str_val> IDENT
%token <int_val> INT_CONST

%type <ast_val> FuncDef FuncType Block Stmt Exp PrimaryExp Number UnaryExp UnaryOp MulExp AddExp

%%

CompUnit
  : FuncDef {
    auto comp_unit = make_unique<CompUnitAST>();
    comp_unit->func_def = unique_ptr<BaseAST>($1);
    ast = move(comp_unit);
  }
  ;

FuncDef
  : FuncType IDENT '(' ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = unique_ptr<BaseAST>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->block = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  ;

FuncType
  : INT {
    auto ast = new FuncTypeAST();
    ast->funcType = "int";
    $$ = ast;
  }
  ;

Block
  : '{' Stmt '}' {
    auto ast = new BlockAST();
    ast->stmt = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

Stmt
  : RETURN Exp ';' {
    auto ast = new StmtAST();
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

Exp
  : AddExp {
    auto ast = new ExpAST();
    ast->addExp = unique_ptr<BaseAST>($1);
    $$ = ast;
  };

PrimaryExp
  : '(' Exp ')' {
    auto ast = new PrimaryExpAST();
    ast->selfMinorType = "0";
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  |
    Number {
    auto ast = new PrimaryExpAST();
    ast->selfMinorType = "1";
    ast->Number = unique_ptr<BaseAST>($1);
    $$ = ast;
  };

Number
  : INT_CONST {
    auto ast = new NumberAST();
    ast->Number = $1;
    $$ = ast;
  };

UnaryExp
  : PrimaryExp {
    auto ast = new UnaryExpAST();
    ast->selfMinorType = "0";
    ast->primaryExp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  |
    UnaryOp UnaryExp {
    auto ast = new UnaryExpAST();
    ast->selfMinorType = "1";
    ast->unaryOp = unique_ptr<BaseAST>($1);
    ast->unaryExp = unique_ptr<BaseAST>($2);
    $$ = ast;
  };

UnaryOp
  : '+' {
    auto ast = new UnaryOpAST();
    ast->unaryOp = "+";
    $$ = ast;
  }
  |
    '-' {
    auto ast = new UnaryOpAST();
    ast->unaryOp = "-";
    $$ = ast;
  }
  |
    '!' {
    auto ast = new UnaryOpAST();
    ast->unaryOp = "!";
    $$ = ast;
  };

MulExp
  : UnaryExp {
    auto ast = new MulExpAST();
    ast->selfMinorType = "0";
    ast->unaryExp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | MulExp '*' UnaryExp {
    auto ast = new MulExpAST();
    ast->selfMinorType = "1";
    ast->mulOperator = "*";
    ast->mulExp = unique_ptr<BaseAST>($1);
    ast->unaryExp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | MulExp '/' UnaryExp {
    auto ast = new MulExpAST();
    ast->selfMinorType = "1";
    ast->mulOperator = "/";
    ast->mulExp = unique_ptr<BaseAST>($1);
    ast->unaryExp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | MulExp '%' UnaryExp {
    auto ast = new MulExpAST();
    ast->selfMinorType = "1";
    ast->mulOperator = "%";
    ast->mulExp = unique_ptr<BaseAST>($1);
    ast->unaryExp = unique_ptr<BaseAST>($3);
    $$ = ast;
  };

AddExp
  : MulExp {
    auto ast = new AddExpAST();
    ast->selfMinorType = "0";
    ast->mulExp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | AddExp '+' MulExp {
    auto ast = new AddExpAST();
    ast->selfMinorType = "1";
    ast->addOperator = "+";
    ast->addExp = unique_ptr<BaseAST>($1);
    ast->mulExp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | AddExp '-' MulExp {
    auto ast = new AddExpAST();
    ast->selfMinorType = "1";
    ast->addOperator = "-";
    ast->addExp = unique_ptr<BaseAST>($1);
    ast->mulExp = unique_ptr<BaseAST>($3);
    $$ = ast;
  };

%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}