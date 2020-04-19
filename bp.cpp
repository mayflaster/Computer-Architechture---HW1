/* 046267 Computer Architecture - Winter 2019/20 - HW #1 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <cmath>
#include <cstddef>
#include <iostream>
using namespace std;

#define EMPTY -1
#define NOT_USING_SHARE 0
#define SHARE_LSB 1
#define SHARE_MID 2
#define MAX_32UNSIGNED 4294967295

enum state {NT,WNT,WT,T};

/*
gets the current state & T/NT. calculates the next state and returns it 
*/

enum state  update_state (enum state current, bool taken){
    switch (current){
        case T:
            if (taken) return T;
            else return WT;
            break;
        case WT:
            if (taken) return T;
            else return WNT;
            break;
        case WNT:
            if (taken) return WT;
            else return NT;
            break;
        case NT:
            if (taken) return WNT;
            else return NT;
            break;
        default:
            return WNT;

    }
}

/*
get the current history array of certain tag and add to it the new occation
*/
void updateHistArr (bool* histArr, unsigned histSize, bool current){
    for (unsigned i = histSize-1; i > 0; i--) {
        histArr[i] = histArr[i-1];
    }
    histArr[0]=current;
}


/*
get pc and returns the n lsb numbers of it (the unsigned numbers)
*/
static  unsigned getLSBpc (uint32_t pc, unsigned n){
    unsigned temp = pc;
    temp = temp >> 2;
    temp =temp << (32-n);
    temp = temp >>(32-n);
    return temp;
}
/*
get pc and returns the n mid numbers of it (the unsigned numbers)
*/
static unsigned geTMIDpc (uint32_t pc, unsigned n){
    unsigned temp = pc;
    temp =temp >> 16;
    temp =temp << (32-n);
    temp =temp >>(32-n);
    return temp;
}

/*
get a boolean history array and convert it to the unsigned nuber it represent
*/
static unsigned  calculateHistory ( bool * historyArr, unsigned historySize){
    unsigned  history =0;
    for (unsigned i = 0; i < historySize; ++i) {
        history += (historyArr[i]*pow(2,i));
    }
    return history;
}

class Btb {
public:
    //unsigned ** btbTable;
    unsigned btbSize;
    unsigned fsmState;
    unsigned historySize;
    unsigned tagSize;
    unsigned * tagTable;
    unsigned  * targetTable;
    int isShared;

    Btb(unsigned btbSize,unsigned historySize, unsigned tagSize, unsigned fsmState,int isShared): btbSize(btbSize),fsmState(fsmState),
    historySize(historySize), tagSize(tagSize),tagTable(new unsigned[btbSize]),
    targetTable(new unsigned [btbSize]),  isShared(isShared) {
        for (unsigned i = 0; i < btbSize ; ++i) {
            tagTable [i] = MAX_32UNSIGNED;
            targetTable[i] = MAX_32UNSIGNED;
        }
    }
/*
get pc and returns tag/set of it (calculated with tag size and btb size of the btb) 
*/

    unsigned  getTag (uint32_t pc){
        unsigned  temp = pc;
        temp = temp >> 2;
        temp = temp << (32-tagSize);
        temp = temp >> (32-tagSize);
        return temp;
    }

    unsigned getSet (uint32_t pc){
        unsigned setSize = (int)log2(btbSize);
        unsigned temp = pc;
        temp = temp >> 2 ;
        temp =temp << (32-setSize);
        temp = temp >> (32-setSize);
        return temp;
    }
};


class Btb_LL: public Btb {
public:
    //unsigned * tagTable;
    bool  ** histTable;
    //unsigned  * targetTable;
     state ** localFsmArr;


    Btb_LL(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,int Shared): Btb(btbSize,historySize,
                                                                                                        tagSize,fsmState,Shared),histTable(new bool* [btbSize]), localFsmArr (new  state* [btbSize]){
        //localFsmArr = new  (enum state)* [btbSize];
        unsigned  size = (unsigned)pow(2,historySize);
        for (unsigned i = 0; i <btbSize ; ++i) {
            localFsmArr[i]= new state [size];
            histTable [i] = new bool [historySize];
            for (unsigned j = 0; j < (unsigned)pow(2,historySize) ; ++j) {
                localFsmArr[i][j] = (enum state) fsmState;
            }
            for (unsigned k = 0; k < historySize; ++k) {
                histTable[i][k] = false;
            }
        }
    }

    ~Btb_LL(){
        for (unsigned i = 0; i < btbSize ; ++i) {
            delete  [] localFsmArr[i];
            delete [] histTable[i];
        }
        delete [] localFsmArr;
        //delete [] tagTable;
        delete [] histTable;
        //delete []targetTable;
    }


