/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ExplosionImpl.h"
#include "FireImpl.h"
#include "faiss/Index.h"

//==============================================================================
QAPAudioProcessorEditor::QAPAudioProcessorEditor (QAPAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),thumbnail(512, audioProcessor.formatManager, thumbnailCache),progressBar (audioProcessor.indexingProgress)
{
    setLookAndFeel(&modernLookAndFeel);
    setResizable(true, true);
    addAndMakeVisible(wavFileList);
    wavFileList.setModel(this);

    addAndMakeVisible(loadLibraryButton);
    loadLibraryButton.setButtonText("Load Library");
    loadLibraryButton.onClick = [this] { chooseLibraryFolder(); };

    
    addAndMakeVisible (progressBar);
    progressBar.setPercentageDisplay (true);
    progressBar.setColour(juce::ProgressBar::backgroundColourId, juce::Colours::black.withAlpha(0.3f));
    progressBar.setColour(juce::ProgressBar::foregroundColourId, juce::Colours::aquamarine);
    progressBar.setTextToDisplay ("AI Scanning Library...");
    
    addAndMakeVisible(searchBar);
    searchBar.setTextToShowWhenEmpty("Search sounds...", juce::Colour(0xff5AC8AA).withAlpha(0.5f));
    searchBar.onTextChange = [this](){ startTimer(500); filterFileList(searchBar.getText());};

    filteredWavFileNames = audioProcessor.getWavFileNames();
    thumbnail.addChangeListener(this);
    setWantsKeyboardFocus(true);
    
    playpauseButton.setButtonText("Play");
    addAndMakeVisible(playpauseButton);
    playpauseButton.setClickingTogglesState(false);
    playpauseButton.onClick = [this]() {
            audioProcessor.startpause();
            const bool nowPlaying = audioProcessor.transportSource.isPlaying();
            playpauseButton.setButtonText(nowPlaying ? "Pause" : "Play");
            };

    addAndMakeVisible(stopButton);
    stopButton.setButtonText("Stop");

    stopButton.onClick = [this] { audioProcessor.stop();
        audioProcessor.transportSource.setPosition(0);
        playpauseButton.setButtonText("Play");};
    
    //Record Button
    addAndMakeVisible(recordButton);
    recordButton.setButtonText("REC");
    recordButton.setClickingTogglesState(true);
    recordButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff500000));
    recordButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff005000));
    recordButton.onClick = [this]
        {
            if (audioProcessor.getIsRecording())
            {
                audioProcessor.stopRecording();
                recordButton.setToggleState(false, juce::dontSendNotification);
            }
            else
            {
                audioProcessor.startRecording();
                recordButton.setToggleState(true, juce::dontSendNotification);
            }
        };
    addAndMakeVisible(loopButton);
    loopButton.setButtonText("Loop");
    loopButton.setClickingTogglesState(true);
    loopButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff500000));
    loopButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff005000));
    loopButton.onClick = [this]
        {
            bool shouldLoop = loopButton.getToggleState();
            if (auto* param = audioProcessor.parameters.getParameter("loop"))
                param->setValueNotifyingHost(shouldLoop ? 1.0f : 0.0f);

            if (audioProcessor.readerSource != nullptr)
                audioProcessor.readerSource->setLooping(shouldLoop);
        };

    
    //Images:
    
    Logo= juce::ImageCache::getFromMemory (BinaryData::QuAPLogo_png, BinaryData::QuAPLogo_pngSize);
    assistantPixels = juce::ImageCache::getFromMemory(BinaryData::QuAPAssistant_png, BinaryData::QuAPAssistant_pngSize);
    assistantImage.setImage(assistantPixels, juce::RectanglePlacement::centred);


    addAndMakeVisible(assistantImage);
    addAndMakeVisible(assistantLabel);
    assistantLabel.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    assistantLabel.setJustificationType(juce::Justification::centredLeft);
    assistantLabel.setBorderSize(juce::BorderSize<int>(0, 10, 0, 10)); // Top, Left, Bottom, Right
    
    assistantLabel.setInterceptsMouseClicks(false, false); //Ables clicks underneath the images

    assistantImage.setVisible(false);
    assistantLabel.setVisible(false);
    
    addAndMakeVisible(assistantCloseButton);
        assistantCloseButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffef233c));
        assistantCloseButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        assistantCloseButton.setVisible(false);
        assistantCloseButton.onClick = [this]()
        {
            assistantImage.setVisible(false);
            assistantLabel.setVisible(false);
            assistantCloseButton.setVisible(false);
        };
    // Sliders for mixing.
    auto setupMixSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& text)
        {
            s.setSliderStyle(juce::Slider::LinearHorizontal);
            s.setTextBoxStyle(juce::Slider::TextBoxRight, false, 45, 20);
            s.setLookAndFeel(&modernLookAndFeel);
            s.setRange(0.0, 1.0);
            addAndMakeVisible(s);
            l.setText(text, juce::dontSendNotification);
            l.setJustificationType(juce::Justification::centredLeft);
            l.setFont(juce::Font(12.0f, juce::Font::bold));
            addAndMakeVisible(l);
        };

        setupMixSlider(crossfadeSlider, crossfadeLabel, "Sample  <  >  Synth");
        crossfadeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "crossfade", crossfadeSlider);

        setupMixSlider(reverbMixSlider, reverbMixLabel, "Reverb");
        reverbMixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "reverbMix", reverbMixSlider);

        setupMixSlider(masterGainSlider, masterGainLabel, "Master");
        masterGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "masterGain", masterGainSlider);
   
    // Procedural Audio Models and menu.
    proceduralVisualiser.setBufferSize(512);
    proceduralVisualiser.setSamplesPerBlock(256);
    proceduralVisualiser.setColours(juce::Colour(0xff1B2127), juce::Colour(0xff5AC8AA));
    addAndMakeVisible(proceduralVisualiser);
    audioProcessor.proceduralVisualiser = &proceduralVisualiser;
    setupModelDropdown();
    
    setupExplosionUI();
    
    setupFireUI();
    
    setupGunUI();
    
    setupJetUI();
    
    setupHelicopterUI();
    
    setupRocketUI();
    
    recordButton.toFront(false);
    setResizeLimits(600, 400, 2000, 1300);
    setSize(700, 780); // Set the overall size of your plugin editor
}

QAPAudioProcessorEditor::~QAPAudioProcessorEditor()
{
    this->setLookAndFeel(nullptr);
    proceduralModelMenu.setLookAndFeel(nullptr);
    // --- Header & General ---
    wavFileList.setLookAndFeel(nullptr); //
    assistantImage.setLookAndFeel(nullptr); //
    loadLibraryButton.setLookAndFeel(nullptr);
    wavFileList.setLookAndFeel(nullptr);
    searchBar.setLookAndFeel(nullptr);
    recordButton.setLookAndFeel(nullptr);
    loopButton.setLookAndFeel(nullptr);
    playpauseButton.setLookAndFeel(nullptr);
    stopButton.setLookAndFeel(nullptr);    
    assistantLabel.setLookAndFeel(nullptr);
    assistantImage.setLookAndFeel(nullptr);
    assistantCloseButton.setLookAndFeel(nullptr);
    crossfadeSlider.setLookAndFeel(nullptr);
    reverbMixSlider.setLookAndFeel(nullptr);
    masterGainSlider.setLookAndFeel(nullptr);

    // --- Explosion ---
    explosionPanel.setLookAndFeel(nullptr);
    triggerButton.setLookAndFeel(nullptr);
    presetMenuExplosion.setLookAndFeel(nullptr);
    rumbleSlider.setLookAndFeel(nullptr);
    RumbleLabel.setLookAndFeel(nullptr);
    rumbleDecaySlider.setLookAndFeel(nullptr);
    RumbleDecayLabel.setLookAndFeel(nullptr);
    AirSlider.setLookAndFeel(nullptr);
    AirLabel.setLookAndFeel(nullptr);
    AirDecaySlider.setLookAndFeel(nullptr);
    AirDecayLabel.setLookAndFeel(nullptr);
    DustSlider.setLookAndFeel(nullptr);
    DustLabel.setLookAndFeel(nullptr);
    DustDecaySlider.setLookAndFeel(nullptr);
    DustDecayLabel.setLookAndFeel(nullptr);
    GritAmountSlider.setLookAndFeel(nullptr);
    GritAmountLabel.setLookAndFeel(nullptr);

    // --- Fire ---
    firePanel.setLookAndFeel(nullptr);
    FireButton.setLookAndFeel(nullptr);
    presetMenuFire.setLookAndFeel(nullptr);
    HissingSlider.setLookAndFeel(nullptr);
    HissingLabel.setLookAndFeel(nullptr);
    CracklingSlider.setLookAndFeel(nullptr);
    CracklingLabel.setLookAndFeel(nullptr);
    IntensitySlider.setLookAndFeel(nullptr);
    IntensityLabel.setLookAndFeel(nullptr);
    LappingSlider.setLookAndFeel(nullptr);
    LappingLabel.setLookAndFeel(nullptr);

    // --- Rocket ---
    RocketPanel.setLookAndFeel(nullptr);
    RocketButton.setLookAndFeel(nullptr);
    presetMenuRocket.setLookAndFeel(nullptr);
    DurationSlider.setLookAndFeel(nullptr);
    DurationLabel.setLookAndFeel(nullptr);
    ChamberResonanceSlider.setLookAndFeel(nullptr);
    ChamberResonanceLabel.setLookAndFeel(nullptr);
    FlutterSlider.setLookAndFeel(nullptr);
    FlutterLabel.setLookAndFeel(nullptr);

    // --- Helicopter ---
    helicopterPanel.setLookAndFeel(nullptr);
    HelicopterButton.setLookAndFeel(nullptr);
    presetMenuHelicopter.setLookAndFeel(nullptr);
    RotorPeriodSlider.setLookAndFeel(nullptr);
    RotorPeriodLabel.setLookAndFeel(nullptr);
    PeriodSlider.setLookAndFeel(nullptr);
    PeriodLabel.setLookAndFeel(nullptr);
    TailMixSlider.setLookAndFeel(nullptr);
    TailMixLabel.setLookAndFeel(nullptr);
    BaseFreqSlider.setLookAndFeel(nullptr);
    BaseFreqLabel.setLookAndFeel(nullptr);
    RotorMixSlider.setLookAndFeel(nullptr);
    RotorMixLabel.setLookAndFeel(nullptr);
    EngineMixSlider.setLookAndFeel(nullptr);
    EngineMixLabel.setLookAndFeel(nullptr);
    BladeNoiseSlider.setLookAndFeel(nullptr);
    BladeNoiseLabel.setLookAndFeel(nullptr);
    EngineSpeedSlider.setLookAndFeel(nullptr);
    EngineSpeedLabel.setLookAndFeel(nullptr);
    
    // --- Jet ---
    JetPanel.setLookAndFeel(nullptr);
    presetMenuJet.setLookAndFeel(nullptr);
    JetButton.setLookAndFeel(nullptr);
    SpeedSlider.setLookAndFeel(nullptr);
    SpeedLabel.setLookAndFeel(nullptr);
    TurbineSlider.setLookAndFeel(nullptr);
    TurbineLabel.setLookAndFeel(nullptr);
    BurnSlider.setLookAndFeel(nullptr);
    BurnLabel.setLookAndFeel(nullptr);
    
    // --- Gun ---
    GunPanel.setLookAndFeel(nullptr);
    GunButton.setLookAndFeel(nullptr);
    presetMenuGun.setLookAndFeel(nullptr);
    ShellFreqSlider.setLookAndFeel(nullptr);
    ShellFrequecyLabel.setLookAndFeel(nullptr);
    ShellFreqDecaySlider.setLookAndFeel(nullptr);
    ShellFrequencyDecayLabel.setLookAndFeel(nullptr);
}

