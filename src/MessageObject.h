/*
 *  Copyright 2009 Reality Jockey, Ltd.
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

#ifndef _MESSAGE_OBJECT_H_
#define _MESSAGE_OBJECT_H_

#include "ConnectionType.h"
#include "ObjectLetPair.h"
#include "PdMessage.h"
#include "StaticUtils.h"

class PdGraph;

class MessageObject {
  
  public:
    MessageObject(int numMessageInlets, int numMessageOutlets, PdGraph *graph);
    virtual ~MessageObject();
    
    /**
     * The generic entrypoint of a message to an object. This function usually
     * either passes the message directly to <code>processMessage</code> in the
     * case of an object which only processes messages, or queues the message for
     * later processing.
     */
    virtual void receiveMessage(int inletIndex, PdMessage *message);
    
    /** The message logic of an object. */
    virtual void processMessage(int inletIndex, PdMessage *message);
    
    /** Sends the given message to all connected objects at the given outlet index. */
    void sendMessage(int outletIndex, PdMessage *message);
    
    /** <code>MessageObject</code>s by default do not process any audio */
    virtual void processDsp();
  
    /** Returns the connection type of the given outlet. */
    virtual ConnectionType getConnectionType(int outletIndex);
  
    /** Establish a connection from another object to this object. */
    virtual void addConnectionFromObjectToInlet(MessageObject *messageObject, int outletIndex, int inletIndex);
  
    /** Establish a connection to another object from this object. */
    virtual void addConnectionToObjectFromOutlet(MessageObject *messageObject, int inletIndex, int outletIndex);
  
    /** Returns the label for this object. This is effectively a string representation of the object type. */
    virtual const char *getObjectLabel() = 0;
  
    /** Returns <code>true</code> if this object processes audio, <code>false</code> otherwise. */
    virtual bool doesProcessAudio();
  
   /**
    * Returns <code>true</code> if this object is a root in the Pd tree. <code>false</code> otherwise.
    * This function is used only while computing the process order of objects. For this reason it also
    * returns true in the cases when the object is receive, receive~, or catch~.
    */
    virtual bool isRootNode();
  
    /**
     * Returns <code>true</code> if this object is a leaf in the Pd tree. <code>false</code> otherwise.
     * This function is used only while computing the process order of objects. For this reason it also
     * returns true in the cases when the object is send, send~, or throw~.
     */
    virtual bool isLeafNode();
  
    /** Returns an ordered list of all parent objects of this object. */
    virtual List *getProcessOrder();
  
    // TODO(mhroth): one day there will have to be a recusive function to reset the isOrdered flag.
    
  protected:
    /** Returns a message that can be sent from the given outlet. */
    PdMessage *getNextOutgoingMessage(int outletIndex);
    
    /** Returns a new message for use at the given outlet. */
    virtual PdMessage *newCanonicalMessage(int outletIndex);
  
    PdGraph *graph;    
    int numMessageInlets;
    int numMessageOutlets;
    List **incomingMessageConnectionsListAtInlet;
    List **outgoingMessageConnectionsListAtOutlet;
    List **messageOutletPools;
  
    /** A flag indicating that this object has already been considered when ordering the process tree. */
    bool isOrdered;
};

#endif // _MESSAGE_OBJECT_H_