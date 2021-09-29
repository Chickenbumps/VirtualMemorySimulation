//
// Virual Memory Simulator Homework
// One-level page table system with FIFO and LRU
// Two-level page table system with LRU
// Inverted page table with a hashing system

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define PAGESIZEBITS 12            // page size = 4Kbytes
#define VIRTUALADDRBITS 32        // virtual address space size = 4Gbytes
int power(int a, int b){
    int i;
    int result = 1;
    for (i = 0; i < b; i++) {
        result = result*a;
    }
    return result;
}
struct node{
    int pid;
    int vpn;
    int frameNum;
    int count;
    int hashindex;
    struct node *next;
};

struct hashTable{
    int pid;
    int vpn;
    struct node *head;
};

struct pageTableEntry{
    int valid;
    int frameNum;
    struct pageTableEntry *second;
};

struct procEntry {
    char *traceName;            // the memory trace name
    int pid;                    // process (trace) id
    int ntraces;                // the number of memory traces
    int num2ndLevelPageTable;    // The 2nd level page created(allocated);
    int numIHTConflictAccess;     // The number of Inverted Hash Table Conflict Accesses
    int numIHTNULLAccess;        // The number of Empty Inverted Hash Table Accesses
    int numIHTNonNULLAcess;        // The number of Non Empty Inverted Hash Table Accesses
    int numPageFault;            // The number of page faults
    int numPageHit;                // The number of page hits
    struct pageTableEntry *firstLevelPageTable;
    FILE *tracefp;
};
struct procEntry procTable[20];
struct pageTable {
    int frameNum;
    int valid;
};
struct pageTable pt[20][1048576];
struct frame {
    int vpn;
    int fvpn;
    int svpn;
    int count;
    int time;
    int process;
    int hashIndex;
};
int firstLevelBits;
int phyMemSizeBits;
int nFrame;

