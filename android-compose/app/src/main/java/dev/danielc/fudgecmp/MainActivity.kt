package dev.danielc.fudgecmp

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.text.BasicText
import androidx.compose.material3.Button
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.tooling.preview.Preview
import dev.danielc.fudgecmp.ui.theme.FudgeTheme
import androidx.compose.runtime.*
import androidx.compose.ui.graphics.Color

fun test(x: Int) {

}

@Composable
fun AppButton(
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
        modifier = modifier,
        enabled = enabled,
    ) {
        content()
    }
}

@Preview(showBackground = true, device = "id:pixel_7")
@Composable
fun MainScreen() {
    var isEnabled by remember { mutableStateOf(true) }
    return FudgeTheme {
        Box(modifier = Modifier.fillMaxSize().background(color = Color.DarkGray)) {
            Column {
                Widgets.Button(ctx = 1, onClick = {
                    isEnabled = !isEnabled
                }, content = {Text("Disable button below")})
                Widgets.Button(ctx = 1, enabled = isEnabled, onClick = {}, content = {Text("Hello")})

                BasicText("hello")
            }
        }
    }
}

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            FudgeTheme {
                MainScreen()
            }
        }
    }
}