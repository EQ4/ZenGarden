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

#include "MessageAbsoluteValue.h"
#include "MessageAdd.h"
#include "MessageArcTangent.h"
#include "MessageArcTangent2.h"
#include "MessageBang.h"
#include "MessageCosine.h"
#include "MessageCputime.h"
#include "MessageChange.h"
#include "MessageClip.h"
#include "MessageCputime.h"
#include "MessageDeclare.h"
#include "MessageDelay.h"
#include "MessageDivide.h"
#include "MessageDbToPow.h"
#include "MessageDbToRms.h"
#include "MessageEqualsEquals.h"
#include "MessageExp.h"
#include "MessageFloat.h"
#include "MessageFrequencyToMidi.h"
#include "MessageGreaterThan.h"
#include "MessageGreaterThanOrEqualTo.h"
#include "MessageInlet.h"
#include "MessageInteger.h"
#include "MessageLessThan.h"
#include "MessageLessThanOrEqualTo.h"
#include "MessageLine.h"
#include "MessageListAppend.h"
#include "MessageListLength.h"
#include "MessageListPrepend.h"
#include "MessageListSplit.h"
#include "MessageListTrim.h"
#include "MessageLoadbang.h"
#include "MessageLog.h"
#include "MessageLogicalAnd.h"
#include "MessageLogicalOr.h"
#include "MessageMaximum.h"
#include "MessageMessageBox.h"
#include "MessageMetro.h"
#include "MessageMidiToFrequency.h"
#include "MessageMinimum.h"
#include "MessageModulus.h"
#include "MessageMoses.h"
#include "MessageMultiply.h"
#include "MessageNotEquals.h"
#include "MessageNotein.h"
#include "MessageOpenPanel.h"
#include "MessageOutlet.h"
#include "MessagePack.h"
#include "MessagePipe.h"
#include "MessagePow.h"
#include "MessagePowToDb.h"
#include "MessagePrint.h"
#include "MessageRandom.h"
#include "MessageReceive.h"
#include "MessageRemainder.h"
#include "MessageRmsToDb.h"
#include "MessageRoute.h"
#include "MessageSamplerate.h"
#include "MessageSelect.h"
#include "MessageSend.h"
#include "MessageSine.h"
#include "MessageSoundfiler.h"
#include "MessageSpigot.h"
#include "MessageSqrt.h"
#include "MessageStripNote.h"
#include "MessageSubtract.h"
#include "MessageSwitch.h"
#include "MessageSwap.h"
#include "MessageSymbol.h"
#include "MessageTable.h"
#include "MessageTableRead.h"
#include "MessageTableWrite.h"
#include "MessageTangent.h"
#include "MessageText.h"
#include "MessageTimer.h"
#include "MessageToggle.h"
#include "MessageTrigger.h"
#include "MessageUntil.h"
#include "MessageUnpack.h"
#include "MessageValue.h"
#include "MessageWrap.h"

#include "MessageSendController.h"

#include "DspAdc.h"
#include "DspAdd.h"
#include "DspBandpassFilter.h"
#include "DspBang.h"
#include "DspCatch.h"
#include "DspClip.h"
#include "DspCosine.h"
#include "DspDac.h"
#include "DspDelayRead.h"
#include "DspDelayWrite.h"
#include "DspDivide.h"
#include "DspEnvelope.h"
#include "DspHighpassFilter.h"
#include "DspInlet.h"
#include "DspLine.h"
#include "DspLog.h"
#include "DspLowpassFilter.h"
#include "DspMinimum.h"
#include "DspMultiply.h"
#include "DspNoise.h"
#include "DspOsc.h"
#include "DspOutlet.h"
#include "DspPhasor.h"
#include "DspPrint.h"
#include "DspReceive.h"
#include "DspReciprocalSqrt.h"
#include "DspRfft.h"
#include "DspRifft.h"
#include "DspSend.h"
#include "DspSignal.h"
#include "DspSqrt.h"
#include "DspSnapshot.h"
#include "DspSubtract.h"
#include "DspTablePlay.h"
#include "DspTableRead.h"
#include "DspTableRead4.h"
#include "DspThrow.h"
#include "DspVariableDelay.h"
#include "DspVCF.h"
#include "DspWrap.h"

#include "PdContext.h"
#include "PdFileParser.h"

// initialise the global graph counter
int PdContext::globalGraphId = 0;

#pragma mark Constructor/Deconstructor

PdContext::PdContext(int numInputChannels, int numOutputChannels, int blockSize, float sampleRate,
    void (*function)(ZGCallbackFunction, void *, void *), void *userData) {
  this->numInputChannels = numInputChannels;
  this->numOutputChannels = numOutputChannels;
  this->blockSize = blockSize;
  this->sampleRate = sampleRate;
  callbackFunction = function;
  callbackUserData = userData;
  blockStartTimestamp = 0.0;
  blockDurationMs = ((double) blockSize / (double) sampleRate) * 1000.0;
  messageCallbackQueue = new OrderedMessageQueue();
  
  numBytesInInputBuffers = blockSize * numInputChannels * sizeof(float);
  numBytesInOutputBuffers = blockSize * numOutputChannels * sizeof(float);
  globalDspInputBuffers = (float *) calloc(blockSize * numInputChannels, sizeof(float));
  globalDspOutputBuffers = (float *) calloc(blockSize * numOutputChannels, sizeof(float));
  
  sendController = new MessageSendController(this);
    
  // configure the context lock, which is recursive
  pthread_mutexattr_t mta;
  pthread_mutexattr_init(&mta);
  pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&contextLock, &mta); 
}

