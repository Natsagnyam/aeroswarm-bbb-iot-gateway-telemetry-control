#include <iostream>
#include <string>
#include <fstream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "gpio_controller.hpp"

#define GPIO1_BASE 0x4804C000
#define GPIO_DATAOUT 0x134
#define GPIO_SETDATAOUT 0x194
#define GPIO_CLEARDATAOUT 0x190
#define GPIO_60_BIT (1 << 28) // GPIO 60 is pin 28 on GPIO1 bank

void* gpio1_map = MAP_FAILED;

void init_hardware_direct() {
    // Ensure LED3 direction is out
    std::ofstream led_dir("/sys/class/leds/beaglebone:green:usr3/direction");
    if (led_dir.is_open()) led_dir << "out";

    // Ensure GPIO 60 (P9_12) is exported and set to out
    std::ofstream exp_file("/sys/class/gpio/export");
    if (exp_file.is_open()) exp_file << "60";
    std::ofstream dir_file("/sys/class/gpio/gpio60/direction");
    if (dir_file.is_open()) dir_file << "out";

    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd != -1) {
        gpio1_map = mmap(NULL, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, GPIO1_BASE);
        close(fd);
    }
}

void set_hardware_state(bool on) {
    // 1. Toggle LED3
    std::ofstream led_file("/sys/class/leds/beaglebone:green:usr3/brightness");
    if (led_file.is_open()) {
        led_file << (on ? "1" : "0");
    }

    // 2. Toggle P9_12 via mmap or sysfs fallback
    if (gpio1_map != MAP_FAILED) {
        volatile uint32_t* reg = (volatile uint32_t*)((char*)gpio1_map + (on ? GPIO_SETDATAOUT : GPIO_CLEARDATAOUT));
        *reg = GPIO_60_BIT;
    } else {
        std::ofstream p9_file("/sys/class/gpio/gpio60/value");
        if (p9_file.is_open()) p9_file << (on ? "1" : "0");
    }
}

bool get_hardware_state() {
    std::ifstream led_file("/sys/class/leds/beaglebone:green:usr3/brightness");
    int val = 0;
    if (led_file.is_open()) {
        led_file >> val;
    }
    return (val == 1);
}

std::string read_file_trim(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return "N/A";
    std::string val;
    std::getline(file, val);
    return val;
}

std::string get_cpu_temp() {
    std::string raw = read_file_trim("/sys/class/thermal/thermal_zone0/temp");
    if (raw == "N/A") return "45.0";
    try {
        float temp = std::stof(raw) / 1000.0f;
        return std::to_string(temp);
    } catch (...) {
        return "45.0";
    }
}

std::string get_cpu_freq() {
    std::string raw = read_file_trim("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq");
    if (raw == "N/A") return "1000000";
    return raw;
}

