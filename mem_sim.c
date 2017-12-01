/***************************************************************************
 * *    Inf2C-CS Coursework 2: TLB and Cache Simulation
 * *
 * *    Instructor: Boris Grot
 * *
 * *    TA: Priyank Faldu
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <math.h>

/* Do not add any more header files */

/*
 * Various structures
 */
typedef enum {tlb_only, cache_only, tlb_cache} hierarchy_t;
typedef enum {instruction, data} access_t;
const char* get_hierarchy_type(uint32_t t) {
    switch(t) {
        case tlb_only: return "tlb_only";
        case cache_only: return "cache-only";
        case tlb_cache: return "tlb+cache";
        default: assert(0); return "";
    };
    return "";
}

typedef struct {
    uint32_t address;
    access_t accesstype;
} mem_access_t;

// These are statistics for the cache and TLB and should be maintained by you.
typedef struct {
    uint32_t tlb_data_hits;
    uint32_t tlb_data_misses;
    uint32_t tlb_instruction_hits;
    uint32_t tlb_instruction_misses;
    uint32_t cache_data_hits;
    uint32_t cache_data_misses;
    uint32_t cache_instruction_hits;
    uint32_t cache_instruction_misses;
} result_t;






/*
 * Parameters for TLB and cache that will be populated by the provided code skeleton.
 */
hierarchy_t hierarchy_type = tlb_cache;
uint32_t number_of_tlb_entries = 0;
uint32_t page_size = 0;
uint32_t number_of_cache_blocks = 0;
uint32_t cache_block_size = 0;
uint32_t num_page_table_accesses = 0;


/*
 * Each of the variables (subject to hierarchy_type) below must be populated by you.
 */
uint32_t g_total_num_virtual_pages = 0;
uint32_t g_num_tlb_tag_bits = 0;
uint32_t g_tlb_offset_bits = 0;
uint32_t g_num_cache_tag_bits = 0;
uint32_t g_cache_offset_bits= 0;
result_t g_result;

/* Reads a memory access from the trace file and returns
 * 1) access type (instruction or data access)
 * 2) 32-bit virtual memory address
 */
mem_access_t read_transaction(FILE *ptr_file) {
    char buf[1002];
    char* token = NULL;
    char* string = buf;
    mem_access_t access;

    if (fgets(buf, 1000, ptr_file)!=NULL) {

        /* Get the access type */
        token = strsep(&string, " \n");
        if (strcmp(token,"I") == 0) {
            access.accesstype = instruction;
        } else if (strcmp(token,"D") == 0) {
            access.accesstype = data;
        } else {
            printf("Unkown access type\n");
            exit(-1);
        }

        /* Get the address */
        token = strsep(&string, " \n");
        access.address = (uint32_t)strtol(token, NULL, 16);

        return access;
    }

    /* If there are no more entries in the file return an address 0 */
    access.address = 0;
    return access;
}

/*
 * Call this function to get the physical page number for a given virtual number.
 * Note that this function takes virtual page number as an argument and not the whole virtual address.
 * Also note that this is just a dummy function for mimicing translation. Real systems maintains multi-level page tables.
 */
uint32_t dummy_translate_virtual_page_num(uint32_t virtual_page_num) {
    uint32_t physical_page_num = virtual_page_num ^ 0xFFFFFFFF;
    num_page_table_accesses++;
    if ( page_size == 256 ) {
        physical_page_num = physical_page_num & 0x00FFF0FF;
    } else {
        assert(page_size == 4096);
        physical_page_num = physical_page_num & 0x000FFF0F;
    }
    return physical_page_num;
}

