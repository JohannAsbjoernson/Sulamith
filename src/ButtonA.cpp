#include "plugin.hpp"
#include <string>
#include <iostream>

struct ButtonA : Module
{
    enum ParamIds
    {
        BT_PARAM,
        TGVA_PARAM,
        TGVB_PARAM,
        PROB_PARAM,
        GT_PARAM,
        RND_PARAM,
        NAME_PARAM,
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
    dsp::PulseGenerator TRout, gateGenerator;
    dsp::SlewLimiter gateSlew, civiSlew, ceviSlew;

    bool TG = true, GTD = false, RTRIG = false, RTRIG2 = false, BOOLSLEW = false, CEVESLEW = false, STARTUP = true;
    float RNG = 5.0, SH1 = 0.0f, SH2 = 0.0f, SH1A = 0.0f, SH2A = 0.0f, NAMEBTNold = 0.0f;
    int s3 = 0;
    std::string label = "";
    bool change = false;

    json_t *dataToJson() override
    {
        json_t *rootJ = json_object();
        json_object_set_new(rootJ, "label", json_stringn(label.c_str(), label.size()));
        json_object_set_new(rootJ, "STARTUP", json_boolean(STARTUP));
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
        json_t *startupJ = json_object_get(rootJ, "STARTUP");
        STARTUP = json_boolean_value(startupJ);
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

        configParam(BT_PARAM, 0, 1, 0, "Manual Trigger");
        configParam(TGVA_PARAM, 1.0f, 10.0f, 10.0f, "A: Constant-Voltage/RND-CV Range", "v");
        configParam(TGVB_PARAM, 1.0f, 10.0f, 10.0f, "B: Constant-Voltage/RND-CV Range", "v");
        getParamQuantity(TGVA_PARAM)->snapEnabled = true;
        getParamQuantity(TGVB_PARAM)->snapEnabled = true;

        configSwitch(RND_PARAM, 0, 2, 0, "Mode", {"A/B TOGGLE", "RND bipolar", "RND unipolar"});
        configParam(NAME_PARAM, 0.f, 10.f, 0.f, "Auto-Name");
        configParam(GT_PARAM, 0.f, 10.f, 0.f, "Set Gate Length (0 = Input)", "sec");
        configParam(PROB_PARAM, 0.f, 1.f, 1.f, "Probability", "%", 0, 100);

        getParamQuantity(RND_PARAM)->randomizeEnabled = false;
        getParamQuantity(TGVA_PARAM)->randomizeEnabled = false;
        getParamQuantity(TGVB_PARAM)->randomizeEnabled = false;

        configInput(TR_INPUT, "Trigger/Gate/Clock");
        configInput(TR2_INPUT, "Trigger/Gate/Clock");

        configOutput(TR_OUTPUT, "Trigger");
        configOutput(GT_OUTPUT, "Gate");
        configOutput(TG_OUTPUT, "A");
        configOutput(TGINV_OUTPUT, "B");
    }

    void onReset() override
    {
        lights[TR_LIGHT].setBrightness(0.f), lights[GT_LIGHT].setBrightness(0.f);
        lights[TG_LIGHT].setBrightness(1.f), lights[TGINV_LIGHT].setBrightness(0.f);
        TG = true;
        GTD = false;
        STARTUP = true;
        change = true;
    }

    void process(const ProcessArgs &args) override
    {

        // VARIABLES, PARAMS, TIME
        bool BT, TRIGD, TRIGR, PROB, GTA, SLEWING;
        float TR, GTL, GTSLEW, GTGO, CVSLEW, CVSLEW2;
        float TGS = params[RND_PARAM].getValue();
        float TGv = params[TGVA_PARAM].getValue(), TGv2 = params[TGVB_PARAM].getValue();
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

        // AUTO NAMING
        float NAMEBTN = params[NAME_PARAM].getValue();
        if (NAMEBTN > 0.f && (NAMEBTNold != NAMEBTN))
        {
            NAMEBTNold = NAMEBTN;
            if (label != "")
                label = "", change = true;
            std::string txtlabel[21] = {
                "SOLOMON",
                "MISCHPOKE",
                "CHUZPE",
                "ROCHUS",
                "TINNEF",
                "SULAMITH",
                "ZORES",
                "SCHICKSE",
                "MATZEN",
                "GANOVE",
                "STUSS",
                "ZOFF",
                "MACKE",
                "SCHMIRA",
                "KVELL",
                "MAZEL",
                "BUBBE",
                "GEVALT",
                "OY OY",
                "PLOTZ",
                "ZAFTIG"};
            int rndlabel = std::floor(random::uniform() * 10);
            label = txtlabel[rndlabel];
            change = true;
        }
        else
            NAMEBTNold = NAMEBTN;

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
                SH1A = (0.5 > random::uniform() ? SH1 : -SH1) * TGv;
                SH1A = clamp(SH1A, -TGv, TGv);
                SH2A = (0.5 > random::uniform() ? SH2 : -SH2) * TGv2;
                SH2A = clamp(SH2A, -TGv2, TGv2);
            }
            else if (TGS == 2)
            {
                SH1A = clamp((SH1 * TGv), 0.f, TGv);
                SH2A = clamp((SH2 * TGv2), 0.f, TGv2);
            }
        }
        else if (!BT)
            GTD = false;

