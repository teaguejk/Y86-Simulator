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
#include "MemoryStage.h"
#include "Memory.h"
#include "Status.h"
#include "Debug.h"
#include "Instructions.h"

void MemoryStage::setWInput(W * wreg, uint64_t stat, uint64_t icode, uint64_t valE,
			    uint64_t valA, uint64_t dstE, uint64_t dstM)
{
    wreg->getstat()->setInput(stat);
    wreg->geticode()->setInput(icode);
    wreg->getvalE()->setInput(valE);
    wreg->getvalM()->setInput(valA);
    wreg->getdstE()->setInput(dstE);
    wreg->getdstM()->setInput(dstM);
}

uint64_t mem_addr(uint64_t M_icode, uint64_t M_valE, uint64_t M_valA)
{
	if(M_icode == IRMMOVQ || M_icode == IPUSHQ || M_icode == ICALL || M_icode == IMRMOVQ)
		return M_valE;

	if(M_icode == IPOPQ || M_icode == IRET)
		return M_valA;

	return 0;
}

bool mem_read(uint64_t M_icode)
{
	return (M_icode == IMRMOVQ || M_icode == IPOPQ || M_icode == IRET); 
}

bool mem_write(uint64_t M_icode)
{
	return (M_icode == IRMMOVQ || M_icode == IPUSHQ || M_icode == ICALL);
}

bool MemoryStage::doClockLow(PipeReg ** pregs, Stage ** stages)
{
    M * mreg = (M*)pregs[MREG];
    W * wreg = (W*)pregs[WREG];
    stat = mreg->getstat()->getOutput();
    uint64_t icode = mreg->geticode()->getOutput();
    //uint64_t Cnd = mreg->getCnd()->getOutput();
    uint64_t valE = mreg->getvalE()->getOutput();
    uint64_t valA = mreg->getvalA()->getOutput();
    uint64_t dstE = mreg->getdstE()->getOutput();
    uint64_t dstM = mreg->getdstM()->getOutput();
    valM = 0;

    uint64_t addr = mem_addr(icode, valE, valA);
    bool error = false;
    Memory * mem = Memory::getInstance();

    if(mem_read(icode))
    {
	    valM = mem->getLong(addr, error);
    }

    if(mem_write(icode))
    {
	    mem->putLong(valA, addr, error);
    }
    
    stat = m_stat(error, stat);

    setWInput(wreg, stat, icode, valE, valM, dstE, dstM);
    return false;
}

void MemoryStage::doClockHigh(PipeReg ** pregs)
{
    W * wreg = (W*)pregs[WREG];
    wreg->getstat()->normal();
    wreg->geticode()->normal();
    wreg->getvalE()->normal();
    wreg->getvalM()->normal();
    wreg->getdstE()->normal();
    wreg->getdstM()->normal();
}

uint64_t MemoryStage::getvalM()
{
	return valM;
}

uint64_t MemoryStage::getm_stat()
{
    return stat;
}

uint64_t MemoryStage::m_stat(bool mem_error, uint64_t M_stat) {
    if (mem_error) return SADR;
    return M_stat;
}

