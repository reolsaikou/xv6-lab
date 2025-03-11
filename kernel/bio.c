// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.

#define BUCKET 13

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"


uint Hash(uint dev, uint blockno){
  return ((dev * 239) + blockno) % BUCKET;
}

struct {
  // struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  // struct buf head;
} bcache;

// struct entry {
//   int key;
//   struct buf* buf;
//   struct entry* next;
// } entrys[NBUF];

struct buf table[BUCKET];
struct spinlock locktable[BUCKET];
char bcache_name[BUCKET][16];
struct spinlock evictlock;

void
binit(void)
{
  struct buf *b;

  // initlock(&bcache.lock, "bcache");
  initlock(&evictlock, "evict");

  for(int i=0;i < BUCKET;i++){
    snprintf(bcache_name[i], 15, "bcache%d", i);
    initlock(&locktable[i], bcache_name[i]);
  }

  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    initsleeplock(&b->lock, "buffer");
    b->lastused = 0;
    b->refcnt = 0;
    b->next = table[0].next;
    table[0].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  uint key = Hash(dev, blockno);

  acquire(&locktable[key]);

  // Is the block already cached?
  
  for(b = table[key].next; b; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&locktable[key]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  release(&locktable[key]);
  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  acquire(&evictlock);
  for(b = table[key].next; b; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      acquire(&locktable[key]);
      b->refcnt++;
      release(&locktable[key]);
      release(&evictlock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  int time = -1;
  struct buf *evictbufprev = b;
  int lastevictkey = -1;
  int evictkey = -1;
  for(uint i=0;i < BUCKET;i++){
    // if(i == key) continue;
    acquire(&locktable[i]);
    b = &table[i];
    while(b->next){
      if((b->next->refcnt == 0) && (time < 0 || time < b->next->lastused)){
        time = b->next->lastused;
        evictbufprev = b;
        if(evictkey != i) lastevictkey = evictkey;
        evictkey = i;
      }
      b = b->next;
    }
    // if(lastevictkey == evictkey) continue;
    if(evictkey != i) release(&locktable[i]);
    if(lastevictkey != -1 && evictkey == i)
      release(&locktable[lastevictkey]);
  }
  if(evictkey == -1) panic("bget: no buffers");
  if(evictkey != key) acquire(&locktable[key]);
  b = evictbufprev->next;
  evictbufprev->next = evictbufprev->next->next;
  b->dev = dev;
  b->blockno = blockno;
  b->valid = 0;
  b->refcnt = 1;
  b->next = table[key].next;
  table[key].next = b;
  if(evictkey != key) release(&locktable[key]);
  release(&locktable[evictkey]);
  release(&evictlock);
  acquiresleep(&b->lock);
  return b;
  // for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
  //   if(b->refcnt == 0) {
  //     b->dev = dev;
  //     b->blockno = blockno;
  //     b->valid = 0;
  //     b->refcnt = 1;
  //     release(&bcache.lock);
  //     acquiresleep(&b->lock);
  //     return b;
  //   }
  // }
  // panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int key = Hash(b->dev, b->blockno);
  acquire(&locktable[key]);
  b->refcnt--;
  if (b->refcnt == 0) {
    b->lastused = ticks;
  }
  release(&locktable[key]);
}

void
bpin(struct buf *b) {
  int key = Hash(b->dev, b->blockno);
  acquire(&locktable[key]);
  b->refcnt++;
  release(&locktable[key]);
}

void
bunpin(struct buf *b) {
  int key = Hash(b->dev, b->blockno);
  acquire(&locktable[key]);
  b->refcnt--;
  release(&locktable[key]);
}


