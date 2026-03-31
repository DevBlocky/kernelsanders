#include "kernel.h"
#include "riscv.h"
#include "types.h"

// offsets for important registers
#define VIRTIO_REG_MAGIC 0x0
#define VIRTIO_REG_VERSION 0x04
#define VIRTIO_REG_DEVICE_ID 0x08
#define VIRTIO_REG_VENDOR_ID 0x0c
#define VIRTIO_REG_DEVICE_FEATURES 0x10
#define VIRTIO_REG_DEVICE_FEATURES_SEL 0x14
#define VIRTIO_REG_DRIVER_FEATURES 0x20
#define VIRTIO_REG_DRIVER_FEATURES_SEL 0x24
#define VIRTIO_REG_QUEUE_SEL 0x30
#define VIRTIO_REG_QUEUE_NUM_MAX 0x34
#define VIRTIO_REG_QUEUE_NUM 0x38
#define VIRTIO_REG_QUEUE_READY 0x44
#define VIRTIO_REG_QUEUE_NOTIFY 0x50
#define VIRTIO_REG_STATUS 0x70
#define VIRTIO_REG_QUEUE_DESC_LO 0x80
#define VIRTIO_REG_QUEUE_DESC_HI 0x84
#define VIRTIO_REG_QUEUE_DRIVER_LO 0x90
#define VIRTIO_REG_QUEUE_DRIVER_HI 0x94
#define VIRTIO_REG_QUEUE_DEVICE_LO 0xa0
#define VIRTIO_REG_QUEUE_DEVICE_HI 0xa4
#define VIRTIO_REG_INTERRUPT_STATUS 0x60
#define VIRTIO_REG_INTERRUPT_ACK    0x64
#define VIRTIO_REG_CONFIG 0x100

// values for important registers
#define VIRTIO_MAGIC 0x74726976
#define VIRTIO_VERSION 0x2
#define VIRTIO_BLOCK_DEVICE 0x2
#define VIRTIO_STATUS_ACKNOWLEDGE 1
#define VIRTIO_STATUS_DRIVER 2
#define VIRTIO_STATUS_DRIVER_OK 4
#define VIRTIO_STATUS_FEATURES_OK 8
#define VIRTIO_STATUS_FAILED 128

// flags for device features
#define VIRTIO_BLK_F_SIZE_MAX (1ULL << 1)
#define VIRTIO_BLK_F_SEG_MAX (1ULL << 2)
#define VIRTIO_BLK_F_RO (1ULL << 5)
#define VIRTIO_BLK_F_BLK_SIZE (1ULL << 6)
#define VIRTIO_BLK_F_FLUSH (1ULL << 9)
#define VIRTIO_F_VERSION_1 (1ULL << 32)

// flags for descriptor table entries
#define VIRTQ_DESC_F_NEXT 1
#define VIRTQ_DESC_F_WRITE 2

// types for block device requests
#define VIRTIO_BLK_T_IN  0 // read
#define VIRTIO_BLK_T_OUT 1 // write

// BLK device configuration
struct virtio_blk_config {
  u32 capacity_lo;
  u32 capacity_hi;
  u32 size_max;
  u32 seg_max;
  struct virtio_blk_geometry {
    u16 cylinders;
    u8 heads;
    u8 sectors;
  } geometry;
  u32 blk_size;
  struct virtio_blk_topology {
    u8 physical_block_exp;
    u8 alignment_offset;
    u16 min_io_size;
    u32 opt_io_size;
  } topology;
  u8 writeback;
  u8 unused0[3];
  u32 max_discard_sectors;
  u32 max_discard_seg;
  u32 discard_sector_alignment;
  u32 max_write_zeroes_sectors;
  u32 max_write_zeroes_seg;
  u8 write_zeroes_may_unmap;
  u8 unused1[3];
} __attribute__((packed));

// virtqueue structure
#define VIRTQ_ENTRY_NUM 256
struct virtq_desc_ent {
  u64 address;
  u32 length;
  u16 flags;
  u16 next;
} __attribute__((packed));
struct virtq_available {
  u16 flags;
  u16 index;
  u16 ring[VIRTQ_ENTRY_NUM];
  u16 event_index;
} __attribute__((packed));
struct virtq_used {
  u16 flags;
  u16 index;
  struct virtq_used_ent {
    u32 index;
    u32 length;
  } ring[VIRTQ_ENTRY_NUM];
  u16 avail_event;
} __attribute__((packed));

// virtio block device request
#define VIOBLK_MAX_REQS (VIRTQ_ENTRY_NUM / 3) // one per 3 desc table slots
struct virtio_blk_req {
  u32 type;
  u32 reserved;
  u64 sector;
  u8 status;
} __attribute__((packed));

