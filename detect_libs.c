/*
 * detect_libs.c - Auto-detect library deps from #include directives
 *
 * Passes through: scan sources → table lookup → pkg-config fallback →
 * deduped flags string. Built-in 50-entry table covers common libs.
 */

#include "detect_libs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Known-header mapping table */

struct LibEntry {
    const char *header;          /* e.g. "gmp.h" (exact match, no path) */
    const char *pkg_config;      /* pkg-config package name, or NULL */
    const char *raw_flags;       /* fallback flags if pkg-config unavailable */
};

/* Keep sorted alphabetically by header for easier maintenance */
static const struct LibEntry lib_table[] = {
    /* Header               pkg-config       raw flags */
    { "AL/al.h",           NULL,             "-lAL" },
    { "GL/gl.h",           NULL,             "-lGL" },
    { "GL/glew.h",         "glew",           "-lGLEW" },
    { "GL/glu.h",          NULL,             "-lGLU" },
    { "GL/glut.h",         "glut",           "-lglut" },
    { "GLFW/glfw3.h",      "glfw3",          "-lglfw" },
    { "SDL.h",             "sdl",            "-lSDL" },
    { "SDL2/SDL.h",        "sdl2",           "-lSDL2" },
    { "SDL2/SDL_image.h",  "SDL2_image",     "-lSDL2_image" },
    { "SDL2/SDL_mixer.h",  "SDL2_mixer",     "-lSDL2_mixer" },
    { "SDL2/SDL_ttf.h",    "SDL2_ttf",       "-lSDL2_ttf" },
    { "X11/Xlib.h",        "x11",            "-lX11" },
    { "X11/extensions/Xrandr.h", "xrandr",   "-lXrandr" },
    { "X11/extensions/XShm.h",    NULL,       "-lXext" },
    { "alsa/asoundlib.h",  "alsa",           "-lasound" },
    { "archive.h",         "libarchive",     "-larchive" },
    { "arpa/inet.h",       NULL,             "" },
    { "assimp.h",          "assimp",         "-lassimp" },
    { "boost/",            NULL,             "" },  /* Boost is header-only mostly */
    { "bzlib.h",           "bzip2",          "-lbz2" },
    { "cairo.h",           "cairo",          "-lcairo" },
    { "curl/curl.h",       "libcurl",        "-lcurl" },
    { "dbus/dbus.h",       "dbus-1",         "-ldbus-1" },
    { "dlfcn.h",           NULL,             "-ldl" },
    { "expat.h",           "expat",          "-lexpat" },
    { "fcntl.h",           NULL,             "" },
    { "fftw3.h",           "fftw3",          "-lfftw3" },
    { "fontconfig/fontconfig.h", "fontconfig", "-lfontconfig" },
    { "freetype2/ft2build.h", "freetype2",   "-lfreetype" },
    { "gdk-pixbuf/gdk-pixbuf.h", "gdk-pixbuf-2.0", "-lgdk_pixbuf-2.0" },
    { "gio/gio.h",         "gio-2.0",        "-lgio-2.0" },
    { "glib.h",            "glib-2.0",       "-lglib-2.0" },
    { "gmpxx.h",           "gmpxx",          "-lgmpxx -lgmp" },
    { "gmp.h",             "gmp",            "-lgmp" },
    { "gobject/gobject.h", "gobject-2.0",    "-lgobject-2.0" },
    { "gsl/gsl_blas.h",    "gsl",            "-lgsl -lgslcblas" },
    { "gsl/gsl_linalg.h",  "gsl",            "-lgsl -lgslcblas" },
    { "gsl/gsl_matrix.h",  "gsl",            "-lgsl -lgslcblas" },
    { "gsl/gsl_vector.h",  "gsl",            "-lgsl -lgslcblas" },
    { "gssapi/gssapi.h",   "krb5",           "-lgssapi_krb5" },
    { "gtk/gtk.h",         "gtk+-2.0",       "-lgtk-x11-2.0" },
    { "gtk/gtkx.h",        "gtk+-2.0",       "-lgtk-x11-2.0" },
    { "gtk-3.0/gtk.h",     "gtk+-3.0",       "-lgtk-3" },
    { "gtk-4.0/gtk.h",     "gtk4",           "-lgtk-4" },
    { "hdf5.h",            "hdf5",           "-lhdf5" },
    { "ibus.h",            "ibus-1.0",       "-libus-1.0" },
    { "jack/jack.h",       "jack",           "-ljack" },
    { "jansson.h",         "jansson",        "-ljansson" },
    { "jpeglib.h",         "libjpeg",        "-ljpeg" },
    { "lzma.h",            "liblzma",        "-llzma" },
    { "lzo/lzo1x.h",       "lzo2",           "-llzo2" },
    { "math.h",            NULL,             "-lm" },
    { "mxml.h",            "mxml",           "-lmxml" },
    { "ncurses.h",         "ncurses",        "-lncurses" },
    { "ncurses/curses.h",  "ncurses",        "-lncurses" },
    { "netdb.h",           NULL,             "" },
    { "netinet/in.h",      NULL,             "" },
    { "nlohmann/json.hpp", NULL,             "" },  /* header-only */
    { "ogg/ogg.h",         "ogg",            "-logg" },
    { "omp.h",             "openmp",         "-fopenmp" },
    { "openssl/evp.h",     "openssl",        "-lssl -lcrypto" },
    { "openssl/ssl.h",     "openssl",        "-lssl -lcrypto" },
    { "pcap.h",            "libpcap",        "-lpcap" },
    { "pcre.h",            "libpcre",        "-lpcre" },
    { "png.h",             "libpng",         "-lpng" },
    { "poll.h",            NULL,             "" },
    { "pulse/pulseaudio.h","libpulse",       "-lpulse" },
    { "pulse/simple.h",    "libpulse-simple","-lpulse-simple" },
    { "python3.12/Python.h", "python3-embed", "-lpython3.12" },
    { "python3.13/Python.h", "python3-embed", "-lpython3.13" },
    { "QtCore/QtCore",     "Qt5Core",        "-lQt5Core" },
    { "QtGui/QtGui",       "Qt5Gui",         "-lQt5Gui" },
    { "QtWidgets/QtWidgets", "Qt5Widgets",   "-lQt5Widgets" },
    { "Qt6/QtCore",        "Qt6Core",        "-lQt6Core" },
    { "Qt6/QtGui",         "Qt6Gui",         "-lQt6Gui" },
    { "Qt6/QtWidgets",     "Qt6Widgets",     "-lQt6Widgets" },
    { "readline/readline.h","readline",      "-lreadline" },
    { "rpc/xdr.h",         NULL,             "" },
    { "rtmidi/RtMidi.h",   "rtmidi",         "-lrtmidi" },
    { "sndfile.h",         "sndfile",        "-lsndfile" },
    { "spdlog/spdlog.h",   "spdlog",         "-lspdlog" },  /* fmt is bundled */
    { "sqlite3.h",         "sqlite3",        "-lsqlite3" },
    { "ssh/libssh.h",      "libssh",         "-lssh" },
    { "sys/epoll.h",       NULL,             "" },
    { "sys/inotify.h",     NULL,             "" },
    { "sys/mman.h",        NULL,             "" },
    { "sys/socket.h",      NULL,             "" },
    { "sys/stat.h",        NULL,             "" },
    { "sys/sysinfo.h",     NULL,             "" },
    { "sys/time.h",        NULL,             "" },
    { "sys/types.h",       NULL,             "" },
    { "taglib/taglib.h",   "taglib",         "-ltag" },
    { "tcl.h",             "tcl",            "-ltcl" },
    { "termios.h",         NULL,             "" },
    { "tesseract/baseapi.h","tesseract",     "-ltesseract" },
    { "theora/theora.h",   "theora",         "-ltheora" },
    { "unistd.h",          NULL,             "" },
    { "uuid/uuid.h",       "uuid",           "-luuid" },
    { "vorbis/codec.h",    "vorbis",         "-lvorbis -lvorbisenc" },
    { "vulkan/vulkan.h",   "vulkan",         "-lvulkan" },
    { "wayland-client.h",  "wayland-client", "-lwayland-client" },
    { "Xlib.h",            NULL,             "-lX11" },
    { "X11/Xatom.h",       NULL,             "-lX11" },
    { "X11/Xutil.h",       NULL,             "-lX11" },
    { "X11/keysym.h",      NULL,             "-lX11" },
    { "xmlrpc.h",          "xmlrpc",         "-lxmlrpc" },
    { "xmlrpc_client.h",   "xmlrpc_client",  "-lxmlrpc_client" },
    { "yaml.h",            "yaml-0.1",       "-lyaml" },
    { "zconf.h",           "zlib",           "-lz" },
    { "zlib.h",            "zlib",           "-lz" },
    { "zstd.h",            "libzstd",        "-lzstd" },
};

