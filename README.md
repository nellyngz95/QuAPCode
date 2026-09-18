# QuAP — Quality Audio Prototyping

A VST3 plugin that unifies content-based audio retrieval, procedural sound generation, and intelligent parameter guidance within a single interface for professional 
sound designers.

## Paper

Garcia-Sihuay, N., Reiss, J. (2026). *Quality Audio Prototyping (QuAP): Unifying Retrieval, Synthesis and Intelligent Guidance for Sound Design*. Proceedings of DAFx 2026, MIT, Boston, USA.

📄 [https://dafx.de/paper-archive/2026/papers/DAFx26_paper_53.pdf)  
🌐 [Project website](https://quap.netlify.app)  
📦 [6KSFX Dataset](https://arxiv.org/abs/2501.17198)

## Overview

QuAP addresses two primary bottlenecks in professional sound design workflows:

1. **Sample retrieval** — finding acoustically similar  sounds in large libraries
2. **Sound variation** — creating new sounds from procedural models without deep synthesis knowledge

### Features

- **Hybrid retrieval** — text search and drag-and-drop acoustic similarity search using MobileNetV3 embeddings 
  and FAISS indexing
- **Procedural audio engine** — six embedded synthesis models (Fire, Explosion, Helicopter, Rocket, Gun, Jet) 
  optimised through perceptual evaluation
- **Intelligent guidance** — embedded parameter assistant providing empirically validated ranges derived from MUSHRA listening tests with 20 audio professionals
- **Mixing stage** — layer retrieved and synthesised  audio within a single interface

## Dependencies

- [JUCE](https://juce.com) (version 7.0+)
- [ONNX Runtime](https://onnxruntime.ai) (version 1.16+)
- [FAISS](https://github.com/facebookresearch/faiss) (version 1.7+)
- MobileNetV3 model (included in BinaryData)

## Building

### Prerequisites

1. Install JUCE and set up Projucer
2. Install ONNX Runtime
3. Install FAISS

### Steps

1. Clone the repository:
```bash
git clone https://github.com/nellyngz/QuAP.git
cd QuAP
```

2. Open `QuAP.jucer` in Projucer

3. Set your JUCE path and export to your IDE 
   (Xcode / Visual Studio)

4. Build the VST3 target

### Tested on
- macOS 13+ (Apple Silicon and Intel)
- Windows 10/11

## Dataset

The 6KSFX dataset used to train and evaluate QuAP 
is publicly available:

📦 [6KSFX on Zenodo](https://zenodo.org/record/...)  
📄 [arXiv:2501.17198](https://arxiv.org/abs/2501.17198)

6,000 procedurally synthesised sound effect samples 
across 30 categories, paired with recorded references.

## Evaluation

QuAP was evaluated with 16 audio professionals through 
a cognitive walkthrough and thematic analysis:

- **75%** found QuAP useful and beneficial to their 
  workflows
- **100%** valued the parameter assistant for 
  maintaining creative agency
- Statistically significant perceptual improvements 
  in **5/6** embedded synthesis models (MUSHRA, n=20)

## Citation

```bibtex
@inproceedings{garcia2026quap,
  title={Quality Audio Prototyping (QuAP): Unifying 
  Retrieval, Synthesis and Intelligent Guidance 
  for Sound Design},
  author={Garcia-Sihuay, Nelly and Reiss, Joshua},
  booktitle={Proceedings of the International 
  Conference on Digital Audio Effects (DAFx)},
  year={2026}
}
```
## Procedural Audio Models

The procedural audio models used in QuAP were developed in collaboration with [Nemisindo](https://www.nemisindo.com) and are 
not included in this repository. To build the full version of QuAP, please contact nellyvags@gmail.com for access to the models.

## License

MIT License — see [LICENSE](LICENSE) for details.

## Contact

Nelly Garcia Sihuay  
📧 nellyvags@gmail.com  
🌐 [nellyngz.netlify.app](https://nellyngz.netlify.app)  
Centre for Digital Music, Queen Mary University of London
