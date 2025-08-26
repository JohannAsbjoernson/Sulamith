#include "plugin.hpp"

struct BPMClk : Module
{
    enum ParamIds
    {
        RUN_BUTTON,
        CLOCK_PARAM,
        CLOCK_DIV,
        CLOCK2_DIV,
        CLOCK3_DIV,
        NUM_PARAMS
    };
    enum InputIds
    {
        CLOCK_RUN,
        CLOCK_RES,
        NUM_INPUTS
    };
    enum OutputIds
    {
        CLOCK_OUTPUT,
        CLOCK2_OUTPUT,
        CLOCK3_OUTPUT,
        CLOCK4_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds
    {
        RUN_LED,
        NUM_LIGHTS
    };

    void onReset() override
    {
        resetClock();
    }

    void resetClock()
    {
        phase = 0.0;
        stepCount = 0;
        outputs[CLOCK_OUTPUT].setVoltage(0.f);
        outputs[CLOCK2_OUTPUT].setVoltage(0.f);
    }

    BPMClk()
    {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configSwitch(RUN_BUTTON, 0, 1, 1, "CLK is", {"Resting", "Running"});
        configParam(CLOCK_PARAM, -2.f, 2.32195f, 1.f, "BPM", "", 2.f, 60.f);
        configSwitch(CLOCK_DIV, 0, 13, 7, "Div/Mult", {"/32", "/24", "/16", "/12", "/8", "/4", "/3", "/2", "x1", "x2", "x4", "x8", "x16", "x32"});
        configSwitch(CLOCK2_DIV, 0, 13, 9, "Div/Mult", {"/32", "/24", "/16", "/12", "/8", "/4", "/3", "/2", "x1", "x2", "x4", "x8", "x16", "x32"});
        configSwitch(CLOCK3_DIV, 0, 13, 10, "Div/Mult", {"/32", "/24", "/16", "/12", "/8", "/4", "/3", "/2", "x1", "x2", "x4", "x8", "x16", "x32"});

        getParamQuantity(CLOCK_DIV)->snapEnabled = true;
        getParamQuantity(CLOCK2_DIV)->snapEnabled = true;
        getParamQuantity(CLOCK3_DIV)->snapEnabled = true;

        configInput(CLOCK_RUN, "Run");
        configInput(CLOCK_RES, "Reset");
        configOutput(CLOCK_OUTPUT, "Clock #1");
        configOutput(CLOCK2_OUTPUT, "Clock #2");
        configOutput(CLOCK3_OUTPUT, "Clock #3");
        configOutput(CLOCK4_OUTPUT, "Clock #4");
    }

    dsp::PulseGenerator gatePulse;
    dsp::SchmittTrigger resetTrigger;

    float phase = 0.0;
    float div[14] = {32, 24, 16, 12, 8, 4, 3, 2, 1, 2, 4, 8, 16, 32};
    int stepCount = 0, divdiv = 0, divdiv2 = 0, divdiv3 = 0, divdiv4 = 0;

