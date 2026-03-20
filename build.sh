#!/bin/bash
set -e # Arrête le script immédiatement s'il y a une erreur

echo "========================================"
echo "🔧 KERNEL BUILD & ISO GENERATION"
echo "========================================"

# 1. Nettoyage
echo "[1/5] Nettoyage des anciens fichiers..."
make clean 2>/dev/null || true
rm -rf iso_root jarvis.iso
# ON NE CRÉE PLUS DE SOUS-DOSSIERS
mkdir -p iso_root

# 2. Compilation via le Makefile
echo "[2/5] Compilation de JARVIS OS..."
make CFLAGS="-I./include -I./kernel/include -I./limine -ffreestanding -O2 -m64 -fno-stack-protector -mno-sse -mno-sse2 -DMINIZ_NO_STDIO -DMINIZ_NO_ARCHIVE_APIS -g"

# Recherche de l'exécutable
KERNEL_BIN=""
if [ -f "build/jarjarvis.kernel" ]; then KERNEL_BIN="build/jarjarvis.kernel";
elif [ -f "jarjarvis.kernel" ]; then KERNEL_BIN="jarjarvis.kernel";
elif [ -f "kernel/jarjarvis.kernel" ]; then KERNEL_BIN="kernel/jarjarvis.kernel";
else
    echo "❌ ERREUR : Le noyau est introuvable après la compilation !"
    exit 1
fi

# 3. Configuration Limine
echo "[3/5] Configuration du Bootloader (Limine)..."
# On copie les fichiers système Limine directement à la racine
cp limine/limine-bios.sys iso_root/
cp limine/limine-bios-cd.bin iso_root/
cp limine/limine-uefi-cd.bin iso_root/

# Création du fichier de config DIRECTEMENT à la racine
CONFIG_FILE="iso_root/limine.conf"
printf "TIMEOUT=0\n" > $CONFIG_FILE
printf "VERBOSE=yes\n\n" >> $CONFIG_FILE
printf ":JARVIS OS\n" >> $CONFIG_FILE
printf "    PROTOCOL=limine\n" >> $CONFIG_FILE
# Le kernel est aussi à la racine maintenant
printf "    KERNEL_PATH=boot:///jarvis.elf\n" >> $CONFIG_FILE
printf "    RESOLUTION=1024x768x32\n" >> $CONFIG_FILE

# Copie du noyau à la racine
cp $KERNEL_BIN iso_root/jarvis.elf

# Ajout de l'animation à la racine si elle existe
if [ -f "animation.bin" ]; then
    cp animation.bin iso_root/
    printf "    MODULE_PATH=boot:///animation.bin\n" >> $CONFIG_FILE
    echo "      -> Animation module ajouté."
fi

# 4. Création de l'ISO
echo "[4/5] Generation de l'ISO (xorriso)..."
# On dit à xorriso d'inclure le boot BIOS (Legacy) ET le boot UEFI
xorriso -as mkisofs -R -J \
    -b limine-bios-cd.bin -no-emul-boot -boot-load-size 4 -boot-info-table \
    --efi-boot limine-uefi-cd.bin -efi-boot-part --efi-boot-image --protective-msdos-label \
    iso_root -o jarvis.iso


# 5. Installation du Bootloader
echo "[5/5] Installation de Limine sur l'ISO..."
if [ ! -f ./limine/limine ]; then
    make -C limine
fi
./limine/limine bios-install jarvis.iso

echo ""
echo "✅ BUILD TERMINE AVEC SUCCES ! Tu peux maintenant lancer ./run.sh"
