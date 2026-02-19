/* ── Filesystem paths ─────────────────────────────────────────────────── */
#define FAS_DOLPHIN_PATH   "/ext/dolphin"
#define FAS_PLAYLISTS_PATH "/ext/apps_data/flipper_animation_switcher"
#define FAS_MANIFEST_PATH  "/ext/dolphin/manifest.txt"

/* ── Limits ───────────────────────────────────────────────────────────── */
#define FAS_MAX_ANIMATIONS    128 // TODO Maybe increase this if possible?
#define FAS_ANIM_NAME_LEN      64
#define FAS_MAX_PLAYLISTS      64
#define FAS_PLAYLIST_NAME_LEN  32
#define FAS_PATH_LEN          160

/* ── Default animation values ─────────────────────────────────────────── */
#define FAS_DEFAULT_MIN_BUTTHURT  0
#define FAS_DEFAULT_MAX_BUTTHURT 14
#define FAS_DEFAULT_MIN_LEVEL     1
#define FAS_DEFAULT_MAX_LEVEL    30
#define FAS_DEFAULT_WEIGHT        3

/* ── Data structures ──────────────────────────────────────────────────── */
typedef struct {
    char name[FAS_ANIM_NAME_LEN];
    bool selected;
    int  min_butthurt;
    int  max_butthurt;
    int  min_level;
    int  max_level;
    int  weight;
} AnimEntry;

typedef struct {
    char name[FAS_PLAYLIST_NAME_LEN];
} PlaylistEntry;