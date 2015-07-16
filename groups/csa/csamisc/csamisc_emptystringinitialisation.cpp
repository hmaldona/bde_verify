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

static std::string const check_name("emptystringinit");

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
internal::DynTypedMatcher dyn_matchEmptyStringInit()
{
	
    internal::DynTypedMatcher matcher = findAll( decl( eachOf( 
													varDecl( hasInitializer( 
												    	hasDescendant ( stringLiteral().bind("string") ) 
											    		)).bind("varDecl"),
													constructorDecl( forEachConstructorInitializer( ctorInitializer(withInitializer(
													hasDescendant( stringLiteral().bind("string")))).bind("varInit"))) ))); 
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
	
	std::string tag = "SI01";
	std::string message = "strings should not be initialized with an empty string";


	//if varibable declaration is matched 
	if (varInit == nullptr) {
		clang::VarDecl const * varDecl = nodes.getNodeAs<clang::VarDecl>("varDecl");  
		std::string varType = varDecl->getInit()->getType().getAsString();

		//rules only apply to string type
		if ( varType == "bsl::string" || 
	   	     varType == "std::string" ) {

			clang::StringLiteral const * str = nodes.getNodeAs<clang::StringLiteral>("string"); 	

			//if string literal is empty
			if (str->getLength() == 0) {
				a.report(varDecl, check_name, tag, message);

				SourceRange range = SourceRange(varDecl->getSourceRange().getBegin(), varDecl->getInit()->getExprLoc());
				
				std::string replacement = a.get_source(range, true).str() + varDecl->getNameAsString(); 
				
				a.report(varDecl, check_name, tag,
                     "Replace with %0", false, DiagnosticIDs::Note)
                << replacement
                << varDecl->getSourceRange();
            	a.ReplaceText(a.get_full_range(varDecl->getSourceRange()), replacement);
			}
		}
    }
	else {
		std::string varType = varInit->getInit()->getType().getAsString();

		if ( varType == "bsl::string" ||
		     varType == "std::string" ) {

			clang::StringLiteral const * str = nodes.getNodeAs<clang::StringLiteral>("string");
			
			if (str->getLength() == 0) {
				a.report(varInit->getSourceLocation(), check_name, tag, message) << varInit->getSourceRange(); 
			
				SourceRange range = varInit->getSourceRange();
					
				std::string replacement = varInit->getMember()->getNameAsString() + "()"; 

				a.report(varInit->getSourceLocation(), check_name, tag,
                     "Replace with %0", false, DiagnosticIDs::Note)
                << replacement
                << range;
            	a.ReplaceText(a.get_full_range(range), replacement);

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