//DRAG AND DROP FUNCTIONS
bool QAPAudioProcessorEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (auto file : files)
        if (file.endsWith(".wav") || file.endsWith(".aif") || file.endsWith(".mp3"))
            return true;
            
    return false;
}
void QAPAudioProcessorEditor::fileDragEnter (const juce::StringArray& files, int x, int y)
{
    isDraggingFile = true;
    repaint();
}
void QAPAudioProcessorEditor::fileDragExit (const juce::StringArray& files)
{
    isDraggingFile = false;
    repaint();
}

void QAPAudioProcessorEditor::filesDropped (const juce::StringArray& files, int x, int y)
{
    isDraggingFile = false;
    juce::File droppedFile (files[0]);

    if (droppedFile.existsAsFile())
        {
            //audioProcessor.PlayAssistantSound.store(true);
            thumbnail.setSource (new juce::FileInputSource (droppedFile));
            
            juce::Thread::launch([this, droppedFile]()
                    {
                        audioProcessor.LoadDroppedFile(droppedFile);
                        SimilaritySearch(droppedFile);
                    });
        }
    repaint();
}

void QAPAudioProcessorEditor::SimilaritySearch(const juce::File& droppedFile)
{
    audioProcessor.performSimilaritySearch(droppedFile);
    auto results = audioProcessor.getSimilarFiles();
    
    juce::StringArray newNames;
    for (auto& f : results)
        newNames.add(f.getFileName());

    // 1. Capture the dropped filename here to use inside the lambda
    juce::String droppedName = droppedFile.getFileName().toLowerCase();

    juce::MessageManager::callAsync([this, newNames, droppedName]()
    {
        filteredWavFileNames = newNames;
        bool categoryFound = false;

        juce::String n = droppedName;
        
        // --- START OF YOUR LOGIC BLOCK ---
        // EXPLOSIONS
        if (n.contains("explosion") || n.contains("blast") || n.contains("boom") ||
            n.contains("detonation") || n.contains("kaboom") || n.contains("impact") ||
            n.contains("thud") || n.contains("grenade") || n.contains("bomb") || n.contains("debris"))
        {
            setExplosionMode(true); categoryFound = true;
        }
        // FIRE
        else if (n.contains("fire") || n.contains("burn") || n.contains("crackle") ||
                 n.contains("flame") || n.contains("inferno") || n.contains("embers") ||
                 n.contains("torch") || n.contains("ignition") || n.contains("sizzle"))
        {
            setFireMode(true); categoryFound = true;
        }
        // GUNS
        else if (n.contains("gun") || n.contains("shot") || n.contains("pistol") ||
                 n.contains("rifle") || n.contains("weapon") || n.contains("bullet") ||
                 n.contains("trigger") || n.contains("reload") || n.contains("firearm"))
        {
            setGunMode(true); categoryFound = true;
        }
        // JETS
        else if (n.contains("jet") || n.contains("engine") || n.contains("turbine") ||
                 n.contains("flyover") || n.contains("sonic") || n.contains("afterburner") ||
                 n.contains("aircraft"))
        {
            setJetMode(true); categoryFound = true;
        }
        // HELICOPTERS
        else if (n.contains("helicopter") || n.contains("chopper") || n.contains("rotor") ||
                 n.contains("heli") || n.contains("propeller") || n.contains("blade") ||
                 n.contains("apache"))
        {
            setHelicopterMode(true); categoryFound = true;
        }
        // ROCKETS
        else if (n.contains("rocket") || n.contains("launch") || n.contains("missile") ||
                 n.contains("shuttle") || n.contains("booster") || n.contains("liftoff"))
        {
            setRocketMode(true); categoryFound = true;
        }

        // 3. SECONDARY: If the dropped filename didn't match, check the similarity results
        if (!categoryFound)
        {
            for (const auto& name : filteredWavFileNames)
            {
                if (!categoryFound)
                {
                    juce::String libraryName = name.toLowerCase();
                    // (You could repeat the if/else logic here, but checking the
                    // dropped file name is usually enough for external files)
                }
            }
        }

        wavFileList.updateContent();
        wavFileList.repaint();

        if (categoryFound)
        {
            // Trigger the assistant using the dropped file name
            updateAssistant(droppedName);
        }
        
        DBG("UI Updated: Checked external file name: " << droppedName);
    });
}

void QAPAudioProcessorEditor::timerCallback()
{
    stopTimer();
    filterFileList(searchBar.getText());
    progressBar.setVisible (audioProcessor.indexingProgress >= 0.0);
    if (isPlayingSelection && selectionEndTime > 0.0)
        {
            double currentPos = audioProcessor.transportSource.getCurrentPosition();
            if (currentPos >= selectionEndTime)
            {
                audioProcessor.transportSource.stop();
                audioProcessor.transportSource.setPosition(
                    juce::jmin(selectionStart, selectionEnd) * thumbnail.getTotalLength()
                ); // rewind to selection start, ready to play again
                isPlayingSelection = false;
                stopTimer();
            }
            return; // don't stop the timer early while monitoring
        }
    stopTimer();
}

//==============================================================================

void QAPAudioProcessorEditor::changeListenerCallback (juce::ChangeBroadcaster* source)
{
    if (source == &thumbnail)
        repaint(); // Esto redibuja la forma de onda cuando el hilo de fondo tiene los picos listos
}

