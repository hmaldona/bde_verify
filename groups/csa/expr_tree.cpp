#include "expr_tree.h"
//#include "breg_matchers.h"

#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Lex/Lexer.h>
#include <clang/AST/Stmt.h>
#include <clang/Sema/Sema.h>
#include <clang/Frontend/CompilerInstance.h>
#include <csabase_ppobserver.h>

#include <csabase_analyser.h>
#include <csabase_debug.h>
#include <csabase_diagnostic_builder.h>
#include <csabase_registercheck.h>
#include <csabase_report.h>
#include <csabase_util.h>
#include <csabase_visitor.h>


using namespace csabase;
using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;


TreeManager::TreeManager() :
    analyser(nullptr),
    context(nullptr),
    options(),
    toolMessages(nullptr),
    manager(nullptr)
{
}


TreeManager::TreeManager( Analyser * analyser, 
                          ASTContext * context, 
                          SourceManager const * manager, 
                          BregOptions options, 
                          std::vector<std::string> * toolMessages ) :
    analyser(analyser),
    context(context),
    options(options),
    toolMessages(toolMessages),
    manager(manager)
{
}


void TreeManager::setCurrentExpr(Expr const * clangNode)
{
    std::unique_ptr<ExprTree> node( new ExprTree(nullptr, nullptr, false, clangNode, this ) );
    this->currentExpr = std::move(node);
}

std::unique_ptr<ExprTree> TreeManager::getCurrentExpr()
{
    return std::move(this->currentExpr);
}

bool TreeManager::currentExprTreeRemovable() 
{
    return this->currentExpr->removable;
}


void TreeManager::pruneCurrentExprTree()
{
    this->currentExpr->pruneTree();
}

std::string TreeManager::rebuildCurrentExprTree()
{
    this->currentExpr->rebuildCondition();
    return this->currentExpr->exprString;
}


ExprTree::ExprTree() :
    manager(nullptr),
    lhs(nullptr),
    rhs(nullptr),
    clangNode(nullptr),
    exprString(),
    op(),
    removable(false),
    evaluated(false),
    sideEffects(false),
    binary(false)
{
}


ExprTree::ExprTree(std::unique_ptr<ExprTree> lhs, 
                   std::unique_ptr<ExprTree> rhs, 
                   bool removable, 
                   Expr const * clangNode, 
                   TreeManager * manager) :
    manager(manager),
    lhs(std::move(lhs)),
    rhs(std::move(rhs)),
    clangNode(clangNode),
    exprString(),
    op(),
    removable(removable),
    evaluated(false),
    sideEffects(false),
    binary(false)
{

    this->setExprString();
    
}