PdContext::~PdContext() {
  free(globalDspInputBuffers);
  free(globalDspOutputBuffers);
  
  delete messageCallbackQueue;
  delete sendController;
  
  // delete all of the PdGraphs in the graph list
  for (int i = 0; i < graphList.size(); i++) {
    delete graphList[i];
  }

  pthread_mutex_destroy(&contextLock);
}

#pragma mark -
#pragma mark Get Context Attributes

int PdContext::getNumInputChannels() {
  return numInputChannels;
}

int PdContext::getNumOutputChannels() {
  return numOutputChannels;
}

int PdContext::getBlockSize() {
  return blockSize;
}

float PdContext::getSampleRate() {
  return sampleRate;
}

float *PdContext::getGlobalDspBufferAtInlet(int inletIndex) {
  return globalDspInputBuffers + (inletIndex * blockSize);
}

float *PdContext::getGlobalDspBufferAtOutlet(int outletIndex) {
  return globalDspOutputBuffers + (outletIndex * blockSize);
}

double PdContext::getBlockStartTimestamp() {
  return blockStartTimestamp;
}

double PdContext::getBlockDuration() {
  return blockDurationMs;
}

int PdContext::getNextGraphId() {
  return ++globalGraphId;
}


#pragma mark -
#pragma mark process

void PdContext::process(float *inputBuffers, float *outputBuffers) {
  lock(); // lock the context
  
  // set up adc~ buffers
  memcpy(globalDspInputBuffers, inputBuffers, numBytesInInputBuffers);
  
  // clear the global output audio buffers so that dac~ nodes can write to it
  memset(globalDspOutputBuffers, 0, numBytesInOutputBuffers);

  // Send all messages for this block
  ObjectMessageLetPair omlPair;
  double nextBlockStartTimestamp = blockStartTimestamp + blockDurationMs;
  while (!messageCallbackQueue->empty() &&
      (omlPair = messageCallbackQueue->peek()).first != NULL &&
      omlPair.second.first->getTimestamp() < nextBlockStartTimestamp) {
    
    messageCallbackQueue->pop(); // remove the message from the queue

    MessageObject *object = omlPair.first;
    PdMessage *message = omlPair.second.first;
    unsigned int outletIndex = omlPair.second.second;
    if (message->getTimestamp() < blockStartTimestamp) {
      // messages injected into the system with a timestamp behind the current block are automatically
      // rescheduled for the beginning of the current block. This is done in order to normalise
      // the treament of messages, but also to avoid difficulties in cases when messages are scheduled
      // in subgraphs with different block sizes.
      message->setTimestamp(blockStartTimestamp);
    }
    
    object->sendMessage(outletIndex, message);
    message->freeMessage(); // free the message now that it has been sent and processed
  }

  int numGraphs = graphList.size();
  PdGraph **graph = (numGraphs > 0) ? &graphList.front() : NULL;
  for (int i = 0; i < numGraphs; ++i) {
    graph[i]->processDsp();
  }
  
  blockStartTimestamp = nextBlockStartTimestamp;
  
  // copy the output audio to the given buffer
  memcpy(outputBuffers, globalDspOutputBuffers, numBytesInOutputBuffers);
  
  unlock(); // unlock the context
}


#pragma mark -
#pragma mark New Graph

PdGraph *PdContext::newGraph(char *directory, char *filename, PdMessage *initMessage, PdGraph *parentGraph) {
  // create file path based on directory and filename. Parse the file.
  char *filePath = StaticUtils::concatStrings(directory, filename);
  
  // if the file does not exist, return
  if (!StaticUtils::fileExists(filePath)) {
    printErr("The file %s could not be opened.", filePath);
    free(filePath);
    return NULL;
  }
  
  // open the file and parse it into individual messages
  PdFileParser *fileParser = new PdFileParser(filePath);
  
  PdGraph *graph = NULL;
  char *line = fileParser->nextMessage();
  if (strncmp(line, "#N canvas", strlen("#N canvas")) == 0) {
    graph = new PdGraph(initMessage, parentGraph, this, getNextGraphId());
    graph->addDeclarePath(directory); // adds the root graph
    bool success = configureEmptyGraphWithParser(graph, fileParser);
    if (!success) {
      printErr("The file %s could not be correctly parsed. Probably an unimplemented object has been referenced, or an abstraction could not be found.", filePath);
      delete graph;
      graph = NULL;
    }
  } else {
    printErr("The first line of Pd file %s does not define a canvas: %s", filePath, line);
  }
  free(filePath);
  delete fileParser;
  
  return graph;
}

