#!/bin/bash

echo "========================================"
echo "🚀 LANCEMENT DE JARVIS DANS QEMU"
echo "========================================"

if [ ! -f "jarvis.iso" ]; then
    echo "❌ ERREUR : jarvis.iso introuvable ! Lance ./build.sh d'abord."
    exit 1
fi

echo "Les erreurs systemes seront sauvegardees dans 'qemu_crash.log'."
echo "Ferme la fenetre QEMU ou fais Ctrl+C ici pour arreter la VM."
echo "----------------------------------------"

# CORRECTION : -vga std au lieu de -vga virtio
# La carte Standard VGA (std) est 100% compatible avec les appels BIOS de Limine !
qemu-system-x86_64 \
    -cdrom jarvis.iso \
    -m 512M \
    -cpu max \
    -vga std \
    -serial stdio \
    -d int,cpu_reset,guest_errors \
    -D qemu_crash.log \
    -no-reboot \
    -no-shutdown

echo "----------------------------------------"
echo "🛑 QEMU s'est arreté."
