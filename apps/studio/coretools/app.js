import { mountHelloPanel } from "./panels/hello-panel.js";

const panelRoot = document.getElementById("hello-panel-root");

if (panelRoot) {
    mountHelloPanel(panelRoot);
}
