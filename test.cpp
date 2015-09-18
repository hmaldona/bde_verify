#include <vector>
#include "test.h"

namespace {

char const * const LOG_CATEGORY = "AIMET.MANAGER";

void foo()
{
    int a =4 ;
    int b = 2;
    // call something
    if ( a == b ) {
            printf("I love clang\n");
            if (a < b){
                for(int i = 0 ;i<12;i++){
                    printf("aasdasad\n");
                }
            }
        }else if (a > b){
            printf("a is larger");
        }
        while(a++ < 10){
            printf("count from 3 to 10 \n");
        }
        a  = 3;
        b =  4;
        if ( a == b ){
            printf("I love clang\n");
            if (a < b){
                for(int i = 0 ;i<12;i++){
                    printf("aasdasad\n");
                }
            }
        }else if (a > b){
            printf("a is larger");
        }
        if ( a == b ){
            printf("I love clang\n");
            if (a < b){
                for(int i = 0 ;i<12;i++){
                    printf("aasdasad\n");
                }
            }
        }else if (a > b){
            printf("a is larger");
        }
        
}

}
    
    // Template functions
    template <class type> type add(type a, type b)
    {

        if ( a == b ){
            printf("I love clang\n");
            if (a < b){
                for(int i = 0 ;i<12;i++){
                    printf("aasdasad\n");
                }
            }
        }else if (a > b){
            printf("a is larger");
        }
        while(a++ < 10){
            printf("count from 3 to 10 \n");
        }
        a  = 3;
        b =  4;
        if ( a == b ){
            printf("I love clang\n");
            if (a < b){
                for(int i = 0 ;i<12;i++){
                    printf("aasdasad\n");
                }
            }
        }else if (a > b){
            printf("a is larger");
        }
        if ( a == b ){
            printf("I love clang\n");
            if (a < b){
                for(int i = 0 ;i<12;i++){
                    printf("aasdasad\n");
                }
            }
        }else if (a > b){
            printf("a is larger");
        }
        return a + b;
    }
    //inline functions
    inline int Max(int x, int y)
    {
         int a  = 3;
        int b =  4;
        if ( a == b ){
            printf("I love clang\n");
            if (a < b){
                for(int i = 0 ;i<12;i++){
                    printf("aasdasad\n");
                }
            }
        }else if (a > b){
            printf("a is larger");
        }
        while(a++ < 10){
            printf("count from 3 to 10 \n");
        }
        a  = 3;
        b =  4;
        if ( a == b ){
            printf("I love clang\n");
            if (a < b){
                for(int i = 0 ;i<12;i++){
                    printf("aasdasad\n");
                }
            }
        }else if (a > b){
            printf("a is larger");
        }
        if ( a == b ){
            printf("I love clang\n");
            if (a < b){
                for(int i = 0 ;i<12;i++){
                    printf("aasdasad\n");
                }
            }
        }else if (a > b){
            printf("a is larger");
        }
       return (x > y)? x : y;
    }

    // 14
    test::test(){
        int a  = 3;
        int b =  4;
        if ( a == b ){
            printf("I love clang\n");
            if (a < b){
                for(int i = 0 ;i<12;i++){
                    printf("aasdasad\n");
                }
            }
        }else if (a > b){
            printf("a is larger");
        }
        while(a++ < 10){
            printf("count from 3 to 10 \n");
        }
        a  = 3;
        b =  4;
        if ( a == b ){
            printf("I love clang\n");
            if (a < b){
                for(int i = 0 ;i<12;i++){
                    printf("aasdasad\n");
                }
            }
        }else if (a > b){
            printf("a is larger");
        }
        if ( a == b ){
            printf("I love clang\n");
            if (a < b){
                for(int i = 0 ;i<12;i++){
                    printf("aasdasad\n");
                }
            }
        }else if (a > b){
            printf("a is larger");
        }
    }
    // 11
    test::~test(){
        int a  = 3;
        int b =  4;
        if (a > b){
            printf("a is larger");
        }
        while(a++ < 10){
            printf("count from 3 to 10 \n");
        }
        a  = 3;
        b =  4;
        if ( a == b ){
            printf("I love clang\n");
            if (a < b){
                for(int i = 0 ;i<12;i++){
                    printf("aasdasad\n");
                }
            }
        }else if (a > b){
            printf("a is larger");
        }
        if ( a == b ){
            printf("I love clang\n");
            if (a < b){
                for(int i = 0 ;i<12;i++){
                    printf("aasdasad\n");
                }
            }
        }else if (a > b){
            printf("a is larger");
        }
    }

    //12
    void test::get(){
        int a  = 3;
        int b =  4;
        if ( a == b ){
            printf("asdasd\n");
        }else if (a > b){
            printf("a is larger");
        }
        while(a++ < 10){
            printf("count from 3 to 10 \n");
        }
        a  = 3;
        b =  4;
        if ( a == b ){
            printf("I love clang\n");
            if (a < b){
                for(int i = 0 ;i<12;i++){
                    printf("aasdasad\n");
                }
            }
        }else if (a > b){
            printf("a is larger");
        }
        if ( a == b ){
            printf("I love clang\n");
            if (a < b){
                for(int i = 0 ;i<12;i++){
                    printf("aasdasad\n");
                }
            }
        }else if (a > b){
            printf("a is larger");
        }

    }

    //10 -11
    void test::test1(int a , int f){
        if (a>3){
            printf("a is larger than 3\n");
        }

        switch(f){
            case 3:
                printf("case3\n");
                if (a < 2){
                    for (int i = 0; i < 12; ++i)
                    {
                        /* code */
                        for (int iii = 0; i < 12; ++i)
                        {
                            /* code */
                            for (int ii = 0; i < 12; ++i)
                            {
                            /* code */
                                switch(i){
                                    case 1:

                                    break;
                                    case 2:

                                    break;

                                    default:
                                    break;
                                }
                            }

                        }

                    }

                }
                try{

                }catch(int e){
                    printf("asdasd\n");
                }

            break;

            // default:
            //     printf("default case \n");
            // break;
        }
        
    }

    