void oneLevelVMSim(int type,int numProcess, char *tName[]) {
    struct frame f[nFrame];
    int j;
    for (j =0; j < nFrame; j++) {
        f[j].vpn = 0;
        f[j].count = 0;
        f[j].time = 0;
        f[j].process = 0;
    }
    int i = 0;
    int curFn = 0;
    int curTime = 0;
    while(1) {
        procTable[i].traceName = tName[4+i];
        curTime++;
        unsigned addr;
        char rw;
        fscanf(procTable[i].tracefp,"%x %c",&addr,&rw);
        if(feof(procTable[i].tracefp)) break;
        int vpn = addr>>12;
        procTable[i].ntraces++;
        if(curFn == nFrame && pt[i][vpn].valid == 0){
            if(type == 0){ // FIFO
                int j;
                int old = f[0].time;
                int oldindex = 0;
                for (j = 1; j < nFrame; j++) {
                    if(old > f[j].time){
                        old = f[j].time;
                        oldindex = j;
                    }
                }
                pt[f[oldindex].process][f[oldindex].vpn].valid = 0;
                f[oldindex].count = 0;
                f[oldindex].vpn = vpn;
                f[oldindex].time = curTime;
                f[oldindex].process = i;
                procTable[i].numPageFault++;
                pt[i][vpn].valid = 1;
                pt[i][vpn].frameNum = oldindex;
            }
            else if(type == 1){ // LRU
                int countless = f[0].count;
                int countlessindex = 0;
                int k;
                for (k = 1; k < nFrame; k++) {
                    if(countless > f[k].count){
                        countless = f[k].count;
                        countlessindex = k;
                    }
                }
                pt[f[countlessindex].process][f[countlessindex].vpn].valid = 0;
                f[countlessindex].count = curTime;
                f[countlessindex].vpn = vpn;
                f[countlessindex].process = i;
                procTable[i].numPageFault++;
                pt[i][vpn].valid = 1;
                pt[i][vpn].frameNum = countlessindex;
            }
        }
        else{
            if(pt[i][vpn].valid == 0){
                procTable[i].numPageFault++;
                pt[i][vpn].valid = 1;
                pt[i][vpn].frameNum = curFn;
                f[curFn].vpn = vpn;
                f[curFn].time = curTime;
                f[curFn].process = i;
                f[curFn].count = curTime;
                curFn++;
            } else if(pt[i][vpn].valid == 1){
                procTable[i].numPageHit++;
                f[pt[i][vpn].frameNum].count = curTime;
            }
        }
        i++;
        if(i == numProcess){
            i = 0;
        }
    }
    for(i=0; i < numProcess; i++) {
        printf("**** %s *****\n",procTable[i].traceName);
        printf("Proc %d Num of traces %d\n",i,procTable[i].ntraces);
        printf("Proc %d Num of Page Faults %d\n",i,procTable[i].numPageFault);
        printf("Proc %d Num of Page Hit %d\n",i,procTable[i].numPageHit);
        assert(procTable[i].numPageHit + procTable[i].numPageFault == procTable[i].ntraces);
    }
    
}
void twoLevelVMSim(int numProcess,char *tName[]) {
    struct frame f[nFrame];
    int j;
    for (j =0; j < nFrame; j++) {
        f[j].vpn = 0;
        f[j].fvpn = 0;
        f[j].svpn = 0;
        f[j].count = 0;
        f[j].time = 0;
        f[j].process = 0;
    }
    int fln = power(2,firstLevelBits);
    int sln = power(2,(20-firstLevelBits));
    for (j = 0; j < numProcess; j++) {
        procTable[j].firstLevelPageTable = malloc(sizeof(struct pageTableEntry)*fln);
    }
    int a,b;
    for (a = 0; a < numProcess; a++) {
        for (b = 0; b < fln; b++) {
            procTable[a].firstLevelPageTable[b].frameNum = 0;
            procTable[a].firstLevelPageTable[b].valid = 0;
            procTable[a].firstLevelPageTable[b].second = NULL;
        }
    }
    int i =0;
    int curFn = 0;
    int curTime = 0;
    while (1) {
        curTime++;
        procTable[i].traceName = tName[4+i];
        procTable[i].pid = i;
        unsigned addr;
        char rw;
        fscanf(procTable[i].tracefp,"%x %c",&addr,&rw);
        if(feof(procTable[i].tracefp)) break;
        int vpn = addr>>12;
        int fvpn = vpn / power(2,20-firstLevelBits);
        int svpn = vpn % power(2,20-firstLevelBits);
        
        if(procTable[i].firstLevelPageTable[fvpn].second == NULL){
            procTable[i].firstLevelPageTable[fvpn].second = malloc(sizeof(struct pageTableEntry)*sln);
            int k;
            for (k = 0 ; k < sln; k++) {
                procTable[i].firstLevelPageTable[fvpn].second[k].valid = 0;
                procTable[i].firstLevelPageTable[fvpn].second[k].frameNum = 0;
                procTable[i].firstLevelPageTable[fvpn].second[k].second = NULL;
            }
            procTable[i].num2ndLevelPageTable++;
        }
        procTable[i].ntraces++;
        if(curFn == nFrame && procTable[i].firstLevelPageTable[fvpn].second[svpn].valid == 0){
            int countless = f[0].count;
            int countlessindex = 0;
            int m;
            for (m = 1; m < nFrame; m++) {
                if(countless > f[m].count){
                    countless = f[m].count;
                    countlessindex = m;
                }
            }
            procTable[f[countlessindex].process].firstLevelPageTable[f[countlessindex].fvpn].second[f[countlessindex].svpn].valid = 0;
            f[countlessindex].count = curTime;
            f[countlessindex].vpn = vpn;
            f[countlessindex].fvpn = fvpn;
            f[countlessindex].svpn = svpn;
            f[countlessindex].process = i;
            procTable[i].numPageFault++;
            procTable[i].firstLevelPageTable[fvpn].second[svpn].valid = 1;
            procTable[i].firstLevelPageTable[fvpn].second[svpn].frameNum = countlessindex;
        }
        else{
            if(procTable[i].firstLevelPageTable[fvpn].second[svpn].valid == 0){
                procTable[i].numPageFault++;
                procTable[i].firstLevelPageTable[fvpn].second[svpn].valid = 1;
                procTable[i].firstLevelPageTable[fvpn].second[svpn].frameNum = curFn;
                f[curFn].vpn = vpn;
                f[curFn].fvpn = fvpn;
                f[curFn].svpn = svpn;
                f[curFn].time = curTime;
                f[curFn].process = i;
                f[curFn].count = curTime;
                curFn++;
            } else if(procTable[i].firstLevelPageTable[fvpn].second[svpn].valid == 1){
                procTable[i].numPageHit++;
                f[procTable[i].firstLevelPageTable[fvpn].second[svpn].frameNum].count = curTime;
            }
        }
        i++;
        if(i == numProcess){
            i = 0;
        }
    }
    for(i=0; i < numProcess; i++) {
        printf("**** %s *****\n",procTable[i].traceName);
        printf("Proc %d Num of traces %d\n",i,procTable[i].ntraces);
        printf("Proc %d Num of second level page tables allocated %d\n",i,procTable[i].num2ndLevelPageTable);
        printf("Proc %d Num of Page Faults %d\n",i,procTable[i].numPageFault);
        printf("Proc %d Num of Page Hit %d\n",i,procTable[i].numPageHit);
        assert(procTable[i].numPageHit + procTable[i].numPageFault == procTable[i].ntraces);
    }
}