bool PdContext::configureEmptyGraphWithParser(PdGraph *emptyGraph, PdFileParser *fileParser) {
  PdGraph *graph = emptyGraph;

#define RESOLUTION_BUFFER_LENGTH 512
#define INIT_MESSAGE_MAX_ELEMENTS 32
  PdMessage *initMessage = PD_MESSAGE_ON_STACK(INIT_MESSAGE_MAX_ELEMENTS);
  
  // configure the graph based on the messages
  char *line = NULL;
  while ((line = fileParser->nextMessage()) != NULL) {
    char *hashType = strtok(line, " ");
    if (strcmp(hashType, "#N") == 0) {
      char *objectType = strtok(NULL, " ");
      if (strcmp(objectType, "canvas") == 0) {
        // A new graph is defined inline. No arguments are passed (from this line)
        // the graphId is not incremented as this is a subpatch, not an abstraction
        PdGraph *newGraph = new PdGraph(graph->getArguments(), graph, this, graph->getGraphId());
        graph->addObject(0, 0, newGraph); // add the new graph to the current one as an object
        
        // the new graph is pushed onto the stack
        graph = newGraph;
      } else {
        printErr("Unrecognised #N object type: \"%s\".", line);
      }
    } else if (strcmp(hashType, "#X") == 0) {
      char *objectType = strtok(NULL, " ");
      if (strcmp(objectType, "obj") == 0) {
        int canvasX = atoi(strtok(NULL, " ")); // read the first canvas coordinate
        int canvasY = atoi(strtok(NULL, " ")); // read the second canvas coordinate
        char *objectLabel = strtok(NULL, " ;"); // delimit with " " or ";"
        char *objectInitString = strtok(NULL, ";"); // get the object initialisation string
        char resBuffer[RESOLUTION_BUFFER_LENGTH];
        initMessage->initWithSARb(INIT_MESSAGE_MAX_ELEMENTS, objectInitString, graph->getArguments(),
            resBuffer, RESOLUTION_BUFFER_LENGTH);
        MessageObject *messageObject = newObject(objectType, objectLabel, initMessage, graph);
        if (messageObject == NULL) {
          char *filename = StaticUtils::concatStrings(objectLabel, ".pd");
          char *directory = graph->findFilePath(filename);
          if (directory == NULL) {
            free(filename);
            printErr("Unknown object or abstraction \"%s\".", objectLabel);
            return false;
          }
          messageObject = newGraph(directory, filename, initMessage, graph);
          free(filename);
        }

        // add the object to the local graph and make any necessary registrations
        graph->addObject(canvasX, canvasY, messageObject);
      } else if (strcmp(objectType, "msg") == 0) {
        int canvasX = atoi(strtok(NULL, " ")); // read the first canvas coordinate
        int canvasY = atoi(strtok(NULL, " ")); // read the second canvas coordinate
        char *objectInitString = strtok(NULL, ";"); // get the message initialisation string
        graph->addObject(canvasX, canvasY ,new MessageMessageBox(objectInitString, graph));
      } else if (strcmp(objectType, "connect") == 0) {
        int fromObjectIndex = atoi(strtok(NULL, " "));
        int outletIndex = atoi(strtok(NULL, " "));
        int toObjectIndex = atoi(strtok(NULL, " "));
        int inletIndex = atoi(strtok(NULL, ";"));
        graph->addConnection(fromObjectIndex, outletIndex, toObjectIndex, inletIndex);
      } else if (strcmp(objectType, "floatatom") == 0) {
        int canvasX = atoi(strtok(NULL, " ")); // read the first canvas coordinate
        int canvasY = atoi(strtok(NULL, " ")); // read the second canvas coordinate
        initMessage->initWithTimestampAndFloat(0.0, 0.0f);
        graph->addObject(canvasX, canvasY, new MessageFloat(initMessage, graph)); // defines a number box
      } else if (strcmp(objectType, "symbolatom") == 0) {
        int canvasX = atoi(strtok(NULL, " ")); // read the first canvas coordinate
        int canvasY = atoi(strtok(NULL, " ")); // read the second canvas coordinate
        initMessage->initWithTimestampAndSymbol(0.0, NULL);
        graph->addObject(canvasX, canvasY, new MessageSymbol(initMessage, graph)); // defines a symbol box
      } else if (strcmp(objectType, "restore") == 0) {
        // the graph is finished being defined
        graph = graph->getParentGraph(); // pop the graph stack to the parent graph
      } else if (strcmp(objectType, "text") == 0) {
        int canvasX = atoi(strtok(NULL, " ")); // read the first canvas coordinate
        int canvasY = atoi(strtok(NULL, " ")); // read the second canvas coordinate
        char *comment = strtok(NULL, ";"); // get the comment
        MessageText *messageText = new MessageText(comment, graph);
        graph->addObject(canvasX, canvasY, messageText);
      } else if (strcmp(objectType, "declare") == 0) {
        // set environment for loading patch
        char *objectInitString = strtok(NULL, ";"); // get the arguments to declare
        initMessage->initWithString(2, objectInitString); // parse them
        if (initMessage->isSymbol(0, "-path")) {
          if (initMessage->isSymbol(1)) {
            // add symbol to declare directories
            graph->addDeclarePath(initMessage->getSymbol(1));
          }
        } else {
          printErr("declare \"%s\" flag is not supported.", initMessage->getSymbol(0));
        }
      } else if (strcmp(objectType, "array") == 0) {
        // creates a new table
        // objectInitString should contain both name and buffer length
        char *objectInitString = strtok(NULL, ";"); // get the object initialisation string
        char resBuffer[RESOLUTION_BUFFER_LENGTH];
        initMessage->initWithSARb(4, objectInitString, graph->getArguments(), resBuffer, RESOLUTION_BUFFER_LENGTH);
        MessageTable *table = new MessageTable(initMessage, graph);
        int bufferLength = 0;
        float *buffer = table->getBuffer(&bufferLength);
        graph->addObject(0, 0, table);
        
        // next many lines should be elements of that array
        // while the next line begins with #A
        while (strcmp(strtok(line = fileParser->nextMessage(), " ;"), "#A") == 0) {
          int index = atoi(strtok(NULL, " ;"));
          char *nextNumber = NULL;
          // ensure that file does not attempt to write more than stated numbers
          while (((nextNumber = strtok(NULL, " ;")) != NULL) && (index < bufferLength)) {
            buffer[index++] = atof(nextNumber);
          }
        }
        // ignore the #X coords line
      } else {
        printErr("Unrecognised #X object type on line: \"%s\"", line);
      }
    } else {
      printErr("Unrecognised hash type on line: \"%s\"", line);
    }
  }
  
  // force dsp ordering as the last step
  // some graphs may not have any connections (only abstractions), and thus may appear to do nothing
  graph->computeLocalDspProcessOrder();

  return true;
}

