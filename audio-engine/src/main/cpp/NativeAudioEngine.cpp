#include <jni.h>
#include <android/log.h>

#include <oboe/Oboe.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "WavWriter.h"

#define AV_LOG_TAG "AURAVOICE_NATIVE"

#define AV_LOGI(...) \
    __android_log_print(ANDROID_LOG_INFO, AV_LOG_TAG, __VA_ARGS__)

#define AV_LOGE(...) \
    __android_log_print(ANDROID_LOG_ERROR, AV_LOG_TAG, __VA_ARGS__)

namespace auravoice {

constexpr float kPi = 3.14159265358979323846f;

static float dbToLinear(float db) {
    return std::pow(10.0f, db / 20.0f);
}

static float linearToDb(float value) {
    if (value <= 0.000001f) {
        return -120.0f;
    }

    return 20.0f * std::log10(value);
}

static float softClip(float sample) {
    return std::tanh(sample);
}

/*
 * Simple first-order high-pass filter.
 *
 * Used as the first stage of the vocal chain.
 */
class HighPassFilter final {

public:

    void prepare(
        int sampleRate,
        float cutoff
    ) {

        sampleRate_ = std::max(
            sampleRate,
            1
        );

        cutoff_ = cutoff;

        reset();

        update();
    }

    void reset() {
        previousInput_ = 0.0f;
        previousOutput_ = 0.0f;
    }

    float process(float input) {

        const float output =
            alpha_ *
            (
                previousOutput_
                +
                input
                -
                previousInput_
            );

        previousInput_ = input;
        previousOutput_ = output;

        return output;
    }

private:

    void update() {

        const float rc =
            1.0f /
            (
                2.0f *
                kPi *
                std::max(
                    cutoff_,
                    1.0f
                )
            );

        const float dt =
            1.0f /
            static_cast<float>(
                sampleRate_
            );

        alpha_ =
            rc /
            (rc + dt);
    }

private:

    int sampleRate_ = 48000;

    float cutoff_ = 80.0f;

    float alpha_ = 0.0f;

    float previousInput_ = 0.0f;

    float previousOutput_ = 0.0f;
};


/*
 * Biquad filter supporting:
 *
 * Peaking EQ
 * High shelf
 */
class Biquad final {

public:

    enum class Type {
        Peaking,
        HighShelf
    };

    void prepare(
        int sampleRate,
        Type type,
        float frequency,
        float gainDb,
        float q
    ) {

        type_ = type;

        sampleRate_ =
            std::max(
                sampleRate,
                1
            );

        frequency_ =
            frequency;

        gainDb_ =
            gainDb;

        q_ =
            std::max(
                q,
                0.01f
            );

        reset();

        update();
    }

    void reset() {

        x1_ = 0.0f;
        x2_ = 0.0f;
        y1_ = 0.0f;
        y2_ = 0.0f;
    }

    float process(float input) {

        const float output =
            (
                b0_ * input
                +
                b1_ * x1_
                +
                b2_ * x2_
                -
                a1_ * y1_
                -
                a2_ * y2_
            )
            /
            a0_;

        x2_ = x1_;
        x1_ = input;

        y2_ = y1_;
        y1_ = output;

        return output;
    }

private:

