// These includes are, apart from sstream and string, optional. They're here for reference.
#include <iomanip>
#include <utility>
#include <sstream>
#include <algorithm>
#include <map>
#include <string>
#include <vector>

// We include rack.hpp always, its the next top boss in the include hierarchy. We include components.hpp here, so its included in all .cpps that include plugin.hpp
// Note that libraries are included <name> while your own .hpp files are "name"
#include <rack.hpp>
#include "includes/components.hpp"

// namespace is a directory of all functions and classes that belong to it. in iomanip its std, for most rack applications rack makes sense.
// functions belonging to a namespace are supposedly easier parsed and during the compiling process the engine knows better how to treat them.
// some will use their own namespace for their module collection when offering a wide range of skins and widget GUI modifications.
// Probably to avoid double-naming (two objects in the same namespace cannot have the same name, less prone to errors)
using namespace rack;

extern Plugin *pluginInstance;

extern Model *modelButtonA;
extern Model *modelBPMClk;
extern Model *modelVoltM;
extern Model *modelSH;
extern Model *modelKnobX;
extern Model *modelGTSeq;
extern Model *modelP2Seq;
extern Model *modelCompare;
extern Model *modelNote;
extern Model *modelMerge;
extern Model *modelSplit;
//  just add new modules below in the same fashion.
//  extern Model *modelCVSeq; etc.

static const int displayAlpha = 23;
NVGcolor prepareDisplay(NVGcontext *vg, Rect *box, int fontSize);