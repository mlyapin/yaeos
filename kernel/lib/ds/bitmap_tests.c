/* UNITY_TEST DEPENDS ON: kernel/lib/ds/bitmap.c
 * UNITY_TEST DEPENDS ON: kernel/test_fakes/panic.c
 * UNITY_TEST DEPENDS ON: kernel/lib/cstd/string/memset.c
 */
#include "lib/ds/bitmap.h"

#include "lib/cppdefs.h"
#include "lib/tests/assert_failed.h"

#include <unity.h>

#define BITS_NUM (1024)

static struct bitmap bitmap;
static unsigned char bits[BITS_NUM];

void setUp(void)
{
        bitmap_init(&bitmap, bits, BITS_NUM);
}

void tearDown(void)
{}

static void inited_to_zero(void)
{
        for (size_t i = 0; i < BITS_NUM; i++) {
                TEST_ASSERT_FALSE(bitmap_get(&bitmap, i));
        }
}

static void manipulate_bits(void)
{
        TEST_ASSERT_FALSE(bitmap_get(&bitmap, 0));

        bitmap_set_true(&bitmap, 0);
        TEST_ASSERT_TRUE(bitmap_get(&bitmap, 0));
        /* Check that there's no harm to reset a bit to the same value. */
        bitmap_set_true(&bitmap, 0);
        TEST_ASSERT_TRUE(bitmap_get(&bitmap, 0));

        bitmap_set_false(&bitmap, 0);
        TEST_ASSERT_FALSE(bitmap_get(&bitmap, 0));
}

static void couldnt_get_past_boundaries(void)
{
        if(!failed_kassert(bitmap_get(&bitmap, BITS_NUM + 1))) {
                TEST_FAIL();
        }
}

static void search_false(void)
{
        size_t expected = BITS_NUM / 4;
        for (size_t i = 0; i < expected; i++) {
                bitmap_set_true(&bitmap, i);
        }

        size_t result = 0;
        if (!bitmap_search_false(&bitmap, &result)) {
                TEST_FAIL();
        }
        TEST_ASSERT_EQUAL_UINT32(expected, result);
}

int main(void)
{
        UNITY_BEGIN();
        RUN_TEST(inited_to_zero);
        RUN_TEST(manipulate_bits);
        RUN_TEST(couldnt_get_past_boundaries);
        RUN_TEST(search_false);
        UNITY_END();
        return (0);
}
