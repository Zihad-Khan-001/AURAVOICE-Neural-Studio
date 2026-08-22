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
import androidx.compose.material.icons.filled.FiberManualRecord
import androidx.compose.material.icons.filled.Pause
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material.icons.filled.Stop
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.Slider
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableLongStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.auravoice.neuralstudio.audio.AudioController
import kotlinx.coroutines.delay

private val StudioBackground = Color(0xFF0F0F12)
private val StudioCard = Color(0xFF1C1C1E)
private val CoralRed = Color(0xFFFF4757)
private val NeonCyan = Color(0xFF00E5CC)

@Composable
fun StudioScreen(
    modifier: Modifier = Modifier
) {
    val context = LocalContext.current

    val controller = remember {
        AudioController(context)
    }

    var isRecording by remember {
        mutableStateOf(false)
    }

    var isPaused by remember {
        mutableStateOf(false)
    }

    var inputGain by remember {
        mutableFloatStateOf(1.0f)
    }

    var elapsedMillis by remember {
        mutableLongStateOf(0L)
    }

    var peakLevel by remember {
        mutableFloatStateOf(0f)
    }

    DisposableEffect(Unit) {
        onDispose {
            controller.release()
        }
    }

    LaunchedEffect(isRecording, isPaused) {
        while (isRecording && !isPaused) {
            delay(50)

            peakLevel = controller.getPeakLevel()
            elapsedMillis += 50
        }
    }

    Surface(
        modifier = modifier.fillMaxSize(),
        color = StudioBackground
    ) {
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(
                    horizontal = 16.dp,
                    vertical = 12.dp
                ),
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
                fontSize = 12.sp
            )

            Spacer(
                modifier = Modifier.height(16.dp)
            )

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
                        text = formatTime(elapsedMillis),
                        color = Color.White,
                        fontSize = 36.sp,
                        fontWeight = FontWeight.Light
                    )

                    Spacer(
                        modifier = Modifier.height(12.dp)
                    )

                    AudioVisualizer(
                        level = peakLevel
                    )

                    Spacer(
                        modifier = Modifier.height(12.dp)
                    )

                    Text(
                        text = "48 kHz • 24-bit • PCM",
                        color = Color.LightGray,
                        fontSize = 12.sp
                    )
                }
            }

            Spacer(
                modifier = Modifier.height(12.dp)
            )

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

            Spacer(
                modifier = Modifier.height(12.dp)
            )

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
                            controller.setInputGain(it)
                        },
                        valueRange = 0f..2f
                    )

                    Text(
                        text = String.format(
                            "%.2fx",
                            inputGain
                        ),
                        color = NeonCyan,
                        fontSize = 12.sp
                    )
                }
            }

            Spacer(
                modifier = Modifier.height(14.dp)
            )

            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.Center,
                verticalAlignment = Alignment.CenterVertically
            ) {

                IconButton(
                    onClick = {
                        if (isRecording && !isPaused) {
                            controller.pauseRecording()
                            isPaused = true
                        }
                    }
                ) {
                    Icon(
                        imageVector = Icons.Default.Pause,
                        contentDescription = "Pause",
                        tint = Color.White
                    )
                }

                Spacer(
                    modifier = Modifier.width(20.dp)
                )

                IconButton(
                    modifier = Modifier.background(
                        color = CoralRed,
                        shape = RoundedCornerShape(50)
                    ),
                    onClick = {

                        if (!isRecording) {

                            val started =
                                controller.startRecording()

                            if (started) {
                                isRecording = true
                                isPaused = false
                                elapsedMillis = 0L
                            }

                        } else if (isPaused) {

                            val resumed =
                                controller.resumeRecording()

                            if (resumed) {
                                isPaused = false
                            }
                        }
                    }
                ) {
                    Icon(
                        imageVector =
                            if (isPaused) {
                                Icons.Default.PlayArrow
                            } else {
                                Icons.Default.FiberManualRecord
                            },
                        contentDescription = "Record",
                        tint = Color.White
                    )
                }

                Spacer(
                    modifier = Modifier.width(20.dp)
                )

                IconButton(
                    onClick = {
                        controller.stopRecording()

                        isRecording = false
                        isPaused = false
                        elapsedMillis = 0L
                    }
                ) {
                    Icon(
                        imageVector = Icons.Default.Stop,
                        contentDescription = "Stop",
                        tint = Color.White
                    )
                }
            }

            Spacer(
                modifier = Modifier.height(12.dp)
            )

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

                    Spacer(
                        modifier = Modifier.height(8.dp)
                    )

                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.Center
                    ) {

                        Button(
                            onClick = {}
                        ) {
                            Text("RAW AUDIO")
                        }

                        Spacer(
                            modifier = Modifier.width(8.dp)
                        )

                        Button(
                            onClick = {}
                        ) {
                            Text("MASTERED")
                        }
                    }
                }
            }
        }
    }
}

@Composable
private fun AudioVisualizer(
    level: Float
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .height(90.dp),
        horizontalArrangement = Arrangement.SpaceEvenly,
        verticalAlignment = Alignment.CenterVertically
    ) {

        repeat(64) { index ->

            val variation =
                ((index * 37) % 100) / 100f

            val normalizedLevel =
                level.coerceIn(0f, 1f)

            val barHeight =
                (
                    12f +
                    normalizedLevel * 62f *
                    (0.45f + variation * 0.55f)
                ).coerceAtMost(78f)

            Spacer(
                modifier = Modifier
                    .width(3.dp)
                    .height(barHeight.dp)
                    .background(
                        color = NeonCyan,
                        shape = RoundedCornerShape(4.dp)
                    )
            )
        }
    }
}

private fun formatTime(
    milliseconds: Long
): String {

    val totalSeconds =
        milliseconds / 1000

    val millis =
        milliseconds % 1000

    val minutes =
        totalSeconds / 60

    val seconds =
        totalSeconds % 60

    return String.format(
        "%02d:%02d.%03d",
        minutes,
        seconds,
        millis
    )
}
