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
#include "DecodeStage.h"
#include "ExecuteStage.h"
#include "MemoryStage.h"
#include "Status.h"
#include "Debug.h"
#include "Instructions.h"

bool DecodeStage::doClockLow(PipeReg ** pregs, Stage ** stages)
{
   D * dreg = (D*)pregs[DREG];
   E * ereg = (E*)pregs[EREG];
   M * mreg = (M*)pregs[MREG];
   W * wreg = (W*)pregs[WREG];
   
   DecodeStage * dstage = (DecodeStage*)stages[DSTAGE];
   ExecuteStage * estage = (ExecuteStage*)stages[ESTAGE];
   MemoryStage * mstage = (MemoryStage*)stages[MSTAGE];

   uint64_t stat = dreg->getstat()->getOutput();
   uint64_t icode = dreg->geticode()->getOutput();
   uint64_t ifun = dreg->getifun()->getOutput();
   uint64_t valC = dreg->getvalC()->getOutput();
   uint64_t dstE = dstEComponent(icode, dreg->getrB()->getOutput());
   uint64_t dstM = dstMComponent(icode, dreg->getrA()->getOutput());
   srcA = srcAComponent(icode, dreg->getrA()->getOutput());
   srcB = srcBComponent(icode, dreg->getrB()->getOutput());

   //bool memError;

   //RegisterFile * regFile = RegisterFile::getInstance();
     
   //uint64_t valA = regFile->readRegister(srcA, memError);
   //uint64_t valB = regFile->readRegister(srcB, memError);
   
   //uint64_t valA = 0;
   //uint64_t valB = 0;

   //uint64_t valA = regFile->readRegister(dreg->getrA()->getOutput(), memError);
   //uint64_t valB = regFile->readRegister(dreg->getrB()->getOutput(), memError);

   uint64_t valA = valAComponent(dreg->getrA()->getOutput(), srcA, mreg, wreg, estage, mstage);
   uint64_t valB = valBComponent(dreg->getrB()->getOutput(), srcB, mreg, wreg, estage, mstage);
   
   uint64_t E_icode = ereg->geticode()->getOutput();
   uint64_t E_dstM = ereg->getdstM()->getOutput();
   uint64_t d_srcA = dstage->getdsrcA();
   uint64_t d_srcB = dstage->getdsrcB();
   E_bubble = calculateControlSignals(E_icode, E_dstM, d_srcA, d_srcB); 

   setEInput(ereg, stat, icode, ifun, valC, valA, valB, dstE, dstM, srcA, srcB);
   return false;
}

void DecodeStage::doClockHigh(PipeReg ** pregs)
{
  if (E_bubble) {
      bubbleE(pregs);
  } else {
      normalE(pregs);
  }
}

void DecodeStage::normalE(PipeReg ** pregs) {
   E * ereg = (E*) pregs[EREG];
   ereg->getstat()->normal();
   ereg->geticode()->normal();
   ereg->getifun()->normal();
   ereg->getvalC()->normal();
   ereg->getvalA()->normal();
   ereg->getvalB()->normal();
   ereg->getdstE()->normal();
   ereg->getdstM()->normal();
   ereg->getsrcA()->normal();
   ereg->getsrcB()->normal();
}

void DecodeStage::bubbleE(PipeReg ** pregs) {
   E * ereg = (E*) pregs[EREG];
   ereg->getstat()->bubble(SAOK);
   ereg->geticode()->bubble(INOP);
   ereg->getifun()->bubble();
   ereg->getvalC()->bubble();
   ereg->getvalA()->bubble();
   ereg->getvalB()->bubble();
   ereg->getdstE()->bubble(RNONE);
   ereg->getdstM()->bubble(RNONE);
   ereg->getsrcA()->bubble(RNONE);
   ereg->getsrcB()->bubble(RNONE);
}

void DecodeStage::setEInput(E * ereg, uint64_t stat, uint64_t icode, uint64_t ifun, 
                            uint64_t valC, uint64_t valA, uint64_t valB,
                            uint64_t dstE, uint64_t dstM,
                            uint64_t srcA, uint64_t srcB)
{

   ereg->getstat()->setInput(stat);
   ereg->geticode()->setInput(icode);
   ereg->getifun()->setInput(ifun);
   ereg->getvalC()->setInput(valC);
   ereg->getvalA()->setInput(valA);
   ereg->getvalB()->setInput(valB);
   ereg->getdstE()->setInput(dstE);
   ereg->getdstM()->setInput(dstM);
   ereg->getsrcA()->setInput(srcA);
   ereg->getsrcB()->setInput(srcB);

}

