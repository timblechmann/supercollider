/*
	SuperCollider real time audio synthesis system
	Copyright (c) 2002 James McCartney. All rights reserved.
	http://www.audiosynth.com

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include "SCBase.h"
#include "PyrParseNode.h"
#include "PyrLexer.h"
#include "PyrKernel.h"
#include "Opcodes.h"
#include "PyrPrimitive.h"
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <cstdio>

#ifdef _WIN32
# define snprintf _snprintf
# define PATH_MAX _MAX_PATH
#endif


extern int textpos;

void dumpNodeList(PyrParseNode *node)
{
	for (; node; node = node->mNext) {
		DUMPNODE(node, 0);
	}
}

void PyrCurryArgNode::dump(int level)
{
	postfl("%2d CurryArg %d\n", level, mArgNum);
}

void PyrSlotNode::dump(int level)
{
	if (mClassno == pn_PushLitNode)
		dumpPushLit(level);
	else if (mClassno == pn_PushNameNode)
		postfl("%2d PushName '%s'\n", level, slotRawSymbol(&mSlot)->name);
	else if (mClassno == pn_LiteralNode)
		dumpLiteral(level);
	else {
		postfl("%2d SlotNode\n", level);
		dumpPyrSlot(&mSlot);
	}
	DUMPNODE(mNext, level);
}

void PyrPushKeyArgNode::dump(int level)
{
	postfl("%2d PushKeyArgNode\n", level);
	DUMPNODE(mSelector, level+1);
	DUMPNODE(mExpr, level+1);
	DUMPNODE(mNext, level);
}

void PyrClassExtNode::dump(int level)
{
	postfl("%2d ClassExt '%s'\n", level, slotRawSymbol(&mClassName->mSlot)->name);
	DUMPNODE(mMethods, level+1);
	DUMPNODE(mNext, level);
}

void PyrClassNode::dump(int level)
{
	postfl("%2d Class '%s'\n", level, slotRawSymbol(&mClassName->mSlot)->name);
	DUMPNODE(mSuperClassName, level+1);
	DUMPNODE(mVarlists, level+1);
	DUMPNODE(mMethods, level+1);
	DUMPNODE(mNext, level);
}

void PyrMethodNode::dump(int level)
{
	postfl("%2d MethodNode '%s'  %s\n", level, slotRawSymbol(&mMethodName->mSlot)->name,
		   mPrimitiveName ? slotRawSymbol(&mPrimitiveName->mSlot)->name:"");
	DUMPNODE(mArglist, level+1);
	DUMPNODE(mBody, level+1);
	DUMPNODE(mNext, level);
}

void PyrArgListNode::dump(int level)
{
	postfl("%2d ArgList\n", level);
	DUMPNODE(mVarDefs, level+1);
	DUMPNODE(mRest, level+1);
	DUMPNODE(mNext, level);
}

void PyrVarListNode::dump(int level)
{
	postfl("%2d VarList\n", level);
	DUMPNODE(mVarDefs, level+1);
	DUMPNODE(mNext, level);
}

void PyrVarDefNode::dump(int level)
{
	postfl("%2d VarDef '%s'\n", level, slotRawSymbol(&mVarName->mSlot)->name);
	DUMPNODE(mDefVal, level);
	DUMPNODE(mNext, level);
}

void PyrCallNode::dump(int level)
{
	postfl("%2d Call '%s'\n", level, slotRawSymbol(&mSelector->mSlot)->name);
	DUMPNODE(mArglist, level+1);
	DUMPNODE(mKeyarglist, level+1);
	DUMPNODE(mNext, level);
}

void PyrBinopCallNode::dump(int level)
{
	postfl("%2d BinopCall '%s'\n", level, slotRawSymbol(&mSelector->mSlot)->name);
	DUMPNODE(mArglist, level+1);
	DUMPNODE(mNext, level);
}

void PyrDropNode::dump(int level)
{
	postfl("%2d Drop (\n", level);
	DUMPNODE(mExpr1, level+1);
	postfl(" -> %2d Drop\n", level);
	DUMPNODE(mExpr2, level+1);
	postfl(") %2d Drop\n", level);
	DUMPNODE(mNext, level);
}

void PyrSlotNode::dumpPushLit(int level)
{
	postfl("%2d PushLit\n", level);
	if (!IsPtr(&mSlot)) dumpPyrSlot(&mSlot);
	else {
		DUMPNODE((PyrParseNode*)slotRawObject(&mSlot), level);
	}
}

void PyrSlotNode::dumpLiteral(int level)
{
	postfl("%2d Literal\n", level);
	if (!IsPtr(&mSlot)) dumpPyrSlot(&mSlot);
	else {
		DUMPNODE((PyrParseNode*)slotRawObject(&mSlot), level);
	}
}

void PyrReturnNode::dump(int level)
{
	postfl("%2d Return (\n", level);
	DUMPNODE(mExpr, level+1);
	postfl(") %2d Return \n", level);
	DUMPNODE(mNext, level);
}

void PyrBlockReturnNode::dump(int level)
{
	postfl("%2d FuncReturn\n", level);
	DUMPNODE(mNext, level);
}

void PyrAssignNode::dump(int level)
{
	postfl("%2d Assign '%s'\n", level, slotRawSymbol(&mVarName->mSlot)->name);
	DUMPNODE(mVarName, level+1);
	DUMPNODE(mExpr, level+1);
	DUMPNODE(mNext, level);
}

void PyrSetterNode::dump(int level)
{
	//postfl("%2d Assign '%s'\n", level, slotRawSymbol(&mVarName->mSlot)->name);
	DUMPNODE(mSelector, level+1);
	DUMPNODE(mExpr1, level+1);
	DUMPNODE(mExpr2, level+1);
}

void PyrMultiAssignNode::dump(int level)
{
	postfl("%2d MultiAssign\n", level);
	DUMPNODE(mVarList, level+1);
	DUMPNODE(mExpr, level+1);
	DUMPNODE(mNext, level);
}

void PyrMultiAssignVarListNode::dump(int level)
{
	postfl("%2d MultiAssignVarList\n", level);
	DUMPNODE(mVarNames, level+1);
	DUMPNODE(mRest, level+1);
	DUMPNODE(mNext, level);
}

void PyrDynDictNode::dump(int level)
{
	postfl("%2d DynDict\n", level);
	DUMPNODE(mElems, level+1);
	DUMPNODE(mNext, level);
}

void PyrDynListNode::dump(int level)
{
	postfl("%2d DynList\n", level);
	DUMPNODE(mElems, level+1);
	DUMPNODE(mNext, level);
}

void PyrLitListNode::dump(int level)
{
	postfl("%2d LitList\n", level);
	postfl(" %2d mElems\n", level);
	DUMPNODE(mElems, level+1);
	postfl(" %2d mNext\n", level);
	DUMPNODE(mNext, level);
}

void PyrBlockNode::dump(int level)
{
	postfl("%2d Func\n", level);
	DUMPNODE(mArglist, level+1);
	DUMPNODE(mBody, level+1);
	DUMPNODE(mNext, level);
}

void dumpPyrSlot(PyrSlot* slot)
{
	char str[1024];
	slotString(slot, str, 1024);
	post("   %s\n", str);
}



namespace detail {

////////////////
// print float

enum {
	floatFullPrecision,
	floatSinglePrecision,
	floatDumpSlot
};


template <int FloatStyle>
static inline int printSlotFloat(PyrSlot * slot, char *str, size_t size)
{
	switch( FloatStyle ) {
	case floatFullPrecision:
		return std::snprintf(str, size, "%.14g", slotRawFloat(slot));
	case floatSinglePrecision:
		return std::snprintf(str, size, "%f", slotRawFloat(slot));
	case floatDumpSlot: {
		union {
			int32 i[2];
			double f;
		} u;
		u.f = slotRawFloat(slot);
		return std::snprintf(str, size, "Float %f   %08X %08X", u.f, u.i[0], u.i[1]);
	}
	}
}

////////////////
// print pointer

enum {
	pointerRawPointer,
	pointerPrintNil,
	pointerPrintWord,
	pointerPrintWithType
};

template <int PointerStyle>
static inline int printSlotPointer(PyrSlot * slot, char *str, size_t size)
{
	switch( PointerStyle ) {
	case pointerRawPointer:
		return std::snprintf(str, size, "%p", slotRawPtr(slot));
	case pointerPrintNil:
		return std::snprintf(str, size, "/*Ptr*/ nil");
	case pointerPrintWord:
		return std::snprintf(str, size, "ptr%p", slotRawPtr(slot));
	case pointerPrintWithType:
		return std::snprintf(str, size, "RawPointer %p", slotRawPtr(slot));
	}
}

