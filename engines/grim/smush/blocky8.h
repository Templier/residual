/* Residual - A 3D game interpreter
 *
 * Residual is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
 * $URL$
 * $Id$
 *
 */

#ifndef SMUSH_BLOCKY8_H
#define SMUSH_BLOCKY8_H

#include "common/scummsys.h"

namespace Grim {

class Blocky8 {
private:

	int32 _deltaSize;
	byte *_deltaBufs[2];
	byte *_deltaBuf;
	byte *_curBuf;
	int32 _prevSeqNb;
	int _lastTableWidth;
	const byte *_d_src, *_paramPtr;
	int _d_pitch;
	int32 _offset1, _offset2;
	byte *_tableBig;
	byte *_tableSmall;
	int16 _table[256];
	int32 _frameSize;
	int _width, _height;

	void makeTablesInterpolation(int param);
	void makeTables47(int width);
	void level1(byte *d_dst);
	void level2(byte *d_dst);
	void level3(byte *d_dst);
	void decode2(byte *dst, const byte *src, int width, int height, const byte *param_ptr);

public:
	Blocky8();
	~Blocky8();
	void init(int width, int height);
	void deinit();
	bool decode(byte *dst, const byte *src);
};

} // End of namespace Grim

#endif
