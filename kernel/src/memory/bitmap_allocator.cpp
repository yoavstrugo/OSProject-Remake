#include <memory/bitmap_allocator.hpp>

#include <memory/paging.hpp>

BitmapAllocator::BitmapAllocator()
{
    this->_freeMemory = 0;
    this->_usedMemory = 0;
    this->_reservedMemory = 0;
}

void BitmapAllocator::initialize(stivale2_struct_tag_memmap *memmapStruct)
{
    if (this->_isInitialized)
        return;
    this->_isInitialized = true;

    // Before we start mapping the memory we have to calculate the memory size
    uint64_t memorySize = countMemory(memmapStruct);

    uint64_t bitmapSize = ((memorySize) / PAGE_SIZE + 1) / BYTE_SIZE;
    uint8_t tempArr[bitmapSize]; // Create a temporary array for the bitmap,
                                 // we will change it late

    memset((char *)tempArr, 0, bitmapSize);

    this->_memoryBitmap.setBuffer(tempArr, bitmapSize);

    // Map the memory to the temporary array
    for (uint64_t entryIdx = 0; entryIdx < memmapStruct->entries; entryIdx++)
    {
        stivale2_mmap_entry entry = memmapStruct->memmap[entryIdx];

        bool memStatus = (entry.type == STIVALE2_MMAP_USABLE);

        this->_freeMemory += memStatus ? entry.length : 0;
        this->_reservedMemory += !memStatus ? entry.length : 0;

        for (uint64_t pageAddr = 0; pageAddr < entry.length; pageAddr += PAGE_SIZE)
        {
            // Calculate the absolute address of the page, get the page index
            // (in bytes) and divide by the bit size
            uint64_t pageBit = (entry.base + pageAddr) / PAGE_SIZE;

            this->_memoryBitmap.set(pageBit, memStatus);
        }
    }

    uint64_t bitmapPage;
    // Find the first place to put the bit map in
    for (uint64_t pageIndex = 0; pageIndex < bitmapSize; pageIndex++)
    {
        if (this->_memoryBitmap[pageIndex])
        {
            // Found an empty page, check if this section has enough space for
            // the bitmap
            bool found = true;
            for (uint64_t i = 0; i < bitmapSize / PAGE_SIZE + 1; i++)
            {
                if (!this->_memoryBitmap[pageIndex + i])
                {
                    found = false;
                    break;
                }
            }

            if (found)
            {
                bitmapPage = pageIndex;
                break;
            }
        }
    }

    // TODO: Do something in case there's no place for the bitmap

    // Calculate the address
    virtual_address_t bitmapAddress = PAGING_APPLY_DIRECTMAP(bitmapPage * PAGE_SIZE);
    memset((char *)bitmapAddress, 0, bitmapSize);

    // Copy the temp array to the location
    memcpy((void *)bitmapAddress, (void *)tempArr, bitmapSize);

    // Change the buffer of the bitmap
    this->_memoryBitmap.setBuffer((uint8_t *)bitmapAddress, bitmapSize);

    // TODO: Lock the pages of the bitmap
    for (uint64_t i = 0; i < PAGING_ALIGN_PAGE_UP(bitmapSize) / PAGE_SIZE + 1; i++)
        this->_lockBlock(PAGING_REMOVE_DIRECTMAP(bitmapAddress) + i * PAGE_SIZE);
}

void BitmapAllocator::freePage(physical_address_t blockAddr)
{
    this->_freeBlock(blockAddr);
}

uint64_t BitmapAllocator::totalMemory()
{
    return this->_freeMemory + this->_usedMemory + this->_reservedMemory;
}

uint64_t BitmapAllocator::freeMemory()
{
    return this->_freeMemory;
}

uint64_t BitmapAllocator::usedMemory()
{
    return this->_usedMemory;
}

uint64_t BitmapAllocator::reservedMemory()
{
    return this->_reservedMemory;
}

physical_address_t BitmapAllocator::allocatePage()
{
    // TODO: do something if no free block
    physical_address_t blockAddr = this->_nextBlock();
    this->_lockBlock(blockAddr);
    return blockAddr;
}

void BitmapAllocator::_lockBlock(physical_address_t blockAddr)
{
    // TODO: check that blockAddr is page aligned and do something
    // if its not

    // TODO: something if not initiallized
    if (!this->_isInitialized)
        return;

    if (this->_memoryBitmap[blockAddr / PAGE_SIZE])
    {
        this->_freeMemory -= PAGE_SIZE;
        this->_usedMemory += PAGE_SIZE;

        this->_memoryBitmap.set(blockAddr / PAGE_SIZE, 0);
    }
}

void BitmapAllocator::_freeBlock(physical_address_t blockAddr)
{
    // TODO: check that blockAddr is page aligned and do something
    // if its not

    // TODO: something if not initiallized
    if (!this->_isInitialized)
        return;

    if (!this->_memoryBitmap[blockAddr / PAGE_SIZE])
    {
        this->_freeMemory += PAGE_SIZE;
        this->_usedMemory -= PAGE_SIZE;

        this->_memoryBitmap.set(blockAddr / PAGE_SIZE, 1);
    }
}

physical_address_t BitmapAllocator::_nextBlock()
{
    // TODO: something if not initiallized
    if (!this->_isInitialized)
        return 0;

    for (uint64_t i = 0; i < this->_memoryBitmap.getSize(); i++)
        if (this->_memoryBitmap[i])
            return i * PAGE_SIZE;
    return 0;
}

uint64_t countMemory(stivale2_struct_tag_memmap *memmapStruct)
{
    uint64_t memorySize = 0;
    {
        for (uint64_t i = 0; i < memmapStruct->entries; i++)
        {
            stivale2_mmap_entry entry = memmapStruct->memmap[i];
            memorySize += entry.length;
        }
    }

    return memorySize;
}