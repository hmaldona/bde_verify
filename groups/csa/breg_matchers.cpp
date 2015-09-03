#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchersInternal.h>
#include <clang/ASTMatchers/ASTMatchersMacros.h>
#include <clang/Lex/Lexer.h>
#include <clang/AST/Stmt.h>
#include <clang/Sema/Sema.h>
#include <clang/Frontend/CompilerInstance.h>
#include <csabase_ppobserver.h>

#include "llvm/ADT/Twine.h"
#include "llvm/Support/Regex.h"

#include <csabase_analyser.h>
#include <csabase_debug.h>
#include <csabase_diagnostic_builder.h>
#include <csabase_registercheck.h>
#include <csabase_report.h>
#include <csabase_util.h>
#include <csabase_visitor.h>


#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <utility>
#include <iostream>

#include "breg_matchers.h"

using namespace csabase;
using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;
using namespace Matchers;


    internal::Matcher<Expr> Matchers::deepExpr( internal::Matcher<Expr> matcher ) { 
        return expr( eachOf( 
                hasDescendant( expr( matcher)), 
                matcher
                ));
    }

    internal::Matcher<UnaryOperator> Matchers::isIncOrDec( internal::Matcher<UnaryOperator> matcher ) { 
        return expr( eachOf( 
            unaryOperator( hasOperatorName("++"), matcher),
            unaryOperator( hasOperatorName("--"), matcher)
            ));
    }

    internal::Matcher<Decl> Matchers::ptrRefVarDecl( internal::Matcher<VarDecl> matcher ) { 
        return varDecl( eachOf( hasType( pointerType( pointee( unless( isConstQualified())))), 
                                hasType( referenceType( pointee( unless( isConstQualified()))))), 
                        matcher);
    }

    internal::Matcher<QualType> Matchers::isPtrRef() { 
        return qualType( anyOf( pointerType( pointee( unless( isConstQualified()))), 
                                referenceType( pointee( unless( isConstQualified()))))
                   );
    }



    internal::DynTypedMatcher Matchers::varDeclMatcher = declStmt( hasSingleDecl( ptrRefVarDecl( 
                                                                        hasInitializer( eachOf( 
                                                                            hasDescendant( declRefExpr( nonLocalDeclRef())), 
                                                                            hasDescendant( callExpr( callee( functionDecl( returns( isNonConstPtr())))))
                                                                            )))));

    internal::DynTypedMatcher Matchers::assignMatcher = expr( eachOf( 
                                            binaryOperator( hasOperatorName("="), hasLHS( declRefExpr( nonLocalDeclRef()))),
                                            binaryOperator( hasOperatorName("="), hasLHS( declRefExpr( ptrRefDeclRefExpr())), 
                                                                                  hasRHS( expr( deepExpr( declRefExpr( nonLocalDeclRef()))))),
                                            binaryOperator( hasOperatorName("="), hasLHS( memberExpr( localMember(), hasType( isPtrRef()))), 
                                                                                  hasRHS( expr( deepExpr( declRefExpr( nonLocalDeclRef()))))),
                                            binaryOperator( hasOperatorName("="), hasLHS( memberExpr( nonLocalMember()))),
                                            binaryOperator( hasOperatorName("="), hasLHS( thisExpr().bind("node") ))
                                            ));

    


    internal::DynTypedMatcher Matchers::incOrDecMatcher = expr( unaryOperator( isIncOrDec( hasUnaryOperand( eachOf( 
                                                                             expr( deepExpr( declRefExpr( nonLocalDeclRef()))), 
                                                                             expr( deepExpr( memberExpr( nonLocalMember()))),
                                                                             expr( deepExpr( thisExpr().bind("node") )),
                                                                             expr( deepExpr( callExpr( callee( functionDecl( returns( isNonConstPtr()))))))
                                                                             )))));

    internal::DynTypedMatcher Matchers::callMatcher = expr( callExpr().bind("node") );

    
    internal::DynTypedMatcher Matchers::ptrFieldDeclMatcher = valueDecl( hasType( recordDecl( forEachDescendant( fieldDecl( hasType( isPtrRef()))))));

    internal::DynTypedMatcher Matchers::hasCompoundStmtAncestor = stmt( hasAncestor( compoundStmt().bind("node")));
