#include "plugin.hpp"

Plugin *pluginInstance;

void init(Plugin *p)
{
    pluginInstance = p;
    p->addModel(modelButtonA);

    //   add new modules here
    //   p->addModel(modelSLUG);
}