    void update() {

        const float omega =
            2.0f *
            kPi *
            frequency_ /
            static_cast<float>(
                sampleRate_
            );

        const float sn =
            std::sin(omega);

        const float cs =
            std::cos(omega);

        const float alpha =
            sn /
            (
                2.0f *
                q_
            );

        const float A =
            std::pow(
                10.0f,
                gainDb_ / 40.0f
            );

        if (type_ ==
            Type::Peaking) {

            b0_ =
                1.0f +
                alpha * A;

            b1_ =
                -2.0f * cs;

            b2_ =
                1.0f -
                alpha * A;

            a0_ =
                1.0f +
                alpha / A;

            a1_ =
                -2.0f * cs;

            a2_ =
                1.0f -
                alpha / A;

        } else {

            const float twoSqrtAAlpha =
                2.0f *
                std::sqrt(A) *
                alpha;

            b0_ =
                A *
                (
                    (A + 1.0f)
                    +
                    (A - 1.0f) * cs
                    +
                    twoSqrtAAlpha
                );

            b1_ =
                -2.0f *
                A *
                (
                    (A - 1.0f)
                    +
                    (A + 1.0f) * cs
                );

            b2_ =
                A *
                (
                    (A + 1.0f)
                    +
                    (A - 1.0f) * cs
                    -
                    twoSqrtAAlpha
                );

            a0_ =
                (A + 1.0f)
                -
                (A - 1.0f) * cs
                +
                twoSqrtAAlpha;

            a1_ =
                2.0f *
                (
                    (A - 1.0f)
                    -
                    (A + 1.0f) * cs
                );

            a2_ =
                (A + 1.0f)
                -
                (A - 1.0f) * cs
                -
                twoSqrtAAlpha;
        }
    }

private:

    Type type_ =
        Type::Peaking;

    int sampleRate_ = 48000;

    float frequency_ = 1000.0f;

    float gainDb_ = 0.0f;

    float q_ = 1.0f;

    float a0_ = 1.0f;

    float a1_ = 0.0f;

    float a2_ = 0.0f;

    float b0_ = 1.0f;

    float b1_ = 0.0f;

    float b2_ = 0.0f;

    float x1_ = 0.0f;

    float x2_ = 0.0f;

    float y1_ = 0.0f;

    float y2_ = 0.0f;
};


/*
 * Gentle vocal compressor.
 *
 * Ratio: 3.2:1
 * Attack: 12 ms
 * Release: 80 ms
 */
class VocalCompressor final {

public:

    void prepare(
        int sampleRate
    ) {

        sampleRate_ =
            std::max(
                sampleRate,
                1
            );

        envelope_ = 0.0f;
    }

    void reset() {
        envelope_ = 0.0f;
    }

    float process(
        float input
    ) {

        const float absolute =
            std::fabs(input);

        const float attackCoeff =
            std::exp(
                -1.0f /
                (
                    0.012f *
                    static_cast<float>(
                        sampleRate_
                    )
                )
            );

        const float releaseCoeff =
            std::exp(
                -1.0f /
                (
                    0.080f *
                    static_cast<float>(
                        sampleRate_
                    )
                )
            );

        if (absolute > envelope_) {

            envelope_ =
                attackCoeff *
                envelope_
                +
                (
                    1.0f -
                    attackCoeff
                ) *
                absolute;

        } else {

            envelope_ =
                releaseCoeff *
                envelope_
                +
                (
                    1.0f -
                    releaseCoeff
                ) *
                absolute;
        }

        const float envelopeDb =
            linearToDb(
                envelope_
            );

        constexpr float thresholdDb =
            -18.0f;

        constexpr float ratio =
            3.2f;

        if (envelopeDb <=
            thresholdDb) {

            return input;
        }

        const float compressedDb =
            thresholdDb
            +
            (
                envelopeDb -
                thresholdDb
            )
            /
            ratio;

        const float gainReductionDb =
            compressedDb -
            envelopeDb;

        const float gain =
            dbToLinear(
                gainReductionDb
            );

        return input * gain;
    }

private:

    int sampleRate_ = 48000;

    float envelope_ = 0.0f;
};


/*
 * Gentle vocal de-esser.
 *
 * This intentionally uses a conservative
 * amount of attenuation so the original
 * vocal character remains intact.
 */
class VocalDeEsser final {

public:

    void prepare(
        int sampleRate
    ) {

        sampleRate_ =
            std::max(
                sampleRate,
                1
            );

        lowState_ = 0.0f;
        highState_ = 0.0f;
        detector_ = 0.0f;
    }

    void reset() {

        lowState_ = 0.0f;
        highState_ = 0.0f;
        detector_ = 0.0f;
    }

