/* Original Code:
 * GenEcho (Gendy / Grandy Echo module)
 * Samuel Laing - 2019
 *
 * VCV Rack module that uses granular stochastic methods to alter a sample
 * See: https://github.com/smbddha/sb-StochKit
 *
 * Edited by Johann Asbjoernson 2025
 * Changelog: Stereo, disabled Breakpoint Controlls, added DRY/WET with Auto Gain Adjustment & CV, Overall Gain, Delete Sample, Auto Reset on Startup, Sample length CV
 * Changelog: Noise removal (Through-Zero for Left input & Slew for Sample End), Gated Octave up +1 and +2, New Panel Design (4HP)
 */

#include "plugin.hpp"
#include "dsp/digital.hpp"
#include "dsp/resampler.hpp"
#include "wavetable.hpp"

#define MAX_BPTS 4096
#define MAX_SAMPLE_SIZE 44100

struct GenEcho : Module
{
    enum ParamIds
    {
        TRIG_PARAM,
        GATE_PARAM,
        SLEN_PARAM,
        GAIN_PARAM,
        CROSS_PARAM,
        TR_PARAM,
        RS_PARAM,
        DL_PARAM,
        O1_PARAM,
        O2_PARAM,
        NUM_PARAMS
    };
    enum InputIds
    {
        WAV0_INPUT,
        WAV1_INPUT,
        GATE_INPUT,
        DEL_INPUT,
        RST_INPUT,
        SLEN_INPUT,
        CROSS_INPUT,
        OCT1_INPUT,
        OCT2_INPUT,
        NUM_INPUTS
    };
    enum OutputIds
    {
        L_OUTPUT,
        R_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds
    {
        BLINK_LIGHT,
        NUM_LIGHTS
    };

    float phase = 1.0;
    float blinkPhase = 0.0;

    dsp::SchmittTrigger smpTrigger;
    dsp::SchmittTrigger gTrigger;
    dsp::SchmittTrigger g2Trigger;
    dsp::SchmittTrigger delTrigger;
    dsp::SchmittTrigger rstTrigger;
    dsp::SlewLimiter slewlim, slewlim2;
    float sample[MAX_SAMPLE_SIZE] = {0.f};
    float _sample[MAX_SAMPLE_SIZE] = {0.f};
    float sample2[MAX_SAMPLE_SIZE] = {0.f};
    float _sample2[MAX_SAMPLE_SIZE] = {0.f};

    unsigned int channels;
    unsigned int sampleRate;

    unsigned int sample_length = MAX_SAMPLE_SIZE;

    unsigned int idx = 0;

    // spacing between breakpoints... in samples rn
    unsigned int bpt_spc = 1500;
    unsigned int env_dur = bpt_spc / 2;

    // number of breakpoints - to be calculated according to size of
    // the sample
    unsigned int num_bpts = MAX_SAMPLE_SIZE / bpt_spc;

    float mAmps[MAX_BPTS] = {0.f};
    float mDurs[MAX_BPTS] = {1.f};

    Wavetable env = Wavetable(TRI);

    unsigned int index = 0;

    float max_amp_step = 0.05f;
    float max_dur_step = 0.05f;

    float amp = 0.f;
    float amp_next = 0.f;
    float g_idx = 0.f;
    float g_idx_next = 0.5f;

    // when true read in from wav0_input and store in the sample buffer
    bool sampling = false;
    bool sampling2 = false;
    bool has_sample = false;
    unsigned int s_i = 0;

    float bpts_sig = 1.f;
    float astp_sig = 1.f;
    float dstp_sig = 1.f;
    float astp = 1.f;
    float dstp = 1.f;
    bool is_accumulating = false;
    bool is_mirroring = false;

    gRandGen rg;
    DistType dt = LINEAR;

    float inA;
    float inB;

