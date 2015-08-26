#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Lex/Lexer.h>
#include <clang/AST/Stmt.h>
#include <clang/Sema/Sema.h>


#include <csabase_analyser.h>
#include <csabase_debug.h>
#include <csabase_diagnostic_builder.h>
#include <csabase_registercheck.h>
#include <csabase_report.h>
#include <csabase_util.h>
#include <csabase_visitor.h>

#include <set>
#include <iostream>
#include <fstream>

#include "breg_replacer.h"

#define log(s) (std::cout << s << std::endl )

using namespace csabase;
using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;

static std::string const check_name("breg-remover");

namespace {

//represents a condition in an if stmt
struct ExprTree {
    std::unique_ptr<ExprTree> lhs;
    std::unique_ptr<ExprTree> rhs;

    Expr const * clangNode;
    std::string op;

    bool removable;
    bool evaluated;
    bool sideEffects;
    bool binary;

    ExprTree(std::unique_ptr<ExprTree> lhs, std::unique_ptr<ExprTree> rhs, bool removable, Expr const * clangNode) {
        this->lhs = std::move(lhs);
        this->rhs = std::move(rhs);
        this->clangNode = clangNode;

        this->removable = removable;
        this->evaluated = false;
        this->sideEffects = false;

        this->binary = false;
    }
};


    
struct data {
};


struct report : Report<data>
{
    //INHERIT_REPORT_CTOR(report, Report, data);
   
    report(Analyser& analyser, PPObserver::CallbackType type = PPObserver::e_None);

    //used to store else stmts removed removed
    std::vector<Stmt const*> elsesRemoved;

    //stores location of breg if stmts
    std::vector<unsigned> bregLocations;
    std::vector<std::string> bregNames;

    std::string getExprString(Expr const * expr);
    std::string getStmtBodyReplacement(CompoundStmt const * stmt, unsigned column);

    bool hasVarDecls(CompoundStmt const * stmt);

    bool sideEffectAnalysis(FunctionDecl const * f);

    void addChainedElses(Stmt const * stmt);
    void expressionPruning(std::unique_ptr<ExprTree>& node);
    void rebuildCondition(std::unique_ptr<ExprTree>& node);
    void removeBranch(std::unique_ptr<ExprTree>& node, Expr const * expr);
    
