#include <WiFi.h>
#include <WebServer.h>
#include "HX711.h"

// --- Pin-Definitionen ---
const int LOADCELL_DOUT_PIN = 5;  // DT an D4
const int LOADCELL_SCK_PIN = 4;   // SCK an RX2 / D16

HX711 scale;
WebServer server(80); // Webserver auf Port 80

// --- HTML & JavaScript (als Raw String) ---
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 Waage</title>
  <style>
    body { font-family: Arial, sans-serif; text-align: center; margin-top: 30px; background-color: #f4f4f9; color: #333; }
    h1 { color: #2c3e50; }
    .dashboard { background: white; padding: 20px; border-radius: 15px; box-shadow: 0 4px 12px rgba(0,0,0,0.15); width: 90%; max-width: 500px; margin: 0 auto; }
    .weight-text { font-size: 3.5rem; font-weight: bold; margin-bottom: 20px; color: #2980b9; }
    .unit { font-size: 1.5rem; color: #7f8c8d; }
    .max-label { font-size: 1.5rem; color: #e74c3c; vertical-align: middle; margin-right: 10px; }
    
    /* Live-Wert Styling */
    .live-wrapper { margin-top: 15px; font-size: 1.2rem; color: #7f8c8d; background-color: #f8f9fa; padding: 10px; border-radius: 8px; display: inline-block; }
    .live-label { font-weight: bold; color: #34495e; }
    
    /* Graph Styling */
    .graph-wrapper { display: flex; align-items: stretch; height: 200px; margin-top: 20px; }
    .y-axis { display: flex; flex-direction: column; justify-content: space-between; padding-right: 10px; font-size: 14px; color: #7f8c8d; font-weight: bold; }
    .canvas-container { flex-grow: 1; position: relative; }
    canvas { background: #fafafa; border: 1px solid #ddd; border-radius: 5px; width: 100%; height: 100%; display: block; }
  </style>
</head>
<body>
  <h1>Live Sensordaten</h1>
  <div class="dashboard">
    <div class="weight-text">
      <span class="max-label">Max:</span><span id="weightValue">--</span><span class="unit"> kg</span>
    </div>
    
    <div class="graph-wrapper">
      <div class="y-axis">
        <span>100</span>
        <span>50</span>
        <span>0</span>
      </div>
      <div class="canvas-container">
        <canvas id="weightChart"></canvas>
      </div>
    </div>

    <div class="live-wrapper">
      <span class="live-label">Aktuell: </span>
      <span id="liveWeight">--</span> kg
    </div>
  </div>

  <script>
    const canvas = document.getElementById("weightChart");
    const ctx = canvas.getContext("2d");
    
    // Interne Auflösung des Canvas anpassen
    function resizeCanvas() {
      canvas.width = canvas.offsetWidth;
      canvas.height = canvas.offsetHeight;
    }
    
    const maxPoints = 100; // Anzahl der Messpunkte, die gleichzeitig sichtbar sind
    let dataPoints = new Array(maxPoints).fill(0);

    function drawGraph() {
      const width = canvas.width;
      const height = canvas.height;
      const yMin = 0;
      const yMax = 100;

      // Alles löschen
      ctx.clearRect(0, 0, width, height);

      // Mittellinie (50) dezent einzeichnen
      ctx.strokeStyle = "#ececec";
      ctx.lineWidth = 1;
      ctx.beginPath();
      ctx.moveTo(0, height / 2);
      ctx.lineTo(width, height / 2);
      ctx.stroke();

      // Den Graphen zeichnen
      ctx.beginPath();
      ctx.strokeStyle = "#e74c3c"; // Rote Linie
      ctx.lineWidth = 3;
      ctx.lineJoin = "round";

      for (let i = 0; i < maxPoints; i++) {
        let val = dataPoints[i];
        
        // --- SICHERHEITS-CLAMPING FÜR DIE UI ---
        if (val < 0) val = 0;      // Verhindert Ausbruch nach unten
        if (val > 100) val = 100;  // Verhindert Ausbruch nach oben

        const x = (i / (maxPoints - 1)) * width;
        // Y-Koordinate berechnen (0 ist unten, 100 ist oben)
        const y = height - ((val - yMin) / (yMax - yMin)) * height;

        if (i === 0) {
          ctx.moveTo(x, y);
        } else {
          ctx.lineTo(x, y);
        }
      }
      ctx.stroke();
    }

    function getWeightData() {
      fetch('/data')
        .then(response => response.text())
        .then(data => {
          let currentWeight = parseFloat(data);
          
          if(!isNaN(currentWeight)) {
            // 1. NEU: Den tatsächlichen Live-Wert im neuen HTML-Element anzeigen
            document.getElementById("liveWeight").innerText = currentWeight.toFixed(1);

            // 2. Neues Element ins Array packen, ältestes vorne löschen
            dataPoints.push(currentWeight);
            dataPoints.shift();
            
            // 3. Maximum aus dem aktuellen Array ermitteln
            let maxWeight = Math.max(...dataPoints);
            
            // 4. Große Textanzeige oben mit dem Maximalwert aktualisieren
            document.getElementById("weightValue").innerText = maxWeight.toFixed(1);
            
            // 5. Graph neu zeichnen
            drawGraph();
          }
        })
        .catch(error => console.error('Fehler:', error));
    }

    // Setup
    window.addEventListener('resize', () => { resizeCanvas(); drawGraph(); });
    resizeCanvas();
    drawGraph();
    setInterval(getWeightData, 50);
  </script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);

  // 1. Waage initialisieren
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  // scale.set_scale(1234.5); 
  scale.tare(); 

  // 2. ESP32 als WLAN Access Point (AP) starten
  Serial.println("Starte WLAN Access Point...");
  WiFi.softAP("ESP32-Waage", "Waage1234"); 
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP-Adresse: ");
  Serial.println(IP);

  // 3. Webserver Routen definieren
  server.on("/", []() {
    server.send(200, "text/html", index_html);
  });

  server.on("/data", []() {
    float currentWeight = scale.get_units(2); 
    server.send(200, "text/plain", String(currentWeight/(1000.0*8.0*1.12), 1)); 
  });

  // 4. Webserver starten
  server.begin();
  Serial.println("HTTP Server gestartet!");
}

void loop() {
  server.handleClient();
}