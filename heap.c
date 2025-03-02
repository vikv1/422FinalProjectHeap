/*
 * This is a C template of malloc( ) and free( ), based on the buddy
 * memory allocation algorithm. 
 */
#include <stdio.h> // printf

/*
 * The following global variables are used to simulate memory allocation
 * Cortex-M's SRAM space.
 */
// Heap
char array[0x8000];            // simulate SRAM: 0x2000.0000 - 0x2000.7FFF
int heap_top   = 0x20001000;   // the top of heap space
int heap_bot   = 0x20004FE0;   // the address of the last 32B in heap
int max_size   = 0x00004000;   // maximum allocation: 16KB = 2^14
int min_size   = 0x00000020;   // minimum allocation: 32B = 2^5

// Memory Control Block: 2^10B = 1KB space
int mcb_top    = 0x20006800;   // the top of MCB
int mcb_bot    = 0x20006BFE;   // the address of the last MCB entry
int mcb_ent_sz = 0x00000002;   // 2B per MCB entry
int mcb_total  = 512;          // # MCB entries: 2^9 = 512 entries

/*
 * Convert a Cortex SRAM address to the corresponding array index.
 * @param  sram_addr address of Cortex-M's SRAM space starting at 0x20000000.
 * @return array index.
 */
int m2a( int sram_addr ) {
  int index = sram_addr - 0x20000000;
  // printf( "m2a: sram_addr = %x array_index = %d\n", sram_addr, index );
  return index;
}

/*
 * Reverse an array index back to the corresponding Cortex SRAM address.
 * @param  array index.
 * @return the corresponding Cortex-M's SRAM address in an integer.
 */ 
int a2m( int array_index ) {
  return array_index + 0x20000000;
}

/*
 * In case if you want to print out, all array elements that correspond
 * to MCB: 0x2006800 - 0x20006C00.
 */
void printArray( ) {
  int c = 0;
  printf( "memroy ............................\n" );
  for ( int i = 0; i < 0x8000; i+=4 )
    if ( a2m( i ) >= 0x20006800 ) {
	 printf( "%x = %x(%d)\n",
		 a2m( i ), *(int *)&array[i], *(int *)&array[i] ); 
     c += 1;

     if(c == 130) {
      break;
     }
   }

}

int mcb2heap(int addr) {
  return heap_top + (addr - mcb_top) * 16;
}

/*
 * _ralloc is _kalloc's helper function that is recursively called to
 * allocate a requested space, using the buddy memory allocaiton algorithm.
 * Implement it by yourself in step 1.
 *
 * @param  size  the size of a requested memory space
 * @param  left  the address of the left boundary of MCB entries to examine
 * @param  right the address of the right boundary of MCB entries to examine
 * @return the address of Cortex-M's SRAM space. While the computation is
 *         made in integers, cast it to (void *). The gcc compiler gives
 *         a warning sign:
                cast to 'void *' from smaller integer type 'int'
 *         Simply ignore it.
 */
// void* _ralloc( int size, int left, int right ) {
//   int entire = right - left + mcb_ent_sz;
//   int half = entire / 2;
//   int mid = left + half;
//   void* heap_addr = NULL;
//   int act_entire_size = entire * 16;
//   int act_half_size = half * 16;

//   if(size <= act_half_size) {
//     heap_addr = _ralloc(size, left, mid - mcb_ent_sz);
//     if(heap_addr == NULL) {
//       return _ralloc(size, mid, right);
//     }
//   } else if (size > act_half_size) {
//     if(*(short *)(array + m2a(left)) % 2 == 0) {
//       *(short *)&array[m2a(left)] = size | 0x01;
//       return (void *) mcb2heap(left);
//     } else {
//       return NULL;
//     }
//   }

//   return NULL;
// }




/*
 * _ralloc is _kalloc's helper function that is recursively called to
 * allocate a requested space, using the buddy memory allocaiton algorithm.
 * Implement it by yourself in step 1.
 *
 * @param  size  the size of a requested memory space
 * @param  left  the address of the left boundary of MCB entries to examine
 * @param  right the address of the right boundary of MCB entries to examine
 * @return the address of Cortex-M's SRAM space. While the computation is
 *         made in integers, cast it to (void *). The gcc compiler gives
 *         a warning sign:
                cast to 'void *' from smaller integer type 'int'
 *         Simply ignore it.
 */
void* _ralloc( int size, int left, int right ) {
  int entire = right - left + mcb_ent_sz;
  int half = entire / 2;
  int midpt = left + half;
  void* heap_addr = NULL;
  int act_entire_size = entire * 16;
  int act_half_size = half * 16;

  // printf("left: %d\n", (left - mcb_top) / 2);
  // printf("right: %d\n", (right - mcb_top) / 2);
  // printf("allocating: %d\n", size);
  // printf("half size: %d\n", act_half_size);

  if (size <= act_half_size) {
    // only designate available space if the left buddy is not already using it
    if(*(short *)(array + m2a(midpt)) == 0 && *(short *)(array + m2a(left)) % 2 == 0) {
      *(short *)&array[m2a(midpt)] = act_half_size;
    }

    // go left
    heap_addr = _ralloc(size, left, midpt - mcb_ent_sz);
    if(heap_addr == NULL) { // no space on left, go right
      return _ralloc(size, midpt, right);
    } else {
      return heap_addr;
    }
  } else if(*(short *)(array + m2a(left)) != 0) { // if available space
    // printf("value at left mcb: %d\n", *(short *)(array + m2a(left)));
    // printf("allocating %d\n", size);
    // printf("entire size: %d\n", act_entire_size);

    if(*(short *)(array + m2a(left)) % 2 == 0) { // if not in use
      //printArray();
      *(short *)&array[m2a(left)] = act_entire_size | 0x01; // mark in use
      return (void *) mcb2heap(left);
    } 
    return NULL;
  }
  return NULL;
}

