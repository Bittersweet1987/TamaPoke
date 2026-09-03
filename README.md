# TamaPoke

[![Flash in browser](https://img.shields.io/badge/flash-in%20browser-FF6B00?logo=googlechrome&logoColor=white)](https://bittersweet1987.github.io/TamaPoke/web/)
[![MakerWorld](https://img.shields.io/badge/MakerWorld-3D%20case-00AE42?logo=bambulab&logoColor=white)](https://makerworld.com/es/models/2937822-tamapoke-a-pokemon-pokeball-tamagotchi)
![Board](https://img.shields.io/badge/board-ESP32--S3%20round%20AMOLED-E7352C?logo=espressif&logoColor=white)
![Firmware](https://img.shields.io/badge/firmware-v1.36.0--moves--switch-8A2BE2)
![Species](https://img.shields.io/badge/species-1025%20(Gen%201--9)-3B4CCA)
![Code](https://img.shields.io/badge/code-MIT-blue)
![Languages](https://img.shields.io/badge/languages-6-FFCB05)
[![Stars](https://img.shields.io/github/stars/Bittersweet1987/TamaPoke?style=flat&logo=github&color=yellow)](https://github.com/Bittersweet1987/TamaPoke/stargazers)

> **This is Bittersweet1987's expanded fork of TamaPoke.** Use only the installer
> linked in this README. The original upstream installer is a different build
> and must not be used for this fork; upstream links are kept only as source and
> attribution credits.

## Install / Flash

**For normal use, use only the web installer. Do not download anything manually.**

### [Open the TamaPoke web installer](https://bittersweet1987.github.io/TamaPoke/web/)

You do **not** need to download the ZIP, release files, Arduino project, firmware
`.bin` files or `sprites.pak` manually. The web installer flashes the firmware
and loads the sprites from the browser.

Use desktop **Chrome or Edge**, connect the Waveshare ESP32-S3 board by USB, and
follow the buttons on the installer page. If you are updating an existing pet,
install **without erase** to keep your save.

Short version:
1. Open the web installer link above.
2. Connect the Waveshare ESP32-S3 board by USB.
3. Click install.
4. Leave **erase device** unchecked when updating.
5. Use the installer page to load sprites if needed (this is now a large,
   ~280 MB one-time download covering all 1025 species — see
   [Generate and load the sprites yourself](#generate-and-load-the-sprites-yourself)).

Everything else in this repository is only for developers who want to build or
modify the firmware themselves.

A Pokémon-inspired tamagotchi for the
**Waveshare ESP32-S3-Touch-AMOLED-1.75** (round 466×466 AMOLED, CO5300 driver
over QSPI, CST9217 touch over I2C). Raise any of **all 1025 Pokémon through
Generation 9**, evolve it, train it, battle your way through 5 regional Gym
leagues, and complete the Pokédex (shinies included).

> **Personal, non-commercial fan project.** Code is MIT; the sprites are from
> PMD SpriteCollab (CC BY-NC, Pokémon © Nintendo/Game Freak), the species cries
> are sourced from PokéAPI's cries archive (ultimately extracted from the
> official games), and the 3D case is CC BY-NC-SA. See **[License](#license)**
> and **[Credits](CREDITS.md)**.

🔴 **3D-printed Pokéball case source/remix credit → [MakerWorld](https://makerworld.com/es/models/2937822-tamapoke-a-pokemon-pokeball-tamagotchi)** · install this fork here → **[web installer](https://bittersweet1987.github.io/TamaPoke/web/)**

## Status

Running on hardware. The public build (web installer and source) is
`1.36.0-moves-switch` (see
[How this fork compares](#how-this-fork-compares-to-socquiquetamapoke--dylanpdaotamapoke)
below for what that build adds on top of the two projects this was built from).
Implemented: **all 1025 species (Gen 1–9)** + shinies animated from microSD,
full life cycle (egg by rarity → evolution → farewell/release/runaway, each
gated behind a decision dialog), bred-Pokédex with gallery, battle stats
(genes + training, real Pokémon-style level scaling), a full **Party system**,
**5-region Gym league** (see below) and a **pet-switching** system to make any
previously-raised Pokémon your active companion again, retention hooks
(streak / bond / medals / name), biome + real-time backgrounds, ball minigame,
training bag, animated bath, RTC with offline progression, battery (AXP2101)
and PWR button, anti-burn-in dimming, **sound (ES8311) with real species
cries**, **6 UI languages (English default)**, **starter choice on first
run**, a one-click **web installer**, manual and rare optional wild battles
(now including evolved forms, gated by level), one-shot catch attempts after
wins with a small battle-win experience bonus, a full **real movepool** (798
moves, real per-species per-level learnsets) with a Pokédex page to freely
reassign a Pokémon's 4 active attacks and a proper learn/replace dialog once
all 4 slots are taken, extra minigames, pet events, personality/profile
cards, daily goals, collector ranks, unlockable cosmetic frames, richer sound
effects and a subtle moving `@SE` attribution watermark on rendered screens.
The Expedition card adds timed background tours and a persistent item
inventory (up to 80 of each item).

> **Species cries are real Pokémon cries**, sourced from
> [PokéAPI's cries archive](https://github.com/PokeAPI/cries) (`legacy/` for
> Gen 1–5, `latest/` for Gen 6–9) and converted to raw PCM for playback from
> the SD card (`audio.cpp: playSpeciesCryFromSD()`). They ultimately originate
> from the official Pokémon games — see **[License](#license)** for the
> non-commercial fan-project terms this falls under.

The final local hardware path is complete. Optional future work is limited to
longer soak tests and balance tuning; it is not required for normal use.

## How this fork compares to socquique/TamaPoke & DylanPDao/TamaPoke

This is Bittersweet1987's fork of [socquique/TamaPoke](https://github.com/socquique/TamaPoke)
(the original project for this board), and it also ports/adapts parts of its
battle and trainer system from [DylanPDao/TamaPoke](https://github.com/DylanPDao/TamaPoke),
a separate expanded fork of the same original (see [Credits](CREDITS.md) for
exact attribution). For anyone comparing the three, here's what's specific to
this fork:

**vs. [socquique/TamaPoke](https://github.com/socquique/TamaPoke)** (the
original — Gen 1 only, no battle system yet):
- **1025 species, Gen 1–9** instead of the original 151 (Gen 1 only).
- **Wild battles and a full turn-based battle system** — upstream still lists
  battles as a roadmap item, not implemented.
- A **5-region Gym league** (Kanto/Johto/Hoenn/Sinnoh/Unova, 8 leaders + Elite
  Four + Champion each, Easy/Hard modes) and a **Party system** (team of up to
  6, live-updating stats as members keep leveling).
- A real **movepool** (798 moves from PokéAPI, real per-species per-level
  learnsets for all 1025 species) with a Pokédex page to freely customize a
  Pokémon's 4 active attacks, plus a proper "learn this move? which one do
  you replace?" dialog once all 4 slots are taken — nothing is silently
  overwritten.
- **Pet-switching**: raise more than one Pokémon over time and freely switch
  which one is your actively-cared-for companion — it resumes exactly where
  it left off (level, genes, training, moves, bond, nickname) — while each
  individual can still only be sent off (farewell/release/runaway) once.
- Wild encounters can be **evolved forms** too (gated by the species' real
  evolution level), levels skew fairer around your own, and every win now
  gives a small battle "experience" bonus (extra age-minutes) on top of the
  training reward.
- Expeditions with a background-tour system and item inventory, daily goals,
  personality/profile cards, medals, collector ranks/frames, a pedometer/steps
  system, real species cries from PokéAPI (instead of synthesized chirps), and
  a generation-paginated Pokédex with an extra "sort by strength" view.

**vs. [DylanPDao/TamaPoke](https://github.com/DylanPDao/TamaPoke)** (the fork
this ported the Gym/trainer/badge system from):
- **1025 species (Gen 1–9)** instead of 386.
- The full **798-move real movepool** with a genuine per-level learnset for
  every one of the 1025 species (fetched from PokéAPI), instead of a curated
  77-move pool — plus the attack-customization/learn-replace UI described
  above, which that project doesn't have.
- **Pet-switching**, as above: that project's Party keeps up to 6 pets but
  freezes their moves once "banked" — here you can pull any previously-raised
  Pokémon back out as your live, ticking, actively-cared-for main pet and keep
  developing it.
- **Genes** (a 90–110% roll per stat at hatch) instead of IVs (0–31), and
  battle stats now scale with level using the real
  `(2×base×level)/100 + level + training` formula instead of a flatter curve.
- Wild encounters can spawn **evolved forms** (gated by evolution level) with
  a small experience bonus per win, and the internal NVS storage for the
  caught/bred Pokédex history was enlarged to avoid a silent-data-loss class
  of bug on very large save files.

## Trainers, Gyms & the League

Battle your way through **5 classic regions** — Kanto, Johto, Hoenn, Sinnoh,
Unova — each with **8 Gym leaders**, an **Elite Four** and a **Champion** (13
trainers per region, 65 total), on a choice of **Easy** or **Hard** mode (Hard
raises every enemy's level by +5, capped at 100). Rosters are full 6-Pokémon
teams with switching, not single-Pokémon fights.

- Open the **Gym** card to pick a region/difficulty and fight the next
  unlocked leader; the **Top 4** card shows the full ladder (all 5 slots are
  always visible, even locked ones) up to the Champion.
- Winning a Gym battle awards its **Badge** and a one-time **training-stat
  bonus** (e.g. "SPEED +8") — badges are collectibles and battle gates, they
  do **not** apply a passive stat boost while held (matching the mechanic in
  the reference project this was ported from).
- The **Badges** card lists every Orden with its real badge icon, the leader
  it came from, and the exact reward you got.
- Badge names, gym rosters and badge artwork are ported/researched from
  [DylanPDao/TamaPoke](https://github.com/DylanPDao/TamaPoke) (MIT) and
  [SteGriff/pokemon-badges](https://github.com/SteGriff/pokemon-badges)
  (CC BY 3.0) — see [`CREDITS.md`](CREDITS.md).

## Party

Build a team of up to **6 Pokémon** from anything you've caught or bred:

- From a known species' Pokédex detail page, tap **INS TEAM** on its
  "caught" or "bred" page to add that exact specimen to your Party.
- Party members are **live references**, not frozen snapshots: a Pokémon
  you're actively raising keeps leveling and its stats keep updating in the
  Party list as it grows.
- Duplicate species are blocked; you can remove any member from the Party
  card at any time.
- Catching (or hatching) a species you already have a saved specimen of opens
  an **ALT / NEU** comparison dialog so you choose which one to keep — this
  never happens silently.

## Game manual (the actual numbers)

A quick reference to how the game really works (values straight from the code).

### Time & leveling
- **1 real minute = 1 in-game minute.** Your Pokémon gains **+1 level every hour**
  of real time. Leveling is purely time-based — caring well doesn't speed it up,
  but neglect *delays evolution*.
- It keeps **aging while powered off** (the RTC runs), catching up to **2 weeks** max.

### The four stats (0–100)
Needs: **FOOD**, **JOY**, **ENE** (energy), **HYG** (hygiene). Start 80 / 80 / 80 / 100.
While **awake**, per minute:

| Stat | Drain/min | Notes |
|---|---|---|
| FOOD | −2 | |
| ENE | −1 | −1 extra if overweight (weight > 50 → sluggish) |
| HYG | −1 | **−4 more per poop** on screen (max 3 poops) |
| JOY | −1 | **−2 extra** if FOOD < 30, **−2 extra** if HYG < 30 |

- ~**15 %/min** chance to poop (only if FOOD > 40). Poops tank hygiene fast.
- **Care slip-up** = letting any stat hit **≤ 10** (30-min cooldown so it counts once).
  Each slip-up **delays evolution by 1 level** and cools the bond.

### Actions
- 🍎 **Berry** (3 flavors): +25 FOOD. Each species has a **hidden favorite flavor**
  → +35 FOOD, +10 JOY, ♥, bond, and it gets revealed.
- 🍬 **Candy:** +10 FOOD, +12 JOY, but **+12 weight** (fattening).
- ⚽ **Play / minigames:** ball, catch, memo, clean and type add variety; rewards
  are moderate and train SPEED/DEFENSE/ATTACK or improve care.
- 🥊 **Training bag:** trains **STRENGTH** (~4 hits = 1 pt, cap +18/session), tires it.
- 🫧 **Bath:** clears poops, HYG → 100.
- 👆 **Pet it:** +5 JOY + bond.
- 🌙 **Sleep:** rest — ENE **+6/min**, needs drain ~**4× slower** with floors
  (FOOD 30 / JOY 35 / HYG 45). No poops, no slip-ups, can't run away while asleep.

### Play menu and minigames
Tap **Play** to open a small menu:

- **BALL**: keep the Pokéball in the air by tapping it. It starts fast, gravity
  rises with score, wall bounces matter, and missed drops end the run after 3
  misses. Rewards joy and SPEED training.
- **CATCH**: tap the appearing berry/icon before it disappears. Targets vanish
  faster as your score rises; 3 mistakes or the timer ends the run. Rewards joy
  and SPEED training, with small food/energy cost.
- **MEMO**: watch and hear the 4 pads flash, then repeat the sequence during the
  clear `YOUR TURN` phase. Each pad has its own tone; correct taps get a green
  ring, and a mistake briefly shows the expected pad before the result. Each
  cleared round adds one more step. Rewards DEFENSE training plus small joy/bond.
- **CLEAN**: tap dirt spots before the timer ends. It improves hygiene, joy and
  may clean one poop, while costing a little energy.
- **TYPE**: pick the type that is strong against the shown enemy type. It trains
  ATTACK and teaches the same lightweight type logic used in battles.

All minigame records are saved and shown on the Personality/Records area.
The daily **CATCH 5** goal counts Catch-minigame targets and successful wild
Pokémon catches.

### Expeditions & inventory

The **Expedition** card offers one background tour at a time. Your Pokémon stays
on the normal screen and can still be fed, played with, put to sleep or used in
battles while the timer runs.

- Start a **15 / 30 / 60 minute** tour for **12 / 20 / 32 ENE**. An egg,
  sleeping Pokémon, active/unfinished tour or full inventory blocks a new tour.
- Every finished tour has one stored reward. Open the card and **Claim** it; a
  reboot cannot reroll the reward.
- Good departure care matters: average needs of at least 60 plus Bond 20, or
  average 80 plus Bond 50, raises the Training Token chance. Tours never fail.
- The inventory holds at most **3 of each**: Trail Snack (+25 FOOD, +5 JOY),
  Energy Tonic (+30 ENE), Care Kit (+30 HYG and one poop), and Train Token
  (+2 chosen ATK / DEF / SPD training, capped at 100).

### Collection ranks and frames

Raised and caught entries combine into one **known** Pokédex total. At **10,
25, 50, 100, 151, 160, 200 and 251** known species, TamaPoke unlocks a new cosmetic collector
frame and rank. Open the **Profile** card to see your rank and choose any frame
you have unlocked. Frames are visual only; they do not change battles, catches
or rewards.

> The frame-unlock thresholds above are still tuned for the original 251-species
> run and were **not** rescaled for the Gen 6–9 expansion — the last frame
> unlocks at 251 known, well short of all 1025. Rebalancing these (and the
> "all lines completable" egg bias below, which already scales automatically)
> is an open follow-up.

### Eggs & who you get (spawn odds)
- **First ever pet:** you pick a starter — **Bulbasaur / Charmander / Squirtle**.
- Hatch the egg: tap it **3×** (or wait — it hatches on its own).
- Every later egg rolls a **rarity tier**, then picks any matching species from
  the full **Gen 1–9 dex** (over the 547 base forms that come from eggs — the
  rest are evolution-only and reached by leveling up):

| Tier | Base chance | After a proper goodbye | # species |
|---|---|---|---|
| ✨ Legendary | ~3 %\* | ~10 % | 93 |
| 🔵 Rare | ~27 % | ~45 % | 48 |
| ⚪ Common | the rest | the rest | 406 |

  \* Legendaries only start appearing once you've **registered ≥ 25** Pokémon.
- A daily **streak** and high **bond** push rare/legendary odds higher.
- A clean **goodbye blesses** the next egg; a **run-away curses** it (forces Common).
- Within a tier it favors species whose **evolution line you haven't finished** (so
  all 1025 are completable, Gen 6–9 included — eggs draw from the whole dex,
  not just a fixed subset).
- **Shiny:** base **1 / 48** (→ **1 / 24** right after a goodbye), improved by
  streak/bond down to a best of **1 / 8**. Tracked separately in the dex.
- Every hatch rolls unique **genes** (90–110 % per stat) — no two are identical.

### Evolution
- Triggers when **level ≥ its evolution level** (16 for most base forms; ~30 for
  stone-style, ~40 for trade-style) and **at least 3 of 4 care values are >40**.
  Friendship, day/night, stat-ratio and branched Gen-2 evolutions are supported;
  item/trade-only triggers use the game's level-equivalent rules.
- **Never automatic** — a **red button** appears once the level is reached and
  **you tap to witness it** (with a flicker between the old and new form). Each
  **slip-up delays it by 1 level**. If a need is below 40, the button stays but
  tapping it reminds you to raise the bars first.
- You can **decline** ("keep form"); it re-offers at the next level, or after
  **one day** if you are already at level 100.
- *Eevee* branches toward whichever evolution you're still missing.

### The three endings (you choose & witness each — none auto-fire)
- 💛 **Farewell** — when it's a **final form** that has lived **3 days**. A button
  appears; triggering it **blesses your next egg**. You can **postpone** ("stay
  together", re-offered in a day). The good ending.
- 💔 **Run-away** — if you let **all four stats sit at 0 for a full hour**. A single
  act of care cancels it. It **curses the next egg** (forces Common). The sad ending.
- 👋 **Release** — long-press the creature to let it go on your terms (neutral).

After any ending, a **new egg** appears.

### Bonds, streaks, medals, Pokédex
- **Streak** (player-wide, survives across pets): first care each real day; milestones
  at **3 / 7 / 30 / 100** days; skipping a day breaks it.
- **Bond** (per pet, resets on hatch): grows with affection (**cap +8/day**), cools on
  neglect. Both streak & bond improve egg/shiny odds.
- **8 medals** (Lv10/25/50, favorite berry found, 7-day streak, max bond, final form,
  "fit" = weight 0 & no slip-ups), per-pet + a global counter.
- **Pokédex:** raising a species registers it; caught wild Pokémon get a separate
  caught marker. The gallery can show known, raised and caught entries, with
  localized Pokémon names in all supported languages.

### Battle stats
ATK / DEF / SPD = real **Pokémon base stats** × genes + level + training (STRENGTH ← bag,
SPEED ← minigames, DEFENSE ← memo/good care). Wild battles can be started from
the Battle card, and rare optional wild prompts can appear on the main screen.
Battles are turn-based with quick/heavy attacks, dodge/counter and limited rest.
Wins/losses/streaks are tracked and wins give small training rewards.
After a win, you get one optional catch attempt; caught wild Pokémon are marked
separately in the Pokédex and do not replace the active pet.
Wild levels skew fairer now: most are near your level, some are a few levels
below, and rare stronger fights still happen. If you lose but bring the wild
Pokémon below 30% HP, a low-chance respect catch may appear.

Battle actions:

- **Attack** opens quick/heavy attack choices. Quick is safer; heavy is stronger
  but less reliable.
- **Dodge** can avoid damage and prepares a counter. The next attack gets a
  moderate damage boost, still capped to avoid cheap one-hit fights.
- **Rest** heals during battle but only has 2 uses per fight.
- **Run** leaves the fight with no reward or catch chance.

Types matter in both directions: effective and weak matchups adjust damage, and
type labels are visible in battle.

Catch rules:

- After a **win**, you get exactly one optional catch attempt: **CATCH** or
  **LEAVE**.
- After a **close loss** with the enemy under 30% HP, a low "respect catch" may
  appear. It can register the Pokémon, but gives no win, no streak and no reward.
- Caught Pokémon go into the Pokédex/Box collection only; they do not replace your
  active pet.

## Hardware

- Board: [ESP32-S3-Touch-AMOLED-1.75](https://www.waveshare.com/wiki/ESP32-S3-Touch-AMOLED-1.75)
  — get the **Standard** (no case) or **-G** (GPS, also fits) version; **not the "-B"**
  (ships with a protective case that won't fit). The separate "1.75**C**" is a different board.
- Round 466×466 AMOLED, **CO5300** driver (QSPI, 80 MHz)
- Capacitive touch **CST9217** (I2C, address 0x5A)
- **AXP2101** (power management + battery + PWR button), **PCF85063** (RTC),
  microSD slot, **ES8311** audio codec (→ amplifier → external speaker on the
  MX1.25 connector)
- Pins taken from the [official Waveshare repo](https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75) (see `pin_config.h`)

## Developer build info

You can ignore this section if you only want to play TamaPoke. The public install
path is the browser-based web installer above.

### Libraries (Arduino IDE / arduino-cli)

| Library | Author | Use |
|---|---|---|
| GFX Library for Arduino (`Arduino_GFX`) | moononournation | CO5300 over QSPI + framebuffer in PSRAM |
| SensorLib | Lewis He | CST9217 touch + PCF85063 RTC |
| XPowersLib | Lewis He | AXP2101 PMU (battery, brightness, PWR button) |
| ESP_I2S (bundled in the ESP32 core) | Espressif | I2S to the ES8311 codec |

### IDE setup / build

- Board: **ESP32S3 Dev Module** · Flash **16MB** · PSRAM **OPI PSRAM**
  (required: the 466×466×16-bit framebuffer ≈ 434 KB lives in PSRAM) ·
  Partition Scheme with FAT (e.g. `16M Flash (3MB APP/9MB FATFS)`) ·
  USB CDC On Boot **Enabled**

```bash
FQBN="esp32:esp32:esp32s3:CDCOnBoot=cdc,FlashSize=16M,PSRAM=opi,PartitionScheme=app3M_fat9M_16MB"
arduino-cli compile --fqbn "$FQBN" .
arduino-cli upload -p /dev/cu.usbmodemXXXX --fqbn "$FQBN" .
```

### Local web installer

The hosted installer is the recommended path:
[`https://bittersweet1987.github.io/TamaPoke/web/`](https://bittersweet1987.github.io/TamaPoke/web/)

For local development, `web/index.html` flashes the firmware (ESP Web Tools) and
pushes the sprites to the SD over Web Serial. Serve it over HTTPS or `localhost`
(secure context) and open it in **Chrome/Edge**. See [`web/README.md`](web/README.md).

For local hardware tests there is a separate page at
`http://127.0.0.1:8000/dev.html`. It uses `manifest-local.json` and the
`1.35.3-soft-step-local` build with extra serial test commands. Its step counter
uses raw accelerometer data with cadence filtering and counts immediately after
USB disconnect. Keep this page
separate from the public `index.html`; do not publish the local manifest or
local-test binaries to GitHub Pages.

This local page is the final expanded hardware-test path for this fork. It is
intentionally separate from the hosted public installer until you explicitly
publish that build.

### Generate and load the sprites yourself

Normal users should use the web installer instead. This is only needed if you are
rebuilding the sprite bundle yourself.

All sprites come from **[PMD SpriteCollab](https://github.com/PMDCollab/SpriteCollab)**
(CC BY-NC). You can regenerate the whole set and load it onto your board with the
pipeline below — the firmware accepts files over USB (PUT protocol with per-block
ACK), so you don't have to remove the card (it formats the SD to FAT if needed).

```bash
python3 tools/pack_pmd.py 1 649           # Gen 1-5: fetch + pack species + shiny -> tools/sdcard/mons/p[s]NNN.bin
python3 tools/gen_dex_650_1025.py > out.txt  # Gen 6-9: regenerate dex.h data from PokéAPI (one-off, see the script header)
python3 tools/pack_pmd.py 650 1025        # Gen 6-9: fetch + pack species + shiny
python3 tools/pack_pmd_fallback.py        # static-frame fallback for species SpriteCollab has no animation for
python3 tools/pack_cries.py 1 1025        # fetch + convert all species cries -> tools/sdcard/cries/pNNN.pcm
python3 tools/make_thumbs.py              # Pokédex thumbnails (from the PMD sprites) -> thumbs.bin
python3 tools/send_sd.py                  # send tools/sdcard/mons/* to the board's SD over USB
python3 tools/send_cries.py               # send tools/sdcard/cries/* to the board's SD over USB
```

To make the **one-click web-installer bundle** instead of sending over USB:

```bash
python3 tools/pack_bundle.py    # bundle tools/sdcard/mons/* into web/sprites.pak
bash tools/build_web.sh         # also rebuilds web/firmware/*.bin from the current sketch
```

Then load it from the web installer's **"Load sprites"** button (or `send_sd.py`
above). `pack_pmd.py`/`pack_cries.py` also take individual dex numbers or ranges,
e.g. `pack_pmd.py 7 25`. (~280 MB total for all 1025 normal/shiny PMD sprites +
fallback frames, another ~18 MB for cries. Versioned under `tools/sdcard/`.)

## How to play

On first run you **choose a starter** (Bulbasaur / Charmander / Squirtle). After
that you start with an **egg**. Tap it 3 times or wait and it hatches. From then
on, care for your companion:

**Four stats** that decay: **FOOD**, **JOY**, **ENE** (energy), **HYG** (hygiene).
If one bottoms out it counts as a *slip-up*.

**Buttons (bottom arc, icons):**
- 🍎 **Feed** → food menu: 3 berries (each species has a hidden favourite that
  gives a bonus) and a candy (+happiness but it fattens; weight makes it sluggish).
- ⚽ **Play** → game menu: ball, catch, memo, clean and type.
- 🌙 **Light** → sleep/wake (recovers energy, dims the screen). While asleep,
  needs decay much slower (rest).
- 🫧 **Bath** → a foam scene that cleans up the poops.

**Touch gestures:**
- Tap the creature = pet it (+happiness, bond; an original species chirp in SND ALL).
- Horizontal swipe = open the **Pokédex / gallery**.
- Vertical swipe up = open the **card view** (Profile / Personality / Daily /
  Box / Battle / Medals / Progress / Expedition / Steps-Trail; swipe between
  them; tap the name on Profile to rename; on Battle you can start wild battles
  or open the training bag).
  The Box card can page through caught Pokémon and cycle sorting by Dex, type,
  or raised status.
- Swipe down = **set the clock** and pick the **language**, sound level and
  optional **Power Save**, or open the built-in **Help** pages. Power Save keeps
  normal play unchanged, but reduces idle work and uses short ESP32 light-sleep
  pauses on battery while idle or screen-off.
- Long press (3 s) on the creature = **release** dialog.

**Physical PWR button:** short = screen on/off · long (4 s) = full power-off
(the RTC stays alive, so time passes even while it's off).

**Day and night** follow the clock you set (swipe down). The habitat already
changes at dawn/day/dusk/night. While awake at night, **FOOD drops slower**.
Wild encounters bias toward Grass/Flying in the morning and Ghost/Poison/Bug
at night. The first visit each morning shows a short greeting (it will not
wake a sleeping pet).

**Motion (QMI8658):** shake the Pokeball on the main screen to play (small JOY,
cooldown, daily cap). Walking with the screen off still counts steps and slowly
raises JOY/BOND; the persistent **today + total** counters are shown in the
top-left HUD and on the Steps card. Daily trail rewards unlock at **500 / 2,000 /
5,000** steps. Wild Shiny odds improve with the day's steps (base 1/512,
bounded near 1/128) and the catch chance gets a small capped bonus. USB charging
ignores steps so a desk bump does not farm stats.

### Card view
Swipe up from the main screen, then swipe between **12 cards**, in this order:

1. **Profile**: nickname, age, bond/streak, favorite berry info, collector rank,
   frame selection and rename access.
2. **Personality**: play-style personality plus records for ball, catch, memo,
   clean, type and training bag.
3. **Battle**: ATK/DEF/SPD/weight, W/L/streak/best, **Wild Battle** and
   **Strength Training**.
4. **Expedition**: launch 15/30/60-minute background tours, collect their reward
   and use the four stored item types.
5. **Daily**: optional daily goals. Completing them gives small rewards; ignoring
   them has no penalty.
6. **Party**: your team of up to 6 (see [Party](#party) above) — add, remove,
   and watch stats update live as members grow.
7. **Gym**: pick a region + Easy/Hard mode and fight the next unlocked leader
   (see [Trainers, Gyms & the League](#trainers-gyms--the-league)).
8. **Top 4**: the Elite Four + Champion ladder for the current region/mode.
9. **Badges**: every Orden you've won, with its icon, leader and reward.
10. **Box**: caught Pokémon collection with paging and sorting by Dex, type or
    raised status.
11. **Progress**: level, next level, evolution readiness and care slip-ups.
12. **Medals**: individual medals and progress.

### Sound modes
Swipe down to settings and tap the sound button:

- **TON VIEL / SND ALL**: all feedback, including taps, swipes, menu sounds,
  card/gallery changes, minigame start, training hits, ball bounces/misses,
  memo sequence steps and real species cries on pet taps, known Pokédex
  details and wild-battle starts.
- **TON MIT / SND MID**: keeps important care, battle, catch, event and result
  sounds plus species cries for known Pokédex details and wild-battle starts,
  but removes many tiny repeated UI/minigame noises and pet-tap cries.
- **TON WEN / SND LOW**: only major events such as hatch, evolution, medals,
  level, win/loss and catch result.
- **TON AUS / SND OFF**: silent.

## Decisions: you choose, and you watch

The three life-cycle endings and evolution **don't happen on their own** — when
the conditions are met a button appears and you tap it (so you're present to
witness it), each opening a two-option dialog:

- **Evolution** (red button): *Evolve* (epic animation: halo, rays, sparkles and
  a **flicker between the old and new form**) or *Keep form* (re-offered next level).
- **Farewell** (gold button, final form + 3 days): *Say goodbye* (warm farewell,
  rising hearts → new egg) or *Stay together* (keep your companion; re-offered in
  a day). Tension: a maxed-out friend vs. completing the Pokédex.
- **Runaway** (dark button, total neglect for 1 h): a somber "feels abandoned"
  ending in the rain — caring for the creature cancels it.

## Sprites and cries: PMD SpriteCollab + PokéAPI

- **PMD SpriteCollab** (everything — main screen, stat card, minigame **and the
  Pokédex grid + detail view**): behaviour sprites — `tools/pack_pmd.py` packs
  actions (Idle, Walk L/R, Sleep, Eat, Hurt, Attack, Pose, Nod, DeepBreath) into
  the multi-action **TPK2** format (`/mons/pNNN.bin`). The engine in `TamaPoke.ino`
  makes the creature wander, gesture, curl up to sleep, chew and wince. Anchored by
  the feet (lowest content row), not the canvas. The Pokédex thumbnails
  (`thumbs.bin`, TPTH) are derived from these by `tools/make_thumbs.py`.
- **~54 species fall back to a single static frame** instead of a full
  animated sprite (`tools/pack_pmd_fallback.py`, sourced from the official
  PokéAPI pixel sprite): mostly Generation 8/9 species that PMD SpriteCollab
  doesn't have animation data for yet. Everything else — 971 of 1025 species,
  normal **and** shiny — is fully animated.
- **In-house workshop** (`tools/sprites.py`): 9 primitive-drawn sprites as a
  no-SD fallback + the UI icons. Generates `species.h`. Preview in
  `tools/sheet.png`, emit with `python3 tools/sprites.py emit`.
- **Species cries** (`tools/pack_cries.py`): fetched from
  [PokéAPI's cries archive](https://github.com/PokeAPI/cries) — the `legacy/`
  set for Gen 1–5, falling back to `latest/` for Gen 6–9 — converted to raw
  16 kHz mono PCM16 (`/cries/pNNN.pcm`) and played back by
  `audio.cpp: playSpeciesCryFromSD()` on pet taps, known Pokédex details and
  wild-battle starts (in **SND ALL**/**SND MID** modes).

`sdmon.h/.cpp` loads the PMD sprites into PSRAM (`PmdMon` for TPK2) plus the
thumbnails (`SdThumbs`). `SdMon` (TPK1) remains as a dormant legacy fallback only.

## Pokédex and species data

`tools/dex_data.py` (Gen 1–5) and `tools/gen_dex_650_1025.py` (Gen 6–9, fetched
live from [PokéAPI](https://pokeapi.co)) are the **data sources**: name, slug,
type (accent colour + background biome), evolution line with levels, rarities
and starters, plus localized names/descriptions in all 6 UI languages.
`tools/dex_stats.py` / `gen_dex.py` emit `dex.h` (the `DEX_TBL[DEX_COUNT + 1]`
table, `DEX_COUNT = 1025`); `tools/merge_dex_650_1025.py` was the one-off
script that spliced the Gen 6–9 data into it. The pet's identity is its
Pokédex number (persisted in NVS).

- **Evolution** gen-1 style (levels 16/36/…; stones ≈30, trade ≈40; Eevee
  branches to whichever evolution you're missing). Each slip-up delays it 1
  level; it evolves only when at least 3 of 4 care values are strictly above 40
  and never while asleep.
- **Generation overview**: the Pokédex opens on a generation picker, paginated
  **5 generations per page** (Gen 1–5, then Gen 6–9), plus a dedicated third
  page — **"NACH STÄRKE"** — that lists every Pokémon you've *caught*
  (not bred), sorted **descending by total strength** (the sum of all 6
  battle stats: HP/ATK/DEF/SpA/SpD/SPD at that specimen's saved level and IVs).
- **Detail pages** for a known species: sprite + name, description, a
  **Moves** page (learnset with level learned, type badge, STAB indicator and
  power/status, styled after
  [DylanPDao/TamaPoke](https://github.com/DylanPDao/TamaPoke)), and separate
  **caught** / **bred** pages (each with its own **INS TEAM** button) when you
  have that origin.

## Battle stats and training

Each creature has ATK/DEF/SPD = real Pokémon base × **genes** (90–110 %, rolled at
hatch) + level + **training**:
- SPEED ← the minigame
- DEFENSE ← sustained good care (12 h with no slip-ups)
- STRENGTH ← the training bag (whacking)

Shown on the Battle page of the stat card. The (hidden) weight goes up with candy
and burns off with training.

## Daily goals, events and personality

- **Daily goals**: three small optional tasks per day, shown on the Daily card.
  Completing one gives a small joy/bond bonus; ignoring them has no penalty.
- **Pet events**: rare positive bubbles on the main screen (berry, heart,
  sparkle) that can be tapped for tiny rewards.
- **Personality**: derived from play style and shown on the Personality card
  without changing balance yet.

## Retention: streak, bond, medals, name

- **Streak** (the player's, persists across creatures): the first care of each
  real day advances the streak; 3/7/30/100 milestones are celebrated; skipping a
  day breaks it. Flame badge on the main screen.
- **Bond** (the creature's): rises slowly with care and petting, drops with slip-ups.
- **Medals** for the individual (level, berry, streak, bond, final form, fit) +
  a global counter. Medals page of the stat card.
- **Name**: touch keyboard; the nickname rules the header and the card.

High streak and bond **improve the egg roll** (rarity and shiny): caring well
always pays off.

## Life cycle, eggs by rarity, languages

The life cycle lasts **3 days** of play. Three endings (all leave a new egg):
**farewell** (final form + 3 days), **release** (long press), **runaway** (all 4
bars at zero for 1 h). Each bred species is recorded in the **bred Pokédex**
(normal and shiny separately).

The egg rolls rarity over the full **Gen 1–9** base forms, **biased towards the
lines you're missing** (all 1025 are completable), blessed by
a farewell and punished by a runaway. Legendaries only with 25+ registered.
**Shiny** 1/48 (better with streak/bond/farewell).

**Languages:** the UI ships in 6 languages — English (default), Spanish, French,
German, Italian, Portuguese — switchable from the settings screen (swipe down).

## Backgrounds: biome + real time

The idle screen paints the sky from the **RTC's real time** (dawn / day / dusk /
night with moon and stars) and the ground from the **type's biome** (meadow,
beach, forest, volcano, mountain, snow). Sleeping forces night.

## Layout

- `TamaPoke.ino` — init, game loop, render of every screen, gestures, serial console, audio,
  Pokédex screens (generation overview, strength sort, moves page), Gym/Top4/Badges cards
- `pet.h` / `pet.cpp` — pet state and logic (stats, evolution, life cycle, streak/bond/medals,
  caught/bred Pokémon history, NVS)
- `party.h` / `party.cpp` — the Party system (up to 6 members, live-referenced stats)
- `battle.h` / `battle.cpp` — shared battle engine (wild and Gym battles, move resolution)
- `trainers.h` — the 5 regions' Gym leaders, Elite Four and Champion rosters (ported from
  [DylanPDao/TamaPoke](https://github.com/DylanPDao/TamaPoke))
- `badges.h` — the 40 badge icon sprites (8 per region × 5 regions)
- `learnsets_real.h` / `moves_real.h` / `types_real.h` — real move data (learnsets, power/type/
  status, type chart) for the Pokédex Moves page and battle move resolution
- `sdmon.h` / `sdmon.cpp` — TPK1 (animated) and TPK2 (PMD) sprites + thumbnails, and file reception over USB (PUT/LS)
- `rtcbat.h` / `rtcbat.cpp` — PCF85063 RTC + AXP2101 PMU (battery, brightness, PWR button)
- `dayphase.h` — hour / phase / visual night from the RTC epoch
- `imu.h` / `imu.cpp` — QMI8658 poll, shake edge, pedometer delta
- `audio.h` / `audio.cpp` — ES8311 + I2S, real species-cry playback from SD, Game-Boy-style
  tone synth for SFX (non-blocking task)
- `i18n.h` / `i18n.cpp` — the 6-language string tables
- `dex.h` — GENERATED (`gen_dex.py` + `tools/merge_dex_650_1025.py`): the 1025-species
  (`DEX_COUNT`) table and evolution rules, Gen 1–9
- `species.h` — GENERATED (`sprites.py`): fallback sprites, UI icons, colours
- `pin_config.h` — the board's official pins
- `tests/` — native (desktop) unit tests for `pet.cpp`/`battle.cpp` logic, see
  [Native logic tests](#native-logic-tests)
- `tools/` — pipeline: `dex_data.py` (Gen 1–5 data), `gen_dex_650_1025.py` (Gen 6–9 data
  from PokéAPI), `dex_stats.py`, `gen_dex.py`, `sprites.py` (workshop), `pack_pmd.py` /
  `pack_pmd_fallback.py` / `make_thumbs.py` (sprite packers), `pack_cries.py` /
  `send_cries.py` (species cries), `pack_bundle.py` (web bundle), `send_sd.py` (SD upload),
  `build_web.sh` (rebuilds the whole web installer), `touch_log.py`
- `tools/sdcard/mons/` — the generated sprite `.bin` files (animated, shiny, PMD, thumbnails)
- `tools/sdcard/cries/` — the generated species-cry `.pcm` files
- `web/` — the browser installer (ESP Web Tools + Web Serial sprite loader)

## Serial console (115200, debug)

`STATS` (full state) · `SPEC <dex>` (change species) · `LVL <n>` · `HATCH` ·
`SHINY` · `NICK <x>` · `BYE` / `RUN` (farewell / runaway) · `ABANDON` (force the
runaway-ready state) · `WIPE` (factory reset → new game) · `BEEP` (audio test) ·
`REG` (Pokédex) · `EGGS` (simulate 20 eggs) · `GAL` (gallery) · `CAREDAY` ·
`TIME <epoch>` / `RTCSET <epoch>` · `IMU` · `SHAKE` · `WALK <n>` · `STEPS` ·
`HEALTH` (uptime + heap for the soak test) · `LS` / `PUT` (SD files).

The local `TAMAPOKE_LOCAL_TEST` build additionally offers `CAUGHT`,
`CAUGHT <dex>`, `BATTLE`, `BATTLE <dex> [shiny]`, `TESTMON <dex> <shiny>` and
`TESTEVO <source> <target>`. The localhost page exposes these safe test and
diagnostic buttons without a terminal.

To test fast: lower `PET_TICK_MS`, `MINUTES_PER_LEVEL` and `FAREWELL_AGE_MIN` in `pet.h`.

## Native logic tests

The core pet rules in `pet.cpp` can be checked on a desktop without ESP32
hardware. The tests use small Arduino/Preferences stubs and cover hatching,
evolution gates, battle stat calculation, training rewards and lifecycle
readiness.

```bash
cd tests
make test
```

## Optional follow-up

- Longer 24–48 h soak test using the `HEALTH` command/heartbeat.
- Balance tuning after more real-world walking and long-term progression data.
- Rescale the collector-frame unlock thresholds and hard-mode Gym tuning for
  the full 1025-species dex (see the note under
  [Collection ranks and frames](#collection-ranks-and-frames)).
- `species_chirp.h`/`.cpp` (the old synthesized-chirp tone tables) are dead
  code now that real species cries play instead — safe to remove.

*(Done: 3D-printed case [published on MakerWorld](https://makerworld.com/es/models/2937822-tamapoke-a-pokemon-pokeball-tamagotchi); repo public with the browser installer + one-click sprite bundle; full Gen 1–9 dex (1025 species) with real cries; 5-region Gym league with Party system.)*

## Credits

All sprites: [PMD SpriteCollab](https://github.com/PMDCollab/SpriteCollab)
(community, CC BY-NC). Base stats, evolutions, move data, names & species
cries: [PokéAPI](https://pokeapi.co) / [PokéAPI/cries](https://github.com/PokeAPI/cries).
Gym/Elite-Four/Champion rosters and badge icon data ported via
[DylanPDao/TamaPoke](https://github.com/DylanPDao/TamaPoke) (MIT); original
badge artwork by [SteGriff/pokemon-badges](https://github.com/SteGriff/pokemon-badges)
(CC BY 3.0). Pokémon is a ™ of Nintendo / Game Freak / The Pokémon Company.
Non-commercial, personal-use project. Charging configuration issue spotted
through ShadowEnemyx's hardware test video: [TikTok](https://pro.tiktok.com/t/ZGdxJB3nr/).
Full list in [`CREDITS.md`](CREDITS.md).

> Earlier builds of this project used a synthesized "chirp" in place of a real
> cry, generated from TamaPoke's own tone-synthesis parameters. Sound
> playback now uses **real species cries** sourced from PokéAPI/cries instead
> (see [Sprites and cries](#sprites-and-cries-pmd-spritecollab--pokéapi)
> above) — the synthesis code remains in the repo as dormant, unused code.

## License

- **Source code** (firmware + tooling): **[MIT](LICENSE)**.
- **Sprites, names, move data & cries**: © Nintendo / Game Freak / The Pokémon
  Company; pixel art from [PMD SpriteCollab](https://github.com/PMDCollab/SpriteCollab)
  (CC BY-NC 4.0); stats/names/moves from [PokéAPI](https://pokeapi.co); species
  cries from [PokéAPI/cries](https://github.com/PokeAPI/cries), extracted from
  the official games. **Non-commercial use only.**
- **Gym rosters & badge data**: ported from [DylanPDao/TamaPoke](https://github.com/DylanPDao/TamaPoke)
  (MIT) and [SteGriff/pokemon-badges](https://github.com/SteGriff/pokemon-badges)
  (CC BY 3.0).
- **3D-printed case**: remix of *"Pokeball"* by **yoyothechicken**
  ([MakerWorld #839922](https://makerworld.com/es/models/839922-pokeball)),
  licensed **CC BY-NC-SA**, and shared here under the same terms.

This is an unofficial fan project, not affiliated with or endorsed by Nintendo.
