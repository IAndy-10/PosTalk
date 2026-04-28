# PosTalk — System Setup

## Overview

PosTalk runs entirely on a single computer. No internet connection is required during performance. The camera feed is processed locally inside the plugin and the resulting gesture data drives DAW parameters in real time.

---

## Components

### Hardware
| Device | Role |
|--------|------|
| Computer | Runs Ableton + PosTalk VST3 |
| Built-in Webcam | Captures hand gestures |
| Audio Interface (USB) | Sends audio to speakers |
| Speakers | Audio output |

### Software
| Component | Role |
|-----------|------|
| Ableton | Audio host |
| PosTalk (VST3) | Gesture tracking and parameter control |

### Internet
Not required at runtime. Only needed to install the plugin.

### Power
| Device | Power Source |
|--------|-------------|
| Computer | Charger (wall outlet) |
| Audio Interface | USB from computer |
| Speakers | Wall outlet |

---

## System Diagram

```dot
digraph PosTalk {
    rankdir=TB;
    fontname="Helvetica";
    node [fontname="Helvetica", fontsize=11, style=filled, rounded=true, shape=box];
    edge [fontname="Helvetica", fontsize=10];

    // Power
    wall [label="Wall Outlet", fillcolor="#FFF3CD", color="#E6B800"];
    charger [label="Charger", fillcolor="#FFF3CD", color="#E6B800"];

    // Hardware
    computer [label="Computer\n(Ableton + PosTalk VST3)", fillcolor="#D6E4FF", color="#4A6CF7"];
    camera [label="Built-in Webcam", fillcolor="#D6E4FF", color="#4A6CF7"];
    audio_if [label="Audio Interface\n(USB)", fillcolor="#D6E4FF", color="#4A6CF7"];
    speakers [label="Speakers", fillcolor="#D6E4FF", color="#4A6CF7"];

    // Performer
    performer [label="Performer", fillcolor="#F9E4FF", color="#9B59B6", shape=ellipse];

    // Power connections
    wall -> charger [label="AC", color="#E6B800", style=dashed];
    charger -> computer [label="power", color="#E6B800", style=dashed];
    wall -> speakers [label="AC power", color="#E6B800", style=dashed];
    computer -> audio_if [label="USB power", color="#E6B800", style=dashed];

    // Data connections
    performer -> camera [label="hand gestures", color="#9B59B6", fontcolor="#9B59B6"];
    camera -> computer [label="video stream\n(built-in)", color="#4A6CF7"];
    computer -> audio_if [label="USB (audio out)", color="#4A6CF7"];
    audio_if -> speakers [label="analog out", color="#4A6CF7"];
}
```

---

## Signal Flow Summary

```
PERFORMER
    |
    | (hand gestures)
    v
[Built-in Webcam] ──► [Computer: Ableton + PosTalk VST3]
                                      |
                              [Audio Interface]
                                      |
                                 [Speakers]
                                      |
                                    SOUND
```

No external servers. No cloud processing. Everything runs locally on the performer's machine.
