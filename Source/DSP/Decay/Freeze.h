#pragma once

// Freeze mode helper. When active, the FDNReverb disables damping and
// maximizes feedback to sustain the current reverb tail indefinitely.
// The actual freeze logic is applied inside FDNReverb; this is a state carrier.
class Freeze {
public:
    void setActive(bool active) { frozen = active; }
    bool isActive() const { return frozen; }

private:
    bool frozen = false;
};

