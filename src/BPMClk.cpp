// GNU GENERAL PUBLIC LICENSE
// Version 3, 29 June 2007
// Shabang Clock

#include "plugin.hpp"

struct BPMClk : Module
{
    enum BPMModes
    {
        BPM_CV,
        BPM_P2,
        BPM_P4,
        BPM_P8,
        BPM_P12,
        BPM_P24,
        NUM_BPM_MODES
    };
    enum ParamIds
    {
        CLOCK_BT,
        CLOCK_BPM,
        CLOCK1_DIV,
        CLOCK2_DIV,
        CLOCK3_DIV,
        NUM_PARAMS
    };
    enum InputIds
    {
        CLOCK_RES,
        CLOCK_TIN,
        CLOCK_EXT,
        NUM_INPUTS
    };
    enum OutputIds
    {
        CLOCK_OUT,
        DIV1_OUT,
        DIV2_OUT,
        DIV3_OUT,
        NUM_OUTPUTS
    };
    enum LightIds
    {
        TOGGLE_LIGHT,
        TR0_LIGHT,
        TR1_LIGHT,
        TR2_LIGHT,
        TR3_LIGHT,
        NUM_LIGHTS
    };

    dsp::SchmittTrigger toggleTrig, bpmInputTrig, resetTrig;
    dsp::PulseGenerator gatePulses[4];
    dsp::BooleanTrigger boolTrig;

    std::string n1;
    std::string n2;
    std::string n3;

    int div1[23] = {48, 32, 24, 16, 12, 8, 6, 5, 4, 3, 2, 1, 2, 3, 4, 5, 6, 8, 12, 16, 24, 32, 48};
    int div2[23] = {48, 32, 24, 16, 12, 8, 6, 5, 4, 3, 2, 1, 2, 3, 4, 5, 6, 8, 12, 16, 24, 32, 48};
    int div3[23] = {48, 32, 24, 16, 12, 8, 6, 5, 4, 3, 2, 1, 2, 3, 4, 5, 6, 8, 12, 16, 24, 32, 48};
    bool divGT[4] = {};
    int bpmInputMode = BPM_CV;
    int extPulseIndex = 0;
    int ppqn = 0;
    float period = 0.0;
    int timeOut = 1; // seconds
    float currentBPM = 120.0;
    float clockFreq = 2.0; // Hz
    float phases[4] = {};
    float rhythm[3] = {};
    int rhythm1, rhythm2, rhythm3, rr1, rr2, rr3;
    int runs;
    bool runin = true, rbool = false;

    BPMClk()
    {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configSwitch(CLOCK_BT, 0, 1, 1, "Clock", {"Off", "On"});
        configParam(CLOCK_BPM, -2.0, 2.32195, 1.0, "Tempo", " BPM", 2.0, 60.0);
        configSwitch(CLOCK1_DIV, 0, 22, 11, "Div/Mult", {"/48", "/32", "/24", "/16", "/12", "/8", "/6", "/5", "/4", "/3", "/2", "x1", "x2", "x3", "x4", "x5", "x6", "x8", "x12", "x16", "x24", "x32", "x48"});
        configSwitch(CLOCK2_DIV, 0, 22, 11, "Div/Mult", {"/48", "/32", "/24", "/16", "/12", "/8", "/6", "/5", "/4", "/3", "/2", "x1", "x2", "x3", "x4", "x5", "x6", "x8", "x12", "x16", "x24", "x32", "x48"});
        configSwitch(CLOCK3_DIV, 0, 22, 11, "Div/Mult", {"/48", "/32", "/24", "/16", "/12", "/8", "/6", "/5", "/4", "/3", "/2", "x1", "x2", "x3", "x4", "x5", "x6", "x8", "x12", "x16", "x24", "x32", "x48"});

        configInput(CLOCK_TIN, "Clock ON/OFF");
        configInput(CLOCK_RES, "Reset");
        configInput(CLOCK_EXT, "External clock");

        configOutput(CLOCK_OUT, "Master Clock");
        configOutput(DIV1_OUT, "Clock 1");
        configOutput(DIV2_OUT, "Clock 2");
        configOutput(DIV3_OUT, "Clock 3");

        rhythm[0] = params[CLOCK1_DIV].getValue();
        rhythm[1] = params[CLOCK2_DIV].getValue();
        rhythm[2] = params[CLOCK3_DIV].getValue();
    }

