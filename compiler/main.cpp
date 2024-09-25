#include <iostream>
#include <memory>

// ANTLR4 stuff

#include "antlr4-runtime.h"
#include "BreadLexer.h"
#include "BreadParser.h"
#include "BreadVisitor.h"
    
// LLVM stuff

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/IR/IRBuilder.h>


std::unique_ptr<llvm::LLVMContext> llvm_context;
std::unique_ptr<llvm::IRBuilder<>> builder;
std::unique_ptr<llvm::Module> mod;

class TestVisitor : public BreadVisitor
{
public:
    std::any visitProg(BreadParser::ProgContext *context) override
    {
        std::cout << "Visiting program context.\n";

        return context->expr()->accept(this);
    }

    std::any visitDiv(BreadParser::DivContext *context) override
    {
        std::cout << "Visiting expression context.\n";

        return std::any_cast<int>(context->left->accept(this)) / std::any_cast<int>(context->right->accept(this));
    }

    std::any visitAdd(BreadParser::AddContext *context) override
    {
        return std::any_cast<int>(context->left->accept(this)) + std::any_cast<int>(context->right->accept(this));
    }

    std::any visitSub(BreadParser::SubContext *context) override
    {
        return std::any_cast<int>(context->left->accept(this)) - std::any_cast<int>(context->right->accept(this));
    }

    std::any visitBrack(BreadParser::BrackContext *context) override
    {
        return std::any_cast<int>(context->expr()->accept(this));
    }

    std::any visitMul(BreadParser::MulContext *context) override
    {
        return std::any_cast<int>(context->left->accept(this)) * std::any_cast<int>(context->right->accept(this));
    }

    std::any visitLit(BreadParser::LitContext *context) override
    {
        return std::stoi(context->INT()->getSymbol()->getText());
    }
};


class IRVisitor : public BreadVisitor
{
public:
    std::any visitProg(BreadParser::ProgContext *context) override
    {
        return context->expr()->accept(this);
    }

    std::any visitDiv(BreadParser::DivContext *context) override
    {
        return builder->CreateUDiv(std::any_cast<llvm::Value*>(context->left->accept(this)), std::any_cast<llvm::Value*>(context->right->accept(this)));
    }

    std::any visitAdd(BreadParser::AddContext *context) override
    {
        return builder->CreateAdd(std::any_cast<llvm::Value*>(context->left->accept(this)), std::any_cast<llvm::Value*>(context->right->accept(this)));
    }

    std::any visitSub(BreadParser::SubContext *context) override
    {
        return builder->CreateSub(std::any_cast<llvm::Value*>(context->left->accept(this)), std::any_cast<llvm::Value*>(context->right->accept(this)));
    }

    std::any visitBrack(BreadParser::BrackContext *context) override
    {
        return context->expr()->accept(this);
    }

    std::any visitMul(BreadParser::MulContext *context) override
    {
        return builder->CreateMul(std::any_cast<llvm::Value*>(context->left->accept(this)), std::any_cast<llvm::Value*>(context->right->accept(this)));
    }

    std::any visitLit(BreadParser::LitContext *context) override
    {
        int value = std::stoi(context->INT()->getSymbol()->getText());
        auto i32 = builder->getInt32Ty();
        return reinterpret_cast<llvm::Value*>(llvm::ConstantInt::get(i32, value));
    }
};

int main(int argc, const char* argv[]) 
{
    std::ifstream stream;
    stream.open("/home/mastamysta/prog/llvm_tst/compiler/input.bread");
    
    antlr4::ANTLRInputStream input(stream);
    BreadLexer lexer(&input);
    antlr4::CommonTokenStream tokens(&lexer);
    BreadParser parser(&tokens);    
    BreadParser::ProgContext* tree = parser.prog();
    TestVisitor visitor;
    
    std::cout << "Running immediate visitor.\n";

    auto i = std::any_cast<int>(visitor.visitProg(tree));
    std::cout << "Calculated value " << i << "\n";

    IRVisitor irv;

    llvm_context = std::make_unique<llvm::LLVMContext>();
    builder = std::make_unique<llvm::IRBuilder<>>(*llvm_context);
    mod = std::make_unique<llvm::Module>("hello", *llvm_context);

    // build a 'main' function
    auto i32 = builder->getInt32Ty();
    auto prototype = llvm::FunctionType::get(i32, false);
    llvm::Function *main_fn = llvm::Function::Create(prototype, llvm::Function::ExternalLinkage, "main", mod.get());
    llvm::BasicBlock *body = llvm::BasicBlock::Create(*llvm_context, "body", main_fn);
    builder->SetInsertPoint(body);
 
    irv.visitProg(tree);

    // return 0 from main
    auto ret = llvm::ConstantInt::get(i32, 0);
    builder->CreateRet(ret);

    std::cout << "Running built LLVM IR...\n";

    llvm::ExecutionEngine *executionEngine = llvm::EngineBuilder(std::move(mod)).setEngineKind(llvm::EngineKind::Interpreter).create();
    llvm::Function *main = executionEngine->FindFunctionNamed(llvm::StringRef("main"));
    auto result = executionEngine->runFunction(main, {});

    return 0;
}
