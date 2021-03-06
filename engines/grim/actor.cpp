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

#include "engines/grim/actor.h"
#include "engines/grim/grim.h"
#include "engines/grim/colormap.h"
#include "engines/grim/costume.h"
#include "engines/grim/lipsync.h"
#include "engines/grim/smush/smush.h"
#include "engines/grim/imuse/imuse.h"
#include "engines/grim/lua.h"

namespace Grim {

int Actor::s_id = 0;

int g_winX1, g_winY1, g_winX2, g_winY2;

Actor::Actor(const char *actorName) :
		Object(), _name(actorName), _setName(""), _talkColor(255, 255, 255), _pos(0, 0, 0),
		// Some actors don't set walk and turn rates, so we default the
		// _turnRate so Doug at the cat races can turn and we set the
		// _walkRate so Glottis at the demon beaver entrance can walk and
		// so Chepito in su.set
		_pitch(0), _yaw(0), _roll(0), _walkRate(0.3f), _turnRate(100.0f),
		_reflectionAngle(80),
		_visible(true), _lipSync(NULL), _turning(false), _walking(false),
		_restCostume(NULL), _restChore(-1),
		_walkCostume(NULL), _walkChore(-1), _walkedLast(false), _walkedCur(false),
		_turnCostume(NULL), _leftTurnChore(-1), _rightTurnChore(-1),
		_lastTurnDir(0), _currTurnDir(0),
		_mumbleCostume(NULL), _mumbleChore(-1), _sayLineText(NULL) {
	_lookingMode = false;
	_lookAtRate = 200;
	_constrain = false;
	_talkSoundName = "";
	_activeShadowSlot = -1;
	_shadowArray = new Shadow[5];
	_winX1 = _winY1 = 1000;
	_winX2 = _winY2 = -1000;
	_toClean = false;
	_lastWasLeft = false;
	_lastStepTime = 0;
	_running = false;

	for (int i = 0; i < 5; i++) {
		_shadowArray[i].active = false;
		_shadowArray[i].dontNegate = false;
		_shadowArray[i].shadowMask = NULL;
		_shadowArray[i].shadowMaskSize = 0;
	}

	for (int i = 0; i < 10; i++) {
		_talkCostume[i] = NULL;
		_talkChore[i] = -1;
	}

	++s_id;
	_id = s_id;

	g_grim->registerActor(this);
}

Actor::Actor() :
	Object() {

	_shadowArray = new Shadow[5];
	_winX1 = _winY1 = 1000;
	_winX2 = _winY2 = -1000;
	_toClean = false;
	_lastWasLeft = false;
	_lastStepTime = 0;
	_running = false;

	for (int i = 0; i < 5; i++) {
		_shadowArray[i].active = false;
		_shadowArray[i].dontNegate = false;
		_shadowArray[i].shadowMask = NULL;
		_shadowArray[i].shadowMaskSize = 0;
	}
}


Actor::~Actor() {
	if (_shadowArray) {
		clearShadowPlanes();
		delete[] _shadowArray;
	}
}

void Actor::saveState(SaveGame *savedState) const {
	// store actor name
	savedState->writeString(_name);
	savedState->writeString(_setName);

	savedState->writeColor(_talkColor);

	savedState->writeVector3d(_pos);

	savedState->writeFloat(_pitch);
	savedState->writeFloat(_yaw);
	savedState->writeFloat(_roll);
	savedState->writeFloat(_walkRate);
	savedState->writeFloat(_turnRate);
	savedState->writeLESint32(_constrain);
	savedState->writeFloat(_reflectionAngle);
	savedState->writeLESint32(_visible);
	savedState->writeLESint32(_lookingMode),

	savedState->writeString(_talkSoundName);

	if (_lipSync) {
		savedState->writeLEUint32(1);
		savedState->writeCharString(_lipSync->getFilename());
	} else {
		savedState->writeLEUint32(0);
	}

	savedState->writeLESint32(_costumeStack.size());
	for (Common::List<Costume *>::const_iterator i = _costumeStack.begin(); i != _costumeStack.end(); ++i) {
		Costume *c = *i;
		savedState->writeCharString(c->getFilename());
		Costume *pc = c->previousCostume();
		int depth = 0;
		while (pc) {
			++depth;
			pc = pc->previousCostume();
		}
		savedState->writeLEUint32(depth);
		pc = c->previousCostume();
		for (int j = 0; j < depth; ++j) { //save the previousCostume hierarchy
			savedState->writeCharString(pc->getFilename());
			pc = pc->previousCostume();
		}
		c->saveState(savedState);
	}

	savedState->writeLESint32(_turning);
	savedState->writeFloat(_destYaw);

	savedState->writeLESint32(_walking);
	savedState->writeVector3d(_destPos);

	if (_restCostume) {
		savedState->writeLEUint32(1);
		savedState->writeCharString(_restCostume->getFilename());
	} else {
		savedState->writeLEUint32(0);
	}
	savedState->writeLESint32(_restChore);

	if (_walkCostume) {
		savedState->writeLEUint32(1);
		savedState->writeCharString(_walkCostume->getFilename());
	} else {
		savedState->writeLEUint32(0);
	}
	savedState->writeLESint32(_walkChore);
	savedState->writeLESint32(_walkedLast);
	savedState->writeLESint32(_walkedCur);

	if (_turnCostume) {
		savedState->writeLEUint32(1);
		savedState->writeCharString(_turnCostume->getFilename());
	} else {
		savedState->writeLEUint32(0);
	}
	savedState->writeLESint32(_leftTurnChore);
	savedState->writeLESint32(_rightTurnChore);
	savedState->writeLESint32(_lastTurnDir);
	savedState->writeLESint32(_currTurnDir);

	for (int i = 0; i < 10; ++i) {
		if (_talkCostume[i]) {
			savedState->writeLEUint32(1);
			savedState->writeCharString(_talkCostume[i]->getFilename());
		} else {
			savedState->writeLEUint32(0);
		}
		savedState->writeLESint32(_talkChore[i]);
	}
	savedState->writeLESint32(_talkAnim);

	if (_mumbleCostume) {
		savedState->writeLEUint32(1);
		savedState->writeCharString(_mumbleCostume->getFilename());
	} else {
		savedState->writeLEUint32(0);
	}
	savedState->writeLESint32(_mumbleChore);

	for (int i = 0; i < 5; ++i) {
		Shadow &shadow = _shadowArray[i];
		savedState->writeString(shadow.name);

		savedState->writeVector3d(shadow.pos);

		savedState->writeLESint32(shadow.planeList.size());
		// Cannot use g_grim->currScene() here because an actor can have walk planes
		// from other scenes. It happens e.g. when Membrillo calls Velasco to tell him
		// Naranja is dead.
		Scene *s = g_grim->findScene(_setName.c_str());
		for (SectorListType::iterator j = shadow.planeList.begin(); j != shadow.planeList.end(); ++j) {
			Sector *sec = *j;
			for (int k = 0; k < s->getSectorCount(); ++k) {
				if (*s->getSectorBase(k) == *sec) {
					savedState->writeLEUint32(k);
					break;
				}
			}
		}

		savedState->writeLESint32(shadow.shadowMaskSize);
		for (int j = 0; j < shadow.shadowMaskSize; ++j) {
			savedState->writeByte(shadow.shadowMask[j]);
		}
		savedState->writeLESint32(shadow.active);
		savedState->writeLESint32(shadow.dontNegate);
	}
	savedState->writeLESint32(_activeShadowSlot);

	if (_sayLineText) {
		savedState->writeLEUint32(g_grim->textObjectId(_sayLineText));
	} else {
		savedState->writeLEUint32(0);
	}

	savedState->writeVector3d(_lookAtVector);
	savedState->writeFloat(_lookAtRate);

	savedState->writeLESint32(_winX1);
	savedState->writeLESint32(_winY1);
	savedState->writeLESint32(_winX2);
	savedState->writeLESint32(_winY2);

	savedState->writeLESint32(_path.size());
	for (Common::List<Graphics::Vector3d>::const_iterator i = _path.begin(); i != _path.end(); ++i) {
		savedState->writeVector3d(*i);
	}
}

bool Actor::restoreState(SaveGame *savedState) {
	for (Common::List<Costume *>::const_iterator i = _costumeStack.begin(); i != _costumeStack.end(); ++i) {
		delete *i;
	}
	_costumeStack.clear();

	// load actor name
	_name = savedState->readString();
	_setName = savedState->readString();

	_talkColor          = savedState->readColor();

	_pos                = savedState->readVector3d();
	_pitch              = savedState->readFloat();
	_yaw                = savedState->readFloat();
	_roll               = savedState->readFloat();
	_walkRate           = savedState->readFloat();
	_turnRate           = savedState->readFloat();
	_constrain          = savedState->readLESint32();
	_reflectionAngle    = savedState->readFloat();
	_visible            = savedState->readLESint32();
	_lookingMode        = savedState->readLESint32();

	_talkSoundName 		= savedState->readString();

	if (savedState->readLEUint32()) {
		const char *fn = savedState->readCharString();
		_lipSync = g_resourceloader->getLipSync(fn);
		delete[] fn;
	} else {
		_lipSync = NULL;
	}

	int32 size = savedState->readLESint32();
	for (int32 i = 0; i < size; ++i) {
		const char *fname = savedState->readCharString();
		const int depth = savedState->readLEUint32();
		Costume *pc = NULL;
		if (depth > 0) {	//build all the previousCostume hierarchy
			const char **names = new const char*[depth];
			for (int j = 0; j < depth; ++j) {
				names[j] = savedState->readCharString();
			}
			for (int j = depth - 1; j >= 0; --j) {
				pc = findCostume(names[j]);
				if (!pc) {
					pc = g_resourceloader->loadCostume(names[j], pc);
				}
				delete[] names[j];
			}
			delete[] names;
		}

		Costume *c = g_resourceloader->loadCostume(fname, pc);
		c->restoreState(savedState);
		_costumeStack.push_back(c);
	}

	_turning = savedState->readLESint32();
	_destYaw = savedState->readFloat();

	_walking = savedState->readLESint32();
	_destPos = savedState->readVector3d();

	if (savedState->readLEUint32()) {
		const char *fname = savedState->readCharString();
		_restCostume = findCostume(fname);
		delete[] fname;
	} else {
		_restCostume = NULL;
	}
	_restChore = savedState->readLESint32();

	if (savedState->readLEUint32()) {
		const char *fname = savedState->readCharString();
		_walkCostume = findCostume(fname);
		delete[] fname;
	} else {
		_walkCostume = NULL;
	}

	_walkChore = savedState->readLESint32();
	_walkedLast = savedState->readLESint32();
	_walkedCur = savedState->readLESint32();

	if (savedState->readLEUint32()) {
		const char *fname = savedState->readCharString();
		_turnCostume = findCostume(fname);
		delete[] fname;
	} else {
		_turnCostume = NULL;
	}
	_leftTurnChore = savedState->readLESint32();
	_rightTurnChore = savedState->readLESint32();
	_lastTurnDir = savedState->readLESint32();
	_currTurnDir = savedState->readLESint32();

	for (int i = 0; i < 10; ++i) {
		if (savedState->readLEUint32()) {
			const char *fname = savedState->readCharString();
			_talkCostume[i] = findCostume(fname);
			delete[] fname;
		} else {
			_talkCostume[i] = NULL;
		}
		_talkChore[i] = savedState->readLESint32();
	}
	_talkAnim = savedState->readLESint32();

	if (savedState->readLEUint32()) {
		const char *fname = savedState->readCharString();
		_mumbleCostume = findCostume(fname);
		delete[] fname;
	} else {
		_mumbleCostume = NULL;
	}
	_mumbleChore = savedState->readLESint32();

	for (int i = 0; i < 5; ++i) {
		Shadow &shadow = _shadowArray[i];
		shadow.name = savedState->readString();

		shadow.pos = savedState->readVector3d();

		size = savedState->readLESint32();
		Scene *scene = g_grim->findScene(_setName.c_str());
		shadow.planeList.clear();
		for (int j = 0; j < size; ++j) {
			int32 id = savedState->readLEUint32();
			Sector *s = new Sector(*scene->getSectorBase(id));
			shadow.planeList.push_back(s);
		}

		shadow.shadowMaskSize = savedState->readLESint32();
		delete[] shadow.shadowMask;
		if (shadow.shadowMaskSize > 0) {
			shadow.shadowMask = new byte[shadow.shadowMaskSize];
			for (int j = 0; j < shadow.shadowMaskSize; ++j) {
				shadow.shadowMask[j] = savedState->readByte();
			}
		} else {
			shadow.shadowMask = NULL;
		}
		shadow.active = savedState->readLESint32();
		shadow.dontNegate = savedState->readLESint32();
	}
	_activeShadowSlot = savedState->readLESint32();

	_sayLineText = g_grim->textObject(savedState->readLEUint32());

	_lookAtVector = savedState->readVector3d();
	_lookAtRate = savedState->readFloat();

	_winX1 = savedState->readLESint32();
	_winY1 = savedState->readLESint32();
	_winX2 = savedState->readLESint32();
	_winY2 = savedState->readLESint32();

	size = savedState->readLESint32();
	for (int i = 0; i < size; ++i) {
		_path.push_back(savedState->readVector3d());
	}

	return true;
}

void Actor::setYaw(float yawParam) {
	// While the program correctly handle yaw angles outside
	// of the range [0, 360), proper convention is to roll
	// these values over correctly
	if (yawParam >= 360.0)
		_yaw = yawParam - 360;
	else if (yawParam < 0.0)
		_yaw = yawParam + 360;
	else
		_yaw = yawParam;
}

void Actor::setRot(float pitchParam, float yawParam, float rollParam) {
	_pitch = pitchParam;
	setYaw(yawParam);
	_roll = rollParam;
	_turning = false;
}

void Actor::setPos(Graphics::Vector3d position) {
	_walking = false;
	_pos = position;
}

// When the actor is walking report where the actor is going to and
// not the actual current position, this fixes some scene change
// change issues with the Bone Wagon (along with other fixes)
Graphics::Vector3d Actor::pos() const {
	// NOTE: These commented out lines break the "su" set, like explained
	// at https://github.com/residual/residual/issues/11 . According to the
	// above comment they however fix some issues. Since i don't know what
	// issues is it talking about, i'm commenting them anyway and we'll
	// see what happens.

// 	if (_walking)
// 		return _destPos;
// 	else
		return _pos;
}

Graphics::Vector3d Actor::destPos() const {
	if (_walking)
		return _destPos;
	else
		return _pos;
}

void Actor::turnTo(float pitchParam, float yawParam, float rollParam) {
	_pitch = pitchParam;
	_roll = rollParam;
	if (_yaw != yawParam) {
		_turning = true;
		_destYaw = yawParam;
	} else
		_turning = false;
}

void Actor::walkTo(Graphics::Vector3d p) {
	if (p == _pos)
		_walking = false;
	else {
		_walking = true;
		_destPos = p;
		_path.clear();

		if (_constrain) {
			Common::List<PathNode *> openList;
			Common::List<PathNode *> closedList;

			PathNode *start = new PathNode;
			start->parent = NULL;
			start->pos = _pos;
			start->dist = 0.f;
			start->cost = 0.f;
			openList.push_back(start);
			g_grim->currScene()->findClosestSector(_pos, &start->sect, NULL);

			Graphics::Vector3d currPos = _pos;
			Common::List<Sector *> sectors;
			for (int i = 0; i < g_grim->currScene()->getSectorCount(); ++i) {
				Sector *s = g_grim->currScene()->getSectorBase(i);
				if (s->type() >= Sector::WalkType && s->visible()) {
					sectors.push_back(s);
				}
			}

			Sector *endSec = NULL;
			g_grim->currScene()->findClosestSector(_destPos, &endSec, NULL);

			do {
				PathNode *node = NULL;
				float cost = -1.f;
				for (Common::List<PathNode *>::iterator j = openList.begin(); j != openList.end(); ++j) {
					PathNode *n = *j;
					float c = n->dist + n->cost;
					if (cost < 0.f || c < cost) {
						cost = c;
						node = n;
					}
				}
				closedList.push_back(node);
				openList.remove(node);
				Sector *sector = node->sect;

				if (sector == endSec) {
					PathNode *n = closedList.back()->parent;
					while (n) {
						_path.push_back(n->pos);
						n = n->parent;
					}

					break;
				}

				for (Common::List<Sector *>::iterator i = sectors.begin(); i != sectors.end(); ++i) {
					Sector *s = *i;
					bool inClosed = false;
					for (Common::List<PathNode *>::iterator j = closedList.begin(); j != closedList.end(); ++j) {
						if ((*j)->sect == s) {
							inClosed = true;
							break;
						}
					}
					if (!inClosed && sector->isAdjacentTo(s)) {
						PathNode *n = NULL;
						for (Common::List<PathNode *>::iterator j = openList.begin(); j != openList.end(); ++j) {
							if ((*j)->sect == s) {
								n = *j;
								break;
							}
						}
						if (n) {
							float newCost = node->cost + (n->pos - node->pos).magnitude();
							if (newCost < n->cost) {
								n->cost = newCost;
								n->parent = node;
							}
						} else {
							n = new PathNode;
							n->parent = node;
							n->sect = s;
							n->pos = (s->closestPoint(_destPos) + s->closestPoint(node->pos)) / 2.f;
							n->dist = (n->pos - _destPos).magnitude();
							n->cost = node->cost + (n->pos - node->pos).magnitude();
							openList.push_back(n);
						}
					}
				}
			} while (!openList.empty());

			for (Common::List<PathNode *>::iterator j = closedList.begin(); j != closedList.end(); ++j) {
				delete *j;
			}
			for (Common::List<PathNode *>::iterator j = openList.begin(); j != openList.end(); ++j) {
				delete *j;
			}
		}

		_path.push_front(_destPos);
	}
}

bool Actor::isWalking() const {
	return _walkedLast || _walkedCur || _walking;
}

bool Actor::isTurning() const {
	if (_turning)
		return true;

	if (_lastTurnDir != 0 || _currTurnDir != 0)
		return true;

	return false;
}

void Actor::walkForward() {
	float dist = g_grim->perSecond(_walkRate);
	float yaw_rad = _yaw * (LOCAL_PI / 180.f), pitch_rad = _pitch * (LOCAL_PI / 180.f);
	//float yaw;
	Graphics::Vector3d forwardVec(-sin(yaw_rad) * cos(pitch_rad),
		cos(yaw_rad) * cos(pitch_rad), sin(pitch_rad));
	Graphics::Vector3d destPos = _pos + forwardVec * dist;

	if (_lastWasLeft)
		if (_running)
			costumeMarkerCallback(RightRun);
		else
			costumeMarkerCallback(RightWalk);
	else
		if (_running)
			costumeMarkerCallback(LeftRun);
		else
			costumeMarkerCallback(LeftWalk);

	if (! _constrain) {
		_pos += forwardVec * dist;
		_walkedCur = true;
		return;
	}

	if (dist < 0) {
		dist = -dist;
		forwardVec = -forwardVec;
	}

	Sector *currSector = NULL, *prevSector = NULL;
	Sector::ExitInfo ei;

	g_grim->currScene()->findClosestSector(_pos, &currSector, &_pos);
	if (!currSector) { // Shouldn't happen...
		_pos += forwardVec * dist;
		_walkedCur = true;
		return;
	}

	while (currSector) {
		prevSector = currSector;
		Graphics::Vector3d puckVec = currSector->projectToPuckVector(forwardVec);
		puckVec /= puckVec.magnitude();
		currSector->getExitInfo(_pos, puckVec, &ei);
		float exitDist = (ei.exitPoint - _pos).magnitude();
		if (dist < exitDist) {
			_pos += puckVec * dist;
			_walkedCur = true;
			return;
		}
		_pos = ei.exitPoint;
		dist -= exitDist;
		if (exitDist > 0.0001)
			_walkedCur = true;

		// Check for an adjacent sector which can continue
		// the path
		currSector = g_grim->currScene()->findPointSector(ei.exitPoint + (float)0.0001 * puckVec, Sector::WalkType);
		if (currSector == prevSector)
			break;
	}

	ei.angleWithEdge *= (float)(180.f / LOCAL_PI);
	int turnDir = 1;
	if (ei.angleWithEdge > 90) {
		ei.angleWithEdge = 180 - ei.angleWithEdge;
		ei.edgeDir = -ei.edgeDir;
		turnDir = -1;
	}
	if (ei.angleWithEdge > _reflectionAngle)
		return;

	ei.angleWithEdge += (float)0.1;
	float turnAmt = g_grim->perSecond(_turnRate) * 5.;
	if (turnAmt > ei.angleWithEdge)
		turnAmt = ei.angleWithEdge;
	setYaw(_yaw + turnAmt * turnDir);
}

void Actor::setRunning(bool running) {
	_running = running;
}

Graphics::Vector3d Actor::puckVector() const {
	float yaw_rad = _yaw * (LOCAL_PI / 180.f);
	Graphics::Vector3d forwardVec(-sin(yaw_rad), cos(yaw_rad), 0);

	Sector *sector = g_grim->currScene()->findPointSector(_pos, Sector::WalkType);
	if (!sector)
		return forwardVec;
	else
		return sector->projectToPuckVector(forwardVec);
}

void Actor::setRestChore(int chore, Costume *cost) {
	if (_restCostume == cost && _restChore == chore)
		return;

	if (_restChore >= 0)
		_restCostume->stopChore(_restChore);

	_restCostume = cost;
	_restChore = chore;

	if (_restChore >= 0)
		_restCostume->playChoreLooping(_restChore);
}

void Actor::setWalkChore(int chore, Costume *cost) {
	if (_walkCostume == cost && _walkChore == chore)
		return;

	if (_walkChore >= 0)
		_walkCostume->stopChore(_walkChore);

	_walkCostume = cost;
	_walkChore = chore;
}

void Actor::setTurnChores(int left_chore, int right_chore, Costume *cost) {
	if (_turnCostume == cost && _leftTurnChore == left_chore &&
		_rightTurnChore == right_chore)
		return;

	if (_leftTurnChore >= 0) {
		_turnCostume->stopChore(_leftTurnChore);
		_turnCostume->stopChore(_rightTurnChore);
	}

	_turnCostume = cost;
	_leftTurnChore = left_chore;
	_rightTurnChore = right_chore;

	if ((left_chore >= 0 && right_chore < 0) || (left_chore < 0 && right_chore >= 0))
		error("Unexpectedly got only one turn chore");
}

void Actor::setTalkChore(int index, int chore, Costume *cost) {
	if (index < 1 || index > 10)
		error("Got talk chore index out of range (%d)", index);

	index--;

	if (_talkCostume[index] == cost && _talkChore[index] == chore)
		return;

	if (_talkChore[index] >= 0)
		_talkCostume[index]->stopChore(_talkChore[index]);

	_talkCostume[index] = cost;
	_talkChore[index] = chore;
}

void Actor::setMumbleChore(int chore, Costume *cost) {
	if (_mumbleChore >= 0)
		_mumbleCostume->stopChore(_mumbleChore);

	_mumbleCostume = cost;
	_mumbleChore = chore;
}

void Actor::turn(int dir) {
	float delta = g_grim->perSecond(_turnRate) * dir;
	setYaw(_yaw + delta);
	_currTurnDir = dir;

	if (_lastWasLeft)
		costumeMarkerCallback(RightTurn);
	else
		costumeMarkerCallback(LeftTurn);
}

float Actor::angleTo(const Actor &a) const {
	float yaw_rad = _yaw * (LOCAL_PI / 180.f);
	Graphics::Vector3d forwardVec(-sin(yaw_rad), cos(yaw_rad), 0);
	Graphics::Vector3d delta = a.pos() - _pos;
	delta.z() = 0;

	return angle(forwardVec, delta) * (180.f / LOCAL_PI);
}

float Actor::yawTo(Graphics::Vector3d p) const {
	Graphics::Vector3d dpos = p - _pos;

	if (dpos.x() == 0 && dpos.y() == 0)
		return 0;
	else
		return atan2(-dpos.x(), dpos.y()) * (180.f / LOCAL_PI);
}

void Actor::sayLine(const char *msg, const char *msgId) {
	assert(msg);
	assert(msgId);

	Common::String textName(msgId);
	textName += ".txt";

	if (msgId[0] == 0) {
		error("Actor::sayLine: No message ID for text");
		return;
	}

	// During Fullscreen movies SayLine is called for text display only
	// However, normal SMUSH movies may call SayLine, for example:
	// When Domino yells at Manny (a SMUSH movie) he does it with
	// a SayLine request rather than as part of the movie!
	if (!g_smush->isPlaying() || g_grim->getMode() == ENGINE_MODE_NORMAL) {
		Common::String soundName = msgId;
		Common::String soundLip = msgId;
		soundName += ".wav";
		soundLip += ".lip";

		if (_talkSoundName == soundName)
			return;

		if (g_imuse->getSoundStatus(_talkSoundName.c_str()) || msg[0] == 0)
			shutUp();

		_talkSoundName = soundName;
		g_imuse->startVoice(_talkSoundName.c_str());
		if (g_grim->currScene()) {
			g_grim->currScene()->setSoundPosition(_talkSoundName.c_str(), pos());
		}

		// If the actor is clearly not visible then don't try to play the lip sync
		if (visible()) {
			// Sometimes actors speak offscreen before they, including their
			// talk chores are initialized.
			// For example, when reading the work order (a LIP file exists for no reason).
			// Also, some lip sync files have no entries
			// In these cases, revert to using the mumble chore.
			_lipSync = g_resourceloader->getLipSync(soundLip.c_str());
			// If there's no lip sync file then load the mumble chore if it exists
			// (the mumble chore doesn't exist with the cat races announcer)
			if (!_lipSync && _mumbleChore != -1)
				_mumbleCostume->playChoreLooping(_mumbleChore);

			_talkAnim = -1;
		}
	}

	if (_sayLineText) {
		g_grim->killTextObject(_sayLineText);
		_sayLineText = NULL;
	}

	if (!g_grim->_sayLineDefaults.font)
		return;

	_sayLineText = new TextObject(false, true);
	_sayLineText->setDefaults(&g_grim->_sayLineDefaults);
	_sayLineText->setText(msg);
	_sayLineText->setFGColor(&_talkColor);
	if (g_grim->getMode() == ENGINE_MODE_SMUSH) {
		_sayLineText->setX(640 / 2);
		_sayLineText->setY(456);
	} else {
		if (_winX1 == 1000 || _winX2 == -1000 || _winY2 == -1000) {
			_sayLineText->setX(640 / 2);
			_sayLineText->setY(463);
		} else {
			_sayLineText->setX((_winX1 + _winX2) / 2);
			_sayLineText->setY(_winY1);
		}
	}
	_sayLineText->createBitmap();
	g_grim->registerTextObject(_sayLineText);
}

bool Actor::talking() {
	// If there's no sound file then we're obviously not talking
	if (strlen(_talkSoundName.c_str()) == 0 || !g_imuse->getSoundStatus(_talkSoundName.c_str())) {
		// If we're not talking and _sayLinetext exists delete it.
		// Without this sometimes after reaping Bruno, his "nice bathrob" isn't deleted
		// and lives through all the cutscene.
		if (_sayLineText) {
			g_grim->killTextObject(_sayLineText);
			_sayLineText = NULL;
		}
		return false;
	}

	return true;
}

void Actor::shutUp() {
	// While the call to stop the sound is usually made by the game,
	// we also need to handle when the user terminates the dialog.
	// Some warning messages will occur when the user terminates the
	// actor dialog but the game will continue alright.
	if (_talkSoundName != "") {
		g_imuse->stopSound(_talkSoundName.c_str());
		_talkSoundName = "";
	}
	if (_lipSync) {
		if (_talkAnim != -1 && _talkChore[_talkAnim] >= 0)
			_talkCostume[_talkAnim]->stopChore(_talkChore[_talkAnim]);
		_lipSync = NULL;
	} else if (_mumbleChore >= 0) {
		_mumbleCostume->stopChore(_mumbleChore);
	}

	if (_sayLineText) {
		g_grim->killTextObject(_sayLineText);
		_sayLineText = NULL;
	}
}

void Actor::pushCostume(const char *n) {
	Costume *newCost = g_resourceloader->loadCostume(n, currentCostume());

	newCost->setColormap(NULL);
	_costumeStack.push_back(newCost);
}

void Actor::setColormap(const char *map) {
	if (!_costumeStack.empty()) {
		Costume *cost = _costumeStack.back();
		cost->setColormap(map);
	} else {
		warning("Actor::setColormap: No costumes");
	}
}

void Actor::setCostume(const char *n) {
	if (!_costumeStack.empty())
		popCostume();

	pushCostume(n);
}

void Actor::popCostume() {
	if (!_costumeStack.empty()) {
		freeCostumeChore(_costumeStack.back(), _restCostume, _restChore);
		freeCostumeChore(_costumeStack.back(), _walkCostume, _walkChore);

		if (_turnCostume == _costumeStack.back()) {
			_turnCostume = NULL;
			_leftTurnChore = -1;
			_rightTurnChore = -1;
		}

		freeCostumeChore(_costumeStack.back(), _mumbleCostume, _mumbleChore);
		for (int i = 0; i < 10; i++)
			freeCostumeChore(_costumeStack.back(), _talkCostume[i], _talkChore[i]);
		delete _costumeStack.back();
		_costumeStack.pop_back();
		Costume *newCost;
		if (_costumeStack.empty())
			newCost = NULL;
		else
			newCost = _costumeStack.back();
		if (!newCost) {
			if (gDebugLevel == DEBUG_NORMAL || gDebugLevel == DEBUG_ALL)
				printf("Popped (freed) the last costume for an actor.\n");
		}
	} else {
		if (gDebugLevel == DEBUG_WARN || gDebugLevel == DEBUG_ALL)
			warning("Attempted to pop (free) a costume when the stack is empty!");
	}
}

void Actor::clearCostumes() {
	// Make sure to destroy costume copies in reverse order
	while (!_costumeStack.empty())
		popCostume();
}

void Actor::setHead(int joint1, int joint2, int joint3, float maxRoll, float maxPitch, float maxYaw) {
	if (!_costumeStack.empty()) {
		_costumeStack.back()->setHead(joint1, joint2, joint3, maxRoll, maxPitch, maxYaw);
	}
}

Costume *Actor::findCostume(const char *n) {
	for (Common::List<Costume *>::iterator i = _costumeStack.begin(); i != _costumeStack.end(); ++i) {
		if (strcasecmp((*i)->getFilename(), n) == 0)
			return *i;
	}

	return NULL;
}

void Actor::updateWalk() {
	if (_path.empty()) {
		return;
	}

	Graphics::Vector3d destPos = _path.back();
	float y = yawTo(destPos);
	if (y < 0.f) {
		y += 360.f;
	}
	if (_pos.x() != destPos.x() || _pos.y() != destPos.y()) {
		turnTo(_pitch, y, _roll);
	}

	Graphics::Vector3d dir = destPos - _pos;
	float dist = dir.magnitude();

	if (dist > 0)
		dir /= dist;

	float walkAmt = g_grim->perSecond(_walkRate);

	if (walkAmt >= dist) {
		_pos = destPos;
		_path.pop_back();
		if (_path.empty()) {
			_walking = false;
// It seems that we need to allow an already active turning motion to
// continue or else turning actors away from barriers won't work right
			_turning = false;
		}
	} else {
		_pos += dir * walkAmt;
	}

	_walkedCur = true;

	if (_lastWasLeft)
		if (_running)
			costumeMarkerCallback(RightRun);
		else
			costumeMarkerCallback(RightWalk);
	else
		if (_running)
			costumeMarkerCallback(LeftRun);
		else
			costumeMarkerCallback(LeftWalk);
}

void Actor::update() {
	// Snap actor to walkboxes if following them.  This might be
	// necessary for example after activating/deactivating
	// walkboxes, etc.
	if (_constrain && !_walking) {
		g_grim->currScene()->findClosestSector(_pos, NULL, &_pos);
	}

	if (_turning) {
		float turnAmt = g_grim->perSecond(_turnRate) * 5.f;
		float dyaw = _destYaw - _yaw;
		while (dyaw > 180)
			dyaw -= 360;
		while (dyaw < -180)
			dyaw += 360;
		// If the actor won't turn because the rate is set to zero then
		// have the actor turn all the way to the destination yaw.
		// Without this some actors will lock the interface on changing
		// scenes, this affects the Bone Wagon in particular.
		if (turnAmt == 0 || turnAmt >= fabs(dyaw)) {
			setYaw(_destYaw);
			_turning = false;
		}
		else if (dyaw > 0)
			setYaw(_yaw + turnAmt);
		else
			setYaw(_yaw - turnAmt);
		_currTurnDir = (dyaw > 0 ? 1 : -1);

		if (_lastWasLeft)
			costumeMarkerCallback(RightTurn);
		else
			costumeMarkerCallback(LeftTurn);
	}

	if (_walking) {
		updateWalk();
	}

	// The rest chore might have been stopped because of a
	// StopActorChore(nil).  Restart it if so.
	if (_restChore >= 0 && _restCostume->isChoring(_restChore, false) < 0)
		_restCostume->playChoreLooping(_restChore);

	if (_walkChore >= 0) {
		if (_walkedCur) {
			if (_walkCostume->isChoring(_walkChore, false) < 0) {
				_lastStepTime = 0;
				_walkCostume->playChoreLooping(_walkChore);
			}
		} else {
			if (_walkCostume->isChoring(_walkChore, false) >= 0)
				_walkCostume->stopChore(_walkChore);
		}
	}

	if (_leftTurnChore >= 0) {
		if (_walkedCur)
			_currTurnDir = 0;
		if (_lastTurnDir != 0 && _lastTurnDir != _currTurnDir)
			_turnCostume->stopChore(getTurnChore(_lastTurnDir));
		if (_currTurnDir != 0 && _currTurnDir != _lastTurnDir)
			_turnCostume->playChore(getTurnChore(_currTurnDir));
	} else
		_currTurnDir = 0;

	_walkedLast = _walkedCur;
	_walkedCur = false;
	_lastTurnDir = _currTurnDir;
	_currTurnDir = 0;

	// Update lip syncing
	if (_lipSync) {
		int posSound;

		// While getPosIn60HzTicks will return "-1" to indicate that the
		// sound is no longer playing, it is more appropriate to check first
		if (g_imuse->getSoundStatus(_talkSoundName.c_str()))
			posSound = g_imuse->getPosIn60HzTicks(_talkSoundName.c_str());
		else
			posSound = -1;
		if (posSound != -1) {
			int anim = _lipSync->getAnim(posSound);
			if (_talkAnim != anim) {
				if (_talkAnim != -1 && _talkChore[_talkAnim] >= 0)
					_talkCostume[_talkAnim]->stopChore(_talkChore[_talkAnim]);
				if (anim != -1) {
					_talkAnim = anim;
					if (_talkChore[_talkAnim] >= 0) {
						_talkCostume[_talkAnim]->playChoreLooping(_talkChore[_talkAnim]);
					}
				}
			}
		}
	}

	for (Common::List<Costume *>::iterator i = _costumeStack.begin(); i != _costumeStack.end(); ++i) {
		Costume *c = *i;
		c->setPosRotate(_pos, _pitch, _yaw, _roll);
		if (_lookingMode) {
			c->setLookAt(_lookAtVector, _lookAtRate);
		}
		c->update();
	}

	for (Common::List<Costume *>::iterator i = _costumeStack.begin(); i != _costumeStack.end(); ++i) {
		Costume *c = *i;
		if (_lookingMode) {
			c->moveHead();
		}
	}
}

void Actor::draw() {
	g_winX1 = g_winY1 = 1000;
	g_winX2 = g_winY2 = -1000;

	for (Common::List<Costume *>::iterator i = _costumeStack.begin(); i != _costumeStack.end(); ++i) {
		Costume *c = *i;
		c->setupTextures();
	}

	if (!g_driver->isHardwareAccelerated() && g_grim->getFlagRefreshShadowMask()) {
		for (int l = 0; l < 5; l++) {
			if (!_shadowArray[l].active)
				continue;
			g_driver->setShadow(&_shadowArray[l]);
			g_driver->drawShadowPlanes();
			g_driver->setShadow(NULL);
		}
	}

	if (!_costumeStack.empty()) {
		Costume *costume = _costumeStack.back();
		if (!g_driver->isHardwareAccelerated()) {
			for (int l = 0; l < 5; l++) {
				if (!_shadowArray[l].active)
					continue;
				g_driver->setShadow(&_shadowArray[l]);
				g_driver->setShadowMode();
				g_driver->startActorDraw(_pos, _yaw, _pitch, _roll);
				costume->draw();
				g_driver->finishActorDraw();
				g_driver->clearShadowMode();
				g_driver->setShadow(NULL);
			}
			// normal draw actor
			g_driver->startActorDraw(_pos, _yaw, _pitch, _roll);
			costume->draw();
			g_driver->finishActorDraw();
		} else {
			// normal draw actor
			g_driver->startActorDraw(_pos, _yaw, _pitch, _roll);
			costume->draw();
			g_driver->finishActorDraw();

			for (int l = 0; l < 5; l++) {
				if (!_shadowArray[l].active)
					continue;
				g_driver->setShadow(&_shadowArray[l]);
				g_driver->setShadowMode();
				g_driver->drawShadowPlanes();
				g_driver->startActorDraw(_pos, _yaw, _pitch, _roll);
				costume->draw();
				g_driver->finishActorDraw();
				g_driver->clearShadowMode();
				g_driver->setShadow(NULL);
			}
		}
	}
	_winX1 = g_winX1;
	_winX2 = g_winX2;
	_winY1 = g_winY1;
	_winY2 = g_winY2;
}

// "Undraw objects" (handle objects for actors that may not be on screen)
void Actor::undraw(bool /*visible*/) {
	if (!talking() || !g_imuse->isVoicePlaying())
		shutUp();
}

#define strmatch(src, dst)     (strlen(src) == strlen(dst) && strcmp(src, dst) == 0)

void Actor::setShadowPlane(const char *n) {
	assert(_activeShadowSlot != -1);

	_shadowArray[_activeShadowSlot].name = n;
}

void Actor::addShadowPlane(const char *n) {
	assert(_activeShadowSlot != -1);

	int numSectors = g_grim->currScene()->getSectorCount();

	for (int i = 0; i < numSectors; i++) {
		// Create a copy so we are sure it will not be deleted by the Scene destructor
		// behind our back. This is important when Membrillo phones Velasco to tell him
		// Naranja is dead, because the scene changes back and forth few times and so
		// the scenes' sectors are deleted while they are still keeped by the actors.
		Sector *sector = new Sector(*g_grim->currScene()->getSectorBase(i));
		if (strmatch(sector->name(), n)) {
			_shadowArray[_activeShadowSlot].planeList.push_back(sector);
			g_grim->flagRefreshShadowMask(true);
			return;
		}
	}
}

void Actor::setActiveShadow(int shadowId) {
	assert(shadowId >= 0 && shadowId <= 4);

	_activeShadowSlot = shadowId;
	_shadowArray[_activeShadowSlot].active = true;
}

void Actor::setShadowValid(int valid) {
	if (valid == -1)
		_shadowArray[_activeShadowSlot].dontNegate = true;
	else
		_shadowArray[_activeShadowSlot].dontNegate = false;
}

void Actor::setActivateShadow(int shadowId, bool state) {
	assert(shadowId >= 0 && shadowId <= 4);

	_shadowArray[shadowId].active = state;
}

void Actor::setShadowPoint(Graphics::Vector3d p) {
	assert(_activeShadowSlot != -1);

	_shadowArray[_activeShadowSlot].pos = p;
}

void Actor::clearShadowPlanes() {
	for (int i = 0; i < 5; i++) {
		Shadow *shadow = &_shadowArray[i];
		while (!shadow->planeList.empty()) {
			delete shadow->planeList.back();
			shadow->planeList.pop_back();
		}
		delete[] shadow->shadowMask;
		shadow->shadowMaskSize = 0;
		shadow->shadowMask = NULL;
		shadow->active = false;
		shadow->dontNegate = false;
	}
}

void Actor::putInSet(const char *setName) {
	_setName = setName;

	// In the set "td" there is bruno inside his coffin. Its actor, "/lrtx001/",
	// is added to the set but its visibility is kept false, and so it needs
	// to be made visible manually.
	if (strcmp("", setName)) {
		_visible = true;
	}
}

bool Actor::inSet(const char *setName) const {
	return _setName == setName;
}

void Actor::freeCostumeChore(Costume *toFree, Costume *&cost, int &chore) {
	if (cost == toFree) {
		cost = NULL;
		chore = -1;
	}
}

extern int refSystemTable;

void Actor::costumeMarkerCallback(Footstep step)
{
	int time = g_system->getMillis();
	float rate = 400;
	if (_running)
		rate = 300;

	if (_lastStepTime != 0 && time - _lastStepTime < rate)
		return;

	_lastStepTime = time;
	_lastWasLeft = !_lastWasLeft;

	lua_beginblock();

	lua_pushobject(lua_getref(refSystemTable));
	lua_pushstring("costumeMarkerHandler");
	lua_Object table = lua_gettable();

	if (lua_istable(table)) {
		lua_pushobject(table);
		lua_pushstring("costumeMarkerHandler");
		lua_Object func = lua_gettable();
		if (lua_isfunction(func)) {
			lua_pushobject(func);
			lua_pushusertag(this, MKTAG('A','C','T','R'));
			lua_pushnumber(step);
			lua_callfunction(func);
		}
	} else if (lua_isfunction(table)) {
		lua_pushusertag(this, MKTAG('A','C','T','R'));
		lua_pushnumber(step);
		lua_callfunction(table);
	}

	lua_endblock();
}

} // end of namespace Grim
