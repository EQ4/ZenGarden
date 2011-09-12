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

#ifndef _DSP_LOG_H_
#define _DSP_LOG_H_

#include "DspObject.h"

/** [log~], [log~ float] */
class DspLog : public DspObject {
    
  public:
    static MessageObject *newObject(PdMessage *initMessage, PdGraph *graph);
    DspLog(PdMessage *initMessage, PdGraph *graph);
    ~DspLog();
  
    static const char *getObjectLabel();
  
  private:
    void processMessage(int inletIndex, PdMessage *message);
    void processDspWithIndex(int fromIndex, int toIndex);
  
    inline float log2Approx(float x);
  
    float log2_base;
};

#endif // _DSP_LOG_H_
