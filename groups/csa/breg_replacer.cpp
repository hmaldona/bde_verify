#include "breg_replacer.h"
#include "breg_options.h"
#include "breg_matchers.h"

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

#include <csautil_bregistry.h>


using namespace csabase;
using namespace csautil;
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
        BregOptions options;

        BRegistry registry;

        report(Analyser& analyser, PPObserver::CallbackType type = PPObserver::e_None);

        void replaceExpr(SourceRange exprRange, std::string replacement, IfStmt const * ifStmt);
        void replaceExpr(Expr const * expr, SourceLocation exprStart, std::string replacement, IfStmt const * ifStmt);
        void replaceExpr(Expr const * expr, SourceLocation exprStart, std::string replacement); 
        std::string getStmtBodyReplacement(CompoundStmt const * stmt, unsigned column);


        void matchBreg(BoundNodes const & nodes);

        bool getBregValue(std::pair<std::string, boolValue> pair); 


        void operator()();
        void operator()(SourceLocation where, bool, std::string const& name); 
    };


    report::report(Analyser& analyser, PPObserver::CallbackType type)
        : Report<data>(analyser, type)
    {
        offset = 0;
           
        this->options = BregOptions();
        std::string csvFile = a.config()->value("breg_file");
        
        bregs = readCSV(csvFile);
    }
    

    //matches string initialization as a variable or in an initialization list
    static const internal::DynTypedMatcher& dyn_matchBreg()
    {
        static const internal::DynTypedMatcher matcher = findAll( functionDecl( hasDescendant( 
                                                             stmt( findAll( stmt(
                                                                 eachOf(
                                                                     ifStmt( hasCondition( expr( findAll( 
                                                                         callExpr().bind("call"))))).bind("ifstmt"),
                                                                     ifStmt( hasCondition( expr( findAll( 
                                                                         declRefExpr().bind("var"))))).bind("ifstmt"),
                                                                     binaryOperator( hasOperatorName("=")).bind("="),
                                                                     declStmt( hasSingleDecl( varDecl().bind("varDecl") ) )
                                                                         )))))).bind("func") );
                                                                         
        return matcher; 
    }

    void report::operator()()
    {
        MatchFinder finder;
        csabase::OnMatch<report, &report::matchBreg> m(this);
        finder.addDynamicMatcher(dyn_matchBreg(), &m);
        finder.match(*d_analyser.context()->getTranslationUnitDecl(), *d_analyser.context());

        this->registry = BRegistry(7034648, 60);

        //write locations of breg if stmts
        std::ofstream bregFile;
        bregFile.open(".bregLocs");

        for (auto i : bregLocations) {
            bregFile << i.first << "\n";
        }

        bregFile.close();
    }

    // replace with actual breg

    bool report::getBregValue(std::pair<std::string, boolValue> pair) 
    {
        if (pair.second == defaultValue) {
           
            //hook database code in here
            
            BREGEntryInfo bregInfo = BREGEntryInfo();

            std::string breg = pair.first;
           
            std::cout << breg << std::endl;

            bool bregFound = registry.getEntryInformation( bregInfo, breg );
            std::cout << registry.getEntryByValue( bregInfo, breg, 8 ) << std::endl;
            std::cout << bregInfo.d_prodValue << std::endl;

            if ( bregFound ) {
                registry.getEntryValueInformation( bregInfo, breg );
            }
            return bregInfo.d_prodValue;
        }
        
        return pair.second;
    } 


    void report::replaceExpr(SourceRange exprRange, std::string replacement, IfStmt const * ifStmt) 
    {
        a.ReplaceText(exprRange, replacement);

        auto foundIfStmt = std::find_if(bregLocations.begin(), bregLocations.end(), [=] (std::pair<unsigned, Stmt const *> p) 
                                                                                        { 
                                                                                            return p.second == ifStmt; 
                                                                                        } 
                                       );

        if ( foundIfStmt == bregLocations.end() ) {
            std::pair<unsigned, Stmt const *> p; 
            p.first = exprRange.getBegin().getRawEncoding() - offset; 
            p.second = ifStmt;
            bregLocations.push_back( p );
        }

        offset += ( exprRange.getEnd().getRawEncoding() - exprRange.getBegin().getRawEncoding() ) - replacement.size();
    }

    
    void report::replaceExpr(Expr const * expr, SourceLocation exprStart, std::string replacement, IfStmt const * ifStmt) 
    {
        SourceLocation end = m.getFileLoc(Lexer::getLocForEndOfToken( expr->getLocEnd(), 0, m, a.context()->getLangOpts()));
        auto exprRange = SourceRange(exprStart, end.getLocWithOffset(0));

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

    void report::replaceExpr(Expr const * expr, SourceLocation exprStart, std::string replacement) 
    {
        SourceLocation end = m.getFileLoc(Lexer::getLocForEndOfToken( expr->getLocEnd(), 0, m, a.context()->getLangOpts()));
        auto exprRange = SourceRange(exprStart, end);

        a.ReplaceText(exprRange, replacement);
        
        offset += ( exprRange.getEnd().getRawEncoding() - exprRange.getBegin().getRawEncoding() ) - replacement.size();
    }

    // match if-stmts 

    void report::matchBreg(BoundNodes const & nodes)
    {
        DeclRefExpr const * var = nodes.getNodeAs<DeclRefExpr>("var");
        CallExpr const * call = nodes.getNodeAs<CallExpr>("call");
        IfStmt const * ifStmt = nodes.getNodeAs<IfStmt>("ifstmt");
        BinaryOperator const * assign = nodes.getNodeAs<BinaryOperator>("=");
        VarDecl const * varDecl = nodes.getNodeAs<VarDecl>("varDecl");

        if ( varDecl != nullptr ) {
            Expr const * init = varDecl->getInit();
            varValue[varDecl] = init;
            
            if ( init != nullptr ) {
                CallExpr const * callInit = llvm::dyn_cast<CallExpr>( init );

                if ( callInit != nullptr ) {
                    call = callInit;
                }
            }
        }
        else if ( assign != nullptr ) {
            DeclRefExpr const * lhs = llvm::dyn_cast<DeclRefExpr>( assign->getLHS() );
            if (lhs != nullptr) {
                varDecl = llvm::dyn_cast<VarDecl>( lhs->getDecl() );
                varValue[varDecl] = assign->getRHS();
                CallExpr const * callInit = llvm::dyn_cast<CallExpr>( assign->getRHS() );

                if ( callInit != nullptr ) {
                    call = callInit;
                }
            }
        }
        else if ( var != nullptr ) {
            if ( var->getDecl() != nullptr ) {
                varDecl = llvm::dyn_cast<VarDecl>( var->getDecl() );
                if ( varDecl != nullptr ) { 
                    auto foundVar = varValue.find(varDecl);
                
                    //get current var value from map
                    if ( foundVar != varValue.end() ) {
                        if ( foundVar->second != nullptr ) {
                            CallExpr const * callInit = llvm::dyn_cast<CallExpr>( foundVar->second );

                            if ( callInit != nullptr ) {
                                call = callInit;
                            }
                        }
                    } 
                }
            }
        }

        if ( call != nullptr ) {
            if ( a.manager().isMacroBodyExpansion( call->getLocStart() ) ) {
                auto r = a.manager().getExpansionRange( call->getLocStart() );

                SourceRange expansionRange = SourceRange(r.first, r.second.getLocWithOffset(1));
                std::string expansion = a.get_source(expansionRange, true).str();
                
                if ( call->getDirectCallee() != nullptr ) {
                    if ( call->getNumArgs() > 0 ) {
                        std::pair<DeclRefExpr const *, bool> bregVar = Matchers::getNodeInExpr<DeclRefExpr>( a.context(), 
                                call->getArgs()[0], 
                                expr( hasDescendant( declRefExpr().bind("node"))));
                        if ( bregVar.second == true ) {

                            std::string callName = call->getDirectCallee()->getNameAsString();
                            if ( callName == "bregdb_eval_bbitcxt_bool_rv" ) {

                                bool foundBreg = false;
                                std::string bregName = bregVar.first->getFoundDecl()->getNameAsString();
                                std::string replacement;

                                if ( this->options.bregValue != csv ) {
                                    replacement = getBregValue( std::make_pair( bregName, defaultValue ) ) ? "true" : "false";
                                    foundBreg = true;
                                }
                                else {
                                    for ( auto e : bregs ) {
                                        if ( bregName == e.first ) {
                                            replacement = getBregValue( e ) ? "true" : "false";
                                            foundBreg = true;
                                            break;
                                        }
                                    }
                                }


                                if ( foundBreg ) {
                                    if ( expansion.find("BREG_ON") != std::string::npos ) {
                                        unsigned int column = a.manager().getPresumedColumnNumber( ifStmt->getLocStart() );
                                        replacement = "if ( " + replacement + " ) {";
                                        SourceRange r1 = SourceRange( r.first, r.first.getLocWithOffset(expansion.size()) );
                                        replaceExpr(r1, replacement, ifStmt);

                                        SourceRange r2 = a.get_line_range( ifStmt->getLocEnd() );
                                        a.ReplaceText(r2, std::string(column-1, ' ') + "}");
                                        offset += ( r2.getEnd().getRawEncoding() - r2.getBegin().getRawEncoding() ) - (column);
                                    }
                                    else if ( var != nullptr ) {
                                        replaceExpr(var, var->getLocStart(), replacement, ifStmt);
                                    }
                                    else if ( ifStmt != nullptr ) {
                                        replaceExpr(call, expansionRange.getBegin(), replacement, ifStmt);
                                    }
                                    else {
                                        replaceExpr(call, expansionRange.getBegin(), replacement);
                                    }

                                }
                            }
                        }
                    }
                }


                //for (auto e : bregs) {
                //    if ( call->getDirectCallee() != nullptr ) {
                //        std::string callName = call->getDirectCallee()->getNameAsString();
                //        
                //        if ( call->getNumArgs() > 0 ) {
                //            std::pair<DeclRefExpr const *, bool> bregVar = Matchers::getNodeInExpr<DeclRefExpr>( a.context(), call->getArgs()[0], expr( hasDescendant( declRefExpr().bind("node"))));
                //            if ( bregVar.second == true ) {

                //                std::string bregName = bregVar.first->getFoundDecl()->getNameAsString();
                //                if ( callName == "bregdb_eval_bbitcxt_bool_rv" 
                //                        && ( ( bregName == e.first || bregName == e.first+"__value") || this->options.bregValue != csv ) ) {
                //                    std::string replacement = getBregValue(e) ? "true" : "false";

                //                    if ( expansion.find("BREG_ON") != std::string::npos ) {
                //                        unsigned int column = a.manager().getPresumedColumnNumber( ifStmt->getLocStart() );
                //                        replacement = "if ( " + replacement + " ) {";
                //                        SourceRange r1 = SourceRange( r.first, r.first.getLocWithOffset(expansion.size()) );
                //                        replaceExpr(r1, replacement, ifStmt);

                //                        SourceRange r2 = a.get_line_range( ifStmt->getLocEnd() );
                //                        a.ReplaceText(r2, std::string(column-1, ' ') + "}");
                //                        offset += ( r2.getEnd().getRawEncoding() - r2.getBegin().getRawEncoding() ) - (column);
                //                    }
                //                    else if ( var != nullptr ) {
                //                        replaceExpr(var, var->getLocStart(), replacement, ifStmt);
                //                    }
                //                    else if ( ifStmt != nullptr ) {
                //                        replaceExpr(call, expansionRange.getBegin(), replacement, ifStmt);
                //                    }
                //                    else {
                //                        replaceExpr(call, expansionRange.getBegin(), replacement);
                //                    }
                //                    return; 
                //                }
                //            }
                //        }
                //    }
                //}
            }
        }
    }


    void subscribe(Analyser& analyser, Visitor& visitor, PPObserver& observer) {
        analyser.onTranslationUnitDone += report(analyser);
    }

}  // close anonymous namespace

// -----------------------------------------------------------------------------

static RegisterCheck register_check(check_name, &subscribe);







