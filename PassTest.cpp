#include <string>
#include <iostream>
#include <utility>
#include <memory>

#include <llvm/IR/Module.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Utils.h>
#include "llvm/Transforms/Utils/Cloning.h"
#include <llvm/IR/IRBuilder.h>
#include "llvm/Support/WithColor.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Linker/Linker.h"
#include <llvm/Transforms/IPO.h>

#include "pass/easypass/src/easypass.hpp"
#include "pass/Power2ToLShift/src/Power2ToLShift.hpp"
#include "pass/GlobalVariableToLocal/src/GlobalVariableToLocal.hpp"

#ifdef test
#include <llvm/Transforms/Scalar/DCE.h>
#include <llvm/Transforms/Scalar/ADCE.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm-c/Core.h>
#include <llvm-c/BitReader.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>
#include <fstream>
#include <iostream>
#endif

using namespace llvm; 
using namespace std;

void dump2(Module &M);

//https://8128.jp/notebooks/llvm-c-api
//https://itchyny.hatenablog.com/entry/2017/03/06/100000
//https://gist.github.com/ysaito8015/d37a154bdd60852ce58f44f4a0949a03
//http://d.hatena.ne.jp/imasahiro/20111217/1324086354
//https://qiita.com/sakasin/items/097fa676ccfd117acaa2#fn1
//https://qiita.com/k2ymg/items/653c5b22b74a091be604

//https://qiita.com/long_long_float/items/50ba0d9dc4075f3934a1

int main(int argc, char* argv[])
{
	LLVMContext Context;
	SMDiagnostic Err;

	if (argc < 2) {
		fprintf(stderr, "usage: %s <bitcode.file>\n", argv[0]);
		return 1;
	}

	// ■Load Guest Instructions
	printf("---[Guest Instructions List]---\n");
	vector<string> GuestInstList = {"sub1", "sub2", "sub1", "sub2"};
	for (auto it : GuestInstList) {	printf("%s\n", it.c_str());	}
	printf("---[End]---\n\n");

	// ■Load Module Base
	printf("---[Library Instruction list]---\n");
	string FuncLibPath = argv[1];
	std::unique_ptr<Module> M = parseIRFile(FuncLibPath, Err, Context);
	if (!M) {
		Err.print(argv[0], errs());
		return 1;
	}
//	printf("File Name: %s\n", FuncLibPath.c_str());
//	printf("Module Name: %s\n", M_FuncLib->getName());
//	printf("Module Dump:\n");
//	M_FuncLib->dump();
	for (auto &F : *M) {
		printf("%s\n", F.getName());
		// ついでに属性変更しとく。
		F.removeFnAttr(Attribute::NoInline);
		F.removeFnAttr(Attribute::OptimizeNone);
		F.addFnAttr(Attribute::AlwaysInline);
	}
	printf("---[End]---\n\n");

	// ■Make Output Module & main Function
//	llvm::Module *M_Out = new llvm::Module("M_Out", Context);
	llvm::IRBuilder<> builder(Context);

	auto *funcType = llvm::FunctionType::get(builder.getVoidTy(), false);
	auto *mainFunc = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "main", M.get());
	auto *entrypoint = llvm::BasicBlock::Create(Context, "entrypoint", mainFunc);
	builder.SetInsertPoint(entrypoint);

	// ■Add Callees
	printf("--------\n\n");
	M->dump();

	for (auto GuestInst : GuestInstList) {

		Function *func;
		func = M->getFunction(GuestInst.c_str());
		if (!func) {
			printf("Can't find function!:%s\n", GuestInst.c_str());
			return 1;
#if 0
			func = M_FuncLib->getFunction(GuestInst.c_str());
			if (!func) {
				printf("Can't find function!:%s\n", GuestInst.c_str());
				return 1;
			}

			ValueToValueMapTy VMap;
//			Function* func_clone = CloneFunction(func, VMap);
//			func_clone->removeFromParent();
//			func_clone->removeFnAttr(Attribute::NoInline);
//			func_clone->removeFnAttr(Attribute::OptimizeNone);
//			func_clone->addFnAttr(Attribute::AlwaysInline);
//			M_Out->getFunctionList().push_back(func_clone);
// Cloneをつくると関数名がsub1.1とかに変わってしまうので、ライブラリから移動してみたが、
// ライブラリからモジュールが消えるのもなんか変。。
// とここで、逆にライブラリにmain関数を追加した方がよい気がしてきたので、作り直し！！
			func->removeFromParent();
			func->removeFnAttr(Attribute::NoInline);
			func->removeFnAttr(Attribute::OptimizeNone);
			func->addFnAttr(Attribute::AlwaysInline);
			M_FuncLib->getFunctionList().push_back(func);
#endif
		}

//		builder.CreateCall(func_clone);
		builder.CreateCall(func);
	}

	builder.CreateRetVoid();

	// ■Optimization
	llvm::legacy::PassManager PM;

	printf("\n----[Before Optimization]----\n", argv[1]);
	M->dump();
