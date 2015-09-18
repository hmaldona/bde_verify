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

static std::string const check_name("complexity-check");

namespace {
    
struct data {
    // Maximum complexity per method
    int d_max_complexity;
};


struct report : Report<data> {

    INHERIT_REPORT_CTOR(report, Report, data);

    void calculateComplex(BoundNodes const & nodes);
    void operator()();
    void generateReport();

    // Maps each function to its complexity value
    std::map<FunctionDecl const *, int> complexDict;
    // Total file complexity
    int complexity = 0;
};


// Matches every if,switch,while,for,try,catch inside a method declaration
static const internal::DynTypedMatcher& dyn_matchComplexity() 
{
    static const internal::DynTypedMatcher& matcher =
    findAll(functionDecl(hasDescendant(stmt(eachOf(
        findAll(stmt(ifStmt().bind("if"))), 
        findAll(stmt(switchCase().bind("switch"))),
        findAll(stmt(whileStmt().bind("while"))),
        findAll(stmt(forStmt().bind("for"))),
        findAll(stmt(tryStmt().bind("try"))),
        findAll(stmt(catchStmt().bind("catch"))),
        findAll(stmt(ifStmt(hasCondition(findAll(binaryOperator(hasOperatorName("&&")).bind("condOp")))))),
        findAll(stmt(ifStmt(hasCondition(findAll(binaryOperator(hasOperatorName("||")).bind("condOp"))))))
        )))).bind("methodDecl"));

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

// Generates report if any of the methods exceeded the verified threshold
void report::generateReport(){
    // Get the complexity threshold
    data& d = a.attachment<data>();
    std::string tag = "CX01";
    std::string message = "Complexity Exceeded the maximum threshold ! Threshold verified: " 
    + std::to_string(d.d_max_complexity) + " Found: ";
    //location of method
    SourceLocation loc; 
    
    for(auto curMethod : complexDict){
        if (curMethod.second > d.d_max_complexity){
            loc = curMethod.first->getLocStart();
            std::string val = std::to_string(curMethod.second);
            a.report(loc, check_name, tag, message + val);
        }
        // Add to total File complexity
        complexity += curMethod.second;
    }
    
}


void report::calculateComplex(BoundNodes const & nodes)
{
    FunctionDecl const* curMethod = nodes.getNodeAs<FunctionDecl>("methodDecl");
    IfStmt const * curIf = nodes.getNodeAs<IfStmt>("if");
    SwitchCase const * curSwitch = nodes.getNodeAs<SwitchCase>("switch");
    WhileStmt const * curWhile = nodes.getNodeAs<WhileStmt>("while");
    ForStmt const * curFor = nodes.getNodeAs<ForStmt>("for");
    CXXTryStmt const * curTry = nodes.getNodeAs<CXXTryStmt>("try");
    CXXCatchStmt const * curCatch = nodes.getNodeAs<CXXCatchStmt>("catch");
    BinaryOperator const * curBin = nodes.getNodeAs<BinaryOperator>("condOp");

    SourceManager &sm = a.manager();
    const auto& fId = sm.getMainFileID();
    const auto& fileId = sm.getFileID(curMethod->getLocation());

    // If matched method is not in the same .cpp file i.e:includes
    if (fId == fileId){
        if (curWhile != nullptr || curSwitch != nullptr || curFor != nullptr || curIf != nullptr
            || curTry != nullptr || curCatch != nullptr || curBin != nullptr){
            if (complexDict.find(curMethod) != complexDict.end()){
                complexDict[curMethod] = complexDict[curMethod] + 1;    
            }else{
                // 2: a basic 1 for each method + the first occurence
                complexDict[curMethod] = 2;
            }
        }
    }
}

void subscribe(Analyser& analyser, Visitor& visitor, PPObserver& observer) {
    std::istringstream is(analyser.config()->value("max_complexity"));
    data& d = analyser.attachment<data>();
    if (!(is >> d.d_max_complexity)) {
        d.d_max_complexity = 10;
    }
    analyser.onTranslationUnitDone += report(analyser); 
}

}  // close anonymous namespace

// -----------------------------------------------------------------------------

static RegisterCheck register_check(check_name, &subscribe);


