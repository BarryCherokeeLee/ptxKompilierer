#include "codegen.h"
#include <iostream>
#include <cassert>
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/Host.h"
#include "llvm/IR/Constant.h"

PTXCodeGenerator::PTXCodeGenerator() : builder(context) {
    std::cout << "Initializing PTXCodeGenerator..." << std::endl;
    module = std::make_unique<llvm::Module>("PTXModule", context);
    if (!module) {
        std::cerr << "Error: Failed to create LLVM module" << std::endl;
    } else {
        std::cout << "LLVM module created successfully" << std::endl;
    }
}

void PTXCodeGenerator::generateCode(Module* ast) {
    std::cout << "Starting generateCode..." << std::endl;
    
    if (!ast) {
        std::cerr << "Error: AST is null" << std::endl;
        return;
    }
    
    if (!module) {
        std::cerr << "Error: Module is null" << std::endl;
        return;
    }
    
    try {
        std::cout << "Calling ast->accept(this)..." << std::endl;
        ast->accept(this);
        std::cout << "ast->accept(this) completed successfully" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception in generateCode: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception in generateCode" << std::endl;
    }
}

llvm::Type* PTXCodeGenerator::getPTXType(const std::string& ptxType) {
    std::cout << "Getting PTX type for: " << ptxType << std::endl;
    
    if (ptxType == "s8" || ptxType == "u8") {
        return llvm::Type::getInt8Ty(context);
    } else if (ptxType == "s16" || ptxType == "u16") {
        return llvm::Type::getInt16Ty(context);
    } else if (ptxType == "s32" || ptxType == "u32" || ptxType == "b32") {
        return llvm::Type::getInt32Ty(context);
    } else if (ptxType == "s64" || ptxType == "u64" || ptxType == "b64") {
        return llvm::Type::getInt64Ty(context);
    } else if (ptxType == "f32") {
        return llvm::Type::getFloatTy(context);
    } else if (ptxType == "f64") {
        return llvm::Type::getDoubleTy(context);
    } else if (ptxType == "pred") {
        return llvm::Type::getInt1Ty(context);
    }
    std::cout << "Using default type (i32) for: " << ptxType << std::endl;
    return llvm::Type::getInt32Ty(context); // default
}

llvm::Value* PTXCodeGenerator::getBuiltinVar(const std::string& name) {
    std::cout << "Getting builtin variable: " << name << std::endl;
    
    if (name == "%tid.x" || name == "tid.x") {
        llvm::GlobalVariable* tidx = module->getGlobalVariable("tid.x");
        if (!tidx) {
            tidx = new llvm::GlobalVariable(*module, llvm::Type::getInt32Ty(context),
                false, llvm::GlobalValue::ExternalLinkage,
                llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0), "tid.x");
        }
        return builder.CreateLoad(llvm::Type::getInt32Ty(context), tidx, "tid.x");
    } else if (name == "%tid.y" || name == "tid.y") {
        llvm::GlobalVariable* tidy = module->getGlobalVariable("tid.y");
        if (!tidy) {
            tidy = new llvm::GlobalVariable(*module, llvm::Type::getInt32Ty(context),
                false, llvm::GlobalValue::ExternalLinkage,
                llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0), "tid.y");
        }
        return builder.CreateLoad(llvm::Type::getInt32Ty(context), tidy, "tid.y");
    } else if (name == "%tid.z" || name == "tid.z") {
        llvm::GlobalVariable* tidz = module->getGlobalVariable("tid.z");
        if (!tidz) {
            tidz = new llvm::GlobalVariable(*module, llvm::Type::getInt32Ty(context),
                false, llvm::GlobalValue::ExternalLinkage,
                llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0), "tid.z");
        }
        return builder.CreateLoad(llvm::Type::getInt32Ty(context), tidz, "tid.z");
    } else if (name == "%ntid.x" || name == "ntid.x") {
        return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 1);
    } else if (name == "%ntid.y" || name == "ntid.y") {
        return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 1);
    } else if (name == "%ntid.z" || name == "ntid.z") {
        return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 1);
    } else if (name == "%ctaid.x" || name == "ctaid.x") {
        return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0);
    } else if (name == "%ctaid.y" || name == "ctaid.y") {
        return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0);
    } else if (name == "%ctaid.z" || name == "ctaid.z") {
        return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0);
    }
    
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0);
}

