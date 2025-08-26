#include "plugin.hpp"

struct Split : Module
{
    enum ParamIds
    {
        NUM_PARAMS
    };
    enum InputIds
    {
        sIN,
        NUM_INPUTS
    };
    enum OutputIds
    {
        ENUMS(sOUT, 16),
        NUM_OUTPUTS
    };
    enum LightIds
    {
        ENUMS(sLED, 16),
        NUM_LIGHTS
    };

    dsp::ClockDivider lightDivider;
    int lastChannels = 0;

    Split()
    {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        for (int i = 0; i < 16; i++)
            configOutput(sOUT + i, string::f("Channel %d", i + 1));

        configInput(sIN, "Polyphonic Split");

        lightDivider.setDivision(512);
    }

    void process(const ProcessArgs &args) override
    {
        for (int c = 0; c < 16; c++)
        {
            lastChannels = inputs[sIN].getChannels();
            float w = 0.f;
            float v = inputs[sIN].getVoltage(c);
            outputs[sOUT + c].setVoltage(v);
            if (c < lastChannels && inputs[sIN].isConnected())
                w = 1.f;
            else
                w = 0.f;
            float deltaTime = args.sampleTime * lightDivider.getDivision();
            lights[sLED + c].setSmoothBrightness(w, deltaTime);
        }
    }
};

struct SplitWidget : ModuleWidget
{
    SplitWidget(Split *module)
    {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/panels/Panel3hp-dark.svg")));

        float y1 = 6, y2 = 18.3, x1 = 4, x2 = 11, x3 = 4.9;

        for (int i = 0; i < 16; i++)
        {
            if (i % 2)
                addOutput(createOutputCentered<PJ301Mvar4>(mm2px(Vec(x2, (y1 * (i - 1)) + 15)), module, Split::sIN + i));
            else
                addOutput(createOutputCentered<PJ301Mvar4>(mm2px(Vec(x1, (y1 * i) + 11)), module, Split::sIN + i));
        }

        for (int q = 0; q < 4; q++)
        {
            addChild(createLightCentered<TinyLight<WhiteLight>>(mm2px(Vec(x3 + (2 * q), y1 * y2 + 0)), module, Split::sLED + (q + 0)));
            addChild(createLightCentered<TinyLight<WhiteLight>>(mm2px(Vec(x3 + (2 * q), y1 * y2 + 2)), module, Split::sLED + (q + 4)));
            addChild(createLightCentered<TinyLight<WhiteLight>>(mm2px(Vec(x3 + (2 * q), y1 * y2 + 4)), module, Split::sLED + (q + 8)));
            addChild(createLightCentered<TinyLight<WhiteLight>>(mm2px(Vec(x3 + (2 * q), y1 * y2 + 6)), module, Split::sLED + (q + 12)));
        }

        addInput(createInputCentered<PJ301Mvar>(mm2px(Vec(8, y1 * 20.5)), module, Split::sOUT));

        CenteredLabel *const topLabel = new CenteredLabel;
        topLabel->box.pos = Vec(11.5, 5);
        topLabel->text = "SPLIT";
        addChild(topLabel);
    }
};

Model *modelSplit = createModel<Split, SplitWidget>("Split");