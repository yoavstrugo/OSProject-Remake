#include <memory/paging.hpp>

#include <memory/memory.hpp>
#include <kernel.hpp>
#include <strings.hpp>
#include <types.hpp>
#include <logger/logger.hpp>

// TODO: checking that the allocateBlock() returned a valid address
// TODO: checking addresses alignment
void pagingMapPageInSpace(virtual_address_t virt, physical_address_t phys,
                          physical_address_t pml4Addr,
                          k_paging_flags flags,
                          bool override)
{

#ifdef VERBOSE_PAGING
        logDebugn("%! Starting mapping of page \
                    \n0x%64x to 0x%64x, at PML4 0x%64x:",
                  "[Paging]", virt, phys, pml4Addr);
                  #endif

    // Creating PDPT
    pagetable_entry_t *pml4 = (pagetable_entry_t *)PAGING_APPLY_DIRECTMAP(pml4Addr);
    pagetable_entry_t pml4e = pml4[PML4_INDEXER(virt)];
    #ifdef VERBOSE_PAGING
        logDebugn("\t- PML4 entry indexed %d", PML4_INDEXER(virt));
        #endif
    pagetable_entry_t *pdpt;

    if (!(pml4e & PAGETABLE_PRESENT))
    {
        pml4e |= flags.pml4Flags;
        pdpt = (pagetable_entry_t *)PAGING_APPLY_DIRECTMAP(memoryPhysicalAllocator.allocatePage());
        memset((char *)pdpt, 0, PAGE_SIZE);
        pml4e |= (uint64_t)PAGING_REMOVE_DIRECTMAP(pdpt);
        pml4[PML4_INDEXER(virt)] = pml4e;
        #ifdef VERBOSE_PAGING
            logDebugn("\t- PDPT was created at 0x%64x", pdpt);
            #endif
    }
    else
    {
        // Page directory pointer table is present, retrieve the address
        pdpt = (pagetable_entry_t *)PAGING_APPLY_DIRECTMAP(ADDRESS_EXCLUDE(pml4e));
        // If override is on we will override the flags of the table
        if (override)
        {
            pml4e &= (~((pagetable_entry_t)0)) << 12;
            pml4e |= flags.pml4Flags;
            pml4[PML4_INDEXER(virt)] = pml4e;
            #ifdef VERBOSE_PAGING
                logDebugn("\t- PDPT was overriden at 0x%64x", pdpt);
                #endif
        }
        else
        {
            #ifdef VERBOSE_PAGING
                logDebugn("\t- PDPT was already at 0x%64x", pdpt);
                #endif
        }
    }

    // Creating PD
    pagetable_entry_t pdpte = pdpt[PDPT_INDEXER(virt)];
    #ifdef VERBOSE_PAGING
        logDebugn("\t- PDPT entry indexed %d", PDPT_INDEXER(virt));
        #endif
    pagetable_entry_t *pd;

    if (!(pdpte & PAGETABLE_PRESENT))
    {
        pdpte |= flags.pdptFlags;
        pd = (pagetable_entry_t *)PAGING_APPLY_DIRECTMAP(memoryPhysicalAllocator.allocatePage());
        memset((char *)pd, 0, PAGE_SIZE);
        pdpte |= (uint64_t)PAGING_REMOVE_DIRECTMAP(pd);
        pdpt[PDPT_INDEXER(virt)] = pdpte;
        #ifdef VERBOSE_PAGING
            logDebugn("\t- PD was created at 0x%64x", pd);
            #endif
    }
    else
    {
        // Page directory is present, retrieve the address
        pd = (pagetable_entry_t *)PAGING_APPLY_DIRECTMAP(ADDRESS_EXCLUDE(pdpte));
        // If override is on we will override the flags of the table
        if (override)
        {
            pdpte &= (~((pagetable_entry_t)0)) << 12;
            pdpte |= flags.pdptFlags;
            pdpt[PDPT_INDEXER(virt)] = pdpte;
            #ifdef VERBOSE_PAGING
                logDebugn("\t- PD was overriden at 0x%64x", pd);
                #endif
        }
        else
        {
            #ifdef VERBOSE_PAGING
                logDebugn("\t- PDPT was already at 0x%64x", pd);
                #endif
        }
    }

    // Creating PT
    pagetable_entry_t pde = pd[PD_INDEXER(virt)];
    #ifdef VERBOSE_PAGING
        logDebugn("\t- PD entry indexed %d", PD_INDEXER(virt));
        #endif
    pagetable_entry_t *pt;

    if (!(pde & PAGETABLE_PRESENT))
    {
        pde |= flags.pdFlags;
        pt = (pagetable_entry_t *)PAGING_APPLY_DIRECTMAP(memoryPhysicalAllocator.allocatePage());
        memset((char *)pt, 0, PAGE_SIZE);
        pde |= (uint64_t)PAGING_REMOVE_DIRECTMAP(pt);
        pd[PD_INDEXER(virt)] = pde;
        #ifdef VERBOSE_PAGING
            logDebugn("\t- PT was created at 0x%64x", pt);
            #endif
    }
    else
    {
        // Page table is present, retrieve the address
        pt = (pagetable_entry_t *)PAGING_APPLY_DIRECTMAP(ADDRESS_EXCLUDE(pde));
        // If override is on we will override the flags of the table
        if (override)
        {
            pde &= (~((pagetable_entry_t)0)) << 12;
            pde |= flags.pdFlags;
            pd[PD_INDEXER(virt)] = pde;
            #ifdef VERBOSE_PAGING
                logDebugn("\t- PT was overriden at 0x%64x", pt);
                #endif
        }
        else
        {
            #ifdef VERBOSE_PAGING
                logDebugn("\t- PT was already at 0x%64x", pt);
                #endif
        }
    }

    // Creating the page
    pagetable_entry_t pte = pt[PT_INDEXER(virt)];
    #ifdef VERBOSE_PAGING
        logDebugn("\t- PT entry indexed %d", PT_INDEXER(virt));
        #endif

    // Whether if override is on or the page isn't present, set the flags and the address.
    if (!(pte & PAGE_PRESENT) || override)
    {
        memset(&pte, 0, sizeof(pagetable_entry_t));
        pte |= flags.ptFlags;
        pte |= phys;
        pt[PT_INDEXER(virt)] = pte;
        #ifdef VERBOSE_PAGING
            logDebugn("\t- PT was created/overriden at 0x%64x", ADDRESS_EXCLUDE(pte));
            #endif
    }
}

