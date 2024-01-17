#ifndef __PCI_H
#define __PCI_H

#include "types.h"

#define PCI_CMD_IOSPACE  (1 << 0)
#define PCI_CMD_MEMSPACE (1 << 1)

struct pci_iterator
{
    uint16_t bus, slot, func;
};

__attribute((packed)) struct pci_device
{
    // each row = 32bits
    uint16_t vendor_id, device_id;
    uint16_t command, status;
    uint8_t revision_id, prog_if, subclass, class;
    uint8_t cl_size, l_timer, header_type, bist;

    uint32_t bar[6];
};


void pci_enum_begin(struct pci_iterator *iter);
BOOL pci_enum_next(struct pci_iterator *iter, volatile struct pci_device **device);

#endif // __PCI_H