    bool BP_predict_LL(uint32_t pc, uint32_t *dst){
        state prediction;
        unsigned calculatedhist;
        unsigned place = getSet(pc);
        unsigned tag = getTag(pc);
        if(tag==this->tagTable[place]) // check if this branch already exist in btb
        {
            calculatedhist = calculateHistory(this->histTable[place],
                                              this->historySize);
            //if (calculatedhist != EMPTY) {
            prediction = this->localFsmArr[place][calculatedhist];
            if (prediction == WT || prediction == T) {
                *dst = targetTable[place];
                return true;
            }
        }
        *dst = pc+(uint32_t)(4);
        return false;


    }

    void BP_update_LL(uint32_t pc, uint32_t targetPc, bool taken){
        //case 1 - br not exist
        //case 2 - br exist
       // state prediction = WNT; //default initialize
        unsigned calculatedhist = 0;
        unsigned place = getSet(pc);
        unsigned tag = getTag(pc);
        if(tag==this->tagTable[place]){// check if this branch already exist in btb

            calculatedhist = calculateHistory(this->histTable[place],
                                              this->historySize);
            this->targetTable[place] = targetPc;
            localFsmArr[place][calculatedhist] = update_state(localFsmArr[place][calculatedhist], taken);
            updateHistArr(histTable[place], historySize, taken);
        }
        else {
            this->tagTable[place] = tag;
            this->targetTable[place] = targetPc;
            for (unsigned i=0; i<(unsigned)pow(2,historySize); i++) {
                localFsmArr[place][i] = (enum state) fsmState;
            }//reset the local fsm arr
            for (unsigned i=0; i<historySize; i++) {
                histTable[place][i] = false;
            }//reset the history arr
            calculatedhist = calculateHistory(histTable[place], this->historySize);
            localFsmArr[place][calculatedhist] = update_state(localFsmArr[place][calculatedhist], taken);
            updateHistArr(histTable[place], historySize, taken);





        }
    }

};

class Btb_GG: public  Btb {
public:
    //unsigned * tagTable;
    //unsigned  * targetTable;
    bool *   hist;
    enum state * globalFsmArr;

    Btb_GG(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,int Shared): Btb(btbSize,historySize,tagSize,fsmState,Shared),
                                                                                                    hist(new bool [historySize]){
        unsigned size = (unsigned)pow (2,historySize);
        globalFsmArr = new enum state [size];
        for (unsigned i = 0; i <(unsigned) pow (2,historySize); ++i) {
            globalFsmArr[i]= (enum state) fsmState;
        }
        for (unsigned j = 0; j <historySize ; ++j) {
            hist[j] =0 ;
        }
    }

    ~Btb_GG(){
        delete globalFsmArr;
//delete [] tagTable;
//delete []targetTable;
    }

    bool BP_predict_GG (uint32_t pc, uint32_t *dst){
        unsigned  tag = getTag(pc);
        unsigned place = getSet(pc);
        unsigned history = calculateHistory (hist, historySize);
        if (tagTable[place] == tag){
            if (isShared == NOT_USING_SHARE){
                if (globalFsmArr[history] == T ||globalFsmArr[history] == WT){
                    *dst = targetTable[place];
                    return true;
                }
            }
            else if (isShared == SHARE_LSB){
                unsigned lsbHist = history^getLSBpc(pc,historySize);
                if (globalFsmArr[lsbHist] == T ||globalFsmArr[lsbHist] == WT){
                    *dst = targetTable[place];
                    return true;
                }
            }
            else if (isShared == SHARE_MID){
                unsigned midHist = history^geTMIDpc(pc,historySize);
                if (globalFsmArr[midHist] == T || globalFsmArr[midHist] == WT){
                    *dst = targetTable[place];
                    return true;
                }
            }
        }
        *dst = (pc+4); // if we predict NT we got here
        return false;
    }

    void BP_update_GG(uint32_t pc, uint32_t targetPc, bool taken){
        unsigned  tag = getTag(pc);
        unsigned place = getSet(pc);
        unsigned history = calculateHistory (hist, historySize);
        unsigned  stateMachinePlace = history;
        if (isShared == SHARE_LSB){
            stateMachinePlace = history^getLSBpc(pc,historySize);
        }
        else if (isShared == SHARE_MID){
            stateMachinePlace= history^geTMIDpc(pc,historySize);
        }
        enum state next = update_state(globalFsmArr[stateMachinePlace],taken);
        globalFsmArr[stateMachinePlace] = next;
        updateHistArr(hist,historySize,taken);
        targetTable[place]=targetPc;
        //case1  - br exist
        if (tagTable[place] != tag){
            tagTable[place] = tag;
        }
    }

};


