/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ExplosionImpl.h"
#include "HelicopterImpl.h"
#include "RocketImpl.h"

QAPAudioProcessor::QAPAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                      ), parameters(*this, nullptr, "parameters", createParameterLayout()),explosionModel(std::make_unique<nemisindo::Explosion>()),fireModel(std::make_unique<nemisindo::Fire>()),gunModel(std::make_unique<nemisindo::Gun>()),JetModel(std::make_unique<nemisindo::Jet::impl>()),HelicopterModel(std::make_unique<nemisindo::Helicopter::impl>()),RocketModel(std::make_unique<nemisindo::Rocket>()),backgroundThread("Audio Recorder Thread")

{

    formatManager.registerBasicFormats();
    backgroundThread.startThread();
    auto* reader = formatManager.createReaderFor (std::make_unique<juce::MemoryInputStream> (
                                                                                             BinaryData::AssistantSound_wav,
                                                                                             BinaryData::AssistantSound_wavSize,
            false));

        if (reader != nullptr)
        {
            readerSource = std::make_unique<juce::AudioFormatReaderSource> (reader, true);
            transportSource.setSource (readerSource.get());
        }
    try {
            Ort::SessionOptions sessionOptions;
            sessionOptions.SetIntraOpNumThreads(1);

            onnxSession = std::make_unique<Ort::Session>(
                env,
                BinaryData::mn_tc11_onnx,
                BinaryData::mn_tc11_onnxSize,
                sessionOptions
            );

            faissIndex = std::make_unique<faiss::IndexFlatL2>(960);//size of the 

            DBG("ONNX and Faiss initialized!");
        }
        catch (const std::exception& e) {
            DBG("Initialization error: " << e.what());
        }
    //Presets Explosion
    presetsexplosion.push_back({ "Big Blast",        0.9f, 3.0f, 0.5f, 2.0f, 0.6f, 1.5f, 0.7f });
       presetsexplosion.push_back({ "Massive Boom",     1.0f, 4.0f, 0.7f, 3.5f, 0.2f, 1.0f, 0.9f });
       presetsexplosion.push_back({ "Dusty Explosion",  0.4f, 1.5f, 1.0f, 5.0f, 0.3f, 2.0f, 0.4f });
       presetsexplosion.push_back({ "Bomb",             0.8f, 2.5f, 0.4f, 1.0f, 0.5f, 1.0f, 0.8f });
       presetsexplosion.push_back({ "Firework",         0.2f, 0.5f, 0.1f, 0.5f, 0.9f, 4.0f, 0.2f });
       presetsexplosion.push_back({ "Distant Explosion",0.9f, 4.0f, 0.0f, 0.0f, 0.1f, 1.0f, 0.0f });
       presetsexplosion.push_back({ "Grenade",          0.5f, 1.0f, 0.6f, 1.5f, 0.7f, 1.0f, 0.9f });
       presetsexplosion.push_back({ "Thud Slam",        0.8f, 0.6f, 0.0f, 0.1f, 0.0f, 1.0f, 0.5f });
       presetsexplosion.push_back({ "Sci-fi Blast",     0.3f, 1.0f, 0.0f, 0.0f, 1.0f, 5.0f, 1.0f });
       presetsexplosion.push_back({ "Big Water Splash", 0.4f, 2.0f, 0.8f, 1.0f, 0.9f, 1.5f, 0.1f });
       presetsexplosion.push_back({ "Medium detonation", 0.5f, 2.0f, 0.4f, 2.0f, 0.4f, 2.0f, 0.5f });
       presetsexplosion.push_back({ "Thud",             0.7f, 0.5f, 0.1f, 0.5f, 0.1f, 1.0f, 0.2f });
       presetsexplosion.push_back({ "Explosion",        0.6f, 2.5f, 0.5f, 2.5f, 0.5f, 2.5f, 0.5f });
    
    //Presets Fire
    //lap, hissing, crackling, intensity
        presetsfire.push_back({ "Burning Embers",  0.3f, 0.6f, 0.75f,1.0f });
        presetsfire.push_back({ "Roaring Flames",   0.8f, 0.45f, 0.15f, 1.0f});
        presetsfire.push_back({ "Ambient Fire", 0.1f, 1.0f, 0.9f, 1.0f});
        presetsfire.push_back({ "Fire Lapping",  0.5f, 0.0f, 0.0f, 1.0f });
        presetsfire.push_back({ "Candle",  0.0f, 0.6f, 0.0f, 1.0f });
        presetsfire.push_back({ "Big Fire",   0.7f, 0.25f, 0.0f, 1.0f});
        presetsfire.push_back({ "Napalm", 1.0f, 0.0f, 0.0f, 1.0f});
        presetsfire.push_back({ "Campfire",  0.08f, 0.05f, 1.0f, 0.2f });
        presetsfire.push_back({ "Fire Hiss",   0.0f, 1.0f, 0.0f, 1.0f});
        presetsfire.push_back({ "Crackle and Pop", 0.01f, 0.05f, 1.0f, 1.0f});
        presetsfire.push_back({ "Bunsen burner",  1.0f, 0.0f, 0.0f, 0.0f });
    
    //Presets Gun
    presetsgun.push_back({ "Rifle", 0.3f, 0.7f });
    presetsgun.push_back({ "Pistol",0.3f, 0.4f});
    presetsgun.push_back({ "Uzi",  0.85f, 0.15f });
    presetsgun.push_back({ "Distant Rumble", 0.4f, 0.6f});
    presetsgun.push_back({ "Machine Gun", 0.35f, 0.3f});
    presetsgun.push_back({ "Rusty Gun",0.6f, 0.5f });
    
    //Jet
    presetsjet.push_back({ "Slow Acceleration",  0.0f, 0.85f, 1.0f});
    presetsjet.push_back({ "Low Thrust",  0.1f, 0.35f, 0.4f });
    presetsjet.push_back({ "Fast Acceleration",  0.1f, 0.85f, 1.0f });
    presetsjet.push_back({ "Airplane Interior Noise", 0.21f, 0.01f, 0.04f });
    presetsjet.push_back({ "Turbine Whine",  0.3f, 0.58f, 0.0f });
    presetsjet.push_back({ "High Speed",0.9f, 0.85f, 0.4f });
    presetsjet.push_back({ "Idle Jet", 0.3f, 0.3f, 0.8f});
    
    //Helicopter
    presetshelicopter.push_back({ "Slow Helicopter",        77.0f, 0.02f, 0.5f, 0.25f, 0.65f, 0.5f, 0.3f,0.3f });
       presetshelicopter.push_back({ "Helicopter Idle",     184.0f, 0.02f, 0.5f, 0.25f,0.0f, 0.3f, 1.0f, 0.0f });
       presetshelicopter.push_back({ "Helicopter on the ground",  100.0f,0.02f,0.5f,0.25f,0.0f,0.5f,0.3f,0.28f });
       presetshelicopter.push_back({ "Fast flying helicopter",    60.0f,0.0f,0.3f,0.65f,0.01f,0.01f,0.3f,0.3f });
       presetshelicopter.push_back({ "Live camera helicopter",   59.0f,0.5f, 0.1f, 0.5f,0.2f,0.01f,0.27f,0.75f });
       presetshelicopter.push_back({ "Pilot headset",  65.0f,0.02f, 0.2f,0.25f,0.09f,0.06f,0.3f,0.47f });
       presetshelicopter.push_back({ "Distant Helicopter", 77.0f, 0.02f, 0.2f,0.25f, 0.65f,0.2f,0.3f,1.0f });
       presetshelicopter.push_back({ "Resonant",        167.67f,0.59f,0.2f,0.44f,0.65,0.79f,0.3f,0.5f });
    
    //Rocket
    presetsrocket.push_back({ "Rocket Launcher",        8.0f, 19.0f, 17.0f });
    presetsrocket.push_back({ "Quick Launch",     1.0f, 17.0f, 2.0f});
    presetsrocket.push_back({ "Distant Launch",  10.0f, 17.5f, 2.0f });
}