void PdContext::attachGraph(PdGraph *graph) {
  lock();
  graphList.push_back(graph);
  graph->attachToContext(true);
  unlock();
}

MessageObject *PdContext::newObject(char *objectType, char *objectLabel, PdMessage *initMessage, PdGraph *graph) {
  if (strcmp(objectType, "obj") == 0) {
    if (strcmp(objectLabel, "+") == 0) {
      return new MessageAdd(initMessage, graph);
    } else if (strcmp(objectLabel, "-") == 0) {
      return new MessageSubtract(initMessage, graph);
    } else if (strcmp(objectLabel, "*") == 0) {
      return new MessageMultiply(initMessage, graph);
    } else if (strcmp(objectLabel, "/") == 0) {
      return new MessageDivide(initMessage, graph);
    } else if (strcmp(objectLabel, "%") == 0) {
      return new MessageRemainder(initMessage, graph);
    } else if (strcmp(objectLabel, "pow") == 0) {
      return new MessagePow(initMessage, graph);
    } else if (strcmp(objectLabel, "powtodb") == 0) {
      return new MessagePowToDb(graph);
    } else if (strcmp(objectLabel, "dbtopow") == 0) {
      return new MessageDbToPow(graph);
    } else if (strcmp(objectLabel, "dbtorms") == 0) {
      return new MessageDbToRms(graph);
    } else if (strcmp(objectLabel, "rmstodb") == 0) {
      return new MessageRmsToDb(graph);
    } else if (strcmp(objectLabel, "log") == 0) {
      return new MessageLog(initMessage, graph);
    } else if (strcmp(objectLabel, "sqrt") == 0) {
      return new MessageSqrt(initMessage, graph);
    } else if (strcmp(objectLabel, ">") == 0) {
      return new MessageGreaterThan(initMessage, graph);
    } else if (strcmp(objectLabel, ">=") == 0) {
      return new MessageGreaterThanOrEqualTo(initMessage, graph);
    } else if (strcmp(objectLabel, "<") == 0) {
      return new MessageLessThan(initMessage, graph);
    } else if (strcmp(objectLabel, "<=") == 0) {
      return new MessageLessThanOrEqualTo(initMessage, graph);
    } else if (strcmp(objectLabel, "==") == 0) {
      return new MessageEqualsEquals(initMessage, graph);
    } else if (strcmp(objectLabel, "!=") == 0) {
      return new MessageNotEquals(initMessage, graph);
    } else if (strcmp(objectLabel, "||") == 0) {
      return new MessageLogicalOr(initMessage, graph);
    } else if (strcmp(objectLabel, "&&") == 0) {
      return new MessageLogicalAnd(initMessage, graph);
    } else if (strcmp(objectLabel, "abs") == 0) {
      return new MessageAbsoluteValue(initMessage, graph);
    } else if (strcmp(objectLabel, "atan") == 0) {
      return new MessageArcTangent(initMessage, graph);
    } else if (strcmp(objectLabel, "atan2") == 0) {
      return new MessageArcTangent2(initMessage, graph);
    } else if (strcmp(objectLabel, "bang") == 0 ||
               strcmp(objectLabel, "bng") == 0 ||
               strcmp(objectLabel, "b") == 0) {
      return new MessageBang(graph);
    } else if (strcmp(objectLabel, "change") == 0) {
      return new MessageChange(initMessage, graph);
    } else if (strcmp(objectLabel, "cos") == 0) {
      return new MessageCosine(initMessage, graph);
    } else if (strcmp(objectLabel, "cputime") == 0) {
      return new MessageCputime(initMessage, graph);
    } else if (strcmp(objectLabel, "clip") == 0) {
      return new MessageClip(initMessage, graph);
    } else if (strcmp(objectLabel, "declare") == 0) {
      return new MessageDeclare(initMessage, graph);
    } else if (strcmp(objectLabel, "delay") == 0 ||
               strcmp(objectLabel, "del") == 0) {
      return new MessageDelay(initMessage, graph);
    } else if (strcmp(objectLabel, "exp") == 0) {
      return new MessageExp(initMessage, graph);
    } else if (strcmp(objectLabel, "float") == 0 ||
               strcmp(objectLabel, "f") == 0) {
      return new MessageFloat(initMessage, graph);
    } else if (strcmp(objectLabel, "ftom") == 0) {
      return new MessageFrequencyToMidi(graph);
    } else if (strcmp(objectLabel, "mtof") == 0) {
      return new MessageMidiToFrequency(graph);
    } else if (StaticUtils::isNumeric(objectLabel)){
      PdMessage *initMessage = PD_MESSAGE_ON_STACK(1);
      initMessage->initWithTimestampAndFloat(0.0, atof(objectLabel));
      return new MessageFloat(initMessage, graph);
    } else if (strcmp(objectLabel, "inlet") == 0) {
      return new MessageInlet(graph);
    } else if (strcmp(objectLabel, "int") == 0 ||
               strcmp(objectLabel, "i") == 0) {
      return new MessageInteger(initMessage, graph);
    } else if (strcmp(objectLabel, "line") == 0) {
      return new MessageLine(initMessage, graph);
    } else if (strcmp(objectLabel, "list") == 0) {
      if (initMessage->isSymbol(0)) {
        if (initMessage->isSymbol(0, "append") ||
            initMessage->isSymbol(0, "prepend") ||
            initMessage->isSymbol(0, "split")) {
          int numElements = initMessage->getNumElements()-1;
          PdMessage *message = PD_MESSAGE_ON_STACK(numElements);
          message->initWithTimestampAndNumElements(0.0, numElements);
          memcpy(message->getElement(0), initMessage->getElement(1), numElements*sizeof(MessageAtom));
          MessageObject *messageObject = NULL;
          if (initMessage->isSymbol(0, "append")) {
            messageObject = new MessageListAppend(message, graph);
          } else if (initMessage->isSymbol(0, "prepend")) {
            messageObject = new MessageListPrepend(message, graph);
          } else if (initMessage->isSymbol(0, "split")) {
            messageObject = new MessageListSplit(message, graph);
          }
          return messageObject;
        } else if (initMessage->isSymbol(0, "trim")) {
          // trim and length do not act on the initMessage
          return new MessageListTrim(initMessage, graph);
        } else if (initMessage->isSymbol(0, "length")) {
          return new MessageListLength(initMessage, graph);
        } else {
          return new MessageListAppend(initMessage, graph);
        }
      } else {
        return new MessageListAppend(initMessage, graph);
      }
    } else if (strcmp(objectLabel, "loadbang") == 0) {
      return new MessageLoadbang(graph);
    } else if (strcmp(objectLabel, "max") == 0) {
      return new MessageMaximum(initMessage, graph);
    } else if (strcmp(objectLabel, "min") == 0) {
      return new MessageMinimum(initMessage, graph);
    } else if (strcmp(objectLabel, "metro") == 0) {
      return new MessageMetro(initMessage, graph);
    } else if (strcmp(objectLabel, "moses") == 0) {
      return new MessageMoses(initMessage, graph);
    } else if (strcmp(objectLabel, "mod") == 0) {
      return new MessageModulus(initMessage, graph);
    } else if (strcmp(objectLabel, "nbx") == 0) {
      // gui number boxes are represented as float objects
      return new MessageFloat(initMessage, graph);
    } else if (strcmp(objectLabel, "notein") == 0) {
      return new MessageNotein(initMessage, graph);
    } else if (strcmp(objectLabel, "pack") == 0) {
      return new MessagePack(initMessage, graph);
    } else if (strcmp(objectLabel, "pipe") == 0) {
      return new MessagePipe(initMessage, graph);
    } else if (strcmp(objectLabel, "print") == 0) {
      return new MessagePrint(initMessage, graph);
    } else if (strcmp(objectLabel, "openpanel") == 0) {
      return new MessageOpenPanel(initMessage, graph);
    } else if (strcmp(objectLabel, "outlet") == 0) {
      return new MessageOutlet(graph);
    } else if (strcmp(objectLabel, "random") == 0) {
      return new MessageRandom(initMessage, graph);
    } else if (strcmp(objectLabel, "receive") == 0 ||
               strcmp(objectLabel, "r") == 0) {
      return new MessageReceive(initMessage, graph);
    } else if (strcmp(objectLabel, "route") == 0) {
      return new MessageRoute(initMessage, graph);
    } else if (strcmp(objectLabel, "select") == 0 ||
               strcmp(objectLabel, "sel") == 0) {
      return new MessageSelect(initMessage, graph);
    } else if (strcmp(objectLabel, "send") == 0 ||
               strcmp(objectLabel, "s") == 0) {
      return new MessageSend(initMessage, graph);
    } else if (strcmp(objectLabel, "sin") == 0) {
      return new MessageSine(initMessage, graph);
    } else if (strcmp(objectLabel, "soundfiler") == 0) {
      return new MessageSoundfiler(initMessage, graph);
    } else if (strcmp(objectLabel, "spigot") == 0) {
      return new MessageSpigot(initMessage, graph);
    } else if (strcmp(objectLabel, "stripnote") == 0) {
      return new MessageStripNote(initMessage, graph);
    } else if (strcmp(objectLabel, "swap") == 0) {
      return new MessageSwap(initMessage, graph);
    } else if (strcmp(objectLabel, "symbol") == 0) {
      return new MessageSymbol(initMessage, graph);
    } else if (strcmp(objectLabel, "table") == 0) {
      return new MessageTable(initMessage, graph);
    } else if (strcmp(objectLabel, "tabread") == 0) {
      return new MessageTableRead(initMessage, graph);
    } else if (strcmp(objectLabel, "tabwrite") == 0) {
      return new MessageTableWrite(initMessage, graph);
    } else if (strcmp(objectLabel, "tan") == 0) {
      return new MessageTangent(initMessage, graph);
    } else if (strcmp(objectLabel, "timer") == 0) {
      return new MessageTimer(initMessage, graph);
    } else if (strcmp(objectLabel, "toggle") == 0 ||
               strcmp(objectLabel, "tgl") == 0) {
      return new MessageToggle(initMessage, graph);
    } else if (strcmp(objectLabel, "trigger") == 0 ||
               strcmp(objectLabel, "t") == 0) {
      return new MessageTrigger(initMessage, graph);
    } else if (strcmp(objectLabel, "until") == 0) {
      return new MessageUntil(graph);
    } else if (strcmp(objectLabel, "unpack") == 0) {
      return new MessageUnpack(initMessage,graph);
    } else if (strcmp(objectLabel, "value") == 0 ||
               strcmp(objectLabel, "v") == 0) {
      return new MessageValue(initMessage, graph);
    } else if (strcmp(objectLabel, "vsl") == 0 ||
               strcmp(objectLabel, "hsl") == 0) {
      // gui sliders are represented as a float objects
      PdMessage *message = PD_MESSAGE_ON_STACK(1);
      message->initWithTimestampAndFloat(0.0, 1.0f);
      return new MessageFloat(message, graph);
    } else if (strcmp(objectLabel, "wrap") == 0) {
      return new MessageWrap(initMessage, graph);
    } else if (strcmp(objectLabel, "+~") == 0) {
      return new DspAdd(initMessage, graph);
    } else if (strcmp(objectLabel, "-~") == 0) {
      return new DspSubtract(initMessage, graph);
    } else if (strcmp(objectLabel, "*~") == 0) {
      return new DspMultiply(initMessage, graph);
    } else if (strcmp(objectLabel, "/~") == 0) {
      return new DspDivide(initMessage, graph);
    } else if (strcmp(objectLabel, "adc~") == 0) {
      return new DspAdc(graph);
    } else if (strcmp(objectLabel, "bp~") == 0) {
      return new DspBandpassFilter(initMessage, graph);
    } else if (strcmp(objectLabel, "bang~") == 0) {
      return new DspBang(initMessage, graph);
    } else if (strcmp(objectLabel, "catch~") == 0) {
      return new DspCatch(initMessage, graph);
    } else if (strcmp(objectLabel, "clip~") == 0) {
      return new DspClip(initMessage, graph);
    } else if (strcmp(objectLabel, "cos~") == 0) {
      return new DspCosine(initMessage,graph);
    } else if (strcmp(objectLabel, "dac~") == 0) {
      return new DspDac(graph);
    } else if (strcmp(objectLabel, "delread~") == 0) {
      return new DspDelayRead(initMessage, graph);
    } else if (strcmp(objectLabel, "delwrite~") == 0) {
      return new DspDelayWrite(initMessage, graph);
    } else if (strcmp(objectLabel, "env~") == 0) {
      return new DspEnvelope(initMessage, graph);
    } else if (strcmp(objectLabel, "hip~") == 0) {
      return new DspHighpassFilter(initMessage, graph);
    } else if (strcmp(objectLabel, "inlet~") == 0) {
      return new DspInlet(graph);
    } else if (strcmp(objectLabel, "line~") == 0) {
      return new DspLine(graph);
    } else if (strcmp(objectLabel, "log~") == 0) {
      return new DspLog(initMessage, graph);
    } else if (strcmp(objectLabel, "lop~") == 0) {
      return new DspLowpassFilter(initMessage, graph);
    } else if (strcmp(objectLabel, "min~") == 0) {
      return new DspMinimum(initMessage, graph);
    } else if (strcmp(objectLabel, "noise~") == 0) {
      return new DspNoise(graph);
    } else if (strcmp(objectLabel, "osc~") == 0) {
      return new DspOsc(initMessage, graph);
    } else if (strcmp(objectLabel, "outlet~") == 0) {
      return new DspOutlet(graph);
    } else if (strcmp(objectLabel, "phasor~") == 0) {
      return new DspPhasor(initMessage, graph);
    } else if (strcmp(objectLabel, "print~") == 0) {
      return new DspPrint(initMessage, graph);
    } else if (strcmp(objectLabel, "receive~") == 0 ||
               strcmp(objectLabel, "r~") == 0) {
      return new DspReceive(initMessage, graph);
    } else if (strcmp(objectLabel, "rfft~") == 0) {
      return new DspRfft(initMessage, graph);
    } else if (strcmp(objectLabel, "rifft~") == 0) {
      return new DspRifft(initMessage, graph);
    } else if (strcmp(objectLabel, "rsqrt~") == 0 ||
               strcmp(objectLabel, "q8_rsqrt~") == 0) {
      return new DspReciprocalSqrt(initMessage, graph);
    } else if (strcmp(objectLabel, "samplerate~") == 0) {
      return new MessageSamplerate(initMessage, graph);
    } else if (strcmp(objectLabel, "send~") == 0 ||
               strcmp(objectLabel, "s~") == 0) {
      return new DspSend(initMessage, graph);
    } else if (strcmp(objectLabel, "sig~") == 0) {
      return new DspSignal(initMessage, graph);
    } else if (strcmp(objectLabel, "snapshot~") == 0) {
      return new DspSnapshot(initMessage, graph);
    } else if (strcmp(objectLabel, "sqrt~") == 0 ||
               strcmp(objectLabel, "q8_sqrt~") == 0) {
      return new DspSqrt(initMessage, graph);
    } else if (strcmp(objectLabel, "switch~") == 0) {
      return new MessageSwitch(initMessage, graph);
    } else if (strcmp(objectLabel, "tabplay~") == 0) {
      return new DspTablePlay(initMessage, graph);
    } else if (strcmp(objectLabel, "tabread~") == 0) {
      return new DspTableRead(initMessage, graph);
    } else if (strcmp(objectLabel, "tabread4~") == 0) {
      return new DspTableRead4(initMessage, graph);
    } else if (strcmp(objectLabel, "throw~") == 0) {
      return new DspThrow(initMessage, graph);
    } else if (strcmp(objectLabel, "vcf~") == 0) {
      return new DspVCF(initMessage, graph);
    } else if (strcmp(objectLabel, "vd~") == 0) {
      return new DspVariableDelay(initMessage, graph);
    } else if (strcmp(objectLabel, "wrap~") == 0) {
      return new DspWrap(initMessage, graph);
    }
  } else if (strcmp(objectType, "msg") == 0) {
    // TODO(mhroth)
  }
  
  // object was not recognised. It has probably not been implemented yet or exists as an abstraction
  return NULL;
}

