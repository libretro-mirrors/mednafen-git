#ifndef __SEXYAL_DRIVERS_DOS_COMMON_H
#define __SEXYAL_DRIVERS_DOS_COMMON_H

#include "../sexyal.h"
#include <math.h>
#include <dos.h>
#include <dpmi.h>
#include <go32.h>
#include <sys/farptr.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

struct pci_vd_pair
{
 uint16_t vendor;
 uint16_t device;
};

bool pci_bios_present(void);
bool pci_find_device(uint16_t vend_id, uint16_t dev_id, uint16_t index, uint16_t* bdf);
pci_vd_pair* pci_find_device(pci_vd_pair* pci_ids, uint32_t index, uint16_t* bdf);	// Terminate list with { 0 } 

uint8_t pci_read_config_u8(uint16_t bdf, unsigned index);
uint16_t pci_read_config_u16(uint16_t bdf, unsigned index);
uint32_t pci_read_config_u32(uint16_t bdf, unsigned index);

void pci_write_config_u8(uint16_t bdf, unsigned index, uint8_t value);
void pci_write_config_u16(uint16_t bdf, unsigned index, uint16_t value);
void pci_write_config_u32(uint16_t bdf, unsigned index, uint32_t value);

#endif
