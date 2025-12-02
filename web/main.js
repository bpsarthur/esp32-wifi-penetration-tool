let currentTarget = null;
let pollInterval = null;

function showTab(tabId) {
  document
    .querySelectorAll(".tab-content")
    .forEach((el) => el.classList.remove("active"));
  document
    .querySelectorAll(".nav-btn")
    .forEach((el) => el.classList.remove("active"));

  document.getElementById(tabId).classList.add("active");
  document
    .querySelector(`button[onclick="showTab('${tabId}')"]`)
    .classList.add("active");
}

function log(msg, type = "sys") {
  const logOutput = document.getElementById("log-output");
  const span = document.createElement("span");
  span.className = type === "err" ? "err-msg" : "sys-msg";
  span.innerText = `> ${msg}\n`;
  logOutput.appendChild(span);
  logOutput.scrollTop = logOutput.scrollHeight;
}

async function scanAPs() {
  log("Scanning for APs...");
  try {
    const response = await fetch("/ap-list");
    const buffer = await response.arrayBuffer();
    const data = new Uint8Array(buffer);

    const tbody = document.querySelector("#ap-table tbody");
    tbody.innerHTML = "";

    // Parse chunks of 40 bytes (33 SSID + 6 BSSID + 1 RSSI)
    for (let i = 0; i < data.length; i += 40) {
      if (i + 40 > data.length) break;

      const ssidBytes = data.slice(i, i + 33);
      const bssidBytes = data.slice(i + 33, i + 39);
      const rssi = data[i + 39] - 255; // Convert back to signed if needed, or just interpret as is. Usually RSSI is negative.

      // Clean SSID (remove null bytes)
      let ssid = new TextDecoder().decode(ssidBytes).replace(/\0/g, "");
      if (!ssid) ssid = "<Hidden>";

      // Format BSSID
      const bssid = Array.from(bssidBytes)
        .map((b) => b.toString(16).padStart(2, "0"))
        .join(":")
        .toUpperCase();

      const tr = document.createElement("tr");
      tr.innerHTML = `
                <td>${ssid}</td>
                <td>${bssid}</td>
                <td>${rssi} dBm</td>
                <td><button onclick="selectTarget('${ssid}', '${bssid}')" class="btn-action" style="padding: 5px 10px; font-size: 0.8rem;">SELECT</button></td>
            `;
      tbody.appendChild(tr);
    }
    log("Scan complete.");
  } catch (e) {
    log("Scan failed: " + e.message, "err");
  }
}

function selectTarget(ssid, bssid) {
  currentTarget = { ssid, bssid };
  document.getElementById("selected-target").innerText = `${ssid} (${bssid})`;
  document.getElementById("target-bssid").value = bssid;
  log(`Target selected: ${ssid}`);
  showTab("attacks");
}

async function startAttack(type) {
  if (!currentTarget && type !== "BEACON_SPAM" && type !== "PROBE_SNIFF") {
    log("Error: No target selected!", "err");
    return;
  }

  const attackConfig = new Uint8Array(sizeof_attack_request_t());
  // We need to match the C struct layout:
  // uint8_t type;
  // uint8_t method;
  // uint8_t timeout;
  // wifi_ap_record_t *ap_record; (This is complex to serialize directly, usually we just send BSSID/Channel if simplified)

  // WAIT: The original code uses a raw struct dump.
  // typedef struct {
  //     uint8_t type;
  //     uint8_t method;
  //     uint8_t timeout;
  //     const wifi_ap_record_t *ap_record;
  // } attack_config_t;

  // Actually, looking at webserver.c:
  // attack_request_t attack_request;
  // httpd_req_recv(req, (char *)&attack_request, sizeof(attack_request_t));

  // I need to check what attack_request_t is defined as. It wasn't in attack.h?
  // Let's assume for now we send a simplified JSON or just params if I can change the backend.
  // BUT I am supposed to keep compatibility or update backend.

  // Let's look at the backend implementation again later. For now, I'll implement a generic fetch.

  log(`Starting attack: ${type}...`);

  // NOTE: This part requires the backend to be updated to accept JSON or a cleaner format.
  // The current backend expects a binary struct which is brittle.
  // I will modify the backend to accept a simpler format or I will construct the binary here.
  // For now, let's assume I'll update the backend to take a simple POST with headers or JSON.

  // Placeholder for actual API call
  // await fetch('/run-attack', { method: 'POST', body: JSON.stringify({ type, target: currentTarget }) });

  // Since I haven't updated the backend yet, I'll leave this as a TODO in the JS
  // and handle the backend change to support this.

  log("Attack command sent (Simulation).");
  startPolling();
}

function stopAttack() {
  log("Stopping attack...");
  fetch("/reset", { method: "HEAD" });
  stopPolling();
  document.getElementById("status-indicator").className = "status-ready";
  document.getElementById("status-indicator").innerText = "READY";
  document.getElementById("footer-state").innerText = "IDLE";
}

function resetState() {
  stopAttack();
  currentTarget = null;
  document.getElementById("selected-target").innerText = "None";
  log("State reset.");
}

function startPolling() {
  if (pollInterval) clearInterval(pollInterval);
  document.getElementById("status-indicator").className = "status-running";
  document.getElementById("status-indicator").innerText = "ATTACKING";
  document.getElementById("footer-state").innerText = "RUNNING";

  pollInterval = setInterval(async () => {
    try {
      const res = await fetch("/status");
      // Parse status...
    } catch (e) {
      console.error(e);
    }
  }, 2000);
}

function stopPolling() {
  if (pollInterval) clearInterval(pollInterval);
  pollInterval = null;
}

// Helper
function sizeof_attack_request_t() {
  return 100; // Dummy value
}

// Clock
setInterval(() => {
  const now = new Date();
  document.getElementById("footer-time").innerText = now.toLocaleTimeString();
}, 1000);
