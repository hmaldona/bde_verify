// csautil_bregistry.h

#ifndef INCLUDED_CSAUTIL_BREGISTRY
#define INCLUDED_CSAUTIL_BREGISTRY

#include <string>
#include <map>
#include <vector>

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
    void setRequesterUuid(unsigned int uuid);

    void setNumDaysSinceExpiry(unsigned int numOfDays);

    bool getEntryInformation(BREGEntryInfo & entryInfo,
                                    int             entryId);

    bool getEntryInformation(BREGEntryInfo     & entryInfo,
                                    std::string const & entryAccessor);

    bool getEntryValueInformation(BREGEntryInfo     & entryInfo,
                                  std::string const & entryAccessor, 
                                  std::string overrideType,
                                  std::string overrideValue);

    bool getEntryByValue(BREGEntryInfo & entryInfo, 
                         std::string const & entryAccessor, 
                         int version);
    
    BRegistry();
    BRegistry(unsigned int s_uuid, unsigned int s_daysSinceExpiry);


private:
    // class data members
    unsigned int s_uuid;
    unsigned int s_daysSinceExpiry;
    std::map<std::string, int> my_vect;
    std::map<std::string, BREGEntryInfo> s_entries;
};

} // namespace csautil

#endif // INCLUDED_CSAUTIL_BREGISTRY
