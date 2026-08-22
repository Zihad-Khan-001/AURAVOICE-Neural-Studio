#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

class WavWriter {
public:

    WavWriter()
        : sampleRate_(48000),
          channels_(1),
          bitsPerSample_(24),
          dataBytes_(0),
          opened_(false) {
    }

    bool open(
        const std::string& path,
        int sampleRate,
        int channels
    ) {

        close();

        sampleRate_ = sampleRate;
        channels_ = channels;
        bitsPerSample_ = 24;
        dataBytes_ = 0;

        file_.open(
            path,
            std::ios::binary |
            std::ios::out |
            std::ios::trunc
        );

        if (!file_.is_open()) {
            return false;
        }

        writeHeaderPlaceholder();

        opened_ = true;

        return true;
    }

    bool writeFloatSamples(
        const float* samples,
        int frameCount
    ) {

        if (!opened_ ||
            samples == nullptr ||
            frameCount <= 0) {
            return false;
        }

        const int sampleCount =
            frameCount * channels_;

        std::vector<uint8_t> pcm24(
            static_cast<size_t>(sampleCount) * 3
        );

        for (int i = 0; i < sampleCount; ++i) {

            float value =
                samples[i];

            if (value > 1.0f) {
                value = 1.0f;
            }

            if (value < -1.0f) {
                value = -1.0f;
            }

            int32_t converted =
                static_cast<int32_t>(
                    value * 8388607.0f
                );

            const size_t index =
                static_cast<size_t>(i) * 3;

            pcm24[index] =
                static_cast<uint8_t>(
                    converted & 0xFF
                );

            pcm24[index + 1] =
                static_cast<uint8_t>(
                    (converted >> 8) & 0xFF
                );

            pcm24[index + 2] =
                static_cast<uint8_t>(
                    (converted >> 16) & 0xFF
                );
        }

        file_.write(
            reinterpret_cast<const char*>(
                pcm24.data()
            ),
            static_cast<std::streamsize>(
                pcm24.size()
            )
        );

        if (!file_) {
            return false;
        }

        dataBytes_ +=
            static_cast<uint32_t>(
                pcm24.size()
            );

        return true;
    }

    void close() {

        if (!file_.is_open()) {
            opened_ = false;
            return;
        }

        finalizeHeader();

        file_.close();

        opened_ = false;
    }

    bool isOpen() const {
        return opened_;
    }

private:

    std::ofstream file_;

    int sampleRate_;
    int channels_;
    int bitsPerSample_;

    uint32_t dataBytes_;

    bool opened_;

    void writeUInt16(
        uint16_t value
    ) {

        char bytes[2];

        bytes[0] =
            static_cast<char>(
                value & 0xFF
            );

        bytes[1] =
            static_cast<char>(
                (value >> 8) & 0xFF
            );

        file_.write(
            bytes,
            2
        );
    }

    void writeUInt32(
        uint32_t value
    ) {

        char bytes[4];

        bytes[0] =
            static_cast<char>(
                value & 0xFF
            );

        bytes[1] =
            static_cast<char>(
                (value >> 8) & 0xFF
            );

        bytes[2] =
            static_cast<char>(
                (value >> 16) & 0xFF
            );

        bytes[3] =
            static_cast<char>(
                (value >> 24) & 0xFF
            );

        file_.write(
            bytes,
            4
        );
    }

    void writeHeaderPlaceholder() {

        file_.write(
            "RIFF",
            4
        );

        writeUInt32(0);

        file_.write(
            "WAVE",
            4
        );

        file_.write(
            "fmt ",
            4
        );

        writeUInt32(16);

        writeUInt16(1);

        writeUInt16(
            static_cast<uint16_t>(
                channels_
            )
        );

        writeUInt32(
            static_cast<uint32_t>(
                sampleRate_
            )
        );

        const uint32_t byteRate =
            static_cast<uint32_t>(
                sampleRate_ *
                channels_ *
                (bitsPerSample_ / 8)
            );

        writeUInt32(byteRate);

        const uint16_t blockAlign =
            static_cast<uint16_t>(
                channels_ *
                (bitsPerSample_ / 8)
            );

        writeUInt16(blockAlign);

        writeUInt16(
            static_cast<uint16_t>(
                bitsPerSample_
            )
        );

        file_.write(
            "data",
            4
        );

        writeUInt32(0);
    }

    void finalizeHeader() {

        const uint32_t riffSize =
            36 + dataBytes_;

        file_.seekp(
            4,
            std::ios::beg
        );

        writeUInt32(
            riffSize
        );

        file_.seekp(
            40,
            std::ios::beg
        );

        writeUInt32(
            dataBytes_
        );

        file_.flush();
    }
};
