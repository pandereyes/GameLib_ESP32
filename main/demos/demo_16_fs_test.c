/* demo_16_fs_test.c — File System API Verification
   Learn: gamelib_file_open, read, write, seek, tell, size, close */

#include "gamelib.h"
#include <string.h>
#include <stdio.h>

#define TEST_FILE "/test_fs.dat"

typedef struct {
    const char *name;
    bool        pass;
} test_result_t;

#define TEST_RESULT_CAPACITY 32

static test_result_t g_results[TEST_RESULT_CAPACITY];
static int           g_test_count = 0;
static int           g_pass_count = 0;

static void add_result(const char *name, bool pass)
{
    if (g_test_count >= TEST_RESULT_CAPACITY) {
        return;
    }

    g_results[g_test_count].name = name;
    g_results[g_test_count].pass = pass;
    g_test_count++;
    if (pass) g_pass_count++;
}

static void draw_results(gamelib_t *g)
{
    int y = 10;
    gamelib_draw_text(g, 5, y, "=== FS API Test ===", COLOR_YELLOW); y += 20;

    for (int i = 0; i < g_test_count; i++) {
        gamelib_draw_text(g, 10, y, g_results[i].name, COLOR_WHITE);
        if (g_results[i].pass) {
            gamelib_draw_text(g, 170, y, "PASS", COLOR_GREEN);
        } else {
            gamelib_draw_text(g, 170, y, "FAIL", COLOR_RED);
        }
        y += 16;
    }

    y += 10;
    gamelib_draw_printf(g, 5, y, COLOR_CYAN, "%d/%d passed", g_pass_count, g_test_count);
    y += 20;

    if (g_pass_count == g_test_count) {
        gamelib_draw_text(g, 5, y, "ALL TESTS PASSED!", COLOR_GREEN);
    } else {
        gamelib_draw_text(g, 5, y, "SOME TESTS FAILED", COLOR_RED);
    }

    gamelib_draw_text(g, 5, 300, "Press B to exit", COLOR_LIGHTGRAY);
}

static void test_write_read(void)
{
    void *fp;
    uint8_t buf[128];
    uint8_t verify[128];
    const char *test_str = "Hello FatFS on ESP32-S3!";
    int len = (int)strlen(test_str) + 1;

    printf("\n--- File Write/Read Test ---\n");

    /* 1. write */
    fp = gamelib_file_open(NULL, TEST_FILE, "wb");
    if (!fp) {
        printf("  FAIL: open wb\n");
        add_result("open(wb)", false);
        return;
    }
    add_result("open(wb)", true);
    printf("  PASS: open(wb)\n");

    int written = gamelib_file_write(NULL, fp, (const uint8_t *)test_str, len);
    if (written != len) {
        printf("  FAIL: write %d vs %d\n", written, len);
        add_result("write", false);
        gamelib_file_close(NULL, fp);
        return;
    }
    add_result("write", true);
    printf("  PASS: write %d bytes\n", written);

    gamelib_file_close(NULL, fp);
    add_result("close(w)", true);
    printf("  PASS: close after write\n");

    /* 2. read back */
    fp = gamelib_file_open(NULL, TEST_FILE, "rb");
    if (!fp) {
        printf("  FAIL: open rb\n");
        add_result("open(rb)", false);
        return;
    }
    add_result("open(rb)", true);
    printf("  PASS: open(rb)\n");

    int file_size = gamelib_file_size(NULL, fp);
    if (file_size != len) {
        printf("  size mismatch: %d vs %d\n", file_size, len);
    }
    add_result("size", file_size == len);
    printf("  %s: size=%d\n", file_size == len ? "PASS" : "FAIL", file_size);

    int read_bytes = gamelib_file_read(NULL, fp, buf, sizeof(buf));
    if (read_bytes != len) {
        printf("  FAIL: read %d vs %d\n", read_bytes, len);
        add_result("read", false);
        gamelib_file_close(NULL, fp);
        return;
    }
    add_result("read", true);
    printf("  PASS: read %d bytes\n", read_bytes);

    bool match = (memcmp(buf, test_str, len) == 0);
    add_result("data verify", match);
    printf("  %s: data match\n", match ? "PASS" : "FAIL");

    gamelib_file_close(NULL, fp);
    add_result("close(r)", true);
    printf("  PASS: close after read\n");
}

