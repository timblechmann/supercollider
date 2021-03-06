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

#include "PyrMessage.h"
#include "PyrPrimitiveProto.h"
#include "PyrInterpreter.h"
#include "PyrPrimitive.h"
#include "PyrListPrim.h"
#include "PyrSymbol.h"
#include "GC.h"
#include <stdlib.h>
#include <assert.h>
#include "PredefinedSymbols.h"
#include "PyrObjectProto.h"
#include "SCBase.h"

#define DEBUGMETHODS 0
#define METHODMETER 0

PyrMethod **gRowTable;

PyrSlot keywordstack[MAXKEYSLOTS];
bool gKeywordError = true;
extern bool gTraceInterpreter;

long cvxUniqueMethods;
extern int ivxIdentDict_array;

void StoreToImmutableB(VMGlobals *g, PyrSlot *& sp, unsigned char *& ip);

void initUniqueMethods()
{
	PyrClass *dummyclass;
	cvxUniqueMethods = classVarOffset("Object", "uniqueMethods", &dummyclass);
}

HOT void sendMessageWithKeys(VMGlobals *g, PyrSymbol *selector, long numArgsPushed, long numKeyArgsPushed)
{
	PyrMethod *meth = NULL;
	PyrMethodRaw *methraw;
	PyrSlot *recvrSlot, *sp;
	PyrClass *classobj;
	long index;
	PyrObject *obj;

	//postfl("->sendMessage\n");
#ifdef GC_SANITYCHECK
	g->gc->SanityCheck();
	CallStackSanity(g, "sendMessageWithKeys");
#endif
	recvrSlot = g->sp - numArgsPushed + 1;

	classobj = classOfSlot(recvrSlot);

	lookup_again:
	index = slotRawInt(&classobj->classIndex) + selector->u.index;
	meth = gRowTable[index];

	if (slotRawSymbol(&meth->name) != selector) {
		doesNotUnderstandWithKeys(g, selector, numArgsPushed, numKeyArgsPushed);
	} else {
		methraw = METHRAW(meth);
		//postfl("methraw->methType %d\n", methraw->methType);
		switch (methraw->methType) {
			case methNormal : /* normal msg send */
				executeMethodWithKeys(g, meth, numArgsPushed, numKeyArgsPushed);
				break;
			case methReturnSelf : /* return self */
				g->sp -= numArgsPushed - 1;
				break;
			case methReturnLiteral : /* return literal */
				sp = g->sp -= numArgsPushed - 1;
				slotCopy(sp, &meth->selectors); /* in this case selectors is just a single value */
				break;
			case methReturnArg : /* return an argument */
				numArgsPushed = keywordFixStack(g, meth, methraw, numArgsPushed, numKeyArgsPushed);
				numKeyArgsPushed = 0;
				g->sp -= numArgsPushed - 1;
				sp = g->sp;
				index = methraw->specialIndex; // zero is index of the first argument
				if (index < numArgsPushed) {
					slotCopy(sp, sp + index);
				} else {
					slotCopy(sp, &slotRawObject(&meth->prototypeFrame)->slots[index]);
				}
				break;
			case methReturnInstVar : /* return inst var */
				sp = g->sp -= numArgsPushed - 1;
				index = methraw->specialIndex;
				slotCopy(sp, &slotRawObject(recvrSlot)->slots[index]);
				break;
			case methAssignInstVar : /* assign inst var */
				sp = g->sp -= numArgsPushed - 1;
				index = methraw->specialIndex;
				obj = slotRawObject(recvrSlot);
				if (obj->IsImmutable()) { StoreToImmutableB(g, sp, g->ip); }
				else {
					if (numArgsPushed >= 2) {
						slotCopy(&obj->slots[index], sp + 1);
						g->gc->GCWrite(obj, sp + 1);
					} else {
						SetNil(&obj->slots[index]);
					}
					slotCopy(sp, recvrSlot);
				}
				break;
			case methReturnClassVar : /* return class var */
				sp = g->sp -= numArgsPushed - 1;
				slotCopy(sp, &g->classvars->slots[methraw->specialIndex]);
				break;
			case methAssignClassVar : /* assign class var */
				sp = g->sp -= numArgsPushed - 1;
				if (numArgsPushed >= 2) {
					slotCopy(&g->classvars->slots[methraw->specialIndex], sp + 1);
					g->gc->GCWrite(g->classvars, sp + 1);
				} else {
					SetNil(&g->classvars->slots[methraw->specialIndex]);
				}
				slotCopy(sp, recvrSlot);
				break;
			case methRedirect : /* send a different selector to self, e.g. this.subclassResponsibility */
				numArgsPushed = keywordFixStack(g, meth, methraw, numArgsPushed, numKeyArgsPushed);
				numKeyArgsPushed = 0;
				selector = slotRawSymbol(&meth->selectors);
				goto lookup_again;
			case methRedirectSuper : /* send a different selector to self, e.g. this.subclassResponsibility */
				numArgsPushed = keywordFixStack(g, meth, methraw, numArgsPushed, numKeyArgsPushed);
				numKeyArgsPushed = 0;
				selector = slotRawSymbol(&meth->selectors);
				classobj = slotRawSymbol(&slotRawClass(&meth->ownerclass)->superclass)->u.classobj;
				goto lookup_again;
			case methForwardInstVar : /* forward to an instance variable */
				numArgsPushed = keywordFixStack(g, meth, methraw, numArgsPushed, numKeyArgsPushed);
				numKeyArgsPushed = 0;
				selector = slotRawSymbol(&meth->selectors);
				index = methraw->specialIndex;
				slotCopy(recvrSlot, &slotRawObject(recvrSlot)->slots[index]);

				classobj = classOfSlot(recvrSlot);

				goto lookup_again;
			case methForwardClassVar : /* forward to a class variable */
				numArgsPushed = keywordFixStack(g, meth, methraw, numArgsPushed, numKeyArgsPushed);
				numKeyArgsPushed = 0;
				selector = slotRawSymbol(&meth->selectors);
				slotCopy(recvrSlot, &g->classvars->slots[methraw->specialIndex]);

				classobj = classOfSlot(recvrSlot);

				goto lookup_again;
			case methPrimitive : /* primitive */
				doPrimitiveWithKeys(g, meth, numArgsPushed, numKeyArgsPushed);
#ifdef GC_SANITYCHECK
	g->gc->SanityCheck();
#endif
				break;
		}
	}
#if TAILCALLOPTIMIZE
	g->tailCall = 0;
#endif
#ifdef GC_SANITYCHECK
	g->gc->SanityCheck();
	CallStackSanity(g, "<sendMessageWithKeys");
#endif
	//postfl("<-sendMessage\n");
}


