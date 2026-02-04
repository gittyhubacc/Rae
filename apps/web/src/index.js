(async function() {
    document.addEventListener('DOMContentLoaded', async function() {
        let context = {
            wasm: null
        };

        const run_wasm_module = async function(module) {
            context.wasm = module.instance.exports;
            context.wasm.main_entry_point();
        };

        WebAssembly
            .instantiateStreaming(
                fetch("index.wasm"), 
                { env: build_env(context) })
            .then(run_wasm_module);
    });
})();
