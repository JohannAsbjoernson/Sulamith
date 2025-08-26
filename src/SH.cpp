#include "plugin.hpp"

struct SH : Module
{
    enum ParamIds
    {
        A_PARAM,
        B_PARAM,
        C_PARAM,
        D_PARAM,
        QU_PARAM,
        A2_PARAM,
        B2_PARAM,
        C2_PARAM,
        D2_PARAM,
        QU2_PARAM,
        BT_PARAM,
        BT2_PARAM,
        BT3_PARAM,
        NUM_PARAMS
    };
    enum InputIds
    {
        CV_INPUT,
        GT_INPUT,
        CV2_INPUT,
        GT2_INPUT,
        CV3_INPUT,
        GT3_INPUT,
        NUM_INPUTS
    };
    enum OutputIds
    {
        SH_OUTPUT,
        SH2_OUTPUT,
        SH3_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds
    {
        BT_LED,
        QU_LED,
        BT2_LED,
        QU2_LED,
        BT3_LED,
        NUM_LIGHTS
    };

    dsp::SchmittTrigger clockTrigger[16], clockTrigger2[16], clockTrigger3[16];
    dsp::SchmittTrigger btTrigger, btTrigger2, btTrigger3;
    dsp::ClockDivider lightDivider, lightDivider2, lightDivider3;

    SH()
    {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        // SH1
        configParam(BT_PARAM, 0, 1, 0, "Manual Button");
        configParam(A_PARAM, -10.f, 10.f, -5.f, "Range A");
        configParam(B_PARAM, -10.f, 10.f, 5.f, "Range B");
        configParam(C_PARAM, 0, 16, 0, "Number of Channels (0 = auto)");
        configParam(D_PARAM, 0, 1, 1, "Probability", "%", 0, 100);
        configSwitch(QU_PARAM, 0, 1, 0, "Quantize to Semi", {"Off", "On"});

        getParamQuantity(A_PARAM)->snapEnabled = true;
        getParamQuantity(B_PARAM)->snapEnabled = true;
        getParamQuantity(C_PARAM)->snapEnabled = true;
        getParamQuantity(C_PARAM)->randomizeEnabled = false;
        getParamQuantity(QU_PARAM)->randomizeEnabled = false;

        configInput(CV_INPUT, "Sample Signal");
        configInput(GT_INPUT, "CLK/GT/TRIG");
        configOutput(SH_OUTPUT, "S&H/Noise");

        // SH2
        configParam(BT2_PARAM, 0, 1, 0, "Manual Button");
        configParam(A2_PARAM, -10.f, 10.f, -5.f, "Range A");
        configParam(B2_PARAM, -10.f, 10.f, 5.f, "Range B");
        configParam(C2_PARAM, 0, 16, 0, "Number of Channels (0 = auto)");
        configParam(D2_PARAM, 0, 1, 1, "Probability", "%", 0, 100);
        configSwitch(QU2_PARAM, 0, 1, 0, "Quantize to Semi", {"Off", "On"});

        getParamQuantity(A2_PARAM)->snapEnabled = true;
        getParamQuantity(B2_PARAM)->snapEnabled = true;
        getParamQuantity(C2_PARAM)->snapEnabled = true;
        getParamQuantity(C2_PARAM)->randomizeEnabled = false;
        getParamQuantity(QU2_PARAM)->randomizeEnabled = false;

        configInput(CV2_INPUT, "Sample Signal");
        configInput(GT2_INPUT, "CLK/GT/TRIG");
        configOutput(SH2_OUTPUT, "S&H/Noise");

        // SH3
        configParam(BT3_PARAM, 0, 1, 0, "Manual Button");

        configInput(CV3_INPUT, "Sample Signal");
        configInput(GT3_INPUT, "CLK/GT/TRIG");
        configOutput(SH3_OUTPUT, "S&H/Noise");

        lightDivider.setDivision(512), lightDivider2.setDivision(512), lightDivider3.setDivision(512);
    }

