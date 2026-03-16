#include <WiFi.h>
#include <WebServer.h>
#include "HX711.h"

// --- Pin-Definitionen ---
const int LOADCELL_DOUT_PIN = 5;  // DT an D4
const int LOADCELL_SCK_PIN = 4;  // SCK an RX2 / D16

HX711 scale;
WebServer server(80); // Webserver auf Port 80

// --- HTML & JavaScript (als Raw String) ---
// Das ist die Webseite, die der ESP32 an deinen Browser schickt
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 Waage</title>
  <style>
    body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; background-color: #f4f4f9; color: #333; }
    h1 { color: #2c3e50; }
    .weight-container { font-size: 4rem; font-weight: bold; margin: 20px auto; padding: 30px; border-radius: 15px; background: white; box-shadow: 0 4px 8px rgba(0,0,0,0.1); width: 80%; max-width: 400px; }
    .unit { font-size: 2rem; color: #7f8c8d; }
  </style>
</head>
<body>
  <h1>Live Sensordaten</h1>
  <div class="weight-container">
    <span id="weightValue">--</span> <span class="unit">g</span>
  </div>

  <script>
    // Diese Funktion holt die Daten per "AJAX" (fetch) vom ESP32
    function getWeightData() {
      fetch('/data')
        .then(response => response.text())
        .then(data => {
          document.getElementById("weightValue").innerText = data;
        })
        .catch(error => console.error('Fehler beim Abrufen der Daten:', error));
    }

    // Rufe die Funktion alle 250 Millisekunden (4x pro Sekunde) auf
    setInterval(getWeightData, 250);
  </script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);

  // 1. Waage initialisieren
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  // HINWEIS: Hier müsstest du später noch deinen Kalibrierungsfaktor einfügen!
  // scale.set_scale(1234.5); 
  scale.tare(); // Setzt die Waage beim Start auf 0

  // 2. ESP32 als WLAN Access Point (AP) starten
  Serial.println("Starte WLAN Access Point...");
  // Name des WLANs und das Passwort (mindestens 8 Zeichen!)
  WiFi.softAP("ESP32-Waage", "Waage1234"); 
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP-Adresse: ");
  Serial.println(IP); // Standardmäßig meist 192.168.4.1

  // 3. Webserver Routen definieren
  // Wenn jemand die Hauptseite aufruft, sende das HTML
  server.on("/", []() {
    server.send(200, "text/html", index_html);
  });

  // Wenn das JavaScript die Route "/data" aufruft, sende nur den Messwert
  server.on("/data", []() {
    // get_units(1) holt nur EINEN Wert, das ist schneller und blockiert den Server nicht
    float currentWeight = scale.get_units(1); 
    server.send(200, "text/plain", String(currentWeight/1000.0, 1)); // 1 Nachkommastelle
  });

  // 4. Webserver starten
  server.begin();
  Serial.println("HTTP Server gestartet!");
}

void loop() {
  // Der Server muss in der Loop ständig Anfragen abarbeiten
  server.handleClient();
}