#endif
QAPAudioProcessor::~QAPAudioProcessor()
{
    backgroundThread.stopThread(4000);
}

void QAPAudioProcessor::stop() { transportSource.stop();
    transportSource.setPosition(0);
    //transportSource.setSource(nullptr);
}
void QAPAudioProcessor::startpause() {
    if (transportSource.isPlaying()) transportSource.stop();
    else transportSource.start();
}

//=======FAISS==========
void QAPAudioProcessor::indexLibraryFolder(const juce::File& folder)
{
    juce::MessageManager::callAsync([]() {
            juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,
                "Library Loading",
                "Starting AI scan... You can keep using the plugin while we work.",
                "OK");
        });
    
    juce::Thread::launch([this, folder]()
    {
        indexingProgress = 0.0;
        DBG("--- STARTING BACKGROUND INDEXING ---");
        {
            const std::lock_guard<std::mutex> lock(faissMutex);
            faissIndex->reset();
            libraryFiles.clear();
        }
        auto files = folder.findChildFiles(juce::File::findFiles, true, "*.wav;*.mp3");
        
        if (files.isEmpty()){indexingProgress = -1.0; return;}
        int totalFiles = files.size();
        std::atomic<int> filesProcessed { 0 };
    
        juce::ThreadPool pool (std::max (1, juce::SystemStats::getNumCpus() - 1));
        
        DBG("--- RUNNING LAPTOP PERFORMANCE TEST ---");
        int numTestFiles = std::min(10, files.size());
        int testSizes[] = { 1, 4, 8, 16 };
        optBatchSize = 1; // Start at 1

        for (int size : testSizes)
        {
            int64_t totalDuration = 0;
            optBatchSize = size; // Set the global batch size for the test

            // 2. Run the test across 10 different files
            for (int i = 0; i < numTestFiles; ++i)
            {
                auto start = juce::Time::getMillisecondCounter();
                
                runInference(files[i]);
                
                totalDuration += (juce::Time::getMillisecondCounter() - start);
            }

            // 3. Calculate average time per file for this batch size
            int avgDuration = (int)(totalDuration / numTestFiles);

            if (avgDuration < 300) {
                DBG("Batch Size " + juce::String(size) + " is stable (Avg: " + juce::String(avgDuration) + "ms)");
            } else {
                optBatchSize = std::max(1, size / 2); // Too heavy! Go back to the last safe size
                DBG("Batch Size " + juce::String(size) + " is too slow. Settling on: " + juce::String(optBatchSize));
                break;
            }
        }

        for (auto& file : files)
        {
            pool.addJob([this, file, &filesProcessed, totalFiles] {
                std::vector<float> fingerprint = runInference(file);
                
                if (!fingerprint.empty() && fingerprint.size() == embeddingSize)
                {
                    const std::lock_guard<std::mutex> lock(faissMutex);
                    faissIndex->add(1, fingerprint.data());
                    libraryFiles.add(file);
                    filesProcessed++;
                    indexingProgress = (double)filesProcessed / (double)totalFiles;
                    DBG("AI Indexed: " << file.getFileName());
                }
            });
        }

        while (pool.getNumJobs() > 0)
        {
            juce::Thread::sleep(10);
        }
        indexingProgress = -1.0;
        DBG("--- INDEXING FINISHED ---");
        DBG("Total sounds ready: " << faissIndex->ntotal);

        // 4. Muy importante: Avisamos a la interfaz que los archivos están listos
        juce::MessageManager::callAsync([this]() {
            if (auto* editor = dynamic_cast<QAPAudioProcessorEditor*>(getActiveEditor()))
            {
                editor->refreshWavFileList();
            }
        });
        int totalFound = faissIndex->ntotal;
        juce::MessageManager::callAsync([this, totalFound]() {
                    if (auto* editor = dynamic_cast<QAPAudioProcessorEditor*>(getActiveEditor()))
                        editor->refreshWavFileList();

                    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,
                        "Scan Complete",
                        "Successfully indexed " + juce::String(totalFound) + " sounds with AI.",
                        "Awesome!");
                });
    });
}