////////////////
// print true/false/nil

static inline int printSlotTrue(char *str, size_t size)
{
	return std::snprintf(str, size, "true");
}

static inline int printSlotFalse(char *str, size_t size)
{
	return std::snprintf(str, size, "false");
}

static inline int printSlotNil(char *str, size_t size)
{
	return std::snprintf(str, size, "nil");
}


////////////////
// print integers

enum {
	intRaw,
	intWithType,
};

template <int IntStyle>
static inline int printSlotInt(PyrSlot const * slot, char *str, size_t size)
{
	switch( IntStyle ) {
	case intRaw:
		return std::snprintf(str, size, "%d", slotRawInt(slot));

	case intWithType:
		return std::snprintf(str, size, "Integer %d", slotRawInt(slot));
	}
}


////////////////
// print char

enum {
	charInt,
	charCompileString,
	charWithType,
};

template <int CharStyle>
static inline int printSlotChar(PyrSlot const * slot, char *str, size_t size)
{
	switch( CharStyle ) {
	case charInt:
		return std::snprintf(str, size, "%d", (int)slotRawChar(slot));
	case charCompileString: {
		int c = slotRawChar(slot);
		if (isprint(c)) {
			return std::snprintf(str, size, "$%c", c);
		} else {
			switch (c) {
			case '\n' : return std::snprintf(str, size, "$\\n");
			case '\r' : return std::snprintf(str, size, "$\\r");
			case '\t' : return std::snprintf(str, size, "$\\t");
			case '\f' : return std::snprintf(str, size, "$\\f");
			case '\v' : return std::snprintf(str, size, "$\\v");
			default   : return std::snprintf(str, size, "%d.asAscii", c);
			}
		}
	}
	case charWithType:
		return std::snprintf(str, size, "Character %d '%c'", (int)(slotRawChar(slot)), (int)(slotRawChar(slot)));
	}
}