    GenEcho()
    {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        configParam(SLEN_PARAM, 0.01f, 1.f, 0.f, "SAMPLE LENGTH", "%", 0, 100);
        configParam(GAIN_PARAM, 0, 2, 1, "GAIN", "%", 0, 100);
        configParam(CROSS_PARAM, 0, 1, 0.5, "DRY/WET", "%", 0, 100);

        configParam(TR_PARAM, 0, 1, 0, "RECORD SAMPLE button");
        configParam(RS_PARAM, 0, 1, 0, "RESTART SAMPLE button");
        configParam(DL_PARAM, 0, 1, 0, "DELETE SAMPLE button");
        configParam(O1_PARAM, 0, 1, 0, "ONE OCTAVE UP button");
        configParam(O2_PARAM, 0, 1, 0, "TWO OCTAVES UP button");

        configInput(SLEN_INPUT, "SAMPLE LENGTH CV (offset)");
        configInput(WAV0_INPUT, "LEFT");
        configInput(WAV1_INPUT, "RIGHT");
        configInput(GATE_INPUT, "RECORD SAMPLE");
        configInput(RST_INPUT, "RESTART SAMPLE");
        configInput(DEL_INPUT, "DELETE SAMPLE");
        configInput(OCT1_INPUT, "ONE OCTAVE UP (gated)");
        configInput(OCT2_INPUT, "TWO OCTAVES UP (gated)");

        configInput(CROSS_INPUT, "DRY/WET CV (offset)");
        configOutput(L_OUTPUT, "LEFT");
        configOutput(R_OUTPUT, "RIGHT");
    }