    float noize[16] = {}, noize2[16] = {}, noize3[16] = {};
    float cvled = 0.f, cvled2 = 0.f, cvled3 = 0.f;

    void process(const ProcessArgs &args) override
    {

        float lightTime = args.sampleTime * lightDivider.getDivision();

        float Ascl = params[A_PARAM].getValue(), Bscl = params[B_PARAM].getValue(), CHnum = params[C_PARAM].getValue(), Nmin, Nmax, Nrng, CVin[16], GTin[16];
        int CVch = inputs[CV_INPUT].getChannels(), GTch = inputs[GT_INPUT].getChannels(), CHin;
        bool CVcon = inputs[CV_INPUT].isConnected(), GTcon = inputs[GT_INPUT].isConnected(), QUstate = params[QU_PARAM].getValue();

        // NEEDS REPAIR!! Want it to be GTin polyphonic not VoltageSum
        float TR = rescale(inputs[GT_INPUT].getVoltageSum(), 0.1f, 2.f, 0.f, 1.f);

        bool BTbool = (bool(params[BT_PARAM].getValue() ? true : false) || TR >= 1.f || TR <= -1.f), TRIGD = btTrigger.process(BTbool ? 1.f : 0.f);

        // CH
        if (CHnum <= 0 && CVcon)
            CHin = CVch;
        else if (CHnum <= 0 && !CVcon && GTcon)
            CHin = GTch;
        else if (CHnum <= 0 && !CVcon && !GTcon)
        {
            if (GTcon)
                CHin = GTch;
            else
                CHin = 1;
        }
        else
            CHin = CHnum;
        outputs[SH_OUTPUT].setChannels(CHin);

        // MIN, MAX & RANGE
        Nmin = std::min(Ascl, Bscl);
        Nmax = std::max(Ascl, Bscl);
        Nrng = Nmax - Nmin;

        if (GTcon || TRIGD)
        {
            for (int i = 0; i < CHin; i++)
            {
                // TRIG
                if (inputs[GT_INPUT].isMonophonic())
                    GTin[i] = {inputs[GT_INPUT].getVoltage()};
                else if (inputs[GT_INPUT].isPolyphonic())
                {
                    GTin[i] = inputs[GT_INPUT].getVoltage(i);
                    if (CVcon && (CVch > GTch) && (i >= GTch)) // clone GTch 1 to match number of CVch
                    {
                        int y = i - GTch;
                        GTin[i] = {GTin[y]};
                    }
                }

                // serially clone CVch to match number of CHin
                CVin[i] = {inputs[CV_INPUT].getVoltage(i)};
                if (i >= CVch)
                {
                    int z = i - CVch;
                    CVin[i] = {CVin[z]};
                }

                // CREATE OUTPUT
                if (TRIGD)
                {
                    // NOISE
                    if (!CVcon)
                        noize[i] = {(random::uniform() < params[D_PARAM].getValue()) ? (clamp((random::uniform() * Nrng) + Nmin, -10.f, 10.f)) : noize[i]};

                    // S&H
                    else if (CVcon)
                        noize[i] = {clamp(inputs[CV_INPUT].getVoltage(i), -10.f, 10.f)};

                    // OUT
                    if (QUstate > 0)
                    {
                        float qua = floor(noize[i]), qub = noize[i] - qua, sem = 1.f / 12.f;
                        qub = round((qub / sem)) * sem;
                        noize[i] = qua + qub;
                    }
                    outputs[SH_OUTPUT].setVoltage(noize[i], i);
                }
            }
        }
        // Controls the Button Light (Fades)
        if (TRIGD)
            cvled = 1.f;
        else
            cvled = cvled - 0.0001;
        lights[BT_LED].setBrightnessSmooth(cvled, lightTime);
        lights[QU_LED].setBrightnessSmooth(QUstate, lightTime);

        ///////////////////////////////////////////////////////////////
        ///////////////// SECOND SECTION
        float lightTime2 = args.sampleTime * lightDivider2.getDivision();

        float Ascl2 = params[A2_PARAM].getValue(), Bscl2 = params[B2_PARAM].getValue(), CHnum2 = params[C2_PARAM].getValue(), Nmin2, Nmax2, Nrng2, CVin2[16], GTin2[16];
        int CVch2 = inputs[CV2_INPUT].getChannels(), GTch2 = inputs[GT2_INPUT].getChannels(), CHin2;
        bool CVcon2 = inputs[CV2_INPUT].isConnected(), GTcon2 = inputs[GT2_INPUT].isConnected(), QU2state = params[QU2_PARAM].getValue();

        float TR2 = rescale(inputs[GT2_INPUT].getVoltageSum(), 0.1f, 2.f, 0.f, 1.f);
        bool BTbool2 = (bool(params[BT2_PARAM].getValue() ? true : false) || TR2 >= 1.f || TR2 <= -1.f), TRIGD2 = btTrigger2.process(BTbool2 ? 1.f : 0.f);

        // CH2
        if (CHnum2 <= 0 && CVcon2)
            CHin2 = CVch2;
        else if (CHnum2 <= 0 && !CVcon2 && GTcon2)
            CHin2 = GTch2;
        else if (CHnum2 <= 0 && !CVcon2 && !GTcon2)
        {
            if (GTcon)
                CHin2 = GTch;
            else
                CHin2 = 1;
        }
        else
            CHin2 = CHnum2;
        outputs[SH2_OUTPUT].setChannels(CHin2);

        // MIN, MAX & RANGE2
        Nmin2 = std::min(Ascl2, Bscl2);
        Nmax2 = std::max(Ascl2, Bscl2);
        Nrng2 = Nmax2 - Nmin2;

        if (GTcon2 || TRIGD2)
        {
            for (int i = 0; i < CHin2; i++)
            {
                // TRIG2
                if (inputs[GT2_INPUT].isMonophonic())
                    GTin2[i] = {inputs[GT2_INPUT].getVoltage()};
                else if (inputs[GT2_INPUT].isPolyphonic())
                {
                    GTin2[i] = inputs[GT2_INPUT].getVoltage(i);
                    if (CVcon2 && (CVch2 > GTch2) && (i >= GTch2)) // clone GTch 1 to match number of CVch
                    {
                        int y2 = i - GTch2;
                        GTin2[i] = {GTin2[y2]};
                    }
                }

                // serially clone CVch to match number of CHin
                CVin2[i] = {inputs[CV2_INPUT].getVoltage(i)};
                if (i >= CVch2)
                {
                    int z = i - CVch2;
                    CVin2[i] = {CVin2[z]};
                }

                // CREATE OUTPUT2
                if (TRIGD2)
                {
                    // NOISE2
                    if (!CVcon2)
                        noize2[i] = {(random::uniform() < params[D2_PARAM].getValue()) ? (clamp((random::uniform() * Nrng2) + Nmin2, -10.f, 10.f)) : noize2[i]};
                    // S&H2
                    else if (CVcon2)
                        noize2[i] = {clamp(inputs[CV2_INPUT].getVoltage(i), -10.f, 10.f)};

                    // OUT2
                    if (QU2state > 0)
                    {
                        float qua = floor(noize2[i]), qub = noize2[i] - qua, sem = 1.f / 12.f;
                        qub = round((qub / sem)) * sem;
                        noize2[i] = qua + qub;
                    }
                    outputs[SH2_OUTPUT].setVoltage(noize2[i], i);
                }
            }
        }
        // Controls the Button Light (Fades)
        if (TRIGD2)
            cvled2 = 1.f;
        else
            cvled2 = cvled2 - 0.0001;
        lights[BT2_LED].setBrightnessSmooth(cvled2, lightTime2);
        lights[QU2_LED].setBrightnessSmooth(QU2state, lightTime2);

        ///////////////////////////////////////////////////////////////
        ///////////////// THIRD SECTION
        float lightTime3 = args.sampleTime * lightDivider3.getDivision();

        float Ascl3 = params[A2_PARAM].getValue(), Bscl3 = params[B2_PARAM].getValue(), CHnum3 = params[C2_PARAM].getValue(), Nmin3, Nmax3, Nrng3, CVin3[16], GTin3[16];
        int CVch3 = inputs[CV3_INPUT].getChannels(), GTch3 = inputs[GT3_INPUT].getChannels(), CHin3;
        bool CVcon3 = inputs[CV3_INPUT].isConnected(), GTcon3 = inputs[GT3_INPUT].isConnected();

        float TR3 = rescale(inputs[GT3_INPUT].getVoltageSum(), 0.1f, 2.f, 0.f, 1.f);
        bool BTbool3 = (bool(params[BT3_PARAM].getValue() ? true : false) || TR3 >= 1.f || TR3 <= -1.f), TRIGD3 = btTrigger3.process(BTbool3 ? 1.f : 0.f);

        // CH3
        if (CHnum3 <= 0 && CVcon3)
            CHin3 = CVch3;
        else if (CHnum3 <= 0 && !CVcon3 && GTcon3)
            CHin3 = GTch3;
        else if (CHnum3 <= 0 && !CVcon3 && !GTcon3)
        {
            if (GTcon)
                CHin3 = GTch;
            else if (GTcon2)
                CHin3 = GTch2;
            else
                CHin3 = 1;
        }
        else
            CHin3 = CHnum3;
        outputs[SH3_OUTPUT].setChannels(CHin3);

        // MIN, MAX & RANGE3
        Nmin3 = std::min(Ascl3, Bscl3);
        Nmax3 = std::max(Ascl3, Bscl3);
        Nrng3 = Nmax3 - Nmin3;

        if (GTcon3 || TRIGD3)
        {
            for (int i = 0; i < CHin3; i++)
            {
                // TRIG3
                if (inputs[GT3_INPUT].isMonophonic())
                    GTin3[i] = {inputs[GT3_INPUT].getVoltage()};
                else if (inputs[GT3_INPUT].isPolyphonic())
                {
                    GTin3[i] = inputs[GT3_INPUT].getVoltage(i);
                    if (CVcon3 && (CVch3 > GTch3) && (i >= GTch3)) // clone GTch 1 to match number of CVch
                    {
                        int y3 = i - GTch3;
                        GTin3[i] = {GTin3[y3]};
                    }
                }

                // serially clone CVch to match number of CHin
                CVin3[i] = {inputs[CV3_INPUT].getVoltage(i)};
                if (i >= CVch3)
                {
                    int z = i - CVch3;
                    CVin3[i] = {CVin3[z]};
                }

                // CREATE OUTPUT3
                if (TRIGD3)
                {
                    // NOISE3
                    if (!CVcon3)
                        noize3[i] = {(random::uniform() < params[D2_PARAM].getValue()) ? (clamp((random::uniform() * Nrng3) + Nmin3, -10.f, 10.f)) : noize3[i]};

                    // S&H3
                    else if (CVcon3)
                        noize3[i] = {clamp(inputs[CV3_INPUT].getVoltage(i), -10.f, 10.f)};

                    // OUT3
                    if (QU2state > 0)
                    {
                        float qua = floor(noize3[i]), qub = noize3[i] - qua, sem = 1.f / 12.f;
                        qub = round((qub / sem)) * sem;
                        noize3[i] = qua + qub;
                    }
                    outputs[SH3_OUTPUT].setVoltage(noize3[i], i);
                }
            }
        }
        // Controls the Button Light (Fades)
        if (TRIGD3)
            cvled3 = 1.f;
        else
            cvled3 = cvled3 - 0.0001;
        lights[BT3_LED].setBrightnessSmooth(cvled3, lightTime3);
    }
};

struct SHWidget : ModuleWidget
{
    SHWidget(SH *module)
    {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/panels/Panel3hp-dark.svg")));
        float col1 = 16, col2 = 36, col3 = 31, col4 = 11, dis = 24, dis2 = dis * 5.4, dis3 = dis * 10.8;

        addParam(createLightParamCentered<VCVLightButton<MediumSimpleLight<WhiteLight>>>(Vec(col2 - 1.8, dis * 1), module, SH::BT_PARAM, SH::BT_LED));
        addInput(createInputCentered<PJ301MPort>(Vec(col1, dis * 1.6), module, SH::GT_INPUT));
        addInput(createInputCentered<PortDark>(Vec(col3, dis * 2.4), module, SH::CV_INPUT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(Vec(col4, dis * 2.95), module, SH::QU_PARAM, SH::QU_LED));
        addOutput(createOutputCentered<PJ301Mvar>(Vec(col3, dis * 3.5), module, SH::SH_OUTPUT));
        addParam(createParamCentered<TrimpotSR>(Vec(col1 - 1, dis * 4.4), module, SH::A_PARAM));
        addParam(createParamCentered<TrimpotSB>(Vec(col3 + 1, dis * 4.4), module, SH::B_PARAM));
        addParam(createParamCentered<TrimpotSY>(Vec(col1 - 1, dis * 5.1), module, SH::C_PARAM));
        addParam(createParamCentered<TrimpotSG>(Vec(col3 + 1, dis * 5.1), module, SH::D_PARAM));

        addParam(createLightParamCentered<VCVLightButton<MediumSimpleLight<WhiteLight>>>(Vec(col2 - 1.8, dis2 + dis * 1), module, SH::BT2_PARAM, SH::BT2_LED));
        addInput(createInputCentered<PJ301MPort>(Vec(col1, dis2 + dis * 1.6), module, SH::GT2_INPUT));
        addInput(createInputCentered<PortDark>(Vec(col3, dis2 + dis * 2.4), module, SH::CV2_INPUT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(Vec(col4, dis2 + dis * 2.95), module, SH::QU2_PARAM, SH::QU2_LED));
        addOutput(createOutputCentered<PJ301Mvar>(Vec(col3, dis2 + dis * 3.5), module, SH::SH2_OUTPUT));
        addParam(createParamCentered<TrimpotSR>(Vec(col1 - 1, dis2 + dis * 4.4), module, SH::A2_PARAM));
        addParam(createParamCentered<TrimpotSB>(Vec(col3 + 1, dis2 + dis * 4.4), module, SH::B2_PARAM));
        addParam(createParamCentered<TrimpotSY>(Vec(col1 - 1, dis2 + dis * 5.1), module, SH::C2_PARAM));
        addParam(createParamCentered<TrimpotSG>(Vec(col3 + 1, dis2 + dis * 5.1), module, SH::D2_PARAM));

        addParam(createLightParamCentered<VCVLightButton<MediumSimpleLight<WhiteLight>>>(Vec(col2 - 1.8, dis3 + dis * 1), module, SH::BT3_PARAM, SH::BT3_LED));
        addInput(createInputCentered<PJ301MPort>(Vec(col1, dis3 + dis * 1.6), module, SH::GT3_INPUT));
        addInput(createInputCentered<PortDark>(Vec(col3, dis3 + dis * 2.4), module, SH::CV3_INPUT));
        addOutput(createOutputCentered<PJ301Mvar>(Vec(col3, dis3 + dis * 3.5), module, SH::SH3_OUTPUT));

        CenteredLabel *const titleLabel = new CenteredLabel;
        titleLabel->box.pos = Vec(11.5, 5);
        titleLabel->text = "S&H";
        addChild(titleLabel);
    }
};

Model *modelSH = createModel<SH, SHWidget>("SH");