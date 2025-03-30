#include "plugin.hpp"
#include <string>
#include <iostream>

struct ButtonA : Module
{
    enum ParamIds
    {
        BT_PARAM,
        TGV_PARAM,
        PROB_PARAM,
        GT_PARAM,
        RND_PARAM,
        NUM_PARAMS
    };
    enum InputIds
    {
        TR_INPUT,
        TR2_INPUT,
        NUM_INPUTS
    };
    enum OutputIds
    {
        TR_OUTPUT,
        GT_OUTPUT,
        TG_OUTPUT,
        TGINV_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds
    {
        TR_LIGHT,
        GT_LIGHT,
        TG_LIGHT,
        TGINV_LIGHT,
        NUM_LIGHTS
    };

    dsp::SchmittTrigger TRin;
    dsp::PulseGenerator TRout;
    dsp::PulseGenerator gateGenerator;
    dsp::SlewLimiter gateSlew;
    dsp::SlewLimiter civiSlew;
    dsp::SlewLimiter ceviSlew;

    bool TG = true, GTD = false, RTRIG = false, RTRIG2 = false, BOOLSLEW = false, CEVESLEW = false;
    float RNG = 5.0, SH1 = 0.0f, SH2 = 0.0f, SH1A = 0.0f, SH2A = 0.0f;
    std::string label = "";
    bool change = false;

