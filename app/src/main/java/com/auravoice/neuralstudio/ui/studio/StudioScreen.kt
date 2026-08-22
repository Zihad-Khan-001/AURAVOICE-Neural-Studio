package com.auravoice.neuralstudio.ui.studio

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Pause
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material.icons.filled.Stop
import androidx.compose.material.icons.filled.FiberManualRecord
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Slider
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

private val StudioBackground = Color(0xFF0F0F12)
private val StudioCard = Color(0xFF1C1C1E)
private val CoralRed = Color(0xFFFF4757)
private val NeonCyan = Color(0xFF00E5CC)
private val SystemBlue = Color(0xFF0A84FF)

@Composable
fun StudioScreen(
    modifier: Modifier = Modifier
) {
    var isRecording by remember { mutableStateOf(false) }
    var isPaused by remember { mutableStateOf(false) }
    var playbackMode by remember { mutableIntStateOf(0) }
    var inputGain by remember { mutableFloatStateOf(1.0f) }

    Surface(
        modifier = modifier.fillMaxSize(),
        color = StudioBackground
    ) {
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(horizontal = 16.dp, vertical = 12.dp),
            horizontalAlignment = Alignment.CenterHorizontally
        ) {

            Text(
                text = "AURAVOICE",
                color = Color.White,
                fontSize = 24.sp,
                fontWeight = FontWeight.Bold
            )

            Text(
                text = "NEURAL STUDIO",
                color = NeonCyan,
                fontSize = 12.sp,
                fontWeight = FontWeight.Medium
            )

            Spacer(modifier = Modifier.height(16.dp))

            Card(
                modifier = Modifier.fillMaxWidth(),
                colors = CardDefaults.cardColors(
                    containerColor = StudioCard
                ),
                shape = RoundedCornerShape(16.dp)
            ) {
                Column(
                    modifier = Modifier.padding(16.dp),
                    horizontalAlignment = Alignment.CenterHorizontally
                ) {

                    Text(
                        text = "00:00.000",
                        color = Color.White,
                        fontSize = 36.sp,
                        fontWeight = FontWeight.Light
                    )

                    Spacer(modifier = Modifier.height(12.dp))

                    AudioVisualizerPlaceholder()

                    Spacer(modifier = Modifier.height(12.dp))

                    Text(
                        text = "48 kHz • 24-bit • PCM",
                        color = Color.LightGray,
                        fontSize = 12.sp
                    )
                }
            }

            Spacer(modifier = Modifier.height(12.dp))

            Surface(
                shape = RoundedCornerShape(50.dp),
                color = StudioCard
            ) {
                Text(
                    text = "Boya BY-M1 Profile Calibrated",
                    color = NeonCyan,
                    fontSize = 12.sp,
                    fontWeight = FontWeight.Medium,
                    modifier = Modifier.padding(
                        horizontal = 14.dp,
                        vertical = 8.dp
                    )
                )
            }

            Spacer(modifier = Modifier.height(12.dp))

            Card(
                modifier = Modifier.fillMaxWidth(),
                colors = CardDefaults.cardColors(
                    containerColor = StudioCard
                ),
                shape = RoundedCornerShape(16.dp)
            ) {
                Column(
                    modifier = Modifier.padding(14.dp)
                ) {

                    Text(
                        text = "INPUT GAIN",
                        color = Color.White,
                        fontSize = 12.sp,
                        fontWeight = FontWeight.Bold
                    )

                    Slider(
                        value = inputGain,
                        onValueChange = {
                            inputGain = it
                        },
                        valueRange = 0f..2f
                    )

                    Text(
                        text = String.format("%.2fx", inputGain),
                        color = NeonCyan,
                        fontSize = 12.sp
                    )
                }
            }

            Spacer(modifier = Modifier.height(14.dp))

            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.Center,
                verticalAlignment = Alignment.CenterVertically
            ) {

                IconButton(
                    onClick = {
                        isPaused = !isPaused
                    }
                ) {
                    Icon(
                        imageVector = if (isPaused) {
                            Icons.Default.PlayArrow
                        } else {
                            Icons.Default.Pause
                        },
                        contentDescription = "Pause or Resume",
                        tint = Color.White
                    )
                }

                Spacer(modifier = Modifier.width(20.dp))

                IconButton(
                    modifier = Modifier
                        .background(
                            color = CoralRed,
                            shape = RoundedCornerShape(50)
                        ),
                    onClick = {
                        isRecording = !isRecording
                        if (!isRecording) {
                            isPaused = false
                        }
                    }
                ) {
                    Icon(
                        imageVector = Icons.Default.FiberManualRecord,
                        contentDescription = "Record",
                        tint = Color.White
                    )
                }

                Spacer(modifier = Modifier.width(20.dp))

                IconButton(
                    onClick = {
                        isRecording = false
                        isPaused = false
                    }
                ) {
                    Icon(
                        imageVector = Icons.Default.Stop,
                        contentDescription = "Stop",
                        tint = Color.White
                    )
                }
            }

            Spacer(modifier = Modifier.height(12.dp))

            Card(
                modifier = Modifier.fillMaxWidth(),
                colors = CardDefaults.cardColors(
                    containerColor = StudioCard
                ),
                shape = RoundedCornerShape(16.dp)
            ) {
                Column(
                    modifier = Modifier.padding(14.dp)
                ) {

                    Text(
                        text = "QUICK PLAYER",
                        color = Color.White,
                        fontSize = 12.sp,
                        fontWeight = FontWeight.Bold
                    )

                    Spacer(modifier = Modifier.height(8.dp))

                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.Center
                    ) {

                        Button(
                            onClick = {
                                playbackMode = 0
                            }
                        ) {
                            Text("RAW AUDIO")
                        }

                        Spacer(modifier = Modifier.width(8.dp))

                        Button(
                            onClick = {
                                playbackMode = 1
                            }
                        ) {
                            Text("MASTERED")
                        }
                    }

                    Spacer(modifier = Modifier.height(6.dp))

                    Text(
                        text = if (playbackMode == 0) {
                            "RAW AUDIO • BYPASS"
                        } else {
                            "MASTERED AUDIO"
                        },
                        color = if (playbackMode == 0) {
                            Color.LightGray
                        } else {
                            NeonCyan
                        },
                        fontSize = 12.sp,
                        modifier = Modifier.align(Alignment.CenterHorizontally)
                    )
                }
            }
        }
    }
}

@Composable
private fun AudioVisualizerPlaceholder() {

    Row(
        modifier = Modifier
            .fillMaxWidth()
            .height(90.dp),
        horizontalArrangement = Arrangement.SpaceEvenly,
        verticalAlignment = Alignment.CenterVertically
    ) {

        repeat(64) { index ->

            val height = when {
                index % 7 == 0 -> 64.dp
                index % 5 == 0 -> 48.dp
                index % 3 == 0 -> 34.dp
                else -> 22.dp
            }

            Spacer(
                modifier = Modifier
                    .width(3.dp)
                    .height(height)
                    .background(
                        color = NeonCyan,
                        shape = RoundedCornerShape(4.dp)
                    )
            )
        }
    }
}