void invertedPageVMSim(int numProcess, char *tName[]) {
    struct frame f[nFrame];
    int g;
    for (g =0; g < nFrame; g++) {
        f[g].vpn = 0;
        f[g].count = 0;
        f[g].hashIndex = 0;
        f[g].process = 0;
    }
    struct hashTable ht[nFrame];
    int k;
    for (k = 0; k < nFrame; k++) {
        ht[k].head = NULL;
        ht[k].pid = 0;
    }
    int curFn = 0;
    int curTime = 0;
    int i = 0;
    while (1) {
        curTime++;
        procTable[i].traceName = tName[4+i];
        procTable[i].pid = i;
        unsigned addr;
        char rw;
        fscanf(procTable[i].tracefp,"%x %c",&addr,&rw);
        if(feof(procTable[i].tracefp)) break;
        int vpn = addr>>12;
        int hashIndex = (vpn + i) % nFrame;
        procTable[i].ntraces++;
        struct node *newNode = (struct node*)malloc(sizeof(struct node));
        newNode->count = curTime;
        newNode->frameNum = curFn;
        newNode->pid = i;
        newNode->vpn = vpn;
        newNode->hashindex = hashIndex;
        newNode->next = NULL;
        
        if(curFn == nFrame){
            struct node *check = ht[hashIndex].head;
            int checkFlag = 0;
            while(check != NULL){
                procTable[i].numIHTConflictAccess++;
                if(check->pid == i && check->vpn == vpn){
                    checkFlag = 1;
                    break;
                }
                check = check->next;
            }
            if(checkFlag == 1){
                struct node *temp3 = ht[hashIndex].head;
                while(temp3 != NULL){
                    if(temp3->pid == newNode->pid && temp3->vpn == newNode->vpn && temp3->hashindex == newNode->hashindex){
                        procTable[i].numIHTNonNULLAcess++;
                        procTable[i].numPageHit++;
                        temp3->count = curTime;
                        f[temp3->frameNum].count = curTime;
                        break;
                    }
                    temp3 = temp3->next;
                }
            }else{
                int countless = f[0].count;
                int countlessindex = 0;
                int m;
                for (m = 1; m < nFrame; m++) {
                    if(countless > f[m].count){
                        countless = f[m].count;
                        countlessindex = m;
                    }
                }
                if(ht[hashIndex].head != NULL){
                    procTable[i].numIHTNonNULLAcess++;
                }else if(ht[hashIndex].head == NULL){
                    procTable[i].numIHTNULLAccess++;
                }
                struct node *prev= NULL;
                struct node *remove =ht[f[countlessindex].hashIndex].head;
                while(remove != NULL){
                    if(countless == remove->count){
                        if(remove == ht[f[countlessindex].hashIndex].head){
                            ht[f [countlessindex].hashIndex].head = remove->next;
                        }else{
                            prev->next = remove->next;
                        }
                        break;
                    }
                    prev = remove;
                    remove = remove->next;
                }
                newNode->frameNum = countlessindex;
                
                if(ht[hashIndex].head == NULL){
                    ht[hashIndex].head = newNode;
                }else{
                    newNode->next = ht[hashIndex].head;
                    ht[hashIndex].head = newNode;
                }
                procTable[i].numPageFault++;
                f[countlessindex].count = newNode->count;
                f[countlessindex].hashIndex = newNode->hashindex;
                f[countlessindex].process = newNode->pid;
                f[countlessindex].vpn = newNode->vpn;
            }
        }
        else{
            if(ht[hashIndex].head == NULL){
                ht[hashIndex].head = newNode;
                procTable[i].numIHTNULLAccess++;
                procTable[i].numPageFault++;
                f[curFn].count = curTime;
                f[curFn].hashIndex= hashIndex;
                f[curFn].process = i;
                f[curFn].vpn = vpn;
                curFn++;
            }
            else{
                procTable[i].numIHTNonNULLAcess++;
                int flag = 0;
                struct node *temp2 = ht[hashIndex].head;
                while(temp2 != NULL){
                    procTable[i].numIHTConflictAccess++;
                    if(temp2->pid == newNode->pid && temp2->vpn == newNode->vpn && temp2->hashindex == newNode->hashindex){
                        procTable[i].numPageHit++;
                        temp2->count = curTime;
                        f[temp2->frameNum].count = curTime;
                        flag = 1;
                        break;
                    }
                    temp2 = temp2->next;
                }
                if(flag == 0){
                    newNode->next = ht[hashIndex].head;
                    ht[hashIndex].head = newNode;
                    procTable[i].numPageFault++;
                    f[curFn].count = newNode->count;
                    f[curFn].vpn = newNode->vpn;
                    f[curFn].process = newNode->pid;
                    f[curFn].hashIndex = hashIndex;
                    curFn++;
                }
            }
        }
        i++;
        if(i == numProcess){
            i = 0;
        }
    }
    for(i=0; i < numProcess; i++) {
        printf("**** %s *****\n",procTable[i].traceName);
        printf("Proc %d Num of traces %d\n",i,procTable[i].ntraces);
        printf("Proc %d Num of Inverted Hash Table Access Conflicts %d\n",i,procTable[i].numIHTConflictAccess);
        printf("Proc %d Num of Empty Inverted Hash Table Access %d\n",i,procTable[i].numIHTNULLAccess);
        printf("Proc %d Num of Non-Empty Inverted Hash Table Access %d\n",i,procTable[i].numIHTNonNULLAcess);
        printf("Proc %d Num of Page Faults %d\n",i,procTable[i].numPageFault);
        printf("Proc %d Num of Page Hit %d\n",i,procTable[i].numPageHit);
        assert(procTable[i].numPageHit + procTable[i].numPageFault == procTable[i].ntraces);
        assert(procTable[i].numIHTNULLAccess + procTable[i].numIHTNonNULLAcess == procTable[i].ntraces);
    }
}

