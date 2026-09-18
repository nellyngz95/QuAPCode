/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <atomic>
#include "ExplosionImpl.h"
#include "FireImpl.h"
#include "GunImpl.h"
#include "JetImpl.h"
#include "HelicopterImpl.h"
#include "RocketImpl.h"
#include <onnxruntime/onnxruntime_cxx_api.h>
#include <mutex>
#include <faiss/IndexFlat.h>
class QAPAudioProcessor  : public juce::AudioProcessor
                          
{
public:
    //==============================================================================
    QAPAudioProcessor();
    ~QAPAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    void loadAllWavFilesFromFolder(const juce::File& folder);
    void refreshWavFileList();          // Refresh list display (called from processor)
    void playWavFileByName(const juce::String& name);
    void startRecording();
    void stopRecording();
    bool getIsRecording() const { return isRecording.load(); }
    void triggerAssistantSound();
    

    // Variables
    juce::StringArray wavFileNames;
    juce::Array<juce::File> wavFilePaths;
    std::unique_ptr<juce::AudioFormatReaderSource> playSource;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transportSource;
    juce::AudioFormatManager formatManager;
    
    juce::StringArray getWavFileNames() const { return wavFileNames; }
    juce::File getWavFileByName(const juce::String& name) const;
    double indexingProgress { -1.0 };
    //Play pause and stop,
    void startpause();
    void stop();
    juce::AudioVisualiserComponent* proceduralVisualiser = nullptr;
    
    //Drag the file
    void LoadDroppedFile (const juce::File& file);
    //Sound for assistant
    std::unique_ptr<juce::AudioFormatReaderSource> assistantSound;
    std::atomic<bool> PlayAssistantSound { false };
    
    //ParameterValueTreeState
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState parameters;
    
    //Procedural Audio
    void triggerExplosion();
    std::unique_ptr<nemisindo::Explosion> explosionModel;
    void triggerFire();
    std::unique_ptr<nemisindo::Fire> fireModel;
    void triggerGun();
    std::unique_ptr<nemisindo::Gun> gunModel;
    void triggerJet();
    std::unique_ptr<nemisindo::Jet::impl> JetModel;
    void triggerHelicopter();
    std::unique_ptr<nemisindo::Helicopter::impl> HelicopterModel;
    void triggerRocket();
    std::unique_ptr<nemisindo::Rocket> RocketModel;
    
    //Recording button
    std::unique_ptr<juce::FileOutputStream> fileStream;
    std::unique_ptr<juce::AudioFormatWriter> writer;
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> threadedWriter;
    juce::TimeSliceThread backgroundThread {"Audio Recorder Thread"};
    std::atomic<juce::AudioFormatWriter::ThreadedWriter*> activeWriter { nullptr };
    
    std::atomic<bool> isRecording { false };
    juce::String SavedFile;
    
    //Post production effects.
    juce::dsp::ProcessSpec spec;
    juce::dsp::Reverb reverb;
    juce::dsp::Compressor <float> compressor;
   
    //Filters
    using Filter=juce::dsp::IIR::Filter<float>;
    using ChannelFilterChain=juce::dsp::ProcessorChain<Filter,Filter,Filter>;
    juce::dsp::AudioBlock<float> lastInputBlock;
    juce::OwnedArray<ChannelFilterChain> processors;
    
    //Scan Library for the model
    void indexLibraryFolder(const juce::File& folder);

    // Logic for finding similar sounds
    void performSimilaritySearch(const juce::File& droppedFile);
    juce::Array<juce::File> getSimilarFiles() { return similarFilesResults; }
    //PRESETS
    struct PresetExplosion {juce::String name;
        float rumble;
        float rumbleDecay;
        float dust;
        float dustDecay;
        float air;
        float airDecay;
        float grit;};
    void loadPresetExplosion(int index);
    std::vector<PresetExplosion> presetsexplosion;
    
    struct PresetFire{
                juce::String name;
                float lapping;
                float hissing;
                float crackling;
                float intensity;
            };

    void loadPresetFire(int index);
    std::vector<PresetFire> presetsfire;
    
    struct PresetGun {
                juce::String name;
                float shellFrequency;
                float shellFrequencyDecay;
            };

            void loadPresetGun(int index);
            std::vector<PresetGun> presetsgun;
    
    
    struct PresetJet {
           juce::String name;
           float speed;
           float turbine;
           float burn;
       };

       std::vector<PresetJet> presetsjet;
       void loadPresetJet(int index);
    
    struct PresetHelicopter {
            juce::String name;
            float rotorPeriod;
            float period;
            float tailMix;
            float baseFreq;
            float rotorMix;
            float engineMix;
            float bladeNoise;
            float engineSpeed;
        };
       
        void loadPresetHelicopter(int index);
        std::vector<PresetHelicopter> presetshelicopter;
    struct PresetRocket {
                juce::String name;
                float duration;
                float chamberResonance;
                float flutter;
              
            };

    void loadPresetRocket(int index);
    std::vector<PresetRocket> presetsrocket;
    std::vector<float> runInference(const juce::File& file);
    int optBatchSize=1;
    
private:
    //ONXX Model
    Ort::Env env{ ORT_LOGGING_LEVEL_WARNING, "QuAP_Inference" };
    std::unique_ptr<Ort::Session> onnxSession;
    const int embeddingSize = 960;
    //FAISS-Search Similarity
    std::unique_ptr<faiss::IndexFlatL2> faissIndex;
    std::mutex faissMutex; 
    juce::Array<juce::File> libraryFiles;
    juce::Array<juce::File> similarFilesResults;
    
    

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (QAPAudioProcessor)
    
    
    
};
