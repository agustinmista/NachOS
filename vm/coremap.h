#ifndef COREMAP_H
#define COREMAP_

#include "copyright.h"
#include "bitmap.h"
#include "addrspace.h"
#include "machine.h"
#include "list.h"
//#include "system.h"

typedef struct CoreMapEntry {
    AddrSpace *space;
    TranslationEntry *entry;
    //int vpn;
} CoreMapEntry;

class CoreMap {
  public:
    CoreMap();
    ~CoreMap();
    
    int Find(AddrSpace *space, TranslationEntry *entry);
    void Clear(int frame);

  private:
	List <int> *physframes;
    CoreMapEntry pages[NumPhysPages];
    int usedPages;
    int clock_find(bool use, bool dirty);
    int clock_find();
};

#endif // BITMAP_H
