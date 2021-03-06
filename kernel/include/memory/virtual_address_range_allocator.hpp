#pragma once

#include <memory/paging.hpp>

/**
 * @brief The header of a address range
 */
struct k_address_range_header
{
    // The start of the address range, must be page-aligned
    virtual_address_t base;

    // How many pages does the address range contain
    uint64_t pages;

    // Is the address range in use?
    bool used;

    char requestBy[4];

    // The next address range in the list
    k_address_range_header *next;
};

/**
 * @brief   Allocator for virtual address ranges.
 *          It can deliver virtual address ranges on-demand,
 *          with a specific number of pages.
 */
struct k_virtual_address_range_allocator
{
    k_virtual_address_range_allocator();

    /**
     * @brief   Adds a range to the virtual address range allocator, so it will
     *          able to allocated ranges from it
     *
     * @param start The start of the range, must be page-aligned
     * @param end   The end of the range, must be page-aligned
     */
    void addRange(virtual_address_t start, virtual_address_t end);

    /**
     * @brief   Allocated a virtual range with how many pages requested
     *
     * @param pages How many pages shall the address range contain
     * @return virtual_address_t The start of the address range
     */
    virtual_address_t allocateRange(uint64_t pages, const char *request);

    /**
     * @brief Free an allocated virtual range.
     */
    void freeRange(virtual_address_t base);

    bool useRange(virtual_address_t start, uint64_t size);

    /**
     * @brief Returns all the ranges of this allocator
     * 
     * @return k_address_range_header* The head of the ranges linked list.
     */
    k_address_range_header *getRanges();

private:
    // The head of the address range linked-list
    k_address_range_header *head;
    k_address_range_header *lastSplit;

    /**
     * @brief Loop over the list and try to merge the address ranges
     */
    void mergeAll();
};