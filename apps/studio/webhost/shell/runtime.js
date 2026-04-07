export function createStudioRuntime({
    socketUrl,
    protocolVersion,
    statusElement,
    consoleElement
}) {
    let socket = null;
    let requestCounter = 0;
    const panelRegistry = [];
    const dockZones = {
        top: document.getElementById("dock-top"),
        left: document.getElementById("dock-left"),
        center: document.getElementById("dock-center"),
        right: document.getElementById("dock-right"),
        bottom: document.getElementById("dock-bottom")
    };

    function writeLine(message) {
        const current = consoleElement.textContent.trim();
        consoleElement.textContent = current ? `${current}\n${message}` : message;
    }

    function setStatus(message) {
        statusElement.textContent = message;
    }

    function nextRequestId() {
        requestCounter += 1;
        return `studio-web-${requestCounter}`;
    }

    function formatBackendMessage(payload) {
        if (typeof payload !== "object" || payload === null) {
            return String(payload);
        }

        const parts = [];
        if (payload.kind) {
            parts.push(`kind=${payload.kind}`);
        }
        if (payload.command) {
            parts.push(`command=${payload.command}`);
        }
        if (payload.event) {
            parts.push(`event=${payload.event}`);
        }
        if (typeof payload.ok === "boolean") {
            parts.push(`ok=${payload.ok}`);
        }
        if (payload.code) {
            parts.push(`code=${payload.code}`);
        }
        if (payload.request_id) {
            parts.push(`request_id=${payload.request_id}`);
        }
        if (payload.message) {
            parts.push(`message=${payload.message}`);
        }

        return parts.join(" | ");
    }

    function sendCommand(command, sender = "studio-web-ui") {
        if (!socket || socket.readyState !== WebSocket.OPEN) {
            writeLine(`[host] socket unavailable, cannot send "${command}"`);
            return false;
        }

        const payload = {
            kind: "command",
            protocol_version: protocolVersion,
            command,
            request_id: nextRequestId(),
            sender
        };

        socket.send(JSON.stringify(payload));
        writeLine(`[host] command sent -> ${payload.command} | request_id=${payload.request_id}`);
        return true;
    }

    function connect() {
        setStatus(`Connecting to ${socketUrl}`);
        writeLine("[host] studio webhost ready");

        socket = new WebSocket(socketUrl);

        socket.addEventListener("open", () => {
            setStatus(`Connected to ${socketUrl}`);
            writeLine("[host] websocket connected");
        });

        socket.addEventListener("message", (event) => {
            try {
                const payload = JSON.parse(event.data);
                writeLine(`[backend] ${formatBackendMessage(payload)}`);
            }
            catch {
                writeLine(`[backend] ${event.data}`);
            }
        });

        socket.addEventListener("close", () => {
            setStatus("Backend connection closed");
            writeLine("[host] websocket closed");
        });

        socket.addEventListener("error", () => {
            setStatus("Backend connection failed");
            writeLine("[host] websocket error");
        });
    }

    function createPanelFrame(panel) {
        const container = document.createElement("article");
        container.className = "dock-panel";
        container.dataset.panelId = panel.id;

        const header = document.createElement("header");
        header.className = "dock-panel-header";
        header.innerHTML = `
            <div>
                <p class="dock-panel-kicker">${panel.toolId || "tool"}</p>
                <h3>${panel.title}</h3>
            </div>
            <span class="dock-panel-meta">${panel.defaultDock}</span>
        `;

        const body = document.createElement("div");
        body.className = "dock-panel-body";

        container.append(header, body);
        return { container, body };
    }

    function registerPanel(panel) {
        const zone = dockZones[panel.defaultDock];
        if (!zone) {
            writeLine(`[host] invalid dock zone for panel ${panel.id}: ${panel.defaultDock}`);
            return false;
        }

        const { container, body } = createPanelFrame(panel);
        panelRegistry.push(panel);
        zone.appendChild(container);

        if (typeof panel.entry === "function") {
            panel.entry({
                mountElement: body,
                runtime,
                panel
            });
        }

        writeLine(`[host] panel registered -> ${panel.id} @ ${panel.defaultDock}`);
        return true;
    }

    const runtime = {
        connect,
        sendCommand,
        writeLine,
        setStatus,
        registerPanel,
        getSocketUrl() {
            return socketUrl;
        },
        getPanels() {
            return [...panelRegistry];
        }
    };

    return runtime;
}