int main() {
    init_hardware_direct();

    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        std::perror("socket failed");
        exit(EXIT_FAILURE);
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) < 0) {
        std::perror("listen");
        exit(EXIT_FAILURE);
    }

    std::cout << "Aeroswarm IoT Gateway & PF Excitation Controller active on port 8080...\n";

    while (true) {
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            std::perror("accept");
            continue;
        }

        char buffer[2048] = {0};
        read(new_socket, buffer, sizeof(buffer) - 1);
        std::string req(buffer);

        std::string response;
        if (req.find("GET / HTTP") != std::string::npos || req.find("GET /index.html") != std::string::npos) {
            response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
                "<!DOCTYPE html>\n<html>\n<head>\n"
                "<title>Aeroswarm BBB IoT Gateway & PF Simulation</title>\n"
                "<style>\n"
                "  body { background-color: #008b8b; color: #f0f0f0; font-family: monospace; margin: 0; padding: 20px; }\n"
                "  h1, h2, h3 { color: #ffffff; text-align: center; text-shadow: 2px 2px #000; }\n"
                "  .grid { display: flex; flex-wrap: wrap; gap: 20px; justify-content: center; }\n"
                "  .card { background: rgba(0, 0, 0, 0.4); border: 2px solid #20b2aa; border-radius: 8px; padding: 20px; width: 450px; }\n"
                "  button { background: #ff4500; color: white; border: none; padding: 12px 24px; font-size: 16px; cursor: pointer; border-radius: 5px; font-weight: bold; margin: 5px; }\n"
                "  button:hover { background: #ff6347; }\n"
                "  canvas { background: #004d4d; border: 2px solid #ffffff; display: block; margin: 10px auto; border-radius: 4px; }\n"
                "  .metric { font-size: 18px; margin: 8px 0; text-align: center; }\n"
                "  .author { text-align: center; font-size: 14px; margin-top: 15px; color: #e0ffff; }\n"
                "</style>\n"
                "</head>\n<body>\n"
                "  <h1>Aeroswarm Embedded IoT Gateway & Excitation Control</h1>\n"
                "  <div class=\"grid\">\n"
                "    <div class=\"card\">\n"
                "      <h2>Hardware Control</h2>\n"
                "      <div class=\"metric\">LED3 & P9_12: <span id=\"hw-status\" style=\"font-weight:bold;\">UNKNOWN</span></div>\n"
                "      <br><div style=\"text-align:center;\"><button onclick=\"toggleHardware()\">Toggle LED3 & P9_12</button></div>\n"
                "      <h2>BBB System Telemetry</h2>\n"
                "      <div class=\"metric\">CPU Temp: <span id=\"cpu-temp\">--</span> &deg;C</div>\n"
                "      <div class=\"metric\">CPU Freq: <span id=\"cpu-freq\">--</span> MHz</div>\n"
                "      <div class=\"metric\">PMIC Status: <span id=\"pmic-status\" style=\"color:#00ff00;\">ONLINE (Active)</span></div>\n"
                "    </div>\n"
                "    <div class=\"card\" style=\"width: 600px;\">\n"
                "      <h2>Power Simulation & PF Meter</h2>\n"
                "      <div class=\"metric\" id=\"state-label\" style=\"font-weight:bold; color:#fffa65;\">State: --</div>\n"
                "      <div style=\"display: flex; justify-content: center; gap: 20px; align-items: center;\">\n"
                "        <canvas id=\"waveCanvas\" width=\"280\" height=\"240\"></canvas>\n"
                "        <canvas id=\"pfCanvas\" width=\"240\" height=\"240\"></canvas>\n"
                "      </div>\n"
                "      <div style=\"font-size:12px; text-align:center; margin-top:8px;\">\n"
                "        <span style=\"color:#000000; background:#ccc; padding:2px 4px; font-weight:bold;\">■ V (Black)</span> &nbsp;\n"
                "        <span style=\"color:#1e90ff; font-weight:bold;\">■ I (Blue)</span> &nbsp;\n"
                "        <span style=\"color:#ff3333; font-weight:bold;\">■ MW (Red)</span> &nbsp;\n"
                "        <span style=\"color:#ffffff; font-weight:bold;\">■ MVAR (White)</span>\n"
                "      </div>\n"
                "      <div class=\"author\">Author (Natska) Natsagnyam NAMKHAI 2026 Nov</div>\n"
                "    </div>\n"
                "  </div>\n"
                "  <script>\n"
                "    function fetchStatus() {\n"
                "      fetch('/api/status').then(r => r.json()).then(data => {\n"
                "        document.getElementById('hw-status').innerText = data.led ? 'ON (HIGH)' : 'OFF (LOW)';\n"
                "        document.getElementById('cpu-temp').innerText = parseFloat(data.cpu_temp).toFixed(1);\n"
                "        document.getElementById('cpu-freq').innerText = (parseInt(data.cpu_freq) / 1000).toFixed(0);\n"
                "      }).catch(err => console.error(err));\n"
                "    }\n"
                "    function toggleHardware() {\n"
                "      fetch('/api/relay', {\n"
                "        method: 'POST',\n"
                "        headers: { 'Authorization': 'Bearer Natska2026', 'Content-Type': 'application/json' },\n"
                "        body: JSON.stringify({ action: 'toggle' })\n"
                "      }).then(r => r.json()).then(data => fetchStatus());\n"
                "    }\n"
                "    setInterval(fetchStatus, 1500);\n"
                "    fetchStatus();\n"
                "\n"
                "    const waveCanvas = document.getElementById('waveCanvas');\n"
                "    const wCtx = waveCanvas.getContext('2d');\n"
                "    const pfCanvas = document.getElementById('pfCanvas');\n"
                "    const pfCtx = pfCanvas.getContext('2d');\n"
                "    let startTime = Date.now();\n"
                "\n"
                "    function draw() {\n"
                "      let elapsed = (Date.now() - startTime) / 1000;\n"
                "      // Phase angle sweeps smoothly from -1.57 rad (-90 deg, PF=0 lag) to +1.57 rad (+90 deg, PF=0 lead)\n"
                "      let phaseTheta = Math.sin(elapsed * 0.8) * 1.57;\n"
                "      let pf = Math.cos(phaseTheta);\n"
                "\n"
                "      let isLeading = phaseTheta > 0;\n"
                "      let stateStr = isLeading ? 'Generator capacitive leading' : 'Generator inductive lagging';\n"
                "      document.getElementById('state-label').innerText = stateStr + ' (PF: ' + pf.toFixed(2) + ')';\n"
                "\n"
                "      // --- 1. Draw Waveform Canvas ---\n"
                "      wCtx.clearRect(0, 0, waveCanvas.width, waveCanvas.height);\n"
                "      wCtx.strokeStyle = '#20b2aa'; wCtx.lineWidth = 1;\n"
                "      wCtx.beginPath(); wCtx.moveTo(0, waveCanvas.height / 2); wCtx.lineTo(waveCanvas.width, waveCanvas.height / 2); wCtx.stroke();\n"
                "\n"
                "      let w = waveCanvas.width, h = waveCanvas.height, midY = h / 2, amp = 60;\n"
                "      let i2Phase = phaseTheta - (Math.PI / 2);\n"
                "\n"
                "      // Voltage V (Black, fat)\n"
                "      wCtx.strokeStyle = '#000000'; wCtx.lineWidth = 3; wCtx.beginPath();\n"
                "      for (let x = 0; x < w; x++) {\n"
                "        let t = (x / w) * 0.02;\n"
                "        let y = midY - (Math.sin(2 * Math.PI * 50 * t) * amp);\n"
                "        if (x === 0) wCtx.moveTo(x, y); else wCtx.lineTo(x, y);\n"
                "      }\n"
                "      wCtx.stroke();\n"
                "\n"
                "      // Current I (Blue)\n"
                "      wCtx.strokeStyle = '#1e90ff'; wCtx.lineWidth = 2; wCtx.beginPath();\n"
                "      for (let x = 0; x < w; x++) {\n"
                "        let t = (x / w) * 0.02;\n"
                "        let y = midY - (Math.sin(2 * Math.PI * 50 * t + phaseTheta) * amp);\n"
                "        if (x === 0) wCtx.moveTo(x, y); else wCtx.lineTo(x, y);\n"
                "      }\n"
                "      wCtx.stroke();\n"
                "\n"
                "      // MW (Red) = V * I\n"
                "      wCtx.strokeStyle = '#ff3333'; wCtx.lineWidth = 2; wCtx.beginPath();\n"
                "      for (let x = 0; x < w; x++) {\n"
                "        let t = (x / w) * 0.02;\n"
                "        let v = Math.sin(2 * Math.PI * 50 * t);\n"
                "        let i = Math.sin(2 * Math.PI * 50 * t + phaseTheta);\n"
                "        let y = midY - ((v * i) * amp);\n"
                "        if (x === 0) wCtx.moveTo(x, y); else wCtx.lineTo(x, y);\n"
                "      }\n"
                "      wCtx.stroke();\n"
                "\n"
                "      // MVAR (White, fat) = V * I2\n"
                "      wCtx.strokeStyle = '#ffffff'; wCtx.lineWidth = 4; wCtx.beginPath();\n"
                "      for (let x = 0; x < w; x++) {\n"
                "        let t = (x / w) * 0.02;\n"
                "        let v = Math.sin(2 * Math.PI * 50 * t);\n"
                "        let i2 = Math.sin(2 * Math.PI * 50 * t + i2Phase);\n"
                "        let y = midY - ((v * i2) * amp);\n"
                "        if (x === 0) wCtx.moveTo(x, y); else wCtx.lineTo(x, y);\n"
                "      }\n"
                "      wCtx.stroke();\n"
                "\n"
                "      // --- 2. Draw Round PF Meter Canvas ---\n"
                "      pfCtx.fillStyle = '#ffffff'; pfCtx.fillRect(0, 0, pfCanvas.width, pfCanvas.height);\n"
                "      let cx = pfCanvas.width / 2, cy = pfCanvas.height / 2, radius = 95;\n"
                "\n"
                "      // Outer black bezel\n"
                "      pfCtx.strokeStyle = '#111'; pfCtx.lineWidth = 8;\n"
                "      pfCtx.beginPath(); pfCtx.arc(cx, cy, radius + 4, 0, 2 * Math.PI); pfCtx.stroke();\n"
                "\n"
                "      // White dial face\n"
                "      pfCtx.fillStyle = '#ffffff'; pfCtx.beginPath(); pfCtx.arc(cx, cy, radius, 0, 2 * Math.PI); pfCtx.fill();\n"
                "\n"
                "      // Tick marks and numbers\n"
                "      pfCtx.fillStyle = '#000'; pfCtx.font = 'bold 11px monospace'; pfCtx.textAlign = 'center'; pfCtx.textBaseline = 'middle';\n"
                "      for (let angleDeg = -80; angleDeg <= 80; angleDeg += 10) {\n"
                "        let rad = (angleDeg - 90) * (Math.PI / 180);\n"
                "        let r1 = radius - 8, r2 = radius - 15;\n"
                "        if (angleDeg % 20 === 0) r2 = radius - 18;\n"
                "        pfCtx.strokeStyle = '#000'; pfCtx.lineWidth = (angleDeg % 20 === 0) ? 2 : 1;\n"
                "        pfCtx.beginPath();\n"
                "        pfCtx.moveTo(cx + r1 * Math.cos(rad), cy + r1 * Math.sin(rad));\n"
                "        pfCtx.lineTo(cx + r2 * Math.cos(rad), cy + r2 * Math.sin(rad));\n"
                "        pfCtx.stroke();\n"
                "\n"
                "        // Number labels for 1.0, .9, .7, .5, 0\n"
                "        if (angleDeg === 0) { pfCtx.fillText('1.0', cx + (r2 - 12) * Math.cos(rad), cy + (r2 - 12) * Math.sin(rad)); }\n"
                "        else if (angleDeg === 30) { pfCtx.fillText('.9', cx + (r2 - 12) * Math.cos(rad), cy + (r2 - 12) * Math.sin(rad)); }\n"
                "        else if (angleDeg === 55) { pfCtx.fillText('.7', cx + (r2 - 12) * Math.cos(rad), cy + (r2 - 12) * Math.sin(rad)); }\n"
                "        else if (angleDeg === 75) { pfCtx.fillText('.5', cx + (r2 - 12) * Math.cos(rad), cy + (r2 - 12) * Math.sin(rad)); }\n"
                "        else if (angleDeg === -30) { pfCtx.fillText('.9', cx + (r2 - 12) * Math.cos(rad), cy + (r2 - 12) * Math.sin(rad)); }\n"
                "        else if (angleDeg === -55) { pfCtx.fillText('.7', cx + (r2 - 12) * Math.cos(rad), cy + (r2 - 12) * Math.sin(rad)); }\n"
                "        else if (angleDeg === -75) { pfCtx.fillText('.5', cx + (r2 - 12) * Math.cos(rad), cy + (r2 - 12) * Math.sin(rad)); }\n"
                "      }\n"
                "\n"
                "      // Labels: LAG, LEAD, POWER FACTOR\n"
                "      pfCtx.font = 'bold 10px sans-serif';\n"
                "      pfCtx.fillText('LAG', cx - 45, cy - 15);\n"
                "      pfCtx.fillText('LEAD', cx + 45, cy - 15);\n"
                "      pfCtx.font = 'bold 11px sans-serif'; pfCtx.fillStyle = '#222';\n"
                "      pfCtx.fillText('POWER FACTOR', cx, cy + 25);\n"
                "\n"
                "      // Center cap hub\n"
                "      pfCtx.fillStyle = '#e0e0e0'; pfCtx.beginPath(); pfCtx.arc(cx, cy, 14, 0, 2 * Math.PI); pfCtx.fill();\n"
                "      pfCtx.strokeStyle = '#888'; pfCtx.lineWidth = 1; pfCtx.stroke();\n"
                "      pfCtx.fillStyle = '#fff'; pfCtx.beginPath(); pfCtx.arc(cx, cy, 5, 0, 2 * Math.PI); pfCtx.fill();\n"
                "\n"
                "      // Needle angle mapping: pf goes 1.0 (top, 0 deg) down to 0 (left/right, +/- 85 deg)\n"
                "      // phaseTheta ranges from -1.57 (lag) to +1.57 (lead)\n"
                "      let needleAngleDeg = (phaseTheta / 1.57) * 75;\n"
                "      let needleRad = (needleAngleDeg - 90) * (Math.PI / 180);\n"
                "\n"
                "      pfCtx.strokeStyle = '#111'; pfCtx.lineWidth = 3;\n"
                "      pfCtx.beginPath();\n"
                "      pfCtx.moveTo(cx, cy);\n"
                "      pfCtx.lineTo(cx + (radius - 22) * Math.cos(needleRad), cy + (radius - 22) * Math.sin(needleRad));\n"
                "      pfCtx.stroke();\n"
                "\n"
                "      requestAnimationFrame(draw);\n"
                "    }\n"
                "    requestAnimationFrame(draw);\n"
                "  </script>\n"
                "</body>\n</html>\n";
        } else if (req.find("GET /api/status") != std::string::npos) {
            bool led = get_hardware_state();
            std::string temp = get_cpu_temp();
            std::string freq = get_cpu_freq();
            response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n"
                       "{\"status\":\"active\",\"led\": " + std::string(led ? "true" : "false") + 
                       ",\"cpu_temp\":\"" + temp + "\",\"cpu_freq\":\"" + freq + "\"}\n";
        } else if (req.find("POST /api/relay") != std::string::npos) {
            if (req.find("Authorization: Bearer Natska2026") != std::string::npos) {
                bool current_led = get_hardware_state();
                set_hardware_state(!current_led);
                response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"success\":true,\"led\":" + std::string(!current_led ? "true" : "false") + "}\n";
            } else {
                response = "HTTP/1.1 401 Unauthorized\r\nContent-Type: application/json\r\n\r\n{\"error\":\"Unauthorized\"}\n";
            }
        } else {
            response = "HTTP/1.1 404 Not Found\r\nContent-Type: application/json\r\n\r\n{\"error\":\"Not Found\"}\n";
        }

        write(new_socket, response.c_str(), response.length());
        close(new_socket);
    }
    return 0;
}
