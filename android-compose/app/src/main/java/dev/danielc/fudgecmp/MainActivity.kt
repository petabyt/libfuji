package dev.danielc.fudgecmp

import android.annotation.SuppressLint
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.annotation.StringRes
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.text.BasicText
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material3.Button
import androidx.compose.material3.CenterAlignedTopAppBar
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.tooling.preview.Preview
import dev.danielc.fudgecmp.ui.theme.FudgeTheme
import androidx.compose.runtime.*
import androidx.compose.ui.draw.paint
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.res.painterResource
import androidx.navigation.compose.rememberNavController

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


@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun TopBarNavigationExample(
    navigateBack: () -> Unit,
) {
    Scaffold(
        topBar = {
            TopAppBar(
                title = {
                    Text(
                        "Navigation example",
                    )
                },
                navigationIcon = {
                    IconButton(onClick = navigateBack) {
                        Icon(
                            imageVector = Icons.AutoMirrored.Filled.ArrowBack,
                            contentDescription = "Localized description"
                        )
                    }
                },
            )
        },
    ) { innerPadding ->
        Box(modifier = Modifier.padding(innerPadding).fillMaxSize().paint(
            painterResource(id = R.drawable.background),
            contentScale = ContentScale.Crop),) {

        }
    }
}

class State {
    var x by mutableStateOf(0)
}

@Preview(showBackground = true, device = "id:pixel_7")
@Composable
fun MainScreen() {
    val navController = rememberNavController()
    var state = State()
    var isEnabled by remember { mutableStateOf(true) }
    return FudgeTheme {
        TopBarNavigationExample({})
//        Box(modifier = Modifier.fillMaxSize().paint(
//            painterResource(id = R.drawable.background),
//            contentScale = ContentScale.Crop) ) {
//            Column {
//                Widgets.Button(ctx = 1, onClick = {
//                    isEnabled = !isEnabled
//                }, content = {Text("Disable button below")})
//                Widgets.Button(ctx = 1, enabled = isEnabled, onClick = {}, content = {Text("Hello")})
//
//                BasicText("hello")
//            }
//        }
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