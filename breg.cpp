#include <string>

#include "/bbsrc/thirdparty/bbit/include/bbit/200911/bbit_send_to_omx.h"                
#include "/bbsrc/thirdparty/bbit/include/bbit/201409/bbit_202535_ox_order_bunched_fields_rw.h"

#define bregT__value() bregdb_eval_bbit_bool_rv(&bbit_send_to_omx, false)
#define bregF__value() bregdb_eval_bbit_bool_rv(&bbit_202535_ox_order_bunched_fields_rw, false)

//int bregT = 1;
//int bregF = 0;
//
//bool bregdb_eval_bbit_bool_rv(int i) {
//    return i;
//}


class str {
public:
    int x;
    int * p;

    void inc() {x++;}
    str() {x = 0;}
};


bool q() {
    return true;
}

bool g() {
    return false;
}

void call() {}

void newFunction() {
    int i = 0;
}

//test early return;
void oldFunction() {
    if ( bregT__value() || q() ) {
        newFunction();
        return;
    }
    //else 
    int i = 0;
}

int global;

bool sideEffectFunction() {
    global++;
    return true;
}

bool referenceFunction() {
    const int& copy = global;
    

    return true;
}

bool pointerFunction() {
    int const * copy = &(*(int*)(&global));
    int * asd = &global;

    return true;
}

str s = str();
bool methodFunction() {
    s.x++;
    s.inc();
    return true;
}

bool twoCallFunction() {
    int x = 0;
    x++;
    referenceFunction();
    return false;
}

bool longFunction() {
    int i = 0;
    ;;;;;;;;;;;;;;;;
    return false;
}

bool argFunction(int & x) {
    x++;
    return true;
}

bool structFunction(str s) {
    (*s.p)++;

    return true;
}

bool refFunction() {
    //int * x;
    //x = &global;
    str local = str();
    local.p = &global;

    return true;
}



void f() 
{
    //comment
    if ( bregF__value() ) {
        // comment 
        if ( bregT__value() ) {
            //comment
        }
    }


    //hey
    if (bbit_send_to_omx__value() && q()) {
        int i = 0;
    }

        
    if ( bregT__value() ) {int i = 0;}
    

    // test for leaving braces behind
    if ( bregT__value() ) {
        int braces_remain;
    }
     
    //test for removing braces;
    int braces_remove = 0;
    if ( bregT__value() ) {
        braces_remove = 1;
    }

    //test for removing if
    if ( bregF__value() ) {
        int remove_if;
    }
    
    //test for leaving else behind 
    if ( bregF__value() ) {
        int remove_if;
    }
    else {
        int leave_this;
    }

    //test with chained elses, removing if
    int a = 0;
    if ( bregF__value() ) {
        int remove_if;
    }
    else if ( 1 == a ) {
        int this_becomes_if;
    }
    else {
        int leave_this;
    }

    //test leaving braces behind, more complex expression
    if ( !bregF__value() ) {
        int leave_this;
    }
   
    //no breg call, no change
    if ( !(true == false) || q() ) {
        int var = 1; 
    }

    //chained elses, breg removes if
    std::string asd;
    int v1 = 0;

    if ( ( asd == "something" || asd == "something else" ) && bregF__value() ) {
        v1 = 1;
    }
    else if ( asd == "this" ) {
        v1 = 2;
    }
    else {
        v1 = 3;
    }

    //chained elses, breg call removed 
    if ( ( asd == "something" || asd == "something else" ) && bregT__value() ) {
        v1 = 1;
    }
    else if ( asd == "this" ) {
        v1 = 2;
    }
    else {
        v1 = 3;
    }

    //remove if condition
    bool c1;
    bool c2;
    bool c3;

    if ( ((c1 == true && c2 == true) || c3) || bregT__value() ) {
        int asd;
    }

    //remove breg call
    if ( ((c1 == true && c2 == true) || c3) || bregF__value() ) {
        int asd;
    }

    //remove breg call deep in condition
    if ( (  ( (c1 == true && c2 == true) || c3) || ( bregF__value() == true ) ) || (v1 == 'c') ) {
        int asd;
    }

    //non constant condition
    if ( ( true == g() && bregF__value() ) || g() == false ) {
        
    }

    //non constant condition
    if ( !!!(bregF__value() == true) && q() ) {

    }

    //no change no breg call
    if (true) {}

    //no change no breg call
    if (true && q()) {

    }

    bool i = bbit_send_to_omx__value();
    
    //var with breg value
    if (true) {
        int one = 0;
    }

    //var with breg value, value change
    i = bregF__value();

    if (i) {
        int two = 0;
    }
    
    //value change
    i = bregT__value();

    if ( i ) {

    }

    //var with breg value
    if (i) {
        int asd = 0;
    }
    else {
        int asd2 = 0;
    }

    //var with breg value
    if (i == false) {
        int a = 0;
    }

    //function that isn't removed
    if ( bregF__value() && longFunction() ) {
        ;
    }

    //function has side effects
    if ( bregF__value() && sideEffectFunction() ) {
        ;
    }

    //function has side effects
    if ( bregF__value() && methodFunction() ) {
        ;
    }

    //function with pointer side effects
    if ( bregF__value() && pointerFunction() ) {
        ;
    }

    //function with reference side effects
    if ( bregF__value() && referenceFunction() ) {
        ;
    }

    //function with reference side effects
    if ( bregF__value() && twoCallFunction() ) {
        ;
    }
    
    //function with reference side effects
    int x = 0;
    if ( bregF__value() && argFunction(x) ) {
        ;
    }

    //function with reference side effects
    str s;
    s.p = &x;
    if ( bregF__value() && structFunction(s) ) {
        ;
    }

    if ( bregF__value() && refFunction() ) {
        ;
    }
    

    if ( ( ( (c1 == true && c2 == true) || c3) || ( bregF__value() == true ) ) || false ) {
        int asd;
    }

     //function with reference side effects
    if ( refFunction() || bregT__value() ) {
        int var = 0;
    }


    if ( (bregF__value() && q()) && (bregF__value() && q()) ) {
        //comment
    }
    else {
        
    }
   
    //start
    if ( bregT__value() && bregF__value() ) {
        //cooment
    }
    //end
    //

    if ( bregT__value() ) {
        if ( true ) {
            int i = 0;
        }
    }

}


class asdasd {
    public:
    void f()
    {
        {
            //comment
            int i = 0;
        }
    }
    
    bool g() {
        return true;
    }
};


void someFunc() {
    int rectyp = 0;
    std::string m_fietTradeType;
    asdasd asd;


    if ( ( ( !bregT__value() && rectyp == 1) && m_fietTradeType == "S" ) && !asd.g() )
    {
        rectyp = 10;
    }

}