    void process(const ProcessArgs &args) override;
};

void GenEcho::process(const ProcessArgs &args)
{
    float deltaTime = args.sampleTime;

    int snum = 55;
    slewlim.setRiseFall(snum, snum);
    slewlim2.setRiseFall(snum, snum);

    float amp_out = 0.0;
    float amp_out2 = 0.0;
    is_accumulating = (int)0;
    dt = (DistType)0;
    inA = inputs[WAV0_INPUT].getVoltage();
    inB = inputs[WAV1_INPUT].isConnected() ? inputs[WAV1_INPUT].getVoltage() : inputs[WAV0_INPUT].getVoltage();

    // handle the 3 switches for accumlating and mirror toggle
    // and probability distrobution selection
    is_accumulating = 0;
    is_mirroring = 0;
    dt = (DistType)0;

    // read in cv vals for astp, dstp and bpts
    bpts_sig = 5.f * dsp::quadraticBipolar(0);
    astp_sig = dsp::quadraticBipolar(0);
    dstp_sig = dsp::quadraticBipolar(0);

    max_amp_step = rescale(0 + (astp_sig / 4.f), 0.0, 1.0, 0.05, 0.3);
    max_dur_step = rescale(0 + (dstp_sig / 4.f), 0.0, 1.0, 0.01, 0.3);

    sample_length = (int)(clamp(params[SLEN_PARAM].getValue() + (inputs[SLEN_INPUT].getVoltage() / 10.f), 0.1, 1.f) * MAX_SAMPLE_SIZE);

    bpt_spc = (unsigned int)0 + 800;
    bpt_spc += (unsigned int)rescale(bpts_sig, -1.f, 1.f, 1.f, 200.f);
    num_bpts = sample_length / bpt_spc + 1;

    env_dur = bpt_spc / 2;

    // snap knob for selecting envelope for the grain
    int env_num = (int)clamp(roundf(4), 1.0f, 4.0f);

    if (env.et != (EnvType)env_num)
    {
        env.switchEnvType((EnvType)env_num);
    }

    // handle sample reset
    if ((delTrigger.process(inputs[DEL_INPUT].getVoltage() / 2.f)) || has_sample == false || params[DL_PARAM].getValue())
    {
        for (unsigned int i = 0; i < MAX_SAMPLE_SIZE; i++)
        {
            sample[i] = 0.f;
            sample2[i] = 0.f;
        }
        for (unsigned int i = 0; i < MAX_BPTS; i++)
        {
            mAmps[i] = 0.f;
            mDurs[i] = 0.f;
        }
        has_sample = true;
    }

    else if (rstTrigger.process(inputs[RST_INPUT].getVoltage() / 2.f) || params[RS_PARAM].getValue())
    {
        for (unsigned int i = 0; i < MAX_SAMPLE_SIZE; i++)
        {
            sample[i] = _sample[i];
            sample2[i] = _sample2[i];
        }
        for (unsigned int i = 0; i < MAX_BPTS; i++)
        {
            mAmps[i] = 0.f;
            mDurs[i] = 1.f;
        }
    }

    // handle sample trigger through gate
    if (gTrigger.process(inputs[GATE_INPUT].getVoltage() / 2.f) || params[TR_PARAM].getValue())
    {

        // reset accumulated breakpoint vals
        for (unsigned int i = 0; i < MAX_BPTS; i++)
        {
            mAmps[i] = 0.f;
            mDurs[i] = 1.f;
        }
        num_bpts = sample_length / bpt_spc;
        sampling = true;
        idx = 0;
        s_i = 0;
        params[TR_PARAM].setValue(0);
    }

    int count = 0;
    while (sampling)
    {
        // sampling2 = true;
        if (inA == 0.0f || count == 192000)
        {
            count = 0;
            sampling2 = true;
            sampling = false;
            break;
        }
        else
        {
            count++;
            sampling2 = false;
            sampling = true;
            continue;
        }
    }

    if (sampling2)
    {
        if (s_i >= MAX_SAMPLE_SIZE - 50)
        {
            float x, y, p;
            x = sample[s_i - 1];
            y = sample[0];
            p = 0.f;
            while (s_i < MAX_SAMPLE_SIZE)
            {
                sample[s_i] = (x * (1 - p)) + (y * p);
                sample2[s_i] = (x * (1 - p)) + (y * p);
                p += 1.f / 50.f;
                s_i++;
                if (s_i >= MAX_SAMPLE_SIZE - 400)
                {
                    sample[s_i] = slewlim.process(deltaTime, 0.4f);
                    sample2[s_i] = slewlim2.process(deltaTime, 0.4f);
                }
            }
            sampling2 = false;
        }
        else
        {
            sample[s_i] = inA;
            sample2[s_i] = inB;
            if (s_i <= 8400)
            {
                float sampleA[s_i] = {sample[s_i]};
                sample[s_i] *= 0.47;
                sample[s_i] = slewlim.process(deltaTime, sampleA[s_i]);
                float sampleB[s_i] = {sample2[s_i]};
                sample2[s_i] *= 0.47;
                sample2[s_i] = slewlim.process(deltaTime, sampleB[s_i]);
            }
            _sample[s_i] = sample[s_i];
            _sample2[s_i] = sample2[s_i];
            s_i++;
        }
        has_sample = true;
    }

    if (phase >= 1.0)
    {
        phase -= 1.0;

        amp = amp_next;
        index = (index + 1) % num_bpts;

        // adjust vals
        astp = max_amp_step * rg.my_rand(dt, random::normal()) * 0.f;
        dstp = max_dur_step * rg.my_rand(dt, random::normal()) * 0.f;

        mAmps[index] = wrap((is_accumulating ? mAmps[index] : 0.f) + astp, -1.0f, 1.0f);
        mDurs[index] = wrap(mDurs[index] + dstp, 0.5, 1.5);

        amp_next = mAmps[index];

        // step/adjust grain sample offsets
        g_idx = g_idx_next;
        g_idx_next = 0.0;
    }

    // change amp in sample buffer
    sample[idx] = wrap(sample[idx] + (amp * env.get(g_idx)), -5.f, 5.f);
    sample2[idx] = wrap(sample2[idx] + (amp * env.get(g_idx)), -5.f, 5.f);
    amp_out = clamp(sample[idx], -5.f, 5.f);
    amp_out2 = clamp(sample2[idx], -5.f, 5.f);

    // Possible Octave up
    int oct = 1;
    if (inputs[OCT1_INPUT].getVoltage() > 1.2f || params[O1_PARAM].getValue())
        oct += 1;
    if (inputs[OCT2_INPUT].getVoltage() > 1.2f || params[O2_PARAM].getValue())
        oct += 2;
    idx = (idx + oct) % sample_length;
    g_idx = fmod(g_idx + (2.f / (4.f * env_dur)), 1.f);
    g_idx_next = fmod(g_idx_next + (2.f / (4.f * env_dur)), 1.f);

    phase += 1.f / (mDurs[index] * bpt_spc);

    // get that amp OUT
    float gains = params[GAIN_PARAM].getValue();
    float cross = clamp(params[CROSS_PARAM].getValue() + (inputs[CROSS_INPUT].getVoltage() / 10), 0.f, 1.f);
    float crossamp = 1;
    if (cross <= 0.5)
        crossamp = 1 + cross;
    else
        crossamp = 1.5 - (cross - 0.5);
    amp_out = clamp((crossfade(inA, amp_out, cross) * crossamp) * gains, -5.f, 5.f);
    amp_out2 = clamp((crossfade(inB, amp_out2, cross) * crossamp) * gains, -5.f, 5.f);

    outputs[L_OUTPUT].setVoltage(amp_out);
    outputs[R_OUTPUT].setVoltage(amp_out2);
}

struct GenEchoWidget : ModuleWidget
{
    GenEchoWidget(GenEcho *module)
    {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/panels/GenEcho.svg")));
        float x = 30.f, x2 = 19.f, x3 = 33.f, y = 24.f;

        addParam(createParamCentered<RoundBlackKnob>(Vec(x, y * 2), module, GenEcho::SLEN_PARAM));
        addInput(createInputCentered<PortDark>(Vec(x, y * 3.2), module, GenEcho::SLEN_INPUT));

        addInput(createInputCentered<PJ301MPort>(Vec(15.f, y * 4.5), module, GenEcho::WAV0_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(45.f, y * 4.5), module, GenEcho::WAV1_INPUT));

        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(x2, y * 5.7), module, GenEcho::GAIN_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(x2, y * 6.7), module, GenEcho::GATE_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(x2, y * 7.7), module, GenEcho::RST_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(x2, y * 8.7), module, GenEcho::DEL_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(x2, y * 9.7), module, GenEcho::OCT1_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(x2, y * 10.7), module, GenEcho::OCT2_INPUT));

