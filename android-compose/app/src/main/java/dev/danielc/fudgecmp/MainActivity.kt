package dev.danielc.fudgecmp

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.background
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
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.res.colorResource
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.navigation.NavHostController
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import java.util.Locale

class State {
    var x by mutableStateOf(0)
}

@Composable
fun BottomLog(modifier: Modifier, text: String): Unit {
    if (text.isNotEmpty()) {
        Text(
            text,
            fontFamily = FontFamily.Monospace,
            modifier = modifier.fillMaxWidth()
                .background(Color.Black.copy(alpha = 0.6f)).padding(5.dp)
        )
    }
}

@OptIn(ExperimentalMaterial3Api::class)
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
                    modifier = Modifier.fillMaxSize()
                ) {
                    Widgets.GreenButton(text = "Go to gallery", onClick = {
                        navController.navigate("gallery")
                    })
                    Widgets.GreenButton(text = "Go to test suite", onClick = {
                        navController.navigate("testsuite")
                    })
                }
                Text(
                    "asd",
                    fontFamily = FontFamily.Monospace,
                    modifier = Modifier.align(Alignment.BottomStart).fillMaxWidth()
                        .background(Color.Black.copy(alpha = 0.6f)).padding(5.dp)
                )
            }
        }
    }
}

@Composable
fun GalleryMenu(navController: NavHostController, innerPadding: PaddingValues) {
    var state = State()
    var isEnabled by remember { mutableStateOf(true) }
    Box(
        modifier = Modifier
            .padding(innerPadding)
            .fillMaxSize()
    ) {
        Column {
            Row {
                Widgets.GrayIconButton(
                    modifier = Modifier.size(50.dp),
                    onClick = {

                    },
                ) {
                    Icon(
                        tint = Color.White,
                        painter = painterResource(R.drawable.baseline_grid_view_24),
                        contentDescription = "Grid View"
                    )
                }
                Widgets.GrayIconButton(
                    modifier = Modifier.size(50.dp),
                    onClick = {},
                ) {
                    Icon(
                        tint = Color.White,
                        painter = painterResource(R.drawable.baseline_view_list_24),
                        contentDescription = "List View"
                    )
                }
            }

            val colors = listOf(
                Color.Red, Color.Green, Color.Blue,
                Color.Cyan, Color.Magenta, Color.Yellow,
                Color.Gray, Color.LightGray, Color.DarkGray,
            )

            LazyVerticalGrid(
                columns = GridCells.Fixed(4),
            ) {
                items(100) { index ->
                    Box(
                        modifier = Modifier
                            .aspectRatio(1f)
                            .background(colors[index % colors.size])
                    )
                }
            }
        }
        Text(
            "asd",
            fontFamily = FontFamily.Monospace,
            modifier = Modifier.align(Alignment.BottomStart).fillMaxWidth()
                .background(Color.Black.copy(alpha = 0.6f))
        )
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Preview(showBackground = true, device = "id:pixel_7")
@Composable
fun GalleryScreen(navController: NavHostController = rememberNavController()) {
    return FudgeTheme {
        Scaffold(
            topBar = {
                TopAppBar(
                    colors = TopAppBarDefaults.topAppBarColors(),
                    title = {
                        Text("Gallery")
                    },
                    navigationIcon = {
                        IconButton(onClick = {
                            navController.navigateUp()
                        }) {
                            Icon(
                                imageVector = Icons.AutoMirrored.Filled.ArrowBack,
                                contentDescription = "Localized description"
                            )
                        }
                    },
                )
            },
        ) { innerPadding ->
            GalleryMenu(navController, innerPadding)
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Preview(showBackground = true, device = "id:pixel_7", name = "asd")
@Composable
fun TestSuite(navController: NavHostController = rememberNavController()) {
    return FudgeTheme {
        Scaffold(
            topBar = {
                TopAppBar(
                    title = {
                        Text("Test Suite")
                    },
                    navigationIcon = {
                        IconButton(onClick = {

                        }) {
                            Icon(
                                imageVector = Icons.AutoMirrored.Filled.ArrowBack,
                                contentDescription = "Localized description"
                            )
                        }
                    },
                    actions = {
                        IconButton(onClick = {

                        }) {
                            Icon(
                                painter = painterResource(R.drawable.baseline_content_copy_24),
                                contentDescription = "Localized description"
                            )
                        }
                    }
                )
            },
        ) { innerPadding ->
            Column(
                modifier = Modifier.padding(innerPadding)
            ) {
                Row {
                    val m = Modifier.weight(1f).padding(5.dp)
                    val color = colorResource(R.color.white)
                    Widgets.GreenButton(modifier = m, onClick = {}, content = {Text("Connect", color = color)})
                    Widgets.BlueButton(modifier = m, onClick = {}, content = {
                        Row(
                            modifier = Modifier.fillMaxWidth(),
                            horizontalArrangement = Arrangement.SpaceEvenly,
                            verticalAlignment = Alignment.CenterVertically
                        ) {
                            Text("Select WiFi", color = color)
                            Icon(
                                painter = painterResource(R.drawable.baseline_wifi_tethering_24),
                                tint = color,
                                contentDescription = "asd"
                            )
                        }
                    })
                }
                Column(
                    modifier = Modifier.fillMaxHeight().fillMaxWidth().padding(5.dp)
                        .verticalScroll(rememberScrollState())
                ) {
                    var x: String = "";
                    for (i in 0..100) {
                        x += String.format(Locale.US, "Testing %d\n", i)
                    }
                    Text(
                        text = x,
                        style = TextStyle(
                            fontFamily = FontFamily.Monospace,
                            fontSize = 14.sp
                        )
                    )
                }
            }
        }
    }
}

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            val navController = rememberNavController()

            NavHost(navController = navController, startDestination = "home") {
                composable("home") { MainScreen(navController) }
                composable("testsuite") { TestSuite(navController) }
                composable("gallery") { GalleryScreen(navController) }
            }
        }
    }
}