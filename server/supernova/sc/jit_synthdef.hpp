//  jit for synthdefs
//  Copyright (C) 2012 Tim Blechmann
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; see the file COPYING.  If not, write to
//  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
//  Boston, MA 02111-1307, USA.

#ifndef JIT_SYNTHDEF_HPP
#define JIT_SYNTHDEF_HPP

#include "sc_synthdef.hpp"

#include "llvm/DerivedTypes.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/PassManager.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Support/IRBuilder.h"
#include "llvm/Support/TargetSelect.h"


namespace nova   {
namespace jit    {

using namespace llvm;

class UnitJIT
{
    Module * module;

public:
    StructType *StructTy_struct_Unit;   // struct Unit;
    FunctionType* UnitCalcFuncTy;       // UnitCalcFunc;
    PointerType* UnitPtrTy;
    PointerType* UnitPtrPtrTy;

    ExecutionEngine * ee;
    FunctionPassManager * fpm;

    Value * functionIndex;
    Value * zero;
    int blocksize;

    UnitJIT()
    {
        LLVMContext &context = getGlobalContext();
        module = new Module("supernova jit", context);

        init_unit_type(context);
        init_jit(context);

        blocksize = 64;
        functionIndex = ConstantInt::get(Type::getInt32Ty(context), 15);
        zero = ConstantInt::get(Type::getInt64Ty(context), 0);
    }

private:
    void init_jit(LLVMContext & context)
    {
        InitializeNativeTarget();

        // Create the JIT.  This takes ownership of the module.
        std::string ErrStr;
        ee = EngineBuilder(module).setErrorStr(&ErrStr)
        .setEngineKind(EngineKind::JIT)
        .setOptLevel(CodeGenOpt::Aggressive).create();
        if (!ee) {
            fprintf(stderr, "Could not create ExecutionEngine: %s\n", ErrStr.c_str());
            exit(1);
        }

        fpm = new FunctionPassManager(module);

        // Set up the optimizer pipeline.  Start with registering info about how the
        // target lays out data structures.
        fpm->add(new TargetData(*ee->getTargetData()));
        // Provide basic AliasAnalysis support for GVN.
        fpm->add(createBasicAliasAnalysisPass());
        // Do simple "peephole" optimizations and bit-twiddling optzns.
        fpm->add(createInstructionCombiningPass());
        // Reassociate expressions.
        fpm->add(createReassociatePass());
        // Eliminate Common SubExpressions.
        fpm->add(createGVNPass());
        // Simplify the control flow graph (deleting unreachable blocks, etc).
        fpm->add(createCFGSimplificationPass());

        fpm->doInitialization();
    }

    void init_unit_type(LLVMContext & context)
    {
        std::vector<Type*> FuncTy_0_args;

        StructTy_struct_Unit = StructType::create(context, "struct.Unit");
        std::vector<Type*> StructTy_struct_Unit_fields;

        StructType *StructTy_struct_World = StructTy_struct_World = StructType::create(context, "struct.World");
        PointerType* PointerTy_2 = PointerType::get(StructTy_struct_World, 0);
        StructTy_struct_Unit_fields.push_back(PointerTy_2);

        StructType *StructTy_struct_UnitDef = StructType::create(context, "struct.UnitDef");
        PointerType* PointerTy_3 = PointerType::get(StructTy_struct_UnitDef, 0);
        StructTy_struct_Unit_fields.push_back(PointerTy_3);

        StructType *StructTy_struct_Graph = StructType::create(context, "struct.Graph");
        PointerType* PointerTy_4 = PointerType::get(StructTy_struct_Graph, 0);

        StructTy_struct_Unit_fields.push_back(PointerTy_4);
        StructTy_struct_Unit_fields.push_back(IntegerType::get(context, 32));
        StructTy_struct_Unit_fields.push_back(IntegerType::get(context, 32));
        StructTy_struct_Unit_fields.push_back(IntegerType::get(context, 16));
        StructTy_struct_Unit_fields.push_back(IntegerType::get(context, 16));
        StructTy_struct_Unit_fields.push_back(IntegerType::get(context, 16));
        StructTy_struct_Unit_fields.push_back(IntegerType::get(context, 16));

        StructType *StructTy_struct_Wire = StructType::create(context, "struct.Wire");
        PointerType* PointerTy_6 = PointerType::get(StructTy_struct_Wire, 0);
        PointerType* PointerTy_5 = PointerType::get(PointerTy_6, 0);

        StructTy_struct_Unit_fields.push_back(PointerTy_5);
        StructTy_struct_Unit_fields.push_back(PointerTy_5);

        StructType *StructTy_struct_Rate = StructType::create(context, "struct.Rate");
        PointerType* PointerTy_7 = PointerType::get(StructTy_struct_Rate, 0);
        StructTy_struct_Unit_fields.push_back(PointerTy_7);

        StructType *StructTy_struct_SC_Unit_Extensions = StructType::create(context, "struct.SC_Unit_Extensions");
        PointerType* PointerTy_8 = PointerType::get(StructTy_struct_SC_Unit_Extensions, 0);

        StructTy_struct_Unit_fields.push_back(PointerTy_8);
        PointerType* PointerTy_10 = PointerType::get(Type::getFloatTy(context), 0);
        PointerType* PointerTy_9 = PointerType::get(PointerTy_10, 0);

        StructTy_struct_Unit_fields.push_back(PointerTy_9);
        StructTy_struct_Unit_fields.push_back(PointerTy_9);

        std::vector<Type*>FuncTy_12_args;
        UnitPtrTy = PointerType::get(StructTy_struct_Unit, 0);

        FuncTy_12_args.push_back(UnitPtrTy);
        FuncTy_12_args.push_back(IntegerType::get(context, 32));
        UnitCalcFuncTy = FunctionType::get(/*Result=*/Type::getVoidTy(context),
                                           /*Params=*/FuncTy_12_args,
                                           /*isVarArg=*/false);

        PointerType* PointerTy_11 = PointerType::get(UnitCalcFuncTy, 0);

        StructTy_struct_Unit_fields.push_back(PointerTy_11);
        StructTy_struct_Unit_fields.push_back(IntegerType::get(context, 32));
        if (StructTy_struct_Unit->isOpaque()) {
            StructTy_struct_Unit->setBody(StructTy_struct_Unit_fields, /*isPacked=*/false);
        }

        UnitPtrPtrTy = PointerType::get(UnitPtrTy, 0);
    }

public:
    sc_synthdef::synthdef_calc_func jit_synthdef(sc_synthdef const & synthdef)
    {


        Function * f = generate_synth_function(module->getContext(), synthdef);

        fpm->run(*f);
//         f->dump();

        void *raw_ptr = ee->getPointerToFunction(f);

        return (sc_synthdef::synthdef_calc_func)(raw_ptr);
    }