    float process(
        float input
    ) {

        const float lowCoeff =
            std::exp(
                -2.0f *
                kPi *
                4500.0f /
                static_cast<float>(
                    sampleRate_
                )
            );

        const float highCoeff =
            std::exp(
                -2.0f *
                kPi *
                7500.0f /
                static_cast<float>(
                    sampleRate_
                )
            );

        lowState_ =
            lowCoeff *
            lowState_
            +
            (
                1.0f -
                lowCoeff
            ) *
            input;

        highState_ =
            highCoeff *
            highState_
            +
            (
                1.0f -
                highCoeff
            ) *
            input;

        const float highBand =
            highState_ -
            lowState_;

        const float amount =
            std::clamp(
                std::fabs(highBand) * 2.0f,
                0.0f,
                1.0f
            );

        detector_ =
            detector_ * 0.995f
            +
            amount * 0.005f;

        /*
         * Maximum attenuation:
         * approximately -3.5 dB.
         */
        const float reduction =
            1.0f -
            0.33f *
            detector_;

        return
            input *
            std::clamp(
                reduction,
                0.67f,
                1.0f
            );
    }

private:

    int sampleRate_ = 48000;

    float lowState_ = 0.0f;

    float highState_ = 0.0f;

    float detector_ = 0.0f;
};


/*
 * Small intimate ambience.
 *
 * Wet level is deliberately low.
 */
class IntimateReverb final {

public:

    void prepare(
        int sampleRate
    ) {

        sampleRate_ =
            std::max(
                sampleRate,
                1
            );

        const int delayFrames =
            std::max(
                1,
                static_cast<int>(
                    0.085f *
                    sampleRate_
                )
            );

        delay_.assign(
            static_cast<size_t>(
                delayFrames
            ),
            0.0f
        );

        index_ = 0;
    }

    void reset() {

        std::fill(
            delay_.begin(),
            delay_.end(),
            0.0f
        );

        index_ = 0;
    }

    float process(
        float input
    ) {

        if (delay_.empty()) {
            return input;
        }

        const float delayed =
            delay_[index_];

        const float feedback =
            0.24f;

        delay_[index_] =
            input +
            delayed * feedback;

        index_++;

        if (index_ >=
            delay_.size()) {

            index_ = 0;
        }

        /*
         * 10% wet.
         */
        return
            input * 0.90f
            +
            delayed * 0.10f;
    }

private:

    int sampleRate_ = 48000;

    std::vector<float> delay_;

    size_t index_ = 0;
};


/*
 * Final true-peak-safe style limiter.
 *
 * This is a conservative sample-domain
 * limiter. Offline true-peak oversampling
 * can be added later without changing
 * the recording architecture.
 */
class VocalLimiter final {

public:

    void prepare() {
        previous_ = 0.0f;
    }

    void reset() {
        previous_ = 0.0f;
    }

    float process(
        float input
    ) {

        constexpr float ceiling =
            0.841395f;
        /*
         * -1.5 dBFS equivalent linear
         * ceiling.
         */

        float output =
            std::clamp(
                input,
                -ceiling,
                ceiling
            );

        /*
         * Very small smoothing step
         * to reduce hard edges.
         */
        output =
            previous_ * 0.02f
            +
            output * 0.98f;

        previous_ = output;

        return output;
    }

private:

    float previous_ = 0.0f;
};


/*
 * Complete mastering chain.
 */
class VocalMasteringChain final {

public:

    void prepare(
        int sampleRate
    ) {

        sampleRate_ =
            std::max(
                sampleRate,
                1
            );

        highPass_.prepare(
            sampleRate_,
            80.0f
        );

        eqWarmth_.prepare(
            sampleRate_,
            Biquad::Type::Peaking,
            250.0f,
            3.0f,
            1.0f
        );

        eqBody_.prepare(
            sampleRate_,
            Biquad::Type::Peaking,
            450.0f,
            -2.0f,
            1.0f
        );

        eqPresence_.prepare(
            sampleRate_,
            Biquad::Type::Peaking,
            3400.0f,
            2.8f,
            1.0f
        );

        eqAir_.prepare(
            sampleRate_,
            Biquad::Type::HighShelf,
            10000.0f,
            1.8f,
            0.707f
        );

        deEsser_.prepare(
            sampleRate_
        );

        compressor_.prepare(
            sampleRate_
        );

        reverb_.prepare(
            sampleRate_
        );

        limiter_.prepare();

        reset();
    }