    json_t *dataToJson() override
    {
        json_t *rootJ = json_object();
        json_object_set_new(rootJ, "label", json_stringn(label.c_str(), label.size()));
        json_object_set_new(rootJ, "RTRIG", json_boolean(RTRIG));
        json_object_set_new(rootJ, "BOOLSLEW", json_boolean(BOOLSLEW));
        json_object_set_new(rootJ, "CEVESLEW", json_boolean(CEVESLEW));

        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override
    {
        json_t *text1J = json_object_get(rootJ, "label");
        if (text1J)
        {
            label = json_string_value(text1J);
            change = true;
        }
        json_t *retrigJ = json_object_get(rootJ, "RTRIG");
        RTRIG = json_boolean_value(retrigJ);
        json_t *gtslewJ = json_object_get(rootJ, "BOOLSLEW");
        BOOLSLEW = json_boolean_value(gtslewJ);
        json_t *cvslewJ = json_object_get(rootJ, "CEVESLEW");
        CEVESLEW = json_boolean_value(cvslewJ);
    }

    ButtonA()
    {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        configParam(BT_PARAM, 0, 1, 0, "ButtonA");
        configParam(TGV_PARAM, 1.0f, 10.0f, 10.0f, "Set Toggle Constant Voltage / SH Range");
        getParamQuantity(TGV_PARAM)->snapEnabled = true;
        configSwitch(RND_PARAM, 0.f, 2.f, 0.f, "", {"TOGGLE", "RND bipolar", "RND unipolar"});
        configParam(GT_PARAM, 0.f, 10.f, 0.f, "Set Gate Length (0 = Same as Input)", "sec");
        configParam(PROB_PARAM, 0.f, 1.f, 1.f, "Probability", "%", 0, 100);

        getParamQuantity(RND_PARAM)->randomizeEnabled = false;
        getParamQuantity(TGV_PARAM)->randomizeEnabled = false;

        configInput(TR_INPUT, "Trigger/Gate/Clock");
        configInput(TR2_INPUT, "Trigger/Gate/Clock");

        configOutput(TR_OUTPUT, "Trigger");
        configOutput(GT_OUTPUT, "Gate");
        configOutput(TG_OUTPUT, "A (Toggle/SH)");
        configOutput(TGINV_OUTPUT, "B (Toggle/SH)");
    }

    void onReset() override
    {
        lights[TR_LIGHT].setBrightness(0.f), lights[GT_LIGHT].setBrightness(0.f);
        lights[TG_LIGHT].setBrightness(1.f), lights[TGINV_LIGHT].setBrightness(0.f);
        TG = true, GTD = false;
        label = "SOLOMO", change = true;
    }
    int s3 = 0;
    void process(const ProcessArgs &args) override
    {
        // VARIABLES, PARAMS, TIME
        bool BT, TRIGD, TRIGR, PROB, GTA, SLEWING;
        float TR, GTL, GTSLEW, GTGO, CVSLEW, CVSLEW2;
        float TGS = params[RND_PARAM].getValue();
        float TGv = params[TGV_PARAM].getValue(), TGv2 = (TGv / 2);
        float deltaTime = args.sampleTime;

        // INPUT OR LOGIC, INTERNAL TRIG, EXTERNAL PULSES
        TR = rescale(inputs[TR_INPUT].getVoltageSum(), 0.1f, 2.f, 0.f, 1.f);
        TR += rescale(inputs[TR2_INPUT].getVoltageSum(), 0.1f, 2.f, 0.f, 1.f);
        BT = (bool(params[BT_PARAM].getValue() ? true : false) || TR >= 1.f || TR <= -1.f);
        TRIGD = TRin.process(BT ? 1.0f : 0.0f);
        TRIGR = TRout.process(deltaTime);
        GTA = gateGenerator.process(deltaTime) ? true : false;
        GTL = params[GT_PARAM].getValue();
        if (GTL < 0.2f && GTL > 0.f)
            GTL = 0.2f;

        // SLEW AND RETRIG PREPARATIONS
        if (GTA && BOOLSLEW)
            GTGO = 10.f;
        else
            GTGO = 0.f;
        if (outputs[GT_OUTPUT].getVoltage() > 0.f)
            SLEWING = true;
        else
            SLEWING = false;
        if (RTRIG && BOOLSLEW)
            RTRIG2 = SLEWING;
        else if (RTRIG && !BOOLSLEW)
            RTRIG2 = GTA;
        else
            RTRIG2 = false;

        // STEP PROB, STEP CONDITIONS, STEP ACTIONS
        if (TRIGD)
            PROB = ((random::uniform() < params[PROB_PARAM].getValue()));

        if (TRIGD && PROB && !RTRIG2)
        {
            TRout.trigger(1e-3f);
            TG = !TG;
            GTD = true;

            if (GTL > 0.f && !GTA && !SLEWING)
                gateGenerator.trigger(GTL);

            SH1 = random::uniform();
            SH2 = random::uniform();

            if (TGS == 1)
            {
                SH1A = (0.5 > random::uniform() ? SH1 : -SH1) * TGv2;
                SH1A = clamp(SH1A, -TGv2, TGv2);
                SH2A = (0.5 > random::uniform() ? SH2 : -SH2) * TGv2;
                SH2A = clamp(SH2A, -TGv2, TGv2);
            }
            else if (TGS == 2)
            {
                SH1A = clamp((SH1 * TGv), 0.f, TGv);
                SH2A = clamp((SH2 * TGv), 0.f, TGv);
            }
        }
        else if (!BT)
            GTD = false;

        // GATE AND CV SLEW SETTINGS & PROCESS
        // float slewtime = 20;
        float s2[8] = {80, 55, 40, 30, 20, 17, 15, 13}, s1[9] = {0.3, 0.8, 2.3, 4.5, 6, 7.5, 9, 10.01, 10.01};
        float slewtime2 = 13;

        if (BOOLSLEW)
        {
            while ((params[GT_PARAM].getValue() > s1[s3]) && (params[GT_PARAM].getValue() < s1[s3 + 1]))
                s3++;
            if (s3 >= 8)
                s3 = 0;
            gateSlew.setRiseFall(s2[s3] - GTL, s2[s3] - (GTL * 0.7));
        }
        GTSLEW = clamp(gateSlew.process(deltaTime, GTGO), 0.f, 10.f);

        if (TGS != 0 && CEVESLEW)
        {
            if (TGv <= 2)
                slewtime2 = 14;
            else if (TGv <= 6)
                slewtime2 = 11;
            else if (TGv > 6)
                slewtime2 = 9;
            civiSlew.setRiseFall(slewtime2 - TGv2, slewtime2 - (TGv2 * 0.9));
            ceviSlew.setRiseFall(slewtime2 - TGv2, slewtime2 - (TGv2 * 0.9));
        }
        CVSLEW = civiSlew.process(deltaTime, SH1A);
        CVSLEW2 = ceviSlew.process(deltaTime, SH2A);

        // OUTPUTS
        outputs[TR_OUTPUT].setVoltage(TRIGR ? 10.0 : 0.0);

        if (GTL > 0.f && BOOLSLEW)
            outputs[GT_OUTPUT].setVoltage(GTSLEW);
        else if (GTL > 0.f && !BOOLSLEW)
            outputs[GT_OUTPUT].setVoltage(GTA ? 10.0 : 0.0);
        else
            outputs[GT_OUTPUT].setVoltage(GTD ? 10.0 : 0.0);

        if (TGS == 0)
        {
            outputs[TG_OUTPUT].setVoltage(TG ? TGv : 0.0);
            outputs[TGINV_OUTPUT].setVoltage(TG ? 0.0 : TGv);
        }
        else if (TGS != 0 && CEVESLEW)
        {
            outputs[TG_OUTPUT].setVoltage(CVSLEW);
            outputs[TGINV_OUTPUT].setVoltage(CVSLEW2);
        }
        else if (TGS != 0 && !CEVESLEW)
        {
            outputs[TG_OUTPUT].setVoltage(SH1A);
            outputs[TGINV_OUTPUT].setVoltage(SH2A);
        }

        // LIGHTS
        lights[TR_LIGHT].setSmoothBrightness(TRIGR, deltaTime);

        if (GTL > 0.f && BOOLSLEW)
            lights[GT_LIGHT].setSmoothBrightness(GTSLEW, deltaTime);
        else if (GTL > 0.f && !BOOLSLEW)
            lights[GT_LIGHT].setSmoothBrightness(GTA, deltaTime);
        else
            lights[GT_LIGHT].setSmoothBrightness(GTD, deltaTime);

        if (TGS == 0)
        {
            lights[TG_LIGHT].setSmoothBrightness(TG, deltaTime);
            lights[TGINV_LIGHT].setSmoothBrightness(!TG, deltaTime);
        }
        else if (TGS != 0 && CEVESLEW)
        {
            lights[TG_LIGHT].setSmoothBrightness(CVSLEW, deltaTime);
            lights[TGINV_LIGHT].setSmoothBrightness(CVSLEW2, deltaTime);
        }
        else if (TGS != 0 && !CEVESLEW)
        {
            lights[TG_LIGHT].setSmoothBrightness(SH1, deltaTime);
            lights[TGINV_LIGHT].setSmoothBrightness(SH2, deltaTime);
        }
    }
};
// TEXT INPUT LABEL
struct CustomLabel : LedDisplayTextField
{
    ButtonA *module;