unsigned long DecodeStage::srcAComponent(unsigned long icode, unsigned long D_rA)
{
    if(icode == IRRMOVQ || icode == IRMMOVQ || icode == IOPQ || icode == IPUSHQ)
	    return D_rA;
    
    if(icode == IPOPQ || icode == IRET)
	    return RSP;

    return RNONE;
}

unsigned long DecodeStage::srcBComponent(unsigned long icode, unsigned long D_rB)
{
    if (icode == IOPQ || icode == IRMMOVQ || icode == IMRMOVQ)
	    return D_rB;

    if (icode == IPUSHQ || icode == IPOPQ || icode == ICALL || icode == IRET)
	    return RSP;

    return RNONE;
}

unsigned long DecodeStage::dstEComponent(uint64_t icode, uint64_t rB) {

    if (icode == IRRMOVQ || icode == IIRMOVQ || icode == IOPQ) return rB;
    if (icode == IPUSHQ || icode == IPOPQ || icode == ICALL || icode == IRET) return RSP;
    return RNONE;

}

unsigned long DecodeStage::dstMComponent(uint64_t icode, uint64_t rA) {
    if (icode == IMRMOVQ || icode == IPOPQ) return rA;
    return RNONE;
}

uint64_t DecodeStage::valAComponent(uint64_t rvalA, uint64_t d_srcA, M * mreg, W * wreg, Stage * e, Stage * m) {

    uint64_t M_dstE = mreg->getdstE()->getOutput();
    uint64_t M_valE = mreg->getvalE()->getOutput();

    uint64_t M_dstM = mreg->getdstM()->getOutput();

    uint64_t W_dstE = wreg->getdstE()->getOutput();
    uint64_t W_valE = wreg->getvalE()->getOutput();

    uint64_t W_dstM = wreg->getdstM()->getOutput();
    uint64_t W_valM = wreg->getvalM()->getOutput();

    ExecuteStage * estage = (ExecuteStage *) e;
    MemoryStage * mstage = (MemoryStage *) m;

    uint64_t m_valM = mstage->getvalM();

    uint64_t e_dstE = estage->e_dstE();
    uint64_t e_valE = estage->e_valE();

    if (d_srcA == RNONE) return 0;
    if (d_srcA == e_dstE) return e_valE;
    if (d_srcA == M_dstM) return m_valM;
    if (d_srcA == M_dstE) return M_valE;
    if (d_srcA == W_dstM) return W_valM;
    if (d_srcA == W_dstE) return W_valE;

    bool memError;
    RegisterFile * regFile = RegisterFile::getInstance();
    uint64_t ret = regFile->readRegister(d_srcA, memError);
    return ret;

}

uint64_t DecodeStage::valBComponent(uint64_t rvalB, uint64_t d_srcB, M * mreg, W * wreg, Stage * e, Stage * m) {
   
    uint64_t M_dstE = mreg->getdstE()->getOutput();
    uint64_t M_valE = mreg->getvalE()->getOutput();

    uint64_t M_dstM = mreg->getdstM()->getOutput();
    //uint64_t m_valM = mreg->getvalM()->getOutput();

    uint64_t W_dstE = wreg->getdstE()->getOutput();
    uint64_t W_valE = wreg->getvalE()->getOutput();

    uint64_t W_dstM = wreg->getdstM()->getOutput();
    uint64_t W_valM = wreg->getvalM()->getOutput();

    ExecuteStage * estage = (ExecuteStage *) e;
    MemoryStage * mstage = (MemoryStage *) m;

    uint64_t m_valM = mstage->getvalM();

    uint64_t e_dstE = estage->e_dstE();
    uint64_t e_valE = estage->e_valE();

    if (d_srcB == RNONE) return 0;
    if (d_srcB == e_dstE) return e_valE;
    if (d_srcB == M_dstM) return m_valM;
    if (d_srcB == M_dstE) return M_valE;
    if (d_srcB == W_dstM) return W_valM;
    if (d_srcB == W_dstE) return W_valE;

    bool memError;
    RegisterFile * regFile = RegisterFile::getInstance();
    uint64_t ret = regFile->readRegister(d_srcB, memError);
    return ret;

}

uint64_t DecodeStage::getdsrcA() {
    return srcA;
}

uint64_t DecodeStage::getdsrcB() {
    return srcB;
}

bool DecodeStage::calculateControlSignals(uint64_t E_icode, uint64_t E_dstM, uint64_t d_srcA, uint64_t d_srcB) {
    if ((E_icode == IMRMOVQ || E_icode == IPOPQ) 
        && (E_dstM == d_srcA || E_dstM == d_srcB))
        return true;
    return false;
}