    void reset() {

        highPass_.reset();

        eqWarmth_.reset();

        eqBody_.reset();

        eqPresence_.reset();

        eqAir_.reset();

        deEsser_.reset();

        compressor_.reset();

        reverb_.reset();

        limiter_.reset();
    }

    float process(
        float input
    ) {

        float sample = input;

        /*
         * Vocal EQ
         */
        sample =
            highPass_.process(
                sample
            );

        sample =
            eqWarmth_.process(
                sample
            );

        sample =
            eqBody_.process(
                sample
            );

        sample =
            eqPresence_.process(
                sample
            );

        sample =
            eqAir_.process(
                sample
            );

        /*
         * Dynamic sibilance control
         */
        sample =
            deEsser_.process(
                sample
            );

        /*
         * Gentle compression
         */
        sample =
            compressor_.process(
                sample
            );

        /*
         * Very subtle ambience
         */
        sample =
            reverb_.process(
                sample
            );

        /*
         * Final safety
         */
        sample =
            limiter_.process(
                sample
            );

        return
            std::clamp(
                sample,
                -1.0f,
                1.0f
            );
    }

private:

    int sampleRate_ = 48000;

    HighPassFilter highPass_;

    Biquad eqWarmth_;

    Biquad eqBody_;

    Biquad eqPresence_;

    Biquad eqAir_;

    VocalDeEsser deEsser_;

    VocalCompressor compressor_;

    IntimateReverb reverb_;

