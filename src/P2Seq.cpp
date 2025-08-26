#include "plugin.hpp"

struct P2Seq : Module
{
    enum ParamIds
    {
        P2_MD,
        P2_RD,
        P2_RDC,
        P2_RDR,
        P2_BT,
        NUM_PARAMS
    };
    enum InputIds
    {
        P2_IN,
        P2_TR,
        P2_RST,
        NUM_INPUTS
    };
    enum OutputIds
    {
        P2_OUT,
        NUM_OUTPUTS
    };
    enum LightIds
    {
        P2_LED1,
        P2_LED2,
        P2_BLED,
        ENUMS(VU_LIGHTS, 16),
        NUM_LIGHTS
    };

    dsp::ClockDivider connectionUpdater;
    dsp::SchmittTrigger buttonTrig;
    dsp::SchmittTrigger resetTrig;
    dsp::SchmittTrigger stepTrigger;

    P2Seq()
    {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        configParam(P2_BT, 0, 1, 0, "Manual Step Button");
        configSwitch(P2_MD, 0.f, 2.f, 0.f, "Sequencing Mode", {"Forwards", "Backwards", "Random"});
        configParam(P2_RD, 0.f, 1.f, 1.f, "Step Probability", "%", 0, 100);
        configParam(P2_RDC, 0.f, 1.f, 0.f, "RND CV Probability (RND * Distance + INPUT)", "%", 0, 100);
        // configParam(P2_RDR, 0.5f, 2.f, 0.85f, "RND CV Distance ", "* RND + IN");
        configSwitch(P2_RDR, 0, 9, 0, "RND range +-", {"0.2", "0.3v", "0.5v", "0.8v", "1.0v", "1.2v", "1.3v", "1.5v", "1.8v", "2.0v"});
        configInput(P2_IN, "Polyphonic Signal");
        configInput(P2_TR, "Trigger");
        configInput(P2_RST, "Reset");
        configOutput(P2_OUT, "Sequence");
        getParamQuantity(P2_RDR)->snapEnabled = true;

        configBypass(P2_IN, P2_OUT);

        for (int i = 0; i < 16; i++)
        {
            configLight(VU_LIGHTS + i, "LED #" + std::to_string(i + 1));
        }

        connectionUpdater.setDivision(1024);
    }

    float rrange[10] = {0.2, 0.3, 0.5, 0.8, 1.0, 1.2, 1.3, 1.5, 1.8, 2.0};
    int mod, chnr, step = 0, activeChannel = -1, lastChannel = -1, rr;
    bool sin = false, trr = false;
    float rnd1, rnd2, rnd3, rnd4, sprob, cprob, rndcv;

    void updateConnections()
    {
        sin = inputs[P2_IN].isConnected();
        if (!sin)
            outputs[P2_OUT].setVoltage(0);
    }

    void process(const ProcessArgs &args) override
    {

        if (connectionUpdater.process())
            updateConnections();

        float deltaTime = args.sampleTime * connectionUpdater.getDivision();

        mod = params[P2_MD].getValue();
        chnr = inputs[P2_IN].getChannels();
        trr = inputs[P2_TR].getVoltage() ? true : false;
        sprob = params[P2_RD].getValue();
        cprob = params[P2_RDC].getValue();
        rr = params[P2_RDR].getValue();
        bool p2BT = (buttonTrig.process(params[P2_BT].getValue()) ? true : false);
        bool p2RS = (resetTrig.process(inputs[P2_RST].getVoltage()) ? true : false);

        for (int i = 0; i < 16; i++)
        {
            if (i < chnr)
                lights[VU_LIGHTS + i].setBrightness(0.3f);
            else
                lights[VU_LIGHTS + i].setBrightness(0.f);
        }

        if (stepTrigger.process(trr) || p2BT || p2RS) // on trigger input
        {
            rnd3 = random::uniform();
            rnd2 = random::uniform();
            rnd1 = (random::uniform() * ((random::uniform() > 0.5f) ? (1.f * rrange[rr]) : (-1.f * rrange[rr])));

            lights[P2_BLED].setBrightnessSmooth(1.0f, deltaTime);

            if (rnd3 <= params[P2_RD].getValue())
            {
                lights[P2_LED1].setBrightness(0.f);
                if (mod == 0) // mode one: forwards sequence. steps are added. reset on max value.
                {
                    step++;
                    if (step >= chnr)
                        step = 0;
                    if (p2RS)
                        step = 0;
                }
                if (mod == 1) // mode two: backwards. steps are substracted. reset on min value.
                {
                    step--;
                    if (step < 0)
                        step = chnr - 1;
                    if (p2RS)
                        step = chnr - 1;
                }
                if (mod == 2) // mode three: random. random is generated between 0 to 1 & multiplied by total channels then rounded down to next integer value.
                    step = std::floor(random::uniform() * chnr);
            }
            else if (rnd3 > params[P2_RD].getValue())
                lights[P2_LED1].setBrightness(1.f);
        }
        else if (step >= chnr)
            step = chnr - 1;
        else
            lights[P2_BLED].setBrightnessSmooth(0.f, deltaTime);
        activeChannel = step; // after each trigger the step is the active channel

        if (chnr <= 0)
            activeChannel = -1;

        if (activeChannel > -1) // if there is an active channel, add it to the output.
        {
            if ((rnd2 <= params[P2_RDC].getValue()) && (rnd3 <= params[P2_RD].getValue()))
            {
                lights[P2_LED2].setBrightness(1.f);
                outputs[P2_OUT].setVoltage((rnd1 + inputs[P2_IN].getVoltage(activeChannel)));
            }

            else
            {
                lights[P2_LED2].setBrightness(0.f);
                outputs[P2_OUT].setVoltage(inputs[P2_IN].getVoltage(activeChannel));
            }

            lights[VU_LIGHTS + activeChannel].setBrightness(1.f);

            if (lastChannel != activeChannel)
            {
                lastChannel = activeChannel;
                for (int i = 0; i < 16; i++)
                {
                    if (i != activeChannel)
                        lights[VU_LIGHTS + i].setBrightness(0.f);
                }
            }
        }
    }
};