void QAPAudioProcessorEditor::paint (juce::Graphics& g)
{
    auto area = getLocalBounds();
    
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    
    int headerHeight = 100;
    auto headerArea = area.removeFromTop(headerHeight);
    
    // Header Background (Darker + Bottom Border)
    g.setColour(juce::Colour(0xff1a1e22)); // Very dark header
    g.fillRect(0, 108, getWidth(), 2);;
    
    // Header Bottom Border (Accent Line)
    g.setColour(juce::Colour(0xff5AC8AA).withAlpha(0.3f)); // Glow
    g.fillRect(0, headerHeight + 2, getWidth(), 5);
    g.setColour(juce::Colour(0xff5AC8AA)); // Solid Teal Line
    g.fillRect(0, headerHeight + 3, getWidth(), 2);
    if (!transportPanelBounds.isEmpty())
        {
            g.setColour(juce::Colour(0xff1B2127));
            g.fillRoundedRectangle(transportPanelBounds.toFloat(), 8.0f);
            g.setColour(juce::Colour(0xff5AC8AA).withAlpha(0.3f));
            g.drawRoundedRectangle(transportPanelBounds.toFloat(), 8.0f, 1.0f);
        }
    if (!mixPanelBounds.isEmpty())
       {
           g.setColour(juce::Colour(0xff1B2127));
           g.fillRoundedRectangle(mixPanelBounds.toFloat(), 8.0f);
           g.setColour(juce::Colour(0xff5AC8AA).withAlpha(0.3f));
           g.drawRoundedRectangle(mixPanelBounds.toFloat(), 8.0f, 1.0f);
       }
    // Header Title / Logo
    juce::Font titleFont (juce::FontOptions()
                                .withName(juce::Font::getDefaultSansSerifFontName())
                                .withHeight(24.0f)
                                .withStyle("Bold"));
        titleFont.setExtraKerningFactor(0.15f); // letter spacing = modern/techy
        g.setFont(titleFont);

        // Subtle teal glow behind text
        g.setColour(juce::Colour(0xff5AC8AA).withAlpha(0.25f));
        g.drawText("Quality Audio Prototyping", 1, 1, getWidth(), 50, juce::Justification::centred);
        // Main text
        g.setColour(juce::Colours::white.withAlpha(0.95f));
 
    g.drawText("Quality Audio Prototyping", 0, 8, getWidth(), 44, juce::Justification::centred);

    if (Logo.isValid())
    {
        g.drawImageWithin (Logo,
                           20, 10,        // X and Y position
                           120, 100,       // Maximum Width and Height
                           juce::RectanglePlacement::centred);
    }
    //Search or Drag
    juce::Rectangle<int> instructionArea (20, 100, 300, 200);
    auto searchBounds = searchBar.getBounds().toFloat();
    g.setColour(juce::Colour(0xff5AC8AA));
    g.fillRoundedRectangle(searchBounds, 4.0f);
    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.drawHorizontalLine(searchBounds.getY(), searchBounds.getX(), searchBounds.getRight());
    g.drawVerticalLine(searchBounds.getX(), searchBounds.getY(), searchBounds.getBottom());
    
    
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.drawHorizontalLine(searchBounds.getBottom(), searchBounds.getX(), searchBounds.getRight());
    g.drawVerticalLine(searchBounds.getRight(), searchBounds.getY(), searchBounds.getBottom());
    auto listBounds = wavFileList.getBounds().toFloat();
    
    juce::ColourGradient panelGrad(juce::Colour(40, 42, 45), listBounds.getTopLeft(),
                                   juce::Colour(30, 32, 35), listBounds.getBottomLeft(), false);
    g.setGradientFill(panelGrad);
    g.fillRoundedRectangle(listBounds, 6.0f);
    
    
    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.drawRoundedRectangle(listBounds.translated(0, 2), 6.0f, 2.0f);
    
    g.setColour(juce::Colours::white.withAlpha(0.15f));
    g.drawRoundedRectangle(listBounds, 6.0f, 1.0f);
    
    if (thumbnail.getTotalLength() > 0.0 && !waveformBounds.isEmpty())
    {
        // 1. Fondo oscuro del contenedor (Tu color original)
        g.setColour(juce::Colour(0xff1B2127));
        g.fillRoundedRectangle(waveformBounds, 3.0f);
        
        auto thumbArea = waveformBounds.reduced(2.0f); // Margen interno
        g.setColour(juce::Colour(0xff5AC8AA));
        thumbnail.drawChannels (g,
                                thumbArea.getSmallestIntegerContainer(),
                                0.0,                          // Tiempo de inicio
                                thumbnail.getTotalLength(),   // Tiempo final
                                1.0f);                        // Zoom vertical (1.0 = normal)
        if (selectionStart >= 0.0f)
        {
            auto x1 = waveformBounds.getX() + (selectionStart * waveformBounds.getWidth());
            auto x2 = waveformBounds.getX() + (selectionEnd * waveformBounds.getWidth());

          
            auto startX = std::min(x1, x2);
            auto width = std::abs(x2 - x1);

            auto selectRect = juce::Rectangle<float>(startX, waveformBounds.getY(), width, waveformBounds.getHeight());

            g.setColour (juce::Colours::white.withAlpha (0.2f));
            g.fillRect (selectRect);

            g.setColour (juce::Colours::white.withAlpha (0.5f));
            g.drawVerticalLine (juce::roundToInt(x1), waveformBounds.getY(), waveformBounds.getBottom());
            g.drawVerticalLine (juce::roundToInt(x2), waveformBounds.getY(), waveformBounds.getBottom());
        }    }
    else
    {
        // Opcional: Mostrar un mensaje si no hay nada cargado
        g.setColour(juce::Colours::grey.withAlpha(0.3f));
        g.drawText("No waveform loaded.", waveformBounds, juce::Justification::centred);
    }
        
 
    
    if (searchBar.getText().isEmpty() && thumbnail.getTotalLength() <= 0.0)
    {
        if (!waveformBounds.isEmpty())
        {
            
            g.setColour(juce::Colour(0xff5AC8AA).withAlpha(0.3f));
            float dashLengths[] = { 5.0f, 5.0f };
            
            juce::Path dashedBorder;
            dashedBorder.addRoundedRectangle(waveformBounds.reduced(2.0f), 3.0f);
            juce::PathStrokeType stroke (1.0f, juce::PathStrokeType::curved, juce::PathStrokeType::butt);

            stroke.createDashedStroke (dashedBorder, dashedBorder, dashLengths, 2);

            g.strokePath (dashedBorder, stroke);
            
            g.setColour(juce::Colours::white.withAlpha(0.6f));
            g.setFont(juce::Font(16.0f, juce::Font::italic));
            g.drawFittedText("Type in search or Drag a sound here\nto find something similar",
                             waveformBounds.toNearestInt(),
                             juce::Justification::centred,
                             2);
        }
        
    }
}
void QAPAudioProcessorEditor::setupModelDropdown()
{
    addAndMakeVisible(proceduralModelMenu);
    proceduralModelMenu.setLookAndFeel(&modernLookAndFeel);
    proceduralModelMenu.setTextWhenNothingSelected("Procedural Model");
    proceduralModelMenu.setJustificationType(juce::Justification::centred);

    proceduralModelMenu.addItem("None", 1);
    proceduralModelMenu.addItem("Explosion", 2);
    proceduralModelMenu.addItem("Fire", 3);
    proceduralModelMenu.addItem("Gun", 4);
    proceduralModelMenu.addItem("Jet", 5);
    proceduralModelMenu.addItem("Helicopter", 6);
    proceduralModelMenu.addItem("Rocket", 7);

    proceduralModelMenu.onChange = [this]()
        {
            int id = proceduralModelMenu.getSelectedId();
            switch (id)
            {
                case 1:
                    setAllModes(false, false, false, false, false, false);
                    assistantImage.setVisible(false);
                    assistantLabel.setVisible(false);
                    break;
                case 2:
                    setAllModes(true, false, false, false, false, false);
                    assistantLabel.setText("Hi! Are you ready to create your own explosion?", juce::dontSendNotification);
                    assistantImage.setVisible(true);
                    assistantLabel.setVisible(true);
                    audioProcessor.PlayAssistantSound = true;
                    break;
                case 3:
                    setAllModes(false, true, false, false, false, false);
                    assistantLabel.setText("Hello! Ready to create your own fire?", juce::dontSendNotification);
                    assistantImage.setVisible(true);
                    assistantLabel.setVisible(true);
                    audioProcessor.PlayAssistantSound = true;
                    break;
                case 4:
                    setAllModes(false, false, true, false, false, false);
                    assistantLabel.setText("Whoa! Are we gonna create some gunshots?", juce::dontSendNotification);
                    assistantImage.setVisible(true);
                    assistantLabel.setVisible(true);
                    audioProcessor.PlayAssistantSound = true;
                    break;
                case 5:
                    setAllModes(false, false, false, true, false, false);
                    assistantLabel.setText("Wohoo! We are about to create Jet sounds!", juce::dontSendNotification);
                    assistantImage.setVisible(true);
                    assistantLabel.setVisible(true);
                    audioProcessor.PlayAssistantSound = true;
                    break;
                case 6:
                    setAllModes(false, false, false, false, true, false);
                    assistantLabel.setText("Are we gonna create a helicopter?", juce::dontSendNotification);
                    assistantImage.setVisible(true);
                    assistantLabel.setVisible(true);
                    audioProcessor.PlayAssistantSound = true;
                    break;
                case 7:
                    setAllModes(false, false, false, false, false, true);
                    assistantLabel.setText("Hello! Are you ready to create rockets?", juce::dontSendNotification);
                    assistantImage.setVisible(true);
                    assistantLabel.setVisible(true);
                    audioProcessor.PlayAssistantSound = true;
                    break;
            }
            resized(); // reposition the assistant now that it's visible
        };
}
void QAPAudioProcessorEditor::setupRocketUI()
{
    addAndMakeVisible(RocketPanel);
    RocketPanel.setVisible(false);
    
    RocketButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkred);
    RocketButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::white);
    addAndMakeVisible(RocketButton);
    RocketButton.setButtonText("Start Rocket");
    RocketButton.onClick = [this]() { audioProcessor.triggerRocket(); };
    
    addAndMakeVisible(presetMenuRocket);
        presetMenuRocket.setJustificationType(juce::Justification::centred);
    presetMenuRocket.setLookAndFeel(&modernLookAndFeel);
        presetMenuRocket.setTextWhenNothingSelected("Select Preset");
        int id = 1;
        for (const auto& p : audioProcessor.presetsrocket)
            {
                presetMenuRocket.addItem(p.name, id++);
            }

            presetMenuRocket.onChange = [this] {
            
                audioProcessor.loadPresetRocket(presetMenuRocket.getSelectedId() - 1);
            };
    auto setupSliderAndLabel = [&](juce::Slider& slider, const char* paramID, juce::Label& label, const char* labelText)
    {
      
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
        
    
        slider.setColour(juce::Slider::thumbColourId, juce::Colours::orange);
        slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::darkgrey);
        slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::red.withAlpha(0.6f));
        addAndMakeVisible(slider);
        

        label.setText(labelText, juce::dontSendNotification);
        label.setFont(juce::Font(12.0f, juce::Font::bold));
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(label);
        
        
        slider.setVisible(false);
        label.setVisible(false);
    };
    
   
    
    setupSliderAndLabel(DurationSlider, "duration", DurationLabel, "Duration");
    durationAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "duration", DurationSlider);
    DurationSlider.onDragStart = [this]() { updateAssistantForParameter("duration"); };
                    
    
    setupSliderAndLabel(ChamberResonanceSlider, "chamberresonance", ChamberResonanceLabel, "Chamber Resonance");
    chamberResonanceDecayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "chamberresonance", ChamberResonanceSlider);
    ChamberResonanceSlider.onDragStart = [this]() { updateAssistantForParameter("chamberresonance"); };
 
    setupSliderAndLabel(FlutterSlider, "flutter", FlutterLabel, "Flutter");
    flutterAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "flutter", FlutterSlider);
    FlutterSlider.onDragStart = [this]() { updateAssistantForParameter("flutter"); };
    
    setSize (400, 300);
}

void QAPAudioProcessorEditor::setupHelicopterUI()
{
    addAndMakeVisible(helicopterPanel);
    helicopterPanel.setVisible(false);
    HelicopterButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkred);
    HelicopterButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::white);
    addAndMakeVisible(HelicopterButton);
    HelicopterButton.setClickingTogglesState(true);
    HelicopterButton.setButtonText("Start Helicopter");
    HelicopterButton.onClick = [this]() { if (HelicopterButton.getToggleState())
    {
        HelicopterButton.setButtonText("Stop Helicopter");
    }
    else
    {
        HelicopterButton.setButtonText("Start Helicopter");
    }audioProcessor.triggerHelicopter(); };
    
    addAndMakeVisible(presetMenuHelicopter);
        presetMenuHelicopter.setJustificationType(juce::Justification::centred);
    presetMenuHelicopter.setLookAndFeel(&modernLookAndFeel);
        presetMenuHelicopter.setTextWhenNothingSelected("Select Preset");    int id = 1;
        for (const auto& p : audioProcessor.presetshelicopter)
        {
            presetMenuHelicopter.addItem(p.name, id++);
        }

        presetMenuHelicopter.onChange = [this] {
            audioProcessor.loadPresetHelicopter(presetMenuHelicopter.getSelectedId() - 1);
        };

    
    auto setupSliderAndLabel = [&](juce::Slider& slider, const char* paramID, juce::Label& label, const char* labelText)
    {
      
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
        
    
        slider.setColour(juce::Slider::thumbColourId, juce::Colours::orange);
        slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::darkgrey);
        slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::red.withAlpha(0.6f));
        addAndMakeVisible(slider);
        

        label.setText(labelText, juce::dontSendNotification);
        label.setFont(juce::Font(12.0f, juce::Font::bold));
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(label);
        
        
        slider.setVisible(false);
        label.setVisible(false);
    };
    
   
    
    setupSliderAndLabel(RotorPeriodSlider, "rotorPeriod", RotorPeriodLabel, "RotorPeriod");
    RotorPeriodAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "rotorPeriod", RotorPeriodSlider);
    RotorPeriodSlider.onDragStart = [this]() { updateAssistantForParameter("rotorPeriod"); };

    setupSliderAndLabel(PeriodSlider, "period",PeriodLabel, "Period");
    PeriodAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "period", PeriodSlider);
   PeriodSlider.onDragStart = [this]() { updateAssistantForParameter("period"); };
    
    setupSliderAndLabel(TailMixSlider, "tailMix",TailMixLabel, "Tail Mix");
    TailMixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "tailMix", TailMixSlider);
    TailMixSlider.onDragStart = [this]() { updateAssistantForParameter("tailMix"); };
    
    setupSliderAndLabel(BaseFreqSlider, "baseFreq",BaseFreqLabel, "Base Frequency");
    BaseFreqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "baseFreq", BaseFreqSlider);
   BaseFreqSlider.onDragStart = [this]() { updateAssistantForParameter("baseFreq"); };
    
    setupSliderAndLabel(RotorMixSlider, "rotorMix",RotorMixLabel, "Rotor Mix");
    RotorMixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "rotorMix", RotorMixSlider);
    RotorMixSlider.onDragStart = [this]() { updateAssistantForParameter("rotorMix"); };
    
    
    setupSliderAndLabel(EngineMixSlider, "engineMix",EngineMixLabel, "Engine Mix ");
    EngineMixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "engineMix", EngineMixSlider);
    EngineMixSlider.onDragStart = [this]() { updateAssistantForParameter("engineMix"); };
    
    setupSliderAndLabel(BladeNoiseSlider, "bladeNoise",BladeNoiseLabel, "Blade Noise");
    BladeNoiseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "bladeNoise", BladeNoiseSlider);
    BladeNoiseSlider.onDragStart = [this]() { updateAssistantForParameter("bladeNoise"); };
    
    setupSliderAndLabel(EngineSpeedSlider, "engineSpeed",EngineSpeedLabel, "Engine Speed");
    EngineSpeedAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "engineSpeed", EngineSpeedSlider);
    EngineSpeedSlider.onDragStart = [this]() { updateAssistantForParameter("engineSpeed"); };

    
    setSize (400, 300);
    
}


