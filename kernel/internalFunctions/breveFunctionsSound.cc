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

#include "kernel.h"

/*! \addtogroup InternalFunctions */
/*@{*/

/*
	\brief Loads a sound file of a given name.

	brSoundData pointer loadSoundFile(string).
*/

#if defined(HAVE_LIBPORTAUDIO) && defined(HAVE_LIBSNDFILE)
int brILoadSoundFile(brEval args[], brEval *target, brInstance *i) {
	char *path;

	path = brFindFile(i->engine, BRSTRING(&args[0]), NULL);

	if(path) {
		BRPOINTER(target) = brLoadSound(path);
		slFree(path);
	} else {
		slMessage(DEBUG_ALL, "warning: cannot locate sound file \"%s\"\n", BRSTRING(&args[0]));
		BRPOINTER(target) = NULL;
	}

	return EC_OK;
}

/*
	\brief Fress sound file data (previously loaded with \ref brILoadSoundFile).

	void freeSoundFile(brSoundData pointer).
*/

int brIFreeSoundFile(brEval args[], brEval *target, brInstance *i) {
	if(BRPOINTER(&args[0])) brFreeSound(BRPOINTER(&args[0]));
	return EC_OK;
}

/*
	\brief Plays sound file data (previously loaded with \ref brILoadSoundFile).

	void playSoundFile(brSoundData pointer).
*/

int brIPlaySoundFile(brEval args[], brEval *target, brInstance *i) {
	brNewPlayer(i->engine->soundMixer, BRPOINTER(&args[0]));
	return EC_OK;
}

/*!
	\brief Creates a sinewave of a given frequency.

	brSoundPlayer pointer newSinewave(int).
*/

int brINewSinewave(brEval args[], brEval *target, brInstance *i) {
	BRPOINTER(target) = brNewSinewave(i->engine->soundMixer, BRINT(&args[0]));
	return EC_OK;
}

/*!
	\brief Stops a sinewave.
*/

int brIFreeSinewave(brEval args[], brEval *target, brInstance *i) {
	brSoundPlayer *p = BRPOINTER(&args[0]);
	p->finished = 1;
	return EC_OK;
}

/*
	\brief Changes the frequency of a sinewave.

	void (brSoundPlayer pointer, int).
*/

int brISetFrequency(brEval args[], brEval *target, brInstance *i) {
	brSoundPlayer *p = BRPOINTER(&args[0]);

	p->frequency = BRINT(&args[1]) * 2. * 3.14159265359 / MIXER_SAMPLE_RATE;

	return EC_OK;
}

/*
	\brief Changes the volumne of a sinewave.

	void setVolume(brSoundPlayer pointer, double).

	Volume is on a scale from 0.0 to 1.0.
*/

int brISetVolume(brEval args[], brEval *target, brInstance *i) {
	brSoundPlayer *p = BRPOINTER(&args[0]);
	p->volume = BRDOUBLE(&args[1]);
	return EC_OK;
}

/*
	\brief Changes the volumne of a sinewave.

	void setBalance(brSoundPlayer pointer, double).

	Balance is on a scale of 0.0 to 1.0, 0.5 is balanced between left and right.
*/

int brISetBalance(brEval args[], brEval *target, brInstance *i) {
	brSoundPlayer *p = BRPOINTER(&args[0]);
	p->balance = BRDOUBLE(&args[1]);
	return EC_OK;
}
#endif 

/*
	\brief Prints an error message when sound functions are not available.

	Used only when breve is compiled without support for sound.
*/

int brISoundUnsupported(brEval args[], brEval *target, brInstance *i) {
	slMessage(DEBUG_ALL, "this version of breve was built without support for sound\n");
	return EC_ERROR;
}
/*@}*/

void breveInitSoundFunctions(brNamespace *n) {
#if defined(HAVE_LIBPORTAUDIO) && defined(HAVE_LIBSNDFILE)
	brNewBreveCall(n, "loadSoundFile", brILoadSoundFile, AT_POINTER, AT_STRING, 0);
	brNewBreveCall(n, "playSoundFile", brIPlaySoundFile, AT_NULL, AT_POINTER, 0);
	brNewBreveCall(n, "freeSoundFile", brIFreeSoundFile, AT_NULL, AT_POINTER, 0);
	brNewBreveCall(n, "newSinewave", brINewSinewave, AT_POINTER, AT_INT, 0);
	brNewBreveCall(n, "freeSinewave", brIFreeSinewave, AT_NULL, AT_POINTER, 0);
	brNewBreveCall(n, "setFrequency", brISetFrequency, AT_NULL, AT_POINTER, AT_INT, 0);
	brNewBreveCall(n, "setVolume", brISetVolume, AT_NULL, AT_POINTER, AT_DOUBLE, 0);
	brNewBreveCall(n, "setBalance", brISetBalance, AT_NULL, AT_POINTER, AT_DOUBLE, 0);
#else
	brNewBreveCall(n, "loadSoundFile", brISoundUnsupported, AT_POINTER, AT_STRING, 0);
	brNewBreveCall(n, "playSoundFile", brISoundUnsupported, AT_NULL, AT_POINTER, 0);
	brNewBreveCall(n, "freeSoundFile", brISoundUnsupported, AT_NULL, AT_POINTER, 0);
	brNewBreveCall(n, "newSinewave", brISoundUnsupported, AT_POINTER, AT_INT, 0);
	brNewBreveCall(n, "freeSinewave", brISoundUnsupported, AT_NULL, AT_POINTER, 0);
	brNewBreveCall(n, "setVolume", brISoundUnsupported, AT_NULL, AT_POINTER, AT_INT, 0);
	brNewBreveCall(n, "setFrequency", brISoundUnsupported, AT_NULL, AT_POINTER, AT_DOUBLE, 0);
#endif
}
