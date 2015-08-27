// csautil_bregsvcclient.cpp

#include <csautil_bregsvcclient.h>

#include <aetutils_servicetcpclient.h>

#include <s_breguisvc_messages.h>

#include <bsls_assert.h>


namespace csautil {

using namespace BloombergLP;

namespace {

char const * const LOG_CATEGORY = "CSAUTIL.BREGSVCCLIENT";

} // namespace <anonymous>


                              // -------------------
                              // class BregSvcClient
                              // -------------------

// class members

bool
BregSvcClient::isExpectedResponse(s_breguisvc::Response const & response,
                                  int                   const   expectedResponseId,
                                  std::string                 * errorMessage)
{
    // Preconditions
    BSLS_ASSERT(expectedResponseId > s_breguisvc::Response::SELECTION_ID_UNDEFINED);

    if (response.selectionId() == expectedResponseId) {
        return true;                                                  // RETURN
    }

    std::string localErrorMessage;

    switch (response.selectionId()) {
        case s_breguisvc::Response::SELECTION_ID_ERROR_RESPONSE: {
            s_breguisvc::ErrorInfoResponse const & error = response.errorResponse();

            std::ostringstream oss;
            oss << error.message() << " (code: " << error.code() << ")";

            localErrorMessage = oss.str();
            break;
        }

        case s_breguisvc::Response::SELECTION_ID_UNDEFINED:
            localErrorMessage = "Undefined response";
            break;

        default: {
            std::ostringstream oss;
            oss << "Unexpected response: " << response;

            localErrorMessage = oss.str();
            break;
        }
    }

    if (errorMessage == NULL) {
        BAEL_LOG_SET_CATEGORY(LOG_CATEGORY);

        BAEL_LOG_ERROR << "Response failed: " << localErrorMessage
                       << BAEL_LOG_END;
    }
    else {
        errorMessage->swap(localErrorMessage);
    }

    return false;
}


// manipulators

int BregSvcClient::executeRequest(
    s_breguisvc::Response      & response,
    s_breguisvc::Request const & request,
    int                  const   uuid,
    int                  const   expectedResponseId)
{
    // Preconditions
    BSLS_ASSERT(uuid > 0);

    if (sendRequest(response, request, 0, uuid)) {
        return 1;                                                     // RETURN
    }

    if (expectedResponseId == s_breguisvc::Response::SELECTION_ID_UNDEFINED) {
        return 0;                                                     // RETURN
    }

    return isExpectedResponse(response, expectedResponseId) ? 0 : 1;
}


} // namespace csautil