void pagingMapPage(virtual_address_t virt, physical_address_t phys,
                   k_paging_flags flags,
                   bool override)
{
    pagingMapPageInSpace(virt, phys, (physical_address_t)pagingGetCurrentSpace(), flags, override);
}

void pagingMapMemoryInTable(virtual_address_t virt, physical_address_t phys, uint64_t size,
                            physical_address_t pml4Addr,
                            k_paging_flags flags,
                            bool override)
{
    // TODO: check alignment
    for (uint64_t offset = 0; offset < size; offset += PAGE_SIZE)
    {
        pagingMapPageInSpace(virt + offset, phys + offset, pml4Addr, flags, override);
    }
}

void pagingMapMemory(virtual_address_t virt, physical_address_t phys, uint64_t size,
                     k_paging_flags flags,
                     bool override)
{
    pagingMapMemoryInTable(virt, phys, size, pagingGetCurrentSpace(), flags, override);
}

bool pagingIsPagetableEmpty(pagetable_entry_t *pagetable)
{
    for (int i; i < PAGETABLE_SIZE; i++)
        if (CHECK_FLAG(pagetable[i], PAGETABLE_PRESENT))
            return false;
    return true;
}

physical_address_t pagingUnmapPageInSpace(virtual_address_t virt, physical_address_t pml4Addr) {
    pagetable_entry_t *pml4 = (pagetable_entry_t *)PAGING_APPLY_DIRECTMAP(pml4Addr);
    physical_address_t mappedPage = NULL;

    pagetable_entry_t pml4e = pml4[PML4_INDEXER(virt)];
    if (pml4e & PAGETABLE_PRESENT)
    {
        pagetable_entry_t *pdpt = (pagetable_entry_t *)PAGING_APPLY_DIRECTMAP(ADDRESS_EXCLUDE(pml4e));

        pagetable_entry_t pdpte = pdpt[PDPT_INDEXER(virt)];
        if (pdpte & PAGETABLE_PRESENT)
        {
            pagetable_entry_t *pd = (pagetable_entry_t *)PAGING_APPLY_DIRECTMAP(ADDRESS_EXCLUDE(pdpte));

            pagetable_entry_t pde = pd[PD_INDEXER(virt)];
            if (pde & PAGETABLE_PRESENT)
            {
                pagetable_entry_t *pt = (pagetable_entry_t *)PAGING_APPLY_DIRECTMAP(ADDRESS_EXCLUDE(pde));

                pagetable_entry_t pte = pt[PT_INDEXER(virt)];
                if (pte & PAGE_PRESENT)
                {
                    // Get the physical address for the page
                    mappedPage = ADDRESS_EXCLUDE(pte);

                    UNSET_FLAG(pte, PAGE_PRESENT);
                    pt[PT_INDEXER(virt)] = pte;

                    // memset(&pt[PT_INDEXER(virt)], 0, sizeof(pagetable_entry_t));

                    // Check if PT is empty now
                    if (pagingIsPagetableEmpty(pt))
                    {
                        // Free the PT space
                        // memset((void *)pt, 0, PAGE_SIZE);
                        memoryPhysicalAllocator.freePage(ADDRESS_EXCLUDE(pde));

                        // Unset the PRESENT bit in the PD entry
                        UNSET_FLAG(pde, PAGETABLE_PRESENT);
                        pd[PD_INDEXER(virt)] = pde;

                        // memset(&pd[PD_INDEXER(virt)], 0, sizeof(pagetable_entry_t));

                        // Check if PD is empty now
                        if (pagingIsPagetableEmpty(pd))
                        {
                            // Free the PD space
                            // memset((void *)pd, 0, PAGE_SIZE);
                            memoryPhysicalAllocator.freePage(ADDRESS_EXCLUDE(pdpte));

                            // Unset the PRESENT bit in the PDPT entry
                            UNSET_FLAG(pdpte, PAGETABLE_PRESENT);
                            pdpt[PDPT_INDEXER(virt)] = pdpte;

                            // memset(&pdpt[PD_INDEXER(virt)], 0, sizeof(pagetable_entry_t));

                            // Check if PDPT is empty now
                            if (pagingIsPagetableEmpty(pdpt))
                            {
                                // Free the PDPT space
                                // memset((void *)pdpt, 0, PAGE_SIZE);
                                memoryPhysicalAllocator.freePage(ADDRESS_EXCLUDE(pml4e));

                                // Unset the PRESENT bit in the PML4 entry
                                UNSET_FLAG(pml4e, PAGETABLE_PRESENT);
                                pml4[PML4_INDEXER(virt)] = pml4e;
                                // memset(&pml4[PML4_INDEXER(virt)], 0, sizeof(pagetable_entry_t));
                            }
                        }
                    }
                }
            }
        }
    }

    return mappedPage;
}

