#pragma once

#include <stivale2.h>
#include <memory/paging.hpp>
#include <storage/ahci/ahci.hpp>

extern k_ahci_driver *mainDriver;

/**
 * @brief The entry point of our kernel
 *
 * @param stivale2_struct The structure provided by the bootloader, following the stivale2 boot protocol.
 */
extern "C" void kernelMain(stivale2_struct *stivaleInfo);

void kernelInitialize(stivale2_struct *stivaleInfo);

void kernelPanic(const char *msg, ...);

void kernelHalt();
