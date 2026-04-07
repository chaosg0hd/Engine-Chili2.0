import { createStudioRuntime } from "./shell/runtime.js";
import { mountCoreTools } from "../tools/coretools/index.js";

const runtime = createStudioRuntime({
    socketUrl: "ws://127.0.0.1:8765",
    protocolVersion: "1",
    statusElement: document.getElementById("statusLine"),
    consoleElement: document.getElementById("consoleOutput")
});

mountCoreTools({
    runtime
});

runtime.connect();