physical_address_t pagingUnmapPage(virtual_address_t virt)
{
    return pagingUnmapPageInSpace(virt, pagingGetCurrentSpace());
}

void pagingInitialize(physical_address_t kernelBase, virtual_address_t hhdm)
{
    physical_address_t pml4Addr = memoryPhysicalAllocator.allocatePage();

    logDebugn("%! Mapping first 4GiB", "[Memory]");
    // Identity map first 4GiB of memory
    pagingMapMemoryInTable(0x0000000000000000, 0x0000000000000000, 4 * GiB_SIZE, pml4Addr, PAGING_DEFAULT_FLAGS, true);
    logDebugn("%! Done mapping first 4GiB", "[Memory]");

    logDebugn("%! Mapping first 4GiB from HHDM", "[Memory]");
    // Map 4GiB from the HHDM
    pagingMapMemoryInTable(hhdm, 0x0000000000000000, 4 * GiB_SIZE, pml4Addr, PAGING_DEFAULT_FLAGS, true);
    logDebugn("%! Done maapping first 4GiB from HHDM", "[Memory]");

    logDebugn("%! Mapping higher half", "[Memory]");
    // Map the higher half kernel
    pagingMapMemoryInTable(0xffffffff80000000, kernelBase, 0x80000000, pml4Addr, PAGING_DEFAULT_FLAGS, true);
    logDebugn("%! Done mapping higherhalf", "[Memory]");

    pagingSwitchSpace(pml4Addr);
}