void QAPAudioProcessor::performSimilaritySearch(const juce::File& droppedFile)
{
    if (libraryFiles.isEmpty()) return;

    std::vector<float> query = runInference(droppedFile);
    if (query.empty()) return;

    int k = 10; // Number of results
    std::vector<float> distances(k);
    std::vector<faiss::idx_t> indices(k);

    // FAISS SEARCH
    faissIndex->search(1, query.data(), k, distances.data(), indices.data());

    similarFilesResults.clear();
    for (int i = 0; i < k; ++i)
    {
        int idx = (int)indices[i];
        if (idx >= 0 && idx < libraryFiles.size())
        {
            similarFilesResults.add(libraryFiles[idx]);
        }
    }
}
const juce::String QAPAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool QAPAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool QAPAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool QAPAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double QAPAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int QAPAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int QAPAudioProcessor::getCurrentProgram()
{
    return 0;
}

void QAPAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String QAPAudioProcessor::getProgramName (int index)
{
    return {};
}

void QAPAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================


#ifndef JucePlugin_PreferredChannelConfigurations
bool QAPAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif
// Inference:
std::vector<float> QAPAudioProcessor::runInference(const juce::File& file)
{
    if (onnxSession == nullptr) return {};

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (!reader || reader->lengthInSamples == 0) return {};

    // 1. Setup Slicing Parameters
    const int windowSize = 32768;      // The size the AI expects (0.7s)
    const int hopSize = windowSize / 2; // 50% overlap for better accuracy
    
    // This will hold the "running sum" of our embeddings
    std::vector<float> averagedEmbedding(embeddingSize, 0.0f);
    int windowCount = 0;

    juce::AudioBuffer<float> tempReadBuffer(1, windowSize);
    auto memoryInfo = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
    std::vector<int64_t> inputShape = { 1, (int64_t)windowSize };

    // 2. The Sliding Window Loop
    // We move startSample by the 'hopSize' each time
    for (int64_t startSample = 0;
         startSample + windowSize <= reader->lengthInSamples;
         startSample += hopSize)
    {
        tempReadBuffer.clear();
        reader->read(&tempReadBuffer, 0, windowSize, startSample, true, false);

        try {
            // Prepare the current slice for ONNX
            Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
                memoryInfo,
                (float*)tempReadBuffer.getReadPointer(0),
                windowSize,
                inputShape.data(),
                inputShape.size());

            const char* inputNames[] = { "waveform" };
            const char* outputNames[] = { "embedding" };

            auto outputTensors = onnxSession->Run(Ort::RunOptions{nullptr}, inputNames, &inputTensor, 1, outputNames, 1);
            float* floatArr = outputTensors[0].GetTensorMutableData<float>();

            // 3. Accumulate the results
            // We are adding this slice's 960 values to our total
            for (int i = 0; i < embeddingSize; ++i) {
                averagedEmbedding[i] += floatArr[i];
            }
            windowCount++;

            // Pilot Safety: If the file is huge (e.g. 1 minute), don't process forever.
            // Limit to roughly 5 seconds of audio (approx 14 windows at 50% overlap)
            if (windowCount >= 14) break;
        }
        catch (const std::exception& e) {
            DBG("Inference Slice Error: " << e.what());
            break;
        }
    }

    // 4. Final Averaging
    if (windowCount > 0) {
        for (int i = 0; i < embeddingSize; ++i) {
            averagedEmbedding[i] /= static_cast<float>(windowCount);
        }
    } else {
        // Fallback: if file is shorter than windowSize, just return empty or 0.7s of silence
        return std::vector<float>(embeddingSize, 0.0f);
    }

    return averagedEmbedding;
}

void QAPAudioProcessor::triggerHelicopter()
{
    if (HelicopterModel)
{
    if (HelicopterModel->isActive())
    {
        HelicopterModel->stop();
    }
    else
    {
        HelicopterModel->start();
    }
}
}
void QAPAudioProcessor::triggerJet()
{
    
    if (JetModel)
    {
        if (JetModel->isActive())
        {
            JetModel->stop();
        }
        else
        {
            JetModel->start();
        }
    }
        
}


void QAPAudioProcessor::triggerGun()
{
    float newShellFreq=0.0f;
    float newShellFreqDecay=0.0f;
    auto* ShellFreqParam = parameters.getRawParameterValue("shellfreq");
    newShellFreq=ShellFreqParam->load();
    gunModel->setShellFreq(newShellFreq);
    auto* ShellFreqDecayParam = parameters.getRawParameterValue("shellfreqdecay");
    newShellFreqDecay=ShellFreqDecayParam->load();
    gunModel->setShellDecay(newShellFreqDecay);
    gunModel->trigger();
    
}


