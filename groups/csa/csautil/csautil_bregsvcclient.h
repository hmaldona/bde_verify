// csautil_bregsvcclient.h

#ifndef INCLUDED_CSAUTIL_BREGSVCCLIENT
#define INCLUDED_CSAUTIL_BREGSVCCLIENT

#include <aetutils_servicetcpclient.h>

#include <s_breguisvc_messages.h>


namespace BloombergLP {

// forward declarations
namespace s_breguisvc { class Request; }
namespace s_breguisvc { class Response; }

} // namespace BloombergLP


namespace csautil {


                              // ===================
                              // class BregSvcClient
                              // ===================

class BregSvcClient : public BloombergLP::aetutils::ServiceTcpClient<BregSvcClient>
{
public:
    // friends
    friend class BloombergLP::aetutils::ServiceTcpClient<BregSvcClient>;

    // class members
    static bool isExpectedResponse(
        BloombergLP::s_breguisvc::Response const & response,
        int                                const   expectedResponseId,
        std::string                              * errorMessage = NULL);

    // manipulators
    int executeRequest(
        BloombergLP::s_breguisvc::Response      & response,
        BloombergLP::s_breguisvc::Request const & request,
        int                               const   uuid,
        int                               const   expectedResponseId = -1);

private:
    // private creators
    BregSvcClient();

    BregSvcClient(BregSvcClient const & rhs);

    BregSvcClient & operator=(BregSvcClient const & rhs);

    ~BregSvcClient();

};


// =============================================================================
//                            INLINE DEFINITIONS
// =============================================================================

                              // -------------------
                              // class BregSvcClient
                              // -------------------

// private creators

inline
BregSvcClient::BregSvcClient()
    : BloombergLP::aetutils::ServiceTcpClient<BregSvcClient>("breguisvc",
                                                             "urn:x-bloomberg-com:BAS.services.breguisvc",
                                                             64730, 1, 12)
{
    // do nothing
}

inline
BregSvcClient::~BregSvcClient()
{
    // do nothing
}

} // namespace csautil

#endif // INCLUDED_CSAUTIL_BREGSVCCLIENT