HOT void sendMessage(VMGlobals *g, PyrSymbol *selector, long numArgsPushed)
{
	PyrMethod *meth = NULL;
	PyrMethodRaw *methraw;
	PyrSlot *recvrSlot, *sp;
	PyrClass *classobj;
	long index;
	PyrObject *obj;

	//postfl("->sendMessage\n");
#ifdef GC_SANITYCHECK
	g->gc->SanityCheck();
	CallStackSanity(g, "sendMessage");
#endif
	recvrSlot = g->sp - numArgsPushed + 1;

	classobj = classOfSlot(recvrSlot);

	lookup_again:
	index = slotRawInt(&classobj->classIndex) + selector->u.index;
	meth = gRowTable[index];

	if (slotRawSymbol(&meth->name) != selector) {
		doesNotUnderstand(g, selector, numArgsPushed);
	} else {
		methraw = METHRAW(meth);
		//postfl("methraw->methType %d\n", methraw->methType);
		switch (methraw->methType) {
			case methNormal : /* normal msg send */
				executeMethod(g, meth, numArgsPushed);
				break;
			case methReturnSelf : /* return self */
				g->sp -= numArgsPushed - 1;
				break;
			case methReturnLiteral : /* return literal */
				sp = g->sp -= numArgsPushed - 1;
				slotCopy(sp, &meth->selectors); /* in this case selectors is just a single value */
				break;
			case methReturnArg : /* return an argument */
				sp = g->sp -= numArgsPushed - 1;
				index = methraw->specialIndex; // zero is index of the first argument
				if (index < numArgsPushed) {
					slotCopy(sp, sp + index);
				} else {
					slotCopy(sp, &slotRawObject(&meth->prototypeFrame)->slots[index]);
				}
				break;
			case methReturnInstVar : /* return inst var */
				sp = g->sp -= numArgsPushed - 1;
				index = methraw->specialIndex;
				slotCopy(sp, &slotRawObject(recvrSlot)->slots[index]);
				break;
			case methAssignInstVar : /* assign inst var */
				sp = g->sp -= numArgsPushed - 1;
				index = methraw->specialIndex;
				obj = slotRawObject(recvrSlot);
				if (obj->IsImmutable()) { StoreToImmutableB(g, sp, g->ip); }
				else {
					if (numArgsPushed >= 2) {
						slotCopy(&obj->slots[index], sp + 1);
						g->gc->GCWrite(obj, sp + 1);
					} else {
						SetNil(&obj->slots[index]);
					}
					slotCopy(sp, recvrSlot);
				}
				break;
			case methReturnClassVar : /* return class var */
				sp = g->sp -= numArgsPushed - 1;
				slotCopy(sp, &g->classvars->slots[methraw->specialIndex]);
				break;
			case methAssignClassVar : /* assign class var */
				sp = g->sp -= numArgsPushed - 1;
				if (numArgsPushed >= 2) {
					slotCopy(&g->classvars->slots[methraw->specialIndex], sp + 1);
					g->gc->GCWrite(g->classvars, sp + 1);
				} else {
					SetNil(&g->classvars->slots[methraw->specialIndex]);
				}
				slotCopy(sp, recvrSlot);
				break;
			case methRedirect : /* send a different selector to self, e.g. this.subclassResponsibility */
				if (numArgsPushed < methraw->numargs) { // not enough args pushed
					/* push default arg values */
					PyrSlot *pslot, *qslot;
					long m, mmax;
					pslot = g->sp;
					qslot = slotRawObject(&meth->prototypeFrame)->slots + numArgsPushed - 1;
					for (m=0, mmax=methraw->numargs - numArgsPushed; m<mmax; ++m) slotCopy(++pslot, ++qslot);
					numArgsPushed = methraw->numargs;
					g->sp += mmax;
				}
				selector = slotRawSymbol(&meth->selectors);
				goto lookup_again;
			case methRedirectSuper : /* send a different selector to self, e.g. this.subclassResponsibility */
				if (numArgsPushed < methraw->numargs) { // not enough args pushed
					/* push default arg values */
					PyrSlot *pslot, *qslot;
					long m, mmax;
					pslot = g->sp;
					qslot = slotRawObject(&meth->prototypeFrame)->slots + numArgsPushed - 1;
					for (m=0, mmax=methraw->numargs - numArgsPushed; m<mmax; ++m) slotCopy(++pslot, ++qslot);
					numArgsPushed = methraw->numargs;
					g->sp += mmax;
				}
				selector = slotRawSymbol(&meth->selectors);
				classobj = slotRawSymbol(&slotRawClass(&meth->ownerclass)->superclass)->u.classobj;
				goto lookup_again;
			case methForwardInstVar : /* forward to an instance variable */
				if (numArgsPushed < methraw->numargs) { // not enough args pushed
					/* push default arg values */
					PyrSlot *pslot, *qslot;
					long m, mmax;
					pslot = g->sp;
					qslot = slotRawObject(&meth->prototypeFrame)->slots + numArgsPushed - 1;
					for (m=0, mmax=methraw->numargs - numArgsPushed; m<mmax; ++m) slotCopy(++pslot, ++qslot);
					numArgsPushed = methraw->numargs;
					g->sp += mmax;
				}
				selector = slotRawSymbol(&meth->selectors);
				index = methraw->specialIndex;
				slotCopy(recvrSlot, &slotRawObject(recvrSlot)->slots[index]);

				classobj = classOfSlot(recvrSlot);

				goto lookup_again;
			case methForwardClassVar : /* forward to a class variable */
				if (numArgsPushed < methraw->numargs) { // not enough args pushed
					/* push default arg values */
					PyrSlot *pslot, *qslot;
					long m, mmax;
					pslot = g->sp;
					qslot = slotRawObject(&meth->prototypeFrame)->slots + numArgsPushed - 1;
					for (m=0, mmax=methraw->numargs - numArgsPushed; m<mmax; ++m) slotCopy(++pslot, ++qslot);
					numArgsPushed = methraw->numargs;
					g->sp += mmax;
				}
				selector = slotRawSymbol(&meth->selectors);
				slotCopy(recvrSlot, &g->classvars->slots[methraw->specialIndex]);

				classobj = classOfSlot(recvrSlot);

				goto lookup_again;
			case methPrimitive : /* primitive */
				doPrimitive(g, meth, numArgsPushed);
#ifdef GC_SANITYCHECK
	g->gc->SanityCheck();
#endif
				break;
			/*
			case methMultDispatchByClass : {
				index = methraw->specialIndex;
				if (index < numArgsPushed) {
					classobj = slotRawObject(sp + index)->classptr;
					selector = slotRawSymbol(&meth->selectors);
					goto lookup_again;
				} else {
					doesNotUnderstand(g, selector, numArgsPushed);
				}
			} break;
			case methMultDispatchByValue : {
				index = methraw->specialIndex;
				if (index < numArgsPushed) {
					index = arrayAtIdentityHashInPairs(array, b);
					meth = slotRawObject(&meth->selectors)->slots[index + 1].uom;
					goto meth_select_again;
				} else {
					doesNotUnderstand(g, selector, numArgsPushed);
				}
			} break;
			*/

		}
	}
#if TAILCALLOPTIMIZE
	g->tailCall = 0;
#endif
#ifdef GC_SANITYCHECK
	g->gc->SanityCheck();
	CallStackSanity(g, "<sendMessage");
#endif
	//postfl("<-sendMessage\n");
}


