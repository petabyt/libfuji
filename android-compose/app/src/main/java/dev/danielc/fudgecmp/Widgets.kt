package dev.danielc.fudgecmp

import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.RectangleShape
import androidx.compose.ui.unit.dp

object Widgets {
    @Composable
    fun Button(
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
            modifier = modifier.padding(10.dp).fillMaxWidth(),
            shape = RectangleShape,
            enabled = enabled,
        ) {
            content()
        }
    }
}