void QAPAudioProcessor::triggerFire()
{
    if (fireModel->isActive()){fireModel->stop();}
      else{fireModel->start();
    }
}

void QAPAudioProcessor::triggerExplosion()
{
    float newRumble=0.0f;
    float newRumbleDecay=0.0f;
    float newAir=0.0f;
    float newAirDecay=0.0f;
    float newDust=0.0f;
    float newDustDecay=0.0f;
    float newGritAmount=0.0f;
    if (explosionModel)
       {
           auto* rumbleParam = parameters.getRawParameterValue("rumble");
           newRumble=rumbleParam->load();
           explosionModel->setRumble(newRumble);
           auto* rumbleDecayParam=parameters.getRawParameterValue("rumbleDecay");
           newRumbleDecay=rumbleDecayParam->load();
           explosionModel->setRumbleDecay(newRumbleDecay);
           auto* airParam=parameters.getRawParameterValue("air");
           newAir=airParam->load();
           explosionModel->setAir(newAir);
           auto* airDecayParam=parameters.getRawParameterValue("airDecay");
           newAirDecay=airDecayParam->load();
            explosionModel->setAirDecay(newAirDecay);
           auto* dustParam=parameters.getRawParameterValue("dust");
           newDust=dustParam->load();
           explosionModel->setDust(newDust);
           auto* dustDecayParam=parameters.getRawParameterValue("dustDecay");
           newDustDecay=dustDecayParam->load();
           explosionModel->setDustDecay(newDustDecay);
                   
           explosionModel->setTimeSeparation(0.0f);
           explosionModel->setGrit(true);
           auto* gritAmountParam=parameters.getRawParameterValue("gritAmount");
           newGritAmount=gritAmountParam->load();
           explosionModel->setGritAmount(newGritAmount);
           explosionModel->setOverTheTop(true);
           explosionModel->trigger();
       }
}


void QAPAudioProcessor::triggerRocket()
{
    if (RocketModel)
           {
               float newDuration=0.0f;
               float newChamberResonance=0.0f;
               float newFlutter=0.0f;
               
               RocketModel->trigger();
               auto* DurationParam= parameters.getRawParameterValue("duration");
               newDuration=DurationParam->load();
               RocketModel->setDuration(newDuration);
               auto* ChamberResonanceParam=parameters.getRawParameterValue("chamberresonance");
               newChamberResonance=ChamberResonanceParam->load();
               RocketModel->setChamberResonance(newChamberResonance);
               auto* FlutterParam=parameters.getRawParameterValue("flutter");
               newFlutter=FlutterParam->load();
               RocketModel->setFlutter(newFlutter);
               
           }
    
}

void QAPAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    transportSource.prepareToPlay(samplesPerBlock, sampleRate);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate=sampleRate;
    spec.maximumBlockSize=(juce::uint32)samplesPerBlock;
    spec.numChannels=(juce::uint32)getTotalNumOutputChannels();
    
    juce::dsp::Reverb::Parameters reverbParams;
    reverbParams.roomSize = 0.6f;
    reverbParams.damping = 0.4f;
    reverbParams.wetLevel = 0.3f;
    reverbParams.dryLevel = 0.7f;
    reverb.setParameters(reverbParams);

    // Compressor setup
    compressor.setThreshold(-12.0f);
    compressor.setRatio(3.0f);
    compressor.setAttack(10.0f);
    compressor.setRelease(100.0f);
    
    //Filters
      processors.clear();
      for(int i=0;i<getTotalNumOutputChannels();++i)
      {
          processors.add(new ChannelFilterChain());
          processors.getLast()->prepare(spec);
      }
    
    if (explosionModel)
    {
        explosionModel->initialize((float) sampleRate);
    }
    
    if (fireModel)
    {
        fireModel->initialize((float) sampleRate);
    }
    if (gunModel)
    {
        gunModel->initialize((float) sampleRate);
    }
    if (JetModel)
    {
        JetModel->initialize((float) sampleRate);
    }
    if (HelicopterModel)
    {
        HelicopterModel->initialize((float) sampleRate);
    }
    if (RocketModel)
    {
        RocketModel->initialize((float) sampleRate);
    }
    reverb.prepare(spec);
    compressor.prepare(spec);
    //eqChain.prepare(spec);
}

void QAPAudioProcessor::releaseResources()
{
    transportSource.releaseResources();
    processors.clear();
}

void QAPAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    float sampleRate = (float)juce::AudioProcessor::getSampleRate();
    buffer.clear();
    
    if (PlayAssistantSound.load())
        {
            transportSource.stop();         // Stop any current ping
            transportSource.setPosition(0); // Rewind immediately
            transportSource.start();        // Start the new ping
            PlayAssistantSound.store(false);
        }
    if (transportSource.isPlaying())
    {
        juce::AudioSourceChannelInfo info(buffer);
        transportSource.getNextAudioBlock(info);
        
        float crossfade = *parameters.getRawParameterValue("crossfade");
        float sampleGain = 1.0f - crossfade;  // 0 = all synth, 1 = all sample
        buffer.applyGain(sampleGain);
    }
    //Filters coefficient
    auto lowCoefficients = juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate, 200.0f, 0.707f, 2.0f);
    auto midCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, 1000.0f, 0.707f, 2.0f);
    auto highCoefficients = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, 3000.0f, 2.0f,2.0f);
        
    
    float newLapping = *parameters.getRawParameterValue("lapping");
    float newHissing = *parameters.getRawParameterValue("hissing");
    float newCrackling = *parameters.getRawParameterValue("crackling");
    float newIntensity = *parameters.getRawParameterValue("intensity");
    //helicopter
    float newRotorPeriod = *parameters.getRawParameterValue("rotorPeriod");
    float newPeriod = *parameters.getRawParameterValue("period");
    float newTailMix = *parameters.getRawParameterValue("tailMix");
    float newBaseFreq = *parameters.getRawParameterValue("baseFreq");
    float newRotorMix = *parameters.getRawParameterValue("rotorMix");
    float newEngineMix = *parameters.getRawParameterValue("engineMix");
    float newBladeNoise = *parameters.getRawParameterValue("bladeNoise");
    float newEngineSpeed = *parameters.getRawParameterValue("engineSpeed");
    
    if (explosionModel->isActive())
        {
            juce::AudioBuffer<float> proceduralBuffer(buffer.getNumChannels(), buffer.getNumSamples());
            proceduralBuffer.clear();
            explosionModel->fillBuffer(const_cast<float**>(proceduralBuffer.getArrayOfWritePointers()), proceduralBuffer.getNumSamples());
            juce::dsp::AudioBlock<float> block(proceduralBuffer);
            juce::dsp::ProcessContextReplacing<float> context(block);
            for (size_t channel = 0; channel < block.getNumChannels(); ++channel)
                    {
                        auto& chain = *processors[(int)channel];
                       

                        chain.get<0>().coefficients=lowCoefficients;
                        chain.get<1>().coefficients=midCoefficients;
                        chain.get<2>().coefficients=highCoefficients;
                        
                        auto channelBlock = block.getSingleChannelBlock ((int)channel);
                        auto context = juce::dsp::ProcessContextReplacing<float>{ channelBlock };
                        chain.process(context);
                        reverb.process(context);
                        compressor.process(context);
                    }
            float modelGain = *parameters.getRawParameterValue("crossfade");
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                buffer.addFrom(ch, 0, proceduralBuffer, ch, 0, proceduralBuffer.getNumSamples(), modelGain);
        }
        
        fireModel->setLapping(newLapping);
        fireModel->setHissing(newHissing);
        fireModel->setCrackling(newCrackling);
        fireModel->setIntensity(newIntensity);
    
        if (fireModel->isActive())
        {
            juce::AudioBuffer<float> proceduralBuffer(buffer.getNumChannels(), buffer.getNumSamples());
            proceduralBuffer.clear();
            fireModel->fillBuffer(const_cast<float**>(proceduralBuffer.getArrayOfWritePointers()), proceduralBuffer.getNumSamples());
            juce::dsp::AudioBlock<float> block(proceduralBuffer);
            juce::dsp::ProcessContextReplacing<float> context(block);
            for (size_t channel = 0; channel < block.getNumChannels(); ++channel)
                    {
                        auto& chain = *processors[(int)channel];
                       

                        chain.get<0>().coefficients=lowCoefficients;
                        chain.get<1>().coefficients=midCoefficients;
                        chain.get<2>().coefficients=highCoefficients;
                        
                        auto channelBlock = block.getSingleChannelBlock ((int)channel);
                        auto context = juce::dsp::ProcessContextReplacing<float>{ channelBlock };
                        chain.process(context);
                        reverb.process(context);
                        compressor.process(context);
                    }
            
            float modelGain = *parameters.getRawParameterValue("crossfade");
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                buffer.addFrom(ch, 0, proceduralBuffer, ch, 0, proceduralBuffer.getNumSamples(), modelGain);

        }
    
        if (gunModel->isActive())
        {
            juce::AudioBuffer<float> proceduralBuffer(buffer.getNumChannels(), buffer.getNumSamples());
            proceduralBuffer.clear();
            gunModel->fillBuffer(const_cast<float**>(proceduralBuffer.getArrayOfWritePointers()), proceduralBuffer.getNumSamples());
            juce::dsp::AudioBlock<float> block(proceduralBuffer);
            juce::dsp::ProcessContextReplacing<float> context(block);
            for (size_t channel = 0; channel < block.getNumChannels(); ++channel)
                    {
                        auto& chain = *processors[(int)channel];
                       

                        chain.get<0>().coefficients=lowCoefficients;
                        chain.get<1>().coefficients=midCoefficients;
                        chain.get<2>().coefficients=highCoefficients;
                        
                        auto channelBlock = block.getSingleChannelBlock ((int)channel);
                        auto context = juce::dsp::ProcessContextReplacing<float>{ channelBlock };
                        chain.process(context);
                        reverb.process(context);
                        compressor.process(context);
                    }
            
            float modelGain = *parameters.getRawParameterValue("crossfade");
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                buffer.addFrom(ch, 0, proceduralBuffer, ch, 0, proceduralBuffer.getNumSamples(), modelGain);

        }
    float newSpeed= 0.0f;
    float newTurbine=0.0f;
    float newBurn=0.0f;
    
    
    if (JetModel->isActive())
    {
        juce::AudioBuffer<float> proceduralBuffer(buffer.getNumChannels(), buffer.getNumSamples());
        proceduralBuffer.clear();
        auto* SpeedParam = parameters.getRawParameterValue("speed");
        newSpeed=SpeedParam->load();
        JetModel->setSpeed(newSpeed);
       auto* TurbineParam = parameters.getRawParameterValue("turbine");
        newTurbine=TurbineParam->load();
        JetModel->setTurbine(newTurbine);
        auto* BurnParam = parameters.getRawParameterValue("burn");
        newBurn=BurnParam->load();
        JetModel-> setBurn(newBurn);
        JetModel->fillBuffer(const_cast<float**>(proceduralBuffer.getArrayOfWritePointers()), proceduralBuffer.getNumSamples());
        juce::dsp::AudioBlock<float> block(proceduralBuffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        for (size_t channel = 0; channel < block.getNumChannels(); ++channel)
                {
                    auto& chain = *processors[(int)channel];
                   

                    chain.get<0>().coefficients=lowCoefficients;
                    chain.get<1>().coefficients=midCoefficients;
                    chain.get<2>().coefficients=highCoefficients;
                    
                    auto channelBlock = block.getSingleChannelBlock ((int)channel);
                    auto context = juce::dsp::ProcessContextReplacing<float>{ channelBlock };
                    chain.process(context);
                    reverb.process(context);
                    compressor.process(context);
                }
        
        float modelGain = *parameters.getRawParameterValue("crossfade");
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.addFrom(ch, 0, proceduralBuffer, ch, 0, proceduralBuffer.getNumSamples(), modelGain);
       // buffer.addFrom(0, 0, proceduralBuffer, 0, 0, proceduralBuffer.getNumSamples());
    }
    
    HelicopterModel->setRotorPeriod(newRotorPeriod);
    HelicopterModel->setResonance(newPeriod);
    HelicopterModel->setTailMix(newTailMix);
    HelicopterModel->setBaseFreq(newBaseFreq);
    HelicopterModel->setRotorMix(newRotorMix);
    HelicopterModel->setEngineMix(newEngineMix);
    HelicopterModel->setBladeNoise(newBladeNoise);
    HelicopterModel->setEngineSpeed(newEngineSpeed);

    if (HelicopterModel->isActive())
    {
        juce::AudioBuffer<float> proceduralBuffer(buffer.getNumChannels(), buffer.getNumSamples());
        proceduralBuffer.clear();
        HelicopterModel->fillBuffer(const_cast<float**>(proceduralBuffer.getArrayOfWritePointers()), proceduralBuffer.getNumSamples());
        juce::dsp::AudioBlock<float> block(proceduralBuffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        for (size_t channel = 0; channel < block.getNumChannels(); ++channel)
                {
                    auto& chain = *processors[(int)channel];
                   

                    chain.get<0>().coefficients=lowCoefficients;
                    chain.get<1>().coefficients=midCoefficients;
                    chain.get<2>().coefficients=highCoefficients;
                    
                    auto channelBlock = block.getSingleChannelBlock ((int)channel);
                    auto context = juce::dsp::ProcessContextReplacing<float>{ channelBlock };
                    chain.process(context);
                    reverb.process(context);
                    compressor.process(context);
                }
        
        float modelGain = *parameters.getRawParameterValue("crossfade");
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.addFrom(ch, 0, proceduralBuffer, ch, 0, proceduralBuffer.getNumSamples(), modelGain);
    }
    if (RocketModel->isActive())
    {
        juce::AudioBuffer<float> proceduralBuffer(buffer.getNumChannels(), buffer.getNumSamples());
        proceduralBuffer.clear();
        RocketModel->fillBuffer(const_cast<float**>(proceduralBuffer.getArrayOfWritePointers()), proceduralBuffer.getNumSamples());
        juce::dsp::AudioBlock<float> block(proceduralBuffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        for (size_t channel = 0; channel < block.getNumChannels(); ++channel)
                {
                    auto& chain = *processors[(int)channel];
                   

                    chain.get<0>().coefficients=lowCoefficients;
                    chain.get<1>().coefficients=midCoefficients;
                    chain.get<2>().coefficients=highCoefficients;
                    
                    auto channelBlock = block.getSingleChannelBlock ((int)channel);
                    auto context = juce::dsp::ProcessContextReplacing<float>{ channelBlock };
                    chain.process(context);
                    reverb.process(context);
                    compressor.process(context);
                }
        
        float modelGain = *parameters.getRawParameterValue("crossfade");
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.addFrom(ch, 0, proceduralBuffer, ch, 0, proceduralBuffer.getNumSamples(), modelGain);
    }
    
    //MAin Reverb
    {
            float reverbMix = *parameters.getRawParameterValue("reverbMix");
            juce::AudioBuffer<float> dryCopy;
            dryCopy.makeCopyOf(buffer);

            juce::dsp::AudioBlock<float> block(buffer);
            juce::dsp::ProcessContextReplacing<float> ctx(block);
            reverb.process(ctx);

            // Blend dry + wet
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                buffer.applyGain(ch, 0, buffer.getNumSamples(), reverbMix);
                buffer.addFrom(ch, 0, dryCopy, ch, 0, buffer.getNumSamples(), 1.0f - reverbMix);
            }
        }
    float masterGain = *parameters.getRawParameterValue("masterGain");
        buffer.applyGain(masterGain);
    if (proceduralVisualiser != nullptr)
        proceduralVisualiser->pushBuffer(buffer);
    

    if (auto* writer = activeWriter.load())
    {
        writer->write(buffer.getArrayOfReadPointers(), buffer.getNumSamples());
    }



}


void QAPAudioProcessor::startRecording()
{
    
    juce::File outputDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
    juce::File outputFile = outputDir.getChildFile("QAP_recording_" + juce::String(juce::Time::getCurrentTime().toMilliseconds()) + ".wav");
    SavedFile = outputFile.getFileName();
    fileStream = outputFile.createOutputStream();
    if (fileStream == nullptr)
    {
        juce::Logger::writeToLog("ERROR: Couldn't open file for writing.");
        return;
    }

    auto newWriter = juce::WavAudioFormat().createWriterFor(
        fileStream.get(),
        getSampleRate(),
        getTotalNumOutputChannels(),
        16,
        {},
        0
    );

    if (newWriter == nullptr)
    {
        juce::Logger::writeToLog("ERROR: Couldn't create WAV writer.");
        return;
    }

    // Transfer ownership
    fileStream.release();
    threadedWriter = std::make_unique<juce::AudioFormatWriter::ThreadedWriter>(newWriter, backgroundThread, 32768);
    activeWriter.store(threadedWriter.get());
    isRecording = true;
    juce::Logger::writeToLog("Recording started: " + outputFile.getFullPathName());
}




void QAPAudioProcessor::stopRecording()
{
    isRecording = false;
    activeWriter.store(nullptr);
    threadedWriter.reset();
    writer.reset();
    fileStream.reset();
    juce::String fileName = SavedFile;
    juce::Logger::writeToLog("Recording stopped.");
    juce::MessageManager::callAsync([fileName]() {
            juce::AlertWindow::showMessageBoxAsync(
                juce::MessageBoxIconType::InfoIcon,
                "Recording Saved",
                "Your sample has been saved as: " + fileName,
                "Great!");
        });
}


void QAPAudioProcessorEditor::chooseLibraryFolder()
{
    folderChooser = std::make_unique<juce::FileChooser>(
        "Select your sound library folder...",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        ""
    );

    const int flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories;
    
    folderChooser->launchAsync(flags, [this](const juce::FileChooser& fc) {
        juce::File selectedFolder = fc.getResult();

        if (selectedFolder.isDirectory())
        {
            audioProcessor.loadAllWavFilesFromFolder(selectedFolder);
            auto result = fc.getResult();
            if (result.isDirectory())
            {
                audioProcessor.indexLibraryFolder(result);
                refreshWavFileList();
            }
        }
        folderChooser = nullptr;
    });
}


void QAPAudioProcessor::loadAllWavFilesFromFolder(const juce::File &folder)
{
    juce::Array<juce::File> wavFiles;
    folder.findChildFiles(wavFiles, juce::File::findFiles, true, "*.wav");
    wavFileNames.clear();
    wavFilePaths.clear();
    

    for (auto& file : wavFiles)
    {
        DBG("Found WAV: " + file.getFileName());
        wavFilePaths.add(file);
        wavFileNames.add(file.getFileName());
    }
    if (auto* editor = dynamic_cast<QAPAudioProcessorEditor*>(getActiveEditor()))
    {
        editor->refreshWavFileList();
    }
}

juce::File QAPAudioProcessor::getWavFileByName(const juce::String& name) const
{
    for (int i = 0; i < wavFilePaths.size(); ++i)
    {
        if (wavFilePaths[i].getFileName() == name)
            return wavFilePaths[i];
    }
    return {};
}

void QAPAudioProcessor::playWavFileByName(const juce::String& name)
{
    auto file = getWavFileByName(name);

    if (!file.existsAsFile())
        return;

    auto* reader = formatManager.createReaderFor(file);

    if (reader != nullptr)
    {
        std::unique_ptr<juce::AudioFormatReaderSource> newSource(new juce::AudioFormatReaderSource(reader, true));
        transportSource.stop();
        transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);
        readerSource = std::move(newSource);
        bool shouldLoop = *parameters.getRawParameterValue("loop") > 0.5f;
        readerSource->setLooping(shouldLoop);
        transportSource.start();
    }
    if (auto* editor = dynamic_cast<QAPAudioProcessorEditor*>(getActiveEditor()))
        {
            editor->loadThumbnail(file);
        }
}