//	dump2(*M);

//	PM.add(createPromoteMemoryToRegisterPass());
//	PM.add(new EasyPass());
//	PM.add(new Power2ToLShift());
//	PM.add(new GlobalVariableToLocal());
	PM.add(createIPSCCPPass());
	PM.run(*M);

	printf("\n----[After Optimization]----\n", argv[1]);
	M->dump();
//	dump2(*M);

	return 0;
}

#if 0
int main(int argc, char* argv[])
{
	LLVMContext Context;
	SMDiagnostic Err;

	if (argc < 2) {
		fprintf(stderr, "usage: %s <bitcode.file>\n", argv[0]);
		return 1;
	}

	// ■Load Guest Instructions
	printf("---[Guest Instructions List]---\n");
	vector<string> GuestInstList = { "sub1", "sub2", "sub1", "sub2" };
	for (auto it : GuestInstList) { printf("%s\n", it.c_str()); }
	printf("---[End]---\n\n");

	// ■Load Function Library Module
	printf("---[Function Library - Function list]---\n");
	string FuncLibPath = argv[1];
	std::unique_ptr<Module> M_FuncLib = parseIRFile(FuncLibPath, Err, Context);
	if (!M_FuncLib) {
		Err.print(argv[0], errs());
		return 1;
	}
	//	printf("File Name: %s\n", FuncLibPath.c_str());
	//	printf("Module Name: %s\n", M_FuncLib->getName());
	//	printf("Module Dump:\n");
	//	M_FuncLib->dump();
	for (auto &F : *M_FuncLib) { printf("%s\n", F.getName()); }
	printf("---[End]---\n\n");

	// ■Make Output Module & main Function
	llvm::Module *M_Out = new llvm::Module("M_Out", Context);
	llvm::IRBuilder<> builder(Context);

	auto *funcType = llvm::FunctionType::get(builder.getVoidTy(), false);
	auto *mainFunc = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "main", M_Out);
	auto *entrypoint = llvm::BasicBlock::Create(Context, "entrypoint", mainFunc);
	builder.SetInsertPoint(entrypoint);

	// ■Add Callees
	printf("--------\n\n");
	M_Out->dump();
	printf("--------\n\n");
	M_FuncLib->dump();

	for (auto GuestInst : GuestInstList) {

		Function *func;
		func = M_Out->getFunction(GuestInst.c_str());
		if (!func) {
			func = M_FuncLib->getFunction(GuestInst.c_str());
			if (!func) {
				printf("Can't find function!:%s\n", GuestInst.c_str());
				return 1;
			}

			ValueToValueMapTy VMap;
			//			Function* func_clone = CloneFunction(func, VMap);
			//			func_clone->removeFromParent();
			//			func_clone->removeFnAttr(Attribute::NoInline);
			//			func_clone->removeFnAttr(Attribute::OptimizeNone);
			//			func_clone->addFnAttr(Attribute::AlwaysInline);
			//			M_Out->getFunctionList().push_back(func_clone);
			// Cloneをつくると関数名がsub1.1とかに変わってしまうので、ライブラリから移動してみたが、
			// ライブラリからモジュールが消えるのもなんか変。。
			// とここで、逆にライブラリにmain関数を追加した方がよい気がしてきたので、作り直し！！
			func->removeFromParent();
			func->removeFnAttr(Attribute::NoInline);
			func->removeFnAttr(Attribute::OptimizeNone);
			func->addFnAttr(Attribute::AlwaysInline);
			M_Out->getFunctionList().push_back(func);
		}

		//		builder.CreateCall(func_clone);
		builder.CreateCall(func);

		printf("--------\n\n");
		M_Out->dump();
		printf("--------\n\n");
		M_FuncLib->dump();

	}

	M_Out->dump();

	builder.CreateRetVoid();

