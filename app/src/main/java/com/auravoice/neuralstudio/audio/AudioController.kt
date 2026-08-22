package com.auravoice.neuralstudio.audio

import android.content.Context
import com.auravoice.audioengine.NativeAudioEngine

class AudioController(
    private val context: Context
) {

    private val nativeEngine =
        NativeAudioEngine()

    private var initialized = false

    fun initialize(): Boolean {

        if (initialized) {
            return true
        }

        val result =
            nativeEngine.nativeInitialize(
                48000,
                1,
                256
            )

        initialized = result

        return result
    }

    fun startRecording(): Boolean {

        if (!initialized) {
            if (!initialize()) {
                return false
            }
        }

        return nativeEngine.nativeStart()
    }

    fun pauseRecording() {

        /*
         * True native pause will be connected
         * with the recording state machine
         * in the next audio-engine step.
         */
        nativeEngine.nativeStop()
    }

    fun resumeRecording(): Boolean {

        return nativeEngine.nativeStart()
    }

    fun stopRecording() {

        nativeEngine.nativeStop()
    }

    fun setInputGain(
        gain: Float
    ) {

        nativeEngine.nativeSetInputGain(
            gain
        )
    }

    fun getInputGain(): Float {

        return nativeEngine.nativeGetInputGain()
    }

    fun getPeakLevel(): Float {

        return nativeEngine.nativeGetLatestPeak()
    }

    fun getRmsLevel(): Float {

        return nativeEngine.nativeGetLatestRms()
    }

    fun isRunning(): Boolean {

        return nativeEngine.nativeIsRunning()
    }

    fun isInitialized(): Boolean {

        return nativeEngine.nativeIsInitialized()
    }

    fun release() {

        nativeEngine.nativeRelease()

        initialized = false
    }
}