    void process(const ProcessArgs &args) override
    {
        float deltaTime = args.sampleRate;

        float clockTime = powf(2.0, (params[CLOCK_PARAM].getValue())) * 32.f;
        phase += clockTime / deltaTime;

        if (resetTrigger.process(inputs[CLOCK_RES].getVoltage(), 0.1, 1.0))
            resetClock();

        bool nextStep = false;
        if ((params[RUN_BUTTON].getValue() <= 0 && inputs[CLOCK_RUN].getVoltage()))
            getParamQuantity(RUN_BUTTON)->setValue(1);
        else if (inputs[CLOCK_RUN].getVoltage())
        {
            getParamQuantity(RUN_BUTTON)->setValue(0);
            resetClock();
        }
        if ((params[RUN_BUTTON].getValue() > 0))
        {
            if (phase >= 1.0)
            {
                phase -= 1.0;
                nextStep = true;
            }

            if (nextStep)
            {
                stepCount = (stepCount + 1) % 1024;
                gatePulse.trigger(1e-3);
            }

            bool gpulse = gatePulse.process(2.0 / deltaTime);
            int dknob = (int)params[CLOCK_DIV].getValue(), dknob2 = (int)params[CLOCK2_DIV].getValue();
            int dknob3 = (int)params[CLOCK3_DIV].getValue();

            if (dknob <= 8)
                divdiv = 32.f * div[dknob];
            if (dknob > 8)
                divdiv = 32.f / div[dknob];

            if (dknob2 <= 8)
                divdiv2 = 32.f * div[dknob2];
            if (dknob2 > 8)
                divdiv2 = 32.f / div[dknob2];

            if (dknob3 <= 8)
                divdiv3 = 32.f * div[dknob3];
            if (dknob3 > 8)
                divdiv3 = 32.f / div[dknob3];

            outputs[CLOCK_OUTPUT].setVoltage(gpulse && (stepCount % 32 == 0) ? 10.0 : 0.0);
            outputs[CLOCK2_OUTPUT].setVoltage(gpulse && (stepCount % divdiv == 0) ? 10.0 : 0.0);
            outputs[CLOCK3_OUTPUT].setVoltage(gpulse && (stepCount % divdiv2 == 0) ? 10.0 : 0.0);
            outputs[CLOCK4_OUTPUT].setVoltage(gpulse && (stepCount % divdiv3 == 0) ? 10.0 : 0.0);
        }
        else
        {
            resetClock();
            getParamQuantity(RUN_BUTTON)->setValue(0);
        }

        float ledvelo = (params[RUN_BUTTON].getValue() || inputs[CLOCK_RUN].getVoltage()) ? 1.f : 0.f;
        lights[RUN_LED].setSmoothBrightness(ledvelo, deltaTime);
    }
};

struct BPMClkWidget : ModuleWidget
{

    BPMClkWidget(BPMClk *module)
    {
        setModule(module);

        box.size = Vec(RACK_GRID_WIDTH * 3, RACK_GRID_HEIGHT);

        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/panels/Panel3hp-dark.svg")));

        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(Vec(23, 24 * 1), module, BPMClk::RUN_BUTTON, BPMClk::RUN_LED));

        addInput(createInputCentered<PJ301MPort>(Vec(23, 24 * 2), module, BPMClk::CLOCK_RUN));
        addInput(createInputCentered<PortDark>(Vec(23, 24 * 3.6), module, BPMClk::CLOCK_RES));
        addParam(createParamCentered<RoundBlackKnob>(Vec(23, 24 * 5.4), module, BPMClk::CLOCK_PARAM));
        addOutput(createOutputCentered<ThemedPJ301MPort>(Vec(23, 24 * 6.6), module, BPMClk::CLOCK_OUTPUT));

        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(23, 24 * 8.4), module, BPMClk::CLOCK_DIV));
        addOutput(createOutputCentered<PJ301Mvar>(Vec(23, 24 * 9.5), module, BPMClk::CLOCK2_OUTPUT));
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(23, 24 * 10.8), module, BPMClk::CLOCK2_DIV));
        addOutput(createOutputCentered<PJ301Mvar>(Vec(23, 24 * 11.9), module, BPMClk::CLOCK3_OUTPUT));
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(23, 24 * 13.2), module, BPMClk::CLOCK3_DIV));
        addOutput(createOutputCentered<PJ301Mvar>(Vec(23, 24 * 14.3), module, BPMClk::CLOCK4_OUTPUT));

        CenteredLabel *const titleLabel = new CenteredLabel;
        titleLabel->box.pos = Vec(11.5, 5);
        titleLabel->text = "CLK";
        addChild(titleLabel);

        CenteredLabel *const titleLabel3 = new CenteredLabel;
        titleLabel3->box.pos = Vec(11.5, 36.2);
        titleLabel3->text = "RES";
        addChild(titleLabel3);

        CenteredLabel *const titleLabel4 = new CenteredLabel;
        titleLabel4->box.pos = Vec(11.5, 56.3);
        titleLabel4->text = "BPM";
        addChild(titleLabel4);

        CenteredLabel *const titleLabel5 = new CenteredLabel;
        titleLabel5->box.pos = Vec(11.5, 93.3);
        titleLabel5->text = "DI/MU";
        addChild(titleLabel5);
    }
};
Model *modelBPMClk = createModel<BPMClk, BPMClkWidget>("BPMClk");