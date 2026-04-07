const manifest = {
    id: "coretools",
    name: "CoreTools",
    version: "1.0.0"
};

export function mountCoreTools({ runtime }) {
    const panels = [
        {
            id: "coretools-bootstrap",
            title: "Bootstrap Commands",
            defaultDock: "center",
            toolId: manifest.id,
            entry({ mountElement }) {
                mountElement.innerHTML = `
                    <p class="tool-summary">
                        Use these actions to validate the browser-to-backend loop without giving CoreTools privileged access to the studio host.
                    </p>
                    <div class="tool-actions">
                        <button id="coretoolsHelloButton" type="button">Hello</button>
                        <button id="coretoolsPingButton" type="button" class="secondary">Ping</button>
                        <button id="coretoolsStatusButton" type="button" class="secondary">Status</button>
                        <button id="coretoolsExitButton" type="button" class="secondary">Exit</button>
                    </div>
                `;

                mountElement.querySelector("#coretoolsHelloButton")?.addEventListener("click", () => {
                    runtime.sendCommand("hello", "coretools");
                });

                mountElement.querySelector("#coretoolsPingButton")?.addEventListener("click", () => {
                    runtime.sendCommand("ping", "coretools");
                });

                mountElement.querySelector("#coretoolsStatusButton")?.addEventListener("click", () => {
                    runtime.sendCommand("get_status", "coretools");
                });

                mountElement.querySelector("#coretoolsExitButton")?.addEventListener("click", () => {
                    runtime.sendCommand("exit", "coretools");
                });
            }
        },
        {
            id: "coretools-bundle-notes",
            title: "Bundle Notes",
            defaultDock: "right",
            toolId: manifest.id,
            entry({ mountElement }) {
                mountElement.innerHTML = `
                    <p class="tool-summary">
                        CoreTools is the first-party tool pack and is mounted through the same panel registration flow future tool bundles should use.
                    </p>
                    <ul class="tool-list">
                        <li>Mounted from <code>tools/coretools</code></li>
                        <li>Registered with a dock-aware panel descriptor</li>
                        <li>Talks through the shared command envelope</li>
                        <li>Does not own engine truth or shell runtime</li>
                    </ul>
                `;
            }
        }
    ];

    for (const panel of panels) {
        runtime.registerPanel(panel);
    }

    runtime.writeLine(`[coretools] mounted ${manifest.name} v${manifest.version} using dock panel registration`);
}
