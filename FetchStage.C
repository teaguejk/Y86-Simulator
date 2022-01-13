#include <string>
#include <cstdint>
#include "RegisterFile.h"
#include "PipeRegField.h"
#include "PipeReg.h"
#include "F.h"
#include "D.h"
#include "E.h"
#include "M.h"
#include "W.h"
#include "Stage.h"
#include "FetchStage.h"
#include "DecodeStage.h"
#include "ExecuteStage.h"
#include "Status.h"
#include "Debug.h"
#include "Instructions.h"
#include "Memory.h"
#include "Tools.h"

#define IFUN_START 0
#define IFUN_END 3
#define ICODE_START 4
#define ICODE_END 7

#define RA_START 4
#define RA_END 7
#define RB_START 0
#define RB_END 3

/*
 * doClockLow:
 * Performs the Fetch stage combinational logic that is performed when
 * the clock edge is low.
 *
 * @param: pregs - array of the pipeline register sets (F, D, E, M, W instances)
 * @param: stages - array of stages (FetchStage, DecodeStage, ExecuteStage,
 *         MemoryStage, WritebackStage instances)
 */
bool FetchStage::doClockLow(PipeReg ** pregs, Stage ** stages)
{
   F * freg = (F*)pregs[FREG];
   D * dreg = (D*)pregs[DREG];
   E * ereg = (E*)pregs[EREG];
   M * mreg = (M*)pregs[MREG];
   W * wreg = (W*)pregs[WREG];
   uint64_t f_pc = 0, icode = 0, ifun = 0, valC = 0, valP = 0;
   uint64_t rA = RNONE, rB = RNONE, stat = SAOK;

   //code missing here to select the value of the PC
   //and fetch the instruction from memory
   //Fetching the instruction will allow the icode, ifun,
   //rA, rB, and valC to be set.
   //The lab assignment describes what methods need to be
   //written.
   f_pc = FetchStage::selectPC(freg, mreg, wreg);

   Memory * mem = Memory::getInstance();
   bool mem_error;
   uint8_t  i_byte = mem->getByte(f_pc, mem_error);
   ifun = Tools::getBits(i_byte, IFUN_START, IFUN_END);
   icode = Tools::getBits(i_byte, ICODE_START, ICODE_END);
   
   ifun = f_ifun(mem_error, ifun);
   icode = f_icode(mem_error, icode);

   stat = f_stat(mem_error, instr_valid(icode), icode);

   DecodeStage * dstage = (DecodeStage*)stages[DSTAGE];
   ExecuteStage * estage = (ExecuteStage*)stages[ESTAGE];
   uint64_t E_icode = ereg->geticode()->getOutput();
   uint64_t E_dstM = ereg->getdstM()->getOutput();
   uint64_t d_srcA = dstage->getdsrcA();
   uint64_t d_srcB = dstage->getdsrcB();
   uint64_t e_Cnd = estage->geteCnd();
   F_stall = fetch_stall(E_icode, E_dstM, d_srcA, d_srcB);
   D_stall = decode_stall(E_icode, E_dstM, d_srcA, d_srcB);
   D_bubble = decode_bubble(E_icode, e_Cnd);


   if (needsRegIds(icode)) {
       getRegIds(f_pc, &rA, &rB);
   }

   if (needsValC(icode)) {
       buildValC(f_pc, &valC, needsRegIds(icode));
   }

   valP = PCincrement(f_pc, needsRegIds(icode), needsValC(icode));
   uint64_t predictedPC = predictPC(icode, valC, valP);
   //The value passed to setInput below will need to be changed
   freg->getpredPC()->setInput(predictedPC);

   //provide the input values for the D register
   setDInput(dreg, stat, icode, ifun, rA, rB, valC, valP);
   return false;
}

/* doClockHigh
 * applies the appropriate control signal to the F
 * and D register intances
 *
 * @param: pregs - array of the pipeline register (F, D, E, M, W instances)
 */
void FetchStage::doClockHigh(PipeReg ** pregs)
{
   F * freg = (F *) pregs[FREG];


   if (D_bubble) {
    bubbleD(pregs);
   }

   if (!D_stall) {
    normalD(pregs);
   }

   if (!F_stall) { 
    freg->getpredPC()->normal();
   }
}

void FetchStage::bubbleD(PipeReg ** pregs) {
    D * dreg = (D *) pregs[DREG];
    dreg->getstat()->bubble(SAOK);
    dreg->geticode()->bubble(INOP);
    dreg->getifun()->bubble();
    dreg->getrA()->bubble(RNONE);
    dreg->getrB()->bubble(RNONE);
    dreg->getvalC()->bubble();
    dreg->getvalP()->bubble();

}

void FetchStage::normalD(PipeReg ** pregs) {
    D * dreg = (D *) pregs[DREG];
    dreg->getstat()->normal();
    dreg->geticode()->normal();
    dreg->getifun()->normal();
    dreg->getrA()->normal();
    dreg->getrB()->normal();
    dreg->getvalC()->normal();
    dreg->getvalP()->normal();
}


/* setDInput
 * provides the input to potentially be stored in the D register
 * during doClockHigh
 *
 * @param: dreg - pointer to the D register instance
 * @param: stat - value to be stored in the stat pipeline register within D
 * @param: icode - value to be stored in the icode pipeline register within D
 * @param: ifun - value to be stored in the ifun pipeline register within D
 * @param: rA - value to be stored in the rA pipeline register within D
 * @param: rB - value to be stored in the rB pipeline register within D
 * @param: valC - value to be stored in the valC pipeline register within D
 * @param: valP - value to be stored in the valP pipeline register within D
*/
void FetchStage::setDInput(D * dreg, uint64_t stat, uint64_t icode, 
                           uint64_t ifun, uint64_t rA, uint64_t rB,
                           uint64_t valC, uint64_t valP)
{
   dreg->getstat()->setInput(stat);
   dreg->geticode()->setInput(icode);
   dreg->getifun()->setInput(ifun);
   dreg->getrA()->setInput(rA);
   dreg->getrB()->setInput(rB);
   dreg->getvalC()->setInput(valC);
   dreg->getvalP()->setInput(valP);
}
     
