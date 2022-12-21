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
%token INT RETURN LT GT LE GE EQ NE LAND LOR CONST
%token <str_val> IDENT
%token <int_val> INT_CONST

%type <ast_val> FuncDef FuncType Block BlockCombinedItem BlockItem Stmt Exp PrimaryExp Number UnaryExp UnaryOp MulExp AddExp RelExp EqExp LAndExp LOrExp Decl ConstDecl BType ConstDef ConstCombinedDef ConstInitVal LVal ConstExp

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
  : '{' BlockCombinedItem '}' {
    auto ast = new BlockAST();
    ast->blockCombinedItem = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

BlockCombinedItem
  : BlockItem {
    auto ast = new BlockCombinedItemAST();
    ast->selfMinorType = "0";
    ast->blockItem = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | BlockCombinedItem BlockItem {
    auto ast = new BlockCombinedItemAST();
    ast->selfMinorType = "1";
    ast->blockCombinedItem = unique_ptr<BaseAST>($1);
    ast->blockItem = unique_ptr<BaseAST>($2);
    $$ = ast;
  };

BlockItem
  : Decl {
    auto ast = new BlockItemAST();
    ast->selfMinorType = "0";
    ast->decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Stmt {
    auto ast = new BlockItemAST();
    ast->selfMinorType = "1";
    ast->stmt = unique_ptr<BaseAST>($1);
    $$ = ast;
  };

Stmt
  : RETURN Exp ';' {
    auto ast = new StmtAST();
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

Exp
  : LOrExp {
    auto ast = new ExpAST();
    ast->lOrExp = unique_ptr<BaseAST>($1);
    $$ = ast;
  };

PrimaryExp
  : '(' Exp ')' {
    auto ast = new PrimaryExpAST();
    ast->selfMinorType = "0";
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | Number {
    auto ast = new PrimaryExpAST();
    ast->selfMinorType = "1";
    ast->Number = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LVal {
    auto ast = new PrimaryExpAST();
    ast->selfMinorType = "2";
    ast->lVal = unique_ptr<BaseAST>($1);
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

RelExp
  : AddExp {
    auto ast = new RelExpAST();
    ast->selfMinorType = "0";
    ast->addExp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | RelExp LT AddExp {
    auto ast = new RelExpAST();
    ast->selfMinorType = "1";
    ast->compOperator = "<";
    ast->relExp = unique_ptr<BaseAST>($1);
    ast->addExp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | RelExp GT AddExp {
    auto ast = new RelExpAST();
    ast->selfMinorType = "1";
    ast->compOperator = ">";
    ast->relExp = unique_ptr<BaseAST>($1);
    ast->addExp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | RelExp LE AddExp {
    auto ast = new RelExpAST();
    ast->selfMinorType = "1";
    ast->compOperator = "<=";
    ast->relExp = unique_ptr<BaseAST>($1);
    ast->addExp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | RelExp GE AddExp {
    auto ast = new RelExpAST();
    ast->selfMinorType = "1";
    ast->compOperator = ">=";
    ast->relExp = unique_ptr<BaseAST>($1);
    ast->addExp = unique_ptr<BaseAST>($3);
    $$ = ast;
  };

EqExp
  : RelExp {
    auto ast = new EqExpAST();
    ast->selfMinorType = "0";
    ast->relExp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | EqExp EQ RelExp {
    auto ast = new EqExpAST();
    ast->selfMinorType = "1";
    ast->eqOperator = "==";
    ast->eqExp = unique_ptr<BaseAST>($1);
    ast->relExp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | EqExp NE RelExp {
    auto ast = new EqExpAST();
    ast->selfMinorType = "1";
    ast->eqOperator = "!=";
    ast->eqExp = unique_ptr<BaseAST>($1);
    ast->relExp = unique_ptr<BaseAST>($3);
    $$ = ast;
  };

LAndExp
  : EqExp {
    auto ast = new LAndExpAST();
    ast->selfMinorType = "0";
    ast->eqExp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LAndExp LAND EqExp {
    auto ast = new LAndExpAST();
    ast->selfMinorType = "1";
    ast->lAndOperator = "&&";
    ast->lAndExp = unique_ptr<BaseAST>($1);
    ast->eqExp = unique_ptr<BaseAST>($3);
    $$ = ast;
  };

LOrExp
  : LAndExp {
    auto ast = new LOrExpAST();
    ast->selfMinorType = "0";
    ast->lAndExp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LOrExp LOR LAndExp {
    auto ast = new LOrExpAST();
    ast->selfMinorType = "1";
    ast->lOrOperator = "||";
    ast->lOrExp = unique_ptr<BaseAST>($1);
    ast->lAndExp = unique_ptr<BaseAST>($3);
    $$ = ast;
  };

Decl
  : ConstDecl {
    auto ast = new DeclAST();
    ast->constDecl = unique_ptr<BaseAST>($1);
    $$ = ast;
  };

ConstDecl
  : CONST BType ConstCombinedDef ';' {
    auto ast = new ConstDeclAST();
    ast->bType = unique_ptr<BaseAST>($2);
    ast->constCombinedDef = unique_ptr<BaseAST>($3);
    $$ = ast;
  };

BType
  : INT {
    auto ast = new BTypeAST();
    ast->bType = "int";
    $$ = ast;
  };

ConstDef
  : IDENT '=' ConstInitVal {
    auto ast = new ConstDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->constInitVal = unique_ptr<BaseAST>($3);
    $$ = ast;
  };

ConstCombinedDef
  : ConstDef {
    auto ast = new ConstCombinedDefAST();
    ast->selfMinorType = "0";
    ast->constDef = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | ConstCombinedDef ',' ConstDef {
    auto ast = new ConstCombinedDefAST();
    ast->selfMinorType = "1";
    ast->constCombinedDef = unique_ptr<BaseAST>($1);
    ast->constDef = unique_ptr<BaseAST>($3);
    $$ = ast;
  };

ConstInitVal
  : ConstExp {
    auto ast = new ConstInitValAST();
    ast->constExp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }

LVal
  : IDENT {
    auto ast = new LValAST();
    ast->ident = *unique_ptr<string>($1);
    $$ = ast;
  };

ConstExp
  : Exp {
    auto ast = new ConstExpAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  };

%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}