#pragma mark -
#pragma mark Lock/Unlock Context

void PdContext::lock() {
  pthread_mutex_lock(&contextLock);
}

void PdContext::unlock() {
  pthread_mutex_unlock(&contextLock);
}

#pragma mark -
#pragma mark PrintStd/PrintErr

void PdContext::printErr(char *msg) {
  if (callbackFunction != NULL) {
    callbackFunction(ZG_PRINT_ERR, callbackUserData, msg);
  }
}

void PdContext::printErr(const char *msg, ...) {
  int maxStringLength = 1024;
  char stringBuffer[maxStringLength];
  va_list ap;
  va_start(ap, msg);
  vsnprintf(stringBuffer, maxStringLength-1, msg, ap);
  va_end(ap);
  
  printErr(stringBuffer);
}

void PdContext::printStd(char *msg) {
  if (callbackFunction != NULL) {
    callbackFunction(ZG_PRINT_STD, callbackUserData, msg);
  }
}

void PdContext::printStd(const char *msg, ...) {
  int maxStringLength = 1024;
  char stringBuffer[maxStringLength];
  va_list ap;
  va_start(ap, msg);
  vsnprintf(stringBuffer, maxStringLength-1, msg, ap);
  va_end(ap);
  
  printStd(stringBuffer);
}


