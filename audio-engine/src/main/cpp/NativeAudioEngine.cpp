#include <jni.h>
#include <android/log.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <vector>

#define AV_LOG_TAG "AURAVOICE_NATIVE"

#define AV_LOGI(...) \
    __android_log_print(ANDROID_LOG_INFO, AV_LOG_TAG, __VA_ARGS__)

#define AV_LOGE(...) \
    __android_log_print(ANDROID_LOG_ERROR, AV_LOG_TAG, __VA_ARGS__)

namespace auravoice {

class AudioEngine {
public:
    AudioEngine() = default;

    ~AudioEngine() {
        release();
    }

    bool initialize(
        int32_t sampleRate,
        int32_t channelCount,
        int32_t bufferSize
    ) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (sampleRate <= 0 ||
            channelCount <= 0 ||
            bufferSize <= 0) {
            AV_LOGE("Invalid audio configuration");
            return false;
        }

        sampleRate_ = sampleRate;
        channelCount_ = channelCount;
        bufferSize_ = bufferSize;

        inputGain_ = 1.0f;
        initialized_ = true;
        running_ = false;

        AV_LOGI(
            "AudioEngine initialized: %d Hz, %d channels, %d frames",
            sampleRate_,
            channelCount_,
            bufferSize_
        );

        return true;
    }

    bool start() {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!initialized_) {
            AV_LOGE("Cannot start: engine is not initialized");
            return false;
        }

        running_ = true;

        AV_LOGI("AudioEngine started");

        return true;
    }

    void stop() {
        std::lock_guard<std::mutex> lock(mutex_);

        running_ = false;

        AV_LOGI("AudioEngine stopped");
    }

    void release() {
        std::lock_guard<std::mutex> lock(mutex_);

        running_ = false;
        initialized_ = false;

        sampleRate_ = 0;
        channelCount_ = 0;
        bufferSize_ = 0;

        AV_LOGI("AudioEngine released");
    }

    void setInputGain(float gain) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (gain < 0.0f) {
            gain = 0.0f;
        }

        if (gain > 4.0f) {
            gain = 4.0f;
        }

        inputGain_ = gain;
    }

    float getInputGain() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return inputGain_;
    }

    bool isInitialized() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return initialized_;
    }

    bool isRunning() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return running_;
    }

private:
    mutable std::mutex mutex_;

    int32_t sampleRate_ = 0;
    int32_t channelCount_ = 0;
    int32_t bufferSize_ = 0;

    float inputGain_ = 1.0f;

    bool initialized_ = false;
    bool running_ = false;
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
    ) ? JNI_TRUE : JNI_FALSE;
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
