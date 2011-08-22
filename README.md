ZenGarden
=======

ZenGarden (ZG) is a runtime for the [Pure Data](http://puredata.info/) (Pd) audio programming language. It is implemented as an extensible audio library allowing full control over signal processing, message passing, and graph manipulation. ZenGarden does not have a GUI, but easily allows one to be built on top of it.

The library is written in C++ and exposes a pure C interface described exclusively in [ZenGarden.h](https://github.com/mhroth/ZenGarden/blob/master/src/ZenGarden.h). Many audio objects are accelerated with vector operations on ARM (NEON) and x86 (SSE) platforms, and works especially well on Apple platforms (both OS X and iOS). ZenGarden allows externals to be built, also ones that override default object functionality.

Symantics
---------

ZenGarden consists of four basic object types. There are the context (ZGContext), graph (ZGGraph), object (ZGObject), and message (ZGMessage). The first three have to do with how the signal graph is organised. The latter represents discrete messages which are sent into, processed by, and out of the graph.

A context (known as a ZGContext) in code represents a unique and independent instance of Pure Data. Think of it as Pure Data's console window. A context is defined by its block size, sample rate, and the number of input and output channels. Contexts are entirely independent and messages and objects cannot be exchanged between them.

A graph (ZGGraph) is a collection of objects (ZGObject) and the connections between them. A ZGGraph is a subclass of ZGObject, and thus ZGGraphs can contain other ZGGraphs (such as abstraction or subgraphs).

Messages (ZGMessage) represent any Pd message, be it a single float or a list of assorted float, symbols, or bangs.

How to Get Started
------------------

  + Download the ZenGarden repository at https://github.com/mhroth/ZenGarden
  
  + The repository contains an Xcode project if you are using a Mac, and also a `make` file for other platforms. It is st
  rongly recommended to use the Xcode project to compile ZG for either iOS or OS X. Targets exist for both cases.
  
  + Once you have built `libzengarden.a`, it may be included in any of your projects also with `ZenGarden.h`
  
Running the Tests
-----------------

ZenGarden includes many tests meant to estabilsh the correct operation of the system. The test may be run by compiling the library via the included `make` file in the /src directory, and then running the `runme-test.sh` script from the base directory.

API Usage
===========

A few examples showing the basics of the ZenGarden API are detailed below. Most importantly is how to create a context, add a preexisting graph, and then process it. An example of how to programmatically create a graph is also described. Finally it is shown how to send arbitrary messages into a context.

Creating and Processing a Context
---------------------------------

A basic example is shown below, describing how to set up a context and graph, and then process the audio blocks. The C API as described in `ZenGarden.h` is used, though wrappers may exist for other languages as well.

```C
// include the ZenGarden API defintion
#include "ZenGarden.h"


// A ZenGarden context (see below) communicates via a single callback funtion as show here.
// A number of ZGCallbackFunction types are available with the most common being print commands.
void *callbackFunction(ZGCallbackFunction function, void *userData, void *ptr) {
  switch (function) {
    case ZG_PRINT_STD: {
      // A standard print command.
      printf("%s\n", (const char *) ptr);
      break;
    }
    case ZG_PRINT_ERR: {
      // A error print command.
      printf("ERROR: %s\n", (const char *) ptr);
      break;
    }
    default: break;
  }
  return NULL;
}

int main(int argc, char * const argv[]) {

  // The number of samples to be processed per block. This can be any positive integer,
  // though a power of two is strongly suggested. Typical values range between 64 and 1024.
  const int blockSize = 64;

  // The number of input channels. This can be any non-negative integer. Typical values are 0 (no input),
  // 1 (mono input), or 2 (stereo input).
  const int numInputChannels = 2;

  // The number of output channels. This can be any non-negative integer. Typical values are 0 (no output),
  // 1 (mono output), or 2 (stereo output).
  const int numOutputChannels = 2;

  // The sample rate. Any positive sample rate is supported. Typical values are i.e. 8000.0f, 22050.0f, or 44100.0f.
  const float sampleRate = 22050.0f;

  // Create a new context.
  ZGContext *context = zg_context_new(numInputChannels, numOutputChannels, blockSize, sampleRate, callbackFunction, NULL);

  // Create a new graph from the given file. The returned graph will be fully functional but it will not be attached
  // to the context. Only attached graphs are processed by the context. Unattached graphs created in a context do not
  // exist from the perspective of the context and add no overhead, but can be quickly added (and removed) as needed.
  // Graphs are not created in the audio thread and will not cause audio underruns.
  PdGraph *graph = zg_context_new_graph_from_file(context, "/path/to/", "file.pd");
  if (graph == NULL) {
    // if the returned graph is NULL then something has gone wrong.
    // If a callback function was provided to the context, then an error message will likely be reported there.
    return 1;
  }

  // Attach the graph to its context. Attaching a graph pauses audio playback, but it is typically a fast operation
  // and is unlikely to cause an underrun.
  zg_graph_attach(graph);

  // dummy input and output audio buffers. Note their size.
  float finputBuffers[blockSize * numInputChannels];
  float foutputBuffers[blockSize * numOutputChannels];

  // the audio loop
  while (1) {
    // Process the context. Messages are executed and audio buffers are consumed and produced.

    // if input and output audio buffers are presented as non-interleaved floating point (32-bit) samples
    // (ZenGarden's native format), they can be processed as shown here.
    zg_context_process(context, finputBuffers, foutputBuffers);

    // if input and output audio buffers are presented as interleaved signed integer (16-bit) samples,
    // they can be processed as shown here. ZenGarden automatically takes care of the translation to and
    // from the native non-interleaved format.
    // zg_context_process_s(context, sinputBuffers, soutputBuffer);
  }

  // Free memory. Memory can be freed in many ways.

  // The easiest is to delete the context. This not deletes the context and everything attached to it.
  zg_context_delete(context);

  // On the other hand, individual graphs can be deleted when they are no longer needed.
  // If an attached graph is deleted, it is automatically removed from the context first.
  // zg_graph_delete(graph);
  // zg_context_delete(context); // ultimately also the context must be deleted.

  return 0;
}
```

Creating a Graph Programmatically
---------------------------------

It is also possible to manipulate graphs programmatically. For example instead of creating a graph via `zg_context_new_graph_from_file()`,

```C
// create a new empty graph in a context
PdGraph *graph = zg_context_new_empty_graph(context);

// create a new [osc~ 440] object in the graph
ZGObject *objOsc = zg_graph_add_new_object(graph, "osc~ 440", 0.0f, 0.0f);

// create a new [dac~] object in the graph
ZGObject *objDac = zg_graph_add_new_object(graph, "dac~", 0.0f, 0.0f);

// conntect output 0 (the left output) of the [osc~] object to inlet 0 (the left inlet) of the [dac~] object
zg_graph_add_connection(graph, objOsc, 0, objDac, 0);

// conntect output 0 of the [osc~] object to inlet 1 (the right inlet) of the [dac~] object
zg_graph_add_connection(graph, objOsc, 0, objDac, 1);

// attach the graph to the context and prepare it for processing
zg_graph_attach(graph);
```

Sending a Message with a Known Structure
----------------------------------------

It is possible to send messages into a context or even directly to objects. If the message has a known structure, use:

```C
zg_context_send_messageV(context, "receverName", 0.0, "fff", 0.0f, 0.0f, 0.0f);
```
A message with three floats each set to 0.0f will be sent at time 0 (i.e., immediately) to receivers named "receiverName".

Sending a Message with an Unnown Structure
----------------------------------------

On the other hand if the structure of the message is not known beforehand then it can be created programmatically.

```C
// Create a message with timestamp 0.0 and three elements.
ZGMessage *message = zg_message_new(0.0, 3);

zg_message_set_float(message, 0, 0.0f);     // set the first element to 0.0f
zg_message_set_symbol(message, 1, "hello"); // set the second element to the symbol "hello"
zg_message_set_bang(message, 2);            // set the third element to a bang

// send the message at the given time to all receivers named "receiverName".
zg_context_send_message(context, "receiverName", message);

// The message is copied by context and can be freed after it is sent.
zg_message_delete(message);
```