struct P2SeqWidget : ModuleWidget
{
    P2SeqWidget(P2Seq *module)
    {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/panels/Panel3hp-dark.svg")));

        float x = 23.f;
        float y = 24.f;

        addParam(createLightParamCentered<VCVLightButton<MediumSimpleLight<WhiteLight>>>(Vec(x, y * 1), module, P2Seq::P2_BT, P2Seq::P2_BLED));
        addInput(createInputCentered<PJ301MPort>(Vec(x, y * 2.2f), module, P2Seq::P2_IN));
        addInput(createInputCentered<PJ301Mvar>(Vec(x, y * 3.4f), module, P2Seq::P2_TR));
        addInput(createInputCentered<PortDark>(Vec(x, y * 4.6f), module, P2Seq::P2_RST));
        addParam(createParamCentered<ModeSwitch>(Vec(x - 11.3, y * 5.7f), module, P2Seq::P2_MD));
        addChild(createLightCentered<SmallSimpleLight<WhiteLight>>(Vec(x, y * 6.f), module, P2Seq::P2_LED1));
        addParam(createParamCentered<TrimpotSG>(Vec(x + 11.5, y * 5.7f), module, P2Seq::P2_RD));
        addParam(createParamCentered<TrimpotSG>(Vec(x - 11.5, y * 6.5f), module, P2Seq::P2_RDC));
        addChild(createLightCentered<SmallSimpleLight<WhiteLight>>(Vec(x, y * 6.8f), module, P2Seq::P2_LED2));
        addParam(createParamCentered<TrimpotSR>(Vec(x + 11.5, y * 6.5f), module, P2Seq::P2_RDR));

        addOutput(createOutputCentered<PJ3410Port>(Vec(x, y * 14.2f), module, P2Seq::P2_OUT));

        float xx = 10.f;
        float yy = 300.f;

        for (int i = 0; i < 4; i++)
        {
            addChild(createLightCentered<TinyLight<WhiteLight>>(Vec(xx + 5.f * (1.f + i), 5.00f + yy), module, P2Seq::VU_LIGHTS + i));
            addChild(createLightCentered<TinyLight<WhiteLight>>(Vec(xx + 5.f * (1.f + i), 10.0f + yy), module, P2Seq::VU_LIGHTS + (4 + i)));
            addChild(createLightCentered<TinyLight<WhiteLight>>(Vec(xx + 5.f * (1.f + i), 15.0f + yy), module, P2Seq::VU_LIGHTS + (8 + i)));
            addChild(createLightCentered<TinyLight<WhiteLight>>(Vec(xx + 5.f * (1.f + i), 20.0f + yy), module, P2Seq::VU_LIGHTS + (12 + i)));
        }

        CenteredLabel *const titleLabel = new CenteredLabel;
        titleLabel->box.pos = Vec(11.5, 5);
        titleLabel->text = "P2Seq";
        addChild(titleLabel);
    }
};

Model *modelP2Seq = createModel<P2Seq, P2SeqWidget>("P2Seq");