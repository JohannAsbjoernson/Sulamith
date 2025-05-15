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
    int channels = -1;
    int autoCh = 0;
    int count = 0;

    json_t *dataToJson() override
    {
        json_t *rootJ = json_object();
        json_object_set_new(rootJ, "channels", json_integer(channels));
        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override
    {
        json_t *channelsJ = json_object_get(rootJ, "channels");
        if (channelsJ)
            channels = json_integer_value(channelsJ);
    }

    Merge()
    {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        for (int i = 0; i < 16; i++)
            configInput(mIN + i, string::f("Channel %d", i + 1));

        configOutput(mOUT, "Polyphonic Merge");
        lightDivider.setDivision(512);
        onReset();
    }

    void onReset() override
    {
        channels = -1;
    }
    void process(const ProcessArgs &args) override
    {
        int lastchannel = -1;

        float w = 0.f;
        float deltaTime = args.sampleTime * lightDivider.getDivision();

        for (int i = 0; i < 16; i++)
        {
            float v = 0.f;
            if (inputs[mIN + i].isConnected())
            {
                w = 1.f;
                lastchannel = i;
                v = inputs[mIN + i].getVoltage();
            }
            else if (!inputs[mIN + i].isConnected())
                w = 0.f;

            if (channels == 0)
                w = 0.f;
            else if (channels > 0 && channels == i)
                w = 1.f;
            else if (channels > 0 && channels < i)
                w = 0.f;

            if (channels >= 0)
            {
                if (channels == 16)
                    lights[mLED + 15].setSmoothBrightness(1.f, deltaTime);
                else if (channels != 16)
                    lights[mLED + 15].setSmoothBrightness(0.f, deltaTime);
                lights[mLED + (i - 1)].setSmoothBrightness(w, deltaTime);
            }
            else
                lights[mLED + (i)].setSmoothBrightness(w, deltaTime);
            outputs[mOUT].setVoltage(v, i);
        }
        autoCh = lastchannel + 1;
        outputs[mOUT].channels = ((channels >= 0) ? channels : (autoCh));
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

        menu->addChild(new MenuSeparator);

        std::vector<std::string> channelLabels;
        channelLabels.push_back(string::f("Automatic (%d)", module->autoCh));
        for (int i = 0; i <= 16; i++)
        {
            channelLabels.push_back(string::f("%d", i));
        }
        menu->addChild(createIndexSubmenuItem(
            "Channels", channelLabels,
            [=]()
            { return module->channels + 1; },
            [=](int i)
            { module->channels = i - 1; }));
    }
};

Model *modelMerge = createModel<Merge, MergeWidget>("Merge");