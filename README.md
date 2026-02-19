# Flipper Animation Switcher

A Flipper Zero application that lets you create, manage, and switch between
**animation playlists** — template `manifest.txt` files that control which
background animations play on your Flipper.

## Features

| Feature | Description |
|---|---||
| **Create Playlist** | Select any subset of animations on your Flipper, optionally fine-tune per-animation settings, then save as a named playlist. |
| **Choose Playlist** | Apply a playlist (overwrites `manifest.txt`). A reboot prompt appears to activate the changes. |
| **Delete Playlist** | Remove any saved playlists. |
| **Preview** | Long-press any playlist to see which animations it contains. |
| **About / Help** | Version info and repo link. |

## Controls

| Screen | Button | Action |
|---|---|---||
| Create Playlist | ↑ / ↓ | Navigate items |
| Create Playlist | OK (short) | Toggle checkbox |
| Create Playlist | OK (long) | Edit min/max butthurt, level & weight |
| Create Playlist | → | Proceed to name-entry |
| Create Playlist (after long press) | ← / → | Adjust value |
| Choose Playlist | OK (short) | Select / confirm |
| Choose Playlist | OK (long) | Preview animations in that playlist |

## Default Animation Values

When an animation is added without editing its per-animation settings, these defaults are applied:

| Setting | Default |
|---|---||
| Min Butthurt | 0 |
| Max Butthurt | 14 |
| Min Level | 1 |
| Max Level | 30 |
| Weight | 3 |

## File Locations

| Path | Contents |
|---|---||
| `/ext/dolphin/` | Animation folders (read-only by this app) |
| `/ext/dolphin/manifest.txt` | Active animation manifest (overwritten on Apply) |
| `/ext/apps_data/flipper_animation_switcher/` | Saved playlist `.txt` files |
