#include <jni.h>
#include <android/log.h>

#include <oboe/Oboe.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

#define AV_LOG_TAG "AURAVOICE_NATIVE"

#define AV_LOGI(...) \
    __android_log_print(ANDROID_LOG_INFO, AV_LOG_TAG, __VA_ARGS__)

#define AV_LOGE(...) \
    __android_log_print(ANDROID_LOG_ERROR, AV_LOG_TAG, __VA_ARGS__)

namespace auravoice {

class AudioEngine final : public oboe::AudioStreamDataCallback,
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

        std::lock_guard<std::mutex> lock(mutex_);

        if (initialized_) {
            return true;
        }

        if (sampleRate <= 0 ||
            channelCount <= 0 ||
            bufferSize <= 0) {

            AV_LOGE("Invalid audio configuration");
            return false;
        }

        requestedSampleRate_ = sampleRate;
        requestedChannelCount_ = channelCount;
        requestedBufferSize_ = bufferSize;

        inputGain_.store(1.0f);

        latestPeak_.store(0.0f);
        latestRms_.store(0.0f);

        initialized_ = true;
        running_ = false;

        AV_LOGI(
            "Engine initialized: requested %d Hz / %d channels / %d frames",
            requestedSampleRate_,
            requestedChannelCount_,
            requestedBufferSize_
        );

        return true;
    }

    bool start() {

        std::lock_guard<std::mutex> lock(mutex_);

        if (!initialized_) {
            AV_LOGE("Cannot start: engine not initialized");
            return false;
        }

        if (running_) {
            return true;
        }

        closeStreamLocked();

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

        builder.setFormatConversionAllowed(true);

        builder.setDataCallback(this);

        builder.setErrorCallback(this);

        oboe::Result result =
            builder.openStream(inputStream_);

        if (result != oboe::Result::OK || inputStream_ == nullptr) {

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
            inputStream_->getFramesPerDataCallback();

        AV_LOGI(
            "Opened input stream: %d Hz / %d channels / %d frames",
            actualSampleRate_,
            actualChannelCount_,
            actualBufferSize_
        );

        result = inputStream_->requestStart();

        if (result != oboe::Result::OK) {

            AV_LOGE(
                "Failed to start input stream: %s",
                oboe::convertToText(result)
            );

            closeStreamLocked();

            return false;
        }

        running_ = true;

        AV_LOGI("Microphone capture started");

        return true;
    }

    void stop() {

        std::lock_guard<std::mutex> lock(mutex_);

        running_ = false;

        if (inputStream_) {
            inputStream_->requestStop();
        }

        AV_LOGI("Microphone capture stopped");
    }

    void release() {

        std::lock_guard<std::mutex> lock(mutex_);

        running_ = false;
        initialized_ = false;

        closeStreamLocked();

        requestedSampleRate_ = 0;
        requestedChannelCount_ = 0;
        requestedBufferSize_ = 0;

        actualSampleRate_ = 0;
        actualChannelCount_ = 0;
        actualBufferSize_ = 0;

        latestPeak_.store(0.0f);
        latestRms_.store(0.0f);

        AV_LOGI("AudioEngine released");
    }

    void setInputGain(float gain) {

        gain = std::clamp(
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
            static_cast<const float*>(audioData);

        const int32_t channels =
            audioStream->getChannelCount();

        if (channels <= 0) {
            return oboe::DataCallbackResult::Continue;
        }

        const float gain =
            inputGain_.load(
                std::memory_order_relaxed
            );

        float peak = 0.0f;

        double sumSquares = 0.0;

        const int64_t sampleCount =
            static_cast<int64_t>(numFrames) *
            static_cast<int64_t>(channels);

        for (int64_t i = 0; i < sampleCount; ++i) {

            float sample =
                input[i] * gain;

            sample = std::clamp(
                sample,
                -1.0f,
                1.0f
            );

            const float absolute =
                std::fabs(sample);

            if (absolute > peak) {
                peak = absolute;
            }

            sumSquares +=
                static_cast<double>(sample) *
                static_cast<double>(sample);
        }

        const float rms =
            sampleCount > 0
                ? static_cast<float>(
                    std::sqrt(
                        sumSquares /
                        static_cast<double>(sampleCount)
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

        /*
         * IMPORTANT:
         *
         * The real DSP chain will be inserted after
         * this capture stage.
         *
         * Current order:
         *
         * Microphone
         *     ↓
         * Oboe PCM callback
         *     ↓
         * Input gain
         *     ↓
         * Peak/RMS analysis
         *
         * Future DSP stages:
         *
         * Neural noise suppression
         * High-pass filter
         * EQ
         * De-esser
         * Compressor
         * Reverb
         * Loudness normalization
         * True-peak limiter
         */

        return oboe::DataCallbackResult::Continue;
    }

    void onErrorBeforeClose(
        oboe::AudioStream*,
        oboe::Result error
    ) override {

        AV_LOGE(
            "Oboe stream error before close: %s",
            oboe::convertToText(error)
        );

        running_ = false;
    }

    void onErrorAfterClose(
        oboe::AudioStream*,
        oboe::Result error
    ) override {

        AV_LOGE(
            "Oboe stream error after close: %s",
            oboe::convertToText(error)
        );

        running_ = false;
    }

private:

    void closeStreamLocked() {

        if (!inputStream_) {
            return;
        }

        inputStream_->requestStop();
        inputStream_->close();

        inputStream_.reset();
    }

private:

    mutable std::mutex mutex_;

    std::shared_ptr<oboe::AudioStream> inputStream_;

    std::atomic<bool> initialized_{false};
    std::atomic<bool> running_{false};

    std::atomic<float> inputGain_{1.0f};

    std::atomic<float> latestPeak_{0.0f};
    std::atomic<float> latestRms_{0.0f};

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

    return auravoice::engine().initialize(
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

    return auravoice::engine().start()
        ? JNI_TRUE
        : JNI_FALSE;
}


extern "C"
JNIEXPORT void JNICALL
Java_com_auravoice_audioengine_NativeAudioEngine_nativeStop(
    JNIEnv*,
    jobject
) {

    auravoice::engine().stop();
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

    auravoice::engine().setInputGain(gain);
}


extern "C"
JNIEXPORT jfloat JNICALL
Java_com_auravoice_audioengine_NativeAudioEngine_nativeGetInputGain(
    JNIEnv*,
    jobject
) {

    return auravoice::engine().getInputGain();
}


extern "C"
JNIEXPORT jboolean JNICALL
Java_com_auravoice_audioengine_NativeAudioEngine_nativeIsInitialized(
    JNIEnv*,
    jobject
) {

    return auravoice::engine().isInitialized()
        ? JNI_TRUE
        : JNI_FALSE;
}


extern "C"
JNIEXPORT jboolean JNICALL
Java_com_auravoice_audioengine_NativeAudioEngine_nativeIsRunning(
    JNIEnv*,
    jobject
) {

    return auravoice::engine().isRunning()
        ? JNI_TRUE
        : JNI_FALSE;
}


extern "C"
JNIEXPORT jfloat JNICALL
Java_com_auravoice_audioengine_NativeAudioEngine_nativeGetLatestPeak(
    JNIEnv*,
    jobject
) {

    return auravoice::engine().getLatestPeak();
}


extern "C"
JNIEXPORT jfloat JNICALL
Java_com_auravoice_audioengine_NativeAudioEngine_nativeGetLatestRms(
    JNIEnv*,
    jobject
) {

    return auravoice::engine().getLatestRms();
}
