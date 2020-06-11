// Copyright Contributors to the OpenVDB Project
// SPDX-License-Identifier: MPL-2.0

/// @file ast/AST.cc

#include "AST.h"

#include <openvdb_ax/Exceptions.h>
// @note We need to include this to get access to axlloc. Should look to
//   re-work this so we don't have to.
#include <openvdb_ax/grammar/axparser.h>

#include <tbb/mutex.h>
#include <sstream>

namespace {
// Declare this at file scope to ensure thread-safe initialization.
tbb::mutex sInitMutex;
std::string sLastParsingError;
}

using YY_BUFFER_STATE = struct yy_buffer_state*;
extern int axparse(openvdb::ax::ast::Tree**);
extern YY_BUFFER_STATE ax_scan_string(const char * str);
extern void ax_delete_buffer(YY_BUFFER_STATE buffer);

/// On a parsing error we store the error string in a global
/// variable so that we can access it later. Note that this does
/// not get called for invalid character lexical errors - we
/// immediate throw in the lexer in this case
extern void axerror (openvdb::ax::ast::Tree**, char const *s) {
    std::ostringstream os;
    os << axlloc.first_line << ":" << axlloc.first_column
       << " '" << s << "'";
    sLastParsingError = os.str();
}

openvdb::ax::ast::Tree::Ptr
openvdb::ax::ast::parse(const char* code)
{
    tbb::mutex::scoped_lock lock(sInitMutex);

    // reset all locations
    axlloc.first_line = axlloc.last_line = 1;
    axlloc.first_column = axlloc.last_column = 0;

    YY_BUFFER_STATE buffer = ax_scan_string(code);

    openvdb::ax::ast::Tree* tree(nullptr);
    const int result = axparse(&tree);

    openvdb::ax::ast::Tree::Ptr ptr(tree);

    ax_delete_buffer(buffer);

    if (result) {
        OPENVDB_THROW(openvdb::LLVMSyntaxError, sLastParsingError)
    }

    assert(ptr);
    return ptr;
}

