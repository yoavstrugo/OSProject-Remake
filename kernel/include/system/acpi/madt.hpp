#pragma once

#include <system/acpi/acpi.hpp>

struct k_madt_hdr
{
    k_acpi_sdt_hdr header;
    uint32_t localAPICAddr;
    uint32_t flags;
} __attribute__((packed));

enum MADT_ENTRY_TYPE
{
    P_LAPIC = 0,    // Processor Local APIC
    IOAPIC = 1,     // I/O APIC
    IOAPIC_ISO = 2, // IO/APIC Interrupt Source Override
    IOAPIC_NIS = 3, // IO/APIC Non-maskable interrupt source
    LAPIC_NI = 4,   // Local APIC Non-maskable interrupts
    LAPIC_AO = 5,   // Local APIC Address Override
    P_L2APIC = 9    // Processor Local x2APIC
};

template <class T>
struct k_madt_entry_header_node
{
    T *header;
    k_madt_entry_header_node<T> *next;
};

struct k_madt_entry_header
{
    uint8_t entryType;
    uint8_t recordLength;
} __attribute__((packed));

// This type represents a single logical processor and its local interrupt controller.
struct k_madt_lapic_entry
{
    k_madt_entry_header header;

    uint8_t processorId;
    uint8_t apicId;
    uint32_t flags;
} __attribute__((packed));

/**
 * @brief This optional structure supports 64-bit systems by
 * providing an override of the physical address of the local
 * APIC in the MADT’s table header, which is defined as a 32-bit field.
 */
struct k_madt_lapic_address_override_entry
{
    // Reserved (must be set to zero)
    uint16_t reserved0;
    // Physical address of Local APIC
    uint64_t localAPICAddr;
} __attribute__((packed));

/**
 * @brief This type represents a I/O APIC.
 * The global system interrupt base is the first interrupt number that this I/O APIC handles.
 */
struct k_madt_ioapic_entry
{
    k_madt_entry_header header;

    uint8_t ioapicId;
    uint8_t reserved0;
    uint32_t ioapicAddress;
    uint32_t globalSystemInterruptBase;
} __attribute__((packed));

/**
 * @brief This entry type contains the data for an Interrupt Source Override.
 * This explains how IRQ sources are mapped to global system interrupts
 */
struct k_madt_ioapic_interrupt_src_override_entry
{
    k_madt_entry_header header;

    uint8_t busSource;
    uint8_t irqSource;
    uint32_t globalSystemInterrupt;
    uint32_t flags;
} __attribute__((packed));

/**
 * @brief Parses the MADT table
 *
 * @param madtHeader The madt header
 */
void madtParse(k_acpi_sdt_hdr *madtHeader);