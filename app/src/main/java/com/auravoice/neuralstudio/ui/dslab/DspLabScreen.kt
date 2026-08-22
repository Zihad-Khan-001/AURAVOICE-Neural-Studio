package com.auravoice.neuralstudio.ui.dslab

import androidx.compose.foundation.Canvas
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
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.ExposedDropdownMenuBox
import androidx.compose.material3.ExposedDropdownMenuDefaults
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Slider
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

private val Background = Color(0xFF0F0F12)
private val CardColor = Color(0xFF1C1C1E)
private val Cyan = Color(0xFF00E5CC)
private val Blue = Color(0xFF0A84FF)

@Composable
fun DspLabScreen(
    modifier: Modifier = Modifier
) {
    var inputGain by remember { mutableFloatStateOf(0f) }
    var outputGain by remember { mutableFloatStateOf(0f) }

    Column(
        modifier = modifier
            .fillMaxSize()
            .background(Background)
            .padding(horizontal = 14.dp, vertical = 10.dp)
    ) {

        Text(
            text = "DSP LAB",
            color = Color.White,
            fontSize = 23.sp
        )

        Text(
            text = "NAISHABDA MASTERING ENGINE",
            color = Cyan,
            fontSize = 11.sp
        )

        Spacer(modifier = Modifier.height(10.dp))

        EngineProfileCard()

        Spacer(modifier = Modifier.height(8.dp))

        EqGraphCard()

        Spacer(modifier = Modifier.height(8.dp))

        GainCard(
            title = "INPUT GAIN",
            value = inputGain,
            onValueChange = { inputGain = it }
        )

        Spacer(modifier = Modifier.height(6.dp))

        GainCard(
            title = "OUTPUT TRIM",
            value = outputGain,
            onValueChange = { outputGain = it }
        )

        Spacer(modifier = Modifier.height(8.dp))

        ProcessingSummaryCard()
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun EngineProfileCard() {

    var expanded by remember { mutableStateOf(false) }

    val profile = "Naishabda 21Y Male Poetry Master"

    Card(
        modifier = Modifier.fillMaxWidth(),
        colors = CardDefaults.cardColors(
            containerColor = CardColor
        ),
        shape = RoundedCornerShape(14.dp)
    ) {

        Column(
            modifier = Modifier.padding(10.dp)
        ) {

            Text(
                text = "ENGINE PROFILE",
                color = Color.White,
                fontSize = 11.sp
            )

            Spacer(modifier = Modifier.height(4.dp))

            ExposedDropdownMenuBox(
                expanded = expanded,
                onExpandedChange = {
                    expanded = !expanded
                }
            ) {

                OutlinedTextField(
                    value = profile,
                    onValueChange = {},
                    readOnly = true,
                    modifier = Modifier
                        .fillMaxWidth()
                        .menuAnchor(),
                    trailingIcon = {
                        ExposedDropdownMenuDefaults.TrailingIcon(
                            expanded = expanded
                        )
                    },
                    singleLine = true
                )

                DropdownMenu(
                    expanded = expanded,
                    onDismissRequest = {
                        expanded = false
                    }
                ) {

                    DropdownMenuItem(
                        text = {
                            Text("Naishabda 21Y Male Poetry Master")
                        },
                        onClick = {
                            expanded = false
                        }
                    )
                }
            }
        }
    }
}

@Composable
private fun EqGraphCard() {

    Card(
        modifier = Modifier.fillMaxWidth(),
        colors = CardDefaults.cardColors(
            containerColor = CardColor
        ),
        shape = RoundedCornerShape(14.dp)
    ) {

        Column(
            modifier = Modifier.padding(10.dp)
        ) {

            Text(
                text = "ACOUSTIC TRANSFER EQ",
                color = Color.White,
                fontSize = 11.sp
            )

            Spacer(modifier = Modifier.height(6.dp))

            Canvas(
                modifier = Modifier
                    .fillMaxWidth()
                    .height(150.dp)
            ) {

                val width = size.width
                val height = size.height

                val gridStepX = width / 6f
                val gridStepY = height / 6f

                for (i in 1..5) {

                    drawLine(
                        color = Color.DarkGray,
                        start = Offset(
                            i * gridStepX,
                            0f
                        ),
                        end = Offset(
                            i * gridStepX,
                            height
                        ),
                        strokeWidth = 1f
                    )

                    drawLine(
                        color = Color.DarkGray,
                        start = Offset(
                            0f,
                            i * gridStepY
                        ),
                        end = Offset(
                            width,
                            i * gridStepY
                        ),
                        strokeWidth = 1f
                    )
                }

                val curve = Path()

                curve.moveTo(
                    0f,
                    height * 0.62f
                )

                curve.cubicTo(
                    width * 0.15f,
                    height * 0.64f,
                    width * 0.20f,
                    height * 0.52f,
                    width * 0.32f,
                    height * 0.48f
                )

                curve.cubicTo(
                    width * 0.45f,
                    height * 0.45f,
                    width * 0.55f,
                    height * 0.55f,
                    width * 0.65f,
                    height * 0.43f
                )

                curve.cubicTo(
                    width * 0.76f,
                    height * 0.34f,
                    width * 0.88f,
                    height * 0.40f,
                    width,
                    height * 0.28f
                )

                drawPath(
                    path = curve,
                    color = Cyan,
                    style = androidx.compose.ui.graphics.drawscope.Stroke(
                        width = 4f
                    )
                )
            }

            Spacer(modifier = Modifier.height(4.dp))

            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween
            ) {
                Text("80 Hz", color = Color.LightGray, fontSize = 9.sp)
                Text("250 Hz", color = Color.LightGray, fontSize = 9.sp)
                Text("450 Hz", color = Color.LightGray, fontSize = 9.sp)
                Text("3.4 kHz", color = Color.LightGray, fontSize = 9.sp)
                Text("10 kHz", color = Color.LightGray, fontSize = 9.sp)
            }
        }
    }
}

@Composable
private fun GainCard(
    title: String,
    value: Float,
    onValueChange: (Float) -> Unit
) {

    Card(
        modifier = Modifier.fillMaxWidth(),
        colors = CardDefaults.cardColors(
            containerColor = CardColor
        ),
        shape = RoundedCornerShape(12.dp)
    ) {

        Column(
            modifier = Modifier.padding(
                horizontal = 10.dp,
                vertical = 6.dp
            )
        ) {

            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween
            ) {

                Text(
                    text = title,
                    color = Color.White,
                    fontSize = 10.sp
                )

                Text(
                    text = String.format("%+.1f dB", value),
                    color = Cyan,
                    fontSize = 10.sp
                )
            }

            Slider(
                value = value,
                onValueChange = onValueChange,
                valueRange = -12f..12f
            )
        }
    }
}

@Composable
private fun ProcessingSummaryCard() {

    Card(
        modifier = Modifier.fillMaxWidth(),
        colors = CardDefaults.cardColors(
            containerColor = CardColor
        ),
        shape = RoundedCornerShape(12.dp)
    ) {

        Column(
            modifier = Modifier.padding(10.dp)
        ) {

            Text(
                text = "MASTERING CHAIN",
                color = Color.White,
                fontSize = 11.sp
            )

            Spacer(modifier = Modifier.height(5.dp))

            Text(
                text = "Neural Noise Suppression  •  Active",
                color = Cyan,
                fontSize = 10.sp
            )

            Text(
                text = "Dynamic De-Esser  •  Active",
                color = Cyan,
                fontSize = 10.sp
            )

            Text(
                text = "Soft-Knee Compressor  •  3.2:1",
                color = Color.LightGray,
                fontSize = 10.sp
            )

            Text(
                text = "True-Peak Limiter  •  -1.5 dBTP",
                color = Color.LightGray,
                fontSize = 10.sp
            )

            Text(
                text = "Target Loudness  •  -14.0 LUFS",
                color = Color.LightGray,
                fontSize = 10.sp
            )
        }
    }
}