    VocalLimiter limiter_;
};


class AudioEngine final :
    public oboe::AudioStreamDataCallback,
    public oboe::AudioStreamErrorCallback {

public:

    AudioEngine() = default;

    ~AudioEngine() override {
        release();
    }

    bool initialize(
        int32_t sampleRate,
        int32_t channelCount,
        int32_t bufferSize
    ) {

        std::lock_guard<std::mutex> lock(
            mutex_
        );

        if (initialized_) {
            return true;
        }

        if (sampleRate <= 0 ||
            channelCount <= 0 ||
            bufferSize <= 0) {

            AV_LOGE(
                "Invalid audio configuration"
            );

            return false;
        }

        requestedSampleRate_ =
            sampleRate;

        requestedChannelCount_ =
            channelCount;

        requestedBufferSize_ =
            bufferSize;

        inputGain_.store(
            1.0f,
            std::memory_order_relaxed
        );

        latestPeak_.store(
            0.0f,
            std::memory_order_relaxed
        );

        latestRms_.store(
            0.0f,
            std::memory_order_relaxed
        );

        initialized_ = true;

        running_ = false;

        recording_ = false;

        paused_ = false;

        AV_LOGI(
            "Engine initialized: %d Hz / %d channels / %d frames",
            requestedSampleRate_,
            requestedChannelCount_,
            requestedBufferSize_
        );

        return true;
    }

    bool startStream() {

        std::lock_guard<std::mutex> lock(
            mutex_
        );

        if (!initialized_) {
            return false;
        }

        if (running_) {
            return true;
        }

        closeStreamLocked();

        return openAndStartStreamLocked();
    }

    bool startRecording(
        const std::string& filePath
    ) {

        if (filePath.empty()) {
            return false;
        }

        std::lock_guard<std::mutex> lock(
            mutex_
        );

        if (!initialized_) {
            return false;
        }

        if (recording_) {
            return true;
        }

        if (!running_) {

            closeStreamLocked();

            if (!openAndStartStreamLocked()) {
                return false;
            }
        }

        actualSampleRate_ =
            inputStream_->getSampleRate();

        actualChannelCount_ =
            inputStream_->getChannelCount();

        masteredFilePath_ =
            makeMasteredPath(
                filePath
            );

        if (!wavWriter_.open(
                filePath,
                actualSampleRate_,
                actualChannelCount_
            )) {

            AV_LOGE(
                "Failed to open RAW WAV"
            );

            return false;
        }

        if (!masteredWriter_.open(
                masteredFilePath_,
                actualSampleRate_,
                actualChannelCount_
            )) {

            AV_LOGE(
                "Failed to open MASTERED WAV"
            );

            wavWriter_.close();

            return false;
        }

        mastering_.prepare(
            actualSampleRate_
        );

        recording_ = true;

        paused_ = false;

        AV_LOGI(
            "RAW recording: %s",
            filePath.c_str()
        );

        AV_LOGI(
            "MASTERED recording: %s",
            masteredFilePath_.c_str()
        );

        return true;
    }

    void pauseRecording() {

        std::lock_guard<std::mutex> lock(
            mutex_
        );

        if (!recording_) {
            return;
        }

        paused_ = true;
    }

    void resumeRecording() {

        std::lock_guard<std::mutex> lock(
            mutex_
        );

        if (!recording_) {
            return;
        }

        paused_ = false;
    }

    void stopRecording() {

        std::lock_guard<std::mutex> lock(
            mutex_
        );

        if (!recording_) {
            return;
        }

        recording_ = false;

        paused_ = false;

        if (wavWriter_.isOpen()) {
            wavWriter_.close();
        }

        if (masteredWriter_.isOpen()) {
            masteredWriter_.close();
        }

        AV_LOGI(
            "RAW + MASTERED recording finalized"
        );
    }

    void stopStream() {

        std::lock_guard<std::mutex> lock(
            mutex_
        );

        running_ = false;

        if (inputStream_) {
            inputStream_->requestStop();
        }

        recording_ = false;

        paused_ = false;

        if (wavWriter_.isOpen()) {
            wavWriter_.close();
        }

        if (masteredWriter_.isOpen()) {
            masteredWriter_.close();
        }
    }

    void release() {

        std::lock_guard<std::mutex> lock(
            mutex_
        );

        running_ = false;

        recording_ = false;

        paused_ = false;

        initialized_ = false;

        if (wavWriter_.isOpen()) {
            wavWriter_.close();
        }

        if (masteredWriter_.isOpen()) {
            masteredWriter_.close();
        }

        closeStreamLocked();

        requestedSampleRate_ = 0;
        requestedChannelCount_ = 0;
        requestedBufferSize_ = 0;

        actualSampleRate_ = 0;
        actualChannelCount_ = 0;
        actualBufferSize_ = 0;

        latestPeak_.store(
            0.0f,
            std::memory_order_relaxed
        );

        latestRms_.store(
            0.0f,
            std::memory_order_relaxed
        );
    }

    void setInputGain(
        float gain
    ) {

        gain =
            std::clamp(
                gain,
                0.0f,
                4.0f
            );

        inputGain_.store(
            gain,
            std::memory_order_relaxed
        );
    }

    float getInputGain() const {
        return inputGain_.load(
            std::memory_order_relaxed
        );
    }

    float getLatestPeak() const {
        return latestPeak_.load(
            std::memory_order_relaxed
        );
    }

    float getLatestRms() const {
        return latestRms_.load(
            std::memory_order_relaxed
        );
    }

    bool isInitialized() const {
        return initialized_.load(
            std::memory_order_relaxed
        );
    }

    bool isRunning() const {
        return running_.load(
            std::memory_order_relaxed
        );
    }

    bool isRecording() const {
        return recording_.load(
            std::memory_order_relaxed
        );
    }

    bool isPaused() const {
        return paused_.load(
            std::memory_order_relaxed
        );
    }

    oboe::DataCallbackResult onAudioReady(
        oboe::AudioStream* audioStream,
        void* audioData,
        int32_t numFrames
    ) override {

        if (!running_) {
            return oboe::DataCallbackResult::Continue;
        }

        if (audioStream == nullptr ||
            audioData == nullptr ||
            numFrames <= 0) {

            return oboe::DataCallbackResult::Continue;
        }

        const auto* input =
            static_cast<const float*>(
                audioData
            );

        const int32_t channels =
            audioStream->getChannelCount();

        if (channels <= 0) {
            return oboe::DataCallbackResult::Continue;
        }

        const float gain =
            inputGain_.load(
                std::memory_order_relaxed
            );

        const int64_t sampleCount =
            static_cast<int64_t>(
                numFrames
            ) *
            static_cast<int64_t>(
                channels
            );

        /*
         * Reuse allocated memory whenever possible.
         */
        if (
            recordingBuffer_.capacity()
            <
            static_cast<size_t>(
                sampleCount
            )
        ) {

            recordingBuffer_.reserve(
                static_cast<size_t>(
                    sampleCount
                )
            );

            masteredBuffer_.reserve(
                static_cast<size_t>(
                    sampleCount
                )
            );
        }

        recordingBuffer_.resize(
            static_cast<size_t>(
                sampleCount
            )
        );

        masteredBuffer_.resize(
            static_cast<size_t>(
                sampleCount
            )
        );

        float peak = 0.0f;

        double sumSquares = 0.0;

        for (
            int64_t i = 0;
            i < sampleCount;
            ++i
        ) {

            float sample =
                input[i] * gain;

            sample =
                std::clamp(
                    sample,
                    -1.0f,
                    1.0f
                );

            recordingBuffer_[
                static_cast<size_t>(i)
            ] = sample;

            const float absolute =
                std::fabs(sample);

            peak =
                std::max(
                    peak,
                    absolute
                );

            sumSquares +=
                static_cast<double>(
                    sample
                ) *
                static_cast<double>(
                    sample
                );

            /*
             * Mastered signal is generated
             * separately.
             *
             * RAW remains untouched by DSP.
             *
             * For multi-channel input, the same
             * vocal processing is applied to
             * each channel independently.
             */
            if (
                recording_
                &&
                !paused_
            ) {

                masteredBuffer_[
                    static_cast<size_t>(i)
                ] =
                    mastering_.process(
                        sample
                    );

            } else {

                masteredBuffer_[
                    static_cast<size_t>(i)
                ] = sample;
            }
        }

        const float rms =
            sampleCount > 0
                ? static_cast<float>(
                    std::sqrt(
                        sumSquares /
                        static_cast<double>(
                            sampleCount
                        )
                    )
                )
                : 0.0f;

        latestPeak_.store(
            peak,
            std::memory_order_relaxed
        );

        latestRms_.store(
            rms,
            std::memory_order_relaxed
        );

        if (
            recording_
            &&
            !paused_
        ) {

            /*
             * RAW output
             */
            if (!wavWriter_.writeFloatSamples(
                    recordingBuffer_.data(),
                    numFrames
                )) {

                AV_LOGE(
                    "RAW WAV write failed"
                );
            }

            /*
             * MASTERED output
             */
            if (!masteredWriter_.writeFloatSamples(
                    masteredBuffer_.data(),
                    numFrames
                )) {

                AV_LOGE(
                    "MASTERED WAV write failed"
                );
            }
        }

        return oboe::DataCallbackResult::Continue;
    }

    void onErrorBeforeClose(
        oboe::AudioStream*,
        oboe::Result error
    ) override {

        AV_LOGE(
            "Oboe error before close: %s",
            oboe::convertToText(error)
        );

        running_ = false;
    }

    void onErrorAfterClose(
        oboe::AudioStream*,
        oboe::Result error
    ) override {

        AV_LOGE(
            "Oboe error after close: %s",
            oboe::convertToText(error)
        );

        running_ = false;
    }

private:

    bool openAndStartStreamLocked() {

        oboe::AudioStreamBuilder builder;

        builder.setDirection(
            oboe::Direction::Input
        );

        builder.setPerformanceMode(
            oboe::PerformanceMode::LowLatency
        );

        builder.setSharingMode(
            oboe::SharingMode::Exclusive
        );

        builder.setFormat(
            oboe::AudioFormat::Float
        );

        builder.setChannelCount(
            requestedChannelCount_
        );

        builder.setSampleRate(
            requestedSampleRate_
        );

        builder.setFramesPerDataCallback(
            requestedBufferSize_
        );

        builder.setInputPreset(
            oboe::InputPreset::VoiceRecognition
        );

        builder.setFormatConversionAllowed(
            true
        );

        builder.setDataCallback(
            this
        );

        builder.setErrorCallback(
            this
        );

        oboe::Result result =
            builder.openStream(
                inputStream_
            );

        if (
            result != oboe::Result::OK
            ||
            inputStream_ == nullptr
        ) {

            AV_LOGE(
                "Failed to open input stream: %s",
                oboe::convertToText(result)
            );

            inputStream_.reset();

            return false;
        }

        actualSampleRate_ =
            inputStream_->getSampleRate();

        actualChannelCount_ =
            inputStream_->getChannelCount();

        actualBufferSize_ =
            inputStream_->
                getFramesPerDataCallback();

        result =
            inputStream_->requestStart();

        if (
            result !=
            oboe::Result::OK
        ) {

            AV_LOGE(
                "Failed to start input stream: %s",
                oboe::convertToText(result)
            );

            closeStreamLocked();

            return false;
        }

        running_ = true;

        AV_LOGI(
            "Audio stream started: %d Hz / %d channels / %d frames",
            actualSampleRate_,
            actualChannelCount_,
            actualBufferSize_
        );

        return true;
    }

    void closeStreamLocked() {

        if (!inputStream_) {
            return;
        }

        inputStream_->requestStop();

        inputStream_->close();

        inputStream_.reset();
    }

    static std::string makeMasteredPath(
        const std::string& rawPath
    ) {

        const size_t dot =
            rawPath.find_last_of(
                '.'
            );

        if (
            dot == std::string::npos
        ) {

            return
                rawPath +
                "_MASTERED.wav";
        }

        return
            rawPath.substr(
                0,
                dot
            )
            +
            "_MASTERED"
            +
            rawPath.substr(
                dot
            );
    }

private:

    mutable std::mutex mutex_;

    std::shared_ptr<oboe::AudioStream>
        inputStream_;

    WavWriter wavWriter_;

    WavWriter masteredWriter_;

    std::vector<float>
        recordingBuffer_;

    std::vector<float>
        masteredBuffer_;

    VocalMasteringChain
        mastering_;

    std::string
        masteredFilePath_;

    std::atomic<bool>
        initialized_{false};

    std::atomic<bool>
        running_{false};

    std::atomic<bool>
        recording_{false};

    std::atomic<bool>
        paused_{false};

    std::atomic<float>
        inputGain_{1.0f};

    std::atomic<float>
        latestPeak_{0.0f};

    std::atomic<float>
        latestRms_{0.0f};

    int32_t requestedSampleRate_ = 0;

    int32_t requestedChannelCount_ = 0;

    int32_t requestedBufferSize_ = 0;

    int32_t actualSampleRate_ = 0;

    int32_t actualChannelCount_ = 0;

    int32_t actualBufferSize_ = 0;
};


AudioEngine& engine() {

    static AudioEngine instance;

    return instance;
}

} // namespace auravoice


