package dev.danielc.fudgecmp

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.HelpOutline
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.material.icons.filled.Wifi
import androidx.compose.material.icons.filled.Usb
import androidx.compose.material.icons.filled.QuestionMark
import androidx.compose.material.icons.filled.Email // Using Email for feedback icon as no specific icon is available.

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun FudgeUi() {
    Scaffold(
        topBar = {
            TopAppBar(
                title = {
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Text(text = "Fudge 0.2.0 (beta)", color = Color.White, fontSize = 20.sp)
                    }
                },
                actions = {
                    IconButton(onClick = { /* Handle folder click */ }) {
                        Icon(
                            imageVector = Icons.Default.Settings, // Using settings for the folder icon, as no exact match.
                            contentDescription = "Folder",
                            tint = Color.White
                        )
                    }
                    IconButton(onClick = { /* Handle settings click */ }) {
                        Icon(
                            imageVector = Icons.Default.Settings,
                            contentDescription = "Settings",
                            tint = Color.White
                        )
                    }
                },
                colors = TopAppBarDefaults.topAppBarColors(containerColor = Color(0xFF2C2C2C)) // Dark grey from image
            )
        },
        content = { paddingValues ->
            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(paddingValues)
                    .background(Color.Black) // Main background as black
                    .padding(horizontal = 16.dp)
            ) {
                Spacer(modifier = Modifier.height(16.dp))

                Text(
                    text = "Courtesy of thevibrantmachine",
                    color = Color.LightGray,
                    fontSize = 12.sp,
                    modifier = Modifier.align(Alignment.End)
                )

                Spacer(modifier = Modifier.height(24.dp))

                // Connect to WiFi Button
                ConnectionButton(
                    text = "CONNECT TO WIFI",
                    icon = Icons.Default.Wifi,
                    backgroundColor = Color(0xFF4CAF50), // Green from image
                    onClick = { /* Handle connect to WiFi click */ }
                )
                Spacer(modifier = Modifier.height(12.dp))

                // Connect to USB Button
                ConnectionButton(
                    text = "CONNECT TO USB",
                    icon = Icons.Default.Usb,
                    backgroundColor = Color(0xFF2E7D32), // Darker green from image
                    onClick = { /* Handle connect to USB click */ }
                )
                Spacer(modifier = Modifier.height(24.dp))

                // Looking for a camera section
                LookingForCameraSection()

                Spacer(modifier = Modifier.height(24.dp))

                // Help Button
                HelpFeedbackButton(
                    text = "HELP",
                    icon = Icons.Default.HelpOutline,
                    onClick = { /* Handle help click */ }
                )
                Spacer(modifier = Modifier.height(12.dp))

                // Send Feedback Button
                HelpFeedbackButton(
                    text = "SEND FEEDBACK",
                    icon = Icons.Default.Email, // Using Email icon as a placeholder for feedback
                    onClick = { /* Handle send feedback click */ }
                )
            }
        }
    )
}

@Composable
fun ConnectionButton(
    text: String,
    icon: androidx.compose.ui.graphics.vector.ImageVector,
    backgroundColor: Color,
    onClick: () -> Unit
) {
    Button(
        onClick = onClick,
        modifier = Modifier
            .fillMaxWidth()
            .height(56.dp),
        colors = ButtonDefaults.buttonColors(containerColor = backgroundColor),
        shape = RoundedCornerShape(8.dp) // Slightly rounded corners
    ) {
        Row(
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.SpaceBetween,
            modifier = Modifier.fillMaxWidth()
        ) {
            Text(text = text, color = Color.White, fontSize = 16.sp)
            Icon(imageVector = icon, contentDescription = null, tint = Color.White)
        }
    }
}

@Composable
fun LookingForCameraSection() {
    Column(
        modifier = Modifier
            .fillMaxWidth()
            .background(Color(0xFF2C2C2C), shape = RoundedCornerShape(8.dp)) // Dark grey background
            .padding(16.dp)
    ) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.SpaceBetween
        ) {
            Text(text = "Looking for a camera...", color = Color.White, fontSize = 16.sp)
            Icon(imageVector = Icons.Default.QuestionMark, contentDescription = null, tint = Color.White)
        }
        Spacer(modifier = Modifier.height(8.dp))
        HorizontalDivider(thickness = 1.dp, color = Color.DarkGray)
        Spacer(modifier = Modifier.height(8.dp))
        Text(text = "• PC AutoSave", color = Color.LightGray, fontSize = 14.sp)
        Spacer(modifier = Modifier.height(4.dp))
        Text(text = "• Wireless Tether Shooting", color = Color.LightGray, fontSize = 14.sp)
    }
}

@Composable
fun HelpFeedbackButton(
    text: String,
    icon: androidx.compose.ui.graphics.vector.ImageVector,
    onClick: () -> Unit
) {
    Button(
        onClick = onClick,
        modifier = Modifier
            .fillMaxWidth()
            .height(56.dp),
        colors = ButtonDefaults.buttonColors(containerColor = Color(0xFF616161)), // Grey from image
        shape = RoundedCornerShape(8.dp)
    ) {
        Row(
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.SpaceBetween,
            modifier = Modifier.fillMaxWidth()
        ) {
            Text(text = text, color = Color.White, fontSize = 16.sp)
            Icon(imageVector = icon, contentDescription = null, tint = Color.White)
        }
    }
}

@Preview(showBackground = true)
@Composable
fun PreviewFudgeUi() {
    FudgeUi()
}

class TestActivity {
}