    void step() override
    {
        LedDisplayTextField::step();
        if (module && module->change)
        {
            setText(module->label);
            module->change = false;
        }
    }

    void onChange(const ChangeEvent &e) override
    {
        if (module)
            module->label = getText();
    }
};

struct DisplayLabel : LedDisplay
{
    void setModule(ButtonA *module)
    {
        CustomLabel *textField = createWidget<CustomLabel>(Vec(0, 0));

        // textField->placeholder = "SCHULA";
        textField->box.size = box.size;
        textField->multiline = false;
        textField->textOffset = -2.5;
        textField->color = nvgRGB(0xff, 0xff, 0xff);
        textField->module = module;
        addChild(textField);
    }
};

struct ButtonAWidget : ModuleWidget
{

    ButtonAWidget(ButtonA *module)
    {
        setModule(module);

        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/panels/Panel3hp-dark.svg")));

        float y1 = 12;
        float y2 = -4;
        float x1 = 7.5;
        float x2 = 13;

        addParam(createParamCentered<WhiteButton>(mm2px(Vec(8, 13)), module, ButtonA::BT_PARAM));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8, 25)), module, ButtonA::TR_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8, 35)), module, ButtonA::TR2_INPUT));
        addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(8, 45.5)), module, ButtonA::PROB_PARAM));

        addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(x1, y1 * 4.7)), module, ButtonA::TR_OUTPUT));
        addChild(createLightCentered<SmallLight<WhiteLight>>(mm2px(Vec(x2, y1 * 4.7 - y2)), module, ButtonA::TR_LIGHT));

        addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(x1, y1 * 5.7)), module, ButtonA::GT_OUTPUT));
        addChild(createLightCentered<SmallLight<WhiteLight>>(mm2px(Vec(x2, y1 * 5.7 - y2)), module, ButtonA::GT_LIGHT));
        addParam(createParamCentered<Trimpot>(mm2px(Vec(x1, y1 * 6.4)), module, ButtonA::GT_PARAM));

        addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(x1, y1 * 7.3)), module, ButtonA::TG_OUTPUT));
        addChild(createLightCentered<SmallLight<WhiteLight>>(mm2px(Vec(x2, y1 * 7.3 - y2)), module, ButtonA::TG_LIGHT));

        addParam(createParamCentered<Trimpot>(mm2px(Vec(x1, y1 * 8)), module, ButtonA::TGV_PARAM));

        addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(x1, y1 * 8.7)), module, ButtonA::TGINV_OUTPUT));
        addChild(createLightCentered<SmallLight<WhiteLight>>(mm2px(Vec(x2, y1 * 8.7 - y2)), module, ButtonA::TGINV_LIGHT));
        addParam(createParamCentered<CKSSThreeHorizontal>(mm2px(Vec(x1, y1 * 9.4)), module, ButtonA::RND_PARAM));

        DisplayLabel *display1 = createWidget<DisplayLabel>(Vec(2.5, 350));
        display1->box.size = Vec(40, 15);
        display1->setModule(module);
        addChild(display1);

        CenteredLabel *const topLabel = new CenteredLabel;
        topLabel->box.pos = Vec(11.5, 5);
        topLabel->text = "BUTTON";
        addChild(topLabel);
    }
    void appendContextMenu(Menu *menu) override;
};

