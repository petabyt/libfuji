package dev.danielc.fudgecmp

import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.util.Log
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.animation.EnterTransition
import androidx.compose.animation.ExitTransition
import androidx.compose.foundation.ExperimentalFoundationApi
import androidx.compose.foundation.background
import androidx.compose.foundation.combinedClickable
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.interaction.PressInteraction
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.grid.GridCells
import androidx.compose.foundation.lazy.grid.LazyVerticalGrid
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.IconButtonDefaults
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.tooling.preview.Preview
import dev.danielc.fudgecmp.ui.theme.FudgeTheme
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.paint
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.ColorFilter
import androidx.compose.ui.graphics.RectangleShape
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalViewConfiguration
import androidx.compose.ui.res.colorResource
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import androidx.navigation.NavHostController
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.launch
import java.util.Locale

class MyModel: ViewModel() {
    init {
        viewModelScope.launch {

        }
    }
}

class State {
    var x by mutableStateOf(0)
}

object Backend {
    var mainLog by mutableStateOf("")
    var tickText by mutableStateOf("5")
    private val h = Handler(Looper.getMainLooper())
    fun log(str: String) {
        h.post { mainLog += str + "\n" }
    }
    fun tick() {
        h.post {
            tickText = (0..10).random().toString()
        }
    }
}

@Composable
fun BottomLog(modifier: Modifier, text: String): Unit {
    if (text.isNotEmpty()) {
        Text(
            text.trim(),
            fontFamily = FontFamily.Monospace,
            modifier = modifier.fillMaxWidth()
                .background(Color.Black.copy(alpha = 0.6f)).padding(5.dp)
        )
    }
}

@OptIn(ExperimentalMaterial3Api::class, ExperimentalFoundationApi::class)
@Preview(showBackground = true, device = "id:pixel_7")
@Composable
fun MainScreen(navController: NavHostController = rememberNavController()) {
    return FudgeTheme {
        Scaffold(
            topBar = {
                TopAppBar(
                    colors = TopAppBarDefaults.topAppBarColors(),
                    title = {
                        Text("Fudge 0.3.0")
                    },
                    actions = {
                        IconButton(onClick = {}) {
                            Icon(
                                painter = painterResource(R.drawable.baseline_folder_open_24),
                                contentDescription = "Localized description"
                            )
                        }
                        IconButton(onClick = {}) {
                            Icon(
                                painter = painterResource(R.drawable.baseline_settings_24),
                                contentDescription = "Localized description"
                            )
                        }
                    },
                )
            },
        ) { innerPadding ->
            var state = State()
            var isEnabled by remember { mutableStateOf(true) }
            Box(modifier = Modifier
                .padding(innerPadding)
                .fillMaxSize()
                .paint(
                    painterResource(id = R.drawable.background),
                    contentScale = ContentScale.Crop
                ) ) {
                Column(
                    modifier = Modifier.align(Alignment.Center).padding(horizontal = 20.dp)
                ) {
                    val m = Modifier.fillMaxWidth()
                    Widgets.LongClickButton(m, {
                        navController.navigate("gallery")
                    }, {
                        Backend.log("Long Press")
                    }, "Gallery")
                    Widgets.GreenButton(modifier = m, text = "Go to test suite", onClick = {
                        navController.navigate("testsuite")
                    })
                    Widgets.GrayButton(modifier = m, text = "Help", onClick = {
                        Backend.log("Hello")
                    })
                    Widgets.GrayButton(modifier = m, text = "Send Feedback", onClick = {})
                    Text(Backend.tickText)
                }
                BottomLog(Modifier.align(Alignment.BottomStart), Backend.mainLog)
            }
        }
    }
}

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        //enableEdgeToEdge()
        Thread {
            while (true) {
                Backend.tick()
                Thread.sleep(100)
            }
        }.start()
        setContent {
            val navController = rememberNavController()

            NavHost(
                enterTransition = { EnterTransition.None },
                exitTransition = { ExitTransition.None },
                navController = navController, startDestination = "home") {
                composable("home") { MainScreen(navController) }
                composable("testsuite") { TestSuite(navController) }
                composable("gallery") { GalleryScreen(navController) }
            }
        }
    }
}