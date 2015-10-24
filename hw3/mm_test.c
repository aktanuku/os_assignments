/* A simple test harness for memory alloction. */

#include "mm_alloc.h"
#include <stdio.h>
int main(int argc, char **argv)
{
    char *data;

    data = (char*) mm_malloc(40);
    data[0] = 1;
    
    char *data1;

    data1 = (char*) mm_malloc(40);
    char* data2;
    data2 = (char*) mm_malloc(40);
   
      char * data3;

    data3 = (char*) mm_malloc(40);

  
    char * data4;

  //  data4 = (char*) mm_malloc(40);
  
//    print_the_fuckin_heap(4+5+6+120);  
//    print_blockmeta_by_address(data);
//    print_blockmeta_by_address(data1);

//    print_blockmeta_by_address(data2);
//    print_blockmeta_by_address(data3);
    print_all_meta();

    int i = 0;
    for(i=0; i< 40; i++)
	data3[i] = 'a';

//    mm_free(data);   
 //   mm_free(data1);
//    mm_free(0x2123);
//    printf("survived first free\n");
//    print_all_meta();
//    mm_free(data3);
//    printf("survived second free\n");
//    mm_free(data2);
//    printf("survived third free\n");
//    mm_free(data2);
    void* ptr = mm_realloc(data3, 80);
    print_all_meta();

    printf("register after array: %ld\n", (size_t)data3[40]);
    printf("malloc sanity test successful!\n");
    return 0;
}
