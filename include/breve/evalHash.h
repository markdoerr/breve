/*****************************************************************************
 *                                                                           *
 * The breve Simulation Environment                                          *
 * Copyright (C) 2000, 2001, 2002, 2003 Jonathan Klein                       *
 *                                                                           *
 * This program is free software; you can redistribute it and/or modify      *
 * it under the terms of the GNU General Public License as published by      *
 * the Free Software Foundation; either version 2 of the License, or         *
 * (at your option) any later version.                                       *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU General Public License for more details.                              *
 *                                                                           *
 * You should have received a copy of the GNU General Public License         *
 * along with this program; if not, write to the Free Software               *
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA *
 *****************************************************************************/

/*!
	\brief A steve/breve hash-table datatype.

	A hash-table datatype.  Used mostly for steve programming -- this
	type is not typically used to pass information between the frontend
	and the engine.  In fact, it's not used at all to pass info back and
	forth, but I wouldn't want to say it's never going to happen.
*/

struct brEvalHash {
	slHash *table;
	int retainCount;
};

#ifdef __cplusplus
extern "C" {
#endif

brEvalHash *brEvalHashNew(void);
void brEvalHashFree(brEvalHash *);

brEvalListHead *brEvalHashValues(brEvalHash *);
brEvalListHead *brEvalHashKeys(brEvalHash *);

void stUnretainEvalHash(brEvalHash *);
void stRetainEvalHash(brEvalHash *);

unsigned int brEvalHashFunction(void *, unsigned int);
unsigned int brEvalHashCompareFunction(void *, void *);

void brEvalHashLookup(brEvalHash *, brEval *, brEval *);
void brEvalHashStore(brEvalHash *, brEval *, brEval *, brEval *);

#ifdef __cplusplus
}
#endif