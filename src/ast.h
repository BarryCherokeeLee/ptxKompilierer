#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include <memory>
#include <iostream>

// Forward declarations
class ASTNode;
class Expression;
class Statement;
class Instruction;

using ASTNodePtr = std::unique_ptr<ASTNode>;
using ExprPtr = std::unique_ptr<Expression>;
using StmtPtr = std::unique_ptr<Statement>;
using InstPtr = std::unique_ptr<Instruction>;

// Base AST Node
class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual void print(int indent = 0) const = 0;
    virtual void accept(class ASTVisitor* visitor) = 0;
};

// Expression base class
class Expression : public ASTNode {
public:
    virtual ~Expression() = default;
};

// Statement base class
class Statement : public ASTNode {
public:
    virtual ~Statement() = default;
};

// Literal expressions
class IntLiteral : public Expression {
public:
    int value;
    IntLiteral(int val) : value(val) {}
    void print(int indent = 0) const override;
    void accept(class ASTVisitor* visitor) override;
};

class FloatLiteral : public Expression {
public:
    float value;
    FloatLiteral(float val) : value(val) {}
    void print(int indent = 0) const override;
    void accept(class ASTVisitor* visitor) override;
};

class StringLiteral : public Expression {
public:
    std::string value;
    StringLiteral(const std::string& val) : value(val) {}
    void print(int indent = 0) const override;
    void accept(class ASTVisitor* visitor) override;
};

// Identifier
class Identifier : public Expression {
public:
    std::string name;
    Identifier(const std::string& n) : name(n) {}
    void print(int indent = 0) const override;
    void accept(class ASTVisitor* visitor) override;
};

// Register reference
class Register : public Expression {
public:
    std::string type;  // "pred", "f32", "b32", "b64"
    std::string name;
    int index;
    
    Register(const std::string& t, const std::string& n, int idx = -1) 
        : type(t), name(n), index(idx) {}
    void print(int indent = 0) const override;
    void accept(class ASTVisitor* visitor) override;
};

// Memory access
class MemoryAccess : public Expression {
public:
    ExprPtr address;
    MemoryAccess(ExprPtr addr) : address(std::move(addr)) {}
    void print(int indent = 0) const override;
    void accept(class ASTVisitor* visitor) override;
};

// Parameter reference
class Parameter : public Expression {
public:
    std::string name;
    Parameter(const std::string& n) : name(n) {}
    void print(int indent = 0) const override;
    void accept(class ASTVisitor* visitor) override;
};

// Built-in variables (%tid.x, %ntid.x, %ctaid.x)
class BuiltinVar : public Expression {
public:
    std::string name;  // "tid.x", "ntid.x", "ctaid.x"
    BuiltinVar(const std::string& n) : name(n) {}
    void print(int indent = 0) const override;
    void accept(class ASTVisitor* visitor) override;
};

// Instructions
class Instruction : public Statement {
public:
    std::string opcode;
    std::vector<std::string> modifiers;
    std::vector<ExprPtr> operands;
    
    Instruction(const std::string& op) : opcode(op) {}
    void addModifier(const std::string& mod) { modifiers.push_back(mod); }
    void addOperand(ExprPtr operand) { operands.push_back(std::move(operand)); }
    void print(int indent = 0) const override;
    void accept(class ASTVisitor* visitor) override;
};

// Label
class Label : public Statement {
public:
    std::string name;
    Label(const std::string& n) : name(n) {}
    void print(int indent = 0) const override;
    void accept(class ASTVisitor* visitor) override;
};

// Branch instruction with predicate
class BranchInst : public Statement {
public:
    std::string target;
    std::unique_ptr<Expression> condition;
    bool negated;
    
    BranchInst(const std::string& tgt,  std::unique_ptr<Expression> cond = nullptr, bool neg = false) 
        : target(tgt), condition(std::move(cond)), negated(neg) {}
    void print(int indent = 0) const override;
    void accept(class ASTVisitor* visitor) override;
};

// Return instruction
class ReturnInst : public Statement {
public:
    std::unique_ptr<Expression> value;
    
    ReturnInst(std::unique_ptr<Expression> val = nullptr) : value(std::move(val)) {}
    void print(int indent = 0) const override;
    void accept(class ASTVisitor* visitor) override;
};

// Register declaration
class RegDecl : public Statement {
public:
    std::string type;
    std::string name;
    int count;
    
    RegDecl(const std::string& t, const std::string& n, int c) 
        : type(t), name(n), count(c) {}
    void print(int indent = 0) const override;
    void accept(class ASTVisitor* visitor) override;
};

// Parameter declaration
class ParamDecl : public Statement {
public:
    std::string type;
    std::string name;
    
    ParamDecl(const std::string& t, const std::string& n) : type(t), name(n) {}
    void print(int indent = 0) const override;
    void accept(class ASTVisitor* visitor) override;
};

// Function declaration
class Function : public ASTNode {
public:
    std::string visibility;  // "visible"
    std::string type;        // "entry"
    std::string name;
    std::vector<std::unique_ptr<ParamDecl>> parameters;
    std::vector<std::unique_ptr<RegDecl>> registers;
    std::vector<std::unique_ptr<Statement>> statements; 
    std::vector<StmtPtr> body;
    
    Function(const std::string& vis, const std::string& t, const std::string& n) 
        : visibility(vis), type(t), name(n) {}
    void addParameter(std::unique_ptr<ParamDecl> param) { 
        parameters.push_back(std::move(param)); 
    }
    void addRegister(std::unique_ptr<RegDecl> reg) { 
        registers.push_back(std::move(reg)); 
    }
    void addStatement(StmtPtr stmt) { body.push_back(std::move(stmt)); }
    void print(int indent = 0) const override;
    void accept(class ASTVisitor* visitor) override;
};

// Module (top-level)
class Module : public ASTNode {
public:
    std::string version;
    std::string target;
    int address_size;
    std::vector<std::unique_ptr<Function>> functions;
    
    void setVersion(const std::string& v) { version = v; }
    void setTarget(const std::string& t) { target = t; }
    void setAddressSize(int size) { address_size = size; }
    void addFunction(std::unique_ptr<Function> func) { 
        functions.push_back(std::move(func)); 
    }
    void print(int indent = 0) const override;
    void accept(class ASTVisitor* visitor) override;
};

// Visitor pattern for AST traversal
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;
    virtual void visit(IntLiteral* node) = 0;
    virtual void visit(FloatLiteral* node) = 0;
    virtual void visit(StringLiteral* node) = 0;
    virtual void visit(Identifier* node) = 0;
    virtual void visit(Register* node) = 0;
    virtual void visit(MemoryAccess* node) = 0;
    virtual void visit(Parameter* node) = 0;
    virtual void visit(BuiltinVar* node) = 0;
    virtual void visit(Instruction* node) = 0;
    virtual void visit(Label* node) = 0;
    virtual void visit(BranchInst* node) = 0;
    virtual void visit(ReturnInst* node) = 0;
    virtual void visit(RegDecl* node) = 0;
    virtual void visit(ParamDecl* node) = 0;
    virtual void visit(Function* node) = 0;
    virtual void visit(Module* node) = 0;
};

#endif // AST_H
