/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include "sfmm.h"
#include "hw3.h"
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

/**
 * You should store the heads of your free lists in these variables.
 * Doing so will make it accessible via the extern statement in sfmm.h
 * which will allow you to pass the address to sf_snapshot in a different file.
 */
free_list seg_free_list[4] = {
    {NULL, LIST_1_MIN, LIST_1_MAX},
    {NULL, LIST_2_MIN, LIST_2_MAX},
    {NULL, LIST_3_MIN, LIST_3_MAX},
    {NULL, LIST_4_MIN, LIST_4_MAX}
};

int sf_errno = 0;

void *sf_malloc(size_t size) {

    if(size == 0 || size > 4*PAGE_SIZE)
    {
        sf_errno = EINVAL;
        return NULL;
    }

    sf_header alloc_header = {0}, free_header = {0};
    sf_footer alloc_footer = {0};
    size_t total_size = size + 2*sizeof(sf_header);


    if(total_size < 4*MEMROW_SIZE)
    {
        total_size = 4*MEMROW_SIZE;
        alloc_header.padded = 1;
        alloc_footer.padded = 1;
    }
    else if(total_size%(2*MEMROW_SIZE) != 0)
    {
        total_size = 2*MEMROW_SIZE*((total_size+2*MEMROW_SIZE)/(2*MEMROW_SIZE));
        alloc_header.padded = 1;
        alloc_footer.padded = 1;
    }

    sf_free_header *to_fill_header = find_first_fit(total_size);
    if(to_fill_header == NULL)
    {
        sf_errno = ENOMEM;
        return NULL;
    }

    remove_from_list(to_fill_header);
    sf_header *sf_header_ptr = &(to_fill_header->header);

    size_t new_free_size = get_block_size(sf_header_ptr) - total_size;
    if(new_free_size <= 4*MEMROW_SIZE)
    {
        total_size += new_free_size;
        alloc_header.padded = 1;
        alloc_footer.padded = 1;
    }
    else
    {
        sf_footer *to_fill_footer = get_footer(sf_header_ptr);
        to_fill_footer->block_size = new_free_size >> 4;

        free_header.block_size = new_free_size >> 4;
        sf_header *to_fill_header = get_header(to_fill_footer);
        *to_fill_header = free_header;
        add_to_list((sf_free_header *)to_fill_header);
    }

    alloc_header.block_size = total_size >> 4;
    alloc_header.allocated = 1;

    alloc_footer.block_size = total_size >> 4;
    alloc_footer.requested_size = size;
    alloc_footer.allocated = 1;

    *(sf_header *)to_fill_header = alloc_header;
    *get_footer((sf_header *)to_fill_header) = alloc_footer;


    return ((void *)to_fill_header) + sizeof(sf_header);
}

void *sf_realloc(void *ptr, size_t size) {
	sf_header *realloc_header = (sf_header *)(ptr - MEMROW_SIZE);
    sf_footer *realloc_footer = get_footer(realloc_header);
    if((void *)realloc_header < (void *)get_heap_start() ||
        (void *)realloc_footer > (void *)get_heap_end() ||
        realloc_header->allocated == 0 || realloc_footer->allocated == 0)
    {
        sf_errno = EINVAL;
        return NULL;
    }
    if((realloc_footer->requested_size + 2*MEMROW_SIZE != get_block_size(realloc_header) &&
        realloc_footer->padded == 0) || realloc_footer->padded != realloc_header->padded)
    {
        sf_errno = EINVAL;
        return NULL;
    }
    if(size == 0)
    {
        sf_free(ptr);
        return NULL;
    }
    size_t original_size = get_block_size(realloc_header);
    size_t new_total_size = size + 2*sizeof(sf_header);
    if(new_total_size < 4*MEMROW_SIZE)
    {
        new_total_size = 4*MEMROW_SIZE;
    }
    else if(new_total_size%(2*MEMROW_SIZE) != 0)
    {
        new_total_size = 2*MEMROW_SIZE*((new_total_size+2*MEMROW_SIZE)/(2*MEMROW_SIZE));
    }
    if(new_total_size == original_size)
    {
        return ptr;
    }
    if(new_total_size > original_size)
    {
        void *new_ptr = sf_malloc(size);
        if(new_ptr == NULL)
        {
            return NULL;
        }
        memcpy(new_ptr, ptr, original_size - 2*MEMROW_SIZE);
        sf_free(ptr);
        return new_ptr;
    }
    if(new_total_size < original_size)
    {
        size_t total_size = size + 2*sizeof(sf_header);
        size_t original_size = get_block_size(realloc_header);
        sf_header new_header = {0};
        if(total_size < 4*MEMROW_SIZE)
        {
            total_size = 4*MEMROW_SIZE;
            new_header.padded = 1;
        }
        else if(total_size%(2*MEMROW_SIZE) != 0)
        {
            total_size = 2*MEMROW_SIZE*((total_size+2*MEMROW_SIZE)/(2*MEMROW_SIZE));
            new_header.padded = 1;
        }
        size_t diff = original_size - total_size;
        if(diff <= 4*MEMROW_SIZE)
        {
            return ptr;
        }
        else
        {
            new_header.allocated = 1;
            new_header.block_size = total_size >> 4;
            *realloc_header = new_header;
            sf_footer *new_footer = get_footer(realloc_header);
            *new_footer = *(sf_footer *)(&new_header);

            realloc_footer->allocated = 0;
            realloc_footer->block_size = diff >> 4;
            sf_header *new_free_header = get_header(realloc_footer);
            new_free_header->allocated = 0;
            new_free_header->block_size = diff >> 4;
            add_to_list(fowards_coalesce((void *)new_free_header));
            return ptr;
        }

    }


    return NULL;

}