class  Btb_LG : public Btb{
public:

    bool  ** histTable;
     state * globalFsmArr;
    //shared?
    Btb_LG (unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,int Shared):
            Btb(btbSize,historySize,tagSize,fsmState,Shared), histTable(new bool* [btbSize]) {
        unsigned size = (unsigned)pow (2,historySize);
        globalFsmArr = new  state [size];

        for (int i = 0; i < pow (2,historySize); ++i) {
            globalFsmArr[i]= ( state) fsmState;
        }
        for (unsigned k = 0; k < btbSize; ++k) {
            histTable[k] = new bool [historySize];
            for (unsigned j = 0; j <historySize ; ++j) {
                histTable[k][j] = 0 ;
            }
        }

    }

    ~Btb_LG() {
        delete[] globalFsmArr;
        for (unsigned i = 0; i < btbSize ; ++i) {
            delete [] histTable[i];
        }
        delete[] histTable;
    }


    bool BP_predict_LG (uint32_t pc, uint32_t *dst){
        unsigned  tag = this->getTag(pc);
        unsigned place = this->getSet(pc);
        unsigned history = calculateHistory (this->histTable[place], this->historySize);
        state prediction = NT;
        if(tag==this->tagTable[place]) // check if this branch already exist in btb
        {
            if (isShared == NOT_USING_SHARE){
                prediction = globalFsmArr[history];

            }
            else if (isShared == SHARE_LSB){
                prediction = this->globalFsmArr[getLSBpc(pc,historySize)^history];

            }
            else if (isShared == SHARE_MID)
                prediction = this->globalFsmArr[geTMIDpc(pc,historySize)^history];

        }
        if (prediction == WT || prediction == T) {
            *dst = targetTable[place];
            return true;
        }else {
            *dst = pc + (uint32_t) (4);
            return false;
        }
    }


    void BP_update_LG(uint32_t pc, uint32_t targetPc, bool taken){
        //cout << "taken ?" << taken<< endl;
        unsigned  tag = this->getTag(pc);
        unsigned place = this->getSet(pc);
        if (tagTable[place] != tag){
            for (unsigned i = 0; i < historySize; ++i) {
                histTable[place][i]= false;
            }
            tagTable[place] = tag;
        }
        unsigned history = calculateHistory (this->histTable[place], this->historySize);
        unsigned  stateMachinePlace = history;
        if (isShared == SHARE_LSB){
            stateMachinePlace = history^getLSBpc(pc,historySize);
        }
        else if (isShared == SHARE_MID){
            stateMachinePlace = history^geTMIDpc(pc,historySize);
        }
        state next = update_state(globalFsmArr[stateMachinePlace],taken);
        targetTable[place]=targetPc;
        globalFsmArr[stateMachinePlace]=next;
        updateHistArr(histTable[place],historySize,taken);
    }


};


class Btb_GL: public Btb {
public:
    //unsigned * tagTable;
    bool  *histTable;
    //unsigned  * targetTable;
    enum state ** localFsmArr;


    Btb_GL(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,int Shared): Btb(btbSize,historySize,
                                                                                                        tagSize,fsmState,Shared),histTable(new bool [historySize]), localFsmArr (new  state* [btbSize]){
        //localFsmArr = new  (enum state)* [btbSize];
        unsigned size = (unsigned)pow (2,historySize);
        for (unsigned i = 0; i <btbSize ; ++i) {
            localFsmArr[i]= new enum state [size];
            for (int j = 0; j < pow(2,historySize) ; ++j) {
                localFsmArr[i][j] = (enum state) fsmState;
            }
            for (unsigned k = 0; k < historySize; ++k) {
                histTable[k] = 0;
            }
        }
    }

    ~Btb_GL(){
        for (unsigned i = 0; i < btbSize ; ++i) {
            delete  [] localFsmArr[i];
        }
        delete [] localFsmArr;
        //delete [] tagTable;
        delete [] histTable;
        //delete []targetTable;
    }


    bool BP_predict_GL (uint32_t pc, uint32_t *dst){
        unsigned  tag = getTag(pc);
        unsigned place = getSet(pc);
        unsigned history = calculateHistory (histTable, historySize);
        if (tagTable[place] == tag ){
            if ((localFsmArr[place][history] == WT || localFsmArr[place][history] == T)){
                *dst = targetTable[place];
                return true;
            }
        }
        *dst = (pc+4); // if we predict NT we got here
        return false;
    }