/*
 * _rfree is _kfree's helper function that is recursively called to
 * deallocate a space, using the buddy memory allocaiton algorithm.
 * Implement it by yourself in step 1.
 *
 * @param  mcb_addr that corresponds to a SRAM space to deallocate
 * @return the same as the mcb_addr argument in success, otherwise 0.
 */
int _rfree( int mcb_addr ) {
  // printf( "_rfree: mcb[%x] = %x\n",
  //	  mcb_addr, *(short *)&array[ m2a( mcb_addr ) ] )

  short mcb_contents = *(short *)&array[m2a(mcb_addr)];
  // printf("mcb_contents: %x\n", mcb_contents);
  int mcb_offset = mcb_addr - mcb_top;
  int mcb_chunk = (mcb_contents /= 16);
  int my_size = (mcb_contents *= 16);

  *(short *)&array[m2a(mcb_addr)] = mcb_contents;

  if((mcb_offset / mcb_chunk) % 2 == 0) {
    int buddy = mcb_addr + mcb_chunk;
    if(buddy < mcb_bot) {
      if(*(short *)(array + m2a(buddy)) % 2 == 0) { // if buddy not in use
        *(short *)&array[m2a(buddy)] = 0;
        my_size *= 2;
        *(short *)&array[m2a(mcb_addr)] = my_size;
        return _rfree(mcb_addr);
      } else { // buddy in use, reset this addr and return
        *(short *)&array[m2a(mcb_addr)] = my_size;
        return mcb_addr;
      }
    }
  } else {
    int buddy = mcb_addr - mcb_chunk; // left buddy addr
    if(buddy >= mcb_top) {
      if(*(short *)(array + m2a(buddy)) % 2 == 0) {  // if left buddy not in use, coalesce
        *(short *)&array[m2a(mcb_addr)] = my_size;
        return _rfree(buddy);
      } else {
        *(short *)&array[m2a(mcb_addr)] = my_size;
      }
      return mcb_addr;
    }
  }
  
  return mcb_addr;
}

/*
 * Initializes MCB entries. In step 2's assembly coding, this routine must
 * be called from Reset_Handler in startup_TM4C129.s before you invoke
 * driver.c's main( ).
 */
void _kinit( ) {
  // Zeroing the heap space: no need to implement in step 2's assembly code.
  for ( int i = 0x20001000; i < 0x20005000; i++ )
    array[ m2a( i ) ] = 0;

  // Initializing MCB: you need to implement in step 2's assembly code.
  *(short *)&array[ m2a( mcb_top ) ] = max_size;
  // printf("at start: %x", *(int *)(array + m2a(mcb_top)));

  for ( int i = 0x20006804; i < 0x20006C00; i += 2 ) {
    array[ m2a( i ) ] = 0;
    array[ m2a( i + 1) ] = 0;
  }
}

/*
 * Step 2 should call _kalloc from SVC_Handler.
 *
 * @param  the size of a requested memory space
 * @return a pointer to the allocated space
 */
void *_kalloc( int size ) {
  // printf( "_kalloc called\n" );
  size = ( size < 32 ) ? 32 : size; // minimum 32 bytes
  void* ret = _ralloc( size, mcb_top, mcb_bot );
  //printArray();
  return ret;
}

/*
 * Step 2 should call _kfree from SVC_Handler.
 *
 * @param  a pointer to the memory space to be deallocated.
 * @return the address of this deallocated space.
 */
void *_kfree( void *ptr ) {
  int addr = (int )ptr;

  // validate the address
  // printf( "\n_kfree( %x )\n", ptr );
  if ( addr < heap_top || addr > heap_bot )
    return NULL;

  // compute the mcb address corresponding to the addr to be deleted
  int mcb_addr =  mcb_top + ( addr - heap_top ) / 16;
  
  if ( _rfree( mcb_addr ) == 0 )
    return NULL;
  else
    return ptr;
}

/*
 * _malloc should be implemented in stdlib.s in step 2.
 * _kalloc must be invoked through SVC in step 2.
 *
 * @param  the size of a requested memory space
 * @return a pointer to the allocated space
 */
void *_malloc( int size ) {
  static int init = 0;
  if ( init == 0 ) {
    init = 1;
    _kinit( ); // In step 2, you will call _kinit from Reset_Handler 
  }
  return _kalloc( size );
}

/*
 * _free should be implemented in stdlib.s in step 2.
 * _kfree must be invoked through SVC in step 2.
 *
 * @param  a pointer to the memory space to be deallocated.
 * @return the address of this deallocated space.
 */
void *_free( void *ptr ) {
  void* ret = _kfree(ptr);
  return ret;
}
