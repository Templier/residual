// Residual - Virtual machine to run LucasArts' 3D adventure games
// Copyright (C) 2003 The ScummVM-Residual Team (www.scummvm.org)
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA

#include "stdafx.h"
#include "lipsynch.h"
#include "bits.h"
#include "debug.h"
#include <cstring>
#include <SDL_endian.h>

// A new define that'll be around when theres a configure script :)
#undef DEBUG_VERBOSE

LipSynch::LipSynch(const char *filename, const char *data, int len) :
	Resource(filename) {
	uint16 readPhoneme;
	int j;

	if (std::memcmp(data, "LIP!", 4) != 0) {
		error("Invalid file format in %s\n", filename);
	} else {
		_numEntries = (len - 8) / 4;

		// There are cases where the lipsync file has no entries
		if (_numEntries == 0) {
			_status = false;
			_entries = NULL;
		}
		else {
		    _status = true;
			data += 8;
#ifdef DEBUG_VERBOSE
			printf("Reading LipSynch %s, %d entries\n", filename, _numEntries);
#endif
			_entries = new LipEntry[_numEntries];
			for (int i = 0; i < _numEntries; i++) {
				_entries[i].frame = READ_LE_UINT16(data);
				readPhoneme = READ_LE_UINT16(data + 2);

				// Look for the animation corresponding to the phoneme
				for (j = 0; j < _animTableSize && readPhoneme != _animTable[j].phoneme; j++);
				if (readPhoneme != _animTable[j].phoneme) {
					warning("Unknown phoneme: 0x%X in file %s\n", readPhoneme, filename);
					_entries[i].anim = 1;
				} else
					_entries[i].anim = _animTable[j].anim;
				data += 4;
			}
#ifdef DEBUG_VERBOSE
				for (int j = 0; j < _numEntries; j++)
					printf("LIP %d) frame %d, anim %d\n", j, _entries[j].frame, _entries[j].anim);
#endif
		    _currEntry = 0;
		}    
	}
}

LipSynch::~LipSynch() {
	delete[] _entries;
}

LipSynch::LipEntry LipSynch::getCurrEntry() {
	return _entries[_currEntry];
}

void LipSynch::advanceEntry() {
	if (_currEntry < _numEntries)
		_currEntry++;
}

const LipSynch::PhonemeAnim LipSynch::_animTable[] = {
	{0x005F, 0}, {0x0251, 1}, {0x0061, 1}, {0x00E6, 1}, {0x028C, 8}, 
	{0x0254, 1}, {0x0259, 1}, {0x0062, 6}, {0x02A7, 2}, {0x0064, 2}, 
	{0x00F0, 5}, {0x025B, 8}, {0x0268, 8}, {0x025A, 9}, {0x025D, 9}, 
	{0x0065, 1}, {0x0066, 4}, {0x0067, 8}, {0x0261, 8}, {0x0068, 8}, 
	{0x026A, 8}, {0x0069, 3}, {0x02A4, 2}, {0x006B, 2}, {0x006C, 5}, 
	{0x026B, 5}, {0x006D, 6}, {0x006E, 8}, {0x014B, 8}, {0x006F, 7}, 
	{0x0070, 6}, {0x0072, 2}, {0x027B, 2}, {0x0279, 2}, {0x0073, 2}, 
	{0x0283, 2}, {0x0074, 2}, {0x027E, 2}, {0x03B8, 5}, {0x028A, 9}, 
	{0x0075, 9}, {0x0076, 4}, {0x0077, 9}, {0x006A, 8}, {0x007A, 2}, 
	{0x0292, 2}, {0x002E, 2}
};

const int LipSynch::_animTableSize = sizeof(LipSynch::_animTable) / sizeof(LipSynch::_animTable[0]);