HOT void sendSuperMessageWithKeys(VMGlobals *g, PyrSymbol *selector, long numArgsPushed, long numKeyArgsPushed)
{
	PyrMethod *meth = NULL;
	PyrMethodRaw *methraw;
	PyrSlot *recvrSlot, *sp;
	PyrClass *classobj;
	long index;
	PyrObject *obj;

	//postfl("->sendMessage\n");
#ifdef GC_SANITYCHECK
	g->gc->SanityCheck();
	CallStackSanity(g, "sendSuperMessageWithKeys");
#endif
	recvrSlot = g->sp - numArgsPushed + 1;

	classobj = slotRawSymbol(&slotRawClass(&g->method->ownerclass)->superclass)->u.classobj;
	//assert(isKindOfSlot(recvrSlot, classobj));

	lookup_again:
	index = slotRawInt(&classobj->classIndex) + selector->u.index;
	meth = gRowTable[index];

	if (slotRawSymbol(&meth->name) != selector) {
		doesNotUnderstandWithKeys(g, selector, numArgsPushed, numKeyArgsPushed);
	} else {
		methraw = METHRAW(meth);
		//postfl("methraw->methType %d\n", methraw->methType);
		switch (methraw->methType) {
			case methNormal : /* normal msg send */
				executeMethodWithKeys(g, meth, numArgsPushed, numKeyArgsPushed);
				break;
			case methReturnSelf : /* return self */
				g->sp -= numArgsPushed - 1;
				break;
			case methReturnLiteral : /* return literal */
				sp = g->sp -= numArgsPushed - 1;
				slotCopy(sp, &meth->selectors); /* in this case selectors is just a single value */
				break;
			case methReturnArg : /* return an argument */
				numArgsPushed = keywordFixStack(g, meth, methraw, numArgsPushed, numKeyArgsPushed);
				numKeyArgsPushed = 0;
				g->sp -= numArgsPushed - 1;
				sp = g->sp;
				index = methraw->specialIndex; // zero is index of the first argument
				if (index < numArgsPushed) {
					slotCopy(sp, sp + index);
				} else {
					slotCopy(sp, &slotRawObject(&meth->prototypeFrame)->slots[index]);
				}
				break;
			case methReturnInstVar : /* return inst var */
				sp = g->sp -= numArgsPushed - 1;
				index = methraw->specialIndex;
				slotCopy(sp, &slotRawObject(recvrSlot)->slots[index]);
				break;
			case methAssignInstVar : /* assign inst var */
				sp = g->sp -= numArgsPushed - 1;
				index = methraw->specialIndex;
				obj = slotRawObject(recvrSlot);
				if (obj->IsImmutable()) { StoreToImmutableB(g, sp, g->ip); }
				else {
					if (numArgsPushed >= 2) {
						slotCopy(&obj->slots[index], sp + 1);
						g->gc->GCWrite(obj, sp + 1);
					} else {
						SetNil(&obj->slots[index]);
					}
					slotCopy(sp, recvrSlot);
				}
				break;
			case methReturnClassVar : /* return class var */
				sp = g->sp -= numArgsPushed - 1;
				slotCopy(sp, &g->classvars->slots[methraw->specialIndex]);
				break;
			case methAssignClassVar : /* assign class var */
				sp = g->sp -= numArgsPushed - 1;
				if (numArgsPushed >= 2) {
					slotCopy(&g->classvars->slots[methraw->specialIndex], sp + 1);
					g->gc->GCWrite(g->classvars, sp + 1);
				} else {
					SetNil(&g->classvars->slots[methraw->specialIndex]);
				}
				slotCopy(sp, recvrSlot);
				break;
			case methRedirect : /* send a different selector to self, e.g. this.subclassResponsibility */
				numArgsPushed = keywordFixStack(g, meth, methraw, numArgsPushed, numKeyArgsPushed);
				numKeyArgsPushed = 0;
				selector = slotRawSymbol(&meth->selectors);
				goto lookup_again;
			case methRedirectSuper : /* send a different selector to self, e.g. this.subclassResponsibility */
				numArgsPushed = keywordFixStack(g, meth, methraw, numArgsPushed, numKeyArgsPushed);
				numKeyArgsPushed = 0;
				selector = slotRawSymbol(&meth->selectors);
				classobj = slotRawSymbol(&slotRawClass(&meth->ownerclass)->superclass)->u.classobj;
				goto lookup_again;
			case methForwardInstVar : /* forward to an instance variable */
				numArgsPushed = keywordFixStack(g, meth, methraw, numArgsPushed, numKeyArgsPushed);
				numKeyArgsPushed = 0;
				selector = slotRawSymbol(&meth->selectors);
				index = methraw->specialIndex;
				slotCopy(recvrSlot, &slotRawObject(recvrSlot)->slots[index]);

				classobj = classOfSlot(recvrSlot);

				goto lookup_again;
			case methForwardClassVar : /* forward to a class variable */
				numArgsPushed = keywordFixStack(g, meth, methraw, numArgsPushed, numKeyArgsPushed);
				numKeyArgsPushed = 0;
				selector = slotRawSymbol(&meth->selectors);
				slotCopy(recvrSlot, &g->classvars->slots[methraw->specialIndex]);

				classobj = classOfSlot(recvrSlot);

				goto lookup_again;
			case methPrimitive : /* primitive */
				doPrimitiveWithKeys(g, meth, numArgsPushed, numKeyArgsPushed);
#ifdef GC_SANITYCHECK
	g->gc->SanityCheck();
#endif
				break;
		}
	}
#if TAILCALLOPTIMIZE
	g->tailCall = 0;
#endif
#ifdef GC_SANITYCHECK
	g->gc->SanityCheck();
	CallStackSanity(g, "<sendSuperMessageWithKeys");
#endif
	//postfl("<-sendMessage\n");
}