void sf_free(void *ptr)
{
    if(ptr == NULL)
    {
        abort();
    }
    sf_header *to_free_header = (sf_header *)(ptr - MEMROW_SIZE);
    sf_footer *to_free_footer = get_footer(to_free_header);
    if((void *)to_free_header < (void *)get_heap_start() ||
        (void *)to_free_footer > (void *)get_heap_end() ||
        to_free_header->allocated == 0 || to_free_footer->allocated == 0)
    {
        abort();
    }
    if((to_free_footer->requested_size + 2*MEMROW_SIZE != get_block_size(to_free_header) &&
        to_free_footer->padded == 0) || to_free_footer->padded != to_free_header->padded)
    {
        abort();
    }
    to_free_header->allocated = 0;
    to_free_footer->allocated = 0;
    add_to_list(fowards_coalesce(to_free_header));

	return;
}



int size_to_list(size_t size)
{
    if(LIST_1_MAX >= size)
    {
        return 0;
    }
    if(LIST_2_MAX >= size)
    {
        return 1;
    }
    if(LIST_3_MAX >= size)
    {
        return 2;
    }
    return 3;
}

sf_free_header *find_first_fit(size_t size)
{
    for(int i = size_to_list(size); i < FREE_LIST_COUNT; i++)
    {
        sf_free_header *current_free_p = seg_free_list[i].head;
        while(current_free_p != NULL)
        {
            if(get_block_size(&(current_free_p->header)) >= size)
            {
                return current_free_p;
            }
            current_free_p = current_free_p->next;
        }
    }
    if(extend_mem() == NULL)
    {
        return NULL;
    }

    return find_first_fit(size);
}

void *extend_mem()
{
    void *new_block = sf_sbrk();
    if(new_block == (void *)-1)
    {
        return NULL;
    }
    sf_header header = {0};
    sf_footer footer = {0};
    header.block_size = PAGE_SIZE >> 4;
    footer.block_size = PAGE_SIZE >> 4;
    *(sf_header *)new_block = header;

    add_to_list((sf_free_header *)new_block);
    *get_footer((sf_header *)new_block) = footer;


    backwards_coalesce(new_block);
    return new_block;
}

sf_footer *get_footer(sf_header *header)
{
    return (sf_footer *)(((void *)(header)) + get_block_size(header) - MEMROW_SIZE);
}

sf_header *get_header(sf_footer *footer)
{
    return (sf_header *)(((void *)(footer)) - get_block_size(footer) + MEMROW_SIZE);
}

size_t get_block_size(void *ptr)
{
    return (*((sf_header *)(ptr))).block_size << 4;
}


void *backwards_coalesce(void *ptr)
{
    sf_free_header *base_block = (sf_free_header *)ptr;
    sf_footer *prev_footer = (sf_footer *)(ptr - MEMROW_SIZE);
    if(prev_footer->allocated || ptr == (void *)get_heap_start())
    {
        return base_block;
    }

    sf_header *prev_header = get_header(prev_footer);
    if((void *)prev_header < (void *)get_heap_start())
    {
        return base_block;
    }

    size_t base_size = get_block_size(base_block);
    size_t prev_size = get_block_size(prev_header);

    remove_from_list(base_block);
    remove_from_list((sf_free_header *)prev_header);
    //sf_header new_header = {0, (base_size+prev_size) >> 4, 0, 0, 0};
    //*prev_header = new_header;

    prev_header->block_size = (base_size+prev_size) >> 4;
    prev_header->allocated = 0;

    add_to_list((sf_free_header *)prev_header);
    get_footer((sf_header *)base_block)->block_size = (base_size+prev_size) >> 4;

    return prev_header;
}

void remove_from_list(sf_free_header *to_remove)
{
    free_list *list = seg_free_list + size_to_list(get_block_size(&(to_remove->header)));

    if(to_remove->next == NULL && to_remove->prev == NULL)
    {
        list->head = NULL;
    }
    else if(to_remove->next == NULL)
    {
        to_remove->prev->next = NULL;
    }
    else if(to_remove->prev == NULL)
    {
        to_remove->next->prev = NULL;
        list->head = to_remove->next;
    }
    else
    {
        to_remove->prev->next = to_remove->next;
        to_remove->next->prev = to_remove->prev;
    }
}

void add_to_list(sf_free_header *to_add)
{
    free_list *list = seg_free_list + size_to_list(get_block_size(&(to_add->header)));
    sf_free_header *list_head = list->head;
    list->head = to_add;
    to_add->next = list_head;
    to_add->prev = NULL;
    if(list_head != NULL)
    {
        list_head->prev = to_add;
    }
}

sf_free_header *fowards_coalesce(void *ptr)
{
    sf_free_header *base_header = (sf_free_header *)ptr;
    sf_header *next_header = (sf_header *)(get_footer((sf_header *)base_header) + 1); //this might cause problems
    if((void *)next_header > (void *)get_heap_end() || next_header->allocated == 1)
    {
        return base_header;
    }
    sf_free_header *next_free_header = (sf_free_header *)next_header;
    sf_footer *next_footer = get_footer(next_header);
    if((void *)next_footer > (void *)get_heap_end())
    {
        return base_header;
    }

    size_t base_size = get_block_size(&(base_header->header));
    size_t next_size = get_block_size(next_header);

    remove_from_list(next_free_header);
    base_header->header.block_size = (base_size + next_size) >> 4;
    next_footer->block_size = (base_size + next_size) >> 4;

    return base_header;

}