void QAPAudioProcessorEditor::setupJetUI()
{
    addAndMakeVisible(JetPanel);
    JetPanel.setVisible(false);
    JetPanel.setVisible(false);
    addAndMakeVisible(JetButton);
    JetButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkred);
    JetButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::white);
    JetButton.setButtonText("Start Jet");
    JetButton.setClickingTogglesState(true);
    JetButton.onClick = [this]() {if (JetButton.getToggleState())
    {
        JetButton.setButtonText("Stop Jet");
    }
    else
    {
        JetButton.setButtonText("Start Jet");
    } audioProcessor.triggerJet(); };
    
    addAndMakeVisible(presetMenuJet);
    presetMenuJet.setJustificationType(juce::Justification::centred);
    presetMenuJet.setLookAndFeel(&modernLookAndFeel);
       presetMenuJet.setTextWhenNothingSelected("Select Preset");
       int id = 1;
       for (const auto& p : audioProcessor.presetsjet)
       {
           presetMenuJet.addItem(p.name, id++);
       }
        presetMenuJet.onChange = [this] {
        audioProcessor.loadPresetJet(presetMenuJet.getSelectedId() - 1);
    };
    
    auto setupSliderAndLabel = [&](juce::Slider& slider, const char* paramID, juce::Label& label, const char* labelText)
    {
      
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    
        slider.setColour(juce::Slider::thumbColourId, juce::Colours::orange);
        slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::darkgrey);
        slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::red.withAlpha(0.6f));
        addAndMakeVisible(slider);
        

        label.setText(labelText, juce::dontSendNotification);
        label.setFont(juce::Font(12.0f, juce::Font::bold));
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(label);
        
        
        slider.setVisible(false);
        label.setVisible(false);
    };
    
    setupSliderAndLabel(SpeedSlider, "speed", SpeedLabel, "Speed");
    SpeedAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "speed", SpeedSlider);
   SpeedSlider.onDragStart = [this]() { updateAssistantForParameter("speed"); };
    

    setupSliderAndLabel(TurbineSlider, "turbine", TurbineLabel, "Turbine");
    TurbineAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "turbine", TurbineSlider);
    TurbineSlider.onDragStart = [this]() { updateAssistantForParameter("turbine"); };
    
    setupSliderAndLabel(BurnSlider, "burn", BurnLabel, "Burn");
    BurnAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "burn", BurnSlider);
    BurnSlider.onDragStart = [this]() { updateAssistantForParameter("burn"); };
}

void QAPAudioProcessorEditor::setupGunUI()
{
    addAndMakeVisible(GunPanel);
    GunPanel.setVisible(false);
   
    addAndMakeVisible(GunButton);
    GunButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkred);
    GunButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::white);
    GunButton.setButtonText("Shot Gun");
    GunButton.onClick = [this]() { audioProcessor.triggerGun(); };
    addAndMakeVisible(presetMenuGun);
    presetMenuGun.setJustificationType(juce::Justification::centred);
    presetMenuGun.setLookAndFeel(&modernLookAndFeel);
    presetMenuGun.setTextWhenNothingSelected("Select Preset");
    int id = 1;
    
    
           for (const auto& p : audioProcessor.presetsgun)
           {
               presetMenuGun.addItem(p.name, id++);
           }

           presetMenuGun.onChange = [this] {
           
               audioProcessor.loadPresetGun(presetMenuGun.getSelectedId() - 1);
           };
    
    
    auto setupSliderAndLabel = [&](juce::Slider& slider, const char* paramID, juce::Label& label, const char* labelText)
    {
      
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
        
    
        slider.setColour(juce::Slider::thumbColourId, juce::Colours::orange);
        slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::darkgrey);
        slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::red.withAlpha(0.6f));
        addAndMakeVisible(slider);
        

        label.setText(labelText, juce::dontSendNotification);
        label.setFont(juce::Font(15.0f, juce::Font::bold));
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(label);
        
        
        slider.setVisible(false);
        label.setVisible(false);
    };
    
    setupSliderAndLabel(ShellFreqSlider, "shellfreq", ShellFrequecyLabel, "Shell Frequency");
    ShellFreqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "shellfreq", ShellFreqSlider);
   ShellFreqSlider.onDragStart = [this]() { updateAssistantForParameter("shellfreq"); };
    
    setupSliderAndLabel(ShellFreqDecaySlider, "shellfreqdecay", ShellFrequencyDecayLabel, "Shell Frequency Decay");
    ShellFreqDecayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "shellfreqdecay", ShellFreqDecaySlider);
    ShellFreqDecaySlider.onDragStart = [this]() { updateAssistantForParameter("shellfreqdecay"); };
    
}

void QAPAudioProcessorEditor::setupFireUI()
{
    addAndMakeVisible(firePanel);
    firePanel.setVisible(false);

    addAndMakeVisible(FireButton);
    FireButton.setButtonText("Start Fire");
    FireButton.setClickingTogglesState(true);
    FireButton.onClick = [this]() {  if (FireButton.getToggleState())
    {
        FireButton.setButtonText("Stop Fire");
    }
    else
    {
        FireButton.setButtonText("Start Fire");
    }audioProcessor.triggerFire(); };
    
    addAndMakeVisible(presetMenuFire);
        presetMenuFire.setJustificationType(juce::Justification::centred);
        presetMenuFire.setLookAndFeel(&modernLookAndFeel);
        presetMenuFire.setTextWhenNothingSelected("Select Preset");
        int id = 1;
        for (const auto& p : audioProcessor.presetsfire)
        {
            presetMenuFire.addItem(p.name, id++);
        }

        presetMenuFire.onChange = [this] {
        
            audioProcessor.loadPresetFire(presetMenuFire.getSelectedId() - 1);
        };
    
    auto setupSliderAndLabel = [&](juce::Slider& slider, const char* paramID, juce::Label& label, const char* labelText)
    {
        
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20); // Slightly wider text box
        
        addAndMakeVisible(slider);
        label.setText(labelText, juce::dontSendNotification);
        label.setFont(juce::Font(14.0f)); // Non-bold looks cleaner with this style
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.8f));
        addAndMakeVisible(label);
        
        slider.setVisible(false);
        label.setVisible(false);
    };

    setupSliderAndLabel(LappingSlider, "lapping", LappingLabel, "Lapping");
    lappingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "lapping", LappingSlider);
    LappingSlider.onDragStart = [this]() { updateAssistantForParameter("lapping"); };
    
    setupSliderAndLabel(HissingSlider, "hissing", HissingLabel, "Hissing");
    hissingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "hissing", HissingSlider);
    HissingSlider.onDragStart = [this]() { updateAssistantForParameter("hissing"); };
    setupSliderAndLabel(CracklingSlider, "crackling", CracklingLabel, "Crackling");
    cracklingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "crackling", CracklingSlider);
    CracklingSlider.onDragStart = [this]() { updateAssistantForParameter("crackling"); };

    setupSliderAndLabel(IntensitySlider, "intensity", IntensityLabel, "Intensity");
    intensityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "intensity", IntensitySlider);
    IntensitySlider.onDragStart = [this]() { updateAssistantForParameter("intensity"); };
}

void QAPAudioProcessorEditor::setupExplosionUI()
{
    // === Explosion Panel ===
    addAndMakeVisible(explosionPanel);
    explosionPanel.setVisible(false);


    addAndMakeVisible(triggerButton);
    triggerButton.setButtonText("BOOM!");
    triggerButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkred);
    triggerButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::white);
    triggerButton.onClick = [this]() { audioProcessor.triggerExplosion(); };
    
    addAndMakeVisible(presetMenuExplosion);
        presetMenuExplosion.setJustificationType(juce::Justification::centred);
        presetMenuExplosion.setLookAndFeel(&modernLookAndFeel);
        presetMenuExplosion.setTextWhenNothingSelected("Select Preset");
        int id = 1;
        for (const auto& p : audioProcessor.presetsexplosion)
        {
            presetMenuExplosion.addItem(p.name, id++);
        }

        presetMenuExplosion.onChange = [this] {
        
            audioProcessor.loadPresetExplosion(presetMenuExplosion.getSelectedId() - 1);
        };
    auto setupSliderAndLabel = [&](juce::Slider& slider, const char* paramID, juce::Label& label, const char* labelText)
    {
      
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
        

        addAndMakeVisible(slider);
        
        // Label Setup: Centered, bold text, and orange color
        label.setText(labelText, juce::dontSendNotification);
        label.setFont(juce::Font(12.0f, juce::Font::bold));
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(label);
        
        // Initial visibility
        slider.setVisible(false);
        label.setVisible(false);
    };

  
    setupSliderAndLabel(rumbleSlider, "rumble", RumbleLabel, "Rumble");
    rumbleAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.parameters, "rumble", rumbleSlider));
    rumbleSlider.onDragStart = [this]() { updateAssistantForParameter("rumble"); };
    
    setupSliderAndLabel(rumbleDecaySlider, "rumbleDecay", RumbleDecayLabel, "Rumble Decay");
    rumbleDecayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "rumbleDecay", rumbleDecaySlider);
    rumbleDecaySlider.onDragStart = [this]() { updateAssistantForParameter("rumbleDecay"); };
    
    // 3. Air
    setupSliderAndLabel(AirSlider, "air", AirLabel, "Air");
    airAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "air", AirSlider);
    AirSlider.onDragStart = [this]() { updateAssistantForParameter("air"); };
    // 4. Air Decay
    setupSliderAndLabel(AirDecaySlider, "airDecay", AirDecayLabel, "Air Decay");
    airDecayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "airDecay", AirDecaySlider);
    AirDecaySlider.onDragStart = [this]() { updateAssistantForParameter("airDecay"); };
    
    // 5. Dust
    setupSliderAndLabel(DustSlider, "dust", DustLabel, "Dust");
    dustAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "dust", DustSlider);
    DustSlider.onDragStart = [this]() { updateAssistantForParameter("dust"); };

    // 6. Dust Decay
    setupSliderAndLabel(DustDecaySlider, "dustDecay", DustDecayLabel, "Dust Decay");
    dustDecayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "dustDecay", DustDecaySlider);
    DustDecaySlider.onDragStart = [this]() { updateAssistantForParameter("dustDecay"); };
    // 7. Grit Amount
    setupSliderAndLabel(GritAmountSlider, "gritAmount", GritAmountLabel, "Grit Amount");
    gritAmountAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "gritAmount", GritAmountSlider);
    GritAmountSlider.onDragStart = [this]() { updateAssistantForParameter("gritAmount"); };
}

int QAPAudioProcessorEditor::getNumRows()
{
    return filteredWavFileNames.size();
}

