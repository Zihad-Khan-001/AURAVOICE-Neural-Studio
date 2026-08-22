package com.auravoice.neuralstudio

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import com.auravoice.neuralstudio.ui.AuraVoiceApp
import com.auravoice.neuralstudio.ui.theme.AuraVoiceTheme

class MainActivity : ComponentActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        setContent {
            AuraVoiceTheme {
                AuraVoiceRoot()
            }
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