    Function * generate_synth_function(LLVMContext & context, sc_synthdef const & synthdef)
    {
        Function * thisFunction = create_synth_function(context, std::string(synthdef.name().c_str()) + "_function");

        BasicBlock & bb = thisFunction->getEntryBlock();

        Function::arg_iterator args = thisFunction->arg_begin();
        args++;
        Value* ptr_units = args++;
        ptr_units->setName("units");

        size_t index_in_calcunits = 0;
        for (size_t i = 0; i != synthdef.unit_count(); ++i) {
            sc_synthdef::unit_spec_t const & spec = synthdef.graph[i];

            int samples_per_tick;
            switch (spec.rate) {
            case 0:
            case 3:
                //
                continue;
            case 1:
                samples_per_tick = 1;
                break;
            case 2:
                samples_per_tick = blocksize;
                break;

            default:
                assert(false);
            }

            std::vector<Value*> unit_index;
            unit_index.push_back(ConstantInt::get(Type::getInt32Ty(context), index_in_calcunits));
            GetElementPtrInst * getUnit = GetElementPtrInst::Create(ptr_units, unit_index, "index_", &bb);

            LoadInst * loadUnit = new LoadInst(getUnit, "unit", &bb);
            loadUnit->setAlignment(8);
            create_unit_call(context, &bb, loadUnit, samples_per_tick);

            index_in_calcunits += 1;
        }

        ReturnInst::Create(context, &bb);
        verifyFunction(*thisFunction);
        return thisFunction;
    }

private:
    Function * create_synth_function(LLVMContext & context, std::string const & function_name)
    {
        std::vector<Type*> args;
        args.push_back(Type::getInt8PtrTy(context));
        args.push_back(UnitPtrPtrTy);
        FunctionType *FT = FunctionType::get(Type::getVoidTy(context), args, false);

        Function * TheFunction = Function::Create(FT, Function::ExternalLinkage, function_name, module);

        TheFunction->setHasUWTable();

        BasicBlock * bb = BasicBlock::Create(context, "entry", TheFunction);

        return TheFunction;
    }

    CallInst * create_unit_call(LLVMContext & context, BasicBlock * bb, Value * unit, int samples_per_tick)
    {
        std::vector<Value*> elementIndex;
        elementIndex.push_back(zero);
        elementIndex.push_back(functionIndex);

        GetElementPtrInst * getFunctionAddress = GetElementPtrInst::Create(unit, elementIndex, "function_address", bb);
        LoadInst * loadFunction = new LoadInst(getFunctionAddress, "fn", bb);
        loadFunction->setAlignment(8);

        Value * inferredCount = ConstantInt::get(Type::getInt32Ty(context), samples_per_tick);

        std::vector<Value*> functionArgs;
        functionArgs.push_back(unit);
        functionArgs.push_back(inferredCount);
        CallInst * callUnit = CallInst::Create(loadFunction, functionArgs, "", bb);
        return callUnit;
    }
};

}

using jit::UnitJIT;

}

#endif