static const int lib_table_count = sizeof(lib_table) / sizeof(lib_table[0]);

/* Caching buffer — results survive for get_detected_libs() */

static char g_cached_flags[4096] = "";
static bool g_initialized = false;

/* Internal helpers */

/* True if header is in the C/C++ standard library (compiler provides it) */
static bool is_standard_header(const char *header)
{
    /* C standard library headers (ISO C) */
    static const char *c_std[] = {
        "assert.h", "complex.h", "ctype.h", "errno.h", "fenv.h",
        "float.h", "inttypes.h", "iso646.h", "limits.h", "locale.h",
        "setjmp.h", "signal.h", "stdalign.h", "stdarg.h", "stdatomic.h",
        "stdbit.h", "stdbool.h", "stdckdint.h", "stddef.h", "stdint.h",
        "stdio.h", "stdlib.h", "stdnoreturn.h", "string.h", "tgmath.h",
        "threads.h", "time.h", "uchar.h", "wchar.h", "wctype.h",
        NULL
    };
    /* POSIX / common Unix headers (no -l needed) */
    static const char *posix[] = {
        "dirent.h", "fnmatch.h", "glob.h", "grp.h", "pwd.h",
        "regex.h", "sched.h", "semaphore.h", "spawn.h",
        "syslog.h", "tar.h", "termio.h", "time.h",
        "utime.h", "utmpx.h", "wordexp.h",
        NULL
    };
    /* C++ standard headers (no -l needed) */
    static const char *cpp_std[] = {
        "algorithm", "array", "atomic", "bitset", "chrono",
        "codecvt", "compare", "concepts", "condition_variable",
        "deque", "exception", "expected", "filesystem", "format",
        "forward_list", "fstream", "functional", "future",
        "initializer_list", "iomanip", "ios", "iosfwd", "iostream",
        "istream", "iterator", "latch", "limits", "list", "locale",
        "map", "memory", "memory_resource", "mutex", "new",
        "numbers", "numeric", "optional", "ostream", "print",
        "queue", "random", "ranges", "ratio", "regex",
        "scoped_allocator", "set", "shared_mutex", "source_location",
        "span", "spanstream", "sstream", "stack", "stacktrace",
        "stdexcept", "stdfloat", "stop_token", "streambuf",
        "string", "string_view", "strstream", "syncstream",
        "system_error", "thread", "tuple", "type_traits",
        "typeindex", "typeinfo", "unordered_map", "unordered_set",
        "utility", "valarray", "variant", "vector", "version",
        "barrier", "semaphore",
        /* C++ wrappers of C headers */
        "cassert", "ccomplex", "cctype", "cerrno", "cfenv",
        "cfloat", "cinttypes", "ciso646", "climits", "clocale",
        "cmath", "csetjmp", "csignal", "cstdalign", "cstdarg",
        "cstdbool", "cstddef", "cstdint", "cstdio", "cstdlib",
        "cstring", "ctgmath", "ctime", "cuchar", "cwchar", "cwctype",
        NULL
    };

    /* Check across all standard lists */
    const char **lists[] = {c_std, posix, cpp_std};
    for (int l = 0; l < 3; l++) {
        for (int i = 0; lists[l][i]; i++) {
            if (strcmp(header, lists[l][i]) == 0)
                return true;
        }
    }
    return false;
}

