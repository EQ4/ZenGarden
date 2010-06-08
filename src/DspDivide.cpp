/*
 *  Copyright 2009,2010 Reality Jockey, Ltd.
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
#include "DspDivide.h"
#include "PdGraph.h"

DspDivide::DspDivide(PdMessage *initMessage, PdGraph *graph) : DspObject(2, 2, 0, 1, graph) {
  constant = initMessage->isFloat(0) ? initMessage->getFloat(0) : 0.0f;
}

DspDivide::~DspDivide() {
  // nothing to do
}

const char *DspDivide::getObjectLabel() {
  return "/~";
}

void DspDivide::processMessage(int inletIndex, PdMessage *message) {
  switch (inletIndex) {
    case 1: {
      if (message->isFloat(0)) {
        processDspToIndex(message->getBlockIndex(graph->getBlockStartTimestamp(), graph->getSampleRate()));
        constant = message->getFloat(0);
      }
      break;
    }
    default: {
      break;
    }
  }
}

void DspDivide::processDspToIndex(float blockIndex) {
  switch (signalPrecedence) {
    case DSP_DSP: {
      ArrayArithmetic::divide(localDspBufferAtInlet[0], localDspBufferAtInlet[1],
          localDspBufferAtOutlet[0], getStartSampleIndex(), getEndSampleIndex(blockIndex));
      break;
    }
    case DSP_MESSAGE: {
      ArrayArithmetic::divide(localDspBufferAtInlet[0], constant, localDspBufferAtOutlet[0],
          getStartSampleIndex(), getEndSampleIndex(blockIndex));
      break;
    }
    case MESSAGE_DSP:
    case MESSAGE_MESSAGE:
    default: {
      break; // nothing to do
    }
  }
  blockIndexOfLastMessage = blockIndex; // update the block index of the last message
}
