#ifndef EXPR_TREE
#define EXPR_TREE

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

#include <iostream>

#include "breg_options.h"
#include "breg_matchers.h"

using namespace csabase;
using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;

class ExprTree;

class TreeManager {
    friend class ExprTree;
private:
    Analyser * analyser;
    ASTContext * context;
    
    BregOptions options;
    std::vector<std::string> * toolMessages;
    std::unique_ptr<ExprTree> currentExpr;

public:
    SourceManager const * manager; 
    TreeManager();
    TreeManager( Analyser * analyser, 
                 ASTContext * context, 
                 SourceManager const * manager, 
                 BregOptions options, 
                 std::vector<std::string> * toolMessages );

    void setCurrentExpr(Expr const * clangNode);
    std::unique_ptr<ExprTree> getCurrentExpr();

    bool currentExprTreeRemovable();
    void pruneCurrentExprTree();
    std::string rebuildCurrentExprTree();
    
};



//represents a condition in an if stmt
class ExprTree {
    friend class TreeManager;
private:
    TreeManager * manager;
       
    void rebuildCondition();
    void pruneTree();
    void setExprString(); 
    void replaceWithBranch( std::unique_ptr<ExprTree>& branch ); 

    std::string getExprString(); 

    std::pair<bool, std::string> sideEffectAnalysis(FunctionDecl const * f);

    ExprTree(std::unique_ptr<ExprTree> lhs, 
             std::unique_ptr<ExprTree> rhs, 
             bool removable, 
             Expr const * clangNode, 
             TreeManager * manager);
public:

    std::unique_ptr<ExprTree> lhs;
    std::unique_ptr<ExprTree> rhs;
    Expr const * clangNode;
    std::string exprString;


    std::string op;

    bool removable;
    bool evaluated;
    bool sideEffects;
    bool binary;
    bool ignoreSideEffects;


    ExprTree();
     
    
};


#endif