void ExprTree::pruneTree()
{
    Expr const * expr = this->clangNode->IgnoreParens()->IgnoreCasts();

    bool expressionValue = true;
    
    bool evaluated = expr->EvaluateAsBooleanCondition( expressionValue, *(this->manager->context) );

    BinaryOperator const * binExpr = llvm::dyn_cast<BinaryOperator>( expr ); 
    UnaryOperator const * unExpr = llvm::dyn_cast<UnaryOperator>( expr );
    CallExpr const * call = llvm::dyn_cast<CallExpr>( expr );
    CXXBoolLiteralExpr const * boolLiteral = llvm::dyn_cast<CXXBoolLiteralExpr>( expr );
       

    if ( evaluated ) {
        this->evaluated = true;

        bool hasPossibleSideEffects = this->clangNode->HasSideEffects( *this->manager->context );
        std::cout << this->exprString << std::endl;
        if ( hasPossibleSideEffects == false && this->sideEffects == false ) {

            this->removable = true;
            return;
        }
        else {
            this->sideEffects = true;
        }
    }

    if ( binExpr != nullptr ) {
       
        Expr const * lhs = binExpr->getLHS();
        Expr const * rhs = binExpr->getRHS();
        
        std::unique_ptr<ExprTree> nodeLHS( new ExprTree(nullptr, nullptr, false, lhs, this->manager) );
        std::unique_ptr<ExprTree> nodeRHS( new ExprTree(nullptr, nullptr, false, rhs, this->manager) );

        nodeLHS->pruneTree();
        nodeRHS->pruneTree();

        this->lhs = std::move(nodeLHS);
        this->rhs = std::move(nodeRHS);
        this->binary = true;
        this->op = binExpr->getOpcodeStr().str();
        
        this->sideEffects = this->lhs->sideEffects || this->rhs->sideEffects;
        this->removable = !this->sideEffects && this->evaluated;
           
        std::cout << this->exprString << std::endl;
        std::cout << this->removable << std::endl;

        // short circuit evalutation check
        if ( this->removable == false ) {
            this->removable = this->evaluated && ( this->lhs->evaluated && !this->lhs->sideEffects );
        }
    }
    else if ( unExpr != nullptr ) {
        Expr const * sub = unExpr->getSubExpr();

        std::unique_ptr<ExprTree> nodeSub( new ExprTree(nullptr, nullptr, false, sub, this->manager) );

        nodeSub->pruneTree();
        
        this->lhs = std::move(nodeSub);
        this->binary = false;
        this->op = UnaryOperator::getOpcodeStr(unExpr->getOpcode()).str();

        this->sideEffects = this->lhs->sideEffects;
        this->removable = !this->sideEffects && this->evaluated;
    }
    else if ( call != nullptr ) {
        FunctionDecl const * f = llvm::dyn_cast<FunctionDecl>( call->getCalleeDecl() );

        if ( f != nullptr ) {
            std::string message;

            if ( this->ignoreSideEffects == false ) {  
                std::pair<bool, std::string> sideEffects = sideEffectAnalysis(f);


                this->sideEffects = sideEffects.first;
                message = call->getLocStart().printToString( *this->manager->manager ) + ": ";

                if ( this->sideEffects == true ) {
                    message += "may not be able to remove call to: '" + 
                               call->getDirectCallee()->getNameAsString() 
                               + "'" + ", " + 
                               sideEffects.second + "\n";
                }
                else {
                    message += "safe to remove call to '" + call->getDirectCallee()->getNameAsString() + "'" + "\n";
                } 
            }
            else {
                message += "safe to remove call to '" + call->getDirectCallee()->getNameAsString() + "'" + "\n";
                this->sideEffects = false;
            }

            if ( this->manager->options.functionRemovalWarnings == false ) {
                this->manager->toolMessages->push_back(message);
            }
        }
    }
    else if ( boolLiteral != nullptr ) {
        this->sideEffects = false;
    }

}


// rebuilds the condition from an expression tree

void ExprTree::rebuildCondition() 
{
    if ( this->binary ) {
        //only one branch will ever be removed 
        if ( !this->lhs->removable ) {
            this->lhs->rebuildCondition();
        }
        else if ( this->op == "&&" || this->op == "||" ) {
            this->replaceWithBranch( this->rhs );
        }

        if ( !this->rhs->removable ) {
            this->rhs->rebuildCondition();
        }
        else if ( this->op == "&&" || this->op == "||" )  {
            this->replaceWithBranch( this->lhs );
        }
        
        if ( !this->lhs->removable && !this->rhs->removable ) {
            std::string oldBranch = this->lhs->getExprString();
            int index = this->exprString.find(oldBranch);
            this->exprString.replace(index, oldBranch.length(), this->lhs->exprString);

            oldBranch = this->rhs->getExprString();
            index = this->exprString.rfind(oldBranch);
            this->exprString.replace(index, oldBranch.length(), this->rhs->exprString);
        }
    }
    else if ( this->lhs != nullptr ) {
        if ( !this->lhs->removable ) {
            this->lhs->rebuildCondition();
        }
    }
}


