#include <stdio.h>
#include "sfmm.h"


int main(int argc, char const *argv[]) {

    sf_mem_init();

    //double* ptr = sf_malloc(4096*4);
    void *x = sf_malloc(1);
    void *y = sf_malloc(1);
    void *z = sf_malloc(1);
    sf_malloc(1); // separator
    sf_free(x);
    sf_free(y);
    sf_free(z);
    sf_snapshot();

    free_list *fl = &seg_free_list[0];
    sf_free_header *head = fl->head;

    printf("head: %p\n", head);
    printf("Z: %p\n", (z-8));

/*
    void *v = sf_malloc(65); //48
    void *w = sf_malloc(230); //160
    void *x = sf_malloc(1095); //544
    void *y = sf_malloc(2600); //2080
    sf_snapshot();
    void *z = sf_malloc(1);

    sf_snapshot();

    //int allocated_block_size[4] = {48, 160, 544, 2080};

    sf_free(x);
    void *a = sf_malloc(900);
    sf_free(v);
    sf_snapshot();
    sf_free(w);
    void *b = sf_malloc(240);
    void *c = sf_malloc(6500);

*/

    // First block in each list should be the most recently freed block
    /*
    for (int i = 0; i < FREE_LIST_COUNT; i++) {
        sf_free_header *fh = (sf_free_header *)&(seg_free_list[i].head);
        cr_assert_not_null(fh, "list %d is NULL!", i);
        cr_assert(fh->header.block_size << 4 == allocated_block_size[i], "Unexpected free block size!");
        cr_assert(fh->header.allocated == 0, "Allocated bit is set!");
    }
    */
    //*ptr = 320320320e-320;
    /*
    printf("%f\n", *ptr);

    sf_free(ptr);

    */
    /*
    sf_free(a);
    sf_free(c);

    sf_snapshot();
    sf_free(b);

    sf_free(y);
    sf_free(z);
    */


    sf_mem_fini();

    return EXIT_SUCCESS;

}