void QAPAudioProcessorEditor::loadThumbnail (const juce::File& file)
{
    thumbnail.setSource (new juce::FileInputSource (file));
}
void QAPAudioProcessorEditor::paintListBoxItem (int rowNumber, juce::Graphics& g,
                                                int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= filteredWavFileNames.size()) return;

    juce::String fileName = filteredWavFileNames[rowNumber];

    if (fileName == currentlyPlayingFileName)
    {
        g.fillAll(juce::Colour(0xff5AC8AA).withAlpha(0.6f));
        g.setColour(juce::Colours::white);
    }
    else if (rowIsSelected)
    {
        g.fillAll(juce::Colour(0xff5AC8AA).withAlpha(0.2f));
        g.setColour(juce::Colours::white);
    }
    else
    {
        g.setColour(juce::Colours::grey);
    }

    g.drawText(fileName, 5, 0, width, height, juce::Justification::centredLeft);
}

void QAPAudioProcessorEditor::listBoxItemClicked (int row, const juce::MouseEvent& e)
{
    if (row >= 0 && row < filteredWavFileNames.size())
    {
        juce::String fileName = filteredWavFileNames[row];
        
        currentlyPlayingFileName = filteredWavFileNames[row];
        
        audioProcessor.playWavFileByName(fileName);
        for (auto file : audioProcessor.getSimilarFiles())
        {
            if (file.getFileName() == fileName)
            {
                thumbnail.setSource (new juce::FileInputSource (file));
                break;
            }
        }
    }
}
void QAPAudioProcessorEditor::refreshWavFileList()
{
    filteredWavFileNames = audioProcessor.getWavFileNames();
    wavFileList.updateContent();
    wavFileList.repaint();
}
 
       

//Mouse movementd for the thumbnail
void QAPAudioProcessorEditor::mouseDown (const juce::MouseEvent& event)
{
    if (waveformBounds.contains (event.position))
    {
        selectionStart = (event.position.x - waveformBounds.getX()) / waveformBounds.getWidth();
        selectionEnd = selectionStart;
        repaint();
    }
}

void QAPAudioProcessorEditor::mouseDrag (const juce::MouseEvent& event)
{
    if (selectionStart >= 0.0f)
    {
        float currentX = (event.position.x - waveformBounds.getX()) / waveformBounds.getWidth();
        selectionEnd = juce::jlimit (0.0f, 1.0f, currentX);
        repaint();
    }
}
void QAPAudioProcessorEditor::mouseUp (const juce::MouseEvent& event)
{
    if (!waveformBounds.contains(event.mouseDownPosition))
        return;

    double totalLength = thumbnail.getTotalLength();
    if (totalLength <= 0.0)
        return;

    
    float normStart = juce::jmin(selectionStart, selectionEnd);
    float normEnd   = juce::jmax(selectionStart, selectionEnd);

    if (std::abs(normEnd - normStart) < 0.005f)
    {
        double seekTime = normStart * totalLength;
        audioProcessor.transportSource.setPosition(seekTime);
        audioProcessor.transportSource.start();
        isPlayingSelection = false;
        selectionEndTime = -1.0;
        stopTimer();
        return;
    }

    double startTime = normStart * totalLength;
    selectionEndTime = normEnd * totalLength;

    audioProcessor.transportSource.setPosition(startTime);
    audioProcessor.transportSource.start();
    isPlayingSelection = true;

    startTimer(30);
}

bool QAPAudioProcessorEditor::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::spaceKey)
    {
        audioProcessor.startpause();

        const bool nowPlaying = audioProcessor.transportSource.isPlaying();
        playpauseButton.setButtonText(nowPlaying ? "Pause" : "Play");
        return true; // consume the keypress
    }
    return false;
}

void QAPAudioProcessorEditor::updateWindowSize()
{
    const int normalWidth = 700;
    const int normalHeight = 780;
    const int proceduralPanelWidth = 250;
    const int padding = 20;

    bool anyModeActive = explosionMode || FireMode || GunMode ||
                         JetMode || HelicopterMode || RocketMode;

    if (anyModeActive)
        setSize(normalWidth + proceduralPanelWidth + padding, normalHeight);
    else
        setSize(normalWidth, normalHeight);
}

//Rocket UI
void QAPAudioProcessorEditor::layoutRocketUI()
{
    if (!RocketMode) return;

    // --- 1. Dimensions ---
    const int panelWidth    = 250;
    const int knobSize      = 80;
    const int labelHeight   = 18;
    const int controlMargin = 15;
    const int buttonHeight  = 35;
    const int controlUnitHeight = labelHeight + knobSize + 25;

    // --- 2. Positioning (Sidebar Anchor) ---
    const int marginRight = 10;
    const int panelX      = getWidth() - panelWidth - marginRight;
    const int panelTop    = searchBar.getY(); // Align with top of search bar

    // Height for 3 sliders in a single column + header area
    const int totalHeight = 160 + (3 * controlUnitHeight);

    RocketPanel.setBounds(panelX, panelTop, panelWidth, totalHeight);
    RocketPanel.toBack(); // Ensure background is behind controls

    // --- 3. Internal Component Layout ---
    int currentY = panelTop + 20;
    int centerX = panelX + (panelWidth / 2);

    // Rocket Button
    RocketButton.setBounds(centerX - 60, currentY, 120, buttonHeight);
    
    // Preset Menu (Corrected positioning)
    currentY = RocketButton.getBottom() + 10;
    presetMenuRocket.setBounds(centerX - 75, currentY, 150, 24);

    // Sliders Start
    currentY = presetMenuRocket.getBottom() + 20;

    juce::Component* sliders[] = { &DurationSlider, &ChamberResonanceSlider, &FlutterSlider };
    juce::Component* labels[] = { &DurationLabel, &ChamberResonanceLabel, &FlutterLabel };
    
    for (int i = 0; i < 3; ++i)
    {
        labels[i]->setBounds(panelX, currentY, panelWidth, labelHeight);
        sliders[i]->setBounds(centerX - (knobSize / 2), labels[i]->getBottom(), knobSize, knobSize + 25);
        
        currentY += controlUnitHeight + controlMargin;
    }
}

//HelicopterUI
void QAPAudioProcessorEditor::layoutHelicopterUI()
{
    if (!HelicopterMode) return;

    // --- 1. Dimensions & Grid Logic ---
    const int numSliders = 8;
    const int numColumns = 2;
    const int panelWidth  = 260; // Slightly wider for 2 columns of 80px knobs
    const int knobSize    = 80;
    const int labelHeight = 18;
    const int buttonHeight  = 35;
    
    // Space between labels and knobs
    const int controlUnitHeight = labelHeight + knobSize + 25;

    // --- 2. Positioning (Sidebar Anchor) ---
    const int marginRight = 10;
    const int panelX      = getWidth() - panelWidth - marginRight;
    const int panelTop    = searchBar.getY(); // Sync with other modules

    // 4 rows of controls + Header space
    const int totalHeight = 160 + (4 * controlUnitHeight);

    helicopterPanel.setBounds(panelX, panelTop, panelWidth, totalHeight);
    helicopterPanel.toBack(); // Fix: Move panel behind the sliders

    // --- 3. Internal Component Layout ---
    int currentY = panelTop + 15;
    int centerX = panelX + (panelWidth / 2);

    // Helicopter Button
    HelicopterButton.setBounds(centerX - 60, currentY, 120, buttonHeight);
    
    // Preset Menu (Inserted below the button)
    currentY = HelicopterButton.getBottom() + 10;
    presetMenuHelicopter.setBounds(centerX - 75, currentY, 150, 24);

    // Grid Start
    currentY = presetMenuHelicopter.getBottom() + 15;

    juce::Component* sliders[] = {
        &RotorPeriodSlider, &PeriodSlider, &TailMixSlider, &BaseFreqSlider,
        &RotorMixSlider, &EngineMixSlider, &BladeNoiseSlider, &EngineSpeedSlider
    };

    juce::Component* labels[] = {
        &RotorPeriodLabel, &PeriodLabel, &TailMixLabel, &BaseFreqLabel,
        &RotorMixLabel, &EngineMixLabel, &BladeNoiseLabel, &EngineSpeedLabel
    };

    // Use internal padding for columns
    int colSpacing = 15;
    int totalGridWidth = (numColumns * knobSize) + ((numColumns - 1) * colSpacing);
    int gridStartX = panelX + (panelWidth - totalGridWidth) / 2;

    for (int i = 0; i < numSliders; ++i)
    {
        int col = i % numColumns;
        int row = i / numColumns;

        int x = gridStartX + (col * (knobSize + colSpacing));
        int y = currentY + (row * controlUnitHeight);

        labels[i]->setBounds(x, y, knobSize, labelHeight);
        sliders[i]->setBounds(x, y + labelHeight, knobSize, knobSize + 20);
    }
}
//JetUI
void QAPAudioProcessorEditor::layoutJetUI()
{
    if(!JetMode) return;

    // --- 1. Dimensions ---
    const int panelWidth    = 250;
    const int knobSize      = 80;
    const int labelHeight   = 20;
    const int controlMargin = 15;
    const int buttonHeight  = 35;
    const int controlUnitHeight = labelHeight + knobSize + 25;
    
    // --- 2. Positioning (Sidebar Anchor) ---
    const int marginRight = 10;
    const int panelX      = getWidth() - panelWidth - marginRight;
    
    // Consistent with Explosion/Fire/Gun
    const int panelTop    = searchBar.getY();

    // Calculate height for 3 sliders in a single column
    const int totalHeight = 160 + (3 * controlUnitHeight);

    JetPanel.setBounds(panelX, panelTop, panelWidth, totalHeight);
    JetPanel.toBack(); // Ensure background is behind controls

    // --- 3. Internal Component Layout ---
    int currentY = panelTop + 20;
    int centerX = panelX + (panelWidth / 2);

    // Jet Button
    JetButton.setBounds(centerX - 60, currentY, 120, buttonHeight);
    
    // Preset Menu (Corrected positioning)
    currentY = JetButton.getBottom() + 10;
    presetMenuJet.setBounds(centerX - 75, currentY, 150, 24);

    // Sliders Start
    currentY = presetMenuJet.getBottom() + 20;

    // Components arrays
    juce::Component* sliders[] = { &SpeedSlider, &TurbineSlider, &BurnSlider };
    juce::Component* labels[]  = { &SpeedLabel,  &TurbineLabel,  &BurnLabel  };
    
    for (int i = 0; i < 3; ++i)
    {
        // Label centered in panel
        labels[i]->setBounds(panelX, currentY, panelWidth, labelHeight);
        
        // Slider centered in panel
        sliders[i]->setBounds(centerX - (knobSize / 2), labels[i]->getBottom(), knobSize, knobSize + 25);
        
        currentY += controlUnitHeight + controlMargin;
    }
}

