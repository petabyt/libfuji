package dev.danielc.fudgecmp

import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.interaction.PressInteraction
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Icon
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.remember
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.RectangleShape
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalViewConfiguration
import androidx.compose.ui.unit.dp
import androidx.compose.ui.res.colorResource
import androidx.compose.ui.res.painterResource
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.collectLatest

object Widgets {

    @Composable
    fun longPress(onClick: () -> Unit, onLongClick: () -> Unit): MutableInteractionSource {
        val context = LocalContext.current

        val interactionSource = remember { MutableInteractionSource() }

        val viewConfiguration = LocalViewConfiguration.current

        LaunchedEffect(interactionSource) {
            var isLongClick = false

            interactionSource.interactions.collectLatest { interaction ->
                when (interaction) {
                    is PressInteraction.Press -> {
                        isLongClick = false
                        delay(viewConfiguration.longPressTimeoutMillis)
                        isLongClick = true
                        onLongClick()
                    }

                    is PressInteraction.Release -> {
                        if (isLongClick.not()) {
                            onClick()
                        }
                    }

                }
            }
        }

        return interactionSource
    }

    @Composable
    fun Button2(
        modifier: Modifier = Modifier,
        enabled: Boolean = true,
        onClick: (Int) -> Unit,
        content: @Composable () -> Unit,
        ctx: Int
    ) {
        Button(
            onClick = {
                onClick(ctx)
            },
            colors = ButtonDefaults.buttonColors(containerColor = colorResource(R.color.teal_700)),
            modifier = modifier.padding(10.dp).fillMaxWidth(),
            shape = RectangleShape,
            enabled = enabled,
        ) {
            content()
        }
    }

    @Composable
    fun GreenButton(
        modifier: Modifier = Modifier,
        enabled: Boolean = true,
        onClick: () -> Unit,
        text: String = "",
        content: @Composable () -> Unit = {Text(text, color = Color.White, modifier = Modifier.padding(5.dp))},
    ) {
        Button(
            onClick = onClick,
            colors = ButtonDefaults.buttonColors(containerColor = colorResource(R.color.go_green)),
            modifier = modifier,
            shape = RectangleShape,
            enabled = enabled,
        ) {
            content()
        }
    }

    @Composable
    fun BlueButton(
        modifier: Modifier = Modifier,
        enabled: Boolean = true,
        onClick: () -> Unit,
        content: @Composable () -> Unit,
    ) {
        Button(
            onClick = onClick,
            colors = ButtonDefaults.buttonColors(containerColor = colorResource(R.color.go_greenblue)),
            modifier = modifier,
            shape = RectangleShape,
            enabled = enabled,
        ) {
            content()
        }
    }

    @Composable
    fun GrayButton(
        modifier: Modifier = Modifier,
        enabled: Boolean = true,
        onClick: () -> Unit,
        text: String = "",
        content: @Composable () -> Unit = {Text(text, color = Color.White, modifier = Modifier.padding(5.dp))},
    ) {
        Button(
            onClick = onClick,
            colors = ButtonDefaults.buttonColors(containerColor = colorResource(R.color.light_gray)),
            modifier = modifier,
            shape = RectangleShape,
            enabled = enabled,
        ) {
            content()
        }
    }

    @Composable
    fun GrayIconButton(
        modifier: Modifier = Modifier,
        onClick: () -> Unit,
        content: @Composable () -> Unit,
    ) {
        Button(
            shape = RectangleShape,
            modifier = modifier.size(50.dp),
            onClick = {},
            colors = ButtonDefaults.buttonColors(containerColor = colorResource(R.color.gray)),
            contentPadding = PaddingValues(0.dp)
        ) {
            content()
        }
    }

    @Composable
    fun LongClickButton(
        modifier: Modifier = Modifier,
        onClick: () -> Unit,
        onLongClick: () -> Unit,
        text: String = "",
        content: @Composable () -> Unit = {Text(text, color = Color.White, modifier = Modifier.padding(5.dp))},
    ) {
        val longPress = longPress(onClick = onClick, onLongClick = onLongClick)
        Button(
            colors = ButtonDefaults.buttonColors(containerColor = colorResource(R.color.go_greenblue)),
            shape = RectangleShape,

            modifier = modifier,
            interactionSource = longPress,
            onClick = {}
        ) {
            content()
        }
    }
}