extern "C"
JNIEXPORT jboolean JNICALL
Java_com_auravoice_audioengine_NativeAudioEngine_nativeInitialize(
    JNIEnv*,
    jobject,
    jint sampleRate,
    jint channelCount,
    jint bufferSize
) {

    return
        auravoice::engine().initialize(
            sampleRate,
            channelCount,
            bufferSize
        )
        ? JNI_TRUE
        : JNI_FALSE;
}


extern "C"
JNIEXPORT jboolean JNICALL
Java_com_auravoice_audioengine_NativeAudioEngine_nativeStart(
    JNIEnv*,
    jobject
) {

    return
        auravoice::engine().startStream()
        ? JNI_TRUE
        : JNI_FALSE;
}


extern "C"
JNIEXPORT jboolean JNICALL
Java_com_auravoice_audioengine_NativeAudioEngine_nativeStartRecording(
    JNIEnv* env,
    jobject,
    jstring filePath
) {

    if (filePath == nullptr) {
        return JNI_FALSE;
    }

    const char* path =
        env->GetStringUTFChars(
            filePath,
            nullptr
        );

    if (path == nullptr) {
        return JNI_FALSE;
    }

    const std::string recordingPath(
        path
    );

    env->ReleaseStringUTFChars(
        filePath,
        path
    );

    return
        auravoice::engine().startRecording(
            recordingPath
        )
        ? JNI_TRUE
        : JNI_FALSE;
}