void QAPAudioProcessor::LoadDroppedFile (const juce::File& file)
{
    transportSource.stop();
    transportSource.setSource(nullptr);

    auto* reader = formatManager.createReaderFor(file);

    if (reader != nullptr)
    {
        
        auto newSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
        transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);
        readerSource.reset(newSource.release());
        transportSource.setPosition(0);
        transportSource.start();
    }
}

bool QAPAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* QAPAudioProcessor::createEditor()
{
    return new QAPAudioProcessorEditor (*this);
}


//==============================================================================
void QAPAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void QAPAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new QAPAudioProcessor();
}

void QAPAudioProcessor::loadPresetFire(int index)
{
    if (index >= 0 && index < presetsfire.size())
    {
        const auto& p = presetsfire[index];

        auto setParam = [&](juce::String id, float val) {
            if (auto* param = parameters.getParameter(id))
            {
                auto range = parameters.getParameterRange(id);
                param->setValueNotifyingHost(range.convertTo0to1(val));
            }
        };

        setParam("lapping",   p.lapping);
        setParam("hissing",   p.hissing);
        setParam("crackling", p.crackling);
        setParam("intensity", p.intensity);
    }
}


void QAPAudioProcessor::loadPresetGun(int index)
{
    if (index >= 0 && index < presetsgun.size())
    {
        const auto& p = presetsgun[index];

        auto setParam = [&](juce::String id, float val) {
            auto* param = parameters.getParameter(id);
            if (param)
            {
               
                auto range = parameters.getParameterRange(id);
                param->setValueNotifyingHost(val);
            }
        };

        setParam("shellfreq",      p.shellFrequency);
        setParam("shellfreqdecay", p.shellFrequencyDecay);
    }
}

