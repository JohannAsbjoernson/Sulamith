#include "plugin.hpp"

Plugin *pluginInstance;

void init(Plugin *p)
{
    pluginInstance = p;
    p->addModel(modelButtonA);
    p->addModel(modelBPMClk);
    p->addModel(modelVoltM);
    p->addModel(modelSH);
    p->addModel(modelKnobX);
    p->addModel(modelGTSeq);
    p->addModel(modelP2Seq);
    p->addModel(modelCompare);
    p->addModel(modelNote);
    p->addModel(modelMerge);
    p->addModel(modelSplit);

    //   add new modules here
    //   p->addModel(modelSLUG);
}

NVGcolor prepareDisplay(NVGcontext *vg, Rect *box, int fontSize)
{
    NVGcolor backgroundColor = nvgRGB(0x38, 0x38, 0x38);
    NVGcolor borderColor = nvgRGB(0x10, 0x10, 0x10);
    nvgBeginPath(vg);
    nvgRoundedRect(vg, 0.0, 0.0, box->size.x, box->size.y, 5.0);
    nvgFillColor(vg, backgroundColor);
    nvgFill(vg);
    nvgStrokeWidth(vg, 1.0);
    nvgStrokeColor(vg, borderColor);
    nvgStroke(vg);
    nvgFontSize(vg, fontSize);
    NVGcolor textColor = nvgRGB(0xaf, 0xd2, 0x2c);
    return textColor;
}