/* Run pkg-config for a package; return 0 on success, -1 if not found */
static int resolve_pkg_config(const char *pkg_name, char *output, size_t size)
{
    if (!pkg_name || !pkg_name[0])
        return -1;

    char cmd[4096];
    FILE *pipe;

    snprintf(cmd, sizeof(cmd),
             "pkg-config --cflags --libs '%s' 2>/dev/null", pkg_name);
    pipe = popen(cmd, "r");
    if (!pipe) return -1;

    size_t n = fread(output, 1, size - 1, pipe);
    output[n] = '\0';
    int ret = pclose(pipe);

    if (ret != 0 || n == 0)
        return -1;

    /* Trim trailing newline/whitespace */
    while (n > 0 && (output[n - 1] == '\n' || output[n - 1] == ' '))
        output[--n] = '\0';

    return 0;
}

/* Parse a source file and collect all #include <...> header names */
static int extract_headers(FILE *fp, const char *headers[], int max_headers)
{
    static char line_buf[65536];
    int count = 0;

    while (fgets(line_buf, sizeof(line_buf), fp) && count < max_headers) {
        char *p = line_buf;

        /* Skip whitespace */
        while (*p == ' ' || *p == '\t') p++;

        /* Must be #include */
        if (p[0] != '#' || strncmp(p, "#include", 8) != 0)
            continue;

        /* Skip whitespace after #include */
        p += 8;
        while (*p == ' ' || *p == '\t') p++;

        /* Must be <...> — skip quoted includes; those are project-local */
        if (*p != '<') continue;

        /* Extract header name between < and > */
        p++;  /* skip '<' */
        const char *start = p;
        while (*p && *p != '>') p++;
        if (*p != '>') continue;

        /* Calculate length and copy into static storage */
        size_t len = (size_t)(p - start);
        if (len == 0 || len > 512) continue;

        /* Check it's not a comment-like false positive */
        if (len >= 2 && start[0] == '/' && (start[1] == '/' || start[1] == '*'))
            continue;

        /* Store as a pointer into line_buf — caller must use before next call.
         * Since we extract all headers in a single pass, this is safe. */
        static char header_pool[64][512];
        static int pool_idx = 0;

        if (count >= 64) break;
        memcpy(header_pool[pool_idx % 64], start, len);
        header_pool[pool_idx % 64][len] = '\0';
        headers[count] = header_pool[pool_idx % 64];
        pool_idx++;
        count++;
    }

    return count;
}

