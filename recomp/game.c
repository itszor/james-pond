/* James Pond (Vectordean for Millennium, 1990; Archimedes conversion by
 * Krisalis - Richard Teather, 1991): what libroger needs to know about the
 * game.
 *
 * !run sets Pond$Dir, runs MemMod (TequeMemory: *TequeMem, which only frees
 * memory; nothing here calls it), RMLoads MusicModule 1.02 (MUSMOD2: a
 * tracker player that also plays the sound effects) and JoyMod (Keith
 * Martin's joystick module: libroger answers its Joy_Read0 from a game
 * controller instead), *MusSampleRate 3, then loads LOADER at &8000 and
 * runs it. LOADER shows the Krisalis logo, loads JamesCode at &AFFC,
 * unpacks it to &B000 (../tools/unpack.py does the same) and jumps there.
 * The Buzz re-release (1994) runs LOADER through a small module (PONDRUN:
 * *Go 8000, then MODE 0) and has an A3010 joystick module (JOYPATCH, the
 * same SWIs); its LOADER and JamesCode are the same.
 *
 * The translated images: the game (gen/, for &B000), LOADER (genload/) and
 * MusicModule (genmus/, for its RMA address &1800000). */
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <roger.h>

extern const struct roger_image recomp_image, recompP_image, recompM_image;   /* gen{,load,mus}/entries.c */

#define APP "../game/!JamesPond"
static const char *const vars[] = {"Pond$Dir", "!JamesPond", NULL};
static const char *const prefixes[] = {"<Obey$Dir>.", NULL};
/* JamesCode is crunched: LOADER loads it at &AFFC and unpacks it to &B000,
 * the game image's base, where it then jumps */
static const struct roger_program programs[] = {{"JamesCode", &recomp_image, 0, 0xAFFC}, {NULL, NULL, 0, 0}};
static struct roger_module modules[] = {{"MUSMOD2", &recompM_image, 0}, {NULL, NULL, 0}};

/* --level=N: a new game starts at level N. The game keeps the level (0-11)
 * at &B8FC (its workspace, R12 = &B454, +&4A8) and loads each level's map,
 * sprites and masks by it (&13CBC: OS_File 255 with names from tables, the
 * maps' at &119C0 with their offsets at &11990). A new game sets it to 0
 * just before loading Map0, so that load is turned into level N's. */
#define LEVEL       0xB8FC
#define MAP_NAMES   0x119C0
#define MAP_OFFSETS 0x11990
static int start_level;

static int arg(const char *a)
{
    if (!strncmp(a, "--level=", 8) && atoi(a + 8) >= 1 && atoi(a + 8) <= 12) start_level = atoi(a + 8);
    else return 0;
    return 1;
}

static int game_swi(uint32_t n, int x)
{
    if (start_level > 1 && n == 0x08 && R[0] == 255 && R[1] == MAP_NAMES && roger_current_image() == &recomp_image
        && RD32(LEVEL) == 0) {
        WR32(LEVEL, start_level - 1);
        R[1] = MAP_NAMES + RD32(MAP_OFFSETS + 4 * (start_level - 1));
    }
    return 0;
}

static void init(void)
{
    roger_cli("MusSampleRate 3");       /* as !run does */
}

/* a file in !JamesPond, whatever ,load-exec suffix the extraction gave it
 * (the discs' and the zip's differ) */
static const char *in_app(const char *leaf)
{
    DIR *d = opendir(APP);
    struct dirent *e;
    char *path = NULL;
    while (d && !path && (e = readdir(d)))
        if (strcspn(e->d_name, ",") == strlen(leaf) && !strncasecmp(e->d_name, leaf, strlen(leaf))) {
            path = malloc(strlen(APP) + strlen(e->d_name) + 2);
            sprintf(path, "%s/%s", APP, e->d_name);
        }
    if (d) closedir(d);
    if (!path) { fprintf(stderr, "no %s/%s: build.sh extracts it\n", APP, leaf); exit(1); }
    return path;
}

static struct roger_config config = {
    .title = "James Pond (recompiled)",
    /* the keys (the Read_Me): Z/X left/right, P up, L down, Space blows a
     * bubble (fires); Tab pauses */
    .pad_map = "left=Z,right=X,up=P,down=L,a=Space,b=P,start=Tab",
    .joystick = ROGER_JOY_KM,
    .image = &recompP_image,
    .programs = programs,
    .modules = modules,
    .appdir = "../game",
    .dir_prefixes = prefixes,
    .variables = vars,
    .command_line = "Go 8000",
    .initial_mode = 12,                 /* the desktop's: LOADER sets its own */
    .wimpslot = 480 * 1024,             /* !run's last *WimpSlot */
    /* The palette is VIDC's, written by a raw IRQ handler the game installs
     * at &100 (&B348, installed at &B310): at vsync it programs IOC timer 1
     * to fire part way down the frame, where it changes the palette (the
     * status panel's) and sets the flag its palette fades wait for (&B4A4) */
    .irq1v = 1,
    .init = init,
    .swi = game_swi,
    .arg = arg,
    .usage = "  --level=N         a new game starts at level N (1-12)\n",
};

int main(int argc, char **argv)
{
    config.image_path = in_app("LOADER");
    modules[0].path = in_app(modules[0].path);
    roger_main(argc, argv, &config);
}
