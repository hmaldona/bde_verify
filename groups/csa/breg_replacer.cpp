#include "breg_replacer.h"

#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Lex/Lexer.h>
#include <clang/AST/Stmt.h>

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
#include <sstream>
#include <map>


using namespace csabase;
using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;

static std::string const check_name("breg-replacer");

namespace {

    struct data {

    };

   
    struct report : Report<data>
    {
        std::vector<std::pair<std::string, boolValue>> bregs;
        std::vector<std::pair<unsigned, Stmt const *>> bregLocations;
        std::vector<VarDecl const *> varDeclsRemoved;
        std::map<VarDecl const *, Expr const *> varValue;

        int offset;

        report(Analyser& analyser, PPObserver::CallbackType type = PPObserver::e_None);

        void replaceExpr(Expr const * expr, SourceLocation exprStart, std::string replacement, IfStmt const * ifStmt);

        void matchBreg(BoundNodes const & nodes);

        void operator()();
        void operator()(SourceLocation where, bool, std::string const& name); 
    };


    report::report(Analyser& analyser, PPObserver::CallbackType type)
        : Report<data>(analyser, type)
    {
        offset = 0;
        std::string csvFile = a.config()->value("breg_file"); 
        
        bregs = readCSV(csvFile);
    }
    

    //matches string initialization as a variable or in an initialization list
    static const internal::DynTypedMatcher& dyn_matchBreg()
    {
        static const internal::DynTypedMatcher& matcher = findAll( functionDecl( hasDescendant( 
                                                             stmt( findAll( stmt(
                                                                 eachOf(                                                                                                                                                               ifStmt( hasCondition( expr( findAll( 
                                                                         callExpr().bind("call"))))).bind("ifstmt"),
                                                                     ifStmt( hasCondition( expr( findAll( 
                                                                         declRefExpr().bind("var"))))).bind("ifstmt"),
                                                                     binaryOperator( hasOperatorName("=")).bind("=")
                                                                         )))))).bind("func") );
                                                                         
        return matcher; 
    }

    void report::operator()()
    {
        MatchFinder finder;
        csabase::OnMatch<report, &report::matchBreg> m(this);
        finder.addDynamicMatcher(dyn_matchBreg(), &m);
        finder.match(*d_analyser.context()->getTranslationUnitDecl(), *d_analyser.context());

        //write locations of breg if stmts
        std::ofstream bregFile;
        bregFile.open(".bregLocs");

        for (auto i : bregLocations) {
            bregFile << i.first << "\n";
        }

        bregFile.close();
    }

    //replace with actual breg
    static bool getBregValue(std::pair<std::string, boolValue> pair) 
    {
        if (pair.second == defaultValue) {
            //hook database code in here
            return false;
        }
        
        return pair.second;
    } 

    
    void report::replaceExpr(Expr const * expr, SourceLocation exprStart, std::string replacement, IfStmt const * ifStmt) 
    {
        SourceLocation end = m.getFileLoc(Lexer::getLocForEndOfToken( expr->getLocEnd(), 0, m, a.context()->getLangOpts()));
        auto exprRange = SourceRange(exprStart, end);

        a.ReplaceText(exprRange, replacement);

        auto foundIfStmt = std::find_if(bregLocations.begin(), bregLocations.end(), [=] (std::pair<unsigned, Stmt const *> p) 
                                                                                        { 
                                                                                            return p.second == ifStmt; 
                                                                                        } 
                                       );

        if ( foundIfStmt == bregLocations.end() ) {
            std::pair<unsigned, Stmt const *> p; 
            p.first = ifStmt->getLocStart().getRawEncoding() - offset; 
            p.second = ifStmt;
            bregLocations.push_back( p );
        }
        
        offset += ( exprRange.getEnd().getRawEncoding() - exprRange.getBegin().getRawEncoding() ) - replacement.size();
    }


    void report::matchBreg(BoundNodes const & nodes)
    {
        FunctionDecl const * func = nodes.getNodeAs<FunctionDecl>("func");
        DeclRefExpr const * var = nodes.getNodeAs<DeclRefExpr>("var");
        CallExpr const * call = nodes.getNodeAs<CallExpr>("call");
        IfStmt const * ifStmt = nodes.getNodeAs<IfStmt>("ifstmt");
        BinaryOperator const * assign = nodes.getNodeAs<BinaryOperator>("=");

        VarDecl const * varDecl;
           
        //found var = expr, update map
        if ( assign != nullptr ) {
            DeclRefExpr const * lhs = llvm::dyn_cast<DeclRefExpr>( assign->getLHS() );
            if (lhs != nullptr) {
                varDecl = llvm::dyn_cast<VarDecl>( lhs->getDecl() );
                varValue[varDecl] = assign->getRHS();
            }
        }
        else if ( var != nullptr ) {
            if ( var->getDecl() != nullptr ) {
                varDecl = llvm::dyn_cast<VarDecl>( var->getDecl() );
                if ( varDecl != nullptr ) { 
                    auto foundVar = varValue.find(varDecl);
                
                    //get current var value from map
                    if ( foundVar != varValue.end() ) {
                        CallExpr const * callInit = llvm::dyn_cast<CallExpr>( foundVar->second );

                        if ( callInit != nullptr ) {
                            call = callInit;
                        }
                    } 
                    else if ( varDecl->hasInit() ) {
                        Expr const * init = varDecl->getInit();
                        varValue[varDecl] = init;

                        CallExpr const * callInit = llvm::dyn_cast<CallExpr>( init );

                        if ( callInit != nullptr ) {
                            call = callInit;
                        }
                    }
                }
            }
        }

        if ( call == nullptr ) {
            return;
        }

        if ( a.manager().isMacroBodyExpansion( call->getLocStart() ) ) {
            auto r = a.manager().getExpansionRange( call->getLocStart() );

            SourceRange expansionRange = SourceRange(r.first, r.second.getLocWithOffset(-1));
            std::string bregName = a.get_source(expansionRange, true).str();

            for (auto e : bregs) {
                if ( call->getDirectCallee() != NULL ) {
                    if ( bregName == (e.first+"__value") ) {
                        std::string replacement = getBregValue(e) ? "true" : "false";

                        if ( var != nullptr ) {

                            //remove var assign
                            auto foundVarDecl = std::find(varDeclsRemoved.begin(), varDeclsRemoved.end(), varDecl);
    
                            if ( foundVarDecl == varDeclsRemoved.end() ) {
                                SourceLocation end = m.getFileLoc(Lexer::getLocForEndOfToken( varDecl->getLocEnd(), 0, m, a.context()->getLangOpts()));
                                auto varDeclRange = SourceRange(varDecl->getLocStart(), end.getLocWithOffset(1));
                                //a.ReplaceText(varDeclRange, "");

                                //offset += ( varDeclRange.getEnd().getRawEncoding() - varDeclRange.getBegin().getRawEncoding() );

                                varDeclsRemoved.push_back(varDecl);
                            }
                            
                            replaceExpr(var, var->getLocStart(), replacement, ifStmt);
                        }
                        else {
                            replaceExpr(call, expansionRange.getBegin(), replacement, ifStmt);
                        }
                    }
                }
            }
        }
    }


    void subscribe(Analyser& analyser, Visitor& visitor, PPObserver& observer) {
        analyser.onTranslationUnitDone += report(analyser);
    }

}  // close anonymous namespace

// -----------------------------------------------------------------------------

static RegisterCheck register_check(check_name, &subscribe);







