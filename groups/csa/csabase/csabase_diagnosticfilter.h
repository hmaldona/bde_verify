// csabase_diagnosticfilter.h                                         -*-C++-*-

#ifndef INCLUDED_CSABASE_DIAGNOSTICFILTER_H
#define INCLUDED_CSABASE_DIAGNOSTICFILTER_H

#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <memory>
#include <set>
#include <string>

// ----------------------------------------------------------------------------

namespace clang { class DiagnosticOptions; }

namespace csabase { class Analyser; }
namespace csabase
{
class DiagnosticFilter : public clang::TextDiagnosticPrinter
{
  public:
    DiagnosticFilter(Analyser const&           analyser,
                     std::string               diagnose,
                     clang::DiagnosticOptions& options);

    void HandleDiagnostic(clang::DiagnosticsEngine::Level level,
                          clang::Diagnostic const&        info) override;

    static void fail_on(unsigned id);
    static bool is_fail(unsigned id);

  private:
    Analyser const*                          d_analyser;
    std::string                              d_diagnose;
    bool                                     d_prev_handle;
    static std::set<unsigned>                s_fail_ids;
};
}

inline
void csabase::DiagnosticFilter::fail_on(unsigned id)
{
    s_fail_ids.insert(id);
}

inline
bool csabase::DiagnosticFilter::is_fail(unsigned id)
{
    return s_fail_ids.count(id);
}

#endif

// ----------------------------------------------------------------------------
// Copyright (C) 2014 Bloomberg Finance L.P.
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