/* Dedup: check if a flag is already in the accumulator */
static bool flag_already_present(const char *accum, const char *flag)
{
    if (!accum || !flag) return false;
    return strstr(accum, flag) != NULL;
}

/* Look up one header in the table and append its flags to the accumulator */
static void append_flags_for_header(const char *header,
                                     char *accum, size_t acc_size)
{
    /* Reject empty or too-short headers */
    if (!header || strlen(header) < 3)
        return;

    /* Look up in the header table */
    for (int i = 0; i < lib_table_count; i++) {
        /* Match against the full header path from <...> */
        if (strcmp(header, lib_table[i].header) != 0)
            continue;

        const char *flags = NULL;
        char pkg_buf[4096] = "";

        /* Prefer pkg-config over raw flags */
        if (lib_table[i].pkg_config &&
            resolve_pkg_config(lib_table[i].pkg_config, pkg_buf, sizeof(pkg_buf)) == 0) {
            flags = pkg_buf;
        } else {
            /* Fall back to raw flags; skip empty entries */
            if (lib_table[i].raw_flags[0] == '\0')
                return;
            flags = lib_table[i].raw_flags;
        }

        /* Add flags, deduping */
        char tmp[4096];
        snprintf(tmp, sizeof(tmp), " %s", flags);

        char *flag = strtok(tmp, " \t");
        while (flag) {
            if (!flag_already_present(accum, flag)) {
                size_t cur = strlen(accum);
                if (cur + strlen(flag) + 2 < acc_size) {
                    if (cur > 0) strncat(accum, " ", acc_size - cur - 1);
                    strncat(accum, flag, acc_size - strlen(accum) - 1);
                }
            }
            flag = strtok(NULL, " \t");
        }
        return;  /* Found a match */
    }

    /* Not in the table — try pkg-config on the bare header name as a guess */
    {
        /* Strip dir prefix and .h to make a pkg-config name */
        const char *base = strrchr(header, '/');
        if (base) base++; else base = header;

        char pkg_name[256];
        size_t blen = strlen(base);
        if (blen > 2 && base[blen - 2] == '.' &&
            (base[blen - 1] == 'h' || base[blen - 1] == 'H')) {
            memcpy(pkg_name, base, blen - 2);
            pkg_name[blen - 2] = '\0';
        } else {
            memcpy(pkg_name, base, blen);
            pkg_name[blen] = '\0';
        }

        char pkg_buf[4096] = "";
        if (resolve_pkg_config(pkg_name, pkg_buf, sizeof(pkg_buf)) == 0) {
            char tmp[4096];
            size_t cur = strlen(accum);
            snprintf(tmp, sizeof(tmp), " %s", pkg_buf);
            char *flag = strtok(tmp, " \t");
            while (flag) {
                if (!flag_already_present(accum, flag)) {
                    if (cur > 0) strncat(accum, " ", acc_size - cur - 1);
                    strncat(accum, flag, acc_size - strlen(accum) - 1);
                }
                flag = strtok(NULL, " \t");
            }
        }
    }
}

/* Public API */

