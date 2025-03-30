#pragma once

using namespace rack;
extern Plugin *pluginInstance;

// A components header. Putting all ports/knobs/sliders/buttons/switches/color schemes into here and globally including these via plugin.hpp
////////////////////////////////////////////////////////////////////////////////////
// CONTENTS
// 1. PORTS | 2. KNOBS | 3. SWITCHES | 4. BUTTONS
// 5. LIGHTS & COLOR SCHEMES | 6. TEXT & CV DISPLAYS
//
////////////////////////////////////////////////////////////////////////////////////
// PORTS

struct PortDark : app::SvgPort
{
    PortDark()
    {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ports/PortDark.svg")));
        shadow->opacity = 0.f;
    }
};

////////////////////////////////////////////////////////////////////////////////////
// KNOBS

////////////////////////////////////////////////////////////////////////////////////
// SWITCHES

////////////////////////////////////////////////////////////////////////////////////
// BUTTONS
struct WhiteButton : app::SvgSwitch
{
    WhiteButton()
    {
        shadow->opacity = 0.0;
        momentary = true;
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/buttons/whitebutton0.svg")));
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/buttons/whitebutton1.svg")));
    }
};

////////////////////////////////////////////////////////////////////////////////////
// LIGHTS & COLOR SCHEMES
template <typename Base = GrayModuleLightWidget>
struct TWhiteBlueLight : Base
{
    TWhiteBlueLight()
    {
        this->addBaseColor(SCHEME_BLUE);
        this->addBaseColor(SCHEME_WHITE);
    }
};
typedef TWhiteBlueLight<> WhiteBlueLight;

// TEXT WIDGETS
///////////////////////////////////////////////////////////////////
// #include <sstream>
// #include <string>
// for text widgets its usually one of those two you need to include. in this tutorial we have already included them in the plugin.hpp
// Put this into the Widget section below your components to make use of the label:
//
//      CenteredLabel *const titleLabel = new CenteredLabel;
//      titleLabel->box.pos = Vec(11.5, 5);
//      titleLabel->text = "TEXT HERE";
//      addChild(titleLabel);
//
// I disabled the box.size because I use only 1-liners with this.

struct CenteredLabel : Widget
{
    std::string text;
    int fontSize;
    CenteredLabel(int _fontSize = 9)
    {
        fontSize = _fontSize;

        // box.size.y = 60.f;
        // use to define height of your box
    }
    void draw(const DrawArgs &args) override
    {
        nvgTextAlign(args.vg, NVG_ALIGN_CENTER);
        nvgFillColor(args.vg, nvgRGB(0xff, 0xff, 0xff));
        nvgFontSize(args.vg, fontSize);
        nvgText(args.vg, box.pos.x, box.pos.y, text.c_str(), NULL);
    }
};