void ExprTree::replaceWithBranch( std::unique_ptr<ExprTree>& branch ) 
{  
    std::string message = this->clangNode->getLocStart().printToString(*this->manager->manager) + ": "
                          "replacing condition: " + "'" + 
                          this->exprString + "'" + "\n\n"
                          " with: " + "'" + branch->exprString +
                          "'" + "\n\n";

    this->manager->toolMessages->push_back( message );

    this->exprString = branch->exprString;

}

std::string ExprTree::getExprString() 
{
    SourceLocation end = this->manager->manager->getFileLoc(Lexer::getLocForEndOfToken( this->clangNode->getLocEnd(), 
                                                                                        0, 
                                                                                        *this->manager->manager, 
                                                                                        this->manager->context->getLangOpts()));
    SourceRange range = SourceRange( this->clangNode->getLocStart(), end );
    return this->manager->analyser->get_source(range, true).str();
}

void ExprTree::setExprString() 
{
    this->exprString = this->getExprString();
}

std::pair<bool, std::string> ExprTree::sideEffectAnalysis(FunctionDecl const * f)
{
    
    if ( f->hasBody() ) {
        CompoundStmt const * body = llvm::dyn_cast<CompoundStmt>( f->getBody() );
       
        if ( body->size() > 15 ) {
            return std::pair<bool, std::string>( true, "function is over 15 lines long" );
        } 

        for (auto iter = f->param_begin(); iter != f->param_end(); iter++) {
            ParmVarDecl const * varDecl = *iter;

            std::pair<Decl const *, bool> structure = Matchers::getNodeInExpr<Decl>( this->manager->context, varDecl, Matchers::ptrFieldDeclMatcher );
            std::pair<Decl const *, bool> ptrRef = Matchers::getNodeInExpr<Decl>( this->manager->context, varDecl, valueDecl( hasType( isNonConstPtr())) );

            if ( structure.second == true ) {
                 return std::pair<bool, std::string>( true, "passing non-const pointer or reference in class or struct" );
            }
            else if ( ptrRef.second == true ) {
                return std::pair<bool, std::string>( true, "passing non-const pointer or reference" );
            }
        }

   
        for (auto iter = body->body_begin(); iter != body->body_end(); iter++) {
            Stmt const * s = *iter;

            std::pair<Expr const *, bool> inc = Matchers::getNodeInExpr<Expr>( this->manager->context, s, Matchers::incOrDecMatcher ); 
            std::pair<Expr const *, bool> varDecl = Matchers::getNodeInExpr<Expr>( this->manager->context, s, Matchers::varDeclMatcher ); 
            std::pair<Expr const *, bool> assign = Matchers::getNodeInExpr<Expr>( this->manager->context, s, Matchers::assignMatcher ); 
            std::pair<Expr const *, bool> callExpr = Matchers::getNodeInExpr<Expr>( this->manager->context, s, Matchers::callMatcher ); 

            if ( inc.second != false ) {
                if ( inc.first != nullptr ) {
                    return std::pair<bool, std::string>( true, "incrementing or decrementing instance variable" );
                }
                else {
                    return std::pair<bool, std::string>( true, "incrementing or decrementing non-local variable" );
                }
            }
            else if ( varDecl.second != false ) {
                return std::pair<bool, std::string>( true, "declaring non-const pointer or reference to non-local variable" );
            }
            else if ( assign.second != false ) {
                if ( assign.first != nullptr ) {
                    return std::pair<bool, std::string>( true, "assigning to instance variable" );
                }
                else {
                    return std::pair<bool, std::string>( true, "assigning to non-local variable" );
                }
            }
            else if ( callExpr.second != false ) {
                if ( callExpr.first != nullptr ) {
                    CallExpr const * c = llvm::dyn_cast<CallExpr>( callExpr.first );
                    FunctionDecl const * func = llvm::dyn_cast<FunctionDecl>( c->getCalleeDecl() );
                    if ( func != nullptr ) {
                        return sideEffectAnalysis( func );
                    }
                }
            }
        }
    }

    return std::pair<bool, std::string>(false, "");
}


