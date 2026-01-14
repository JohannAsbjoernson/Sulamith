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
#define MAX_SAMPLE_SIZE 48000

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
        DL_PARAM,
        OC_PARAM,
        ENUMS(STOR, 3),
        NUM_PARAMS
    };
    enum InputIds
    {
        WAV0_INPUT,
        WAV1_INPUT,
        GATE_INPUT,
        DEL_INPUT,
        SLEN_INPUT,
        CROSS_INPUT,
        OCT_INPUT,
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

    dsp::SchmittTrigger gTrigger;
    dsp::SchmittTrigger delTrigger;

    float phase = 1.0;
    unsigned int sample_length = MAX_SAMPLE_SIZE;
    float sample[MAX_SAMPLE_SIZE] = {0.f};
    float sample2[MAX_SAMPLE_SIZE] = {0.f};
    float inA, inB;
    bool sampling = false, sampling2 = false, has_sample = true;
    unsigned int si = 0, idx = 0;

    // Likely most variables below here are barely working
    unsigned int index = 0;
    float amp = 0.f, amp_next = 0.f;
    float g_idx = 0.f, g_idx_next = 0.5f;
    // spacing between breakpoints... in samples rn
    unsigned int bpt_spc = 1500;
    unsigned int env_dur = bpt_spc / 2;
    unsigned int num_bpts = MAX_SAMPLE_SIZE / bpt_spc;
    float mAmps[MAX_BPTS] = {0.f}, mDurs[MAX_BPTS] = {1.f};
    float max_amp_step = 0.05f, max_dur_step = 0.05f;
    float bpts_sig = 1.f, astp_sig = 1.f, dstp_sig = 1.f, astp = 1.f, dstp = 1.f;
    Wavetable env = Wavetable(SIN);
    gRandGen rg;
    DistType dt = LINEAR;

    GenEcho()
    {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        configParam(SLEN_PARAM, 0.15f, 0.9f, 0.3f, "SAMPLE LENGTH", "%", 0, 100);
        configParam(GAIN_PARAM, 0, 2, 0.7, "GAIN", "%", 0, 100);
        configParam(CROSS_PARAM, 0, 1, 0.5, "DRY/WET", "%", 0, 100);

        configParam(TR_PARAM, 0, 1, 0, "RECORD SAMPLE button");
        configParam(DL_PARAM, 0, 1, 0, "DELETE SAMPLE button");
        configSwitch(OC_PARAM, 0, 3, 0, "OCTAVE UP", {"+0", "+1", "+2", "+3"});

        configInput(SLEN_INPUT, "SAMPLE LENGTH CV (offset)");
        configInput(WAV0_INPUT, "LEFT");
        configInput(WAV1_INPUT, "RIGHT");
        configInput(GATE_INPUT, "RECORD SAMPLE");
        configInput(DEL_INPUT, "DELETE SAMPLE");
        configInput(OCT_INPUT, "OCT +1 (+ button)");

        configInput(CROSS_INPUT, "DRY/WET CV (offset)");
        configOutput(L_OUTPUT, "LEFT");
        configOutput(R_OUTPUT, "RIGHT");
    }

    json_t *dataToJson() override
    {
        json_t *rootJ = json_object();
        json_object_set_new(rootJ, "Sampl", json_boolean(has_sample));
        for (int i = 0; i < 3; i++)
        {
            for (int n = 0; n < MAX_SAMPLE_SIZE; n++)
            {
                json_t *json_loop1 = json_array();
                json_array_append_new(json_loop1, json_real(sample[n]));
                std::string name1 = string::f("LoopA%d", n);
                json_object_set_new(rootJ, name1.c_str(), json_loop1);

                json_t *json_loop2 = json_array();
                json_array_append_new(json_loop2, json_real(sample2[n]));
                std::string name2 = string::f("LoopB%d", n);
                json_object_set_new(rootJ, name2.c_str(), json_loop2);
            }
        }
        return rootJ;
    }
    void dataFromJson(json_t *rootJ) override
    {
        json_t *samplsJ = json_object_get(rootJ, "Sampl");
        has_sample = json_boolean_value(samplsJ);

        for (int n = 0; n < MAX_SAMPLE_SIZE; n++)
        {
            std::string name1 = string::f("LoopA%d", n);
            std::string name2 = string::f("LoopB%d", n);
            json_t *loop1J = json_object_get(rootJ, name1.c_str());
            json_t *loop2J = json_object_get(rootJ, name2.c_str());
            size_t index;
            json_t *value;

            json_array_foreach(loop1J, index, value)
                sample[n] = json_real_value(value);
            json_array_foreach(loop2J, index, value)
                sample2[n] = json_real_value(value);
        }
    }

    void process(const ProcessArgs &args) override;
};

