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

#include <set>
#include <iostream>
#include <fstream>

#include "breg_replacer.h"
#include "expr_tree.h"
#include "breg_options.h"


#define log(s) (std::cout << s << std::endl )
#define maybe(check) if (check == nullptr) {return;} 

using namespace csabase;
using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;

static std::string const check_name("breg-remover");

namespace {

    
struct data {
    TreeManager treeManager;

};


struct report : Report<data>
{
    //INHERIT_REPORT_CTOR(report, Report, data);
   
    report(Analyser& analyser, PPObserver::CallbackType type = PPObserver::e_None);

    Stmt const * node;

    BregOptions options;

    //used to store else stmts removed removed
    std::vector<Stmt const*> elsesRemoved;

    //stores location of breg if stmts
    std::vector<unsigned> bregLocations;
    std::vector<std::string> bregNames;

    std::vector<std::string> toolMessages;

    std::string getStmtBodyReplacement(CompoundStmt const * stmt, unsigned column);

    bool hasVarDecls(CompoundStmt const * stmt);

    void addChainedElses(Stmt const * stmt);

    std::pair<std::string, SourceRange> rebuildIfStmt(bool expressionValue, 
                                                      IfStmt const * ifStmt, 
                                                      FunctionDecl const * func); 

    void matchIf(BoundNodes const & nodes);
    void operator()();
    void operator()(SourceLocation where, bool, std::string const& name); 
    
};



//matches string initialization as a variable or in an initialization list
static const internal::DynTypedMatcher dyn_matchIf()
{
    static const internal::DynTypedMatcher matcher = findAll( functionDecl( hasDescendant( stmt ( findAll (                                                                                                                    ifStmt( hasCondition( expr().bind("expr"))).bind("ifstmt")
                                                                   )))).bind("func"));
    return matcher; 
}

report::report(Analyser& analyser, PPObserver::CallbackType type)
    : Report<data>(analyser, type)
{
    std::string csvFile = a.config()->value("breg_file");
    
    this->options = BregOptions();
    this->options.ignoreSideEffects = a.config()->value("side_effects") == "true";

    auto csvData = readCSV(csvFile);

    this->bregNames.resize(csvData.size());

    for ( auto e : csvData ) {
        this->bregNames.push_back(e.first);
    }
}

// remove includes

void report::operator()(SourceLocation where, bool, std::string const& name)
{
    for (std::string bregName : bregNames) {

        if ( name.find( bregName ) != std::string::npos ) {

            // #include <> + name.length
            SourceRange range = SourceRange( where, 
                                             where.getLocWithOffset( 11 + name.length() ) ); 
            a.ReplaceText( a.get_full_range(range), "" );
        }

    }
}

// remove bregs 

void report::operator()()
{
    std::ifstream bregFile;
    bregFile.open(".bregLocs");

    this->d.treeManager = TreeManager( &a,
                                       a.context(), 
                                       &m, 
                                       this->options, 
                                       &this->toolMessages );


    // read breg locations 
    if ( bregFile.is_open() ) {
        std::string locStr;
        while (std::getline(bregFile, locStr)) {
            bregLocations.push_back( atoi(locStr.c_str()) );
        }
    }
    else {
        // nothing to be done, no bregs found
        return; 
    }
    bregFile.close();

    // match bregs 
    MatchFinder finder;
    csabase::OnMatch<report, &report::matchIf> m(this);
    finder.addDynamicMatcher(dyn_matchIf(), &m);
    finder.match(*a.context()->getTranslationUnitDecl(), *a.context());
    
    // output tool information
   
    if ( this->options.silent == false ) {
        for ( auto m : toolMessages) {
            std::cout << m; 
        }
    }
}

// recursively adds else stmts

void report::addChainedElses(Stmt const * stmt) 
{
    // stmt is null when there are no more else stmts
    if (stmt != nullptr ) { 
        IfStmt const * ifStmt = llvm::dyn_cast<IfStmt>(stmt);

        if (ifStmt != nullptr) {

            // collect elses that have been removed 
            elsesRemoved.push_back(ifStmt);
            addChainedElses(ifStmt->getElse());
        }
    }

}

// check if the body has variable declations 

bool report::hasVarDecls(CompoundStmt const * stmt)
{
    for (auto iter = stmt->body_begin(); iter < stmt->body_end(); iter++) {
        Stmt const * s = *iter;
        clang::DeclStmt const * varDeclStmt = llvm::dyn_cast<clang::DeclStmt>(s);

        // stmt is a variable declaration 
        if (varDeclStmt != nullptr) {
            return true;
        }
    }
    
    return false;
}

// get the stmts within the body with correct indentation

std::string report::getStmtBodyReplacement(CompoundStmt const * stmt, unsigned column)
{
    std::string replacement;

    if ( stmt->body_empty() == false ) {
        
        // first stmt has correct indentation already 
        Stmt const * firstStmt = *( stmt->body_begin() );
        SourceLocation stmtEnd = m.getFileLoc(Lexer::getLocForEndOfToken( firstStmt->getLocEnd(), 0, m, a.context()->getLangOpts()));
        SourceRange range = SourceRange( firstStmt->getLocStart(), stmtEnd );
        replacement = a.get_source(range, true).str() + ";\n";


        // if the stmt was just a semi colon 
        if ( replacement == ";;" ) {
            replacement = ";";
        }

        for (auto iter = stmt->body_begin()+1; iter < stmt->body_end(); iter++) {
            Stmt const * s = *iter;

            SourceLocation stmtEnd = m.getFileLoc(Lexer::getLocForEndOfToken( s->getLocEnd(), 0, m, a.context()->getLangOpts()));
            SourceRange r = SourceRange( s->getLocStart(), stmtEnd );

            // indent the correct amount before the stmt
            replacement += std::string(column-1, ' ') + a.get_source(r, true).str() + ";\n";
        }
    }

    return replacement;
}


std::pair<std::string, SourceRange> report::rebuildIfStmt(bool expressionValue, IfStmt const * ifStmt, FunctionDecl const * func ) 
{
    SourceRange rangeToBeReplaced;
    std::string replacement;

    Stmt const * elseStmt = ifStmt->getElse();

    //std::string replacement;   
    rangeToBeReplaced = SourceRange(ifStmt->getLocStart(), ifStmt->getLocEnd().getLocWithOffset(1));

    if (expressionValue == true) { //replace entire if stmt with body of if

        CompoundStmt const * body = static_cast<CompoundStmt const*>( ifStmt->getThen() );
        bool hasVars = hasVarDecls( body ); // does the if body has var decls 

        if (body->body_back() != nullptr) {
            ReturnStmt const * ret = llvm::dyn_cast<ReturnStmt>( body->body_back() );

            if ( ret != nullptr ) {
                CompoundStmt const * funcBody = llvm::dyn_cast<CompoundStmt>( func->getBody() );
                if ( funcBody != nullptr ) {
                    rangeToBeReplaced = SourceRange(ifStmt->getLocStart(), funcBody->getLocEnd());
                    unsigned columnNumber = a.manager().getPresumedColumnNumber(ifStmt->getLocStart());
                    replacement = getStmtBodyReplacement(body, columnNumber);
                }
            }
            else if ( hasVars ) { // braces must remain 
                SourceRange replacementRange = SourceRange(body->getLBracLoc(), body->getRBracLoc().getLocWithOffset(1));
                replacement += a.get_source(replacementRange, true).str();
            }
            else { // braces can be removed 
                unsigned columnNumber = a.manager().getPresumedColumnNumber(ifStmt->getLocStart());
                replacement += getStmtBodyReplacement(body, columnNumber);
            }
        }

        if ( elseStmt != nullptr ) { 
            addChainedElses( elseStmt ); //store else statements removed 
        }   
    }
    else { //removes the if stmt, leaves the else parts if there are any

        if ( elseStmt != nullptr ) {

            SourceRange elseRange = SourceRange(elseStmt->getLocStart().getLocWithOffset(0), ifStmt->getLocEnd().getLocWithOffset(1));
            replacement += a.get_source(elseRange, true).str();

            CompoundStmt const * elseBody = llvm::dyn_cast<CompoundStmt>( elseStmt );

            if (elseBody != nullptr) {
                bool hasVars = hasVarDecls( elseBody ); 

                if ( !hasVars ) {
                    unsigned columnNumber = a.manager().getPresumedColumnNumber(ifStmt->getLocStart());
                    replacement += getStmtBodyReplacement(elseBody, columnNumber);
                }
            }
        }
    }

    return std::pair<std::string, SourceRange>(replacement, rangeToBeReplaced);
}



void report::matchIf(BoundNodes const & nodes)
{
    FunctionDecl const * func = nodes.getNodeAs<FunctionDecl>("func");
    IfStmt const * ifStmt = nodes.getNodeAs<IfStmt>("ifstmt");

    auto bregLocFound = std::find(bregLocations.begin(), bregLocations.end(), ifStmt->getLocStart().getRawEncoding());
    auto elseFound = std::find( elsesRemoved.begin(), elsesRemoved.end(), ifStmt );
    
    if ( !( elseFound != elsesRemoved.end() || bregLocFound == bregLocations.end() ) ) {
        bool modifyIf = true;
        bool expressionValue = true;

        bool evaluated = ifStmt->getCond()->EvaluateAsBooleanCondition(expressionValue, *a.context());
        bool hasPossibleSideEffects = ifStmt->getCond()->HasSideEffects( *a.context() );

        std::string replacement;
        SourceRange rangeToBeReplaced;

        if ( !evaluated || hasPossibleSideEffects ) {
            this->d.treeManager.setCurrentExpr( ifStmt->getCond() );
            this->d.treeManager.pruneCurrentExprTree();
            if ( this->d.treeManager.currentExprTreeRemovable() == false ) {
                std::string newCondition = this->d.treeManager.rebuildCurrentExprTree();
                replacement = newCondition + ";\n";

                // changing condition only
                if ( !evaluated || this->options.conditionExtraction == false ) {
                    SourceLocation end = m.getFileLoc(Lexer::getLocForEndOfToken( ifStmt->getCond()->getLocEnd(), 0, m, a.context()->getLangOpts()));
                    rangeToBeReplaced = SourceRange( ifStmt->getCond()->getLocStart(), end );
                    replacement = newCondition;
                    modifyIf = false;
                }
            }
        }

        if ( modifyIf == true ) {
            std::pair<std::string, SourceRange> p = rebuildIfStmt(expressionValue, ifStmt, func);
            replacement += p.first;
            rangeToBeReplaced = p.second;
        }

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