void PTXCodeGenerator::visit(IntLiteral* node) {
    std::cout << "Visiting IntLiteral: " << node->value << std::endl;
    if (!node) {
        std::cerr << "Error: IntLiteral node is null" << std::endl;
        lastValue = nullptr;
        return;
    }
    lastValue = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), node->value);
}

void PTXCodeGenerator::visit(FloatLiteral* node) {
    std::cout << "Visiting FloatLiteral: " << node->value << std::endl;
    if (!node) {
        std::cerr << "Error: FloatLiteral node is null" << std::endl;
        lastValue = nullptr;
        return;
    }
    lastValue = llvm::ConstantFP::get(llvm::Type::getFloatTy(context), node->value);
}

void PTXCodeGenerator::visit(StringLiteral* node) {
    std::cout << "Visiting StringLiteral: " << node->value << std::endl;
    if (!node) {
        std::cerr << "Error: StringLiteral node is null" << std::endl;
        lastValue = nullptr;
        return;
    }
    lastValue = builder.CreateGlobalStringPtr(node->value, "str");
}

void PTXCodeGenerator::visit(Identifier* node) {
    std::cout << "Visiting Identifier: " << node->name << std::endl;
    if (!node) {
        std::cerr << "Error: Identifier node is null" << std::endl;
        lastValue = nullptr;
        return;
    }
    
    auto it = namedValues.find(node->name);
    if (it != namedValues.end()) {
        lastValue = it->second;
    } else {
        std::cerr << "Unknown identifier: " << node->name << std::endl;
        lastValue = nullptr;
    }
}

void PTXCodeGenerator::visit(Register* node) {
    std::cout << "Visiting Register: " << node->name << " (type: " << node->type << ")" << std::endl;
    if (!node) {
        std::cerr << "Error: Register node is null" << std::endl;
        lastValue = nullptr;
        return;
    }
    
    auto it = registers.find(node->name);
    if (it != registers.end()) {
        lastValue = it->second;
        std::cout<<"Found existing register:"<<node->name<<std::endl;
    } else {
        // Create a new register if it doesn't exist
        llvm::Type* regType = getPTXType(node->type);
        llvm::Value* reg = builder.CreateAlloca(regType, nullptr, node->name);
        registers[node->name] = reg;
        lastValue = reg;
        std::cout<<"Created new register: "<<node->name<<std::endl;
    }
}

void PTXCodeGenerator::visit(MemoryAccess* node) {
    std::cout << "Visiting MemoryAccess" << std::endl;
    if (!node || !node->address) {
        std::cerr << "Error: MemoryAccess node or address is null" << std::endl;
        lastValue = nullptr;
        return;
    }
    
    node->address->accept(this);
    if (lastValue) {
        llvm::Type* elementType = llvm::Type::getFloatTy(context);
        if (lastValue->getType()->isPointerTy()) {
            lastValue = builder.CreateLoad(elementType, lastValue, "memload");
        }
    }
}

void PTXCodeGenerator::visit(Parameter* node) {
    std::cout << "Visiting Parameter: " << node->name << std::endl;
    if (!node) {
        std::cerr << "Error: Parameter node is null" << std::endl;
        lastValue = nullptr;
        return;
    }
    
    auto it = parameters.find(node->name);
    if (it != parameters.end()) {
        llvm::Type* ptrType = it->second->getType();
        llvm::Type* elementType = nullptr;
        
        if (ptrType->isPointerTy()) {
            if (auto allocaInst = llvm::dyn_cast<llvm::AllocaInst>(it->second)) {
                elementType = allocaInst->getAllocatedType();
            } else {
                elementType = llvm::Type::getInt64Ty(context);
            }
        } else {
            elementType = ptrType;
        }
        
        lastValue = builder.CreateLoad(elementType, it->second, "paramload");
    } else {
        std::cerr << "Unknown parameter: " << node->name << std::endl;
        lastValue = nullptr;
    }
}