// initialized block device information
static struct {
  usize dev;
  struct virtq_desc_ent *virtq_desc;
  struct virtq_available *virtq_available;
  struct virtq_used *virtq_used;
  u64 capacity;
  u32 size_max;
  u32 seg_max;
  u32 blk_size;

  struct virtio_blk_req blk_reqs[VIOBLK_MAX_REQS];
  u16 blk_req;
} kblk;

#define R(dev, reg) ((volatile u32 *)((usize)(dev) + (reg)))

// initialize a virtio block device
static void virtio_blk_init(usize dev) {
  memset(&kblk, 0, sizeof(kblk));
  kblk.dev = dev;
  if (*R(dev, VIRTIO_REG_MAGIC) != VIRTIO_MAGIC ||
      *R(dev, VIRTIO_REG_VERSION) != VIRTIO_VERSION ||
      *R(dev, VIRTIO_REG_DEVICE_ID) != VIRTIO_BLOCK_DEVICE)
    panic("invalid virtio block device");

  // reset the device
  *R(dev, VIRTIO_REG_STATUS) = 0;
  // acknowledge the device
  *R(dev, VIRTIO_REG_STATUS) |= VIRTIO_STATUS_ACKNOWLEDGE;
  // set driver status bit
  *R(dev, VIRTIO_REG_STATUS) |= VIRTIO_STATUS_DRIVER;

  // negotiate features
  u64 dev_features, dri_features = 0;
  *R(dev, VIRTIO_REG_DEVICE_FEATURES_SEL) = 0;
  dev_features = (u64)*R(dev, VIRTIO_REG_DEVICE_FEATURES);
  *R(dev, VIRTIO_REG_DEVICE_FEATURES_SEL) = 1;
  dev_features |= (u64)*R(dev, VIRTIO_REG_DEVICE_FEATURES) << 32;
  if (dev_features & VIRTIO_BLK_F_SIZE_MAX)
    dri_features |= VIRTIO_BLK_F_SIZE_MAX;
  if (dev_features & VIRTIO_BLK_F_SEG_MAX)
    dri_features |= VIRTIO_BLK_F_SEG_MAX;
  if (dev_features & VIRTIO_BLK_F_RO)
    panic("virtio blk device F_RO set (readonly)");
  if (dev_features & VIRTIO_BLK_F_BLK_SIZE)
    dri_features |= VIRTIO_BLK_F_BLK_SIZE;
  if (dev_features & VIRTIO_F_VERSION_1)
    dri_features |= VIRTIO_F_VERSION_1;
  else
    panic("virtio blk device F_VERSION_1 not set (legacy)");

  // set driver features from negotiation
  *R(dev, VIRTIO_REG_DRIVER_FEATURES_SEL) = 0;
  *R(dev, VIRTIO_REG_DRIVER_FEATURES) = dri_features & 0xffffffff;
  *R(dev, VIRTIO_REG_DRIVER_FEATURES_SEL) = 1;
  *R(dev, VIRTIO_REG_DRIVER_FEATURES) = (dri_features >> 32) & 0xffffffff;
  // set FEATURES_OK
  *R(dev, VIRTIO_REG_STATUS) |= VIRTIO_STATUS_FEATURES_OK;
  if ((*R(dev, VIRTIO_REG_STATUS) & VIRTIO_STATUS_FEATURES_OK) == 0)
    panic("virtio block device FEATURES_OK is not set");

  // note features about block device
  volatile struct virtio_blk_config *cfg =
      (volatile struct virtio_blk_config *)R(dev, VIRTIO_REG_CONFIG);
  kblk.capacity = ((u64)cfg->capacity_hi << 32) | cfg->capacity_lo;
  kblk.size_max = cfg->size_max;
  kblk.seg_max = cfg->seg_max;
  kblk.blk_size = cfg->blk_size;
  printf("capacity: %p\n", kblk.capacity);
  printf("size_max: %u\n", kblk.size_max);
  printf("seg_max:  %u\n", kblk.seg_max);
  printf("blk_size: %u\n", kblk.blk_size);

  // allocate physical pages for the virtqueue
  kblk.virtq_desc = pgalloc();
  kblk.virtq_available = pgalloc();
  kblk.virtq_used = pgalloc();
  kvmmap((usize)kblk.virtq_desc, (usize)kblk.virtq_desc, PGSIZE, PTE_R | PTE_W);
  kvmmap((usize)kblk.virtq_available, (usize)kblk.virtq_available, PGSIZE,
         PTE_R | PTE_W);
  kvmmap((usize)kblk.virtq_used, (usize)kblk.virtq_used, PGSIZE, PTE_R | PTE_W);
  memset(kblk.virtq_desc, 0, PGSIZE);
  memset(kblk.virtq_available, 0, PGSIZE);
  memset(kblk.virtq_used, 0, PGSIZE);

  // setup the virtqueue on the device
  *R(dev, VIRTIO_REG_QUEUE_SEL) = 0;
  if (*R(dev, VIRTIO_REG_QUEUE_NUM_MAX) < VIRTQ_ENTRY_NUM)
    panic("VIRTQ_ENTRY_NUM exceeds maximum virtqueue size");
  *R(dev, VIRTIO_REG_QUEUE_NUM) = VIRTQ_ENTRY_NUM;
  *R(dev, VIRTIO_REG_QUEUE_DESC_LO) = ((usize)kblk.virtq_desc) & 0xffffffff;
  *R(dev, VIRTIO_REG_QUEUE_DESC_HI) =
      ((usize)kblk.virtq_desc >> 32) & 0xffffffff;
  *R(dev, VIRTIO_REG_QUEUE_DRIVER_LO) =
      ((usize)kblk.virtq_available) & 0xffffffff;
  *R(dev, VIRTIO_REG_QUEUE_DRIVER_HI) =
      ((usize)kblk.virtq_available >> 32) & 0xffffffff;
  *R(dev, VIRTIO_REG_QUEUE_DEVICE_LO) = ((usize)kblk.virtq_used) & 0xffffffff;
  *R(dev, VIRTIO_REG_QUEUE_DEVICE_HI) =
      ((usize)kblk.virtq_used >> 32) & 0xffffffff;
  *R(dev, VIRTIO_REG_QUEUE_READY) = 1;

  // set the device as ready to use
  *R(dev, VIRTIO_REG_STATUS) |= VIRTIO_STATUS_DRIVER_OK;
}
void vioblkinit(void) { virtio_blk_init(VIRTIO_MMIO); }