// CONTEXT MENU
struct ButtonRTRItem : MenuItem
{
    ButtonA *bt1;
    void onAction(const event::Action &e) override
    {
        bt1->RTRIG = false;
    }
    void step() override
    {
        rightText = CHECKMARK(!bt1->RTRIG);
    }
};
struct ButtonRTR2Item : MenuItem
{
    ButtonA *bt1;
    void onAction(const event::Action &e) override
    {
        bt1->RTRIG = true;
    }
    void step() override
    {
        rightText = CHECKMARK(bt1->RTRIG);
    }
};
struct ButtonSlewItem : MenuItem
{
    ButtonA *bt1;
    void onAction(const event::Action &e) override
    {
        bt1->BOOLSLEW = true;
    }
    void step() override
    {
        rightText = CHECKMARK(bt1->BOOLSLEW);
    }
};
struct ButtonSlew2Item : MenuItem
{
    ButtonA *bt1;
    void onAction(const event::Action &e) override
    {
        bt1->BOOLSLEW = false;
    }
    void step() override
    {
        rightText = CHECKMARK(!bt1->BOOLSLEW);
    }
};
struct ButtonCVSItem : MenuItem
{
    ButtonA *bt1;
    void onAction(const event::Action &e) override
    {
        bt1->CEVESLEW = true;
    }
    void step() override
    {
        rightText = CHECKMARK(bt1->CEVESLEW);
    }
};
struct ButtonCVS2Item : MenuItem
{
    ButtonA *bt1;
    void onAction(const event::Action &e) override
    {
        bt1->CEVESLEW = false;
    }
    void step() override
    {
        rightText = CHECKMARK(!bt1->CEVESLEW);
    }
};

void ButtonAWidget::appendContextMenu(Menu *menu)
{
    menu->addChild(new MenuSeparator());

    ButtonA *bt1 = dynamic_cast<ButtonA *>(module);
    assert(bt1);

    menu->addChild(construct<MenuLabel>(&MenuLabel::text, "Global ReTrig"));
    ButtonRTRItem *bt1RTRIG1Item = createMenuItem<ButtonRTRItem>("ON");
    bt1RTRIG1Item->bt1 = bt1;
    menu->addChild(bt1RTRIG1Item);
    ButtonRTR2Item *bt1RTRIG2Item = createMenuItem<ButtonRTR2Item>("OFF");
    bt1RTRIG2Item->bt1 = bt1;
    menu->addChild(bt1RTRIG2Item);
    menu->addChild(construct<MenuLabel>(&MenuLabel::text, "turn off to sync custom GT w/ RND CV"));
    menu->addChild(new MenuSeparator());
    menu->addChild(construct<MenuLabel>(&MenuLabel::text, "Gate (custom length) Slewing"));
    ButtonSlewItem *btsItem = createMenuItem<ButtonSlewItem>("ON");
    btsItem->bt1 = bt1;
    menu->addChild(btsItem);
    ButtonSlew2Item *bts2Item = createMenuItem<ButtonSlew2Item>("OFF");
    bts2Item->bt1 = bt1;
    menu->addChild(bts2Item);
    menu->addChild(new MenuSeparator());
    menu->addChild(construct<MenuLabel>(&MenuLabel::text, "Random CV Slewing"));
    ButtonCVSItem *cvsItem = createMenuItem<ButtonCVSItem>("ON");
    cvsItem->bt1 = bt1;
    menu->addChild(cvsItem);
    ButtonCVS2Item *cvs2Item = createMenuItem<ButtonCVS2Item>("OFF");
    cvs2Item->bt1 = bt1;
    menu->addChild(cvs2Item);
    menu->addChild(new MenuSeparator());
}

Model *modelButtonA = createModel<ButtonA, ButtonAWidget>("ButtonA");