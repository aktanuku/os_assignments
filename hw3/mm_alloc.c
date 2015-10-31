/*
 * mm_alloc.c
 *
 * Stub implementations of the mm_* routines. Remove this comment and provide
 * a summary of your allocator's design here.
 */

#include "mm_alloc.h"

#include <stdlib.h>
/* Your final implementation should comment out this macro. */
//#define MM_USE_STUBS
s_block_ptr block_list_head = NULL;
void print_the_fuckin_heap(int bytes){
	char* p = block_list_head;
	int i = 0;
	printf("this is the heap %02X\n", block_list_head ) ;
	for( i = 0; i< bytes; i++){
		printf("%02X\n", (int)p[i]);	
	}

}
void find_max_block(){
	size_t maxsz = 0;
	int maxi = 0;
	int i = 0;
	s_block_ptr iter;

	for(iter = block_list_head; iter != NULL; iter = iter->next){
		if(iter->size > maxsz){
			maxsz = iter->size;
			maxi = i;
		}
		i = i + 1;		
	}
	printf("max :%d, number %d\n", maxsz, i);

}
void print_blockmeta_by_address(void *p){

	s_block_ptr sbp = get_block(p);
	printf("size: %ld\n", sbp->size);
	printf("next addr: %p\n", sbp->next);
	printf("prev addr: %p\n", sbp->prev);
	printf("free?: %d\n", sbp->free);
	printf("ptr: %p\n", sbp->ptr);
	printf("data: %p\n", sbp->data);
}


void print_all_meta(){
	s_block_ptr iter;
	printf("MALLOC LIST CHECK\n");
	for(iter = block_list_head; iter != NULL; iter = iter->next){
		printf("**************BLOCK*****************\n");
		print_blockmeta_by_address(iter->data);
		printf("************************************\n");
	}
}

void* mm_malloc(size_t size)
{
	void * return_ptr;
	s_block_ptr sp;
	s_block_ptr iter;
	s_block_ptr last_block;
	int found = 0;
	#ifdef MM_USE_STUBS
	    return calloc(1, size);
	#else
	if(block_list_head == NULL){
		found = 1;
		sp = extend_heap(block_list_head, size);
		return_ptr = sp->data;
		return return_ptr;
	}
	else{
		last_block = block_list_head;
		int i = 0;
		for(iter = block_list_head; iter != NULL; iter = iter->next){
			i = i + 1;
			
			last_block = iter;
			if(iter->free != 1){
				continue;
			}
			else{
				if(iter->size > size + BLOCK_SIZE){
					found = 1;
					split_block(iter, size);
					iter->next->free = 1;
					iter->free = 0;
					return_ptr = iter->data;
					break;

				}
				else if(iter->size == size + BLOCK_SIZE){
					found = 1;
					iter->free = 0;
					return_ptr = iter->data;
					break;
				}
				
			}
		}
		
		if(found != 1){
			
			sp = extend_heap(last_block, size);
			return_ptr = sp->data;
		

		}
		return return_ptr;
	}
	

	#endif
}


s_block_ptr get_block(void *p){
	return (s_block_ptr)((char*)p - BLOCK_SIZE);
}



void split_block (s_block_ptr b, size_t s){

	
	struct s_block sb;

	s_block_ptr third_block;

	third_block = b->next;

	/*fix next  block*/
	sb.size = b->size - s - BLOCK_SIZE;	
	sb.next =(s_block_ptr)b->next;
	sb.prev = (s_block_ptr)b->ptr;
	sb.free = 1;
	sb.ptr = b->ptr + s + BLOCK_SIZE; /* base address + size of new alloc + block size*/

	/*fix current block*/
	b->size = s;
	b->next = sb.ptr;
	
	/*fix last block */
	if(third_block)
		third_block->prev = sb.ptr;

	/*copy new block meta into memory*/
	memcpy(sb.ptr, &sb, BLOCK_SIZE);

	/*set everything to 0*/
	s_block_ptr new_ptr = (s_block_ptr) sb.ptr;

	memset(new_ptr->data, 0x00, new_ptr->size);
	memset(b->data, 0x00, b->size);
}