uint64_t FetchStage::selectPC(F * freg, M * mreg, W * wreg) {
    uint64_t m_icode = mreg->geticode()->getOutput();
    uint64_t m_Cnd = mreg->getCnd()->getOutput();
    uint64_t w_icode = wreg->geticode()->getOutput();

    if (w_icode == IRET) {
        return wreg->getvalM()->getOutput();
    }

    if (m_icode == IJXX && !m_Cnd) {
        return mreg->getvalA()->getOutput();
    }

    return freg->getpredPC()->getOutput();
}

bool FetchStage::needsRegIds(uint64_t icode) {
    return (icode == IOPQ || icode == IPOPQ || icode == IPUSHQ || icode == IRRMOVQ || icode == IRMMOVQ || icode == IIRMOVQ || icode == IMRMOVQ);
}

bool FetchStage::needsValC(uint64_t icode) {
    return (icode == IRMMOVQ || icode == IIRMOVQ || icode == IMRMOVQ || icode == IJXX || icode == ICALL);
}

/**
unsigned int FetchStage::getvalP(unsigned int pc, unsigned int icode)
{
    
    switch(icode)
    {
	case IHALT:
	case INOP:
	case IRET:
		return pc + 1;

	case IRRMOVQ:
	case IPUSHQ:
	case IPOPQ:
		return pc + 2;

	case IIRMOVQ:
	case IRMMOVQ:
	case IMRMOVQ:
		return pc + 6;
	
	default: return pc + 1;
    }
}
**/

uint64_t FetchStage::predictPC(uint64_t f_icode, uint64_t f_valC, uint64_t f_valP)
{
    
    if (f_icode == IJXX || f_icode == ICALL)
	{
        return f_valC;
    } 
    else 
    {
        return f_valP;
    }
    //return getvalP(f_valP, f_icode);
}

uint64_t FetchStage::PCincrement(int32_t f_pc, bool needsRegIds, bool needsValC) {
    if (needsValC) {
        f_pc += 8;
    }

    if (needsRegIds) {
        f_pc += 1;
    }

    f_pc += 1;
    return f_pc;
}

void FetchStage::getRegIds(uint64_t f_pc, uint64_t * rA, uint64_t * rB) { 
   bool mem_error;
   Memory * mem = Memory::getInstance(); 
   uint8_t registerByte = mem->getByte(f_pc + 1, mem_error);
   *rA = Tools::getBits(registerByte, RA_START, RA_END); 
   *rB = Tools::getBits(registerByte, RB_START, RB_END); 
}

void FetchStage::buildValC(uint64_t f_pc, uint64_t * valC, bool needsRegIds) {

   int start = f_pc + 1;

   //if it has regs in the encoding, skip over
   if (needsRegIds) start += 1;

   //holds the 8 bytes
   uint8_t val[8];

   Memory * m = Memory::getInstance(); 
   bool mem_error;
   
   //get bytes
   for (int i = 0; i < 8; i++) {
       val[i] = m->getByte(start+i, mem_error);
   }

   //combine into long
   *valC = Tools::buildLong(val);
   
   //*valC = m->getLong(start, mem_error);
}

bool FetchStage::instr_valid(uint64_t f_icode) {

    if (f_icode == INOP || f_icode == IHALT || f_icode == IRRMOVQ || f_icode == IIRMOVQ || f_icode == IRMMOVQ 
        || f_icode == IMRMOVQ || f_icode == IOPQ || f_icode == IJXX || f_icode == ICALL || f_icode == IRET
        || f_icode == IPUSHQ || f_icode == IPOPQ)
        return true;
    return false;

}

uint64_t FetchStage::f_stat(bool mem_error, bool valid, uint64_t f_icode) {
    if (mem_error) return SADR;
    if (!valid) return SINS;
    if (f_icode == IHALT) return SHLT;
    return SAOK;

}

uint64_t FetchStage::f_icode(bool mem_error, uint64_t mem_icode) {
    if (mem_error) return INOP;
    return mem_icode;
}

uint64_t FetchStage::f_ifun(bool mem_error, uint64_t mem_ifun) {
    if (mem_error) return FNONE;
    return mem_ifun;
}

bool FetchStage::fetch_stall(uint64_t E_icode, uint64_t E_dstM, uint64_t d_srcA, uint64_t d_srcB) {
    if ((E_icode == IMRMOVQ || E_icode == IPOPQ) 
        && (E_dstM == d_srcA || E_dstM == d_srcB))
            return true;
    return false;
}

bool FetchStage::decode_stall(uint64_t E_icode, uint64_t E_dstM, uint64_t d_srcA, uint64_t d_srcB) {
    if ((E_icode == IMRMOVQ || E_icode == IPOPQ) 
        && (E_dstM == d_srcA || E_dstM == d_srcB))
            return true;
    return false;
}

bool FetchStage::decode_bubble(uint64_t E_icode, uint64_t e_Cnd) {
    if ((E_icode == IJXX) && !e_Cnd) 
        return true;
    return false;
}

void calculateControlSignals(bool fstall, bool dstall) {

}

