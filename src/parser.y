%{
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include "ast.h"

extern int yylex();
extern int yylineno;
extern FILE* yyin;
extern void yyerror(const char* s);
Module* root = nullptr;
Function* currentFunc = nullptr;
%}

%union {
    int ival;
    float fval;
    char* sval;
    ASTNode* node;
    Expression* expr;
    Statement* stmt;
    Instruction* inst;
    Function* func;
    Module* module;
    RegDecl* regdecl;
    ParamDecl* paramdecl;
}

%token <ival> INTEGER_LITERAL
%token <fval> FLOAT_LITERAL
%token <sval> STRING_LITERAL IDENTIFIER REGISTER BUILTIN_VAR
%token <sval> DTYPE_8 DTYPE_16 DTYPE_32 DTYPE_64 DTYPE_F32 DTYPE_F64
%token <sval> MODIFIER LABEL LABEL_REF

%token VERSION TARGET ADDRESS_SIZE VISIBLE ENTRY GLOBL
%token PARAM REG PRED RET
%token LD ST MOV ADD SUB MUL DIV MAD FMA CVTA SETP BRA
%token GLOBAL SHARED LOCAL CONST TEXTURE
%token DOT COMMA SEMICOLON COLON LPAREN RPAREN LBRACE RBRACE LBRACKET RBRACKET LT GT AT BANG

%type <module> module
%type <func> function
%type <stmt> statement
%type <inst> instruction
%type <inst> ld_instruction st_instruction mov_instruction arith_instruction cvta_instruction setp_instruction
%type <expr> operand
%type <regdecl> register_decl
%type <paramdecl> parameter_decl

%start module

%%

module:
    {
        root = new Module();
    }
    module_items {
        $$ = root;
    }
    ;

module_items:
    module_items module_item
    | module_item
    ;

module_item:
    directive
    | function {
        if (root && $1) {
            root->addFunction(std::unique_ptr<Function>($1));
        }
    }
    ;

directive:
    DOT VERSION FLOAT_LITERAL {
        if (root) {
            char version_str[32];
            sprintf(version_str, "%.1f", $3);
            root->setVersion(version_str);
        }
    }
    | DOT TARGET IDENTIFIER {
        if (root) {
            root->setTarget($3);
        }
    }
    | DOT ADDRESS_SIZE INTEGER_LITERAL {
        if (root) {
            root->setAddressSize($3);
        }
    }
    ;

function:
    DOT VISIBLE DOT ENTRY IDENTIFIER LPAREN {
        currentFunc = new Function("visible", "entry", $5);
    }
    param_list RPAREN LBRACE
    reg_list
    stmt_list RBRACE {
        $$ = currentFunc;
        currentFunc = nullptr;
    }
    ;

param_list:
    param_list COMMA parameter_decl {
        if (currentFunc && $3) {
            currentFunc->addParameter(std::unique_ptr<ParamDecl>($3));
        }
    }
    | parameter_decl {
        if (currentFunc && $1) {
            currentFunc->addParameter(std::unique_ptr<ParamDecl>($1));
        }
    }
    | /* empty */
    ;

parameter_decl:
    DOT PARAM DOT DTYPE_32 IDENTIFIER {
        $$ = new ParamDecl($4, $5);
    }
    | DOT PARAM DOT DTYPE_64 IDENTIFIER {
        $$ = new ParamDecl($4, $5);
    }
    ;

reg_list:
    reg_list register_decl {
        if (currentFunc && $2) {
            currentFunc->addRegister(std::unique_ptr<RegDecl>($2));
        }
    }
    | register_decl {
        if (currentFunc && $1) {
            currentFunc->addRegister(std::unique_ptr<RegDecl>($1));
        }
    }
    | /* empty */
    ;

register_decl:
    DOT REG DOT PRED REGISTER LT INTEGER_LITERAL GT SEMICOLON {
        $$ = new RegDecl("pred", $5, $7);
    }
    | DOT REG DOT DTYPE_F32 REGISTER LT INTEGER_LITERAL GT SEMICOLON {
        $$ = new RegDecl($4, $5, $7);
    }
    | DOT REG DOT DTYPE_32 REGISTER LT INTEGER_LITERAL GT SEMICOLON {
        $$ = new RegDecl($4, $5, $7);
    }
    | DOT REG DOT DTYPE_64 REGISTER LT INTEGER_LITERAL GT SEMICOLON {
        $$ = new RegDecl($4, $5, $7);
    }
    ;

stmt_list:
    stmt_list statement {
        if (currentFunc && $2) {
            currentFunc->addStatement(std::unique_ptr<Statement>($2));
        }
    }
    | statement {
        if (currentFunc && $1) {
            currentFunc->addStatement(std::unique_ptr<Statement>($1));
        }
    }
    | /* empty */
    ;

statement:
    instruction SEMICOLON { $$ = $1; }
    | LABEL {
        std::string labelStr($1);
        if (!labelStr.empty() && labelStr.back() == ':') {
            labelStr.pop_back();
        }
        $$ = new Label(labelStr);
    }
    | AT REGISTER BRA LABEL_REF SEMICOLON {
        $$ = new BranchInst($4, std::make_unique<Register>("pred", $2), false);
    }
    | RET SEMICOLON {
        $$ = new ReturnInst();
    }
    ;

instruction:
    ld_instruction { $$ = $1; }
    | st_instruction { $$ = $1; }
    | mov_instruction { $$ = $1; }
    | arith_instruction { $$ = $1; }
    | cvta_instruction { $$ = $1; }
    | setp_instruction { $$ = $1; }
    ;