/*not used currently*/
int does_block_exit(s_block_ptr b){

	return 1;
}


s_block_ptr extend_heap(s_block_ptr last, size_t s){
	s_block_ptr sp;
	struct s_block sb;
	

	// get current location of program break
	void *p = sbrk(s + BLOCK_SIZE);

	if(p == -1)
		return NULL;

	sb.size = s; /*set size of new block*/
	
	/*if first block set prev block to null*/
	/* else set prev to the previous block*/
	if (last != NULL){
		sb.prev = last->ptr;
		/*update last block's pointer*/
		last->next = p;
	}
	else{
		sb.prev = NULL;
		/*btw set block_list_head*/
		block_list_head = p;
	}
	/*this is the last block in the list*/
	sb.next = NULL;
	/* only do this if there is a request that can't be filled*/
	sb.free = 0;
	/*set pointer to block which will be exactly where sbrk returned earlier*/
	sb.ptr = p;
	memcpy(p, &sb, BLOCK_SIZE);
	
	/*set everything to 0*/
	memset(((s_block_ptr)p)->data, 0x00, s);

	return (s_block_ptr) p;

}
s_block_ptr fusion(s_block_ptr b){
	
	s_block_ptr next_block = NULL;
	s_block_ptr cur_block = NULL;
	
	s_block_ptr new_block;

	size_t new_size;

	

	if(b == NULL)
		return NULL;
	else{
		cur_block = b->ptr;
		
		/*does next block exist?*/
		if(b->next){
			next_block = b->next;
			
			/*is next block free?*/
			if(next_block->free){

				/*compute  new size*/
				cur_block->size = cur_block->size + next_block->size + BLOCK_SIZE;
				
				/*update forward ptr*/
				cur_block->next = next_block->next;

				/*update backward ptr for block in front of us (first check that it exists)*/
	
				if(cur_block->next)
					cur_block->next->prev = cur_block->ptr;

				/*set everything to zero*/
				memset(cur_block->data, 0x00, cur_block->size);	

				return cur_block;
			
			}
			else{
				return cur_block;
			}
			
		}
		else{
			return cur_block;
		}	
	}		
		
			


}

void* mm_realloc(void* ptr, size_t size)
{
#ifdef MM_USE_STUBS
    return realloc(ptr, size);
#else

	void* ptr1; 
	s_block_ptr sb_old;

	sb_old = get_block(ptr);


	ptr1 = mm_malloc(size);

	if(ptr1){
		memcpy(ptr1, sb_old->data, sb_old->size);
		mm_free(ptr);
	}
	
	return ptr1;

#endif
}

void mm_free(void* ptr)
{
#ifdef MM_USE_STUBS
    free(ptr);
#else
    s_block_ptr sbp;
    s_block_ptr new_ptr;
    sbp = get_block(ptr);

    int found = 0;	
    s_block_ptr iter;

    for(iter = block_list_head; iter != NULL; iter = iter->next){
	if(sbp == iter){
		found = 1;
		break;
	}
    }

    if(found != 1){
	printf("Tried to free a pointer you don't own! Exiting. \n");
			
   	print_all_meta(); 
	exit(1);

    }

    if(sbp-> free == 1){
	printf("Tried to free a pointer you don't own but its in my list! Exiting. \n");
	printf("%02X\n", ptr);

	exit(1);
    }

    sbp->free = 1;
    /*does block behind me exist?*/
    if(sbp->prev){
	/*is it free?*/
	if(sbp->prev->free){
		/*attempt fusion*/
		new_ptr = fusion(sbp->prev);

		/*did fusion succede?*/
		if(new_ptr){
			
			/*attempt fusion again*/
			new_ptr = fusion(new_ptr);
		}
	}
	else{
		/*attempt fusion on current block if previous block isnt free*/
		new_ptr = fusion(sbp);
	}

    }
    else{
	/*attempt fusion on current block if previous block doesnt exist*/
        new_ptr = fusion(sbp);
    }
#endif
}