static void test_seek_tell(void)
{
    void *fp;
    uint8_t buf[32];
    int ret;

    printf("\n--- File Seek/Tell Test ---\n");

    fp = gamelib_file_open(NULL, TEST_FILE, "rb");
    if (!fp) { add_result("seek: open", false); return; }

    /* tell at start */
    int pos = gamelib_file_tell(NULL, fp);
    add_result("tell(0)", pos == 0);
    printf("  %s: tell at start = %d\n", pos == 0 ? "PASS" : "FAIL", pos);

    /* seek to pos 6 */
    ret = gamelib_file_seek(NULL, fp, 6, 0);
    add_result("seek(SET,6)", ret == 0);
    printf("  %s: seek to 6\n", ret == 0 ? "PASS" : "FAIL");

    pos = gamelib_file_tell(NULL, fp);
    add_result("tell(6)", pos == 6);
    printf("  %s: tell = %d\n", pos == 6 ? "PASS" : "FAIL", pos);

    /* read the substring starting at offset 6 ("FatFS...") */
    int n = gamelib_file_read(NULL, fp, buf, 5);
    buf[n] = '\0';
    bool sub_match = (n == 5 && memcmp(buf, "FatFS", 5) == 0);
    add_result("seek read", sub_match);
    printf("  %s: read at pos 6 = \"%.5s\"\n", sub_match ? "PASS" : "FAIL", buf);

    /* seek from current (+5 → skip " on E") */
    ret = gamelib_file_seek(NULL, fp, 4, 1);
    add_result("seek(CUR,+4)", ret == 0);
    printf("  %s: seek cur +4\n", ret == 0 ? "PASS" : "FAIL");

    /* seek from end */
    ret = gamelib_file_seek(NULL, fp, 0, 2);
    add_result("seek(END,0)", ret == 0);
    printf("  %s: seek to end\n", ret == 0 ? "PASS" : "FAIL");

    pos = gamelib_file_tell(NULL, fp);
    int expected_end = gamelib_file_size(NULL, fp);
    add_result("tell(end)", pos == expected_end);
    printf("  %s: tell at end = %d (size=%d)\n", pos == expected_end ? "PASS" : "FAIL", pos, expected_end);

    gamelib_file_close(NULL, fp);
}

static void test_append(void)
{
    void *fp;
    uint8_t buf[64];
    int size_before;
    int ret;

    printf("\n--- File Append Test ---\n");

    /* open existing file, get original size */
    fp = gamelib_file_open(NULL, TEST_FILE, "rb");
    if (!fp) { add_result("append: pre-size", false); return; }
    size_before = gamelib_file_size(NULL, fp);
    gamelib_file_close(NULL, fp);

    /* open in append mode and write */
    fp = gamelib_file_open(NULL, TEST_FILE, "ab");
    if (!fp) {
        add_result("open(ab)", false);
        printf("  FAIL: open ab\n");
        return;
    }
    add_result("open(ab)", true);
    printf("  PASS: open(ab)\n");

    /* verify position is at end */
    int pos = gamelib_file_tell(NULL, fp);
    add_result("ab tell == end", pos == size_before);
    printf("  %s: append pos=%d, orig_size=%d\n", pos == size_before ? "PASS" : "FAIL", pos, size_before);

    const char *extra = " [APPENDED]";
    int extra_len = (int)strlen(extra) + 1;
    ret = gamelib_file_write(NULL, fp, (const uint8_t *)extra, extra_len);
    add_result("append write", ret == extra_len);
    printf("  %s: appended %d bytes\n", ret == extra_len ? "PASS" : "FAIL", ret);

    int size_after = gamelib_file_size(NULL, fp);
    add_result("size grows", size_after == size_before + extra_len);
    printf("  %s: size %d → %d\n", size_after == size_before + extra_len ? "PASS" : "FAIL", size_before, size_after);

    gamelib_file_close(NULL, fp);
}

static void test_error_cases(void)
{
    void *fp;

    printf("\n--- Error Cases Test ---\n");

    /* open non-existent file for reading */
    fp = gamelib_file_open(NULL, "/nonexist.xyz", "rb");
    add_result("open missing rb → NULL", fp == NULL);
    printf("  %s: open nonexistent rb returns NULL\n", fp == NULL ? "PASS" : "FAIL");

    /* invalid mode */
    fp = gamelib_file_open(NULL, TEST_FILE, "xx");
    add_result("open bad mode → NULL", fp == NULL);
    printf("  %s: open bad mode returns NULL\n", fp == NULL ? "PASS" : "FAIL");

    /* close NULL */
    int ret = gamelib_file_close(NULL, NULL);
    add_result("close(NULL) → -1", ret == -1);
    printf("  %s: close(NULL) = %d\n", ret == -1 ? "PASS" : "FAIL", ret);

    /* read NULL */
    ret = gamelib_file_read(NULL, NULL, NULL, 10);
    add_result("read(NULL) → -1", ret == -1);
    printf("  %s: read(NULL) = %d\n", ret == -1 ? "PASS" : "FAIL", ret);

    /* write NULL */
    ret = gamelib_file_write(NULL, NULL, NULL, 10);
    add_result("write(NULL) → -1", ret == -1);
    printf("  %s: write(NULL) = %d\n", ret == -1 ? "PASS" : "FAIL", ret);

    /* seek NULL */
    ret = gamelib_file_seek(NULL, NULL, 0, 0);
    add_result("seek(NULL) → -1", ret == -1);
    printf("  %s: seek(NULL) = %d\n", ret == -1 ? "PASS" : "FAIL", ret);
}

static void demo_16_fs_test(gamelib_t *g)
{
    (void)g;
    printf("\n========== GameLib File System API Test ==========\n");

    /* reset counters */
    g_test_count = 0;
    g_pass_count = 0;

    test_write_read();
    test_seek_tell();
    test_append();
    test_error_cases();

    printf("\nResult: %d/%d tests passed\n\n", g_pass_count, g_test_count);

    /* display loop */
    while (!gamelib_is_closed(g)) {
        gamelib_clear(g, COLOR_DARK_BLUE);
        draw_results(g);

        if (gamelib_is_key_pressed(g, KEY_B)) break;

        gamelib_update(g);
        gamelib_wait_frame(g);
    }
}
