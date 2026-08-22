package com.auravoice.neuralstudio.audio

import android.content.Context
import com.auravoice.audioengine.NativeAudioEngine

class AudioController(
    private val context: Context
) {

    private val nativeEngine =
        NativeAudioEngine()

    private var initialized = false
    private var paused = false

    fun initialize(): Boolean {

        if (initialized) {
            return true
        }

        val result =
            nativeEngine.nativeInitialize(
                48_000,
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

        paused = false

        return nativeEngine.nativeStart()
    }

    fun pauseRecording() {

        if (!initialized) {
            return
        }

        if (!nativeEngine.nativeIsRunning()) {
            return
        }

        nativeEngine.nativeStop()

        paused = true
    }

    fun resumeRecording(): Boolean {

        if (!initialized) {
            return false
        }

        paused = false

        return nativeEngine.nativeStart()
    }

    fun stopRecording() {

        if (!initialized) {
            return
        }

        nativeEngine.nativeStop()

        paused = false
    }

    fun setInputGain(gain: Float) {

        if (!initialized) {
            initialize()
        }

        nativeEngine.nativeSetInputGain(gain)
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

    fun isRecording(): Boolean {

        return nativeEngine.nativeIsRunning()
    }

    fun isPaused(): Boolean {

        return paused
    }

    fun release() {

        if (!initialized) {
            return
        }

        nativeEngine.nativeRelease()

        initialized = false
        paused = false
    }
}
