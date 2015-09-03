// csautil_bregistry.cpp

#include <csautil_bregistry.h>

#include <csautil_bregsvcclient.h>

#include <s_breguisvc_messages.h>

#include <bdet_date.h>
#include <bdetu_systemtime.h>
#include <bdepu_typesparser.h>
#include <bael_log.h>

#include <string>
#include <iostream>
#include <map>
#include <utility>
#include <algorithm>


namespace csautil {

using namespace BloombergLP;

namespace {

char const * LOG_CATEGORY = "CSAUTIL.BREGISTRY";

} // namespace <anonymous>


// class data members

BRegistry::BRegistry()
{
}


BRegistry::BRegistry(unsigned int s_uuid, unsigned int s_daysSinceExpiry)
{
    s_uuid = s_uuid;
    s_daysSinceExpiry = s_daysSinceExpiry;
}


//unsigned int BRegistry::s_uuid = 7034648;
//
//unsigned int BRegistry::s_daysSinceExpiry = 60;
//std::map<std::string, BREGEntryInfo> BRegistry::s_entries;

// class members

void BRegistry::setRequesterUuid(unsigned int uuid)
{
    s_uuid = uuid;
}


void BRegistry::setNumDaysSinceExpiry(unsigned int numOfDays)
{
    s_daysSinceExpiry = numOfDays;
}


bool BRegistry::getEntryInformation(BREGEntryInfo & entryInfo,
                                    int             entryId)
{
    // s_breguisvc::Request request;

    // request.makeSearchById(entryId);

    // s_breguisvc::Response response;

    // if (BregSvcClient::instance().executeRequest(response, request)) {
    //     std::cerr << "Failed to execute request:" << request << std::endl;
    // }

    // std::cout << response << std::endl;

    // entryInfo.d_name = response.nodeTree()[0].name();

    return true;
}


bool BRegistry::getEntryByValue(BREGEntryInfo & entryInfo, 
                                std::string const & entryAccessor, 
                                int version) 
{
    BAEL_LOG_SET_CATEGORY(LOG_CATEGORY);

    std::map<std::string, BREGEntryInfo>::iterator iterFind =
        s_entries.find(entryAccessor);

    if (iterFind == s_entries.end()) {
        BAEL_LOG_ERROR << "Trying to retrieve value for unknown BREG entry: "
            << entryAccessor
            << BAEL_LOG_END;

        return false;                                                 // RETURN
    }

    BREGEntryInfo & entry = iterFind->second;

    //if (entry.d_isCached) {
    //    entryInfo = entry;
//
//        return true;                                                  // RETURN
//    }

    s_breguisvc::Request request;

    s_breguisvc::ValueRequest value;
    value.entryId() = entryInfo.d_id;
    value.version() = version;

    request.makeGetEntryValue(value);

    s_breguisvc::Response response;

    int rcode = BregSvcClient::instance().executeRequest(response,
                                                         request,
                                                         s_uuid,
                                                         s_breguisvc::Response::SELECTION_ID_ENTRY_VALUE);
    

    if (!rcode) {
        if (response.entryValue().entryId() == entryInfo.d_id) {
            
            entryInfo.d_prodValue = response.entryValue().entryData().value().boolValue();
            std::cout << "file: " << entryInfo.d_prodValue << std::endl;

            return true;                                          // RETURN
        }
        else {
            BAEL_LOG_ERROR << "Failed to find BREG entry for '" << entryAccessor 
                           << "': Found more than one entry"
                           << BAEL_LOG_END;

            return false;                                             // RETURN
        }
    }

    return false;


}



bool BRegistry::getEntryInformation(BREGEntryInfo     & entryInfo,
                                    std::string const & entryAccessor)
{
    BAEL_LOG_SET_CATEGORY(LOG_CATEGORY);

    std::map<std::string, BREGEntryInfo>::iterator iterFind = s_entries.find(entryAccessor);

    if (iterFind != s_entries.end()) {
        entryInfo = iterFind->second;

        return true;                                                  // RETURN
    }

    s_breguisvc::Request request;

    request.makeSearchByAccessor(entryAccessor);

    s_breguisvc::Response response;

    int rcode = BregSvcClient::instance().executeRequest(response,
                                                         request,
                                                         s_uuid,
                                                         s_breguisvc::Response::SELECTION_ID_NODE_TREE);

    if (!rcode) {
        if (response.nodeTree().size() == 1) {
            s_breguisvc::Node const & node = response.nodeTree()[0];
            
            entryInfo.d_id = node.id();
            entryInfo.d_accessor = entryAccessor;
            entryInfo.d_hasExpired = node.hasExpired();
            entryInfo.d_isObsolete =  node.obsoleteTraceEnabled();
            
            request.makeGetRegistry(entryInfo.d_id);
            response.reset();

            rcode = BregSvcClient::instance().executeRequest(response,
                                                             request,
                                                             s_uuid,
                                                             s_breguisvc::Response::SELECTION_ID_REGISTRY_INFO);
            
            
            if (!rcode) {
                s_breguisvc::NodeInfo const & nodeInfo = response.registryInfo().node();

                entryInfo.d_drqsGroup = nodeInfo.contacts().drqsGroup();

                s_entries[entryInfo.d_accessor] = entryInfo;

                return true;                                          // RETURN
            }
        }
        else {
            BAEL_LOG_ERROR << "Failed to find BREG entry for '" << entryAccessor
                           << "': Found more than one entry"
                           << BAEL_LOG_END;

            return false;                                             // RETURN
        }
    }

    return false;
}


bool BRegistry::getEntryValueInformation(BREGEntryInfo     & entryInfo,
                                         std::string const & entryAccessor)
{
    BAEL_LOG_SET_CATEGORY(LOG_CATEGORY);

    static bdet_Date today = bdetu_SystemTime::nowAsDatetimeUtc().date();

    std::map<std::string, BREGEntryInfo>::iterator iterFind =
        s_entries.find(entryAccessor);

    if (iterFind == s_entries.end()) {
        BAEL_LOG_ERROR << "Trying to retrieve value for unknown BREG entry: "
                       << entryAccessor
                       << BAEL_LOG_END;

        return false;                                                 // RETURN
    }

    BREGEntryInfo & entry = iterFind->second;

    if (entry.d_isCached) {
        entryInfo = entry;

        return true;                                                  // RETURN
    }

    s_breguisvc::Request request;

    request.makeGetNodeDetails(entry.d_id);

    s_breguisvc::Response response;


    int rcode = BregSvcClient::instance().executeRequest(response,
                                                         request,
                                                         s_uuid,
                                                         s_breguisvc::Response::SELECTION_ID_NODE_DETAILS);
    
    

    if (!rcode) {
        s_breguisvc::NodeDetails const & nodeDetails = response.nodeDetails();

        //
        // processing expiry date
        //
        bdet_Date expiryDate(today);

        entry.d_expiryDate = nodeDetails.expiryTsp();

        std::replace(entry.d_expiryDate.begin(),
                     entry.d_expiryDate.end(),
                     '-', '/');

        char const * endPosition = NULL;

        if (bdepu_TypesParser::parseDate(&endPosition,
                                         &expiryDate,
                                         entry.d_expiryDate.c_str())) {
            BAEL_LOG_ERROR << "Invalid expiry date: " << entry.d_expiryDate
                           << BAEL_LOG_END;
        }

        entry.d_prodValue = nodeDetails.inProd();

        if (entry.d_hasExpired
            && entry.d_hasExpired == nodeDetails.hasExpired()
            && expiryDate < (today - s_daysSinceExpiry)
            && nodeDetails.prodOverrides().empty()) {
            entry.d_isExpiredCall = true;
            entry.d_expiredValue = entry.d_prodValue;
        }

        entry.d_isCached = true;

        entryInfo = entry;

        return true;
    }

    return false;
}


} // namespace csautil
