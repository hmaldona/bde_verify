#ifndef BREG_OPTIONS
#define BREG_OPTIONS

#include <string>

enum BregValue {
        UUID,
        pricingNumber,
        machineGroup,
        csv
};


struct BregOptions {
    
    bool silent;
    bool ignoreSideEffects;
    bool functionRemovalWarnings;
    bool conditionExtraction;

    BregValue bregValue;
    std::string dateString;
    unsigned int UUID;
    unsigned int pricingNumber;
    std::string machineGroup;
    
    
    BregOptions() : silent(false), 
                    ignoreSideEffects(false), 
                    functionRemovalWarnings(false), 
                    conditionExtraction(false),
                    bregValue(csv),
                    dateString(),
                    UUID(0),
                    pricingNumber(0),
                    machineGroup(0)
    {}

};


#endif
