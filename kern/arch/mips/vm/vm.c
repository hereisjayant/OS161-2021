#include <types.h>
#include <spinlock.h>
#include <vm.h>

#define INVALID_COREMAP_ENTRY -1

struct coremap_entry* coremap;      // the coremap array
struct spinlock coremap_spinlock;   // spinlock for the coremap
bool bootstrapped;                  // tracks whether or not vm_bootstrap has completed
int num_pages_ram;                  // the amount pages in ram and the amout of coremap entries we can have
vaddr_t coremap_end;                // used to calculate which physical page a virtual address should map to

/**
 * @brief Initializes the structures required for the vm
 * 
 */
void vm_bootstrap(void) {
    paddr_t ram_free_end;
    paddr_t ram_free_start;
    int num_coremap_ppage;

    spinlock_init(&coremap_spinlock);

    // calculate the pages of physical memory we can use
    ram_free_end = ram_getsize();
    ram_free_start = ram_getfirstfree();
    num_pages_ram = (ram_free_end - ram_free_start) / PAGE_SIZE;

    // figure out how much of the free physical memory we just calculated will be used by the coremap
    // no coremap entries will map to these pages
    num_coremap_ppage = (sizeof(struct coremap_entry) * num_pages_ram) / PAGE_SIZE;
    if ((sizeof(struct coremap_entry)*num_pages_ram) % PAGE_SIZE != 0) {
        num_coremap_ppage++;
    }
    num_pages_ram = num_pages_ram - num_coremap_ppage;

    // manually create a coremap array, and figure out how much memory it takes (page aligned)
    coremap = (struct coremap_entry*)PADDR_TO_KVADDR(ram_free_start);
    coremap_end = (vaddr_t)coremap + num_coremap_ppage * PAGE_SIZE;
    
    // fill in the coremap with inital values
    for (int i = 0; i < num_pages_ram; i++) {
        coremap[i].kvaddr = coremap_end + i * PAGE_SIZE;
        coremap[i].is_free = true;
        coremap[i].npages = 0;
    }

    // if vm_boostrap has not been called, we should use ram_stealmem to allocate kpages, but if
    // it has been called, use our alloc_kpages function
    bootstrapped = true;
}

/* Fault handling function called by trap code */
int vm_fault(int faulttype, vaddr_t faultaddress) {
    (void)faulttype;
    (void)faultaddress;
    return 0;
}

/**
 * @brief Allocates @param npages continous pages to a kernel process
 * 
 * @note returns zero if no contiguous pages could be found
 * @param npages The amount of pages to contiguously allocate
 * @return vaddr_t The virtual address of the allocated page(s)
 */
vaddr_t alloc_kpages(unsigned npages) {
    int free_coremap_entry = INVALID_COREMAP_ENTRY;
    unsigned pages_so_far = 0;

    // check if we have initialized the coremap and the vm, use ram_stealmem if we have not
    if (!bootstrapped) {
        return PADDR_TO_KVADDR(ram_stealmem(npages));
    }
    
    spinlock_acquire(&coremap_spinlock);

    // go through the coremap and find the first entry with npages that are free (contiguous)
    for (int i = 0; i < num_pages_ram; i++) {
        // if we reach the number of pages we are looking for
        if (pages_so_far == npages) {
            break;        
        } else if (coremap[i].is_free) {
            if (pages_so_far == 0) {
                free_coremap_entry = i;
            }
            pages_so_far++;         
        } else {
            free_coremap_entry = INVALID_COREMAP_ENTRY;
            pages_so_far = 0;
        }
    }
    // if we cannot find contiguous free coremap entries to be allocated, we return 0
    if (free_coremap_entry == INVALID_COREMAP_ENTRY || pages_so_far != npages) {
        return 0;
    }
    // mark all coremap entries we allocated as not free, and keep track of npages so we can easily free them
    for (unsigned i = 0; i < npages; i++) {
        coremap[free_coremap_entry + i].is_free = false;
        coremap[free_coremap_entry + i].npages = npages;
    }
    vaddr_t alloc_addr = coremap[free_coremap_entry].kvaddr;
    
    spinlock_release(&coremap_spinlock);

    return alloc_addr;
}

/**
 * @brief Frees up the pages that are related to the pages corresponding to the given address
 * @note leaks memory allocated before the vm was initialized
 * @param addr the address corresponding to which we want to free the related pages
 */
void free_kpages(vaddr_t addr) {
    int coremap_entry;
    unsigned npages;

    // leak memory that was allocated before the coremap was initialized as in piazza post @1329
    if (addr < coremap_end) {
        return;
    }
    spinlock_acquire(&coremap_spinlock);
    // calculate the index of the coremap entry the address corresponds to 
    // if addr is not page aligned we will level it out
    coremap_entry = (addr - coremap_end) / PAGE_SIZE;
    if ((addr - coremap_end) % PAGE_SIZE != 0) {
        coremap_entry++;
    }
    
    // free the amount of pages that was initally allocated
    npages = coremap[coremap_entry].npages;
    for (unsigned i = 0; i < npages; i++) {
        coremap[coremap_entry + i].is_free = true;
        coremap[coremap_entry + i].npages = 0;
    }
    spinlock_release(&coremap_spinlock);
}

/* TLB shootdown handling called from interprocessor_interrupt */
void vm_tlbshootdown_all(void) {

}
void vm_tlbshootdown(const struct tlbshootdown* shootdown) {
    (void) shootdown;
}