void GenEcho::process(const ProcessArgs &args)
{

    float amp_out = 0.0;
    float amp_out2 = 0.0;
    float volume = params[GAIN_PARAM].getValue();
    int count = 0;
    inA = inputs[WAV0_INPUT].getVoltage() * volume;
    inB = inputs[WAV1_INPUT].isConnected() ? inputs[WAV1_INPUT].getVoltage() : inputs[WAV0_INPUT].getVoltage() * volume;

    sample_length = (int)(clamp(params[SLEN_PARAM].getValue() + (inputs[SLEN_INPUT].getVoltage() / 10.f), 0.1, 1.f) * MAX_SAMPLE_SIZE);

    // read in cv vals for astp, dstp and bpts
    bpts_sig = 5.f * dsp::quadraticBipolar(0), astp_sig = dsp::quadraticBipolar(0), dstp_sig = dsp::quadraticBipolar(0);
    max_amp_step = rescale(0 + (astp_sig / 4.f), 0.0, 1.0, 0.05, 0.3), max_dur_step = rescale(0 + (dstp_sig / 4.f), 0.0, 1.0, 0.01, 0.3);
    bpt_spc = (unsigned int)0 + 800, bpt_spc += (unsigned int)rescale(bpts_sig, -1.f, 1.f, 1.f, 200.f);
    num_bpts = sample_length / bpt_spc + 1;
    env_dur = bpt_spc / 2;

    int env_num = 0;
    if (env.et != (EnvType)env_num)
        env.switchEnvType((EnvType)env_num);

    // RECORD BUFFER
    if (gTrigger.process(inputs[GATE_INPUT].getVoltage() / 2.f) || params[TR_PARAM].getValue())
    {
        for (unsigned int i = 0; i < MAX_BPTS; i++)
            mAmps[i] = 0.f, mDurs[i] = 1.f;
        num_bpts = sample_length / bpt_spc;
        sampling = true;
        idx = 0, si = 0;
        params[TR_PARAM].setValue(0);
    }
    // DELETE BUFFER
    if ((delTrigger.process(inputs[DEL_INPUT].getVoltage() / 2.f)) || params[DL_PARAM].getValue())
    {
        for (unsigned int i = 0; i < MAX_SAMPLE_SIZE; i++)
            sample[i] = 0.f, sample2[i] = 0.f;
        for (unsigned int i = 0; i < MAX_BPTS; i++)
            mAmps[i] = 0.f, mDurs[i] = 0.f;
        has_sample = false;
    }
    // WEIRD AS FUCK - WHY DOESN'T IT WORK OTHERWISE?
    while (sampling)
    {
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
    // RAISES AND LOWERS VOLUME AT BUFFER START AND END
    if (sampling2)
    {

        if (si < 4000)
        {
            while (si < MAX_SAMPLE_SIZE - 44000)
            {
                sample[si] = inA, sample2[si] = inB;
                sample[si] *= 1 / 4000;
                sample2[si] *= 1 / 4000;
                si++;
            }
        }
        else if (si >= MAX_SAMPLE_SIZE - 3000)
        {
            while (si < MAX_SAMPLE_SIZE)
            {
                sample[si] = inA, sample2[si] = inB;
                sample[si] *= 1 / 4500;
                sample2[si] *= 1 - (1 / 3000);
                si++;
            }
            sampling2 = false;
        }
        else
        {
            sample[si] = inA, sample2[si] = inB;
            si++;
        }
        has_sample = true;
    }
    // MOST OF THIS DOES NOTHING, TODO: FIND OUT WHAT CAN BE DELETED
    if (phase >= 1.f)
    {
        phase -= 1.f;
        amp = amp_next;
        index = (index + 1) % num_bpts;
        astp = max_amp_step * rg.my_rand(dt, random::normal()) * 0.f;
        dstp = max_dur_step * rg.my_rand(dt, random::normal()) * 0.f;
        mAmps[index] = wrap(mAmps[index] + astp, -1.0f, 1.0f);
        mDurs[index] = wrap(mDurs[index] + dstp, 0.5, 1.5);
        amp_next = mAmps[index];
        g_idx = g_idx_next;
        g_idx_next = 0.0;
    }

    sample[idx] = wrap(sample[idx] + (amp * env.get(g_idx)), -5.f, 5.f);
    sample2[idx] = wrap(sample2[idx] + (amp * env.get(g_idx)), -5.f, 5.f);

    float gains = params[GAIN_PARAM].getValue();
    amp_out = clamp(sample[idx] * gains, -5.f, 5.f);
    amp_out2 = clamp(sample2[idx] * gains, -5.f, 5.f);

    int oct = 1 + params[OC_PARAM].getValue() + (inputs[OCT_INPUT].getVoltage() > 1 ? 1 : 0);

    idx = (idx + oct) % sample_length;
    g_idx = fmod(g_idx + (2.f / (4.f * env_dur)), 1.f);
    g_idx_next = fmod(g_idx_next + (2.f / (4.f * env_dur)), 1.f);
    phase += 1.f / (mDurs[index] * bpt_spc);

    // THE CROSSFADE adds a lot of movement when input and loop are mixed
    float cross = clamp(params[CROSS_PARAM].getValue() + (inputs[CROSS_INPUT].getVoltage() / 10), 0.f, 1.f);
    float crossamp = 1;
    if (cross <= 0.5)
        crossamp = 1 + cross;
    else
        crossamp = 1.5 - (cross - 0.5);
    amp_out = clamp((crossfade(inA, amp_out, cross) * crossamp), -5.f, 5.f);
    amp_out2 = clamp((crossfade(inB, amp_out2, cross) * crossamp), -5.f, 5.f);
    outputs[L_OUTPUT].setVoltage(amp_out);
    outputs[R_OUTPUT].setVoltage(amp_out2);
}

struct GenEchoWidget : ModuleWidget
{
    GenEchoWidget(GenEcho *module)
    {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/panels/GenEcho.svg")));
        float x = 30.f, x2 = 19.f, x3 = 34.5f, y = 24.f;

        addParam(createParamCentered<RoundBlackKnob>(Vec(x, y * 2), module, GenEcho::SLEN_PARAM));
        addInput(createInputCentered<PJ301Mvar>(Vec(x, y * 3.2), module, GenEcho::SLEN_INPUT));

        addInput(createInputCentered<PJ301MPort>(Vec(15.f, y * 4.5), module, GenEcho::WAV0_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(45.f, y * 4.5), module, GenEcho::WAV1_INPUT));

        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(x2, y * 5.7), module, GenEcho::GAIN_PARAM));
        addInput(createInputCentered<PJ301Mvar>(Vec(x2, y * 6.9), module, GenEcho::GATE_INPUT));
        addInput(createInputCentered<PJ301Mvar>(Vec(x2, y * 8.2), module, GenEcho::DEL_INPUT));
        addInput(createInputCentered<PJ301Mvar>(Vec(x2, y * 9.5), module, GenEcho::OCT_INPUT));

        addParam(createParamCentered<WBsmall>(Vec(x3, y * 6.5f), module, GenEcho::TR_PARAM));
        addParam(createParamCentered<WBsmall>(Vec(x3, y * 7.8f), module, GenEcho::DL_PARAM));
        addParam(createParamCentered<WBsmallt>(Vec(x3, y * 9.1f), module, GenEcho::OC_PARAM));

        addParam(createParamCentered<RoundBlackKnob>(Vec(x, y * 12.4), module, GenEcho::CROSS_PARAM));
        addInput(createInputCentered<PJ301Mvar>(Vec(x, y * 13.6), module, GenEcho::CROSS_INPUT));

        addOutput(createOutputCentered<ThemedPJ301MPort>(Vec(15.f, y * 14.6), module, GenEcho::L_OUTPUT));
        addOutput(createOutputCentered<ThemedPJ301MPort>(Vec(45.f, y * 14.6), module, GenEcho::R_OUTPUT));
    }
};

Model *modelGenEcho = createModel<GenEcho, GenEchoWidget>("GenEcho");