        // GATE AND CV SLEW SETTINGS & PROCESS
        // float slewtime = 20;
        float s2[8] = {70, 45, 30, 20, 17, 15, 13, 10}, s1[9] = {0.3, 0.8, 2.3, 4.5, 6, 7.5, 9, 10.01, 10.01};
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
            civiSlew.setRiseFall(slewtime2 - TGv, slewtime2 - (TGv * 0.9));
            ceviSlew.setRiseFall(slewtime2 - TGv2, slewtime2 - (TGv2 * 0.9));
        }
        CVSLEW = civiSlew.process(deltaTime, SH1A);
        CVSLEW2 = ceviSlew.process(deltaTime, SH2A);
        float CVSLEW3 = CVSLEW / 10.f, CVSLEW4 = CVSLEW2 / 10.f;

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
            outputs[TGINV_OUTPUT].setVoltage(TG ? 0.0 : TGv2);
        }
        else if (TGS != 0 && CEVESLEW)
        {
            outputs[TG_OUTPUT].setVoltage(CVSLEW3);
            outputs[TGINV_OUTPUT].setVoltage(CVSLEW4);
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
            lights[TG_LIGHT].setSmoothBrightness(TG ? 1.f : 0.0f, deltaTime);
            lights[TGINV_LIGHT].setSmoothBrightness(!TG ? 1.f : 0.0f, deltaTime);
        }
        else if (TGS != 0 && CEVESLEW)
        {
            if (CVSLEW >= 0)
                lights[TG_LIGHT].setSmoothBrightness((CVSLEW / 10.f), deltaTime);
            else
                lights[TG_LIGHT].setSmoothBrightness((-CVSLEW / 10.f), deltaTime);
            if (CVSLEW2 >= 0)
                lights[TGINV_LIGHT].setSmoothBrightness((CVSLEW2 / 10.f), deltaTime);
            else
                lights[TGINV_LIGHT].setSmoothBrightness((-CVSLEW2 / 10.f), deltaTime);
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
        CustomLabel *textField = createWidget<CustomLabel>(Vec(3, 3));

        // textField->placeholder = "SULAMITH";
        textField->bgColor = nvgRGB(0x00, 0x00, 0x00);
        textField->fontPath = std::string(asset::plugin(pluginInstance, "res/fonts/Oswald-Regular.ttf"));
        textField->box.size = box.size;
        textField->multiline = false;
        textField->textOffset = Vec(-3.f, -6.3f);
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

        addOutput(createOutputCentered<PJ301Mvar>(mm2px(Vec(x1, y1 * 4.7)), module, ButtonA::TR_OUTPUT));
        addChild(createLightCentered<SmallLight<WhiteLight>>(mm2px(Vec(x2, y1 * 4.7 - y2)), module, ButtonA::TR_LIGHT));

        addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(x1, y1 * 5.7)), module, ButtonA::GT_OUTPUT));
        addChild(createLightCentered<SmallLight<WhiteLight>>(mm2px(Vec(x2, y1 * 5.7 - y2)), module, ButtonA::GT_LIGHT));
        addParam(createParamCentered<TrimpotW>(mm2px(Vec(x1, y1 * 6.4)), module, ButtonA::GT_PARAM));

        addOutput(createOutputCentered<PJ301Mvar2>(mm2px(Vec(x1 - 0.9, y1 * 7.3)), module, ButtonA::TG_OUTPUT));
        addChild(createLightCentered<SmallLight<WhiteLight>>(mm2px(Vec(x2 - 0.9, y1 * 7.3 - y2)), module, ButtonA::TG_LIGHT));
        addParam(createParamCentered<TrimpotSB>(mm2px(Vec(x1 - 2.5, y1 * 8 - 1.7)), module, ButtonA::TGVA_PARAM));
        addParam(createParamCentered<TrimpotSY>(mm2px(Vec(x1 + 2.5, y1 * 8 + 1.7)), module, ButtonA::TGVB_PARAM));

        addOutput(createOutputCentered<PJ301Mvar2>(mm2px(Vec(x1 + 0.9, y1 * 8.7)), module, ButtonA::TGINV_OUTPUT));
        addChild(createLightCentered<SmallLight<WhiteLight>>(mm2px(Vec(x2, y1 * 8.7 - y2)), module, ButtonA::TGINV_LIGHT));
        addParam(createParamCentered<BTSwitch>(mm2px(Vec(x1, y1 * 9.5)), module, ButtonA::RND_PARAM));

        addParam(createParamCentered<SGB>(mm2px(Vec(x1 - 4, y1 * 10.5)), module, ButtonA::NAME_PARAM));

        DisplayLabel *display1 = createWidget<DisplayLabel>(Vec(2.5, 350));
        display1->box.size = Vec(40, 15);
        display1->setModule(module);
        addChild(display1);

        CenteredLabel *const topLabel = new CenteredLabel;
        topLabel->box.pos = Vec(11.5, 5);
        topLabel->text = "BUTTON";
        addChild(topLabel);
    }
    void appendContextMenu(Menu *menu) override
    {
        ButtonA *module = dynamic_cast<ButtonA *>(this->module);

        menu->addChild(new MenuSeparator());
        menu->addChild(createBoolPtrMenuItem("Global Retrig", "", &module->RTRIG));
        menu->addChild(createBoolPtrMenuItem("Add Slew to custom Gates", "", &module->BOOLSLEW));
        menu->addChild(createBoolPtrMenuItem("Add Slew to random CV", "", &module->CEVESLEW));
    }
};

Model *modelButtonA = createModel<ButtonA, ButtonAWidget>("ButtonA");