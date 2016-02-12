// csabbg_classsections.cpp                                           -*-C++-*-

#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/DeclarationName.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/Specifiers.h>
#include <csabase_analyser.h>
#include <csabase_config.h>
#include <csabase_debug.h>
#include <csabase_diagnostic_builder.h>
#include <csabase_location.h>
#include <csabase_ppobserver.h>
#include <csabase_registercheck.h>
#include <csabase_report.h>
#include <csabase_util.h>
#include <ctype.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/Regex.h>
#include <stddef.h>
#include <stdlib.h>
#include <utils/event.hpp>
#include <utils/function.hpp>
#include <cctype>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace csabase { class Visitor; }

using namespace clang;
using namespace csabase;

// ----------------------------------------------------------------------------

static std::string const check_name("class-sections");

// ----------------------------------------------------------------------------

namespace
{

struct TagInfo
{
    enum TagTypes
    {
        BitFunction = 0x0010,
        BitData = 0x0020,
        BitType = 0x0040,
        BitOther = 0x0080,
        BitPrivate = 0x0100,
        BitPublic = 0x0200,
        BitImplicitPrivate = 0x0400,
        BitImplicitPublic = 0x0800,
        BitWrong = 0x1000,

        Accessors = BitFunction + 1 + BitImplicitPublic,
        Aspects = BitFunction + 2 + BitImplicitPublic,
        Creators = BitFunction + 3 + BitImplicitPublic,
        ClassMethods = BitFunction + 4 + BitImplicitPublic,
        FreeOperators = BitFunction + 5 + BitImplicitPublic,
        Manipulators = BitFunction + 6 + BitImplicitPublic,
        NotImplemented = BitFunction + 7 + BitImplicitPrivate,
        Traits = BitFunction + 8 + BitImplicitPublic,

        ClassData = BitData + 1 + BitImplicitPrivate,
        Data = BitData + 2 + BitImplicitPrivate,
        Constant = BitData + 3 + BitImplicitPublic,

        Friends = BitOther + 1,
        None = BitOther + 2,

        Types = BitType + 1 + BitImplicitPublic,

        FreeFunctions = BitWrong + BitFunction + 1,
    };

    const char *tag;
    TagTypes    type;
} const tags[] = {
    {"", TagInfo::None},
    {"ACCESSOR", TagInfo::Accessors},
    {"ASPECT", TagInfo::Aspects},
    {"CLASS DATA", TagInfo::ClassData},
    {"CLASS METHOD", TagInfo::ClassMethods},
    {"CONSTANT", TagInfo::Constant},
    {"CREATOR", TagInfo::Creators},
    {"DATA", TagInfo::Data},
    {"DATA MEMBER", TagInfo::Data},
    {"FREE FUNCTION", TagInfo::FreeFunctions},
    {"FREE OPERATOR", TagInfo::FreeOperators},
    {"FRIEND", TagInfo::Friends},
    {"FUNCTION", TagInfo::Manipulators},
    {"MANIPULATOR", TagInfo::Manipulators},
    {"NOT IMPLEMENTED", TagInfo::NotImplemented},
    {"TRAIT", TagInfo::Traits},
    {"TYPE", TagInfo::Types},
};

struct TagData
{
    SourceRange              range;  // tag location
    const TagInfo::TagTypes  type;   // type of tag
    const TagDecl           *decl;   // owning definition; global if nullptr

    TagData(SourceRange, TagInfo::TagTypes, const TagDecl *);
};

TagData::TagData(SourceRange        range,
                 TagInfo::TagTypes  type,
                 const TagDecl     *decl)
: range(range), type(type), decl(decl)
{
}

struct Sort
{
    enum E { Comment, RangeStart, RangeEnd } type;
    union {
        TagData *comment;
        const TagDecl *scope;
    };

