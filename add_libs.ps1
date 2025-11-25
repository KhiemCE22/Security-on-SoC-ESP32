Write-Host "========== ADDING ARDUINO LIBRARIES FOR ESP-IDF ==========" -ForegroundColor Cyan

# Move to project root
cd $PSScriptRoot

# Create components folder if not exists
if (!(Test-Path components)) {
    mkdir components
}
cd components

function Add-Lib($name, $url, $srcs) {
    if (!(Test-Path $name)) {
        git clone $url
    } else {
        Write-Host "$name already exists"
    }

    cd $name
    Write-Host "Creating CMakeLists.txt for $name..."

    $cmake = @"
idf_component_register(
    SRCS $srcs
    INCLUDE_DIRS "."
)
"@

    $cmake | Out-File -Encoding ASCII CMakeLists.txt

    cd ..
}

# 1. Adafruit NeoPixel
Add-Lib "Adafruit_NeoPixel" `
        "https://github.com/adafruit/Adafruit_NeoPixel.git" `
        '"Adafruit_NeoPixel.cpp"'

# 2. DHT20
Add-Lib "DHT20" `
        "https://github.com/RobTillaart/DHT20.git" `
        '"DHT20.cpp"'

# 3. LiquidCrystal_I2C
Add-Lib "LiquidCrystal_I2C" `
        "https://github.com/johnrickman/LiquidCrystal_I2C.git" `
        '"LiquidCrystal_I2C.cpp"'

# 4. IRremote
Add-Lib "IRremote" `
        "https://github.com/Arduino-IRremote/Arduino-IRremote.git" `
        '"*.cpp"'

# 5. ESP32Servo
Add-Lib "ESP32Servo" `
        "https://github.com/madhephaestus/ESP32Servo.git" `
        '"*.cpp"'

# 6. PubSubClient
Add-Lib "PubSubClient" `
        "https://github.com/knolleary/pubsubclient.git" `
        '"PubSubClient.cpp"'

# 7. ArduinoJson
Add-Lib "ArduinoJson" `
        "https://github.com/bblanchon/ArduinoJson.git" `
        '"*.cpp"'

Write-Host "========== DONE ==========" -ForegroundColor Green
Write-Host "Now run:  rmdir /s /q build && idf.py build"