ld_instruction:
    LD DOT PARAM DOT DTYPE_64 REGISTER COMMA LBRACKET IDENTIFIER RBRACKET {
        auto inst = new Instruction("ld");
        inst->addModifier("param");
        inst->addModifier($5);
        inst->addOperand(std::make_unique<Register>("u64", $6));
        inst->addOperand(std::make_unique<Parameter>($9));
        $$ = inst;
    }
    | LD DOT PARAM DOT DTYPE_32 REGISTER COMMA LBRACKET IDENTIFIER RBRACKET {
        auto inst = new Instruction("ld");
        inst->addModifier("param");
        inst->addModifier($5);
        inst->addOperand(std::make_unique<Register>("u32", $6));
        inst->addOperand(std::make_unique<Parameter>($9));
        $$ = inst;
    }
    | LD DOT GLOBAL DOT DTYPE_F32 REGISTER COMMA LBRACKET REGISTER RBRACKET {
        auto inst = new Instruction("ld");
        inst->addModifier("global");
        inst->addModifier($5);
        inst->addOperand(std::make_unique<Register>("f32", $6));
        inst->addOperand(std::make_unique<MemoryAccess>(std::make_unique<Register>("u64", $9)));
        $$ = inst;
    }
    ;

st_instruction:
    ST DOT GLOBAL DOT DTYPE_F32 LBRACKET REGISTER RBRACKET COMMA REGISTER {
        auto inst = new Instruction("st");
        inst->addModifier("global");
        inst->addModifier($5);
        inst->addOperand(std::make_unique<MemoryAccess>(std::make_unique<Register>("u64", $7)));
        inst->addOperand(std::make_unique<Register>("f32", $10));
        $$ = inst;
    }
    ;

mov_instruction:
    MOV DOT DTYPE_32 REGISTER COMMA BUILTIN_VAR {
        auto inst = new Instruction("mov");
        inst->addModifier($3);
        inst->addOperand(std::make_unique<Register>("u32", $4));
        inst->addOperand(std::make_unique<BuiltinVar>($6));
        $$ = inst;
    }
    ;

arith_instruction:
    MAD DOT MODIFIER DOT DTYPE_32 REGISTER COMMA REGISTER COMMA REGISTER COMMA REGISTER {
        auto inst = new Instruction("mad");
        inst->addModifier($3);
        inst->addModifier($5);
        inst->addOperand(std::make_unique<Register>("u32", $6));
        inst->addOperand(std::make_unique<Register>("u32", $8));
        inst->addOperand(std::make_unique<Register>("u32", $10));
        inst->addOperand(std::make_unique<Register>("u32", $12));
        $$ = inst;
    }
    | MUL DOT MODIFIER DOT DTYPE_32 REGISTER COMMA REGISTER COMMA INTEGER_LITERAL {
        auto inst = new Instruction("mul");
        inst->addModifier($3);
        inst->addModifier($5);
        inst->addOperand(std::make_unique<Register>("u64", $6));
        inst->addOperand(std::make_unique<Register>("u32", $8));
        inst->addOperand(std::make_unique<IntLiteral>($10));
        $$ = inst;
    }
    | ADD DOT DTYPE_64 REGISTER COMMA REGISTER COMMA REGISTER {
        auto inst = new Instruction("add");
        inst->addModifier($3);
        inst->addOperand(std::make_unique<Register>("u64", $4));
        inst->addOperand(std::make_unique<Register>("u64", $6));
        inst->addOperand(std::make_unique<Register>("u64", $8));
        $$ = inst;
    }
    | FMA DOT MODIFIER DOT DTYPE_F32 REGISTER COMMA REGISTER COMMA REGISTER COMMA REGISTER {
        auto inst = new Instruction("fma");
        inst->addModifier($3);
        inst->addModifier($5);
        inst->addOperand(std::make_unique<Register>("f32", $6));
        inst->addOperand(std::make_unique<Register>("f32", $8));
        inst->addOperand(std::make_unique<Register>("f32", $10));
        inst->addOperand(std::make_unique<Register>("f32", $12));
        $$ = inst;
    }
    ;

cvta_instruction:
    CVTA DOT MODIFIER DOT GLOBAL DOT DTYPE_64 REGISTER COMMA REGISTER {
        auto inst = new Instruction("cvta");
        inst->addModifier($3);
        inst->addModifier("global");
        inst->addModifier($7);
        inst->addOperand(std::make_unique<Register>("u64", $8));
        inst->addOperand(std::make_unique<Register>("u64", $10));
        $$ = inst;
    }
    ;

setp_instruction:
    SETP DOT MODIFIER DOT DTYPE_32 REGISTER COMMA REGISTER COMMA REGISTER {
        auto inst = new Instruction("setp");
        inst->addModifier($3);
        inst->addModifier($5);
        inst->addOperand(std::make_unique<Register>("pred", $6));
        inst->addOperand(std::make_unique<Register>("u32", $8));
        inst->addOperand(std::make_unique<Register>("u32", $10));
        $$ = inst;
    }
    ;

operand:
    REGISTER { $$ = new Register("", $1); }
    | INTEGER_LITERAL { $$ = new IntLiteral($1); }
    | FLOAT_LITERAL { $$ = new FloatLiteral($1); }
    | BUILTIN_VAR { $$ = new BuiltinVar($1); }
    | LBRACKET IDENTIFIER RBRACKET { $$ = new Parameter($2); }
    | LBRACKET REGISTER RBRACKET { $$ = new MemoryAccess(std::make_unique<Register>("", $2)); }
    ;

%%

void yyerror(const char* s) {
    fprintf(stderr, "Parse error at line %d: %s", yylineno, s);
}
