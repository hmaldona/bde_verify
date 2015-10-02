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

#include <iostream>
#include <sstream>

using namespace csabase;
using namespace clang;
using namespace clang::ast_matchers;

static std::string const check_name("complexity");

namespace {
    
struct data {
    // Maximum complexity per function
    int d_max_complexity;
    // Maps each function to its complexity value
    std::map<FunctionDecl const *, int> complexDict;
    int totalFileComplexity = 0;
};


struct report : Report<data> {
    INHERIT_REPORT_CTOR(report, Report, data);
    void calculateComplex(BoundNodes const & nodes);
    void operator()();
    void generateReport();
};


// Matches every if,switch,while,for,try,catch inside a function declaration
static internal::DynTypedMatcher dyn_matchComplexity() 
{
    static internal::DynTypedMatcher matcher =
    findAll(functionDecl(hasDescendant(
        findAll(stmt(eachOf( 
                            ifStmt().bind("if"),
                            switchCase().bind("switch") , 
                            whileStmt().bind("while") ,
                            forStmt().bind("for"),
                            tryStmt().bind("try"),
                            catchStmt().bind("catch"),
                            conditionalOperator().bind("?operator"),
                            ifStmt(hasCondition(findAll(binaryOperator(eachOf(hasOperatorName("&&") , hasOperatorName("||"))).bind("condOp")))),
                            whileStmt(hasCondition(findAll(binaryOperator(eachOf(hasOperatorName("&&") , hasOperatorName("||"))).bind("condOp"))))
                            ))))).bind("funcDecl"));
    return matcher; 
}


void report::operator()()
{
    MatchFinder finder;
    csabase::OnMatch<report, &report::calculateComplex> m(this);
    finder.addDynamicMatcher(dyn_matchComplexity(), &m);
    finder.match(*d_analyser.context()->getTranslationUnitDecl(), *d_analyser.context());
    generateReport();
}

// Generates report if any of the functions exceeded the verified threshold
void report::generateReport(){
    // Get the complexity threshold
    data& d = a.attachment<data>();
    std::string tag = "CX01";
    std::string message = "Function complexity Exceeded the maximum threshold ! Threshold verified: " 
    + std::to_string(d.d_max_complexity) + " Found: ";
    //location of function
    SourceLocation loc; 

    for(auto curFunc : d.complexDict){
        if (curFunc.second > d.d_max_complexity){
            loc = curFunc.first->getLocStart();
            std::string val = std::to_string(curFunc.second);
            a.report(loc, check_name, tag, message + val);
        }
        d.totalFileComplexity += curFunc.second;
    }
    
}


void report::calculateComplex(BoundNodes const & nodes)
{
    FunctionDecl const* curFunc = nodes.getNodeAs<FunctionDecl>("funcDecl");
    IfStmt const * curIf = nodes.getNodeAs<IfStmt>("if");
    SwitchCase const * curSwitch = nodes.getNodeAs<SwitchCase>("switch");
    WhileStmt const * curWhile = nodes.getNodeAs<WhileStmt>("while");
    ForStmt const * curFor = nodes.getNodeAs<ForStmt>("for");
    CXXTryStmt const * curTry = nodes.getNodeAs<CXXTryStmt>("try");
    CXXCatchStmt const * curCatch = nodes.getNodeAs<CXXCatchStmt>("catch");
    BinaryOperator const * curBin = nodes.getNodeAs<BinaryOperator>("condOp");
    ConditionalOperator const * curCond = nodes.getNodeAs<ConditionalOperator>("?operator");

    
    // If matched function is not in the same .cpp file i.e:includes
    if (curFunc != nullptr && a.is_component(curFunc)){
        if (d.complexDict.find(curFunc) == d.complexDict.end()){
            d.complexDict[curFunc] = 1;
        }
        if (curWhile != nullptr || curSwitch != nullptr || curFor != nullptr || curIf != nullptr
                || curTry != nullptr || curCatch != nullptr || curBin != nullptr || curCond != nullptr){
                  d.complexDict[curFunc] += 1;    
        }
    }
}

void subscribe(Analyser& analyser, Visitor& visitor, PPObserver& observer) {
    std::istringstream is(analyser.config()->value("max_function_complexity"));
    data& d = analyser.attachment<data>();
    if (!(is >> d.d_max_complexity)) {
        d.d_max_complexity = 10;
    }
    analyser.onTranslationUnitDone += report(analyser); 
}

}  // close anonymous namespace

// -----------------------------------------------------------------------------

static RegisterCheck register_check(check_name, &subscribe);