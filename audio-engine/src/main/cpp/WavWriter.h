#pragma once

#include <cstdint>
#include <fstream>
#include <string>

class WavWriter {
public:

    WavWriter();

    ~WavWriter();

    bool open(
        const std::string& path,
        int sampleRate,
        int channels
    );

    bool writeFloatSamples(
        const float* samples,
        int frameCount
    );

    void close();

    bool isOpen() const;

private:

    void writeUInt16(
        uint16_t value
    );

    void writeUInt32(
        uint32_t value
    );

    void writeHeaderPlaceholder();

    void finalizeHeader();

private:

    std::ofstream file_;

    int sampleRate_;
    int channels_;
    int bitsPerSample_;

    uint32_t dataBytes_;

    bool opened_;
};