    void checkPhases()
    {
        for (int i = 0; i < 4; i++)
        {
            if (phases[i] >= 1.0)
                phases[i] = 0.0;
            if (phases[i] == 0.0)
                gatePulses[i].trigger(1e-3f);
            // if (i > 0)
            // {
            //     if (phases[i] == 0.0)
            //         gatePulses[i].trigger(1e-3f);
            // }
            // else
            // {
            //     if (phases[i] == 0.0)
            //         gatePulses[i].trigger(1e-3f);
            // }
        }
    }

    void resetPhases()
    {
        for (int i = 0; i < 4; i++)
            phases[i] = 0.0;
    }

    json_t *dataToJson() override
    {
        json_t *rootJ = json_object();

        json_object_set_new(rootJ, "runs", json_integer(runs));
        json_object_set_new(rootJ, "extmode", json_integer(bpmInputMode));

        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override
    {
        json_t *runsJ = json_object_get(rootJ, "runs");
        if (runsJ)
            runs = json_integer_value(runsJ);

        json_t *extmodeJ = json_object_get(rootJ, "extmode");
        if (extmodeJ)
            bpmInputMode = json_integer_value(extmodeJ);
    }

    void process(const ProcessArgs &args) override
    {
        if (resetTrig.process(inputs[CLOCK_RES].getVoltage()))
            resetPhases();
        runs = params[CLOCK_BT].getValue();
        runin = boolTrig.process(inputs[CLOCK_TIN].getVoltage() ? true : false);
        if (runin)
        {
            if (runs > 0)
            {
                getParamQuantity(CLOCK_BT)->setValue(0);
                runs = 0;
                if (rbool)
                    resetPhases();
                rbool = false;
            }
            else
            {
                getParamQuantity(CLOCK_BT)->setValue(1);
                runs = 1;
                rbool = true;
            }
        }
        lights[TOGGLE_LIGHT].setBrightness(runs ? 1.0 : 0.0);

        rhythm1 = div1[(int)params[CLOCK1_DIV].getValue()];
        rhythm2 = div2[(int)params[CLOCK2_DIV].getValue()];
        rhythm3 = div3[(int)params[CLOCK3_DIV].getValue()];
        rr1 = params[CLOCK1_DIV].getValue();
        rr2 = params[CLOCK2_DIV].getValue();
        rr3 = params[CLOCK3_DIV].getValue();

        for (int i = 0; i < 4; i++)
            divGT[i] = false;

        bool bpmDetect = false;
        if (inputs[CLOCK_EXT].isConnected())
        {
            if (bpmInputMode == BPM_CV)
                clockFreq = 2.0 * std::pow(2.0, inputs[CLOCK_EXT].getVoltage());

            else
            {
                bpmDetect = bpmInputTrig.process(inputs[CLOCK_EXT].getVoltage());

                if (bpmDetect)
                    runs = 1;

                switch (bpmInputMode)
                {
                case BPM_P2:
                    ppqn = 2;
                    break;
                case BPM_P4:
                    ppqn = 4;
                    break;
                case BPM_P8:
                    ppqn = 8;
                    break;
                case BPM_P12:
                    ppqn = 12;
                    break;
                case BPM_P24:
                    ppqn = 24;
                    break;
                }
            }
        }
        else
        {
            float bpmParam = params[CLOCK_BPM].getValue();
            clockFreq = (int)round(std::pow(2.0, bpmParam) * 60) / 60.f;
        }

        currentBPM = clockFreq * 60;

        if (runs)
        {
            if (bpmInputMode != BPM_CV && inputs[CLOCK_EXT].isConnected())
            {
                period += args.sampleTime;
                if (period > timeOut)
                    runs = false;
                if (bpmDetect)
                {
                    if (extPulseIndex > 1)
                        clockFreq = (1.0 / period) / (float)ppqn;

                    extPulseIndex++;
                    if (extPulseIndex >= ppqn)
                        extPulseIndex = 0;
                    period = 0.0;
                }
            }

            if (rhythm[0] != rhythm1 && phases[0] == 0.0)
            {
                rhythm[0] = (float)rhythm1;
                phases[1] = 0.0;
            }
            if (rhythm[1] != rhythm2 && phases[1] == 0.0)
            {
                rhythm[1] = (float)rhythm2;
                phases[2] = 0.0;
            }
            if (rhythm[2] != rhythm3 && phases[2] == 0.0)
            {
                rhythm[2] = (float)rhythm3;
                phases[3] = 0.0;
            }
            float d1 = (params[CLOCK1_DIV].getValue() >= 11 ? rhythm[0] : (1 / rhythm[0]));
            float d2 = (params[CLOCK2_DIV].getValue() > 10 ? rhythm[1] : (1 / rhythm[1]));
            float d3 = (params[CLOCK3_DIV].getValue() > 10 ? rhythm[2] : (1 / rhythm[2]));
            phases[0] += clockFreq * args.sampleTime;
            float accFreq = clockFreq * d1;
            phases[1] += accFreq * args.sampleTime;
            accFreq = clockFreq * d2;
            phases[2] += accFreq * args.sampleTime;
            accFreq = clockFreq * d3;
            phases[3] += accFreq * args.sampleTime;
            checkPhases();
        }
        else
            resetPhases();

        for (int i = 0; i < 4; i++)
            divGT[i] = gatePulses[i].process(1.0 / args.sampleRate);

        n1 = std::to_string(div1[(int)params[CLOCK1_DIV].getValue()]);
        n2 = std::to_string(div2[(int)params[CLOCK2_DIV].getValue()]);
        n3 = std::to_string(div3[(int)params[CLOCK3_DIV].getValue()]);

        if (outputs[CLOCK_OUT].isConnected())
            outputs[CLOCK_OUT].setVoltage(divGT[0] ? 10.0 : 0.0);
        else
            outputs[CLOCK_OUT].setVoltage(0.0);
        if (outputs[DIV1_OUT].isConnected())
            outputs[DIV1_OUT].setVoltage(divGT[1] ? 10.0 : 0.0);
        else
            outputs[DIV1_OUT].setVoltage(0.0);
        if (outputs[DIV2_OUT].isConnected())
            outputs[DIV2_OUT].setVoltage(divGT[2] ? 10.0 : 0.0);
        else
            outputs[DIV2_OUT].setVoltage(0.0);
        if (outputs[DIV3_OUT].isConnected())
            outputs[DIV3_OUT].setVoltage(divGT[3] ? 10.0 : 0.0);
        else
            outputs[DIV3_OUT].setVoltage(0.0);

        float deltaTime = args.sampleTime;
        lights[TR0_LIGHT].setSmoothBrightness(divGT[0] ? 1.f : 0.f, deltaTime);
        lights[TR1_LIGHT].setSmoothBrightness(divGT[1] ? 1.f : 0.f, deltaTime);
        lights[TR2_LIGHT].setSmoothBrightness(divGT[2] ? 1.f : 0.f, deltaTime);
        lights[TR3_LIGHT].setSmoothBrightness(divGT[3] ? 1.f : 0.f, deltaTime);
    }
};

struct ExternalClockModeValueItem : MenuItem
{
    BPMClk *module;
    BPMClk::BPMModes bpmMode;
    void onAction(const event::Action &e) override
    {
        module->bpmInputMode = bpmMode;
    }
};

struct ExternalClockModeItem : MenuItem
{
    BPMClk *module;
    Menu *createChildMenu() override
    {
        Menu *menu = new Menu;
        std::vector<std::string> bpmModeNames = {"CV (Impromptu [experimental]))", "2 PPQN", "4 PPQN", "8 PPQN", "12 PPQN", "24 PPQN"};
        for (int i = 0; i < BPMClk::NUM_BPM_MODES; i++)
        {
            BPMClk::BPMModes bpmMode = (BPMClk::BPMModes)i;
            ExternalClockModeValueItem *item = new ExternalClockModeValueItem;
            item->text = bpmModeNames[i];
            item->rightText = CHECKMARK(module->bpmInputMode == bpmMode);
            item->module = module;
            item->bpmMode = bpmMode;
            menu->addChild(item);
        }
        return menu;
    }
};

struct BPMDisplay : Widget
{
    std::string text;
    int fontSize;
    NVGcolor color;
    BPMClk *module;
    BPMDisplay(int _fontSize = 9)
    {
        fontSize = _fontSize;
        box.size.y = BND_WIDGET_HEIGHT;
        color = nvgRGB(255, 255, 255);
    }
    void draw(const DrawArgs &args) override
    {
        if (module == NULL)
            return;

        // int bpm = int(std::pow(2.0, module->params[BPMClk::CLOCK_BPM].getValue()) * 60);
        int bpm = static_cast<int>(round(module->currentBPM));
        text = std::to_string(bpm) + " BPM";
        nvgTextAlign(args.vg, NVG_ALIGN_CENTER + NVG_ALIGN_TOP);
        nvgFillColor(args.vg, color);
        nvgFontSize(args.vg, fontSize);
        nvgText(args.vg, 0, 0, text.c_str(), NULL);
    }
};

struct RatioDisplay : Widget
{
    std::string text1, text2, text3;
    int fontSize;
    NVGcolor color;
    BPMClk *module;
    RatioDisplay(int _fontSize = 8)
    {
        fontSize = _fontSize;
        box.size.y = BND_WIDGET_HEIGHT;
    }
    void draw(const DrawArgs &args) override
    {
        if (module == NULL)
            return;

        color = nvgRGB(255, 255, 255);

        int nm1 = module->rr1;
        text1 = (nm1 < 18 && nm1 > 4) ? "  " : "";
        text1 = (nm1 < 11 ? " /" : "x");
        text1 += module->n1;

        int nm2 = (int)module->rr2;
        text2 = (nm2 < 18 && nm2 > 4) ? "  " : "";
        text2 = (nm2 < 11 ? " /" : "x");
        text2 += module->n2;

        int nm3 = (int)module->rr3;
        text3 = nm3 < 18 && nm3 > 4 ? "  " : "";
        text3 = (nm3 < 11 ? " /" : "x");
        text3 += module->n3;

        nvgTextAlign(args.vg, NVG_ALIGN_LEFT + NVG_ALIGN_TOP);
        nvgFillColor(args.vg, color);
        nvgFontSize(args.vg, fontSize);
        nvgText(args.vg, 0, 0, text1.c_str(), NULL);
        nvgText(args.vg, 0, 55.0, text2.c_str(), NULL);
        nvgText(args.vg, 0, 110.0, text3.c_str(), NULL);
    }
};

struct BPMClkWidget : ModuleWidget
{
    BPMDisplay *bpmLabel = new BPMDisplay();
    RatioDisplay *ratioLabel1 = new RatioDisplay();

