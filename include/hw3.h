#include "sfmm.h"


#define PAGE_SIZE 4096
#define WORD_SIZE 2
#define MEMROW_SIZE 8

int size_to_list(size_t size);
sf_free_header *find_first_fit(size_t size);
void *extend_mem();
sf_footer *get_footer(sf_header *header);
sf_header *get_header(sf_footer *footer);
size_t get_block_size(void *ptr);

void add_to_list(sf_free_header *header); // TODO
void remove_from_list(sf_free_header *header); // TODO

void *backwards_coalesce(void *ptr); //TODO

sf_free_header *fowards_coalesce(void *ptr);