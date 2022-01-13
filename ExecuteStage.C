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
#include "ExecuteStage.h"
#include "MemoryStage.h"
#include "Status.h"
#include "Debug.h"
#include "Instructions.h"
#include "Tools.h"
#include "ConditionCodes.h"


bool ExecuteStage::doClockLow(PipeReg ** pregs, Stage ** stages)
{
    E * ereg = (E*)pregs[EREG];
    M * mreg = (M*)pregs[MREG];
    W * wreg = (W*)pregs[WREG];

    uint64_t stat = ereg->getstat()->getOutput();
    uint64_t icode = ereg->geticode()->getOutput();
    uint64_t ifun = ereg->getifun()->getOutput();
    valE = ereg->getvalC()->getOutput();
    uint64_t valA = ereg->getvalA()->getOutput();
    dstE = ereg->getdstE()->getOutput();
    uint64_t dstM = ereg->getdstM()->getOutput();
    uint64_t valB = ereg->getvalB()->getOutput();
    uint64_t valC = ereg->getvalC()->getOutput();

    uint64_t e_aluA = aluA(icode, valA, valC);
    uint64_t e_aluB = aluB(icode, valB);
    uint64_t e_alufun = alufun(icode, ereg);

    MemoryStage * mstage = (MemoryStage*)stages[MSTAGE]; 
 
    uint64_t m_stat = mstage->getm_stat();
    uint64_t W_stat = wreg->getstat()->getOutput();

    M_bubble = calculateControlSignals(m_stat, W_stat);

    valE = alu(e_alufun, e_aluA, e_aluB, set_cc(icode, m_stat, W_stat));

    cnd = cond(icode, ifun);
    dstE = edstE(icode, cnd, dstE);

    setMInput(mreg, stat, icode, cnd, valE, valA, dstE, dstM);
    return false;
}

void ExecuteStage::doClockHigh(PipeReg ** pregs)
{
   //M * mreg = (M*)pregs[MREG];

   if (M_bubble) {
     bubbleM(pregs);
   } else {
     normalM(pregs);
   }
}

void ExecuteStage::bubbleM(PipeReg ** pregs) {
   M * mreg = (M*)pregs[MREG];
   mreg->getstat()->bubble(SAOK);
   mreg->geticode()->bubble(INOP);
   mreg->getCnd()->bubble();
   mreg->getvalE()->bubble();
   mreg->getvalA()->bubble();
   mreg->getdstE()->bubble(RNONE);
   mreg->getdstM()->bubble(RNONE);
}

void ExecuteStage::normalM(PipeReg ** pregs) {
   M * mreg = (M*)pregs[MREG];
   mreg->getstat()->normal();
   mreg->geticode()->normal();
   mreg->getCnd()->normal();
   mreg->getvalE()->normal();
   mreg->getvalA()->normal();
   mreg->getdstE()->normal();
   mreg->getdstM()->normal();
}


void ExecuteStage::setMInput(M * mreg, uint64_t stat, uint64_t icode, uint64_t Cnd,
               uint64_t valE, uint64_t valA,
               uint64_t dstE, uint64_t dstM)
{
   mreg->getstat()->setInput(stat);
   mreg->geticode()->setInput(icode);
   mreg->getCnd()->setInput(Cnd);
   mreg->getvalE()->setInput(valE);
   mreg->getvalA()->setInput(valA);
   mreg->getdstE()->setInput(dstE);
   mreg->getdstM()->setInput(dstM);

}

uint64_t ExecuteStage::aluA(uint64_t icode, uint64_t E_valA, uint64_t E_valC) {
    //uint64_t E_valA = ereg->getvalA()->getOutput();
    //uint64_t E_valC = ereg->getvalC()->getOutput();

    if (icode == IRRMOVQ || icode == IOPQ) return E_valA;
    if (icode == IIRMOVQ || icode == IRMMOVQ || icode == IMRMOVQ) return E_valC;
    if (icode == ICALL || icode == IPUSHQ) return -8;
    if (icode == IRET || icode == IPOPQ) return 8;
    return 0;
}