extern "C"
JNIEXPORT void JNICALL
Java_com_auravoice_audioengine_NativeAudioEngine_nativePauseRecording(
    JNIEnv*,
    jobject
) {

    auravoice::engine().pauseRecording();
}


extern "C"
JNIEXPORT void JNICALL
Java_com_auravoice_audioengine_NativeAudioEngine_nativeResumeRecording(
    JNIEnv*,
    jobject
) {

    auravoice::engine().resumeRecording();
}


extern "C"
JNIEXPORT void JNICALL
Java_com_auravoice_audioengine_NativeAudioEngine_nativeStopRecording(
    JNIEnv*,
    jobject
) {

    auravoice::engine().stopRecording();
}


extern "C"
JNIEXPORT void JNICALL
Java_com_auravoice_audioengine_NativeAudioEngine_nativeStop(
    JNIEnv*,
    jobject
) {

    auravoice::engine().stopStream();
}


extern "C"
JNIEXPORT void JNICALL
Java_com_auravoice_audioengine_NativeAudioEngine_nativeRelease(
    JNIEnv*,
    jobject
) {

    auravoice::engine().release();
}


extern "C"
JNIEXPORT void JNICALL
Java_com_auravoice_audioengine_NativeAudioEngine_nativeSetInputGain(
    JNIEnv*,
    jobject,
    jfloat gain
) {

    auravoice::engine().setInputGain(
        gain
    );
}