class SecondClass{

    //14
    void get(){
        int a  = 3;
        int b =  4;
        if ( a == b ){
            printf("I love clang\n");
            if (a < b){
                for(int i = 0 ;i<12;i++){
                    printf("aasdasad\n");
                }
            }
        }else if (a > b){
            printf("a is larger");
        }
        while(a++ < 10){
            printf("count from 3 to 10 \n");
        }
        a  = 3;
        b =  4;
        if ( a == b ){
            printf("I love clang\n");
            if (a < b){
                for(int i = 0 ;i<12;i++){
                    printf("aasdasad\n");
                }
            }
        }else if (a > b){
            printf("a is larger");
        }
        if ( a == b ){
            printf("I love clang\n");
            if (a < b){
                for(int i = 0 ;i<12;i++){
                    printf("aasdasad\n");
                }
            }
        }else if (a > b){
            printf("a is larger");
        }

    }

    //6
    int test1(int a , int f){
        if (a>3){
            printf("a is larger than 3\n");
        }

        switch(f){
            case 3:
                printf("case3\n");
                if (a < 2){
                    for (int i = 0; i < 12; ++i)
                    {
                        /* code */
                    }
                }

            break;

            default:
                printf("default case \n");
            break;
        }

        return 0;
    }


    // 5
    void test2(){
        bool bln = false;
        if (bln == true){
            printf("A thread must be playing here\n");
        }

        for(int i = 0 ; i < 10 ;i++){
            printf("Count from 1 to 10\n");
            if (i<3){
                printf("asdasd\n");
            }
            if (i+2<4)
            {
                /* code */
            }
        }

    }
};


class ThirdClass{

    //14
    void get(){
        int a  = 3;
        int b =  4;
        if ( a == b ){
            printf("I love clang\n");
            if (a < b){
                printf("asd\n");
            }
        }else if (a > b){
            printf("a is larger");
        }
        while(a++ < 10){
            printf("count from 3 to 10 \n");
        }
        a  = 3;
        b =  4;
        if ( a == b ){
            printf("I love clang\n");
            if (a < b){
                for(int i = 0 ;i<12;i++){
                    printf("aasdasad\n");
                }
            }
        }else if (a > b){
            printf("a is larger");
        }
        if ( a == b ){
            printf("I love clang\n");
            if (a < b){
                for(int i = 0 ;i<12;i++){
                    printf("aasdasad\n");
                }
            }
        }else if (a > b){
            printf("a is larger");
        }

    }

    //6
    int test1(int a , int f){
        if (a>3){
            printf("a is larger than 3\n");
        }

        switch(f){
            case 3:
                printf("case3\n");
                if (a < 2){
                    for (int i = 0; i < 12; ++i)
                    {
                        /* code */
                    }
                }

            break;

            default:
                printf("default case \n");
            break;
        }
        bool ff = true;
        bool k = false;

        bool s = ff && k;

        if ((s && ff && k && test3()) || (ff && k) ){
            printf("asdasd\n");
        }

        return 0;
    }


    // 5
    void test2(){
        bool bln = false;
        if (bln == true){
            printf("A thread must be playing here\n");
        }

        for(int i = 0 ; i < 10 ;i++){
            printf("Count from 1 to 10\n");
            if (i<3){
                printf("asdasd\n");
            }
            if ( (i + 2 < 4 && i < 23 ) || i>23)
            {
                /* code */
            }
        }

    }

    bool test3(){
        return true;
    }
};