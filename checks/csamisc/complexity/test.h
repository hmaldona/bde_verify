class test{
public:
	test();
	~test();
	void get();
	void test1(int a, int f);
	inline int max1(int x, int y);
    void foo()
    {   
        int a =4 ;
        int b = 2;
        // call something
        
        printf("I love clang\n");
        if (a < b){

            for(int i = 0 ;i<12;i++){
                printf("aasdasad\n");
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
	


};