#pragma mark - Register/Unregister Objects

void PdContext::registerRemoteMessageReceiver(RemoteMessageReceiver *receiver) {
  sendController->addReceiver(receiver);
}

void PdContext::unregisterRemoteMessageReceiver(RemoteMessageReceiver *receiver) {
  sendController->removeReceiver(receiver);
}

void PdContext::registerDspReceive(DspReceive *dspReceive) {
  dspReceiveList.push_back(dspReceive);
  
  // connect receive~ to associated send~
  DspSend *dspSend = getDspSend(dspReceive->getName());
  if (dspSend != NULL) {
    dspReceive->setBuffer(dspSend->getBuffer());
  }
}

void PdContext::unregisterDspReceive(DspReceive *dspReceive) {
  dspReceiveList.remove(dspReceive);
  dspReceive->setBuffer(NULL);
}

void PdContext::registerDspSend(DspSend *dspSend) {
  DspSend *sendObject = getDspSend(dspSend->getName());
  if (sendObject != NULL) {
    printErr("Duplicate send~ object found with name \"%s\".", dspSend->getName());
    return;
  }
  dspSendList.push_back(dspSend);
  
  // connect associated receive~s to send~.
  for (list<DspReceive *>::iterator it = dspReceiveList.begin(); it != dspReceiveList.end(); it++) {
    if (!strcmp((*it)->getName(), dspSend->getName())) {
      (*it)->setBuffer(dspSend->getBuffer());
    }
  }
}