//GunUI
void QAPAudioProcessorEditor::layoutGunUI()
{
    if(!GunMode) return;

    // --- 1. Dimensions ---
    const int panelWidth    = 250;
    const int knobSize      = 80;
    const int labelHeight   = 20;
    const int controlMargin = 15;
    const int buttonHeight  = 35;
    const int internalPadding = 15;
    const int controlUnitHeight = labelHeight + knobSize + 25;

    // --- 2. Positioning (Sidebar Anchor) ---
    const int marginRight = 10;
    const int panelX      = getWidth() - panelWidth - marginRight;
    const int panelTop    = searchBar.getY(); // Align with top of search bar

    // Calculate height for 2 sliders in a single column
    const int totalHeight = 150 + (2 * controlUnitHeight);

    GunPanel.setBounds(panelX, panelTop, panelWidth, totalHeight);
    GunPanel.toBack(); // Fix: Move panel behind the sliders

    // --- 3. Internal Component Layout ---
    int currentY = panelTop + 20;

    // Gun Button (Shot Gun)
    GunButton.setBounds(panelX + (panelWidth / 2) - 60, currentY, 120, buttonHeight);
    
    // Preset Menu (Corrected positioning below the button)
    currentY = GunButton.getBottom() + 10;
    presetMenuGun.setBounds(panelX + (panelWidth / 2) - 75, currentY, 150, 24);

    // Sliders Start
    currentY = presetMenuGun.getBottom() + 20;
    int centerX = panelX + (panelWidth / 2);

    // Shell Frequency
    ShellFrequecyLabel.setBounds(panelX, currentY, panelWidth, labelHeight);
    ShellFreqSlider.setBounds(centerX - (knobSize / 2), ShellFrequecyLabel.getBottom(), knobSize, knobSize + 20);

    // Shell Frequency Decay
    currentY = ShellFreqSlider.getBottom() + controlMargin;
    ShellFrequencyDecayLabel.setBounds(panelX, currentY, panelWidth, labelHeight);
    ShellFreqDecaySlider.setBounds(centerX - (knobSize / 2), ShellFrequencyDecayLabel.getBottom(), knobSize, knobSize + 20);
}

//Fire UI
void QAPAudioProcessorEditor::layoutFireUI()
{
    if (!FireMode) return;

    const int numColumns = 2;
    const int panelWidth  = 250;
    const int knobSize     = 80;
    const int labelHeight  = 18;
    const int buttonHeight = 35;
    const int controlUnitHeight = labelHeight + knobSize + 25;

    const int marginRight = 10;
    const int panelX      = getWidth() - panelWidth - marginRight;
    const int panelTop    = searchBar.getY();

    const int totalHeight = 160 + (2 * controlUnitHeight);
    firePanel.setBounds(panelX, panelTop, panelWidth, totalHeight);
    firePanel.toBack();

    int currentY = panelTop + 20;
    int centerX  = panelX + (panelWidth / 2);

    // Fire Button
    FireButton.setBounds(centerX - 60, currentY, 120, buttonHeight);

    // Preset Menu
    currentY = FireButton.getBottom() + 10;
    presetMenuFire.setBounds(centerX - 75, currentY, 150, 24);

    // Grid Start
    currentY = presetMenuFire.getBottom() + 15;

    juce::Component* sliders[] = { &LappingSlider, &HissingSlider, &CracklingSlider, &IntensitySlider };
    juce::Component* labels[]  = { &LappingLabel,  &HissingLabel,  &CracklingLabel,  &IntensityLabel  };

    int colSpacing = 15;
    int totalGridWidth = (numColumns * knobSize) + ((numColumns - 1) * colSpacing);
    int gridStartX = panelX + (panelWidth - totalGridWidth) / 2;

    for (int i = 0; i < 4; ++i)
    {
        int col = i % numColumns;
        int row = i / numColumns;

        int x = gridStartX + (col * (knobSize + colSpacing));
        int y = currentY + (row * controlUnitHeight);

        labels[i]->setBounds(x, y, knobSize, labelHeight);
        sliders[i]->setBounds(x, y + labelHeight, knobSize, knobSize + 20);
    }
}
//EXPLOSION UI
void QAPAudioProcessorEditor::layoutExplosionUI()
{
    if (!explosionMode) return;
    const int numColumns = 3;
    const int knobSize = 75;
    const int labelHeight = 18;
    const int controlSpacing = 12;
    const int controlUnitHeight = labelHeight + knobSize + 25;
    const int panelWidth  = (numColumns * knobSize) + ((numColumns + 1) * controlSpacing);

    const int marginRight = 10;
    const int panelX      = getWidth() - panelWidth - marginRight;
    const int panelTop    = searchBar.getY();

    const int numSliders = 7;
    const int numRows = (numSliders + numColumns - 1) / numColumns;
    const int panelHeight = 130 + (numRows * controlUnitHeight);

    explosionPanel.setBounds(panelX, panelTop, panelWidth, panelHeight);

    int currentY = panelTop + 20;

    triggerButton.setBounds(panelX + (panelWidth / 2) - 50, currentY, 100, 32);

    currentY = triggerButton.getBottom() + 10;
    presetMenuExplosion.setBounds(panelX + (panelWidth / 2) - 75, currentY, 150, 24);

    currentY = presetMenuExplosion.getBottom() + 15;

    juce::Component* sliders[] = {
        &rumbleSlider, &rumbleDecaySlider, &AirSlider, &AirDecaySlider,
        &DustSlider, &DustDecaySlider, &GritAmountSlider
    };

    juce::Component* labels[] = {
        &RumbleLabel, &RumbleDecayLabel, &AirLabel, &AirDecayLabel,
        &DustLabel, &DustDecayLabel, &GritAmountLabel
    };

    for (int i = 0; i < numSliders; ++i)
    {
        int col = i % numColumns;
        int row = i / numColumns;

        int x = panelX + controlSpacing + (col * (knobSize + controlSpacing));
        int y = currentY + (row * controlUnitHeight);

        labels[i]->setBounds(x, y, knobSize, labelHeight);
        sliders[i]->setBounds(x, y + labelHeight, knobSize, knobSize + 20);
    }
}


void QAPAudioProcessorEditor::filterFileList(const juce::String& searchText)
{
    filteredWavFileNames.clear();
    if (currentlyPlayingFileName.isNotEmpty())
        filteredWavFileNames.add(currentlyPlayingFileName);
    if (searchText.isEmpty())
    {
        filteredWavFileNames = audioProcessor.getWavFileNames();
    }
    else
    {
        for (auto& name : audioProcessor.getWavFileNames())
        {
            if (name.containsIgnoreCase(searchText))
                filteredWavFileNames.add(name);
        }
    }

    wavFileList.updateContent();
    if (currentlyPlayingFileName.isNotEmpty())
        {
            for (int i = 0; i < filteredWavFileNames.size(); ++i)
            {
                if (filteredWavFileNames[i] == currentlyPlayingFileName)
                {
                    wavFileList.selectRow(i, false, false);
                    break;
                }
            }
        }
    wavFileList.repaint();
    updateAssistant(searchText);
}

void QAPAudioProcessorEditor::selectedRowsChanged(int lastRowSelected)
{
    if (lastRowSelected >= 0 && lastRowSelected < filteredWavFileNames.size())
    {
        auto fileName = filteredWavFileNames[lastRowSelected];
        audioProcessor.playWavFileByName(fileName);

        auto file = audioProcessor.getWavFileByName(fileName);
        if (file.existsAsFile())
        {
            thumbnail.setSource(new juce::FileInputSource(file));

            repaint(); 
        }
    }
}

//Rocket SetUp
void QAPAudioProcessorEditor::setRocketMode(bool shouldShow)
{
    if (RocketMode == shouldShow)
        return;
    //Rocket
    RocketMode = shouldShow;
    RocketPanel.setVisible(shouldShow);
    RocketButton.setVisible(shouldShow);
    presetMenuRocket.setVisible(shouldShow);
    DurationSlider.setVisible(shouldShow);
    DurationLabel.setVisible(shouldShow);
    ChamberResonanceSlider.setVisible(shouldShow);
    ChamberResonanceLabel.setVisible(shouldShow);
    FlutterSlider.setVisible(shouldShow);
    FlutterLabel.setVisible(shouldShow);
    
    //Helicopter
    HelicopterMode = false;
    helicopterPanel.setVisible(false);
    HelicopterButton.setVisible(false);
    RotorPeriodSlider.setVisible(false);
    PeriodSlider.setVisible(false);
    TailMixSlider.setVisible(false);
    BaseFreqSlider.setVisible(false);
    RotorMixSlider.setVisible(false);
    EngineMixSlider.setVisible(false);
    BladeNoiseSlider.setVisible(false);
    EngineSpeedSlider.setVisible(false);
    //Hide Jet
    JetPanel.setVisible(false);
    JetButton.setVisible(false);
    SpeedSlider.setVisible(false);
    TurbineSlider.setVisible(false);
    BurnSlider.setVisible(false);
    
    //Hide GUN GUI
    GunPanel.setVisible(false);
    GunButton.setVisible(false);
    ShellFreqSlider.setVisible(false);
    ShellFreqDecaySlider.setVisible(false);
    
    // hide all Fire UI elements
    firePanel.setVisible(false);
    FireButton.setVisible(false);
    LappingSlider.setVisible(false);
    HissingSlider.setVisible(false);
    CracklingSlider.setVisible(false);
    IntensitySlider.setVisible(false);

    // Explicitly hide all Explosion UI elements
    explosionPanel.setVisible(false);
    presetMenuExplosion.setVisible(false);
    triggerButton.setVisible(false);
    rumbleSlider.setVisible(false);
    RumbleLabel.setVisible(false);
    rumbleDecaySlider.setVisible(false);
    RumbleDecayLabel.setVisible(false);
    AirSlider.setVisible(false);
    AirLabel.setVisible(false);
    AirDecaySlider.setVisible(false);
    AirDecayLabel.setVisible(false);
    DustSlider.setVisible(false);
    DustLabel.setVisible(false);
    DustDecaySlider.setVisible(false);
    DustDecayLabel.setVisible(false);
    GritAmountSlider.setVisible(false);
    GritAmountSlider.setVisible(false);


    updateWindowSize();
    resized();
}

//HELICOPTER STUP

void QAPAudioProcessorEditor::setHelicopterMode(bool shouldShow)
{
    if (HelicopterMode == shouldShow)
        return;
    HelicopterMode = shouldShow;
    helicopterPanel.setVisible(shouldShow);
    HelicopterButton.setVisible(shouldShow);
    presetMenuHelicopter.setVisible(shouldShow);
    RotorPeriodSlider.setVisible(shouldShow);
    RotorPeriodLabel.setVisible(shouldShow);
    PeriodSlider.setVisible(shouldShow);
    PeriodLabel.setVisible(shouldShow);
    TailMixSlider.setVisible(shouldShow);
    TailMixLabel.setVisible(shouldShow);
    BaseFreqSlider.setVisible(shouldShow);
    BaseFreqLabel.setVisible(shouldShow);
    RotorMixSlider.setVisible(shouldShow);
    RotorMixLabel.setVisible(shouldShow);
    EngineMixSlider.setVisible(shouldShow);
    EngineMixLabel.setVisible(shouldShow);
    BladeNoiseSlider.setVisible(shouldShow);
    BladeNoiseLabel.setVisible(shouldShow);
    EngineSpeedSlider.setVisible(shouldShow);
    EngineSpeedLabel.setVisible(shouldShow);
    
    //Hide Rocket
    RocketPanel.setVisible(false);
    RocketButton.setVisible(false);
    RocketPanel.setVisible(false);
    RocketButton.setVisible(false);
    DurationSlider.setVisible(false);
    ChamberResonanceSlider.setVisible(false);
    FlutterSlider.setVisible(false);
    //Hide Jet
    JetPanel.setVisible(false);
    JetButton.setVisible(false);
    SpeedSlider.setVisible(false);
    TurbineSlider.setVisible(false);
    BurnSlider.setVisible(false);
    
    //Hide GUN GUI
    GunPanel.setVisible(false);
    GunButton.setVisible(false);
    ShellFreqSlider.setVisible(false);
    ShellFreqDecaySlider.setVisible(false);
    
    // hide all Fire UI elements
    firePanel.setVisible(false);
    FireButton.setVisible(false);
    LappingSlider.setVisible(false);
    HissingSlider.setVisible(false);
    CracklingSlider.setVisible(false);
    IntensitySlider.setVisible(false);

    // Explicitly hide all Explosion UI elements
    explosionPanel.setVisible(false);
    presetMenuExplosion.setVisible(false);
    triggerButton.setVisible(false);
    rumbleSlider.setVisible(false);
    rumbleDecaySlider.setVisible(false);
    AirSlider.setVisible(false);
    AirDecaySlider.setVisible(false);
    DustSlider.setVisible(false);
    DustDecaySlider.setVisible(false);
    GritAmountSlider.setVisible(false);


    updateWindowSize();
    resized();
}