#if 0

	//	Module::FunctionListType &FLT = M_FuncLib->getFunctionList();
	//	for (auto &FLTm : FLT) {
	//		printf("[Lib Function Name: %s]\n", FLTm.getName());
	//		FLTm.dump();
	//	}


	printf("\n----[Cloning]----\n");
	ValueToValueMapTy VMap;
	//	Function* func_clone = CloneFunction(func, VMap, false);
	Function* func_clone = CloneFunction(func, VMap);
	//	llvm::RemapFunction(*func_clone, VM);
	func_clone->dump();

	func_clone->dump(); printf("\n---\n");
	//	func_clone->eraseFromParent();
	func_clone->removeFromParent();
	M_Out->getFunctionList().push_back(func_clone);

	//	M_Out->getOrInsertFunction("test", builder.getVoidTy());


	WithColor::error() << " loading file '" << "'\n";
	errs() << "test\n";
	Linker(*Composite);

	for (auto &F : *M_FuncLib)
	{

		ValueToValueMapTy VMap;
		Function* callee_clone = CloneFunction(&F, VMap, false);
		M_Out->dump();
		callee_clone->dump();
		M_Out->getFunctionList().push_back(callee_clone);
		M_Out->dump();

#if 0
		for (auto &B : F)
		{
			for (auto &I : B)
			{

			}
		}
#endif
	}



	printf("\n----[Create End]----\n");
	M_Out->dump();


	llvm::legacy::PassManager PM;


	//	SmallVector<char, 0> Buffer;
	//	SmallVector<char, 0> FirstRunBuffer;
	//	std::unique_ptr<Module> M2(CloneModule(*M));


	printf("\n----[Before Optimization]----\n", argv[1]);
	//	M->dump();
	dump2(*M_Out);

	//	PM.add(createPromoteMemoryToRegisterPass());
	//	PM.add(new EasyPass());
	//	PM.add(new Power2ToLShift());
	PM.add(new GlobalVariableToLocal());
	PM.run(*M_Out);

	printf("\n----[After Optimization]----\n", argv[1]);
	//	M->dump();
	dump2(*M_Out);


	//	PM.add(createReassociatePass());
	//	PM.add(createAggressiveDCEPass());




	//	FirstRunBuffer = Buffer;
	//	Buffer.clear();
	//	PM.run(*M2);
	//	printf("\n----[After Optimization]----\n", argv[1]);
	//	dump2(*M);
	//	printf("\n----[After Optimization2]----\n", argv[1]);
	//	dump2(*M2);

#endif

	return 0;
}
#endif

void dump2(Module &M)
{
	for (auto &F : M)
	{
		printf("[Function Name: %s]\n", F.getName());
		for (auto &B : F)
		{
			printf("[BasicBlock Name: %s]\n", B.getName());
			for (auto &I : B)
			{
				I.dump();
			}
		}
	}
	printf("[End of Dump2]\n\n");
}