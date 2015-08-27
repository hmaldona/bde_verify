// csautil_bregistry.h

#ifndef INCLUDED_CSAUTIL_BREGISTRY
#define INCLUDED_CSAUTIL_BREGISTRY

#include <string>
#include <map>

namespace csautil {

struct BREGEntryInfo
{
public:
    // creators
    BREGEntryInfo();

    ~BREGEntryInfo();

    // data members
    int          d_id;
    std::string  d_accessor;
    unsigned int d_drqsGroup;
    bool         d_hasExpired;
    std::string  d_expiryDate;
    bool         d_isObsolete;
    bool         d_prodValue;

    bool         d_isExpiredCall;
    bool         d_expiredValue;

    bool         d_isCached;

private:

};

inline
BREGEntryInfo::BREGEntryInfo()
    : d_id(0)
    , d_accessor()
    , d_drqsGroup(0)
    , d_hasExpired(false)
    , d_expiryDate("Unknown")
    , d_isObsolete(false)
    , d_prodValue(false)
    , d_isExpiredCall(false)
    , d_expiredValue(false)
    , d_isCached(false)
{
    // do nothing
}

inline
BREGEntryInfo::~BREGEntryInfo()
{
    // do nothing
}


struct BRegistry
{
public:
    // class members
    static void setRequesterUuid(unsigned int uuid);

    static void setNumDaysSinceExpiry(unsigned int numOfDays);

    static bool getEntryInformation(BREGEntryInfo & entryInfo,
                                    int             entryId);

    static bool getEntryInformation(BREGEntryInfo     & entryInfo,
                                    std::string const & entryAccessor);

    static bool getEntryValueInformation(BREGEntryInfo     & entryInfo,
                                         std::string const & entryAccessor);

private:
    // class data members
    static unsigned int s_uuid;
    static unsigned int s_daysSinceExpiry;

    static std::map<std::string, BREGEntryInfo> s_entries;

};

} // namespace csautil

#endif // INCLUDED_CSAUTIL_BREGISTRY