void QAPAudioProcessorEditor::setJetMode(bool shouldShow)
{
    if (JetMode == shouldShow)
        return;

    JetMode = shouldShow;
    JetPanel.setVisible(shouldShow);
    presetMenuJet.setVisible(shouldShow);
    JetButton.setVisible(shouldShow);
    SpeedSlider.setVisible(shouldShow);
    SpeedLabel.setVisible(shouldShow);
    TurbineSlider.setVisible(shouldShow);
    TurbineLabel.setVisible(shouldShow);
    BurnSlider.setVisible(shouldShow);
    BurnLabel.setVisible(shouldShow);
    //ROCKET
    RocketPanel.setVisible(false);
    RocketButton.setVisible(false);
    RocketPanel.setVisible(false);
    RocketButton.setVisible(false);
    DurationSlider.setVisible(false);
    ChamberResonanceSlider.setVisible(false);
    FlutterSlider.setVisible(false);
    //Hide Helicopter
    helicopterPanel.setVisible(false);
    HelicopterButton.setVisible(false);
    RotorPeriodSlider.setVisible(false);
    PeriodSlider.setVisible(false);
    TailMixSlider.setVisible(false);
    BaseFreqSlider.setVisible(false);
    RotorMixSlider.setVisible(false);
    EngineMixSlider.setVisible(false);
    BladeNoiseSlider.setVisible(false);
    EngineSpeedSlider.setVisible(false);
    
    //Hide JEt GUI
    GunPanel.setVisible(false);
    GunButton.setVisible(false);
    ShellFreqSlider.setVisible(false);
    ShellFreqDecaySlider.setVisible(false);
    
    // hide all Fire UI elements
    firePanel.setVisible(false);
    FireButton.setVisible(false);
    LappingSlider.setVisible(false);
    HissingSlider.setVisible(false);
    CracklingSlider.setVisible(false);
    IntensitySlider.setVisible(false);

    // Explicitly hide all Explosion UI elements
    explosionPanel.setVisible(false);
    triggerButton.setVisible(false);
    rumbleSlider.setVisible(false);
    rumbleDecaySlider.setVisible(false);
    AirSlider.setVisible(false);
    AirDecaySlider.setVisible(false);
    DustSlider.setVisible(false);
    DustDecaySlider.setVisible(false);
    GritAmountSlider.setVisible(false);


    updateWindowSize();
    resized();
}

void QAPAudioProcessorEditor::setGunMode(bool shouldShow)
{
    if (GunMode == shouldShow)
        return;

    GunMode = shouldShow;
    GunPanel.setVisible(shouldShow);
    GunButton.setVisible(shouldShow);
    ShellFreqSlider.setVisible(shouldShow);
    ShellFrequecyLabel.setVisible(shouldShow);
    ShellFreqDecaySlider.setVisible(shouldShow);
    ShellFrequencyDecayLabel.setVisible(shouldShow);
    //ROCKET
    RocketPanel.setVisible(false);
    RocketButton.setVisible(false);
    RocketPanel.setVisible(false);
    RocketButton.setVisible(false);
    DurationSlider.setVisible(false);
    ChamberResonanceSlider.setVisible(false);
    FlutterSlider.setVisible(false);
    //Hide Helicopter GUI
    helicopterPanel.setVisible(false);
    HelicopterButton.setVisible(false);
    RotorPeriodSlider.setVisible(false);
    PeriodSlider.setVisible(false);
    TailMixSlider.setVisible(false);
    BaseFreqSlider.setVisible(false);
    RotorMixSlider.setVisible(false);
    EngineMixSlider.setVisible(false);
    BladeNoiseSlider.setVisible(false);
    EngineSpeedSlider.setVisible(false);
    
    //Hide JEt GUI
    JetPanel.setVisible(false);
    JetButton.setVisible(false);
    SpeedSlider.setVisible(false);
    TurbineSlider.setVisible(false);
    BurnSlider.setVisible(false);
    
    // hide all Fire UI elements
    firePanel.setVisible(false);
    FireButton.setVisible(false);
    LappingSlider.setVisible(false);
    HissingSlider.setVisible(false);
    CracklingSlider.setVisible(false);
    IntensitySlider.setVisible(false);

    // Explicitly hide all Explosion UI elements
    explosionPanel.setVisible(false);
    triggerButton.setVisible(false);
    rumbleSlider.setVisible(false);
    rumbleDecaySlider.setVisible(false);
    AirSlider.setVisible(false);
    AirDecaySlider.setVisible(false);
    DustSlider.setVisible(false);
    DustDecaySlider.setVisible(false);
    GritAmountSlider.setVisible(false);

    updateWindowSize();
    resized();
}

void QAPAudioProcessorEditor::setExplosionMode(bool shouldShow)
{
    if (explosionMode == shouldShow)
        return;

    explosionMode = shouldShow;
    explosionPanel.setVisible(shouldShow);
    triggerButton.setVisible(shouldShow);
    presetMenuExplosion.setVisible(shouldShow);
    rumbleSlider.setVisible(shouldShow);
    RumbleLabel.setVisible(shouldShow);
    
    rumbleDecaySlider.setVisible(shouldShow);
    RumbleDecayLabel.setVisible(shouldShow);
    
    AirSlider.setVisible(shouldShow);
    AirLabel.setVisible(shouldShow);
    
    AirDecaySlider.setVisible(shouldShow);
    AirDecayLabel.setVisible(shouldShow);
    
    DustSlider.setVisible(shouldShow);
    DustLabel.setVisible(shouldShow);
    
    DustDecaySlider.setVisible(shouldShow);
    DustDecayLabel.setVisible(shouldShow);
    
    GritAmountSlider.setVisible(shouldShow);
    GritAmountLabel.setVisible(shouldShow);
    
    //Rocket
    RocketPanel.setVisible(false);
    RocketButton.setVisible(false);
    RocketPanel.setVisible(false);
    RocketButton.setVisible(false);
    DurationSlider.setVisible(false);
    ChamberResonanceSlider.setVisible(false);
    FlutterSlider.setVisible(false);
    //Fire
    firePanel.setVisible(false);
    FireButton.setVisible(false);
    HissingSlider.setVisible(false);
    LappingSlider.setVisible(false);
    CracklingSlider.setVisible(false);
    IntensitySlider.setVisible(false);
    
    //GUN
    GunPanel.setVisible(false);
    GunButton.setVisible(false);
    ShellFreqSlider.setVisible(false);
    ShellFreqDecaySlider.setVisible(false);
    //JET
    JetPanel.setVisible(false);
    JetButton.setVisible(false);
    SpeedSlider.setVisible(false);
    TurbineSlider.setVisible(false);
    BurnSlider.setVisible(false);
    //Helicopter
    helicopterPanel.setVisible(false);
    HelicopterButton.setVisible(false);
    RotorPeriodSlider.setVisible(false);
    PeriodSlider.setVisible(false);
    TailMixSlider.setVisible(false);
    BaseFreqSlider.setVisible(false);
    RotorMixSlider.setVisible(false);
    EngineMixSlider.setVisible(false);
    BladeNoiseSlider.setVisible(false);
    EngineSpeedSlider.setVisible(false);
    

    updateWindowSize(); // Check if we need to grow/shrink
    resized();
}


void QAPAudioProcessorEditor::setFireMode(bool shouldShow)
{
    if (FireMode == shouldShow)
        return;

    FireMode = shouldShow;

    // Show/hide all Fire UI elements
    firePanel.setVisible(shouldShow);
    FireButton.setVisible(shouldShow);
    presetMenuFire.setVisible(shouldShow);
    LappingSlider.setVisible(shouldShow);
    LappingLabel.setVisible(shouldShow);
    HissingSlider.setVisible(shouldShow);
    HissingLabel.setVisible(shouldShow);
    CracklingSlider.setVisible(shouldShow);
    CracklingLabel.setVisible(shouldShow);
    IntensitySlider.setVisible(shouldShow);
    IntensityLabel.setVisible(shouldShow);
    //Rocket
    RocketPanel.setVisible(false);
    RocketButton.setVisible(false);
    RocketPanel.setVisible(false);
    RocketButton.setVisible(false);
    DurationSlider.setVisible(false);
    ChamberResonanceSlider.setVisible(false);
    FlutterSlider.setVisible(false);
    // Explicitly hide all Explosion UI elements
    explosionPanel.setVisible(false);
    triggerButton.setVisible(false);
    rumbleSlider.setVisible(false);
    RumbleLabel.setVisible(false);
    rumbleDecaySlider.setVisible(false);
    RumbleDecayLabel.setVisible(false);
    AirSlider.setVisible(false);
    AirLabel.setVisible(false);
    AirDecaySlider.setVisible(false);
    AirDecayLabel.setVisible(false);
    DustSlider.setVisible(false);
    DustLabel.setVisible(false);
    DustDecaySlider.setVisible(false);
    DustDecayLabel.setVisible(false);
    GritAmountSlider.setVisible(false);
    GritAmountLabel.setVisible(false);
    
    //Gun
    GunPanel.setVisible(false);
    GunButton.setVisible(false);
    ShellFreqSlider.setVisible(false);
    ShellFreqDecaySlider.setVisible(false);
    //Jet
    JetPanel.setVisible(false);
    JetButton.setVisible(false);
    SpeedSlider.setVisible(false);
    TurbineSlider.setVisible(false);
    BurnSlider.setVisible(false);

    //helicopter
    helicopterPanel.setVisible(false);
    HelicopterButton.setVisible(false);
    RotorPeriodSlider.setVisible(false);
    PeriodSlider.setVisible(false);
    TailMixSlider.setVisible(false);
    BaseFreqSlider.setVisible(false);
    RotorMixSlider.setVisible(false);
    EngineMixSlider.setVisible(false);
    BladeNoiseSlider.setVisible(false);
    EngineSpeedSlider.setVisible(false);
    

    updateWindowSize();
    resized();
}

void QAPAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    // =============================================
        // HEADER SECTION
        // =============================================
        auto headerArea = area.removeFromTop(110);

        // Buttons centered vertically in the header
        int buttonWidth = 95;
        int buttonHeight = 32;
        int gap = 8;
        int numButtons = 5;
        int totalWidth = (buttonWidth * numButtons) + (gap * (numButtons - 1));
        const int baseContentWidth = 700;
       int anchorRight = juce::jmin(getWidth(), baseContentWidth);
       int startX = anchorRight - totalWidth - 20;
        int buttonY = headerArea.getY() + (headerArea.getHeight() - buttonHeight) / 2 + 20;

        // Rectangle drawn behind the buttons (used in paint())
        transportPanelBounds = juce::Rectangle<int>(startX - 12, buttonY - 8,
                                                    totalWidth + 24, buttonHeight + 16);

        loadLibraryButton.setBounds(startX,                          buttonY, buttonWidth, buttonHeight);
        recordButton.setBounds     (startX + (buttonWidth + gap),    buttonY, buttonWidth, buttonHeight);
        playpauseButton.setBounds  (startX + (buttonWidth + gap)*2,  buttonY, buttonWidth, buttonHeight);
        stopButton.setBounds       (startX + (buttonWidth + gap)*3,  buttonY, buttonWidth, buttonHeight);
        loopButton.setBounds       (startX + (buttonWidth + gap)*4,  buttonY, buttonWidth, buttonHeight);

    // =============================================
    // BODY SECTION
    // =============================================
    area.reduce(20, 0);

    // Side Panel Reservation (procedural models)
    int proceduralWidth = 250;
    bool proceduralOpen = explosionMode || FireMode || GunMode || JetMode || HelicopterMode || RocketMode;
    if (proceduralOpen)
        area.removeFromRight(proceduralWidth + 10);

    // --- Remove from BOTTOM upward ---

    // 1. Three mix sliders SIDE BY SIDE in one row
        const int mixRowHeight = 50;
        auto mixRow = area.removeFromBottom(mixRowHeight);
        mixPanelBounds = mixRow.expanded(8, 4); // rectangle behind the sliders
        mixRow = mixRow.reduced(10, 4);         // inner padding for the sliders
        area.removeFromBottom(12);

    // 2. Procedural waveform (bottom of the two)
    const int waveformHeight = 70;
    proceduralWaveformBounds = area.removeFromBottom(waveformHeight).toFloat();
    proceduralVisualiser.setBounds(proceduralWaveformBounds.toNearestInt().reduced(2));

    // 3. Gap between the two waveforms
    area.removeFromBottom(8);

    // 4. Sample waveform (top of the two)
    waveformBounds = area.removeFromBottom(waveformHeight).toFloat();

    // 5. Gap between file list and waveforms
    area.removeFromBottom(10);

    // --- Layout the three mix sliders (label on top, slider below, side by side) ---
    int colW = mixRow.getWidth() / 3;
    auto col1 = mixRow.removeFromLeft(colW);
    auto col2 = mixRow.removeFromLeft(colW);
    auto col3 = mixRow;

    auto layoutVerticalMix = [](juce::Rectangle<int> col, juce::Label& label, juce::Slider& slider)
    {
        auto labelArea = col.removeFromTop(16);
        label.setBounds(labelArea);
        label.setJustificationType(juce::Justification::centred);
        slider.setBounds(col.reduced(6, 0));
    };
    layoutVerticalMix(col1, crossfadeLabel, crossfadeSlider);
    layoutVerticalMix(col2, reverbMixLabel, reverbMixSlider);
    layoutVerticalMix(col3, masterGainLabel, masterGainSlider);

    // --- Remove from TOP downward ---

    // Progress Bar
    progressBar.setBounds(area.removeFromTop(10).reduced(20, 0));
    area.removeFromTop(4);

    // Search Bar (3/4) + Procedural Model Dropdown (1/4)
    auto searchRow = area.removeFromTop(35);
    int dropdownWidth = searchRow.getWidth() / 4;
    proceduralModelMenu.setBounds(searchRow.removeFromRight(dropdownWidth));
    searchRow.removeFromRight(8); // small gap
    searchBar.setBounds(searchRow);
    area.removeFromTop(15);
    wavFileList.setBounds(area);

    // Assistant
    if (assistantImage.isVisible())
      {
          int charSize   = 80;
          int assistantY = wavFileList.getBottom() - 100;
         int assistantX = wavFileList.getX() + 10;
        assistantImage.setBounds(assistantX, assistantY, charSize, charSize);
        assistantLabel.setBounds(assistantX + charSize + 10, assistantY + 20, 250, 40);
          // Close button in the top-right corner of the assistant image
          assistantCloseButton.setBounds(assistantX + charSize - 12, assistantY - 4, 20, 20);
          assistantCloseButton.setVisible(true);
      }
      else
      {
          assistantCloseButton.setVisible(false);
      }
    // Procedural UI layouts
    layoutExplosionUI();
    layoutFireUI();
    layoutGunUI();
    layoutJetUI();
    layoutHelicopterUI();
    layoutRocketUI();
}


void QAPAudioProcessorEditor::updateAssistantForParameter(const juce::String& paramID)
{
    juce::String oldText = assistantLabel.getText();
    juce::String newText = "";

    if (paramID == "rumble")             newText = "It's the deep low-end rumble of the explosion.";
    else if (paramID == "rumbleDecay")    newText = "How fast or slow does the rumble fades.";
    else if (paramID == "air")           newText = "Try values >0.6. This controls the frequency of the shockwave.";
    else if (paramID == "airDecay")      newText = "Controls how fast the air blast fades.";
    else if (paramID == "dust")          newText = "Controls the debris after the explosion.Try values >0.6";
    else if (paramID == "dustDecay")     newText = "Controls how fast the dust fades.";
    else if (paramID == "gritAmount")    newText = "Adds roughness. How harsh you want the explosion to sound like?";
    else if (paramID == "lapping")       newText = "Try values <0.70. This controls the motion of the flames.";
    else if (paramID == "hissing")       newText = "Try values <0.50. This controls the steam 'Pssst'.";
    else if (paramID == "crackling")     newText = "Try values <0.50. Controls the 'Pops' from the material.";
    else if (paramID == "intensity")     newText = "Controls the overall energy of the fire.";
    else if (paramID == "shellfreq")     newText = "Controls the impacts depth. Higher values create denser impacts.";
    else if (paramID == "shellfreqdecay")newText = "Controls how fast the impact swill fade away.";
    else if (paramID == "speed")         newText = "Controls the intensity of the engin, the speed and movement.";
    else if (paramID == "turbine")       newText = "Controls the high-pitched mechanical sound from the engine.";
    else if (paramID == "burn")          newText = "Controls the combustion roar. Adds heat and intensity.";
    else if (paramID == "rotorPeriod")   newText = "Controls the timing between each blade pass.";
    else if (paramID == "period")        newText = "Try to start with 149. Acts like a timing reference for the rotor.";
    else if (paramID == "tailMix")       newText = "It's the 'Buzzz' higher values emphasize the effect.";
    else if (paramID == "baseFreq")      newText = "Controls the main body. Lower=heavy. Higher=lighter.";
    else if (paramID == "rotorMix")      newText = "Controls the 'Whup-whup'sound.";
    else if (paramID == "engineMix")     newText = "Controls the balance of the engine noise and the rotor sound.";
    else if (paramID == "bladeNoise")    newText = "Controls the air rush and 'slap' from rotor tips.";
    else if (paramID == "engineSpeed")   newText = "Try with higher values. Controls the overall intensity and pitch.";
    else if (paramID == "duration")      newText = "Controls how long the Rocket will sound.";
    else if (paramID == "flutter")       newText = "Adjusts the flutter or instability.";
    else if (paramID == "chamberresonance") newText = "It gives a reverb sensation.Adds depth. ";

    // Apply the text
    assistantLabel.setText(newText, juce::dontSendNotification);
    assistantImage.setVisible(newText.isNotEmpty());
    assistantLabel.setVisible(newText.isNotEmpty());
}
bool containsAny(const juce::String& text, const juce::StringArray& keywords)
{
    for (auto& word : keywords)
        if (text.containsIgnoreCase(word))
            return true;
    return false;
}

// Helper to reset all modes easily
void QAPAudioProcessorEditor::setAllModes(bool explosion, bool fire, bool gun, bool jet, bool heli, bool rocket)
{
    setExplosionMode(explosion);
    setFireMode(fire);
    setGunMode(gun);
    setJetMode(jet);
    setHelicopterMode(heli);
    setRocketMode(rocket);
}

void QAPAudioProcessorEditor::updateAssistant(const juce::String& searchText)
{
    bool wasVisible = assistantImage.isVisible();
    bool shouldBeVisible = false;

    // Define keyword lists for better variety
    juce::StringArray explosionWords = { "explosion", "boom", "detonation", "blast", "kaboom", "dynamite" };
    juce::StringArray fireWords      = { "fire", "burn", "flame", "inferno", "campfire", "blaze" };
    juce::StringArray gunWords       = { "gun", "shot", "weapon", "pistol", "rifle", "shoot", "ammo" };
    juce::StringArray jetWords       = { "jet", "plane", "aircraft", "turbine", "flyover", "sonic" };
    juce::StringArray heliWords      = { "helicopter", "chopper", "rotor", "heli", "propeller" };
    juce::StringArray rocketWords    = { "rocket", "missile", "launch", "space", "thruster", "nasa" };

    if (containsAny(searchText, explosionWords))
    {
        assistantLabel.setText("Hi! Are you ready to create your own explosion?", juce::dontSendNotification);
        setAllModes(true, false, false, false, false, false);
        shouldBeVisible = true;
    }
    else if (containsAny(searchText, fireWords))
    {
        assistantLabel.setText("Hello! Ready to create your own fire?", juce::dontSendNotification);
        setAllModes(false, true, false, false, false, false);
        shouldBeVisible = true;
    }
    else if (containsAny(searchText, gunWords))
    {
        assistantLabel.setText("Whoa! Are we gonna create some gunshots?", juce::dontSendNotification);
        setAllModes(false, false, true, false, false, false);
        shouldBeVisible = true;
    }
    else if (containsAny(searchText, jetWords))
    {
        assistantLabel.setText("Wohoo! We are about to create Jet sounds!", juce::dontSendNotification);
        setAllModes(false, false, false, true, false, false);
        shouldBeVisible = true;
    }
    else if (containsAny(searchText, heliWords))
    {
        assistantLabel.setText("Are we gonna create a helicopter?", juce::dontSendNotification);
        setAllModes(false, false, false, false, true, false);
        shouldBeVisible = true;
    }
    else if (containsAny(searchText, rocketWords))
    {
        assistantLabel.setText("Hello! Are you ready to create rockets?", juce::dontSendNotification);
        setAllModes(false, false, false, false, false, true);
        shouldBeVisible = true;
    }
    else
    {
        assistantImage.setVisible(false);
        assistantLabel.setVisible(false);
        setAllModes(false, false, false, false, false, false);
    }

    if (shouldBeVisible && !wasVisible)
    {
        audioProcessor.PlayAssistantSound = true;
    }

    if (shouldBeVisible)
    {
        assistantImage.setVisible(true);
        assistantLabel.setVisible(true);
    }

    resized();
}

