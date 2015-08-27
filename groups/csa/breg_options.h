#ifndef BREG_OPTIONS
#define BREG_OPTIONS

struct BregOptions {
    bool silent;
    bool ignoreSideEffects;
    bool silentSideEffectsWarnings;
    bool conditionExtraction;

    BregOptions() : silent(false), 
                    ignoreSideEffects(false), 
                    silentSideEffectsWarnings(false), 
                    conditionExtraction(false) 
    {}

};


#endif