void QAPAudioProcessor::loadPresetExplosion(int index)
{
    if (index >= 0 && index < presetsexplosion.size())
    {
        const auto& p = presetsexplosion[index];

        auto setParam = [&](juce::String id, float val) {
            auto* param = parameters.getParameter(id);
            if (param)
            {
               
                auto range = parameters.getParameterRange(id);
                param->setValueNotifyingHost(val);
            }
        };

        setParam("rumble",      p.rumble);
        setParam("rumbleDecay", p.rumbleDecay);
        setParam("dust",        p.dust);
        setParam("dustDecay",   p.dustDecay);
        setParam("air",         p.air);
        setParam("airDecay",    p.airDecay);
        setParam("gritAmount",  p.grit);
    }
}
void QAPAudioProcessor::loadPresetJet(int index)
{
    if (index >= 0 && index < presetsjet.size())
    {
        const auto& p = presetsjet[index];

        auto setParam = [&](juce::String id, float val) {
            auto* param = parameters.getParameter(id);
            if (param)
            {auto range = parameters.getParameterRange(id);
            param->setValueNotifyingHost(val);
            }
        };

        setParam("speed",      p.speed);
        setParam("turbine", p.turbine);
        setParam("burn",        p.burn);
       
    }
}

void QAPAudioProcessor::loadPresetHelicopter(int index)
{
    if (index >= 0 && index < presetshelicopter.size())
    {
        const auto& p = presetshelicopter[index];

        auto setParam = [&](juce::String id, float val) {
            auto* param = parameters.getParameter(id);
            if (param)
            {
               
                auto range = parameters.getParameterRange(id);
                param->setValueNotifyingHost(val);
            }
        };

        setParam("rotorPeriod",      p.rotorPeriod);
        setParam("period", p.period);
        setParam("tailMix",        p.tailMix);
        setParam("baseFreq",   p.baseFreq);
        setParam("rotorMix",         p.rotorMix);
        setParam("engineMix",    p.engineMix);
        setParam("bladeNoise",  p.bladeNoise);
        setParam("engineSpeed",  p.engineSpeed);
    }
}