    Sort(E, TagData *);
    Sort(E, const TagDecl *);
};

Sort::Sort(E type, TagData *comment) : type(type), comment(comment) { }
Sort::Sort(E type, const TagDecl *scope) : type(type), scope(scope) { }

struct data
    // Data attached to analyzer for this check.
{

    typedef std::vector<TagData> Tags;
    Tags d_tags;  // Tag locations, as per above array.

    typedef std::set<const TagDecl *> Defs;
    Defs d_defs;  // Tag (class, union, etc.) definitions.

    typedef std::set<const Decl *> Decls;
    Decls d_decls;  // All declarations.
};

struct report : public Report<data>
    // Callback object invoked upon completion.
{
    INHERIT_REPORT_CTOR(report, Report, data);

    SourceLocation getLoc(SourceLocation loc) const;
    SourceLocation getLoc(const TagData &td) const;
    SourceLocation getLoc(const Decl *decl) const;
    SourceLocation getLoc(const Sort &sort) const;
    SourceLocation getDCLoc(const Decl *decl) const;
        // Get location for object.

    void operator()();
        // Invoked to process reports.

    void operator()(SourceRange range);
        // Callback function for processing comments.

    template <typename A, typename B>
    bool operator()(const A& a, const B& b) const;
        // Compare the specified 'a' and 'b' by translation unit position.

    void operator()(const TagDecl *decl);
        // Callback function for processing tag declarations.

    void operator()(const Decl *decl);
        // Callback function for processing all declarations.

    void tagIsHere(data::Tags::const_iterator i, const char *code);
        // Issue the "Tag is here" note with the specified code for i.

    bool requirePublic(const Decl *decl, data::Tags::const_iterator i);
    bool requirePrivate(const Decl *decl, data::Tags::const_iterator i);
    bool requireFunction(const Decl *decl, data::Tags::const_iterator i);
    bool requireField(const Decl *decl, data::Tags::const_iterator i);
    bool requireVar(const Decl *decl, data::Tags::const_iterator i);
    bool requireType(const Decl *decl, data::Tags::const_iterator i);
    bool requireConstMethod(const Decl *decl, data::Tags::const_iterator i);
    bool requireNotConstMethod(const Decl *decl, data::Tags::const_iterator i);
    bool requireStaticMethod(const Decl *decl, data::Tags::const_iterator i);
    bool requireCDtor(const Decl *decl, data::Tags::const_iterator i);
    bool requireConstVar(const Decl *decl, data::Tags::const_iterator i);
    bool requireFreeOperator(const Decl *decl, data::Tags::const_iterator i);
    bool requireFreeFunction(const Decl *decl, data::Tags::const_iterator i);
    bool requireConversionMethod(const Decl                 *decl,
                                 data::Tags::const_iterator  i);
        // Issue various warnings for the specified 'decl' and 'i'.
};

SourceLocation report::getLoc(SourceLocation loc) const
{
    return m.getFileLoc(loc);
    return m.getExpansionLoc(loc);
    return m.getSpellingLoc(loc);
    return loc;
}

SourceLocation report::getLoc(const TagData &td) const
{
    return getLoc(td.range.getBegin());
}

SourceLocation report::getLoc(const Decl *decl) const
{
    return getLoc(decl->getLocStart());
}

SourceLocation report::getLoc(const Sort &sort) const
{
    switch (sort.type) {
    default:
    case Sort::Comment:
        return getLoc(sort.comment->range.getBegin());
    case Sort::RangeStart:
        return getLoc(sort.scope->getOuterLocStart());
    case Sort::RangeEnd:
        return getLoc(sort.scope->getRBraceLoc());
    }
}

SourceLocation report::getDCLoc(const Decl *decl) const
{
    auto rd = llvm::dyn_cast<RecordDecl>(decl->getDeclContext());
    return rd ? getLoc(rd->getLocStart()) :
                m.getLocForStartOfFile(m.getMainFileID());
}


template <typename A, typename B>
bool report::operator()(const A& a, const B& b) const
{
    return m.isBeforeInTranslationUnit(getLoc(a), getLoc(b));
}

void report::operator()(const TagDecl *decl)
{
    if (decl == decl->getCanonicalDecl() &&
        decl->isThisDeclarationADefinition() &&
        decl->getRBraceLoc().isValid() &&
        getLoc(decl).isFileID() &&
        decl->getRBraceLoc().isFileID()) {
        d.d_defs.insert(decl);
    }
}

void report::operator()(const Decl *decl)
{
    if (decl == decl->getCanonicalDecl() && getLoc(decl).isValid()) {
        if (llvm::dyn_cast<AccessSpecDecl>(decl) ||
            llvm::dyn_cast<UsingDecl>(decl) ||
            llvm::dyn_cast<UsingShadowDecl>(decl)) {
            return;                                                   // RETURN
        }
        if (auto rd = llvm::dyn_cast<RecordDecl>(decl)) {
            if (rd->isInjectedClassName()) {
                return;                                               // RETURN
            }
        }
        if (auto td = llvm::dyn_cast<TagDecl>(decl)) {
            if (!td->hasNameForLinkage()) {
                return;                                               // RETURN
            }
        }
        auto fd = llvm::dyn_cast<FunctionDecl>(decl);
        bool include = false;
        if (fd) {
            include = !fd->isDefaulted() &&
                      !fd->isMain() &&
                      !fd->getLocation().isMacroID() &&
                      fd->getTemplatedKind() == fd->TK_NonTemplate &&
                      fd->getLinkageInternal() == Linkage::ExternalLinkage;
        }
        else if (decl->getDeclContext()->isRecord()) {
            include = llvm::dyn_cast<RecordDecl>(decl->getDeclContext())
                          ->hasNameForLinkage();
        }
        if (include) {
            d.d_decls.insert(decl);
        }
    }
}

void report::operator()(SourceRange range)
{
    llvm::StringRef comment = a.get_source(range);
    if (range.isValid() && comment.startswith("//")) {
        comment = comment.drop_front(2).trim();
        bool saysPublic = false;
        bool saysPrivate = false;
        if (comment.startswith_lower("public ")) {
            comment = comment.drop_front(6).trim();
            saysPublic = true;
        }
        else if (comment.startswith_lower("private ")) {
            comment = comment.drop_front(7).trim();
            saysPrivate = true;
        }
        if (comment.startswith_lower("instance ")) {
            comment = comment.drop_front(8).trim();
        }
        if (comment.endswith_lower("s")) {
            comment = comment.drop_back(1).trim();
        }
        if (comment.size() && std::isupper(comment[0] & 0xFFU)) {
            for (const auto& tag : tags) {
                if (comment.equals_lower(tag.tag)) {
                    auto type = tag.type;
                    if (saysPublic) {
                        type = TagInfo::TagTypes(type | TagInfo::BitPublic);
                    }
                    else if (saysPrivate) {
                        type = TagInfo::TagTypes(type | TagInfo::BitPrivate);
                    }
                    d.d_tags.emplace_back(range, type, nullptr);
                    break;
                }
            }
        }
    }
}

void report::tagIsHere(data::Tags::const_iterator i, const char *code)
{
    a.report(i->range.getBegin(), check_name, code,
             "Tag is here", true, DiagnosticIDs::Note);
}

bool report::requirePublic(const Decl *decl, data::Tags::const_iterator i)
{
    if (decl->getAccess() == AS_private ||
        decl->getAccess() == AS_protected) {
        a.report(decl, check_name, "KS01", "Tag requires public declaration");
        tagIsHere(i, "KS01");
        return false;                                                 // RETURN
    }
    return true;
}

bool report::requirePrivate(const Decl *decl, data::Tags::const_iterator i)
{
    if (decl->getAccess() == AS_public && !(i->type & TagInfo::BitPublic)) {
        a.report(decl, check_name, "KS02",
                 i->type & TagInfo::BitPrivate ?
                     "Tag requires private declaration" :
                     "Tag implicitly requires private declaration");
        tagIsHere(i, "KS02");
        return false;                                                 // RETURN
    }
    return true;
}

bool report::requireFunction(const Decl *decl, data::Tags::const_iterator i)
{
    if (auto ftd = llvm::dyn_cast<FunctionTemplateDecl>(decl)) {
        decl = ftd->getTemplatedDecl();
    }
    auto fd = llvm::dyn_cast<FunctionDecl>(decl);
    if (!fd) {
        a.report(decl, check_name, "KS03",
                 "Tag requires function declaration");
        tagIsHere(i, "KS03");
        return false;                                                 // RETURN
    }
    return true;
}

bool report::requireField(const Decl *decl, data::Tags::const_iterator i)
{
    if (!llvm::dyn_cast<FieldDecl>(decl)) {
        a.report(decl, check_name, "KS04",
                 "Tag requires instance data field declaration");
        tagIsHere(i, "KS04");
        return false;                                                 // RETURN
    }
    return true;
}

bool report::requireVar(const Decl *decl, data::Tags::const_iterator i)
{
    if (!llvm::dyn_cast<VarDecl>(decl)) {
        a.report(decl, check_name, "KS05",
                 "Tag requires static data field declaration");
        tagIsHere(i, "KS05");
        return false;                                                 // RETURN
    }
    return true;
}

bool report::requireType(const Decl *decl, data::Tags::const_iterator i)
{
    if (!llvm::dyn_cast<TypeDecl>(decl) &&
        !llvm::dyn_cast<ClassTemplateDecl>(decl)) {
        a.report(decl, check_name, "KS06", "Tag requires type declaration");
        tagIsHere(i, "KS06");
        return false;                                                 // RETURN
    }
    return true;
}

bool report::requireConstMethod(const Decl *decl, data::Tags::const_iterator i)
{
    if (auto ftd = llvm::dyn_cast<FunctionTemplateDecl>(decl)) {
        decl = ftd->getTemplatedDecl();
    }
    auto md = llvm::dyn_cast<CXXMethodDecl>(decl);
    if (!md || !md->isConst()) {
        a.report(decl, check_name, "KS07",
                 "Tag requires const method declaration");
        tagIsHere(i, "KS07");
        return false;                                                 // RETURN
    }
    return true;
}

bool report::requireNotConstMethod(const Decl                 *decl,
                                   data::Tags::const_iterator  i)
{
    if (auto ftd = llvm::dyn_cast<FunctionTemplateDecl>(decl)) {
        decl = ftd->getTemplatedDecl();
    }
    auto md = llvm::dyn_cast<CXXMethodDecl>(decl);
    if (!md || md->isConst()) {
        a.report(decl, check_name, "KS08",
                 "Tag requires non-const method declaration");
        tagIsHere(i, "KS08");
        return false;                                                 // RETURN
    }
    return true;
}

bool report::requireCDtor(const Decl *decl, data::Tags::const_iterator i)
{
    if (auto ftd = llvm::dyn_cast<FunctionTemplateDecl>(decl)) {
        decl = ftd->getTemplatedDecl();
    }
    if (!llvm::dyn_cast<CXXConstructorDecl>(decl) &&
        !llvm::dyn_cast<CXXDestructorDecl>(decl)) {
        a.report(decl, check_name, "KS09",
                 "Tag requires constructor or destructor declaration");
        tagIsHere(i, "KS09");
        return false;                                                 // RETURN
    }
    return true;
}

bool report::requireConstVar(const Decl *decl, data::Tags::const_iterator i)
{
    auto vd = llvm::dyn_cast<VarDecl>(decl);
    if (!vd || !vd->getType().isConstQualified()) {
        a.report(decl, check_name, "KS10",
                 "Tag requires constant data declaration");
        tagIsHere(i, "KS10");
        return false;                                                 // RETURN
    }
    return true;
}

bool report::requireStaticMethod(const Decl                 *decl,
                                 data::Tags::const_iterator  i)
{
    if (auto ftd = llvm::dyn_cast<FunctionTemplateDecl>(decl)) {
        decl = ftd->getTemplatedDecl();
    }
    auto md = llvm::dyn_cast<CXXMethodDecl>(decl);
    if (!md || !md->isStatic()) {
        a.report(decl, check_name, "KS11",
                 "Tag requires static method declaration");
        tagIsHere(i, "KS11");
        return false;                                                 // RETURN
    }
    return true;
}

bool report::requireFreeOperator(const Decl                 *decl,
                                 data::Tags::const_iterator  i)
{
    if (auto ftd = llvm::dyn_cast<FunctionTemplateDecl>(decl)) {
        decl = ftd->getTemplatedDecl();
    }
    auto fd = llvm::dyn_cast<FunctionDecl>(decl);
    if (!fd ||
        llvm::dyn_cast<CXXMethodDecl>(fd) ||
        !fd->isOverloadedOperator()) {
        a.report(decl, check_name, "KS12",
                 "Tag requires free operator declaration");
        tagIsHere(i, "KS12");
        return false;                                                 // RETURN
    }
    return true;
}

bool report::requireFreeFunction(const Decl                 *decl,
                                 data::Tags::const_iterator  i)
{
    if (auto ftd = llvm::dyn_cast<FunctionTemplateDecl>(decl)) {
        decl = ftd->getTemplatedDecl();
    }
    auto fd = llvm::dyn_cast<FunctionDecl>(decl);
    if (!fd ||
        llvm::dyn_cast<CXXMethodDecl>(fd) ||
        fd->isOverloadedOperator()) {
        a.report(decl, check_name, "KS13",
                 "Tag requires free function declaration");
        tagIsHere(i, "KS13");
        return false;                                                 // RETURN
    }
    return true;
}

bool report::requireConversionMethod(const Decl                 *decl,
                                     data::Tags::const_iterator  i)
{
    if (auto ftd = llvm::dyn_cast<FunctionTemplateDecl>(decl)) {
        decl = ftd->getTemplatedDecl();
    }
    auto fd = llvm::dyn_cast<CXXConversionDecl>(decl);
    if (!fd) {
        a.report(decl, check_name, "KS14",
                 "Tag requires conversion operator declaration");
        tagIsHere(i, "KS14");
        return false;                                                 // RETURN
    }
    return true;
}

void report::operator()()
{
    if (a.is_test_driver()) {
        return;                                                       // RETURN
    }

    std::vector<Sort> sorts;
    for (auto& tag : d.d_tags) {
        sorts.emplace_back(Sort::Comment, &tag);
    }
    for (const auto def : d.d_defs) {
        sorts.emplace_back(Sort::RangeStart, def);
        sorts.emplace_back(Sort::RangeEnd, def);
    }
    std::sort(sorts.begin(), sorts.end(), *this);
    std::vector<const TagDecl *> scopes(1, nullptr);
    for (const auto& sort : sorts) {
        switch (sort.type) {
        case Sort::Comment:
            sort.comment->decl = scopes.back();
            break;
        case Sort::RangeStart:
            scopes.emplace_back(sort.scope);
            break;
        case Sort::RangeEnd:
            if (scopes.size() > 1) {
                scopes.pop_back();
            }
            break;
        }
    }
    for (auto decl : d.d_decls) {
        auto i =
            std::lower_bound(d.d_tags.begin(), d.d_tags.end(), decl, *this);
        if (i == d.d_tags.begin() ||
            !m.isWrittenInSameFile(getLoc(decl), getLoc(*--i)) ||
            m.isBeforeInTranslationUnit(getLoc(i->range.getBegin()),
                                        getDCLoc(decl))) {
            if (!decl->isInAnonymousNamespace()) {
                a.report(decl, check_name, "KS00", "Declaration witout tag");
            }
            continue;
        }
        TagInfo::TagTypes base_type = TagInfo::TagTypes(
                         i->type & ~TagInfo::BitPrivate & ~TagInfo::BitPublic);
        if (llvm::dyn_cast<FriendDecl>(decl)) {
            if (base_type != TagInfo::Friends) {
                a.report(decl, check_name, "KS15",
                         "Friend declaration requires FRIENDS tag");
                tagIsHere(i, "KS15");
            }
            continue;
        }
        if (llvm::dyn_cast<CXXConstructorDecl>(decl) ||
            llvm::dyn_cast<CXXDestructorDecl>(decl)) {
            if (base_type != TagInfo::Creators &&
                base_type != TagInfo::NotImplemented) {
                a.report(decl, check_name, "KS09",
                         "Constructor or destructor requires CREATORS tag");
                tagIsHere(i, "KS09");
                continue;
            }
        }
        switch (base_type) {
        case TagInfo::Accessors: {
            if (!requireConstMethod(decl, i)) {
                continue;
            }
        } break;
        case TagInfo::Aspects: {
            if (!requireFunction(decl, i)) {
                continue;
            }
        } break;
        case TagInfo::Creators: {
            if (!requireCDtor(decl, i)) {
                continue;
            }
        } break;
        case TagInfo::ClassMethods: {
            if (!requireStaticMethod(decl, i)) {
                continue;
            }
        } break;
        case TagInfo::FreeOperators: {
            if (!requireFreeOperator(decl, i)) {
                continue;
            }
        } break;
        case TagInfo::Manipulators: {
            if (!requireNotConstMethod(decl, i)) {
                continue;
            }
        } break;
        case TagInfo::NotImplemented: {
            if (!requireFunction(decl, i)) {
                continue;
            }
        } break;
        case TagInfo::Traits: {
            if (!requireConversionMethod(decl, i)) {
                continue;
            }
        } break;
        case TagInfo::ClassData: {
            if (!requireVar(decl, i)) {
                continue;
            }
        } break;
        case TagInfo::Data: {
            if (!requireField(decl, i)) {
                continue;
            }
        } break;
        case TagInfo::Constant: {
            if (!requireConstVar(decl, i)) {
                continue;
            }
        } break;
        case TagInfo::Friends: {
            a.report(decl, check_name, "KS16",
                     "Tag requires friend declaration");
            tagIsHere(i, "KS16");
            continue;
        } break;
        case TagInfo::None: {
        } break;
        case TagInfo::Types: {
            if (!requireType(decl, i)) {
                continue;
            }
        } break;
        case TagInfo::FreeFunctions: {
            if (!requireFreeFunction(decl, i)) {
                continue;
            }
        } break;
        default: {
        } break;
        }
        if (i->type & TagInfo::BitPrivate) {
            requirePrivate(decl, i);
        }
        else if (i->type & TagInfo::BitPublic) {
            requirePublic(decl, i);
        }
        else if (i->type & TagInfo::BitImplicitPrivate) {
            requirePrivate(decl, i);
        }
        else if (i->type & TagInfo::BitImplicitPublic) {
            requirePublic(decl, i);
        }
    }
}

void subscribe(Analyser& analyser, Visitor& visitor, PPObserver& observer)
    // Hook up the callback functions.
{
    analyser.onTranslationUnitDone += report(analyser);
    visitor.onTagDecl += report(analyser);
    visitor.onDecl += report(analyser);
    observer.onComment += report(analyser);
}

}  // close anonymous namespace

// ----------------------------------------------------------------------------

static RegisterCheck c1(check_name, &subscribe);

// ----------------------------------------------------------------------------
// Copyright (C) 2016 Bloomberg Finance L.P.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
// ----------------------------- END-OF-FILE ----------------------------------
