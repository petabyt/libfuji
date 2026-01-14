package dev.danielc.fudgecmp

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.lazy.grid.GridCells
import androidx.compose.foundation.lazy.grid.LazyVerticalGrid
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.navigation.NavHostController
import androidx.navigation.compose.rememberNavController
import dev.danielc.fudgecmp.ui.theme.FudgeTheme

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