void QAPAudioProcessor::loadPresetRocket(int index)
{
    if (index >= 0 && index < presetsrocket.size())
    {
        const auto& p = presetsrocket[index];
        
        auto setParam = [&](juce::String id, float val) {
            auto* param = parameters.getParameter(id);
            if (param)
            {
                
                auto range = parameters.getParameterRange(id);
                param->setValueNotifyingHost(val);
            }
        };
        
        setParam("duration",      p.duration);
        setParam("chamberResonance", p.chamberResonance);
        setParam("flutter",        p.flutter);
        
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout QAPAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;
    //Mix.. More sample or more procedural model?
    parameters.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "loop", 1 }, "Loop",false));
    // 4 PARAMETERS FOR POST PRODUCTION.
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"crossfade", 1}, "Sample/Synth Mix", 0.0f, 1.0f, 0.5f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"reverbMix", 1}, "Reverb Mix", 0.0f, 1.0f, 0.3f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"masterGain", 1}, "Master Gain", 0.0f, 1.0f, 0.8f));
    //Explosion
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"rumble", 1},"Rumble", 0.1f, 1.0f, 0.5f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"rumbleDecay",1},"Rumble Decay", 0.5f, 4.0f, 4.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"dust",1}, "Dust", 0.0f, 1.0f, 0.5f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"dustDecay",1}, "Dust Decay", 0.0f, 5.0f, 1.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"air",1}, "Air", 0.0f, 1.0f, 0.5f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"airDecay",1}, "Air Decay", 1.0f, 5.0f, 1.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"gritAmount",1}, "Grit Amount", 0.0f, 1.0f, 0.5f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"timeSeparation",1}, "Time Separation", 0.0f, 1.0f, 0.5f));
    //Fire
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"lapping", 1},"Lapping", 0.0f, 1.0f, 0.5f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"hissing", 1},"Hissing", 0.0f, 1.0f, 0.5f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"crackling", 1},"Crackling", 0.0f, 1.0f, 0.5f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"intensity", 1},"Intensity", 0.0f, 1.0f, 0.5f));
    //Gun
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"shellfreq", 1},"Shell Frequency", 0.1f,1.0f, 0.5f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"shellfreqdecay",1},"Shell Frequency Decay", 0.5f, 1.0f, 0.1f));
    
    //Jet
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"speed", 1},"Speed", 0.1f, 1.0f, 0.5f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"turbine",1},"Turbine", 0.5f, 1.0f, 0.1f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"burn",1},"Burn", 0.5f, 1.0f, 0.1f));
    
    //Helicopter
    
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"rotorPeriod", 1},"Rotor Period", 10.0f, 210.0f, 77.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"period",1},"Period", 0.5f, 1.0f, 0.1f));
     parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"tailMix",1},"Tail Mix", 0.0f,1.0f, 0.2f));
     parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"baseFreq",1},"Base Frequency", 0.0f,1.0f, 0.25f));
     parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"rotorMix",1},"Rotor Mix", 0.0f,1.0f, 0.65f));
     parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"engineMix",1},"Engine Mix", 0.0f,1.0f, 0.2f));
     parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"bladeNoise",1},"Blade Noise", 0.0f,1.0f, 0.3f));
     parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"engineSpeed",1},"Engine Speed", 0.0f,1.0f, 0.5f));
    
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"duration", 1},"duration", 1.0f, 10.0f, 5.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"chamberresonance",1},"Chamber Resonance", 16.0f, 20.0f, 19.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"flutter",1},"Flutter", 2.0f, 40.0f, 30.0f));
   
    return { parameters.begin(), parameters.end() };
}
