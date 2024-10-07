#include <iostream>
#include <memory>
#include <vector>
#include <map>

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
std::stack<std::map<std::string, llvm::Value*>> local_vars;

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
        local_vars.clear();

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

        // Set up arguments as local variables

        auto i = 0;
        for (const auto& it: argl.second)
        {
            local_vars.top()[it] = reinterpret_cast<llvm::Value*>(fn->getArg(i));
            i++;
        }

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

    std::any visitAss(BreadParser::AssContext *context)
    {
        if (local_vars.top().find(context->name->getText()) == local_vars.top().end())
        {
            std::cout << "Assigning to undeclared variable " << context->name->getText() << " is not allowed.\n";
            exit(1);
        }

        local_vars.top()[context->name->getText()] = std::any_cast<llvm::Value*>(context->expr()->accept(this));

        return local_vars.top()[context->name->getText()];
    }

    std::any visitVar(BreadParser::VarContext *context)
    {
        if (local_vars.top().find(context->name->getText()) == local_vars.top().end())
        {
            std::cout << "Accessing to undeclared variable " << context->name->getText() << " is not allowed.\n";
            exit(1);
        }

        return local_vars.top()[context->name->getText()];
    }

    std::any visitDecl(BreadParser::DeclContext *context)
    {
        // We assign the element in the map to null to indicate that the variable has been declared.
        // The assignment expression checks that the variable has been declared by performing a find()
        // over the local variable map.
        local_vars.top()[context->name->getText()] = nullptr;

        return 0;
    }

    llvm::BasicBlock *parseBodyStatements(std::vector<StmtContext*> stmt)
    {
        llvm::Function *fn = builder->GetInsertBlock()->getParent();
        llvm::BasicBlock *inner_blk = llvm::BasicBlock::Create(*llvm_context, fn);
        auto outer_blk = builder->GetInsertBlock();
        builder->SetInsertPoint(inner_blk);

        for (const auto& it: stmt)
        {
            it->accept(this);
        }

        // In LLVM we must terminate each block with a control-flow statement.
        builder->CreateBr(outer_blk); // TODO: This is incorrect, we are jumping back to the start of the outer block!
        builder->SetInsertPoint(outer_blk);

        return inner_blk;
    }

    std::any visitBody(BreadParser::BodyContext *context)
    {
        // Copy pre-existing local vars
        local_vars.push(local_vars.top());

        parseBodyStatements(context->stmt())

        // Pop our locals!
        auto body_locals = local_vars.top();
        local_vars.pop();

        // Merge back modified locals
        for (const auto& it: local_vars.top())
        {
            if (it.second != body_locals[it.first])
                local_vars[it.first] = body_locals[it.first];
        }

        return 0;
    }

    std::any visitCbod(BreadParser::CbodContext *context)
    {
        // Create merge block to contain PHI's and the insert point to be the merge block. 
        // The current implementation of parseBodyStatements will create an uncoditional 
        // branch to the merge block when it is done processing the body.
        auto cond = builder->GetInsertBlock();
        llvm::Function *fn = cond->getParent();
        llvm::BasicBlock *merge = llvm::BasicBlock::Create(*llvm_context, fn);
        builder->SetInsertPoint(merge);

        // Copy pre-existing local vars
        local_vars.push(local_vars.top());

        auto then = parseBodyStatements(context->stmt())

        // Pop our locals!
        auto body_locals = local_vars.top();
        local_vars.pop();

        // Instead of simply merging back any modified locals, we must create PHI nodes
        // for each variable modified within the block.
        for (const auto& it: local_vars.top())
        {
            if (it.second != body_locals[it.first])
            {
                auto phi = builder->CreatePHI(builder->getInt32Ty(), 2); // Two reserved value, one for the original and one for the modified value.
                phi->addIncoming(then, body_locals[it.first]);
                phi->addIncoming(cond, it.second);
                local_vars.top()[it.first] = reinterpret_cast<Value*>(phi);
            }
        }

        // Begin a new block to jump to from merge.
        cont = bui

        return std::pair<>{then, merge};
    }

    std::any visitIf(BreadParser::IfContext *context)
    {
        auto condition = std::any_cast<llvm::Value*>(context->expr()->accept(this));
        auto [then, merge] = std::any_cast<std::pair<llvm::BasicBlock*, llvm::BasicBlock*>>(context->body()->accept(this));
        builder->CreateCondBr(condition, then, merge);

        return 0;
    }

    std::any visitExp(BreadParser::ExpContext *context)
    {
        return context->expr()->accept(this);
    }

    std::any visitLt(BreadParser::LtContext *context)
    {
        auto lhs = std::any_cast<llvm::Value*>(context->left->accept(this));
        auto rhs = std::any_cast<llvm::Value*>(context->right->accept(this));

        return builder->CreateICmpSLT(lhs, rhs);
    }

    std::any visitEq(BreadParser::EqContext *context)
    {
        auto lhs = std::any_cast<llvm::Value*>(context->left->accept(this));
        auto rhs = std::any_cast<llvm::Value*>(context->right->accept(this));

        return builder->CreateICmpEQ(lhs, rhs);

    }

    std::any visitGt(BreadParser::GtContext *context)
    {
        auto lhs = std::any_cast<llvm::Value*>(context->left->accept(this));
        auto rhs = std::any_cast<llvm::Value*>(context->right->accept(this));

        return builder->CreateICmpSGT(lhs, rhs);
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
