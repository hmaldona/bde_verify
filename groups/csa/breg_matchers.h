#ifndef BREG_MATCHERS
#define BREG_MATCHERS

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

using namespace csabase;
using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;





namespace clang {
    namespace ast_matchers {
        AST_MATCHER(DeclRefExpr, nonLocalDeclRef) {
            bool nonLocal = false;
            VarDecl const * varDecl = llvm::dyn_cast<VarDecl>( Node.getDecl() );
            if ( varDecl != nullptr ) {
                if ( !varDecl->hasLocalStorage() ) {
                    nonLocal = true;    
                }
            }

            return nonLocal;
        }


        AST_MATCHER(DeclRefExpr, ptrRefDeclRefExpr) {
            bool isPtrRef = false;
            VarDecl const * varDecl = llvm::dyn_cast<VarDecl>( Node.getDecl() );
            if ( varDecl != nullptr ) {
                QualType qualType = varDecl->getType();
                if ( qualType.getTypePtr() != nullptr ) {
                    if ( qualType.getTypePtr()->isPointerType() || qualType.getTypePtr()->isReferenceType() ) {
                        if ( !qualType.getTypePtr()->getPointeeType().isConstQualified() ) {
                            isPtrRef = true;
                        }
                    }
                }
            }

            return isPtrRef ;
        }

        AST_MATCHER(MemberExpr, nonLocalMember) {
            bool nonLocal = false;
            
            DeclRefExpr const * var = llvm::dyn_cast<DeclRefExpr>( Node.getBase() );

            if ( var != nullptr ) { 
                VarDecl const * varDecl = llvm::dyn_cast<VarDecl>( var->getDecl() );
                if ( varDecl != nullptr ) {
                    if ( !varDecl->hasLocalStorage() ) {
                        nonLocal = true;    
                    }
                } 
            }
            
            return nonLocal;
        }


        AST_MATCHER(MemberExpr, localMember) {
            bool nonLocal = false;

            DeclRefExpr const * var = llvm::dyn_cast<DeclRefExpr>( Node.getBase() );

            if ( var != nullptr ) { 
                VarDecl const * varDecl = llvm::dyn_cast<VarDecl>( var->getDecl() );
                if ( varDecl != nullptr ) {
                    if ( !varDecl->hasLocalStorage() ) {
                        nonLocal = true;    
                    }
                } 
            }

            return !nonLocal;
        }

       

        AST_MATCHER(QualType, isNonConstPtr) {
            if ( Node.getTypePtr() != nullptr ) {
                if ( Node.getTypePtr()->isPointerType() || Node.getTypePtr()->isReferenceType() ) {
                    if ( !Node.getTypePtr()->getPointeeType().isConstQualified() ) {
                        return true;
                    }
                }
            }
            return false;
        }
    };
};


namespace Matchers {

    internal::Matcher<Expr> deepExpr( internal::Matcher<Expr> matcher ); 
    internal::Matcher<UnaryOperator> isIncOrDec( internal::Matcher<UnaryOperator> matcher );
    internal::Matcher<Decl> ptrRefVarDecl( internal::Matcher<VarDecl> matcher );
    internal::Matcher<QualType> isPtrRef();

    extern internal::DynTypedMatcher varDeclMatcher;
    extern internal::DynTypedMatcher assignMatcher;
    


    extern internal::DynTypedMatcher incOrDecMatcher;
    extern internal::DynTypedMatcher callMatcher;

    
    extern internal::DynTypedMatcher ptrFieldDeclMatcher;
    
    extern internal::DynTypedMatcher hasCompoundStmtAncestor;
    
    template<typename T>
    std::pair<T const *, bool> getNodeInExpr(clang::ASTContext * context, Stmt const * e, const internal::DynTypedMatcher &NodeMatch )
    { 
        if ( e == nullptr ) {
            return std::pair<T const *, bool>( nullptr, false );
        }

        const internal::DynTypedMatcher& matcher = NodeMatch;

        MatchFinder finder;

        T const * node = nullptr;
        bool found = false;

        OnMatch<> m([&node, &found](const BoundNodes &nodes) { 
                T const * n = nodes.getNodeAs<T>("node");
                node = n;
                found = true;
                });


        finder.addDynamicMatcher(matcher, &m);

        finder.match(*e, *context );

        if ( found == false || node == nullptr ) {
            return std::pair<T const *, bool>( nullptr, found );
        }

        return std::pair<T const *, bool>( node , found );
    }


    template<typename T>
    std::pair<T const *, bool> getNodeInExpr(clang::ASTContext * context, Decl const * d, const internal::DynTypedMatcher &NodeMatch )
    { 
        if ( d == nullptr ) {
            return std::pair<T const *, bool>( nullptr, false );
        }

        const internal::DynTypedMatcher& matcher = NodeMatch;

        MatchFinder finder;

        T const * node = nullptr;
        bool found = false;

        OnMatch<> m([&node, &found](const BoundNodes &nodes) { 
                T const * n = nodes.getNodeAs<T>("node");
                node = n;
                found = true;
                });


        finder.addDynamicMatcher(matcher, &m);

        finder.match(*d, *context );

        if ( found == false || node == nullptr ) {
            return std::pair<T const *, bool>( nullptr, found );
        }

        return std::pair<T const *, bool>( node, found );
    }


};

#endif
