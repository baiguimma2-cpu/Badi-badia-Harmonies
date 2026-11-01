# Badi-badia Harmony — plugin VST3 / AU

Génère jusqu'à 6 voix d'harmonie (unisson, tierces, quinte, octaves) à partir d'une seule voix.
Les harmonies sont calées sur la tonalité choisie. Latence : ~25 ms (déclarée au logiciel, la voix directe est alignée automatiquement).

## Option 1 — Compiler sans rien installer (recommandé)
1. Crée un compte gratuit sur github.com et un nouveau dépôt.
2. Envoie-y tout le contenu de ce dossier (y compris le dossier `.github`).
3. Onglet **Actions** → « Build Badi-badia Harmony » → attends la fin.
4. Télécharge les fichiers `BadiBadiaHarmony-windows-latest` (VST3) et `BadiBadiaHarmony-macos-latest` (VST3 + AU).

## Option 2 — Compiler sur ton ordinateur
Prérequis : CMake 3.22+, Git, et Visual Studio 2022 (Windows) ou Xcode (Mac).

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
Les plugins sont dans `build/BadiBadiaHarmony_artefacts/Release/`.

## Installation
- **Windows** : copie `Badi-badia Harmony.vst3` dans `C:\Program Files\Common Files\VST3`.
- **Mac VST3** : `~/Library/Audio/Plug-Ins/VST3`
- **Mac AU** : `~/Library/Audio/Plug-Ins/Components`
  Si macOS bloque le plugin : `xattr -cr "/chemin/vers/le/plugin"` dans le Terminal.

Relance ensuite ton logiciel (FL Studio, Ableton, Logic, Reaper…) et rescanne les plugins.
Place le plugin sur une piste de voix (insert/effet).

## Utilisation
1. Choisis la **tonalité** et le **mode** de ta chanson.
2. Active les **voix** voulues (ou un préréglage : Duo, Trio, Chœur gospel, Ange, Profond).
3. Règle **Ma voix**, **Harmonies** et **Largeur stéréo**.

Conseil : fonctionne mieux sur une voix seule, sans réverbération ni écho, et chantée juste dans la tonalité choisie.

## Licence JUCE
JUCE est gratuit pour un usage personnel / projets open source (AGPLv3) ; la vente d'un plugin fermé demande une licence JUCE (juce.com/get-juce).
