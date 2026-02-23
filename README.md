# Flipper Animation Switcher

A Flipper Zero application for creating, managing, and switching **animation playlists** - template `manifest.txt` files that control which background animations play on your Flipper. Supports up to 128 animations per playlist.

---

## Menus

| Menu | Description |
|---|---|
| **Create Playlist** | Select animations, optionally fine-tune per-animation settings, then save as a named playlist. |
| **Choose Playlist** | Apply a saved playlist (overwrites `manifest.txt`). |
| **Delete Playlist** | Remove any saved playlist. |
| **About / Help** | App info and help section. |

---

## Controls

### Main Menu

| Button | Action |
|---|---|
| ↑ / ↓ | Navigate options |
| OK | Select option |
| ← (Back) | Exit app |

### Create Playlist — Animation List

| Button | Action |
|---|---|
| ↑ / ↓ | Navigate animations |
| OK (short) | Toggle checkbox |
| OK (long) | Edit per-animation settings |
| → | Proceed to name entry (requires ≥ 1 selected) |

### Create Playlist — Animation Settings

| Button | Action |
|---|---|
| ↑ / ↓ | Navigate settings |
| ← / → | Adjust value |
| ← (Back) | Return to animation list |

### Choose / Delete Playlist

| Button | Action |
|---|---|
| ↑ / ↓ | Navigate playlists |
| OK (short) | Apply playlist / Confirm delete |
| OK (long) | Preview playlist animations |
| ← (Back) | Return to main menu |

---

## Default Animation Values

When an animation is added without customising its settings, these defaults are used:

| Setting | Default |
|---|---|
| Min Butthurt | 0 |
| Max Butthurt | 14 |
| Min Level | 1 |
| Max Level | 30 |
| Weight | 3 |

---

## File Locations

| Path | Contents |
|---|---|
| `/ext/dolphin/` | Animation folders (read-only by this app) |
| `/ext/dolphin/manifest.txt` | Active animation manifest (overwritten on Apply) |
| `/ext/apps_data/animation_switcher/` | Saved playlist `.txt` files |

---

## Building

This app targets the **official Flipper Zero firmware**. Most custom firmware builds should also be supported.

Install `ufbt` - instructions on its official [GitHub page](https://github.com/flipperdevices/flipperzero-ufbt).

Then run this command from the root of the repo:

```bash
# Builds and uploads the app to the Flipper
ufbt launch
```

---

## Roadmap

- Manual `manifest.txt` import as a playlist
- Automatic backup of the previous `manifest.txt` before applying a new playlist
- Animation previewer screen
- Increase maximum animation count beyond 128

---

## License

MIT © 2026 SLK