    void matchIf(BoundNodes const & nodes);
    void operator()();
    void operator()(SourceLocation where, bool, std::string const& name); 

};


//matches string initialization as a variable or in an initialization list
static const internal::DynTypedMatcher& dyn_matchIf()
{
    static const internal::DynTypedMatcher& matcher = findAll( functionDecl( hasDescendant( stmt ( findAll (                                                                                                                    ifStmt( hasCondition( expr().bind("expr"))).bind("ifstmt")
                                                                   )))).bind("func"));
    return matcher; 
}

report::report(Analyser& analyser, PPObserver::CallbackType type)
    : Report<data>(analyser, type)
{
    std::string csvFile = a.config()->value("breg_file"); 
        
    auto arr = readCSV(csvFile);

    bregNames.resize(arr.size());

    for ( auto e : arr ) {
        bregNames.push_back(e.first);
    }
}

//remove includes
void report::operator()(SourceLocation where, bool, std::string const& name)
{
    for (std::string bregName : bregNames) {
        if ( name.find( bregName ) != std::string::npos ) {
            SourceRange range = SourceRange(where, where.getLocWithOffset(11 + name.length()));  //#include <> + name.length
            a.ReplaceText(a.get_full_range(range), "");
        }
    }
}


void report::operator()()
{
    std::ifstream bregFile;
    bregFile.open(".bregLocs");
    if ( bregFile.is_open() ) {
        std::string locStr;
        while (std::getline(bregFile, locStr)) {
            bregLocations.push_back( atoi(locStr.c_str()) );
        }
    }
    else {
        return; //nothing to be done, no bregs found
    }
    bregFile.close();

    MatchFinder finder;
    csabase::OnMatch<report, &report::matchIf> m(this);
    finder.addDynamicMatcher(dyn_matchIf(), &m);
    finder.match(*d_analyser.context()->getTranslationUnitDecl(), *d_analyser.context());
}

void report::addChainedElses(Stmt const * stmt) {
    if (stmt == NULL) {
        return;
    }
    IfStmt const * ifStmt = llvm::dyn_cast<IfStmt>(stmt);

    if (ifStmt != NULL) {
        elsesRemoved.push_back(ifStmt);
        addChainedElses(ifStmt->getElse());
    }
}

bool report::hasVarDecls(CompoundStmt const * stmt)
{
    typedef clang::CompoundStmt::body_iterator bdy_iter;
   
    for (bdy_iter iter = (bdy_iter)stmt->body_begin(); iter != (bdy_iter)stmt->body_end(); iter++) {
        Stmt const * s = *iter;
        clang::DeclStmt const * varDeclStmt = llvm::dyn_cast<clang::DeclStmt>(s);
        if (varDeclStmt != NULL) {
            return true;
        }
    }
    
    return false;
}

std::string report::getStmtBodyReplacement(CompoundStmt const * stmt, unsigned column)
{
    if ( stmt->body_empty() ) {
        return "";
    }
    typedef clang::CompoundStmt::body_iterator bdy_iter;
    Stmt const * firstStmt = *( stmt->body_begin() );
    SourceLocation stmtEnd = m.getFileLoc(Lexer::getLocForEndOfToken( firstStmt->getLocEnd(), 0, m, a.context()->getLangOpts()));

    SourceRange range = SourceRange( firstStmt->getLocStart(), stmtEnd );
    
    std::string replacement = a.get_source(range, true).str() + ";\n";
    if ( replacement == ";;" ) {
        replacement = ";";
    }
    
    for (bdy_iter iter = (bdy_iter)stmt->body_begin()+1; iter != (bdy_iter)stmt->body_end(); iter++) {
        Stmt const * s = *iter;
                    
        SourceLocation stmtEnd = m.getFileLoc(Lexer::getLocForEndOfToken( s->getLocEnd(), 0, m, a.context()->getLangOpts()));

        SourceRange r = SourceRange( s->getLocStart(), stmtEnd );
        replacement += std::string(column-1, ' ') + a.get_source(r, true).str() + ";\n";
    }
    return replacement;
}

bool report::sideEffectAnalysis(FunctionDecl const * f)
{
    if ( f->hasBody() ) {
        CompoundStmt const * body = llvm::dyn_cast<CompoundStmt>( f->getBody() );
        if ( body->size() > 15 ) {
            return true;
        }   
    }
    return false;
}


//any sub expression that can be evaluated can be removed or replaced 
//the branches of the parent node will only ever reduce to the identity element for that logical operator

void report::expressionPruning(std::unique_ptr<ExprTree>& node)
{
    Expr const * expr = node->clangNode->IgnoreParens()->IgnoreCasts();
   
    bool expressionValue = true;
    bool evaluated = expr->EvaluateAsBooleanCondition(expressionValue, *d_analyser.context());

    BinaryOperator const * binExpr = llvm::dyn_cast<BinaryOperator>( expr ); 
    UnaryOperator const * unExpr = llvm::dyn_cast<UnaryOperator>( expr );
    CallExpr const * call = llvm::dyn_cast<CallExpr>( expr );
    CXXBoolLiteralExpr const * boolLiteral = llvm::dyn_cast<CXXBoolLiteralExpr>( expr );
        
    if ( evaluated ) {
        node->evaluated = true;

        bool hasPossibleSideEffects = node->clangNode->HasSideEffects( *a.context() );

        if ( hasPossibleSideEffects == false && node->sideEffects == false ) {
            node->removable = true;
            return;
        }
        else {
            node->sideEffects = true;
        }
    }

    if ( binExpr != nullptr ) {
       
        Expr const * lhs = binExpr->getLHS();
        Expr const * rhs = binExpr->getRHS();
        
        std::unique_ptr<ExprTree> nodeLHS( new ExprTree(nullptr, nullptr, false, lhs) );
        std::unique_ptr<ExprTree> nodeRHS( new ExprTree(nullptr, nullptr, false, rhs) );
        
        nodeLHS->sideEffects = node->sideEffects;
        nodeRHS->sideEffects = node->sideEffects;

        expressionPruning( nodeLHS );
        expressionPruning( nodeRHS );
 
        node->lhs = std::move(nodeLHS);
        node->rhs = std::move(nodeRHS);
        node->binary = true;
        node->op = binExpr->getOpcodeStr().str();
        
        node->sideEffects = node->lhs->sideEffects || node->rhs->sideEffects;
        node->removable = !node->sideEffects && node->evaluated;
    }
    else if ( unExpr != nullptr ) {
        Expr const * sub = unExpr->getSubExpr();

        std::unique_ptr<ExprTree> nodeSub( new ExprTree(nullptr, nullptr, false, sub) );

        nodeSub->sideEffects = node->sideEffects;

        expressionPruning( nodeSub );
        
        node->lhs = std::move(nodeSub);
        node->binary = false;
        node->op = UnaryOperator::getOpcodeStr(unExpr->getOpcode()).str();

        node->sideEffects = node->lhs->sideEffects;
        node->removable = !node->sideEffects && node->evaluated;
    }
    else if ( call != nullptr ) {
        FunctionDecl const * f = llvm::dyn_cast<FunctionDecl>( call->getCalleeDecl() );
        //call->dump();

        if ( f != nullptr ) {
            bool sideEffects = sideEffectAnalysis(f);
            if ( sideEffects == true ) {
                std::cout << "Unable to remove function: " << call->getDirectCallee()->getNameAsString() << std::endl;
            }
            node->sideEffects = sideEffects;
        }
    }
    else if ( boolLiteral != nullptr ) {
        node->sideEffects = false;
    }

}


void report::rebuildCondition(std::unique_ptr<ExprTree>& node) 
{
    if ( node->binary ) {
        //only one branch will ever be removed 
        if ( !node->lhs->removable ) {
            rebuildCondition( node->lhs );
        }
        else if ( node->op == "&&" || node->op == "||" ) {
            removeBranch(node->rhs, node->clangNode);
        }

        if ( !node->rhs->removable ) {
            rebuildCondition( node->rhs );
        }
        else if ( node->op == "&&" || node->op == "||" )  {
            removeBranch(node->lhs, node->clangNode);
        }
    }
    else if ( node->lhs != nullptr ) {
        if ( !node->lhs->removable ) {
            rebuildCondition( node->lhs );
        }
    }
}

void report::removeBranch(std::unique_ptr<ExprTree>& node, Expr const * expr)
{
    SourceLocation end = m.getFileLoc(Lexer::getLocForEndOfToken( expr->getLocEnd(), 0, m, a.context()->getLangOpts()));
    SourceRange rangeToBeReplaced = SourceRange( expr->getLocStart(), end );
     
    unsigned length = rangeToBeReplaced.getEnd().getRawEncoding() - rangeToBeReplaced.getBegin().getRawEncoding();
    Replacement replace = Replacement( a.manager(), rangeToBeReplaced.getBegin(), length, getExprString(node->clangNode) );
    replace.apply( a.rewriter() );

    rangeToBeReplaced.getBegin().dump(a.manager());
    std::cout << ": ";
    std::cout << "replacing: " << a.get_source(rangeToBeReplaced).str();
    std::cout << " with: " << getExprString(node->clangNode) << std::endl;
}

std::string report::getExprString(Expr const * expr) 
{
    SourceLocation end = m.getFileLoc(Lexer::getLocForEndOfToken( expr->getLocEnd(), 0, m, a.context()->getLangOpts()));
    SourceRange range = SourceRange( expr->getLocStart(), end );
    return a.get_source(range, true).str();
}

void report::matchIf(BoundNodes const & nodes)
{
    clang::IfStmt const * ifStmt = nodes.getNodeAs<clang::IfStmt>("ifstmt");

    auto i = std::find(bregLocations.begin(), bregLocations.end(), ifStmt->getLocStart().getRawEncoding());

    if ( i == bregLocations.end() ) {
        return;
    }
    else {
        bregLocations.erase(i); //don't check this one again 
    }
        
    //check if this if stmt has been removed already
    for ( auto e : elsesRemoved ) {
        if (e == ifStmt) {
            return;
        }
    }

    bool expressionValue = true;
    bool evaluated = ifStmt->getCond()->EvaluateAsBooleanCondition(expressionValue, *d_analyser.context());

    bool hasPossibleSideEffects = ifStmt->getCond()->HasSideEffects( *a.context() );

    bool modifyIf = false;

    if ( evaluated && !hasPossibleSideEffects ) {
        modifyIf = true;
    }
    else {
        std::unique_ptr<ExprTree> node( new ExprTree(nullptr, nullptr, false, ifStmt->getCond()) );

        expressionPruning( node );

        if ( node->removable == true ) {
            modifyIf = true;
        }
        else { // if stmt cannot be altered, branches of condition expression may be removed though 
            rebuildCondition( node );
        }
    }


    if ( modifyIf ) { // if stmt can change immediately, either the body can go or the condition can go

        Stmt const * elseStmt = ifStmt->getElse();

        std::string replacement;   
        SourceRange rangeToBeReplaced = SourceRange(ifStmt->getLocStart(), ifStmt->getLocEnd().getLocWithOffset(1));

        if (expressionValue == true) { //replace entire if stmt with body of if

            CompoundStmt const * body = static_cast<CompoundStmt const*>( ifStmt->getThen() );
            bool hasVars = hasVarDecls( body ); // does the if body has var decls 
            
            if ( hasVars ) { // braces must remain 
                SourceRange replacementRange = SourceRange(body->getLBracLoc(), body->getRBracLoc().getLocWithOffset(1));
                replacement = a.get_source(replacementRange, true).str();
            }
            else { // braces can be removed 
                unsigned columnNumber = a.manager().getPresumedColumnNumber(ifStmt->getLocStart());
                replacement = getStmtBodyReplacement(body, columnNumber);
            }

            if ( elseStmt != NULL ) { 
                addChainedElses( elseStmt ); //store else statements removed 
            }   
        }
        else { //removes the if stmt, leaves the else parts if there are any
            
            if ( elseStmt != NULL ) {
                
                SourceRange elseRange = SourceRange(elseStmt->getLocStart().getLocWithOffset(0), ifStmt->getLocEnd().getLocWithOffset(1));
                replacement = a.get_source(elseRange, true).str();

                CompoundStmt const * elseBody = llvm::dyn_cast<CompoundStmt>( elseStmt );

                if (elseBody != NULL) {
                    bool hasVars = hasVarDecls( elseBody ); 
                    
                    if ( !hasVars ) {
                        unsigned columnNumber = a.manager().getPresumedColumnNumber(ifStmt->getLocStart());
                        replacement = getStmtBodyReplacement(elseBody, columnNumber);
                    }
                }
            }
        }
        
        rangeToBeReplaced.getBegin().dump(a.manager());
        std::cout << ": ";
        std::cout << "replacing: " << a.get_source(rangeToBeReplaced).str();
        std::cout << " with: " << replacement << std::endl;


        unsigned length = rangeToBeReplaced.getEnd().getRawEncoding() - rangeToBeReplaced.getBegin().getRawEncoding();
        Replacement replace = Replacement( a.manager(), rangeToBeReplaced.getBegin(), length, replacement );
        replace.apply( a.rewriter() );
    }
}


void subscribe(Analyser& analyser, Visitor& visitor, PPObserver& observer) {
    observer.onInclude += report(analyser);
    analyser.onTranslationUnitDone += report(analyser); 
}

}  // close anonymous namespace

// -----------------------------------------------------------------------------

static RegisterCheck register_check(check_name, &subscribe);