void print_statistics(uint32_t num_virtual_pages, uint32_t num_tlb_tag_bits, uint32_t tlb_offset_bits, uint32_t num_cache_tag_bits, uint32_t cache_offset_bits, result_t* r) {
    /* Do Not Modify This Function */

    printf("NumPageTableAccesses:%u\n", num_page_table_accesses);
    printf("TotalVirtualPages:%u\n", num_virtual_pages);
    if ( hierarchy_type != cache_only ) {
        printf("TLBTagBits:%u\n", num_tlb_tag_bits);
        printf("TLBOffsetBits:%u\n", tlb_offset_bits);
        uint32_t tlb_total_hits = r->tlb_data_hits + r->tlb_instruction_hits;
        uint32_t tlb_total_misses = r->tlb_data_misses + r->tlb_instruction_misses;
        printf("TLB:Accesses:%u\n", tlb_total_hits + tlb_total_misses);
        printf("TLB:data-hits:%u, data-misses:%u, inst-hits:%u, inst-misses:%u\n", r->tlb_data_hits, r->tlb_data_misses, r->tlb_instruction_hits, r->tlb_instruction_misses);
        printf("TLB:total-hit-rate:%2.2f%%\n", tlb_total_hits / (float)(tlb_total_hits + tlb_total_misses) * 100.0);
    }

    if ( hierarchy_type != tlb_only ) {
        printf("CacheTagBits:%u\n", num_cache_tag_bits);
        printf("CacheOffsetBits:%u\n", cache_offset_bits);
        uint32_t cache_total_hits = r->cache_data_hits + r->cache_instruction_hits;
        uint32_t cache_total_misses = r->cache_data_misses + r->cache_instruction_misses;
        printf("Cache:data-hits:%u, data-misses:%u, inst-hits:%u, inst-misses:%u\n", r->cache_data_hits, r->cache_data_misses, r->cache_instruction_hits, r->cache_instruction_misses);
        printf("Cache:total-hit-rate:%2.2f%%\n", cache_total_hits / (float)(cache_total_hits + cache_total_misses) * 100.0);
    }
}

//Creating cache_lines struct
typedef struct {
    int validBit;
    uint32_t tag;
    int index;
    uint32_t data;
} cache_lines;


// Creating tlb struct with the following parameters.
typedef struct {
    uint32_t tag;
    uint32_t pagenum;
    int index;
    uint32_t data;
} tlbLines;


/*
 *
 * Add any global variables and/or functions here as you wish.
 *
 */

 int flag; // Keeps track of the physicalAddress from the TLB.


cache_lines * direct_map_cache; //Direct mapped_cache_array is going to be used for the cache-only with direct-mapped caching.
tlbLines* fa_cache; //fa_cache array is going to be used for the tlb_only with fully-asscociative caching.
int* lru_array; //Used for determining the LRU elements.





//This function implemets the cache-only method using direct-mapped caching.
void directMapping(uint32_t physicalAddress, access_t type) {

    int indexSize, offsetSize, tagSize;

    //This switch statement is used to get the indexSize depending on the number_of_cache_blocks.
    switch (number_of_cache_blocks) {
        case 256:
            indexSize = 8;
            break;
        default:
            indexSize = 11;
    }

    switch (cache_block_size) {
        case 32:
            offsetSize = 5;
            break;
        default:
            offsetSize= 6;
    }


    /*We calculate the tagSize from the values we got from indexSize and offsetSize. After that we populate the global variables
    which are defined at the top with "g_".
    */
    tagSize = 32 - indexSize - offsetSize;
    g_num_cache_tag_bits = tagSize;
    g_cache_offset_bits = offsetSize;
    int representBitsVirtualPage =32-(log2(page_size));
    g_total_num_virtual_pages = pow(2,representBitsVirtualPage);


    //Here we get the tag,index,offset of the physical address by shifting.
    uint32_t getTag = physicalAddress >> (32-tagSize);
    uint32_t getIndex = physicalAddress << g_num_cache_tag_bits  >> (32 - indexSize);
    uint32_t getOffset = physicalAddress << (getTag+getIndex);

    /*The implementation of direct-mapped caching is done here.
    */
    if (direct_map_cache[getIndex].tag == getTag && direct_map_cache[getIndex].validBit) {
        if ( type == data) {
            g_result.cache_data_hits++;
        } else {
            g_result.cache_instruction_hits++;
        }
    } else {
        direct_map_cache[getIndex].validBit = 1;
        direct_map_cache[getIndex].data = type;
        direct_map_cache[getIndex].tag = getTag;
        if ( type == data) {
            g_result.cache_data_misses++;
        } else {
            g_result.cache_instruction_misses++;
        }
    }

}


/*This function is used to look through the array and find if the current tag
matches other which is already in the tlb. If there is one it returns it as int,
if not return -1. */

int findHit(uint32_t tag) {
    for (int i = 0; i <number_of_tlb_entries ; i++) {
        if (fa_cache[i].tag == tag) {
            return i;
        }
    }
    return -1;
}

//This increments the each value in the lru_array.
void lowerUse(){
    for (int i=0; i<number_of_tlb_entries;i++)
        lru_array[i]++;
}

//This is used to set the least recentrly used entry to 0.
void used(int item_id){
    lru_array[item_id] = 0;
}


