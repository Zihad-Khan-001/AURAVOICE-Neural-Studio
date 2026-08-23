package com.auravoice.neuralstudio.audio

import android.content.Context
import android.content.Intent
import android.os.Build
import android.os.Environment
import androidx.core.content.ContextCompat
import com.auravoice.audioengine.NativeAudioEngine
import java.io.File
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

class AudioController(
    private val context: Context
) {

    private val nativeEngine =
        NativeAudioEngine()

    private var initialized = false

    private var currentRecordingFile: File? = null

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

        val recordingsDirectory =
            File(
                context.getExternalFilesDir(
                    Environment.DIRECTORY_MUSIC
                ),
                "Auravoice/Recordings"
            )

        if (!recordingsDirectory.exists()) {
            recordingsDirectory.mkdirs()
        }

        val timestamp =
            SimpleDateFormat(
                "yyyyMMdd_HHmmss",
                Locale.US
            ).format(Date())

        val file =
            File(
                recordingsDirectory,
                "Auravoice_$timestamp.wav"
            )

        startRecordingService()

        val started =
            nativeEngine.nativeStartRecording(
                file.absolutePath
            )

        if (started) {
            currentRecordingFile = file
        } else {
            stopRecordingService()
        }

        return started
    }

    fun pauseRecording() {

        nativeEngine.nativePauseRecording()
    }

    fun resumeRecording(): Boolean {

        nativeEngine.nativeResumeRecording()

        return true
    }

    fun stopRecording() {

        nativeEngine.nativeStopRecording()

        stopRecordingService()
    }

    fun startPreviewStream(): Boolean {

        if (!initialized) {
            if (!initialize()) {
                return false
            }
        }

        return nativeEngine.nativeStart()
    }

    fun stopPreviewStream() {

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

    fun isRecording(): Boolean {

        return nativeEngine.nativeIsRecording()
    }

    fun isPaused(): Boolean {

        return nativeEngine.nativeIsPaused()
    }

    fun getCurrentRecordingFile(): File? {

        return currentRecordingFile
    }

    fun release() {

        nativeEngine.nativeRelease()

        stopRecordingService()

        initialized = false

        currentRecordingFile = null
    }

    private fun startRecordingService() {

        val intent =
            Intent(
                context,
                AudioRecordingService::class.java
            )

        if (Build.VERSION.SDK_INT >=
            Build.VERSION_CODES.O
        ) {

            ContextCompat.startForegroundService(
                context,
                intent
            )

        } else {

            context.startService(intent)
        }
    }

    private fun stopRecordingService() {

        val intent =
            Intent(
                context,
                AudioRecordingService::class.java
            )

        context.stopService(intent)
    }
}
