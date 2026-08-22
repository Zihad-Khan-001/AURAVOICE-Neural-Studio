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

        std::lock_guard<std::mutex> lock(mutex_);

        if (!initialized_) {

            AV_LOGE(
                "Cannot start: engine not initialized"
            );

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

        if (result != oboe::Result::OK ||
            inputStream_ == nullptr) {

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

        result =
            inputStream_->requestStart();

        if (result != oboe::Result::OK) {

            AV_LOGE(
                "Failed to start input stream: %s",
                oboe::convertToText(result)
            );

            closeStreamLocked();

            return false;
        }

        running_ = true;

        AV_LOGI(
            "Microphone capture started"
        );

        return true;
    }

    bool startRecording(
        const std::string& filePath
    ) {

        if (filePath.empty()) {

            AV_LOGE(
                "Recording path is empty"
            );

            return false;
        }

        std::lock_guard<std::mutex> lock(
            mutex_
        );

        if (!initialized_) {

            AV_LOGE(
                "Cannot record: engine not initialized"
            );

            return false;
        }

        if (recording_) {

            AV_LOGI(
                "Recording already active"
            );

            return true;
        }

        if (actualSampleRate_ <= 0 ||
            actualChannelCount_ <= 0) {

            AV_LOGE(
                "Audio stream configuration is not ready"
            );

            return false;
        }

        /*
         * Open the WAV file before starting
         * the microphone stream.
         */
        if (!wavWriter_.open(
                filePath,
                actualSampleRate_,
                actualChannelCount_
            )) {

            AV_LOGE(
                "Failed to open WAV file: %s",
                filePath.c_str()
            );

            return false;
        }

        recording_ = true;

        paused_ = false;

        /*
         * If the audio stream is not running,
         * start it now.
         */
        if (!running_) {

            /*
             * We cannot call startStream() here
             * because it also locks mutex_.
             *
             * Build/start the stream directly.
             */

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

            if (result != oboe::Result::OK ||
                inputStream_ == nullptr) {

                AV_LOGE(
                    "Failed to open recording stream: %s",
                    oboe::convertToText(result)
                );

                wavWriter_.close();

                recording_ = false;

                return false;
            }

            actualSampleRate_ =
                inputStream_->getSampleRate();

            actualChannelCount_ =
                inputStream_->getChannelCount();

            actualBufferSize_ =
                inputStream_->getFramesPerDataCallback();

            /*
             * If the actual device format differs from
             * the requested format, reopen the WAV
             * writer with the real stream format.
             */
            wavWriter_.close();

            if (!wavWriter_.open(
                    filePath,
                    actualSampleRate_,
                    actualChannelCount_
                )) {

                AV_LOGE(
                    "Failed to reopen WAV file"
                );

                closeStreamLocked();

                recording_ = false;

                return false;
            }

            result =
                inputStream_->requestStart();

            if (result != oboe::Result::OK) {

                AV_LOGE(
                    "Failed to start recording stream: %s",
                    oboe::convertToText(result)
                );

                wavWriter_.close();

                closeStreamLocked();

                recording_ = false;

                return false;
            }

            running_ = true;
        }

        AV_LOGI(
            "RAW recording started: %s",
            filePath.c_str()
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

        /*
         * Keep the microphone stream alive.
         *
         * We only stop writing PCM into the WAV file.
         *
         * This allows Resume to continue writing
         * into the same WAV file without creating
         * another file.
         */
        paused_ = true;

        AV_LOGI(
            "Recording paused"
        );
    }

    void resumeRecording() {

        std::lock_guard<std::mutex> lock(
            mutex_
        );

        if (!recording_) {
            return;
        }

        paused_ = false;

        AV_LOGI(
            "Recording resumed"
        );
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

        /*
         * Finalize RIFF/WAV header.
         */
        if (wavWriter_.isOpen()) {
            wavWriter_.close();
        }

        AV_LOGI(
            "Recording stopped"
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

        if (recording_) {
            recording_ = false;
        }

        paused_ = false;

        if (wavWriter_.isOpen()) {
            wavWriter_.close();
        }

        AV_LOGI(
            "Microphone capture stopped"
        );
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

        AV_LOGI(
            "AudioEngine released"
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

        float peak = 0.0f;

        double sumSquares = 0.0;

        /*
         * This buffer contains the
         * input-gain-adjusted signal.
         *
         * No EQ.
         * No noise suppression.
         * No compressor.
         * No reverb.
         * No limiter.
         *
         * This is still the RAW capture stage.
         */
        recordingBuffer_.resize(
            static_cast<size_t>(
                sampleCount
            )
        );

        for (int64_t i = 0;
             i < sampleCount;
             ++i) {

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

            if (absolute > peak) {
                peak = absolute;
            }

            sumSquares +=
                static_cast<double>(
                    sample
                ) *
                static_cast<double>(
                    sample
                );
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

        /*
         * Write audio only when the recording
         * state is active and not paused.
         */
        if (recording_ && !paused_) {

            if (!wavWriter_.writeFloatSamples(
                    recordingBuffer_.data(),
                    numFrames
                )) {

                AV_LOGE(
                    "Failed to write WAV audio data"
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

    std::shared_ptr<oboe::AudioStream>
        inputStream_;

    WavWriter wavWriter_;

    std::vector<float>
        recordingBuffer_;

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

    return auravoice::engine().startStream()
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

    const std::string recordingPath(path);

    env->ReleaseStringUTFChars(
        filePath,
        path
    );

    return auravoice::engine().startRecording(
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
JNIEXPORT jboolean JNICALL
Java_com_auravoice_audioengine_NativeAudioEngine_nativeIsRecording(
    JNIEnv*,
    jobject
) {

    return auravoice::engine().isRecording()
        ? JNI_TRUE
        : JNI_FALSE;
}


extern "C"
JNIEXPORT jboolean JNICALL
Java_com_auravoice_audioengine_NativeAudioEngine_nativeIsPaused(
    JNIEnv*,
    jobject
) {

    return auravoice::engine().isPaused()
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