    BPMClkWidget(BPMClk *module)
    {
        setModule(module);

        box.size = Vec(RACK_GRID_WIDTH * 3, RACK_GRID_HEIGHT);

        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/panels/Panel3hp-dark.svg")));

        bpmLabel->module = module;
        bpmLabel->box.pos = Vec(23.f, 115.f);
        addChild(bpmLabel);

        ratioLabel1->module = module;
        ratioLabel1->box.pos = Vec(4.5, 213.f);
        ratioLabel1->box.size.x = 20;
        ratioLabel1->text2 = "5";

        addChild(ratioLabel1);

        addInput(createInputCentered<PJ301MPort>(Vec(23, 24 * 1.2), module, BPMClk::CLOCK_TIN));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(Vec(35, 24 * 0.45), module, BPMClk::CLOCK_BT, BPMClk::TOGGLE_LIGHT));
        addInput(createInputCentered<PJ301MPort>(Vec(23, 24 * 2.76), module, BPMClk::CLOCK_RES));

        addInput(createInputCentered<PortDark>(Vec(23, 24 * 4.2), module, BPMClk::CLOCK_EXT));
        addParam(createParamCentered<RoundBlackKnob>(Vec(23, 24 * 5.8), module, BPMClk::CLOCK_BPM));
        addOutput(createOutputCentered<ThemedPJ301MPort>(Vec(23, 24 * 7.1), module, BPMClk::CLOCK_OUT));
        addChild(createLightCentered<SmallLight<WhiteLight>>(Vec(8, 24 * 7.5), module, BPMClk::TR0_LIGHT));

        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(23, 24 * 8.5), module, BPMClk::CLOCK1_DIV));
        addOutput(createOutputCentered<PJ301Mvar>(Vec(23, 24 * 9.6), module, BPMClk::DIV1_OUT));
        addChild(createLightCentered<SmallLight<WhiteLight>>(Vec(8, 24 * 10), module, BPMClk::TR1_LIGHT));

        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(23, 24 * 10.8), module, BPMClk::CLOCK2_DIV));
        addOutput(createOutputCentered<PJ301Mvar>(Vec(23, 24 * 11.9), module, BPMClk::DIV2_OUT));
        addChild(createLightCentered<SmallLight<WhiteLight>>(Vec(8, 24 * 12.3), module, BPMClk::TR2_LIGHT));

        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(23, 24 * 13.1), module, BPMClk::CLOCK3_DIV));
        addOutput(createOutputCentered<PJ301Mvar>(Vec(23, 24 * 14.2), module, BPMClk::DIV3_OUT));
        addChild(createLightCentered<SmallLight<WhiteLight>>(Vec(8, 24 * 14.6), module, BPMClk::TR3_LIGHT));

        CenteredLabel *const titleLabel = new CenteredLabel;
        titleLabel->box.pos = Vec(5.5, 5);
        titleLabel->text = "CLK";
        addChild(titleLabel);

        CenteredLabel *const titleLabel2 = new CenteredLabel;
        titleLabel2->box.pos = Vec(11.5, 25.8);
        titleLabel2->text = "RES";
        addChild(titleLabel2);

        CenteredLabel *const titleLabel3 = new CenteredLabel;
        titleLabel3->box.pos = Vec(11.5, 43.2);
        titleLabel3->text = "SYNC";
        addChild(titleLabel3);

        CenteredLabel *const titleLabel5 = new CenteredLabel;
        titleLabel5->box.pos = Vec(11.5, 93.4);
        titleLabel5->text = "___";
        addChild(titleLabel5);
    }

    void appendContextMenu(Menu *menu) override
    {
        BPMClk *module = dynamic_cast<BPMClk *>(this->module);
        menu->addChild(new MenuEntry);

        ExternalClockModeItem *extClockModeItem = new ExternalClockModeItem;
        extClockModeItem->text = "External Clock Mode";
        extClockModeItem->rightText = RIGHT_ARROW;
        extClockModeItem->module = module;
        menu->addChild(extClockModeItem);
    }
};

Model *modelBPMClk = createModel<BPMClk, BPMClkWidget>("BPMClk");