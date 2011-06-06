/*
 *  Copyright 2010,2011 Reality Jockey, Ltd.
 *                 info@rjdj.me
 *                 http://rjdj.me/
 *
 *  This file is part of ZenGarden.
 *
 *  ZenGarden is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  ZenGarden is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with ZenGarden.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "ArrayArithmetic.h"
#include "DspCatch.h"
#include "DspThrow.h"
#include "PdGraph.h"

DspCatch::DspCatch(PdMessage *initMessage, PdGraph *graph) : DspObject(0, 0, 0, 1, graph) {
  if (initMessage->isSymbol(0)) {
    name = StaticUtils::copyString(initMessage->getSymbol(0));
  } else {
    name = NULL;
    graph->printErr("catch~ must be initialised with a name.");
  }
}

DspCatch::~DspCatch() {
  free(name);
}

const char *DspCatch::getObjectLabel() {
  return "catch~";
}

ObjectType DspCatch::getObjectType() {
  return DSP_CATCH;
}

char *DspCatch::getName() {
  return name;
}

void DspCatch::addThrow(DspThrow *dspThrow) {
  if (!strcmp(dspThrow->getName(), name)) {
    throwList.push_back(dspThrow); // NOTE(mhroth): no dupicate detection
  }
}

void DspCatch::removeThrow(DspThrow *dspThrow) {
  throwList.remove(dspThrow);
}

void DspCatch::processDsp() {
  switch (throwList.size()) {
    case 0: {
      memset(dspBufferAtOutlet0, 0, numBytesInBlock);
      break;
    }
    case 1: {
      DspThrow *dspThrow = throwList.front();
      memcpy(dspBufferAtOutlet0, dspThrow->getBuffer(), numBytesInBlock);
      break;
    }
    default: {
      DspThrow *dspThrow = throwList.front();
      memcpy(dspBufferAtOutlet0, dspThrow->getBuffer(), numBytesInBlock);
      
      list<DspThrow *>::iterator it = throwList.begin(); it++; // start from second element
      for (; it != throwList.end(); it++) {
        dspThrow = (*it);
        ArrayArithmetic::add(dspBufferAtOutlet0, dspThrow->getBuffer(), dspBufferAtOutlet0,
            0, blockSizeInt);
      }
      break;
    }
  }
}