HOT void sendSuperMessage(VMGlobals *g, PyrSymbol *selector, long numArgsPushed)
{
	PyrMethod *meth = NULL;
	PyrMethodRaw *methraw;
	PyrSlot *recvrSlot, *sp;
	PyrClass *classobj;
	long index;
	PyrObject *obj;

	//postfl("->sendMessage\n");
#ifdef GC_SANITYCHECK
	g->gc->SanityCheck();
	CallStackSanity(g, "sendSuperMessage");
#endif
	recvrSlot = g->sp - numArgsPushed + 1;

	classobj = slotRawSymbol(&slotRawClass(&g->method->ownerclass)->superclass)->u.classobj;
	//assert(isKindOfSlot(recvrSlot, classobj));

	lookup_again:
	index = slotRawInt(&classobj->classIndex) + selector->u.index;
	meth = gRowTable[index];

	if (slotRawSymbol(&meth->name) != selector) {
		doesNotUnderstand(g, selector, numArgsPushed);
	} else {
		methraw = METHRAW(meth);
		//postfl("methraw->methType %d\n", methraw->methType);
		switch (methraw->methType) {
			case methNormal : /* normal msg send */
				executeMethod(g, meth, numArgsPushed);
				break;
			case methReturnSelf : /* return self */
				g->sp -= numArgsPushed - 1;
				break;
			case methReturnLiteral : /* return literal */
				sp = g->sp -= numArgsPushed - 1;
				slotCopy(sp, &meth->selectors); /* in this case selectors is just a single value */
				break;
			case methReturnArg : /* return an argument */
				sp = g->sp -= numArgsPushed - 1;
				index = methraw->specialIndex; // zero is index of the first argument
				if (index < numArgsPushed) {
					slotCopy(sp, sp + index);
				} else {
					slotCopy(sp, &slotRawObject(&meth->prototypeFrame)->slots[index]);
				}
				break;
			case methReturnInstVar : /* return inst var */
				sp = g->sp -= numArgsPushed - 1;
				index = methraw->specialIndex;
				slotCopy(sp, &slotRawObject(recvrSlot)->slots[index]);
				break;
			case methAssignInstVar : /* assign inst var */
				sp = g->sp -= numArgsPushed - 1;
				index = methraw->specialIndex;
				obj = slotRawObject(recvrSlot);
				if (obj->IsImmutable()) { StoreToImmutableB(g, sp, g->ip); }
				else {
					if (numArgsPushed >= 2) {
						slotCopy(&obj->slots[index], sp + 1);
						g->gc->GCWrite(obj, sp + 1);
					} else {
						SetNil(&obj->slots[index]);
					}
					slotCopy(sp, recvrSlot);
				}
				break;
			case methReturnClassVar : /* return class var */
				sp = g->sp -= numArgsPushed - 1;
				slotCopy(sp, &g->classvars->slots[methraw->specialIndex]);
				break;
			case methAssignClassVar : /* assign class var */
				sp = g->sp -= numArgsPushed - 1;
				if (numArgsPushed >= 2) {
					slotCopy(&g->classvars->slots[methraw->specialIndex], sp + 1);
					g->gc->GCWrite(g->classvars, sp + 1);
				} else {
					SetNil(&g->classvars->slots[methraw->specialIndex]);
				}
				slotCopy(sp, recvrSlot);
				break;
			case methRedirect : /* send a different selector to self, e.g. this.subclassResponsibility */
				if (numArgsPushed < methraw->numargs) { // not enough args pushed
					/* push default arg values */
					PyrSlot *qslot = slotRawObject(&meth->prototypeFrame)->slots + numArgsPushed;
					slotCopy( g->sp + 1, qslot, methraw->numargs - numArgsPushed );
					numArgsPushed = methraw->numargs;
					g->sp += methraw->numargs - numArgsPushed;
				}
				selector = slotRawSymbol(&meth->selectors);
				goto lookup_again;
			case methRedirectSuper : /* send a different selector to self, e.g. this.subclassResponsibility */
				if (numArgsPushed < methraw->numargs) { // not enough args pushed
					/* push default arg values */
					PyrSlot *qslot = slotRawObject(&meth->prototypeFrame)->slots + numArgsPushed;
					slotCopy( g->sp + 1, qslot, methraw->numargs - numArgsPushed );
					numArgsPushed = methraw->numargs;
					g->sp += methraw->numargs - numArgsPushed;
				}
				selector = slotRawSymbol(&meth->selectors);
				classobj = slotRawSymbol(&slotRawClass(&meth->ownerclass)->superclass)->u.classobj;
				goto lookup_again;
			case methForwardInstVar : /* forward to an instance variable */
				if (numArgsPushed < methraw->numargs) { // not enough args pushed
					/* push default arg values */
					PyrSlot *qslot = slotRawObject(&meth->prototypeFrame)->slots + numArgsPushed;
					slotCopy( g->sp + 1, qslot, methraw->numargs - numArgsPushed );
					numArgsPushed = methraw->numargs;
					g->sp += methraw->numargs - numArgsPushed;
				}
				selector = slotRawSymbol(&meth->selectors);
				index = methraw->specialIndex;
				slotCopy(recvrSlot, &slotRawObject(recvrSlot)->slots[index]);

				classobj = classOfSlot(recvrSlot);

				goto lookup_again;
			case methForwardClassVar : /* forward to a class variable */
				if (numArgsPushed < methraw->numargs) { // not enough args pushed
					/* push default arg values */
					/* push default arg values */
					PyrSlot *qslot = slotRawObject(&meth->prototypeFrame)->slots + numArgsPushed;
					slotCopy( g->sp + 1, qslot, methraw->numargs - numArgsPushed );
					numArgsPushed = methraw->numargs;
					g->sp += methraw->numargs - numArgsPushed;
				}
				selector = slotRawSymbol(&meth->selectors);
				slotCopy(recvrSlot, &g->classvars->slots[methraw->specialIndex]);

				classobj = classOfSlot(recvrSlot);

				goto lookup_again;
			case methPrimitive : /* primitive */
				doPrimitive(g, meth, numArgsPushed);
#ifdef GC_SANITYCHECK
	g->gc->SanityCheck();
#endif
				break;
			/*
			case methMultDispatchByClass : {
				index = methraw->specialIndex;
				if (index < numArgsPushed) {
					classobj = slotRawObject(sp + index)->classptr;
					selector = slotRawSymbol(&meth->selectors);
					goto lookup_again;
				} else {
					doesNotUnderstand(g, selector, numArgsPushed);
				}
			} break;
			case methMultDispatchByValue : {
				index = methraw->specialIndex;
				if (index < numArgsPushed) {
					index = arrayAtIdentityHashInPairs(array, b);
					meth = slotRawObject(&meth->selectors)->slots[index + 1].uom;
					goto meth_select_again;
				} else {
					doesNotUnderstand(g, selector, numArgsPushed);
				}
			} break;
			*/

		}
	}
#if TAILCALLOPTIMIZE
	g->tailCall = 0;
#endif
#ifdef GC_SANITYCHECK
	g->gc->SanityCheck();
	CallStackSanity(g, "<sendSuperMessage");
#endif
	//postfl("<-sendMessage\n");
}


