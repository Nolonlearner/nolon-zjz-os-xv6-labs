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


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"
#define NBUCKETS 13
int HASHNUM(uint n)
{
  return n % NBUCKETS;
}

struct
{
  struct spinlock lock;
  struct buf buf[NBUF];
 
  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head[NBUCKETS];           // 哈希桶头
  struct spinlock hash_lock[NBUCKETS]; // 哈希桶锁
} bcache;
 
void binit(void)
{
  struct buf *b;
 
  initlock(&bcache.lock, "bcache");
 
  // 哈希桶和哈希桶锁初始化
  for (int i = 0; i < NBUCKETS; i++)
  {
    // snprintf(name, 20, "bcache.bucket.%d", i);
    // printf("name:%s\n",);
    initlock(&bcache.hash_lock[i], "bcache.bucket");
    bcache.head[i].prev = &bcache.head[i];
    bcache.head[i].next = &bcache.head[i];
  }
  int hash_num;
  for (int i = 0; i < NBUF; i++)
  {
    b = &bcache.buf[i];
    // printf("blockno:%d\n",b->blockno);
    hash_num = HASHNUM(b->blockno);
    b->next = bcache.head[hash_num].next;
    b->prev = &bcache.head[hash_num];
    initsleeplock(&b->lock, "buffer");
    bcache.head[hash_num].next->prev = b;
    bcache.head[hash_num].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf *
bget(uint dev, uint blockno)
{
  struct buf *b;
 
  int hash_num = HASHNUM(blockno);
 
  // acquire(&bcache.lock);
 
  // todo 获取哈希桶锁
  acquire(&bcache.hash_lock[hash_num]);
 
  // Is the block already cached?
  // 如果块已经映射在缓冲中
  // 增加块引用数 维护LRU队列 释放bcache锁 获取块的睡眠锁
  for (b = bcache.head[hash_num].next; b != &bcache.head[hash_num]; b = b->next)
  {
    if (b->dev == dev && b->blockno == blockno)
    {
      b->refcnt++;
      // todo 释放哈希锁
      release(&bcache.hash_lock[hash_num]);
      // release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.hash_lock[hash_num]);
 
  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  // 遍历所有哈希桶找到一个引用为空的块
 
  acquire(&bcache.lock);
  acquire(&bcache.hash_lock[hash_num]);
  for (b = bcache.head[hash_num].next; b != &bcache.head[hash_num]; b = b->next)
  {
    if (b->dev == dev && b->blockno == blockno)
    {
      b->refcnt++;
      // todo 释放哈希锁
      release(&bcache.lock);
      release(&bcache.hash_lock[hash_num]);
      // release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.hash_lock[hash_num]);
 
  for (int i = 0; i < NBUCKETS; i++)
  {
    acquire(&bcache.hash_lock[i]);
    for (b = bcache.head[i].prev; b != &bcache.head[i]; b = b->prev)
    {
      if (b->refcnt==0)
      {
        b->dev=dev;
        b->blockno=blockno;
        b->valid=0;
        b->refcnt=1;
 
        b->prev->next=b->next;
        b->next->prev=b->prev;
 
        b->next=bcache.head[hash_num].next;
        b->prev=&bcache.head[hash_num];
        bcache.head[hash_num].next->prev=b;
        bcache.head[hash_num].next=b;
        
        release(&bcache.hash_lock[i]);
        release(&bcache.lock);
        // release(&bcache.hash_lock[hash_num]);
        // release(&bcache.lock);
        acquiresleep(&b->lock);
        return b;
      }      
    }
    release(&bcache.hash_lock[i]);
  }
 
  panic("bget: no buffers");
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
void brelse(struct buf *b)
{
  if (!holdingsleep(&b->lock))
    panic("brelse");
 
  releasesleep(&b->lock);
 
  // acquire(&bcache.lock);
  
  int hash_num = HASHNUM(b->blockno);
  // acquire(&bcache.hash_lock[hash_num]);
  if (b->refcnt > 0)
  {
    b->refcnt--;
  }
  if (b->refcnt == 0)
  {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head[hash_num].next;
    b->prev = &bcache.head[hash_num];
    bcache.head[hash_num].next->prev = b;
    bcache.head[hash_num].next = b;
  }
}
void bpin(struct buf *b)
{
  int hash_num=HASHNUM(b->blockno);
  acquire(&bcache.hash_lock[hash_num]);
  b->refcnt++;
  release(&bcache.hash_lock[hash_num]);
  // acquire(&bcache.lock);
  // b->refcnt++;
  // release(&bcache.lock);
}
 
void bunpin(struct buf *b)
{
  int hash_num=HASHNUM(b->blockno);
  acquire(&bcache.hash_lock[hash_num]);
  b->refcnt--;
  release(&bcache.hash_lock[hash_num]);
  // acquire(&bcache.lock);
  // b->refcnt++;
  // release(&bcache.lock);
}