extern "C"
JNIEXPORT jfloat JNICALL
Java_com_auravoice_audioengine_NativeAudioEngine_nativeGetInputGain(
    JNIEnv*,
    jobject
) {

    return
        auravoice::engine().getInputGain();
}


extern "C"
JNIEXPORT jboolean JNICALL
Java_com_auravoice_audioengine_NativeAudioEngine_nativeIsInitialized(
    JNIEnv*,
    jobject
) {

    return
        auravoice::engine().isInitialized()
        ? JNI_TRUE
        : JNI_FALSE;
}


extern "C"
JNIEXPORT jboolean JNICALL
Java_com_auravoice_audioengine_NativeAudioEngine_nativeIsRunning(
    JNIEnv*,
    jobject
) {

    return
        auravoice::engine().isRunning()
        ? JNI_TRUE
        : JNI_FALSE;
}


extern "C"
JNIEXPORT jboolean JNICALL
Java_com_auravoice_audioengine_NativeAudioEngine_nativeIsRecording(
    JNIEnv*,
    jobject
) {

    return
        auravoice::engine().isRecording()
        ? JNI_TRUE
        : JNI_FALSE;
}


extern "C"
JNIEXPORT jboolean JNICALL
Java_com_auravoice_audioengine_NativeAudioEngine_nativeIsPaused(
    JNIEnv*,
    jobject
) {

    return
        auravoice::engine().isPaused()
        ? JNI_TRUE
        : JNI_FALSE;
}


extern "C"
JNIEXPORT jfloat JNICALL
Java_com_auravoice_audioengine_NativeAudioEngine_nativeGetLatestPeak(
    JNIEnv*,
    jobject
) {

    return
        auravoice::engine().getLatestPeak();
}


extern "C"
JNIEXPORT jfloat JNICALL
Java_com_auravoice_audioengine_NativeAudioEngine_nativeGetLatestRms(
    JNIEnv*,
    jobject
) {

    return
        auravoice::engine().getLatestRms();
}