void PTXCodeGenerator::visit(BuiltinVar* node) {
    std::cout << "Visiting BuiltinVar: " << node->name << std::endl;
    if (!node) {
        std::cerr << "Error: BuiltinVar node is null" << std::endl;
        lastValue = nullptr;
        return;
    }
    lastValue = getBuiltinVar(node->name);
}

void PTXCodeGenerator::visit(Instruction* node) {
    std::cout << "Visiting Instruction: " << node->opcode << std::endl;
    if (!node) {
        std::cerr << "Error: Instruction node is null" << std::endl;
        return;
    }
    
    std::vector<llvm::Value*> operandValues;
    
    // Process operands
    std::cout << "Processing " << node->operands.size() << " operands" << std::endl;
    for (const auto& operand : node->operands) {
        if (!operand) {
            std::cerr << "Warning: Found null operand" << std::endl;
            continue;
        }
        
        operand->accept(this);
        if (lastValue) {
            operandValues.push_back(lastValue);
        }
    }
    
    std::cout << "Generated " << operandValues.size() << " operand values" << std::endl;
    
    // Generate instruction based on opcode
    try {
        if (node->opcode == "ld") {
            std::cout << "Processing ld instruction" << std::endl;
            if (operandValues.size() >= 2) {
                llvm::Value* src = operandValues[1];
                llvm::Value* dst = operandValues[0];

                if (src->getType()->isPointerTy()) {
                    llvm::Type* elementType = llvm::Type::getInt64Ty(context);
                    if (auto allocaInst = llvm::dyn_cast<llvm::AllocaInst>(src)) {
                        elementType = allocaInst->getAllocatedType();
                    }
                    src = builder.CreateLoad(elementType, src, "param_load");
                }
                
                if (dst->getType()->isPointerTy()) {
                    builder.CreateStore(src, dst);
                }
                lastValue = src;

            }
        } else if (node->opcode == "st") {
            std::cout << "Processing st instruction" << std::endl;
            if (operandValues.size() >= 2) {
                llvm::Value* dst = operandValues[0];
                llvm::Value* src = operandValues[1];
                if (src->getType()->isPointerTy()) {
                    llvm::Type* elementType = llvm::Type::getFloatTy(context);
                    if (auto allocaInst = llvm::dyn_cast<llvm::AllocaInst>(src)) {
                        elementType = allocaInst->getAllocatedType();
                    }
                    src = builder.CreateLoad(elementType, src, "reg_load");
                }
                
                if (dst->getType()->isPointerTy()) {
                    builder.CreateStore(src, dst);
                }
                lastValue = src;
            }
        } else if (node->opcode == "mov") {
            std::cout << "Processing mov instruction" << std::endl;
            if (operandValues.size() >= 2) {
                llvm::Value* dst = operandValues[0];
                llvm::Value* src = operandValues[1];
                
                if (src->getType()->isPointerTy() && !llvm::isa<llvm::GlobalVariable>(src)) {
                }
                
                if (dst->getType()->isPointerTy()) {
                    builder.CreateStore(src, dst);
                }
                lastValue = src;
            }
        } else if (node->opcode == "add") {
            std::cout << "Processing add instruction" << std::endl;
            if (operandValues.size() >= 3) {
                llvm::Value* dst = operandValues[0];
                llvm::Value* op1 = operandValues[1];
                llvm::Value* op2 = operandValues[2];
                
                if (op1->getType()->isPointerTy()) {
                    llvm::Type* elementType = llvm::Type::getInt64Ty(context);
                    if (auto allocaInst = llvm::dyn_cast<llvm::AllocaInst>(op1)) {
                        elementType = allocaInst->getAllocatedType();
                    }
                    op1 = builder.CreateLoad(elementType, op1, "load_op1");
                }
                
                if (op2->getType()->isPointerTy()) {
                    llvm::Type* elementType = llvm::Type::getInt64Ty(context);
                    if (auto allocaInst = llvm::dyn_cast<llvm::AllocaInst>(op2)) {
                        elementType = allocaInst->getAllocatedType();
                    }
                    op2 = builder.CreateLoad(elementType, op2, "load_op2");
                }
                
                llvm::Value* result;
                if(op1->getType()->isFloatingPointTy()||op2->getType()->isFloatingPointTy()){
                    result = builder.CreateFAdd(op1,op2,"fadd");
                }else{
                    result = builder.CreateAdd(op1,op2,"add");
                }

                if(dst->getType()->isPointerTy()){
                    builder.CreateStore(result,dst);
                }
                lastValue = result;
            }
        } else if (node->opcode == "mul") {
            std::cout << "Processing mul instruction" << std::endl;
            if (operandValues.size() >= 3) {
                llvm::Value* dst = operandValues[0];
                llvm::Value* op1 = operandValues[1];
                llvm::Value* op2 = operandValues[2];
                
                if (op1->getType()->isPointerTy()) {
                    llvm::Type* elementType = llvm::Type::getInt32Ty(context);
                    if (auto allocaInst = llvm::dyn_cast<llvm::AllocaInst>(op1)) {
                        elementType = allocaInst->getAllocatedType();
                    }
                    op1 = builder.CreateLoad(elementType, op1, "load_op1");
                }
                
                if (op2->getType()->isPointerTy()) {
                    llvm::Type* elementType = llvm::Type::getInt32Ty(context);
                    if (auto allocaInst = llvm::dyn_cast<llvm::AllocaInst>(op2)) {
                        elementType = allocaInst->getAllocatedType();
                    }
                    op2 = builder.CreateLoad(elementType, op2, "load_op2");
                }

                llvm::Value* result;
                if(std::find(node->modifiers.begin(),node->modifiers.end(),"wide")!=node->modifiers.end()){
                    if(op1->getType()->isIntegerTy(32)){
                        op1 = builder.CreateSExt(op1,llvm::Type::getInt64Ty(context),"sext_op1");
                    }
                    if(op2->getType()->isIntegerTy(32)){
                        op2=builder.CreateSExt(op2,llvm::Type::getInt64Ty(context),"sext_op2");
                    }
                }

                if(op1->getType()->isFloatingPointTy()||op2->getType()->isFloatingPointTy()){
                    result = builder.CreateFMul(op1,op2,"fmul");
                }else{
                    result = builder.CreateMul(op1,op2,"mul");
                }

                if(dst->getType()->isPointerTy()){
                    builder.CreateStore(result,dst);
                }
                lastValue=result;
            }
        } else if (node->opcode == "mad" || node->opcode == "fma") {
            std::cout << "Processing mad/fma instruction" << std::endl;
            if (operandValues.size() >= 4) {
                llvm::Value* dst = operandValues[0];
                llvm::Value* op1 = operandValues[1];
                llvm::Value* op2 = operandValues[2];
                llvm::Value* op3 = operandValues[3];
                
                if (op1->getType()->isPointerTy()) {
                    llvm::Type* elementType = llvm::Type::getInt32Ty(context);
                    if (auto allocaInst = llvm::dyn_cast<llvm::AllocaInst>(op1)) {
                        elementType = allocaInst->getAllocatedType();
                    }
                    op1 = builder.CreateLoad(elementType, op1, "load_op1");
                }
                
                if (op2->getType()->isPointerTy()) {
                    llvm::Type* elementType = llvm::Type::getInt32Ty(context);
                    if (auto allocaInst = llvm::dyn_cast<llvm::AllocaInst>(op2)) {
                        elementType = allocaInst->getAllocatedType();
                    }
                    op2 = builder.CreateLoad(elementType, op2, "load_op2");
                }
                
                if (op3->getType()->isPointerTy()) {
                    llvm::Type* elementType = llvm::Type::getInt32Ty(context);
                    if (auto allocaInst = llvm::dyn_cast<llvm::AllocaInst>(op3)) {
                        elementType = allocaInst->getAllocatedType();
                    }
                    op3 = builder.CreateLoad(elementType, op3, "load_op3");
                }
                
                llvm::Value* result;
                if(op1->getType()->isFloatingPointTy()){
                    llvm::Value* mulResult = builder.CreateFMul(op1,op2,"fmul");
                    result = builder.CreateFAdd(mulResult,op3,"fma");
                }else{
                    llvm::Value* mulResult = builder.CreateMul(op1,op2,"mul");
                    result = builder.CreateAdd(mulResult,op3,"mad");
                }

                if(dst->getType()->isPointerTy()){
                    builder.CreateStore(result,dst);
                }

                lastValue=result;
            }
        } else if (node->opcode == "setp") {
            std::cout << "Processing setp instruction" << std::endl;
            if (operandValues.size() >= 3) {
                llvm::Value* dst = operandValues[0];
                llvm::Value* op1 = operandValues[1];
                llvm::Value* op2 = operandValues[2];
                
                if (op1->getType()->isPointerTy()) {
                    llvm::Type* elementType = llvm::Type::getInt32Ty(context);
                    if (auto allocaInst = llvm::dyn_cast<llvm::AllocaInst>(op1)) {
                        elementType = allocaInst->getAllocatedType();
                    }
                    op1 = builder.CreateLoad(elementType, op1, "load_op1");
                }
                
                if (op2->getType()->isPointerTy()) {
                    llvm::Type* elementType = llvm::Type::getInt32Ty(context);
                    if (auto allocaInst = llvm::dyn_cast<llvm::AllocaInst>(op2)) {
                        elementType = allocaInst->getAllocatedType();
                    }
                    op2 = builder.CreateLoad(elementType, op2, "load_op2");
                }
                
                std::cout << "Operand 1 type: " << (op1->getType()->isIntegerTy() ? "integer" : "other") << std::endl;
                std::cout << "Operand 2 type: " << (op2->getType()->isIntegerTy() ? "integer" : "other") << std::endl;
                
                llvm::Value* cmp = nullptr;
                bool found_cmp = false;
                bool isFloatComparison = op1->getType()->isFloatingPointTy()||op2->getType()->isFloatingPointTy();
                for (const auto& mod : node->modifiers) {
                    std::cout << "Processing modifier: " << mod << std::endl;
                    if (mod == "eq") {
                        cmp = isFloatComparison ? 
                            builder.CreateFCmpOEQ(op1, op2, "fcmpeq") :
                            builder.CreateICmpEQ(op1, op2, "cmpeq");
                        found_cmp = true;
                        break;
                    } else if (mod == "ne") {
                        cmp = isFloatComparison ?
                            builder.CreateFCmpONE(op1, op2, "fcmpne") :
                            builder.CreateICmpNE(op1, op2, "cmpne");
                        found_cmp = true;
                        break;
                    } else if (mod == "lt") {
                        cmp = isFloatComparison ?
                            builder.CreateFCmpOLT(op1, op2, "fcmplt") :
                            builder.CreateICmpSLT(op1, op2, "cmplt");
                        found_cmp = true;
                        break;
                    } else if (mod == "le") {
                        cmp = isFloatComparison ?
                            builder.CreateFCmpOLE(op1, op2, "fcmple") :
                            builder.CreateICmpSLE(op1, op2, "cmple");
                        found_cmp = true;
                        break;
                    } else if (mod == "gt") {
                        cmp = isFloatComparison ?
                            builder.CreateFCmpOGT(op1, op2, "fcmpgt") :
                            builder.CreateICmpSGT(op1, op2, "cmpgt");
                        found_cmp = true;
                        break;
                    } else if (mod == "ge") {
                        cmp = isFloatComparison ?
                            builder.CreateFCmpOGE(op1, op2, "fcmpge") :
                            builder.CreateICmpSGE(op1, op2, "cmpge");
                        found_cmp = true;
                        break;
                    }
                }
                
                if (!found_cmp) {
                    std::cout << "No comparison modifier found, using default eq" << std::endl;
                    cmp = isFloatComparison ?
                        builder.CreateFCmpOEQ(op1, op2, "fcmpeq") :
                        builder.CreateICmpEQ(op1, op2, "cmpeq");
                }
                
                if (dst->getType()->isPointerTy()) {
                    builder.CreateStore(cmp, dst);
                    std::cout << "Stored comparison result to register" << std::endl;
                }
                lastValue = cmp;
            }
        } else if (node->opcode == "cvta") {
            std::cout << "Processing cvta instruction" << std::endl;
            if (operandValues.size() >= 2) {
                llvm::Value* dst = operandValues[0];
                llvm::Value* src = operandValues[1];
                
                if (src->getType()->isPointerTy()) {
                    llvm::Type* elementType = llvm::Type::getInt64Ty(context);
                    if (auto allocaInst = llvm::dyn_cast<llvm::AllocaInst>(src)) {
                        elementType = allocaInst->getAllocatedType();
                    }
                    src = builder.CreateLoad(elementType, src, "load_src");
                }
                
                if (dst->getType()->isPointerTy()) {
                    builder.CreateStore(src, dst);
                }
                lastValue = src;
            }
        } else {
            std::cout << "Unknown instruction opcode: " << node->opcode << std::endl;
            if(!operandValues.empty()){
                lastValue = operandValues[0];
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception in visit(Instruction): " << e.what() << std::endl;
        lastValue =nullptr;
    } catch (...) {
        std::cerr << "Unknown exception in visit(Instruction)" << std::endl;
        lastValue = nullptr;
    }
}

void PTXCodeGenerator::visit(BranchInst* node) {
    std::cout << "Visiting BranchInst to: " << node->target << std::endl;
    if (!node) {
        std::cerr << "Error: BranchInst node is null" << std::endl;
        return;
    }
    
    if (builder.GetInsertBlock()->getTerminator()) {
        std::cout << "Warning: Current block already has terminator" << std::endl;
        return;
    }
    
    auto labelIt = labels.find(node->target);
    if (labelIt == labels.end()) {
        // Create the target block if it doesn't exist
        llvm::BasicBlock* targetBB = llvm::BasicBlock::Create(context, node->target, currentFunction);
        labels[node->target] = targetBB;
        labelIt = labels.find(node->target);
    }
    
    if (node->condition) {
        std::cout << "Processing conditional branch" << std::endl;
        node->condition->accept(this);
        if (lastValue) {
            llvm::BasicBlock* trueBB = labelIt->second;
            llvm::BasicBlock* continueBB = llvm::BasicBlock::Create(context, "continue", currentFunction);
            
            llvm::Value* condValue = lastValue;
            
            if (condValue->getType()->isPointerTy()) {
                llvm::Type* elementType = llvm::Type::getInt1Ty(context);
                if (auto allocaInst = llvm::dyn_cast<llvm::AllocaInst>(condValue)) {
                    elementType = allocaInst->getAllocatedType();
                }
                condValue = builder.CreateLoad(elementType, condValue, "cond_load");
            }
            
            if (!condValue->getType()->isIntegerTy(1)) {
                llvm::Constant* zeroConst = llvm::ConstantInt::get(condValue->getType(), 0);
                condValue = builder.CreateICmpNE(condValue, zeroConst, "tobool");
            }
            
            if (node->negated) {
                builder.CreateCondBr(condValue, continueBB, trueBB);
            } else {
                builder.CreateCondBr(condValue, trueBB, continueBB);
            }
            
            builder.SetInsertPoint(continueBB);
        }
    } else {
        std::cout << "Creating unconditional branch" << std::endl;
        builder.CreateBr(labelIt->second);

        llvm::BasicBlock* unreachableBB = llvm::BasicBlock::Create(context, "unreachable", currentFunction);
        builder.SetInsertPoint(unreachableBB);
    }
}

void PTXCodeGenerator::visit(Label* node) {
    std::cout << "Visiting Label: " << node->name << std::endl;
    if (!node) {
        std::cerr << "Error: Label node is null" << std::endl;
        return;
    }
    
    auto labelIt = labels.find(node->name);
    llvm::BasicBlock* labelBB;
    
    if (labelIt != labels.end()) {
        labelBB = labelIt->second;
    } else {
        labelBB = llvm::BasicBlock::Create(context, node->name, currentFunction);
        labels[node->name] = labelBB;
    }
    
    if (!builder.GetInsertBlock()->getTerminator()) {
        builder.CreateBr(labelBB);
    }
    
    builder.SetInsertPoint(labelBB);
}

void PTXCodeGenerator::visit(ReturnInst* node) {
    std::cout << "Visiting ReturnInst" << std::endl;
    if (!node) {
        std::cerr << "Error: ReturnInst node is null" << std::endl;
        return;
    }
    if (!builder.GetInsertBlock()->getTerminator()) {
        builder.CreateRetVoid();
    }
}

void PTXCodeGenerator::visit(RegDecl* node) {
    std::cout << "Visiting RegDecl: " << node->name << " (type: " << node->type << ", count: " << node->count << ")" << std::endl;
    if (!node || !currentFunction) {
        std::cerr << "Error: RegDecl node is null or no current function" << std::endl;
        return;
    }
    
    llvm::Type* regType = getPTXType(node->type);
    
    if (node->count > 1) {
        // Array of registers
        for (int i = 0; i < node->count; i++) {
            std::string regName = node->name + std::to_string(i);
            llvm::Value* reg = builder.CreateAlloca(regType, nullptr, regName);
            registers[regName] = reg;
            std::cout << "Created register: " << regName << std::endl;
        }
    } else {
        // Single register
        llvm::Value* reg = builder.CreateAlloca(regType, nullptr, node->name);
        registers[node->name] = reg;
    }
}

void PTXCodeGenerator::visit(ParamDecl* node) {
    std::cout << "Visiting ParamDecl: " << node->name << " (type: " << node->type << ")" << std::endl;
    if (!node) {
        std::cerr << "Error: ParamDecl node is null" << std::endl;
        return;
    }
    // Parameters are handled in Function visit
}

void PTXCodeGenerator::visit(Function* node) {
    std::cout << "Visiting Function: " << node->name << std::endl;
    if (!node) {
        std::cerr << "Error: Function node is null" << std::endl;
        return;
    }
    
    try {
        // Create function type
        std::vector<llvm::Type*> paramTypes;
        std::cout << "Processing " << node->parameters.size() << " parameters" << std::endl;
        for (const auto& param : node->parameters) {
            if (!param) {
                std::cerr << "Warning: Found null parameter" << std::endl;
                continue;
            }
            llvm::Type* paramType = getPTXType(param->type);
            if (param->type.find("64") != std::string::npos) {
                paramType = paramType->getPointerTo(); // Parameters are often pointers
            }
            paramTypes.push_back(paramType);
        }
        
        llvm::FunctionType* funcType = llvm::FunctionType::get(
            llvm::Type::getVoidTy(context), paramTypes, false);
        
        currentFunction = llvm::Function::Create(funcType, 
            llvm::Function::ExternalLinkage, node->name, module.get());
        
        if (!currentFunction) {
            std::cerr << "Error: Failed to create LLVM function" << std::endl;
            return;
        }
        
        // Create entry block first
        llvm::BasicBlock* entryBB = llvm::BasicBlock::Create(context, "entry", currentFunction);
        builder.SetInsertPoint(entryBB);
        
        // Set parameter names and create allocas
        auto argIt = currentFunction->arg_begin();
        for (const auto& param : node->parameters) {
            if (!param) continue;
            
            argIt->setName(param->name);
            
            // Create alloca for parameter
            llvm::AllocaInst* alloca = builder.CreateAlloca(argIt->getType(), nullptr, param->name);
            parameters[param->name] = alloca;
            
            // Store argument value
            builder.CreateStore(&*argIt, alloca);
            ++argIt;
        }
        
        // Process register declarations
        std::cout << "Processing " << node->registers.size() << " register declarations" << std::endl;
        for (const auto& reg : node->registers) {
            if (reg) {
                reg->accept(this);
            }
        }
        
        // Process statements
        std::cout << "Processing " << node->body.size() << " statements in body" << std::endl;
        for (const auto& stmt : node->body) {
            if (stmt) {
                stmt->accept(this);
            }
        }
        
        // Add return if function doesn't end with one
        if (!builder.GetInsertBlock()->getTerminator()) {
            builder.CreateRetVoid();
        }
        
        currentFunction = nullptr;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception in visit(Function): " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception in visit(Function)" << std::endl;
    }
}

void PTXCodeGenerator::visit(Module* node) {
    std::cout << "Visiting Module with " << node->functions.size() << " functions" << std::endl;
    if (!node) {
        std::cerr << "Error: Module node is null" << std::endl;
        return;
    }
    
    try {
        for (const auto& func : node->functions) {
            if (func) {
                func->accept(this);
            } else {
                std::cerr << "Warning: Found null function in module" << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception in visit(Module): " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception in visit(Module)" << std::endl;
    }
}

void PTXCodeGenerator::dumpModule() {
    std::cout << "=== Generated LLVM IR ===" << std::endl;
    if (module) {
        module->print(llvm::outs(), nullptr);
    } else {
        std::cerr << "Error: Module is null" << std::endl;
    }
}

bool PTXCodeGenerator::verifyModule() {
    std::cout << "Verifying module..." << std::endl;
    if (!module) {
        std::cerr << "Error: Module is null, cannot verify" << std::endl;
        return false;
    }
    
    bool result = !llvm::verifyModule(*module, &llvm::errs());
    if (result) {
        std::cout << "Module verification passed" << std::endl;
    } else {
        std::cout << "Module verification failed" << std::endl;
    }
    return result;
}

void PTXCodeGenerator::optimizeModule() {
    std::cout << "Optimizing module..." << std::endl;
    if (!module) {
        std::cerr << "Error: Module is null, cannot optimize" << std::endl;
        return;
    }
    
    try {
        llvm::LoopAnalysisManager LAM;
        llvm::FunctionAnalysisManager FAM;
        llvm::CGSCCAnalysisManager CGAM;
        llvm::ModuleAnalysisManager MAM;
        
        llvm::PassBuilder PB;
        
        PB.registerModuleAnalyses(MAM);
        PB.registerCGSCCAnalyses(CGAM);
        PB.registerFunctionAnalyses(FAM);
        PB.registerLoopAnalyses(LAM);
        PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
        
        llvm::ModulePassManager MPM = PB.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O2);
        
        MPM.run(*module, MAM);
        std::cout << "Module optimization completed" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception in optimizeModule: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception in optimizeModule" << std::endl;
    }
}

void PTXCodeGenerator::generateRISCVAssembly(const std::string& filename) {
    std::cout << "Generating RISC-V assembly to: " << filename << std::endl;
    if (!module) {
        std::cerr << "Error: Module is null, cannot generate assembly" << std::endl;
        return;
    }
    
    try {
        std::error_code EC;
        llvm::raw_fd_ostream dest(filename, EC, llvm::sys::fs::OF_None);
        
        if (EC) {
            std::cerr << "Could not open file: " << EC.message() << std::endl;
            return;
        }
        
        dest << "; Generated LLVM IR from PTX\n";
        module->print(dest, nullptr);
        dest.flush();
        
        std::cout << "LLVM IR written to: " << filename << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception in generateRISCVAssembly: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception in generateRISCVAssembly" << std::endl;
    }
}

bool PTXCodeGenerator::isFloatingPointOperation(const std::string& opcode, const std::vector<llvm::Value*>& operands) {
    if (opcode.find("f") == 0) {
        return true;
    }

    for (const auto& operand : operands) {
        if (operand && operand->getType()->isFloatingPointTy()) {
            return true;
        }
    }
    
    return false;
}
