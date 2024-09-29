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

std::map<std::string, llvm::Function*> functable;

class IRVisitor : public BreadVisitor
{
public:
    std::any visitProg(BreadParser::ProgContext *context) override
    {
        for (const auto& it: context->func())
        {
            it->accept(this);
        }

        return 0;
    }

    std::any visitDiv(BreadParser::DivContext *context) override
    {
        return builder->CreateSDiv(std::any_cast<llvm::Value*>(context->left->accept(this)), std::any_cast<llvm::Value*>(context->right->accept(this)));
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

    // Function call support

    std::any visitFunc(BreadParser::FuncContext *context)
    {
        context->prot()->accept(this);

        for (const auto& it: context->expr())
        {
            it->accept(this);
        }

        return 0;
    }

    std::any visitProt(BreadParser::ProtContext *context)
    {
        auto argl = std::any_cast<std::pair<std::vector<llvm::Type*>, std::vector<std::string>>>(context->argl()->accept(this));
        auto fname = context->name->getText();


        auto i32 = builder->getInt32Ty();
        auto prototype = llvm::FunctionType::get(i32, argl.first, false);
        llvm::Function *fn = llvm::Function::Create(prototype, llvm::Function::ExternalLinkage, fname, mod.get());
        llvm::BasicBlock *body = llvm::BasicBlock::Create(*llvm_context, "body", fn);
        builder->SetInsertPoint(body);

        functable[fname] = fn;

        return 0;
    }

    std::any visitArgl(BreadParser::ArglContext *context)
    {
        std::pair<std::vector<llvm::Type*>, std::vector<std::string>> ret;

        for (const auto& it: context->SYMNAME())
        {
            ret.first.push_back(builder->getInt32Ty());
            ret.second.push_back(it->getSymbol()->getText());
        }

        return ret;
    }

    std::any visitCall(BreadParser::CallContext *context)
    {
        auto func = functable[context->name->getText()];

        std::vector<llvm::Value*> args;

        std::cout << "Call to " << context->name->getText() << " has " << context->expr().size() << " args\n";

        for (const auto& it: context->expr())
        {
            args.push_back(std::any_cast<llvm::Value*>(it->accept(this)));
        }

        return reinterpret_cast<llvm::Value*>(builder->CreateCall(func, args));
    }

    std::any visitRet(BreadParser::RetContext *context)
    {
        builder->CreateRet(std::any_cast<llvm::Value*>(context->expr()->accept(this)));

        return 0;
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

    IRVisitor irv;

    llvm_context = std::make_unique<llvm::LLVMContext>();
    builder = std::make_unique<llvm::IRBuilder<>>(*llvm_context);
    mod = std::make_unique<llvm::Module>("hello", *llvm_context);
 
    irv.visitProg(tree);

    std::cout << "Running built LLVM IR...\n";

    llvm::ExecutionEngine *executionEngine = llvm::EngineBuilder(std::move(mod)).setEngineKind(llvm::EngineKind::Interpreter).create();
    llvm::Function *main = executionEngine->FindFunctionNamed(llvm::StringRef("main"));
    auto result = executionEngine->runFunction(main, {});
    int pr = result.IntVal.getLimitedValue();

    std::cout << pr << "\n";

    return 0;
}
