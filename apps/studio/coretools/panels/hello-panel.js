export function mountHelloPanel(target) {
    target.innerHTML = `
        <section class="hello-panel">
            <h2>Hello from CoreTools</h2>
            <p>
                This left sidebar is running inside an embedded WebView2 surface
                hosted by the native Engine Studio window.
            </p>
        </section>
    `;
}
