#include <iostream>

// ANTLR4 stuff

#include "antlr4-runtime.h"
#include "BreadLexer.h"
#include "BreadParser.h"
#include "BreadVisitor.h"
    
// LLVM stuff

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include "llvm/IR/IRBuilder.h"



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

    auto context = std::make_unique<LLVMContext>();
    llvm::IRBuilder<> builder(*context);

    auto i = std::any_cast<int>(visitor.visitProg(tree));
    std::cout << "Calculated value " << i << "\n";

    return 0;
}