////////////////
// print symbols

enum {
	symbolShort,
	symbolShortWithType,
	symbolCompileString,
};

template <int SymbolStyle>
static inline int printSlotSymbol(PyrSlot * slot, char *str, size_t size)
{
	const PyrSymbol * symbol = slotRawSymbol(slot);
	const char * symbolName = nullptr;

	switch( SymbolStyle ) {
	case symbolShort: {
		if (strlen(slotRawSymbol(slot)->name) > 240) {
			char str2[256];
			memcpy(str2, slotRawSymbol(slot)->name, 240);
			str2[240] = 0;
			snprintf(str, size, "'%s...'", str2);
		} else {
			snprintf(str, size, "'%s'", slotRawSymbol(slot)->name);
		}
	}

	case symbolShortWithType: {
		if (std::strlen(symbol->name) > 240) {
			char str2[256];
			std::memcpy(str2, symbol->name, 240);
			str2[240] = 0;
			return std::snprintf(str, size, "Symbol '%s...'", str2);
		} else
			return std::snprintf(str, size, "Symbol '%s'", symbol->name);
	}
	case symbolCompileString:
		return std::snprintf(str, size, "\\%s", symbol->name);
	}
}


////////////////
// objects

enum {
	objectPrint,
	objectPrintEveryClass,
	objectDump
};

static int printObject(PyrSlot * slot, PyrObject * obj, char *str, size_t size)
{
	assert(obj);
	PyrClass * classptr = obj->classptr;
	if (classptr == class_class) {
		return std::snprintf(str, size, "class %s", slotRawSymbol(&((PyrClass*)obj)->name)->name);
	} else if (classptr == class_string) {
		char str2[32];
		int len;
		if (obj->size > 31) {
			memcpy(str2, (char*)obj->slots, 28);
			str2[28] = '.';
			str2[29] = '.';
			str2[30] = '.';
			str2[31] = 0;
		} else {
			len = sc_min(31, obj->size);
			memcpy(str2, (char*)obj->slots, len);
			str2[len] = 0;
		}
		return std::snprintf(str, size, "\"%s\"", str2);
	} else if (classptr == class_method) {
		return std::snprintf(str, size, "%s:%s",
							 slotRawSymbol(&slotRawClass(&slotRawMethod(slot)->ownerclass)->name)->name,
							 slotRawSymbol(&slotRawMethod(slot)->name)->name);
	} else if (classptr == class_fundef) {
		PyrSlot *context, *nextcontext;
		// find function's method
		nextcontext = &slotRawBlock(slot)->contextDef;
		if (NotNil(nextcontext)) {
			do {
				context = nextcontext;
				nextcontext = &slotRawBlock(context)->contextDef;
			} while (NotNil(nextcontext));
			if (isKindOf(slotRawObject(context), class_method)) {
				return std::snprintf(str, size, "< FunctionDef in Method %s:%s >",
									 slotRawSymbol(&slotRawClass(&slotRawMethod(context)->ownerclass)->name)->name,
									 slotRawSymbol(&slotRawMethod(context)->name)->name);
			} else {
				return std::snprintf(str, size, "< FunctionDef in closed FunctionDef >");
			}
		} else {
			return std::snprintf(str, size, "< closed FunctionDef >");
		}
	} else if (classptr == class_frame) {
		if (!slotRawFrame(slot)) {
			return std::snprintf(str, size, "Frame (null)");
		} else if (!slotRawBlock(&slotRawFrame(slot)->method)) {
			return std::snprintf(str, size, "Frame (null method)");
		} else if (slotRawBlock(&slotRawFrame(slot)->method)->classptr == class_method) {
			return std::snprintf(str, size, "Frame (%p) of %s:%s", obj,
								 slotRawSymbol(&slotRawClass(&slotRawMethod(&slotRawFrame(slot)->method)->ownerclass)->name)->name,
								 slotRawSymbol(&slotRawMethod(&slotRawFrame(slot)->method)->name)->name);
		} else {
			return std::snprintf(str, size, "Frame (%p) of Function", obj);
		}
	} else if (classptr == class_array) {
		return std::snprintf(str, size, "[*%d]", obj->size);
	} else {
		return std::snprintf(str, size, "<instance of %s>", slotRawSymbol(&classptr->name)->name);
	}
}

