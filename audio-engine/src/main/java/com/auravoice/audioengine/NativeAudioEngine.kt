package com.auravoice.audioengine

class NativeAudioEngine {

    companion object {
        init {
            System.loadLibrary("auravoice_audio")
        }
    }

    external fun nativeInitialize(
        sampleRate: Int,
        channelCount: Int,
        bufferSize: Int
    ): Boolean

    external fun nativeStart(): Boolean

    external fun nativeStop()

    external fun nativeRelease()

    external fun nativeSetInputGain(
        gain: Float
    )

    external fun nativeGetInputGain(): Float

    external fun nativeIsInitialized(): Boolean

    external fun nativeIsRunning(): Boolean
}