/*Here we set the max to be equal to the first entry of the lru_array and max_id to 0,
then we loop through the array and we find a value which greater than the max value
and set it equal to i and return that value back to the initial function.*/
int findLRU(){
    uint32_t max;
    uint32_t max_id;

    max = lru_array[0];
    max_id = 0;
    for (int i = 1; i<number_of_tlb_entries;i++)
        if(lru_array[i] > max){
            max = lru_array[i];
            max_id = i;
        }
    return max_id;
}

//This is the function used to simulate the tlb-only.
void tlbOnly (uint32_t VirtualPageNum, access_t type)  {

    /*The varibales item_id and new_item_id are used to keep track of tlb tags,
    where representBitsVirtualPage is used to populate the g_total_num_virtual_pages.*/
    int item_id;
    int new_item_id;
    int representBitsVirtualPage =32-(log2(page_size));
    g_total_num_virtual_pages = pow(2,representBitsVirtualPage);


    item_id = findHit(VirtualPageNum);
    lowerUse();
    if (item_id !=-1) {
        if (type == instruction ) g_result.tlb_instruction_hits++;
        else if (type == data) g_result.tlb_data_hits++;
        else printf("%s", "No access type");
        flag = fa_cache[item_id].pagenum;
        used(item_id);
    }

    else {

        if (type == instruction) g_result.tlb_instruction_misses++;
        else if (type == data) g_result.tlb_data_misses++;
        new_item_id = findLRU();
        fa_cache[new_item_id].tag = VirtualPageNum;
        fa_cache[new_item_id].pagenum = dummy_translate_virtual_page_num(VirtualPageNum);
        flag = fa_cache[new_item_id].pagenum;


        used(new_item_id);
    }


}


int flag; // This is used for storing the physical address when we use the tlb.