int main(int argc, char *argv[]) {
    int i,c, simType, numProcess;
    simType = atoi(argv[1]);
    firstLevelBits = atoi(argv[2]);
    phyMemSizeBits = atoi(argv[3]);
    numProcess = argc - 4;
    // initialize procTable for Memory Simulations
    for(i = 0; i < numProcess; i++) {
        // opening a tracefile for the process
        printf("process %d opening %s\n",i,argv[i + 1 + 3]);
        procTable[i].tracefp = fopen(argv[i + 1 + 3],"r");
        if (procTable[i].tracefp == NULL) {
            printf("ERROR: can't open %s file; exiting...",argv[i+1+3]);
            exit(1);
        }
    }
    
    nFrame = power(2,(phyMemSizeBits - PAGESIZEBITS));
    printf("Num of Frames %d Physical Memory Size %ld bytes\n",nFrame, (1L<<phyMemSizeBits));
    //initProcTable(numProcess);
    if (simType == 0) {
        printf("=============================================================\n");
        printf("The One-Level Page Table with FIFO Memory Simulation Starts .....\n");
        printf("=============================================================\n");
        oneLevelVMSim(0,numProcess,argv);
    }
    
    if (simType == 1) {
        printf("=============================================================\n");
        printf("The One-Level Page Table with LRU Memory Simulation Starts .....\n");
        printf("=============================================================\n");
        oneLevelVMSim(1,numProcess,argv);
    }
    
    if (simType == 2) {
        printf("=============================================================\n");
        printf("The Two-Level Page Table Memory Simulation Starts .....\n");
        printf("=============================================================\n");
        twoLevelVMSim(numProcess,argv);
    }
    
    if (simType == 3) {
        printf("=============================================================\n");
        printf("The Inverted Page Table Memory Simulation Starts .....\n");
        printf("=============================================================\n");
        invertedPageVMSim(numProcess,argv);
    }

    return(0);
}
