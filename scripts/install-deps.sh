#!/bin/bash

# Instalador de dependencias para Tetris en macOS/Linux
# Autor: RGiskard7

set -e

GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m'

print_info()    { echo -e "${BLUE}ℹ ${NC}$1"; }
print_success() { echo -e "${GREEN}✅ ${NC}$1"; }
print_error()   { echo -e "${RED}❌ ${NC}$1"; }

echo "═══════════════════════════════════════════════════"
echo "    Tetris - Instalador de dependencias"
echo "═══════════════════════════════════════════════════"

if [[ "$OSTYPE" == "darwin"* ]]; then
    print_info "Detectado macOS. Usando Homebrew..."
    if ! command -v brew &> /dev/null; then
        print_error "Homebrew no está instalado. Instálalo desde https://brew.sh"
        exit 1
    fi
    brew install gcc pkg-config allegro
    print_success "Dependencias instaladas."
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    print_info "Detectado Linux. Usando apt..."
    sudo apt update
    sudo apt install -y build-essential pkg-config liballegro5-dev
    print_success "Dependencias instaladas."
else
    print_error "Sistema no soportado por este script: $OSTYPE"
    exit 1
fi

echo ""
print_success "Listo. Ahora ejecuta: ./scripts/build.sh run"
