#include "plugin.hpp"

struct Merge : Module
{
    enum ParamIds
    {
        NUM_PARAMS
    };
    enum InputIds
    {
        ENUMS(mIN, 16),
        NUM_INPUTS
    };
    enum OutputIds
    {
        mOUT,
        NUM_OUTPUTS
    };
    enum LightIds
    {
        ENUMS(mLED, 16),
        NUM_LIGHTS
    };

    dsp::ClockDivider lightDivider;

    Merge()
    {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        for (int i = 0; i < 16; i++)
            configInput(mIN + i, string::f("Channel %d", i + 1));

        configOutput(mOUT, "Polyphonic Merge");
        lightDivider.setDivision(512);
    }

    bool ch_overflow = false, MONOP = false;

    void process(const ProcessArgs &args) override
    {
        float deltaTime = args.sampleTime * lightDivider.getDivision();
        int chmax[16] = {0};
        int ch_sum = 0;
        ch_overflow = false;
        if (!MONOP)
        {
            for (int i = 0; i < 16; i++)
            {
                if (inputs[mIN + i].isConnected())
                    chmax[i] = inputs[mIN + i].getChannels();
            }

            for (int n = 0; n < 16; n++)
            {
                if (inputs[mIN + n].isConnected())
                {
                    for (int c = 0; c < chmax[n]; c++)
                    {
                        if (ch_sum < 16)
                        {
                            float volt = inputs[mIN + n].getVoltage(c);
                            outputs[mOUT].setVoltage(volt, ch_sum);
                        }
                        else
                        {
                            ch_overflow = true;
                            break;
                        }
                        ch_sum++;
                    }
                }
            }
        }
        else if (MONOP)
        {
            for (int n = 0; n < 16; n++)
            {
                if (inputs[mIN + n].isConnected())
                {
                    float volt = inputs[mIN + n].getVoltage();
                    outputs[mOUT].setVoltage(volt, n);
                    ch_sum++;
                }
            }
        }

        if (ch_sum > 16)
            ch_sum = 16;
        // outputs
        outputs[mOUT].channels = ch_sum;
        int ch_p = outputs[mOUT].getChannels();
        for (int y = 0; y < 16; y++)
        {
            if (y < ch_p)
                lights[mLED + y].setSmoothBrightness(1.f, deltaTime);
            else
                lights[mLED + y].setSmoothBrightness(0.f, deltaTime);
        }
    }
};

struct MergeWidget : ModuleWidget
{
    MergeWidget(Merge *module)
    {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/panels/Panel3hp-dark.svg")));

        float y1 = 6, y2 = 18.3, x1 = 4, x2 = 11, x3 = 4.9;

        for (int i = 0; i < 16; i++)
        {
            if (i % 2)
                addInput(createInputCentered<PJ301Mvar3>(mm2px(Vec(x2, (y1 * (i - 1)) + 15)), module, Merge::mIN + i));
            else
                addInput(createInputCentered<PJ301Mvar3>(mm2px(Vec(x1, (y1 * i) + 11)), module, Merge::mIN + i));
        }

        for (int q = 0; q < 4; q++)
        {
            addChild(createLightCentered<TinyLight<WhiteLight>>(mm2px(Vec(x3 + (2 * q), y1 * y2 + 0)), module, Merge::mLED + (q + 0)));
            addChild(createLightCentered<TinyLight<WhiteLight>>(mm2px(Vec(x3 + (2 * q), y1 * y2 + 2)), module, Merge::mLED + (q + 4)));
            addChild(createLightCentered<TinyLight<WhiteLight>>(mm2px(Vec(x3 + (2 * q), y1 * y2 + 4)), module, Merge::mLED + (q + 8)));
            addChild(createLightCentered<TinyLight<WhiteLight>>(mm2px(Vec(x3 + (2 * q), y1 * y2 + 6)), module, Merge::mLED + (q + 12)));
        }

        addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(8, y1 * 20.5)), module, Merge::mOUT));

        CenteredLabel *const topLabel = new CenteredLabel;
        topLabel->box.pos = Vec(11.5, 5);
        topLabel->text = "MERGE";
        addChild(topLabel);
    }
    void appendContextMenu(Menu *menu) override
    {
        Merge *module = dynamic_cast<Merge *>(this->module);

        menu->addChild(new MenuSeparator());
        menu->addChild(createBoolPtrMenuItem("Monophonic Inputs Only", "", &module->MONOP));
    }
};

Model *modelMerge = createModel<Merge, MergeWidget>("Merge");