int detect_libraries(const char **sources, int source_count,
                     char *output, size_t output_size)
{
    char accum[4096] = "";
    bool found_any = false;

    if (!output || output_size == 0)
        return -1;

    for (int f = 0; f < source_count; f++) {
        FILE *fp = fopen(sources[f], "r");
        if (!fp) continue;

        const char *headers[64];
        int nh = extract_headers(fp, headers, 64);
        fclose(fp);

        for (int h = 0; h < nh; h++) {
            /* Skip standard library headers (stdio.h, etc.) */
            if (is_standard_header(headers[h]))
                continue;
            /* Skip filesystem-level stuff like <linux/...>, <asm/...>, etc. */
            if (strncmp(headers[h], "linux/", 6) == 0 ||
                strncmp(headers[h], "asm/", 4) == 0 ||
                strncmp(headers[h], "bits/", 5) == 0 ||
                strncmp(headers[h], "sys/platform", 12) == 0)
                continue;

            append_flags_for_header(headers[h], accum, sizeof(accum));
        }
    }

    /* Trim leading space (if any) */
    const char *src = accum;
    while (*src == ' ') src++;
    strncpy(output, src, output_size - 1);
    output[output_size - 1] = '\0';

    /* Cache it so get_detected_libs() works */
    strncpy(g_cached_flags, output, sizeof(g_cached_flags) - 1);
    g_cached_flags[sizeof(g_cached_flags) - 1] = '\0';
    g_initialized = true;

    found_any = (output[0] != '\0');
    return found_any ? 0 : 1;
}

const char *get_detected_libs(void)
{
    if (!g_initialized)
        return "";
    return g_cached_flags;
}

int get_suggested_packages(const char **sources, int source_count,
                           char *output, size_t output_size)
{
    char all_headers[4096] = "";
    int missing_count = 0;

    if (!output || output_size == 0)
        return 0;

    for (int f = 0; f < source_count; f++) {
        FILE *fp = fopen(sources[f], "r");
        if (!fp) continue;

        const char *headers[64];
        int nh = extract_headers(fp, headers, 64);
        fclose(fp);

        for (int h = 0; h < nh; h++) {
            if (is_standard_header(headers[h]))
                continue;

            /* Look up in table for pkg-config name */
            const char *pkg = NULL;
            for (int i = 0; i < lib_table_count; i++) {
                if (strcmp(headers[h], lib_table[i].header) == 0) {
                    if (lib_table[i].pkg_config) {
                        /* Check if actually installed */
                        char cmd[512];
                        snprintf(cmd, sizeof(cmd),
                                 "pkg-config --exists '%s' 2>/dev/null",
                                 lib_table[i].pkg_config);
                        if (system(cmd) != 0) {
                            pkg = lib_table[i].pkg_config;
                        }
                    } else if (lib_table[i].raw_flags[0]) {
                        /* No pkg-config name, check if the flag exists in /usr/lib */
                        char flag_path[512];
                        char flag_copy[128];
                        snprintf(flag_copy, sizeof(flag_copy), "%s",
                                 lib_table[i].raw_flags);
                        char *flag = strtok(flag_copy, " \t");
                        bool found_lib = false;
                        while (flag) {
                            if (flag[0] == '-' && flag[1] == 'l') {
                                /* Check both lib64 and lib paths */
                                snprintf(flag_path, sizeof(flag_path),
                                         "/usr/lib64/lib%s.so",
                                         flag + 2);
                                if (access(flag_path, F_OK) == 0) {
                                    found_lib = true;
                                    break;
                                }
                                snprintf(flag_path, sizeof(flag_path),
                                         "/usr/lib/lib%s.so",
                                         flag + 2);
                                if (access(flag_path, F_OK) == 0) {
                                    found_lib = true;
                                    break;
                                }
                            }
                            flag = strtok(NULL, " \t");
                        }
                        if (!found_lib) {
                            /* Can't verify, but add a note */
                            if (strlen(all_headers) + 128 < sizeof(all_headers)) {
                                strncat(all_headers, " ", sizeof(all_headers) - strlen(all_headers) - 1);
                                strncat(all_headers, headers[h], sizeof(all_headers) - strlen(all_headers) - 1);
                            }
                            missing_count++;
                        }
                    }
                    break;
                }
            }

            if (pkg) {
                size_t cur = strlen(all_headers);
                size_t pkg_len = strlen(pkg);
                if (cur + pkg_len + 64 < sizeof(all_headers)) {
                    if (cur > 0)
                        strncat(all_headers, ", ", sizeof(all_headers) - cur - 1);
                    strncat(all_headers, pkg, sizeof(all_headers) - strlen(all_headers) - 1);
                }
                missing_count++;
            }
        }
    }

    if (missing_count > 0) {
        int written = snprintf(output, output_size,
                 "⚠  Detected missing libraries. Try installing:\n"
                 "     sudo dnf install%s\n"
                 "   Or on Debian/Ubuntu:\n"
                 "     sudo apt install%s\n",
                 all_headers, all_headers);
        (void)written;
    }

    return missing_count;
}