void PdContext::unregisterDspSend(DspSend *dspSend) {
  dspSendList.remove(dspSend);
  
  // inform all previously connected receive~s that the send~ buffer does not exist anymore.
  for (list<DspReceive *>::iterator it = dspReceiveList.begin(); it != dspReceiveList.end(); it++) {
    if (!strcmp((*it)->getName(), dspSend->getName())) {
      (*it)->setBuffer(NULL);
    }
  }
}

DspSend *PdContext::getDspSend(char *name) {
  for (list<DspSend *>::iterator it = dspSendList.begin(); it != dspSendList.end(); it++) {
    if (!strcmp((*it)->getName(), name)) return (*it);
  }
  return NULL;
}

void PdContext::registerDelayline(DspDelayWrite *delayline) {
  if (getDelayline(delayline->getName()) != NULL) {
    printErr("delwrite~ with duplicate name \"%s\" registered.", delayline->getName());
    return;
  }
  delaylineList.push_back(delayline);
  
  // connect this delayline to all same-named delay receivers
  for (list<DelayReceiver *>::iterator it = delayReceiverList.begin(); it != delayReceiverList.end(); it++) {
    if (!strcmp((*it)->getName(), delayline->getName())) (*it)->setDelayline(delayline);
  }
}

void PdContext::registerDelayReceiver(DelayReceiver *delayReceiver) {
  delayReceiverList.push_back(delayReceiver);
  
  // connect the delay receiver to the named delayline
  DspDelayWrite *delayline = getDelayline(delayReceiver->getName());
  delayReceiver->setDelayline(delayline);
}

