# Credits

TamaPoke is a **non-commercial, personal-use** project. It does not sell or
commercially redistribute any copyrighted material. Pokémon and all related
names, designs and characters are trademarks and © of **Nintendo / Game Freak /
The Pokémon Company**.

This project is not affiliated with or endorsed by any of those companies.

## Sprites and data

| Resource | Source | Use in the project |
|---|---|---|
| **All sprites** (idle, walk, sleep, eat, hurt, attack…) | [PMD Sprite Collaboration (PMDCollab/SpriteCollab)](https://github.com/PMDCollab/SpriteCollab) | Mystery-Dungeon-style animated sprites used everywhere: main screen, stat card, minigame, and the Pokédex grid + detail view. ~54 of the 1025 species (mostly Gen 8/9, where SpriteCollab has no animation yet) fall back to a static frame taken from PokéAPI's official pixel sprite instead (`tools/pack_pmd_fallback.py`) |
| **Base stats, types, evolutions, names & descriptions for all 1025 species** | [PokéAPI](https://pokeapi.co) | Real HP/ATK/DEF/SpA/SpD/SPD, type chart, evolution chains and localized names/flavor text for every species (`tools/dex_stats.py` for Gen 1, `tools/gen_dex_650_1025.py` for Gen 6–9) |
| **Species cries** (all 1025) | [PokéAPI/cries](https://github.com/PokeAPI/cries) — `legacy/` set for Gen 1–5, `latest/` set for Gen 6–9, ultimately extracted from the official Pokémon games | Played back on pet taps, known Pokédex details and wild-battle starts (`tools/pack_cries.py`, `audio.cpp: playSpeciesCryFromSD()`) |
| **Move data** (learnsets, power, type, status) | [PokéAPI](https://pokeapi.co) | Pokédex Moves page and battle move resolution (`learnsets_real.h`, `moves_real.h`, `types_real.h`) |
| **Gym badge icons** (Kanto, Johto, Hoenn, Sinnoh, Unova) | [SteGriff/pokemon-badges](https://github.com/SteGriff/pokemon-badges) (Stephen Griffiths, 2011, CC BY 3.0, traced from Bulbapedia), pixel data ported via [DylanPDao/TamaPoke](https://github.com/DylanPDao/TamaPoke) (MIT) | Badge icons on the ARENEN/ORDEN gym-battle pages (`badges.h`) |
| **Gym/Elite-Four/Champion rosters** (Kanto, Johto, Hoenn, Sinnoh, Unova) | [DylanPDao/TamaPoke](https://github.com/DylanPDao/TamaPoke) (MIT), hand-authored there from the original/remake games (Unova not verified against a disassembly, see `trainers.h`) | Real trainer teams and levels for the gym battle system (`trainers.h`) |

The **SpriteCollab** sprites are the work of its community of artists under their
own terms (Creative Commons Attribution-NonCommercial 4.0). Per-species/per-author
credit is in the original repository's
[tracker.json](https://github.com/PMDCollab/SpriteCollab/blob/master/tracker.json).
Huge thanks to that whole community for an enormous amount of work.

> **Important if you reuse this repo:** the packaged sprite files
> (`tools/sdcard/mons/*.bin`) and species-cry files (`tools/sdcard/cries/*.pcm`)
> are derived from the sources above — the cries in particular are extracted
> from the official games via PokéAPI. Don't redistribute them commercially.
> If you publish the project, the clean approach is to distribute **only the
> code and scripts**, and have each user download and package the sprites/cries
> from the original sources with `tools/pack_*.py` (or the web installer).

## Software / hardware

| Component | Author / source |
|---|---|
| GFX Library for Arduino | [moononournation](https://github.com/moononournation/Arduino_GFX) |
| SensorLib (CST9217 touch, PCF85063 RTC) | [Lewis He / lewisxhe](https://github.com/lewisxhe/SensorLib) |
| XPowersLib (AXP2101 PMU) | [Lewis He / lewisxhe](https://github.com/lewisxhe/XPowersLib) |
| Board and pinout | [Waveshare ESP32-S3-Touch-AMOLED-1.75](https://www.waveshare.com/wiki/ESP32-S3-Touch-AMOLED-1.75) |
| Web installer | [ESP Web Tools](https://esphome.github.io/esp-web-tools/) (Nabu Casa) |

## Testing notes

The AXP2101 charging configuration issue was spotted through ShadowEnemyx's
hardware test video: [TikTok](https://pro.tiktok.com/t/ZGdxJB3nr/).

TamaPoke's own code (firmware and tools) is original work.
