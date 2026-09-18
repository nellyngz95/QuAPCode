/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ExplosionImpl.h"
#include "FireImpl.h"
#include "GunImpl.h"
#include "JetImpl.h"
#include "HelicopterImpl.h"
#include "RocketImpl.h"
//==============================================================================
//Bubble for the little assistant
struct SpeechBubbleLabel : public juce::Label
{
    void paint (juce::Graphics& g) override
    {
        
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 10.0f);

        
        g.setColour (juce::Colours::white.withAlpha (0.1f));
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced(0.5f), 10.0f, 1.0f);
        juce::Label::paint (g);
    }
};


class QAPAudioProcessorEditor  : public juce::AudioProcessorEditor,
public juce::ListBoxModel,public juce::FileDragAndDropTarget, public juce::ChangeListener, juce::Timer
{
public:
    QAPAudioProcessorEditor (QAPAudioProcessor&);
    ~QAPAudioProcessorEditor() override;

    //==============================================================================
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    void refreshWavFileList();
    void filterFileList(const juce::String& searchText);

    //Drag and Drop menu
    void SimilaritySearch(const juce::File& droppedFile);
    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;
    void fileDragEnter (const juce::StringArray& files, int x, int y) override;
    void fileDragExit (const juce::StringArray& files) override;
    
    // Procedural model dropdown
    juce::ComboBox proceduralModelMenu;
    void setupModelDropdown();
    //Procedural waveform
    juce::AudioVisualiserComponent proceduralVisualiser { 1 };
    juce::Rectangle<float> proceduralWaveformBounds;
    
    
    //Assistant
    void updateAssistant(const juce::String& searchText);
    void updateAssistantForParameter(const juce::String& paramID);
    void setAllModes(bool explosion, bool fire, bool gun, bool jet, bool heli, bool rocket);
    void paint (juce::Graphics&) override;
    void resized() override;
    void chooseLibraryFolder();
    void selectedRowsChanged(int lastRowSelected) override; //Check the changes
    //Waveform and cursors
    void loadThumbnail (const juce::File& file);
    bool keyPressed(const juce::KeyPress& key) override; //spacebar
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseUp (const juce::MouseEvent& event) override;
    void mouseDrag (const juce::MouseEvent& event) override;
    //Check the highlight of the sample we're listening to.
    void changeListenerCallback (juce::ChangeBroadcaster* source)override;
    void listBoxItemClicked (int row, const juce::MouseEvent& e) override;
    
    //Procedural UI
    void setupExplosionUI();
    void layoutExplosionUI();
    void setExplosionMode (bool shouldShow);
    void setupFireUI();
    void layoutFireUI();
    void setFireMode (bool shouldShow);
    void setupGunUI();
    void layoutGunUI();
    void setGunMode (bool shouldShow);
    
    void setupJetUI();
    void layoutJetUI();
    void setJetMode (bool shouldShow);
   
    void setupHelicopterUI();
    void layoutHelicopterUI();
    void setHelicopterMode (bool shouldShow);
    
    void setupRocketUI();
    void layoutRocketUI();
    void setRocketMode (bool shouldShow);
    
    void updateWindowSize();
    void timerCallback() override;
 

    

private:
    
    
    std::unique_ptr<juce::FileChooser> fileChooser;
    
    juce::TextButton loadLibraryButton {"Load Library"};
    juce::ListBox wavFileList;
    juce::TextEditor searchBar;
    juce::StringArray filteredWavFileNames;
    std::unique_ptr<juce::FileChooser> folderChooser;
    bool isDraggingFile = false;
    QAPAudioProcessor& audioProcessor;
    juce::AudioThumbnailCache thumbnailCache {10};
    juce::AudioThumbnail thumbnail;
    juce::String currentlyPlayingFileName;
    juce::Rectangle<float> waveformBounds;
    float selectionStart=-1.0f;
    float selectionEnd=-1.0f;
    double selectionEndTime = -1.0; 
    bool isPlayingSelection = false;
    juce::ProgressBar progressBar;
    
    //Buttons play pause and stop
    juce::TextButton playpauseButton,stopButton,loopButton;
    juce::TextButton recordButton{"Record Sound"};
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> loopAttachment;
     //Logo
    juce::Image Logo;
    //IREDOKI Assistant
    juce::Image assistantPixels;
    SpeechBubbleLabel assistantLabel;
    juce::TextButton assistantCloseButton { "x" };
    juce::ImageComponent assistantImage;
    //Sliders for mixing
    juce::Slider crossfadeSlider;
    juce::Slider reverbMixSlider;
    juce::Slider masterGainSlider;
    juce::Label  crossfadeLabel;
    juce::Label  reverbMixLabel;
    juce::Label  masterGainLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> crossfadeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reverbMixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterGainAttachment;
    juce::Rectangle<int> transportPanelBounds; 
    juce::Rectangle<int> mixPanelBounds;
    //Panels for the procedural Audio Models
    bool explosionMode=false;
    juce::GroupComponent explosionPanel { "explosionPanel", "Procedural Explosion" };
    juce::TextButton triggerButton;
    juce::Label RumbleLabel;
    juce::Label RumbleDecayLabel;
    juce::Label DustLabel;
    juce::Label DustDecayLabel;
    juce::Label AirLabel;
    juce::Label AirDecayLabel;
    juce::Label GritAmountLabel;
    juce::ComboBox presetMenuExplosion;
    
    bool FireMode=false;
    juce::GroupComponent firePanel { "firePanel", "Procedural Fire" };
    juce::TextButton FireButton;
    //Explosion
    juce::Slider rumbleSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> rumbleAttachment;
    juce::Slider rumbleDecaySlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> rumbleDecayAttachment;
    juce::Slider AirSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> airAttachment;
    juce::Slider AirDecaySlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> airDecayAttachment;
    juce::Slider DustSlider;
    juce::Slider DustDecaySlider;
    juce::Slider GritAmountSlider;
    juce::ComboBox presetMenuFire;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dustAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dustDecayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gritAmountAttachment;
    
    //Fire
    juce::Slider LappingSlider;
    juce::Slider HissingSlider;
    juce::Slider CracklingSlider;
    juce::Slider IntensitySlider;
    juce::Label LappingLabel;
    juce::Label HissingLabel;
    juce::Label IntensityLabel;
    juce::Label CracklingLabel;
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lappingAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> hissingAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> cracklingAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> intensityAttachment;
    
    //Gun
    bool GunMode=false;
    juce::GroupComponent GunPanel { "GunPanel", "Procedural Gun" };
    juce::TextButton GunButton;
    juce::ComboBox presetMenuGun;
    juce::Slider ShellFreqSlider;
    juce::Slider ShellFreqDecaySlider;
    juce::Label ShellFrequecyLabel;
    juce::Label ShellFrequencyDecayLabel;
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ShellFreqAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ShellFreqDecayAttachment;
    
    //Jet
    bool JetMode=false;
    juce::GroupComponent JetPanel { "JetPanel", "Procedural Jet" };
    juce::TextButton JetButton;
    juce::ComboBox presetMenuJet;
    juce::Slider SpeedSlider;
    juce::Slider TurbineSlider;
    juce::Slider BurnSlider;
    
    juce::Label SpeedLabel;
    juce::Label TurbineLabel;
    juce::Label BurnLabel;
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> SpeedAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> TurbineAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> BurnAttachment;
    
    //Helicopter
    bool HelicopterMode=false;
    juce::GroupComponent helicopterPanel { "HelicopterPanel", "Procedural Helicopter" };
    juce::TextButton HelicopterButton;
    juce::ComboBox presetMenuHelicopter;
    juce::Slider RotorPeriodSlider;
    juce::Slider PeriodSlider;
    juce::Slider TailMixSlider;
    juce::Slider BaseFreqSlider;
    juce::Slider RotorMixSlider;
    juce::Slider EngineMixSlider;
    juce::Slider BladeNoiseSlider;
    juce::Slider EngineSpeedSlider;
    
    juce::Label RotorPeriodLabel;
     juce::Label PeriodLabel;
     juce::Label TailMixLabel;
     juce::Label BaseFreqLabel;
     juce::Label RotorMixLabel;
     juce::Label EngineMixLabel;
     juce::Label BladeNoiseLabel;
     juce::Label EngineSpeedLabel;
    
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> RotorPeriodAttachment;
     std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> PeriodAttachment;
     std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> TailMixAttachment;
     std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> BaseFreqAttachment;
     std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> RotorMixAttachment;
     std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> EngineMixAttachment;
     std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> BladeNoiseAttachment;
     std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> EngineSpeedAttachment;
    
    //Rocket Mode
    bool RocketMode=false;
    juce::GroupComponent RocketPanel { "RocketPanel", "Procedural Rocket" };
    juce::TextButton RocketButton;
    juce::ComboBox presetMenuRocket;
    
    juce::Slider DurationSlider;
    juce::Slider ChamberResonanceSlider;
    juce::Slider FlutterSlider;
 
    juce::Label DurationLabel;
    juce::Label ChamberResonanceLabel;
    juce::Label FlutterLabel;
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> durationAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> chamberResonanceDecayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> flutterAttachment;
    
    //STRUCTS FOR LOOK AND FEEL

    struct ModernLookAndFeel : public juce::LookAndFeel_V4
    {
        ModernLookAndFeel()
        {
            // Global Palette
            setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff0a0d0f));
            setColour(juce::TextButton::buttonColourId,          juce::Colour(0xff2b2d42));
            setColour(juce::TextButton::buttonOnColourId,        juce::Colour(0xffef233c)); // Active Red
            setColour(juce::TextButton::textColourOffId,         juce::Colours::white.withAlpha(0.9f));
                    
            // Slider Texts
            setColour(juce::Slider::textBoxTextColourId, juce::Colours::white.withAlpha(0.9f));
            setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentWhite); // No ugly box border
            setColour(juce::Slider::textBoxHighlightColourId, juce::Colour(0xffef233c).withAlpha(0.4f));
            setColour(juce::Label::textColourId,              juce::Colours::white.withAlpha(0.9f));
            setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff1B2127));
            setColour(juce::ComboBox::textColourId,       juce::Colour(0xff5AC8AA));
            setColour(juce::ComboBox::outlineColourId,    juce::Colour(0xff5AC8AA).withAlpha(0.5f));
            setColour(juce::ComboBox::arrowColourId,      juce::Colour(0xff5AC8AA));
            setColour(juce::PopupMenu::backgroundColourId,        juce::Colour(0xff1B2127));
            setColour(juce::PopupMenu::textColourId,              juce::Colours::white.withAlpha(0.9f));
            setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff5AC8AA).withAlpha(0.4f));
            setColour(juce::PopupMenu::highlightedTextColourId,  juce::Colours::white);
            
        }

        // Button
        void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                   const juce::Colour& backgroundColour,
                                   bool shouldDrawButtonAsHighlighted,
                                   bool shouldDrawButtonAsDown) override
        {
            auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
            auto cornerSize = 4.0f;
            auto baseColour = backgroundColour;
            
            
            if (button.getToggleState())
                baseColour = button.findColour(juce::TextButton::buttonOnColourId);
            
            
            if (shouldDrawButtonAsHighlighted) baseColour = baseColour.brighter(0.1f);
            if (shouldDrawButtonAsDown)        baseColour = baseColour.darker(0.1f);

            juce::ColourGradient gradient(baseColour.brighter(0.1f), bounds.getTopLeft(),
                                          baseColour.darker(0.2f),   bounds.getBottomLeft(), false);
            g.setGradientFill(gradient);
            g.fillRoundedRectangle(bounds, cornerSize);

            g.setColour(juce::Colours::white.withAlpha(0.05f));
            g.fillRoundedRectangle(bounds.removeFromTop(bounds.getHeight() * 0.4f), cornerSize);


            
            g.setColour(baseColour.darker(0.8f));
            g.drawRoundedRectangle(button.getLocalBounds().toFloat().reduced(0.5f), cornerSize, 1.0f);
        }

        void drawButtonText (juce::Graphics& g, juce::TextButton& button,
                             bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
        {
            juce::Font font(14.0f, juce::Font::bold);
            g.setFont(font);

            // Calculate text colour (dim if disabled)
            g.setColour(button.findColour(button.getToggleState() ? juce::TextButton::textColourOnId
                                                                  : juce::TextButton::textColourOffId)
                                   .withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f));

            const int yIndent = juce::jmin(4, button.proportionOfHeight(0.3f));
            const int cornerSize = 6; // Match background corner size

            const int leftIndent = juce::jmin(font.getStringWidth("..."), button.proportionOfWidth(0.2f));
            const int rightIndent = leftIndent;

            g.drawFittedText(button.getButtonText(),
                             leftIndent, yIndent,
                             button.getWidth() - leftIndent - rightIndent,
                             button.getHeight() - yIndent - yIndent,
                             juce::Justification::centred, 2);
        }
        
        // Search Bar
        void drawTextEditorOutline (juce::Graphics& g, int width, int height,
                                    juce::TextEditor& textEditor) override
        {
            if (textEditor.hasKeyboardFocus(true))
            {
                g.setColour(textEditor.findColour(juce::TextEditor::focusedOutlineColourId));
                g.drawRoundedRectangle(0.5f, 0.5f, width - 1.0f, height - 1.0f, 4.0f, 2.0f);
            }
        }
        //Sliders
        void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                   const float rotaryStartAngle, const float rotaryEndAngle,
                                   juce::Slider& slider) override
            {
                auto radius = (float) juce::jmin (width / 2, height / 2) - 4.0f;
                auto centreX = (float) x + (float) width  * 0.5f;
                auto centreY = (float) y + (float) height * 0.5f;
                
                auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        
                juce::Path backgroundArc;
                backgroundArc.addCentredArc (centreX, centreY, radius, radius, 0.0f,
                                             rotaryStartAngle, rotaryEndAngle, true);

                g.setColour (juce::Colour(0xff2b2d42).brighter(0.1f)); // Dark Grey Track
                g.strokePath (backgroundArc, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));


                if (slider.isEnabled())
                {
                    juce::Path valueArc;
                    valueArc.addCentredArc (centreX, centreY, radius, radius, 0.0f,
                                            rotaryStartAngle, angle, true);

                    g.setColour (juce::Colour(0xff5AC8AA));
                    g.strokePath (valueArc, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                }

            
                auto knobRadius = radius * 0.75f;
                juce::ColourGradient knobGradient(juce::Colour(0xff2b2d42).brighter(0.2f), centreX, centreY - knobRadius,
                                                  juce::Colour(0xff141418), centreX, centreY + knobRadius, false);
                g.setGradientFill(knobGradient);
                g.fillEllipse (centreX - knobRadius, centreY - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f);
                
            
                g.setColour(juce::Colours::black.withAlpha(0.5f));
                g.drawEllipse(centreX - knobRadius, centreY - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f, 1.0f);

               
                float dotDist = knobRadius * 0.7f;
                float dotX = centreX + dotDist * std::sin(angle);
                float dotY = centreY - dotDist * std::cos(angle);
                float dotSize = 4.0f;
                
                g.setColour(juce::Colours::white.withAlpha(0.9f));
                g.fillEllipse(dotX - dotSize/2.0f, dotY - dotSize/2.0f, dotSize, dotSize);
            }
        void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                               float sliderPos, float minSliderPos, float maxSliderPos,
                               juce::Slider::SliderStyle style, juce::Slider& slider) override
        {
            // Track dimensions
            float trackThickness = 5.0f;
            float trackY = y + height * 0.5f - trackThickness * 0.5f;
            float trackLeft  = (float)x;
            float trackRight = (float)(x + width);

            // Background track — same dark grey as rotary
            g.setColour(juce::Colour(0xff2b2d42).brighter(0.1f));
            g.fillRoundedRectangle(trackLeft, trackY, trackRight - trackLeft, trackThickness, trackThickness * 0.5f);

            // Teal filled portion (left → thumb)
            if (slider.isEnabled())
            {
                g.setColour(juce::Colour(0xff5AC8AA));
                g.fillRoundedRectangle(trackLeft, trackY, sliderPos - trackLeft, trackThickness, trackThickness * 0.5f);
            }

            // Thumb — gradient circle matching the rotary knob
            float thumbSize = 18.0f;
            float thumbX = sliderPos - thumbSize * 0.5f;
            float thumbY = y + height * 0.5f - thumbSize * 0.5f;

            juce::ColourGradient thumbGradient(
                juce::Colour(0xff2b2d42).brighter(0.2f), sliderPos, thumbY,
                juce::Colour(0xff141418),                sliderPos, thumbY + thumbSize, false);
            g.setGradientFill(thumbGradient);
            g.fillEllipse(thumbX, thumbY, thumbSize, thumbSize);

            // Thumb border
            g.setColour(juce::Colours::black.withAlpha(0.5f));
            g.drawEllipse(thumbX, thumbY, thumbSize, thumbSize, 1.0f);

            // White dot in the centre of the thumb — same as rotary
            float dotSize = 4.0f;
            g.setColour(juce::Colours::white.withAlpha(0.9f));
            g.fillEllipse(sliderPos - dotSize * 0.5f,
                          y + height * 0.5f - dotSize * 0.5f,
                          dotSize, dotSize);
        }
        
    };
    
    ModernLookAndFeel modernLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (QAPAudioProcessorEditor)
};