    void BP_update_GL(uint32_t pc, uint32_t targetPc, bool taken){
        //case 1 - br not exist
        //case 2 - br exist
        //state prediction;
        unsigned calculatedhist=0;
        unsigned place = getSet(pc);
        unsigned tag = getTag(pc);
        if(tag==this->tagTable[place]){// check if this branch already exist in btb

            calculatedhist = calculateHistory(histTable,
                                              historySize);
            this->targetTable[place] = targetPc;
            localFsmArr[place][calculatedhist] = update_state(localFsmArr[place][calculatedhist], taken);
            updateHistArr(histTable, historySize, taken);
        }
        else {
            this->tagTable[place] = tag;
            this->targetTable[place] = targetPc;
            for (int i=0; i<pow(2,historySize); i++) {
                localFsmArr[place][i] = (enum state) fsmState;
            }//reset the local fsm arr
            calculatedhist = calculateHistory(histTable, historySize);
            localFsmArr[place][calculatedhist] = update_state(localFsmArr[place][calculatedhist], taken);
            updateHistArr(histTable, historySize, taken);





        }
    }

};



//global
Btb_GG * btbGg = NULL;
Btb_GL * btbGl = NULL;
Btb_LG * btbLg = NULL;
Btb_LL * btbLl = NULL;

unsigned brCounter =0;
unsigned  flushCounter =0 ;


int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
            bool isGlobalHist, bool isGlobalTable, int Shared){
    if (isGlobalHist && isGlobalTable ){ // global global
       Btb_GG * btbGG1 = new Btb_GG ( btbSize,  historySize,  tagSize,  fsmState, Shared);
       if (btbGG1==NULL){
           return -1;
       }
       btbGg = btbGG1;
    }
    if (isGlobalHist && !isGlobalTable ){ //global hist local fsm
        Btb_GL * btbGL1 = new Btb_GL ( btbSize,  historySize,  tagSize,  fsmState, Shared);
        if (btbGL1==NULL){
            return -1;
        }
        btbGl = btbGL1;

    }

     if (!isGlobalHist && !isGlobalTable ){ //local hist local fsm
        Btb_LL * btbLL1 = new Btb_LL ( btbSize,  historySize,  tagSize,  fsmState, Shared);
         if (btbLL1==NULL){
             return -1;
         }
        btbLl = btbLL1;
    }
      if (!isGlobalHist && isGlobalTable ){ //local hist global fsm
        Btb_LG * btbLG1 = new Btb_LG ( btbSize,  historySize,  tagSize,  fsmState, Shared);
          if (btbLG1==NULL){
              return -1;
          }
        btbLg = btbLG1;
    }
    return 0;
}


bool BP_predict(uint32_t pc, uint32_t *dst){
    if (btbGl)
        return btbGl->BP_predict_GL(pc,dst);
    else if (btbLg)
        return btbLg->BP_predict_LG(pc,dst);
    else if (btbGg)
        return btbGg->BP_predict_GG(pc,dst);
    else
        return btbLl-> BP_predict_LL(pc,dst);
    return false;
}











void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){

    brCounter ++;
    if ((targetPc != pred_dst && taken== true )|| (taken == false && pred_dst != (pc+4)) )
         flushCounter++;
    if (btbGg){
        btbGg->BP_update_GG(pc,targetPc,taken);
    } else if (btbLl){
        btbLl->BP_update_LL(pc,targetPc,taken);
    }
    else if(btbLg) {
        btbLg->BP_update_LG(pc,targetPc,taken);
    }
    else if(btbGl){
        btbGl->BP_update_GL(pc,targetPc,taken);
    }
}

void BP_GetStats(SIM_stats *curStats){
    curStats->br_num = brCounter;
    curStats->flush_num = flushCounter;
    unsigned  memorySize = 0;
    memorySize = 0;
    if (btbGg){
        memorySize += (btbGg ->btbSize)*(btbGg->tagSize+30); //same for all the tyoes
        memorySize +=( (btbGg->historySize)+2*pow(2,btbGg->historySize));
        delete btbGg;
    }
    else if (btbGl){
        memorySize += (btbGl ->btbSize)*(btbGl->tagSize+30);
        memorySize +=( (btbGl->historySize)+2*(btbGl->btbSize)*pow(2,btbGl->historySize));
        delete btbGl;
    }
    else if (btbLg){
        memorySize += (btbLg ->btbSize)*(btbLg->tagSize+30);
        memorySize += ((btbLg->btbSize)*(btbLg->historySize)+2*pow(2,btbLg->historySize));
        delete btbLg;
    }
    else { //local local
        memorySize += (btbLl->btbSize)*(btbLl->tagSize+30);
        unsigned x = pow(2,btbLl->historySize);
        memorySize += ((btbLl->btbSize)*(btbLl->historySize) +2*(btbLl->btbSize)*x);
        delete btbLl;
    }
    curStats->size=memorySize;
    return;
}