void pagingSwitchSpace(physical_address_t phys)
{
    asm("mov cr3, %[aPhys]"
        :
        : [aPhys] "r"(phys));
}

physical_address_t pagingGetCurrentSpace()
{
    // Get the address of the current space from cr3
    pagetable_entry_t *pml4;
    asm volatile("mov %0, cr3"
                 : "=r"(pml4));
    return (physical_address_t)pml4;
}

physical_address_t pagingVirtualToPhysical(virtual_address_t virt)
{
    pagetable_entry_t *pml4 = (pagetable_entry_t *)PAGING_APPLY_DIRECTMAP(pagingGetCurrentSpace());

    pagetable_entry_t pml4e = pml4[PML4_INDEXER(virt)];
    if (!(pml4e & PAGETABLE_PRESENT))
        return NULL;

    pagetable_entry_t *pdpt = (pagetable_entry_t *)PAGING_APPLY_DIRECTMAP(ADDRESS_EXCLUDE(pml4e));

    pagetable_entry_t pdpte = pdpt[PDPT_INDEXER(virt)];
    if (!(pdpte & PAGETABLE_PRESENT))
        return NULL;
    if (CHECK_FLAG(pdpte, PAGETABLE_PAGE_SIZE))
        return ADDRESS_EXCLUDE(pdpte); // Handle 4GiB pages

    pagetable_entry_t *pd = (pagetable_entry_t *)PAGING_APPLY_DIRECTMAP(ADDRESS_EXCLUDE(pdpte));

    pagetable_entry_t pde = pd[PD_INDEXER(virt)];
    if (!(pde & PAGETABLE_PRESENT))
        return NULL;
    if (CHECK_FLAG(pde, PAGETABLE_PAGE_SIZE))
        return ADDRESS_EXCLUDE(pde); // Handle 2MiB pages

    pagetable_entry_t *pt = (pagetable_entry_t *)PAGING_APPLY_DIRECTMAP(ADDRESS_EXCLUDE(pde));

    pagetable_entry_t pte = pt[PT_INDEXER(virt)];
    if (!(pte & PAGE_PRESENT))
        return NULL;

    return (physical_address_t)(ADDRESS_EXCLUDE(pte) + OFFSET_EXCLUDE(virt));
}

 
void pagingCopyKernelMappings(physical_address_t dest) {
    pagetable_entry_t *currPML4 = (pagetable_entry_t *)PAGING_APPLY_DIRECTMAP(pagingGetCurrentSpace());
    pagetable_entry_t *destPML4 = (pagetable_entry_t *)PAGING_APPLY_DIRECTMAP(dest);
    for (uint64_t i = 256; i < 512; i++)
        destPML4[i] = currPML4[i];    
}