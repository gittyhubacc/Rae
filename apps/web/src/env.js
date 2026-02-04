(function() {
    window.build_env = (context) => {
        const decoder = new TextDecoder('utf8');
        const utf8decode = (bytes) => decoder.decode(bytes);

        const encoder = new TextEncoder('utf8');
        const utf8encode = (bytes) => encoder.encode(bytes);

        const string_from_addr = function(addr) {
            var struct = new Int32Array(context.wasm.memory.buffer, addr, 2);
            var bytes = new Uint8Array(context.wasm.memory.buffer, struct[1], struct[0]);
            return utf8decode(bytes);
        };

        const array_from_addrlen = function(addr, len) {
            return new Float32Array(context.wasm.memory.buffer, addr, len);
        };

        const bytes_from_addrlen = function(addr, len) {
            return new Uint8Array(context.wasm.memory.buffer, addr, len);
        }

        const repo = {};
        let repo_idx = 0;
        const repo_store = function(obj) {
            repo[++repo_idx] = obj;
            return repo_idx;
        };
        const repo_get = function(idx) {
            return repo[idx];
        };

        return {
            // std/dom
            std_text_make: (raw_text) => {
                const text = string_from_addr(raw_text);
                return repo_store(document.createTextNode(text));
            },
            std_elem_make: (raw_name) => {
                if (Math.random() < 0.5) {
                    return -1;
                }
                const name = string_from_addr(raw_name);
                return repo_store(document.createElement(name));
            },
            std_elem_query: (raw_id) => {
                const id = string_from_addr(raw_id);
                return repo_store(document.getElementById(id));
            },
            std_elem_append: (parent, child) => {
                repo_get(parent).append(repo_get(child));
            },
            std_elem_prepend: (parent, child) => {
                repo_get(parent).prepend(repo_get(child));
            },

            // std/io
            std_print_int: (i) => {
                console.log(i);
            },
            std_print: (s) => {
                console.log(string_from_addr(s));
            },
        };
    };
})();