        addParam(createParamCentered<WBsmall>(Vec(x3, y * 6.3f), module, GenEcho::TR_PARAM));
        addParam(createParamCentered<WBsmall>(Vec(x3, y * 7.3f), module, GenEcho::RS_PARAM));
        addParam(createParamCentered<WBsmall>(Vec(x3, y * 8.3f), module, GenEcho::DL_PARAM));
        addParam(createParamCentered<WBsmallt>(Vec(x3, y * 9.3f), module, GenEcho::O1_PARAM));
        addParam(createParamCentered<WBsmallt>(Vec(x3, y * 10.3f), module, GenEcho::O2_PARAM));

        addParam(createParamCentered<RoundBlackKnob>(Vec(x, y * 12.4), module, GenEcho::CROSS_PARAM));
        addInput(createInputCentered<PortDark>(Vec(x, y * 13.6), module, GenEcho::CROSS_INPUT));

        addOutput(createOutputCentered<PJ301Mvar>(Vec(15.f, y * 14.6), module, GenEcho::L_OUTPUT));
        addOutput(createOutputCentered<PJ301Mvar>(Vec(45.f, y * 14.6), module, GenEcho::R_OUTPUT));
    }
};

Model *modelGenEcho = createModel<GenEcho, GenEchoWidget>("GenEcho");