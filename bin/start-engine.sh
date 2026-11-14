#!/bin/bash
BUILD_DIR="/opt/actriz/core/actriz_ai/bin/llama.cpp/build"
MODEL="/opt/actriz/core/actriz_ai/modelos/openzero-qwen3-1.7b-agentic-q4_k_m.gguf"

pkill -f "llama-server.*8080" > /dev/null 2>&1
sleep 1

export LD_LIBRARY_PATH="$BUILD_DIR/bin:$BUILD_DIR/src:$BUILD_DIR/ggml/src:$BUILD_DIR/ggml/src/ggml-cpu:$LD_LIBRARY_PATH"

echo "Iniciando motor Actriz AI (Qwen3-1.7B - Modo Rápido)..."
nohup "$BUILD_DIR/bin/llama-server" -m "$MODEL" -t 8 -c 512 --host 127.0.0.1 --port 8080 --jinja --reasoning-format none > /tmp/actriz_ai.log 2>&1 &
echo "¡Motor iniciado!"