DspDelayWrite *PdContext::getDelayline(char *name) {
  for (list<DspDelayWrite *>::iterator it = delaylineList.begin(); it != delaylineList.end(); it++) {
    if (!strcmp((*it)->getName(), name)) return *it;
  }
  return NULL;
}

void PdContext::registerDspThrow(DspThrow *dspThrow) {
  throwList.push_back(dspThrow);
  
  DspCatch *dspCatch = getDspCatch(dspThrow->getName());
  if (dspCatch != NULL) {
    dspCatch->addThrow(dspThrow);
  }
}

void PdContext::registerDspCatch(DspCatch *dspCatch) {
  DspCatch *catchObject = getDspCatch(dspCatch->getName());
  if (catchObject != NULL) {
    printErr("catch~ with duplicate name \"%s\" already exists.", dspCatch->getName());
    return;
  }
  catchList.push_back(dspCatch);
  
  // connect catch~ to all associated throw~s
  for (list<DspThrow *>::iterator it = throwList.begin(); it != throwList.end(); it++) {
    if (!strcmp((*it)->getName(), dspCatch->getName())) dspCatch->addThrow((*it));
  }
}

DspCatch *PdContext::getDspCatch(char *name) {
  for (list<DspCatch *>::iterator it = catchList.begin(); it != catchList.end(); it++) {
    if (!strcmp((*it)->getName(), name)) return (*it);
  }
  return NULL;
}

void PdContext::registerTable(MessageTable *table) {  
  if (getTable(table->getName()) != NULL) {
    printErr("Table with name \"%s\" already exists.", table->getName());
    return;
  }
  tableList.push_back(table);
  
  for (list<TableReceiverInterface *>::iterator it = tableReceiverList.begin();
      it != tableReceiverList.end(); it++) {
    if (!strcmp((*it)->getName(), table->getName())) (*it)->setTable(table);
  }
}

MessageTable *PdContext::getTable(char *name) {
  for (list<MessageTable *>::iterator it = tableList.begin(); it != tableList.end(); it++) {
    if (!strcmp((*it)->getName(), name)) return (*it);
  }
  return NULL;
}

void PdContext::registerTableReceiver(TableReceiverInterface *tableReceiver) {
  tableReceiverList.push_back(tableReceiver); // add the new receiver
  
  MessageTable *table = getTable(tableReceiver->getName());
  tableReceiver->setTable(table); // set table whether it is NULL or not
}

void PdContext::unregisterTableReceiver(TableReceiverInterface *tableReceiver) {
  tableReceiverList.remove(tableReceiver); // remove the receiver
  tableReceiver->setTable(NULL);
}

void PdContext::setValueForName(char *name, float constant) {
  // TODO(mhroth): requires implementation!
}

float PdContext::getValueForName(char *name) {
  // TODO(mhroth): requires implementation!
  return 0.0f;
}


#pragma mark -
#pragma mark Manage Messages

void PdContext::sendMessageToNamedReceivers(char *name, PdMessage *message) {
  sendController->receiveMessage(name, message);
}

void PdContext::scheduleExternalMessageV(const char *receiverName, double timestamp,
    const char *messageFormat, va_list ap) {
  int numElements = strlen(messageFormat);
  PdMessage *message = PD_MESSAGE_ON_STACK(numElements);
  message->initWithTimestampAndNumElements(timestamp, numElements);
  for (int i = 0; i < numElements; i++) { // format message
    switch (messageFormat[i]) {
      case 'f': {
        message->setFloat(i, (float) va_arg(ap, double));
        break;
      }
      case 's': {
        message->setSymbol(i, (char *) va_arg(ap, char *));
        break;
      }
      case 'b': {
        message->setBang(i);
        break;
      }
      default: {
        break;
      }
    }
  }
  
  lock();
  int receiverNameIndex = sendController->getNameIndex((char *) receiverName);
  if (receiverNameIndex >= 0) { // if the receiver exists
    scheduleMessage(sendController, receiverNameIndex, message);
  }
  unlock();
}

PdMessage *PdContext::scheduleMessage(MessageObject *messageObject, unsigned int outletIndex, PdMessage *message) {
  // basic argument checking. It may happen that the message is NULL in case a cancel message
  // is sent multiple times to a particular object, when no message is pending
  if (message != NULL && messageObject != NULL) {
    message = message->copyToHeap();
    messageCallbackQueue->insertMessage(messageObject, outletIndex, message);
    return message;
  }
  return NULL;
}

void PdContext::cancelMessage(MessageObject *messageObject, int outletIndex, PdMessage *message) {
  if (message != NULL && outletIndex >= 0 && messageObject != NULL) {
    messageCallbackQueue->removeMessage(messageObject, outletIndex, message);
    message->freeMessage();
  }
}

void PdContext::receiveSystemMessage(PdMessage *message) {
  // TODO(mhroth): What are all of the possible system messages?
  if (message->isSymbol(0, "obj")) {
    // TODO(mhroth): dynamic patching
  } else if (callbackFunction != NULL) {
    if (message->isSymbol(0, "dsp") && message->isFloat(1)) {
      int result = (message->getFloat(1) != 0.0f) ? 1 : 0;
      callbackFunction(ZG_PD_DSP, callbackUserData, &result);
    }
  } else {
    char *messageString = message->toString();
    printErr("Unrecognised system command: %s", messageString);
    free(messageString);
  }
}
