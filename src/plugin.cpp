#include "plugin.hpp"

Plugin *pluginInstance;

void init(Plugin *p)
{
    pluginInstance = p;
    p->addModel(modelButtonA);
    p->addModel(modelVoltM);
    p->addModel(modelKnobX);
    p->addModel(modelAntifaQ);
    p->addModel(modelMerge);
    p->addModel(modelSplit);

    //   add new modules here
    //   p->addModel(modelSLUG);
}