uint64_t ExecuteStage::aluB(uint64_t icode, uint64_t E_valB) {
    //uint64_t E_valB = ereg->getvalB()->getOutput();

    if (icode == IRMMOVQ || icode == IMRMOVQ || icode == IOPQ || icode == ICALL
        || icode == IPUSHQ || icode == IRET || icode == IPOPQ) return E_valB;
    if (icode == IRRMOVQ || icode == IIRMOVQ) return 0;
    return 0;
}

uint64_t ExecuteStage::alufun(uint64_t E_icode, E* ereg)
{
    if(E_icode == IOPQ) return ereg->getifun()->getOutput();
    return ADDQ;
}

bool ExecuteStage::set_cc (uint64_t E_icode, uint64_t m_stat, uint64_t W_stat)
{
    if ((E_icode == IOPQ)
        && (m_stat != SADR && m_stat != SINS && m_stat != SHLT) 
        && (W_stat != SADR && W_stat != SINS && W_stat != SHLT))
            return true;
    return false;
}

uint64_t ExecuteStage::edstE(uint64_t E_icode, uint64_t e_Cnd, uint64_t E_dstE)
{
    if (E_icode == IRRMOVQ && !e_Cnd) return RNONE;
    return  E_dstE;
}

uint64_t ExecuteStage::alu(uint64_t ifun, uint64_t aluA, uint64_t aluB, bool setCC) {
    uint64_t ret = 0;
    bool of = false;
    bool zf = false;
    bool sf = false;

    if (ifun == ADDQ) {
       of = Tools::addOverflow(aluA, aluB);
       ret = aluA + aluB;
    }
    if (ifun == SUBQ) {
       of = Tools::subOverflow(aluA, aluB);
       ret = aluB - aluA;
    }
    if (ifun == ANDQ) {
       ret = aluA & aluB;
    }
    if (ifun == XORQ) {
       ret = aluA ^ aluB;
    }

    zf = (ret == 0);
    sf = (Tools::sign(ret));

    if (setCC) {
        CC(of, zf, sf);
    }

    return ret;
}

void ExecuteStage::CC(bool of, bool zf, bool sf) {
    ConditionCodes * codes = ConditionCodes::getInstance();
    bool error;

    /**
    bool setCC = set_cc(ifun);
    
    bool prev_of = codes->getConditionCode(OF, error);
    bool prev_zf = codes->getConditionCode(ZF, error);
    bool prev_sf = codes->getConditionCode(SF, error);


    of = (setCC) ? of : prev_of;
    zf = (setCC) ? zf : prev_zf;
    sf = (setCC) ? sf : prev_sf;
    **/

    codes->setConditionCode(of, OF, error);
    codes->setConditionCode(zf, ZF, error);
    codes->setConditionCode(sf, SF, error);
}

uint64_t ExecuteStage::cond(uint64_t icode, uint64_t ifun) {

    ConditionCodes * codes = ConditionCodes::getInstance();
    bool error;
    bool of = codes->getConditionCode(OF, error);
    bool zf = codes->getConditionCode(ZF, error);
    bool sf = codes->getConditionCode(SF, error);
    
    if (icode != IJXX && icode != ICMOVXX) {
        return 0;
    }

    switch(ifun)
    {	
	case LESSEQ:    return (sf ^ of) | zf;
    break;

	case LESS:	return (sf ^ of);
    break;

	case EQUAL:	return zf;
    break;

	case NOTEQUAL:	return !zf;
    break;

	case GREATER:	return !(sf ^ of) & !zf;
    break;

	case GREATEREQ: return !(sf ^ of);
    break;

	case UNCOND: return 1;
    default: return 0;
    }
}

uint64_t ExecuteStage::e_valE() {
    return valE;
}

uint64_t ExecuteStage::e_dstE() {
    return dstE;
}

bool ExecuteStage::calculateControlSignals(uint64_t m_stat, uint64_t W_stat) {

    if (m_stat == SADR || m_stat == SINS || m_stat == SHLT) 
        return true;
    if (W_stat == SADR || W_stat == SINS || W_stat == SHLT)
        return true;
    return false;

}

uint64_t ExecuteStage::geteCnd() {
    return cnd;
}