int main(int argc, char** argv) {

    /*
     *
     * Read command-line parameters and initialize configuration variables.
     *
     */
    int improper_args = 0;
    char file[10000];
    if ( argc < 2 ) {
        improper_args = 1;
        printf("Usage: ./mem_sim [hierarchy_type: tlb-only cache-only tlb+cache] [number_of_tlb_entries: 8/16] [page_size: 256/4096] [number_of_cache_blocks: 256/2048] [cache_block_size: 32/64] mem_trace.txt\n");
    } else  {
        /* argv[0] is program name, parameters start with argv[1] */
        if ( strcmp(argv[1], "tlb-only") == 0 ) {//
            if ( argc != 5 ) {
                improper_args = 1;
                printf("Usage: ./mem_sim tlb-only [number_of_tlb_entries: 8/16] [page_size: 256/4096] mem_trace.txt\n");
            } else {
                hierarchy_type = tlb_only;
                number_of_tlb_entries = atoi(argv[2]);
                page_size = atoi(argv[3]);
                strcpy(file, argv[4]);
            }
        } else if ( strcmp(argv[1], "cache-only") == 0 ) {
            if ( argc != 6 ) {
                improper_args = 1;
                printf("Usage: ./mem_sim cache-only [page_size: 256/4096] [number_of_cache_blocks: 256/2048] [cache_block_size: 32/64] mem_trace.txt\n");
            } else {
                hierarchy_type = cache_only;
                page_size = atoi(argv[2]);
                number_of_cache_blocks = atoi(argv[3]);
                cache_block_size = atoi(argv[4]);
                strcpy(file, argv[5]);
            }
        } else if ( strcmp(argv[1], "tlb+cache") == 0 ) {
            if ( argc != 7 ) {
                improper_args = 1;
                printf("Usage: ./mem_sim tlb+cache [number_of_tlb_entries: 8/16] [page_size: 256/4096] [number_of_cache_blocks: 256/2048] [cache_block_size: 32/64] mem_trace.txt\n");
            } else {
                hierarchy_type = tlb_cache;
                number_of_tlb_entries = atoi(argv[2]);
                page_size = atoi(argv[3]);
                number_of_cache_blocks = atoi(argv[4]);
                cache_block_size = atoi(argv[5]);
                strcpy(file, argv[6]);
            }
        } else {
            printf("Unsupported hierarchy type: %s\n", argv[1]);
            improper_args = 1;
        }
    }
    if ( improper_args ) {
        exit(-1);
    }
    assert(page_size == 256 || page_size == 4096);
    if ( hierarchy_type != cache_only) {
        assert(number_of_tlb_entries == 8 || number_of_tlb_entries == 16);
    }
    if ( hierarchy_type != tlb_only) {
        assert(number_of_cache_blocks == 256 || number_of_cache_blocks == 2048);
        assert(cache_block_size == 32 || cache_block_size == 64);
    }

    printf("input:trace_file: %s\n", file);
    printf("input:hierarchy_type: %s\n", get_hierarchy_type(hierarchy_type));
    printf("input:number_of_tlb_entries: %u\n", number_of_tlb_entries);
    printf("input:page_size: %u\n", page_size);
    printf("input:number_of_cache_blocks: %u\n", number_of_cache_blocks);
    printf("input:cache_block_size: %u\n", cache_block_size);
    printf("\n");


/* Open the file mem_trace.txt to read memory accesses */
    FILE *ptr_file;
    ptr_file =fopen(file,"r");
    if (!ptr_file) {
        printf("Unable to open the trace file: %s\n", file);
        exit(-1);
    }

    /* result structure is initialized for you. */
    memset(&g_result, 0, sizeof(result_t));

    /* Do not delete any of the lines below.
     * Use the following snippet and add your code to finish the task. */

    /* You may want to setup your TLB and/or Cache structure here. */

    //Dynamically allocate memory for cache-only.
    direct_map_cache =(cache_lines*) calloc(number_of_cache_blocks, sizeof(cache_lines));

    //Dynamically allocate memory for tlb.
    fa_cache = (tlbLines*) calloc(number_of_tlb_entries, sizeof(tlbLines));

    //Dynamically allocate memory for lru_array which is going to be used for tlb_only and initilaized it with 0s.
    lru_array = (int*) calloc(number_of_tlb_entries, sizeof(int));



    mem_access_t access;
    /* Loop until the whole trace file has been read. */
    while(1) {
        access = read_transaction(ptr_file);
        // If no transactions left, break out of loop.
        if (access.address == 0)
            break;
        /* Add your code here */
        /* Feed the address to your TLB and/or Cache simulator and collect statistics. */

        //numberOfOffsetBits is being used for calculating the offset of an address and then using that information to find the tag and index.
        int numberOfOffsetBits;


        // If statements for determining which hierarchy_type is being used and start calculating the number of hits and misses.
        if (hierarchy_type == cache_only) {


            if (page_size == 4096) {
                numberOfOffsetBits=12;
            }
            else {
                numberOfOffsetBits=8;
            }

            //Using the dummy function to translate virtual address to physical and then shifting it to get the actualPhysicalAddress.
            uint32_t physicalPageNum = dummy_translate_virtual_page_num(access.address >> numberOfOffsetBits);
            uint32_t actualPhysicalAddress = physicalPageNum << numberOfOffsetBits | (access.address << (32-numberOfOffsetBits) >> (32-numberOfOffsetBits));

            //Use the direcMapping function to calculate hits and misses for cache-only.
            directMapping(actualPhysicalAddress, access.accesstype);
        }

        //Calculate numberOfOffsetBits depending on the page_size, get the Virtual Adrees and use the tlbOnly function.
        //to calculate misses and hits.
        else if (hierarchy_type == tlb_only) {

          if (page_size == 4096) {
              numberOfOffsetBits=12;
          }
          else {
              numberOfOffsetBits=8;
          }

            g_tlb_offset_bits = numberOfOffsetBits;
            g_num_tlb_tag_bits = 32- g_tlb_offset_bits;

            uint32_t VirtualPageNum = access.address >> numberOfOffsetBits;
            tlbOnly(VirtualPageNum, access.accesstype);
        }


        //Calculate the numberOfOffsetBits and just apply what we have already done for cache-only and tlb-only.
        else if (hierarchy_type == tlb_cache) {

          switch (page_size) {
            case 4096:
              numberOfOffsetBits =12;
              break;

            default:
                numberOfOffsetBits =8;
                break;
          }

          g_tlb_offset_bits = numberOfOffsetBits;
          g_num_tlb_tag_bits = 32 - g_tlb_offset_bits;

          uint32_t VirtualPageNum = access.address >> numberOfOffsetBits;
          tlbOnly(VirtualPageNum, access.accesstype);


          uint32_t actualPhysicalAddres = flag << numberOfOffsetBits | (access.address << (32-numberOfOffsetBits) >> (32-numberOfOffsetBits));
          directMapping(actualPhysicalAddres, access.accesstype);

        }

    }

    //Free the Dynamically allocated space for the arrays after the has ended.
    free(direct_map_cache);
    free(fa_cache);
    free(lru_array);





    /* Do not modify code below. */
    /* Make sure that all the parameters are appropriately populated. */
    print_statistics(g_total_num_virtual_pages, g_num_tlb_tag_bits, g_tlb_offset_bits, g_num_cache_tag_bits, g_cache_offset_bits, &g_result);

    /* Close the trace file. */
    fclose(ptr_file);
    return 0;

}
