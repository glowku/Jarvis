🤖 Jarvis OS & The Myst AI
A Polymorphic Bare-Metal Ecosystem for Synthetic Intelligence.

🌌 Vision du Projet
Jarvis OS n'est pas un système d'exploitation classique. C'est une architecture "Bare-Metal" conçue pour supprimer l'abstraction entre l'IA et le matériel. Là où Windows ou Linux traitent l'IA comme une simple application, Jarvis l'intègre comme une extension du noyau.

The Myst AI est un agent polymorphe dont la structure logique s'adapte à la complexité des données qu'il traite. L'objectif est de tester les limites de l'efficacité computationnelle en utilisant des structures non-linéaires (fibres neuronales).

🚀 État Actuel du Prototype
Développé en un temps record, le système est actuellement stable et capable de :

Bootloader : Séquence d'initialisation personnalisée (Matrix White Rabbit 00101).

Kernel : Noyau x86_64 complet avec GDT, IDT et gestion mémoire (PMM/VMM).

Interface (GUI) : Bureau fonctionnel avec gestionnaire de fenêtres minimaliste.

Input : Support complet Clavier/Souris (PS/2).

Stabilité : Environnement de test prêt pour l'intégration de l'IA Core.

🧬 Architecture Polymorphe
Le projet explore l'idée d'un Compilateur Dédié capable de traduire des modèles d'IA directement en instructions machine optimisées pour le "Freestanding Environment" de Jarvis.

Réduction de Latence : Zéro appel système (Syscalls) entre l'IA et le matériel.

Gestion Fibreuse : Allocation mémoire dynamique basée sur la topologie des neurones.

Agnosticisme : Le moteur de calcul (Myst AI) est conçu pour être portable entre l'hôte (Windows/WSL) et l'invité (Jarvis OS).

🛠️ Build & Run
Le projet utilise Limine comme bootloader moderne pour garantir un accès direct au Framebuffer.

Dépendances
gcc-x86-64-elf, nasm, xorriso, mtools, qemu-system-x86_64

Lancement
Bash
make
make run
📍 RoadMap
[ ] Finalisation du compilateur de "Fibres" (Myst-Compiler).

[ ] Mapping direct de la mémoire IA dans le Framebuffer pour visualisation.

[ ] Système de communication inter-mondes (Windows <-> Jarvis).
