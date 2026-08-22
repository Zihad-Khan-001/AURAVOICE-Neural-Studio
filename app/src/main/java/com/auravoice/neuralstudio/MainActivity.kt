package com.auravoice.neuralstudio

import android.Manifest
import android.content.pm.PackageManager
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.core.content.ContextCompat
import com.auravoice.neuralstudio.ui.AuraVoiceApp
import com.auravoice.neuralstudio.ui.theme.AuraVoiceTheme

class MainActivity : ComponentActivity() {

    private val microphonePermissionLauncher =
        registerForActivityResult(
            ActivityResultContracts.RequestPermission()
        ) { granted ->

            if (!granted) {
                // Recording will remain unavailable
                // until microphone permission is granted.
            }
        }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        requestMicrophonePermission()

        setContent {
            AuraVoiceTheme {
                AuraVoiceRoot()
            }
        }
    }

    private fun requestMicrophonePermission() {

        val permission =
            Manifest.permission.RECORD_AUDIO

        if (
            ContextCompat.checkSelfPermission(
                this,
                permission
            ) != PackageManager.PERMISSION_GRANTED
        ) {
            microphonePermissionLauncher.launch(permission)
        }
    }
}

@Composable
private fun AuraVoiceRoot() {

    Surface(
        modifier = Modifier.fillMaxSize(),
        color = MaterialTheme.colorScheme.background
    ) {
        AuraVoiceApp()
    }
}