static int printObjectInstance(PyrSlot * slot, char *str, size_t size)
{
	const PyrObject * slotObj = slotRawObject(slot);
	if (slotObj) {
		PyrClass * classptr = slotObj->classptr;
		if (classptr == class_class) {
			return std::snprintf(str, size, "class %s (%p)",
								 slotRawSymbol(&((PyrClass*)slotObj)->name)->name, slotObj);
		} else if (classptr == class_string) {
			char str2[48];
			int len;
			if (slotObj->size > 47) {
				memcpy(str2, (char*)slotObj->slots, 44);
				str2[44] = '.';
				str2[45] = '.';
				str2[46] = '.';
				str2[47] = 0;
			} else {
				len = sc_min(47, slotObj->size);
				memcpy(str2, (char*)slotObj->slots, len);
				str2[len] = 0;
			}
			return std::snprintf(str, size, "\"%s\"", str2);
		} else if (classptr == class_method) {
			return std::snprintf(str, size, "instance of Method %s:%s (%p)",
								 slotRawSymbol(&slotRawClass(&slotRawMethod(slot)->ownerclass)->name)->name,
								 slotRawSymbol(&slotRawMethod(slot)->name)->name, slotRawMethod(slot));
		} else if (classptr == class_fundef) {
			PyrSlot *context, *nextcontext;
			// find function's method
			nextcontext = &slotRawBlock(slot)->contextDef;
			if (NotNil(nextcontext)) {
				do {
					context = nextcontext;
					nextcontext = &slotRawBlock(context)->contextDef;
				} while (NotNil(nextcontext));
				if (isKindOf(slotRawObject(context), class_method)) {
					return std::snprintf(str, size, "instance of FunctionDef in Method %s:%s",
										 slotRawSymbol(&slotRawClass(&slotRawMethod(context)->ownerclass)->name)->name,
										 slotRawSymbol(&slotRawMethod(context)->name)->name);
				} else {
					return std::snprintf(str, size, "instance of FunctionDef in closed FunctionDef");
				}
			} else {
				return std::snprintf(str, size, "instance of FunctionDef - closed");
			}
		} else if (classptr == class_frame) {
			if (!slotRawFrame(slot)) {
				return std::snprintf(str, size, "Frame (%0X)", slotRawInt(slot));
			} else if (slotRawBlock(&slotRawFrame(slot)->method)->classptr == class_method) {
				return std::snprintf(str, size, "Frame (%p) of %s:%s", slotObj,
									 slotRawSymbol(&slotRawClass(&slotRawMethod(&slotRawFrame(slot)->method)->ownerclass)->name)->name,
									 slotRawSymbol(&slotRawMethod(&slotRawFrame(slot)->method)->name)->name);
			} else {
				return std::snprintf(str, size, "Frame (%p) of Function", slotRawFrame(slot));
			}
		} else {
			return std::snprintf(str, size, "instance of %s (%p, size=%d, set=%d)",
								 slotRawSymbol(&classptr->name)->name,
								 slotObj, slotObj->size,
								 slotObj->obj_sizeclass);
		}
	} else {
		return std::snprintf(str, size, "NULL Object Pointer");
	}
}