u16 vioblkbeginrw(u64 bufferpa, usize seg, BOOL write) {
  assert(seg < kblk.capacity);

  // create the virtio_blk_req
  u16 reqidx = kblk.blk_req++ % VIOBLK_MAX_REQS;
  struct virtio_blk_req *req = &kblk.blk_reqs[reqidx];
  req->type = write ? VIRTIO_BLK_T_OUT : VIRTIO_BLK_T_IN;
  req->reserved = 0;
  req->sector = seg;
  req->status = 0xff; // device will overwrite with 0 on success

  // emplace descriptors
  u16 descidx = reqidx * 3;
  kblk.virtq_desc[descidx] = (struct virtq_desc_ent){
      .address = (u64)req,
      .length = sizeof(*req) - sizeof(req->status),
      .flags = VIRTQ_DESC_F_NEXT,
      .next = descidx + 1,
  };
  kblk.virtq_desc[descidx + 1] = (struct virtq_desc_ent){
      .address = (u64)bufferpa,
      .length = kblk.blk_size,
      .flags = VIRTQ_DESC_F_NEXT | (write ? 0 : VIRTQ_DESC_F_WRITE),
      .next = descidx + 2,
  };
  kblk.virtq_desc[descidx + 2] = (struct virtq_desc_ent){
      .address = (u64)&req->status,
      .length = sizeof(req->status),
      .flags = VIRTQ_DESC_F_WRITE,
      .next = 0,
  };

  // notify the device that there is a new request
  u16 availidx = kblk.virtq_available->index++ % VIRTQ_ENTRY_NUM;
  kblk.virtq_available->ring[availidx] = descidx;
  fence();
  *R(kblk.dev, VIRTIO_REG_QUEUE_NOTIFY) = 0;

  // return descidx for polling
  return descidx;
}

int vioblkstatus(u16 handle) {
  struct virtio_blk_req *req = (struct virtio_blk_req *)kblk.virtq_desc[handle].address;
  return req->status;
}

void vioblkintr(void) {
  u32 status = *R(kblk.dev, VIRTIO_REG_INTERRUPT_STATUS);
  *R(kblk.dev, VIRTIO_REG_INTERRUPT_ACK) = status;
}

void vioblkwait(u16 *handles, int n) {
  for (int i = 0; i < n; i++)
    while (vioblkstatus(handles[i]) == 0xff)
      wfi();
}

usize vioblkcap(void) { return kblk.capacity; }
u32 vioblkblksz(void) { return kblk.blk_size; }

// bulk read or write contiguous sectors from the block device
//
// NOTE: buf should be aligned 512 so each segment does not
// cross a page boundary
void vioblkrw(u8 *buf, usize start, usize count, BOOL write) {
  u16 handles[VIOBLK_MAX_REQS];
  for (usize s = 0; s < count; ) {
    usize batch = count - s;
    if (batch > VIOBLK_MAX_REQS) batch = VIOBLK_MAX_REQS;

    for (usize i = 0; i < batch; i++) {
      u8 *sector = buf + (start + s + i) * kblk.blk_size;
      handles[i] = vioblkbeginrw(kvmpa((usize)sector), start + s + i, write);
    }

    vioblkwait(handles, (int)batch);
    s += batch;
  }
}
