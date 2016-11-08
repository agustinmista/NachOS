#include "coremap.h"

//------------------------------------------
//  CoreMap::CoreMap(int nitems)
//------------------------------------------
CoreMap::CoreMap() {
	
	physframes = new List<int>;
	
    for (int i=0; i<NumPhysPages; i++){
        pages[i].space = NULL;
        pages[i].entry = NULL;
	}
    usedPages = 0;
}

//------------------------------------------
//  CoreMap::~CoreMap()
//------------------------------------------
CoreMap::~CoreMap() {
	delete physframes;
}


//------------------------------------------
//  void CoreMap::Clear(int which)
//------------------------------------------
void CoreMap::Clear(int frame) {
    ASSERT(frame > 0 && frame < NumPhysPages);
	
    if (pages[frame].space) usedPages--;
    pages[frame].space = NULL;
    pages[frame].entry = NULL;
    physframes->RemItem(frame);
    
}

//------------------------------------------
//  int CoreMap::Find(AddrSpace *space, int page)
//------------------------------------------
int CoreMap::Find(AddrSpace *space, TranslationEntry *entry) {
    
    // no hay lugar en memoria, mandamos una víctima a 
    // swap y usamos su lugar
    if (usedPages == NumPhysPages) {
        #ifdef CLOCK
        int victim = clock_find();
        #else
        int victim = physframes->Remove();
        #endif
        int victimVPN = pages[victim].entry->virtualPage;
        pages[victim].space->SwapOut(victimVPN);
        pages[victim].space = space;
        pages[victim].entry = entry;
        physframes->Append(victim);
        //pages[victim].vpn = vpn;
        
        return victim;
    }
    
    // hay lugar en memoria, buscamos el primero disponible
    for (int i=0; i<NumPhysPages; i++){
        if (!pages[i].space) {
            pages[i].space = space;
            //pages[i].vpn = vpn;
            pages[i].entry = entry;
			physframes->Append(i);
			
            usedPages++;
            return i;
        }
    }
    
    ASSERT(false); //nunca se alcanza
    return -1;
}

#ifdef CLOCK
//Search for clean 
int CoreMap::clock_find(bool use, bool dirty){
	//Al cases can be reduced to (0,0) and (0,1) because clock resets
	//used bits
	
	ASSERT(!physframes->IsEmpty());
	int current = physframes->Remove();
	int init = current;
	
	do{
		TranslationEntry *centry = pages[current].entry;
		//Search for (0,0)
		if(!use && !dirty)
			if (!centry->use && !centry->dirty) return current;
			
		//Search for (0,1) 
		if(!use && dirty)
			if(!centry->use && centry->dirty) return current;

		if(centry->use) centry->use = false;
		physframes->Append(current);
		current = physframes->Remove();
	} while (current != init);
	
	physframes->Prepend(current);
	
	return -1; //if no page was found
}

int CoreMap::clock_find(){
	int victim;
	//(0,0)
	victim = clock_find(false, false);
	if (victim != -1) return victim;
	//(0,1)
	victim = clock_find(false, true);
	if (victim != -1) return victim;
	//(1,0)
	victim = clock_find(false, false);
	if (victim != -1) return victim;
	//(1,1)
	victim = clock_find(false, true);
	if (victim != -1) return victim;
	
	ASSERT(false);
	return -1;
}
#endif