extern PyrClass *class_identdict;
void doesNotUnderstandWithKeys(VMGlobals *g, PyrSymbol *selector,
	long numArgsPushed, long numKeyArgsPushed)
{
	PyrSlot *qslot, *pslot, *pend;
	long i, index;
	PyrSlot *uniqueMethodSlot, *arraySlot, *recvrSlot, *selSlot, *slot;
	PyrClass *classobj;
	PyrMethod *meth;
	PyrObject *array;

#ifdef GC_SANITYCHECK
	g->gc->SanityCheck();
#endif
	// move args up by one to make room for selector
	qslot = g->sp + 1;
	pslot = g->sp + 2;
	pend = pslot - numArgsPushed + 1;
	while (pslot > pend) *--pslot = *--qslot;

	selSlot = g->sp - numArgsPushed + 2;
	SetSymbol(selSlot, selector);
	g->sp++;

	recvrSlot = selSlot - 1;

	classobj = classOfSlot(recvrSlot);

	index = slotRawInt(&classobj->classIndex) + s_doesNotUnderstand->u.index;
	meth = gRowTable[index];


	if (slotRawClass(&meth->ownerclass) == class_object) {
		// lookup instance specific method
		uniqueMethodSlot = &g->classvars->slots[cvxUniqueMethods];
		if (isKindOfSlot(uniqueMethodSlot, class_identdict)) {
			arraySlot = slotRawObject(uniqueMethodSlot)->slots + ivxIdentDict_array;
			if ((IsObj(arraySlot) && (array = slotRawObject(arraySlot))->classptr == class_array)) {
				i = arrayAtIdentityHashInPairs(array, recvrSlot);
				if (i >= 0) {
					slot = array->slots + i;
					if (NotNil(slot)) {
						++slot;
						if (isKindOfSlot(slot, class_identdict)) {
							arraySlot = slotRawObject(slot)->slots + ivxIdentDict_array;
							if ((IsObj(arraySlot) && (array = slotRawObject(arraySlot))->classptr == class_array)) {
								i = arrayAtIdentityHashInPairs(array, selSlot);
								if (i >= 0) {
									slot = array->slots + i;
									if (NotNil(slot)) {
										++slot;
										slotCopy(selSlot, recvrSlot);
										slotCopy(recvrSlot, slot);
										blockValueWithKeys(g, numArgsPushed+1, numKeyArgsPushed);
										return;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	executeMethodWithKeys(g, meth, numArgsPushed+1, numKeyArgsPushed);

#ifdef GC_SANITYCHECK
	g->gc->SanityCheck();
#endif
}

int blockValue(struct VMGlobals *g, int numArgsPushed);

void doesNotUnderstand(VMGlobals *g, PyrSymbol *selector,
	long numArgsPushed)
{
	PyrSlot *qslot, *pslot, *pend;
	long i, index;
	PyrSlot *uniqueMethodSlot, *arraySlot, *recvrSlot, *selSlot, *slot;
	PyrClass *classobj;
	PyrMethod *meth;
	PyrObject *array;

#ifdef GC_SANITYCHECK
	g->gc->SanityCheck();
#endif
	// move args up by one to make room for selector
	qslot = g->sp + 1;
	pslot = g->sp + 2;
	pend = pslot - numArgsPushed + 1;
	while (pslot > pend) *--pslot = *--qslot;

	selSlot = g->sp - numArgsPushed + 2;
	SetSymbol(selSlot, selector);
	g->sp++;

	recvrSlot = selSlot - 1;

	classobj = classOfSlot(recvrSlot);

	index = slotRawInt(&classobj->classIndex) + s_doesNotUnderstand->u.index;
	meth = gRowTable[index];


	if (slotRawClass(&meth->ownerclass) == class_object) {
		// lookup instance specific method
		uniqueMethodSlot = &g->classvars->slots[cvxUniqueMethods];
		if (isKindOfSlot(uniqueMethodSlot, class_identdict)) {
			arraySlot = slotRawObject(uniqueMethodSlot)->slots + ivxIdentDict_array;
			if ((IsObj(arraySlot) && (array = slotRawObject(arraySlot))->classptr == class_array)) {
				i = arrayAtIdentityHashInPairs(array, recvrSlot);
				if (i >= 0) {
					slot = array->slots + i;
					if (NotNil(slot)) {
						++slot;
						if (isKindOfSlot(slot, class_identdict)) {
							arraySlot = slotRawObject(slot)->slots + ivxIdentDict_array;
							if ((IsObj(arraySlot) && (array = slotRawObject(arraySlot))->classptr == class_array)) {
								i = arrayAtIdentityHashInPairs(array, selSlot);
								if (i >= 0) {
									slot = array->slots + i;
									if (NotNil(slot)) {
										++slot;
										slotCopy(selSlot, recvrSlot);
										slotCopy(recvrSlot, slot);
										blockValue(g, numArgsPushed+1);
										return;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	executeMethod(g, meth, numArgsPushed+1);

#ifdef GC_SANITYCHECK
	g->gc->SanityCheck();
#endif
}

template <bool WithKey>
HOT void doExecuteMethod(VMGlobals *g, PyrMethod *meth, long allArgsPushed, long numKeyArgsPushed)
{
#ifdef GC_SANITYCHECK
	g->gc->SanityCheck();
	CallStackSanity(g, "doExecuteMethod");
#endif
#if DEBUGMETHODS
	if (gTraceInterpreter) {
		if (g->method) {
			postfl(" %s:%s -> %s:%s\n",
				slotRawSymbol(&slotRawClass(&g->method->ownerclass)->name)->name, slotRawSymbol(&g->method->name)->name,
				slotRawSymbol(&slotRawClass(&meth->ownerclass)->name)->name, slotRawSymbol(&meth->name)->name);
		} else {
			postfl(" top -> %s:%s\n",
				slotRawSymbol(&slotRawClass(&meth->ownerclass)->name)->name, slotRawSymbol(&meth->name)->name);
		}
	}
#endif
#if METHODMETER
	if (gTraceInterpreter) {
		slotRawInt(&meth->callMeter)++;
	}
#endif

#if TAILCALLOPTIMIZE
	int tailCall = g->tailCall;
	if (tailCall) {
		if (tailCall == 1) {
			returnFromMethod(g);
		} else {
			returnFromBlock(g);
		}
	}
#endif


	PyrObject * proto = slotRawObject(&meth->prototypeFrame);
	PyrMethodRaw * methraw = METHRAW(meth);
	const long numtemps = methraw->numtemps;
	const long numargs = methraw->numargs;
	PyrFrame * caller = g->frame;
	const long numArgsPushed = allArgsPushed - (numKeyArgsPushed<<1);
	//DumpStack(g, g->sp);
	//postfl("executeMethod allArgsPushed %d numKeyArgsPushed %d\n", allArgsPushed, numKeyArgsPushed);

	PyrFrame * frame = (PyrFrame*)g->gc->NewFrame(methraw->frameSize, 0, obj_slot, methraw->needsHeapContext);
	PyrSlot * vars = frame->vars;
	frame->classptr = class_frame;
	frame->size = FRAMESIZE + proto->size;
	SetObject(&frame->method, meth);
	SetObject(&frame->homeContext, frame);
	SetObject(&frame->context, frame);

	if (caller) {
		SetPtr(&caller->ip, g->ip);
		SetObject(&frame->caller, caller);
	} else {
		SetInt(&frame->caller, 0);
	}
	SetPtr(&frame->ip,  0);

	g->method     = meth;
	g->ip         = slotRawInt8Array(&meth->code)->b - 1;
	g->frame      = frame;
	g->block      = meth;
	g->sp        -= allArgsPushed;
	g->execMethod = WithKey ? 10 : 20;

	/* push all (normal) args to frame */
	slotCopy( vars, g->sp + 1, std::min<long>( numArgsPushed, numargs ) );

	if (numArgsPushed <= numargs) {	/* not enough args pushed */
		/* push default arg & var values */
		slotCopy( vars + numArgsPushed, proto->slots + numArgsPushed, numtemps - numArgsPushed );
	} else {
		if (methraw->varargs) {
			/* push list */
			long i = numArgsPushed - numargs;
			PyrObject * list = newPyrArray(g->gc, i, 0, false);
			list->size = i;

			SetObject(vars + numargs, list);

			/* put extra args into list */
			// fixed and raw sizes are zero
			slotCopy( list->slots, g->sp + numargs + 1, i );
		}

		if (methraw->numvars) {
			int varArgOffset = methraw->varargs ? 1 : 0;

			/* push default keyword and var values */
			slotCopy( vars + numargs + varArgOffset, proto->slots + numargs + varArgOffset, methraw->numvars );
		}
	}

	if( WithKey ) {
		// do keyword lookup:
		if (numKeyArgsPushed && methraw->posargs) {
			PyrSymbol **name0 = slotRawSymbolArray(&meth->argNames)->symbols + 1;
			PyrSlot * key = g->sp + numArgsPushed + 1;
			for (long i=0; i<numKeyArgsPushed; ++i, key+=2) {
				PyrSymbol ** name = name0;
				for (long j=1; j<methraw->posargs; ++j, ++name) {
					if (*name == slotRawSymbol(key)) {
						slotCopy( vars + j, key + 1);
						goto found1;
					}
				}
				if ( BOOST_UNLIKELY( gKeywordError) ) {
					post("WARNING: keyword arg '%s' not found in call to %s:%s\n",
						 slotRawSymbol(key)->name, slotRawSymbol(&slotRawClass(&meth->ownerclass)->name)->name, slotRawSymbol(&meth->name)->name);
				}
			found1:
				;
			}
		}
	}

	slotCopy( &g->receiver, vars );
#ifdef GC_SANITYCHECK
	g->gc->SanityCheck();
	CallStackSanity(g, "<doExecuteMethod");
#endif
}

HOT FLATTEN void executeMethodWithKeys(VMGlobals *g, PyrMethod *meth, long allArgsPushed, long numKeyArgsPushed)
{
	doExecuteMethod<true>( g, meth, allArgsPushed, numKeyArgsPushed );
}

HOT FLATTEN void executeMethod(VMGlobals *g, PyrMethod *meth, long numArgsPushed)
{
	doExecuteMethod<false>( g, meth, numArgsPushed, 0 );
}

void switchToThread(VMGlobals *g, PyrThread *newthread, int oldstate, int *numArgsPushed);

HOT void returnFromBlock(VMGlobals *g)
{
	PyrFrame *curframe;
	PyrFrame *returnFrame;
	PyrFrame *homeContext;
	PyrBlock *block;
	PyrMethod *meth;
	PyrMethodRaw *methraw;
	PyrMethodRaw *blockraw;

	//if (gTraceInterpreter) postfl("->returnFromBlock\n");
	//printf("->returnFromBlock\n");
#ifdef GC_SANITYCHECK
	g->gc->SanityCheck();
	CallStackSanity(g, "returnFromBlock");
#endif
	curframe = g->frame;

//again:
	returnFrame = slotRawFrame(&curframe->caller);
	if (returnFrame) {
		block = slotRawBlock(&curframe->method);
		blockraw = METHRAW(block);

		g->frame = returnFrame;
		g->ip = (unsigned char *)slotRawPtr(&returnFrame->ip);
		g->block = slotRawBlock(&returnFrame->method);
		homeContext = slotRawFrame(&returnFrame->homeContext);
		meth = slotRawMethod(&homeContext->method);
		methraw = METHRAW(meth);
		slotCopy(&g->receiver, &homeContext->vars[0]); //??
		g->method = meth;

		meth = slotRawMethod(&curframe->method);
		methraw = METHRAW(meth);
		if (!methraw->needsHeapContext) {
			g->gc->Free(curframe);
		} else {
			SetInt(&curframe->caller, 0);
		}

	} else {
		////// this should never happen .
		error("return from Function at top of call stack.\n");
		g->method = NULL;
		g->block = NULL;
		g->frame = NULL;
		g->sp = g->gc->Stack()->slots - 1;
		longjmp(g->escapeInterpreter, 1);
	}
	//if (gTraceInterpreter) postfl("<-returnFromBlock\n");
#ifdef GC_SANITYCHECK
	g->gc->SanityCheck();
	CallStackSanity(g, "returnFromBlock");
#endif
}


HOT void returnFromMethod(VMGlobals *g)
{
	PyrFrame *returnFrame, *curframe, *homeContext;
	PyrMethod *meth;
	PyrMethodRaw *methraw;
	curframe = g->frame;

	//assert(slotRawFrame(&curframe->context) == NULL);

	/*if (gTraceInterpreter) {
		post("returnFromMethod %s:%s\n", slotRawClass(&g->method->ownerclass)->name.us->name, g->slotRawSymbol(&method->name)->name);
		post("tailcall %d\n", g->tailCall);
	}*/
#ifdef GC_SANITYCHECK
	g->gc->SanityCheck();
#endif
	homeContext = slotRawFrame(&slotRawFrame(&curframe->context)->homeContext);
	if (homeContext == NULL) {
		null_return:
#if TAILCALLOPTIMIZE
		if (g->tailCall) return; // do nothing.
#endif

	/*
		static bool once = true;
		if (once || gTraceInterpreter)
		{
			once = false;
			post("return all the way out. sd %d\n", g->sp - g->gc->Stack()->slots);
			postfl("%s:%s\n",
				slotRawClass(&g->method->ownerclass)->name.us->name, g->slotRawSymbol(&method->name)->name
			);
			post("tailcall %d\n", g->tailCall);
			post("homeContext %p\n", homeContext);
			post("returnFrame %p\n", returnFrame);
			dumpObjectSlot(&homeContext->caller);
			DumpStack(g, g->sp);
			DumpBackTrace(g);
		}
		gTraceInterpreter = false;
	*/
		//if (IsNil(&homeContext->caller)) return; // do nothing.

		// return all the way out.
		PyrSlot *bottom = g->gc->Stack()->slots;
		slotCopy(bottom, g->sp);
		g->sp = bottom; // ??!! pop everybody
		g->method = NULL;
		g->block = NULL;
		g->frame = NULL;
		longjmp(g->escapeInterpreter, 2);
	} else {
		returnFrame = slotRawFrame(&homeContext->caller);

		if (returnFrame == NULL) goto null_return;
		// make sure returnFrame is a caller and find earliest stack frame
		{
			PyrFrame *tempFrame = curframe;
			while (tempFrame != returnFrame) {
				tempFrame = slotRawFrame(&tempFrame->caller);
				if (!tempFrame) {
					if (isKindOf((PyrObject*)g->thread, class_routine) && NotNil(&g->thread->parent)) {
						// not found, so yield to parent thread and continue searching.
						PyrSlot value;
						slotCopy(&value, g->sp);

						int numArgsPushed = 1;
						switchToThread(g, slotRawThread(&g->thread->parent), tSuspended, &numArgsPushed);

						// on the other side of the looking glass, put the yielded value on the stack as the result..
						g->sp -= numArgsPushed - 1;
						slotCopy(g->sp, &value);

						curframe = tempFrame = g->frame;
					} else {
						slotCopy(&g->sp[2], &g->sp[0]);
						slotCopy(g->sp, &g->receiver);
						g->sp++; SetObject(g->sp, g->method);
						g->sp++;
						sendMessage(g, getsym("outOfContextReturn"), 3);
						return;
					}
				}
			}
		}

		{
			PyrFrame *tempFrame = curframe;
			while (tempFrame != returnFrame) {
				meth = slotRawMethod(&tempFrame->method);
				methraw = METHRAW(meth);
				PyrFrame *nextFrame = slotRawFrame(&tempFrame->caller);
				if (!methraw->needsHeapContext) {
					SetInt(&tempFrame->caller, 0);
					g->gc->Free(tempFrame);
				} else {
					if (tempFrame != homeContext)
						SetInt(&tempFrame->caller, 0);
				}
				tempFrame = nextFrame;
			}
		}

		// return to it
		g->ip = (unsigned char *)slotRawPtr(&returnFrame->ip);
		g->frame = returnFrame;
		g->block = slotRawBlock(&returnFrame->method);

		homeContext = slotRawFrame(&returnFrame->homeContext);
		meth = slotRawMethod(&homeContext->method);
		methraw = METHRAW(meth);

#if DEBUGMETHODS
if (gTraceInterpreter) {
	postfl("%s:%s <- %s:%s\n",
		slotRawSymbol(&slotRawClass(&meth->ownerclass)->name)->name, slotRawSymbol(&meth->name)->name,
		slotRawSymbol(&slotRawClass(&g->method->ownerclass)->name)->name, slotRawSymbol(&g->method->name)->name
	);
}
#endif

		g->method = meth;
		slotCopy(&g->receiver, &homeContext->vars[0]);

	}
#ifdef GC_SANITYCHECK
	g->gc->SanityCheck();
#endif
}


int keywordFixStack(VMGlobals *g, PyrMethod *meth, PyrMethodRaw *methraw, long allArgsPushed,
		long numKeyArgsPushed)
{
	if (numKeyArgsPushed) {
		PyrSlot *pslot, *qslot;
		// evacuate keyword args to separate area
		pslot = keywordstack + (numKeyArgsPushed<<1);
		qslot = g->sp + 1;
		for (int m=0; m<numKeyArgsPushed; ++m) {
			slotCopy(--pslot, --qslot);
			slotCopy(--pslot, --qslot);
		}
	}

	PyrSlot *vars = g->sp - allArgsPushed + 1;

	long       numArgsPushed = allArgsPushed - (numKeyArgsPushed<<1);
	const long numArgsNeeded = methraw->numargs;
	const long diff          = numArgsNeeded - numArgsPushed;
	if (diff > 0) {  // not enough args
		PyrSlot *pslot, *qslot;
		pslot = vars + numArgsPushed;
		qslot = slotRawObject(&meth->prototypeFrame)->slots + numArgsPushed;

		slotCopy( pslot, qslot, diff );

		numArgsPushed = numArgsNeeded;
	}

	// do keyword lookup:
	if (numKeyArgsPushed && methraw->posargs) {
		PyrSymbol **name0 = slotRawSymbolArray(&meth->argNames)->symbols + 1;
		PyrSlot *key = keywordstack;
		for (int i=0; i<numKeyArgsPushed; ++i, key+=2) {
			PyrSymbol **name = name0;
			for (int j=1; j<methraw->posargs; ++j, ++name) {
				if (*name == slotRawSymbol(key)) {
					slotCopy(&vars[j], &key[1]);
					goto found;
				}
			}
			if (gKeywordError) {
					post("WARNING: keyword arg '%s' not found in call to %s:%s\n",
						slotRawSymbol(key)->name, slotRawSymbol(&slotRawClass(&meth->ownerclass)->name)->name, slotRawSymbol(&meth->name)->name);
			}
			found: ;
		}
	}

	g->sp += numArgsPushed - allArgsPushed;
	return numArgsPushed;
}

