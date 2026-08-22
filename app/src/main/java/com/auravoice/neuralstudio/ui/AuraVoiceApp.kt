package com.auravoice.neuralstudio.ui

import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.GraphicEq
import androidx.compose.material.icons.filled.Tune
import androidx.compose.material3.Icon
import androidx.compose.material3.NavigationBar
import androidx.compose.material3.NavigationBarItem
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import com.auravoice.neuralstudio.ui.studio.StudioScreen
import com.auravoice.neuralstudio.ui.dslab.DspLabScreen

@Composable
fun AuraVoiceApp() {

    var selectedTab by remember {
        mutableIntStateOf(0)
    }

    Scaffold(
        modifier = Modifier.fillMaxSize(),
        bottomBar = {
            NavigationBar {

                NavigationBarItem(
                    selected = selectedTab == 0,
                    onClick = { selectedTab = 0 },
                    icon = {
                        Icon(
                            imageVector = Icons.Default.GraphicEq,
                            contentDescription = "Studio"
                        )
                    },
                    label = {
                        Text("STUDIO")
                    }
                )

                NavigationBarItem(
                    selected = selectedTab == 1,
                    onClick = { selectedTab = 1 },
                    icon = {
                        Icon(
                            imageVector = Icons.Default.Tune,
                            contentDescription = "DSP Lab"
                        )
                    },
                    label = {
                        Text("DSP LAB")
                    }
                )
            }
        }
    ) { innerPadding ->

        when (selectedTab) {

            0 -> StudioScreen(
                modifier = Modifier.padding(innerPadding)
            )

            1 -> DspLabScreen(
                modifier = Modifier.padding(innerPadding)
            )
        }
    }
}
