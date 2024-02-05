#include "kernel.h"
#include "riscv.h"

void pci_enum_begin(struct pci_iterator *iter)
{
    iter->bus = 0;
    iter->slot = 0;
    iter->func = 0;
}

BOOL pci_enum_next(struct pci_iterator *iter, volatile struct pci_device **device)
{
    u16 vendor_id = 0x0000;
    while ((vendor_id == 0x0000 || vendor_id == 0xFFFF) && iter->bus < 256)
    {
        u64 offset = (iter->bus << 16) | (iter->slot << 11) | (iter->func << 8);
        *device = (volatile struct pci_device *)(PCI_MMIO + offset);
        vendor_id = (*device)->vendor_id;

        // increment iterator index
        iter->func = (iter->func + 1) % 8;
        iter->slot = (iter->slot + (iter->func == 0)) % 32;
        // don't constrain bus to 0-255, since we check if >=256 to stop
        iter->bus = (iter->bus + (iter->slot == 0 && iter->func == 0));
    }
    return vendor_id != 0x0000 && vendor_id != 0xFFFF;
}

void pci_dbgprint(volatile struct pci_device* device) {
    printf("PCI display device initialized\n");
    printf("device_id:%hu vendor_id:%hu\n", device->device_id, device->vendor_id);
    printf("    header:%hu command:%hu status:%hu\n", device->header_type, device->command, device->status);
    for (usize i = 0; i < sizeof(device->bar) / sizeof(u32); i++)
        printf("    bar%p: %hu\n", i, device->bar[i]);
}