template <int ObjectStyle>
static inline int printSlotObject(PyrSlot * slot, char *str, size_t size)
{
	PyrObject * slotObj = slotRawObject(slot);
	switch( ObjectStyle ) {
	case objectPrint: {
		if (slotObj) {
			PyrClass * classptr = slotObj->classptr;
			if (classptr == class_class || classptr == class_method || classptr == class_fundef || classptr == class_frame)
				return printObject(slot, slotObj, str, size);
			else {
				str[0] = 0;
				return 0;
			}
		}
		else
			return std::snprintf(str, size, "NULL Object Pointer");
	}

	case objectPrintEveryClass:
		if (slotObj)
			return printObject(slot, slotObj, str, size);
		else
			return std::snprintf(str, size, "NULL Object Pointer");

	case objectDump:
		return printObjectInstance(slot, str, size);
	}
}

} // namespace detail

int slotString(PyrSlot *slot, char *str, size_t size)
{
	using namespace detail;

	switch (GetTag(slot)) {
	case tagInt   : return printSlotInt<intWithType>(slot, str, size);
	case tagChar  : return printSlotChar<charWithType>(slot, str, size);
	case tagSym   : return printSlotSymbol<symbolShortWithType>(slot, str, size);
	case tagObj   : return printSlotObject<objectDump>(slot, str, size);
	case tagNil   : return printSlotNil(str, size);
	case tagFalse : return printSlotFalse(str, size);
	case tagTrue  : return printSlotTrue(str, size);
	case tagPtr   : return printSlotPointer<pointerPrintWithType>(slot, str, size);
	default       : return printSlotFloat<floatDumpSlot>(slot, str, size);
	}
}

int slotOneWord(PyrSlot *slot, char *str, size_t size)
{
	using namespace detail;

	switch (GetTag(slot)) {
	case tagInt :   return printSlotInt<intRaw>(slot, str, size);
	case tagChar :  return printSlotChar<charCompileString>(slot, str, size);
	case tagSym :   return printSlotSymbol<symbolShort>(slot, str, size);
	case tagObj :   return printSlotObject<objectPrintEveryClass>(slot, str, size);
	case tagNil   : return printSlotNil(str, size);
	case tagFalse : return printSlotFalse(str, size);
	case tagTrue  : return printSlotTrue(str, size);
	case tagPtr   : return printSlotPointer<pointerPrintWord>(slot, str, size);
	default       : return printSlotFloat<floatFullPrecision>(slot, str, size);
	}
}

int postString(PyrSlot *slot, char *str, size_t size)
{
	using namespace detail;

	switch (GetTag(slot)) {
	case tagInt   : return printSlotInt<intRaw>(slot, str, size);
	case tagChar  : return printSlotChar<charInt>(slot, str, size);
	case tagSym   : break;
	case tagObj   : return printSlotObject<objectPrint>(slot, str, size);
	case tagNil   : return printSlotNil(str, size);
	case tagFalse : return printSlotFalse(str, size);
	case tagTrue  : return printSlotTrue(str, size);
	case tagPtr   : return printSlotPointer<pointerRawPointer>(slot, str, size);
	default       : return printSlotFloat<floatFullPrecision>(slot, str, size);
	}

	str[0] = 0;
	return -1;
}

int asCompileString(PyrSlot *slot, char *str, size_t size)
{
	using namespace detail;

	size_t printed = 0;
	switch (GetTag(slot)) {
	case tagInt   : printed = printSlotInt<intRaw>(slot, str, size); break;
	case tagChar  : printed = printSlotChar<charCompileString>( slot, str, size); break;
	case tagSym :
	case tagObj :
		return errFailed;
	case tagNil   : printed = printSlotNil(str, size);   break;
	case tagFalse : printed = printSlotFalse(str, size); break;
	case tagTrue  : printed = printSlotTrue(str, size);  break;
	case tagPtr   : printed = printSlotPointer<pointerPrintNil>(slot, str, size);  break;
	default       : printed = printSlotFloat<floatFullPrecision>(slot, str, size); break;
	}
	return errNone;
}


void stringFromPyrString(PyrString *obj, char *str, int maxlength)
{
	if (obj->classptr == class_string) {
		int len;
		if (obj->size > maxlength-4) {
			memcpy(str, obj->s, maxlength-4);
			str[maxlength-4] = '.';
			str[maxlength-3] = '.';
			str[maxlength-2] = '.';
			str[maxlength-1] = 0;
		} else {
			len = sc_min(maxlength-1, obj->size);
			memcpy(str, obj->s, len);
			str[len] = 0;
		}
	} else {
		sprintf(str, "not a string");
	}
}
