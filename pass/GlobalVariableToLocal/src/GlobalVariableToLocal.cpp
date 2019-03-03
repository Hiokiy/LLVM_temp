#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/APInt.h"
#include "llvm/IR/PatternMatch.h"

#include "GlobalVariableToLocal.hpp"

using namespace llvm;
using namespace PatternMatch;

//Ç‹ÇæçÏÇ¡ÇƒÇ»Ç¢ÇÊ
bool GlobalVariableToLocal::runOnModule(Module &M)
{
	for (auto &F : M)
	{
		for (auto &B : F)
		{
			for (auto &I : B)
			{
				I.dump();
				if (auto *si = dyn_cast<StoreInst>(&I))
				{
					si->dump();
					si->getOperand(1)->dump();
					auto *gv = dyn_cast<GlobalVariable>(si->getOperand(1));
					if (gv)
					{
						gv->dump();
	
					}
					//return true;
				}
			}
		}
	}
	return false;
}


void GlobalVariableToLocal::getAnalysisUsage(llvm::AnalysisUsage &AU) const{
	AU.setPreservesCFG();
}

char GlobalVariableToLocal::ID=0;
static RegisterPass<GlobalVariableToLocal> X("GlobalVariableToLocal", "GlobalVariableToLocal Test Pass",
                                false /* Only looks at CFG */,
                                true /* Transform Pass */);

