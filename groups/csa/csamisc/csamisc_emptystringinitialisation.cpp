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

using namespace csabase;
using namespace clang;
using namespace clang::ast_matchers;

static std::string const check_name("string-initialisation");

namespace {
    
struct data {

};


struct report : Report<data>
{
    INHERIT_REPORT_CTOR(report, Report, data);

    void stringInit(BoundNodes const & nodes);
    void operator()();
};


//matches string initialization as a variable or in an initialization list
static const internal::DynTypedMatcher& dyn_matchEmptyStringInit()
{
    
    static const internal::DynTypedMatcher& matcher = findAll( decl( eachOf( 
                                                               varDecl( hasInitializer( 
                                                                   hasDescendant ( stringLiteral().bind("string")) 
                                                                   )).bind("varDecl"),
                                                               constructorDecl( 
                                                                   forEachConstructorInitializer( 
                                                                       ctorInitializer( withInitializer(
                                                                           hasDescendant( stringLiteral().bind("string")) 
                                                                           )).bind("varInit")))))); 
    return matcher; 
}


void report::operator()()
{
    MatchFinder finder;
    csabase::OnMatch<report, &report::stringInit> m(this);
    finder.addDynamicMatcher(dyn_matchEmptyStringInit(), &m);
    finder.match(*d_analyser.context()->getTranslationUnitDecl(), *d_analyser.context());
}


void report::stringInit(BoundNodes const & nodes)
{
    clang::CXXCtorInitializer const* varInit = nodes.getNodeAs<clang::CXXCtorInitializer>("varInit");
    clang::VarDecl const * varDecl = nodes.getNodeAs<clang::VarDecl>("varDecl"); 
    clang::StringLiteral const * str = nodes.getNodeAs<clang::StringLiteral>("string"); 
    
    //no null check needed as string should always be matched
    if (str->getLength() != 0) return;

    std::string tag = "SI01";
    std::string message = "strings should not be initialized with an empty string";
    
    SourceRange range; //range of source to be replaced
    SourceLocation loc; //location of varDecl or initialiser
    std::string varType; 
    std::string replacement; //the new code 
     
    if (varInit == nullptr) { //if varibable declaration is matched
        varType = varDecl->getInit()->getType().getAsString();
        loc = varDecl->getSourceRange().getBegin();
        range = varDecl->getSourceRange();
        SourceRange declTypeRange = SourceRange(varDecl->getSourceRange().getBegin(), varDecl->getInit()->getExprLoc());
        replacement = a.get_source(declTypeRange, true).str() + varDecl->getNameAsString(); 
    }
    else { //if initialiser is matched
        varType = varInit->getInit()->getType().getAsString();
        loc = varInit->getSourceLocation();
        range = varInit->getSourceRange();
        replacement = varInit->getMember()->getNameAsString() + "()"; 
    }

    if ( varType == "bsl::string" ||
         varType == "std::string" ) {
                        
        a.report(loc, check_name, tag, message) << range; 
        a.report(loc, check_name, tag,
            "Replace with %0", false, DiagnosticIDs::Note)
            << replacement
            << range;
        a.ReplaceText(a.get_full_range(range), replacement);
    }
}


void subscribe(Analyser& analyser, Visitor& visitor, PPObserver& observer) {
    analyser.onTranslationUnitDone += report(analyser); 
}

}  // close anonymous namespace

// -----------------------------------------------------------------------------

static RegisterCheck register_check(check_name, &subscribe);







