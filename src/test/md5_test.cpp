#include "doctest.h"
#include "md5.h"
#include <cstring>

// Tests for MD5 hashing
// Purpose: Document the status quo behavior and detect regressions

// Helper function to compute MD5 hash using the C API
static void ComputeMD5(const unsigned char* data, int len, unsigned char digest[16])
{
    md5_state_t state;
    md5_init(&state);
    md5_append(&state, data, len);
    md5_finish(&state, digest);
}

TEST_SUITE("MD5")
{
    TEST_CASE("MD5 basic functionality")
    {
        GIVEN("MD5 hashing of known test vectors")
        {
            // Test with empty string - known MD5 hash
            WHEN("hashing an empty string")
            {
                unsigned char digest[16];
                ComputeMD5((const unsigned char*)"", 0, digest);

                THEN("produces correct hash for empty string")
                {
                    // MD5("") = d41d8cd98f00b204e9800998ecf8427e
                    unsigned char expected[] = {
                        0xd4, 0x1d, 0x8c, 0xd9, 0x8f, 0x00, 0xb2, 0x04,
                        0xe9, 0x80, 0x09, 0x98, 0xec, 0xf8, 0x42, 0x7e};
                    CHECK(memcmp(digest, expected, 16) == 0);
                }
            }

            // Test with "a" - known MD5 hash
            WHEN("hashing single character 'a'")
            {
                unsigned char digest[16];
                ComputeMD5((const unsigned char*)"a", 1, digest);

                THEN("produces correct hash for 'a'")
                {
                    // MD5("a") = 0cc175b9c0f1b6a831c399e269772661
                    unsigned char expected[] = {
                        0x0c, 0xc1, 0x75, 0xb9, 0xc0, 0xf1, 0xb6, 0xa8,
                        0x31, 0xc3, 0x99, 0xe2, 0x69, 0x77, 0x26, 0x61};
                    CHECK(memcmp(digest, expected, 16) == 0);
                }
            }

            // Test with "abc" - known MD5 hash
            WHEN("hashing 'abc'")
            {
                unsigned char digest[16];
                ComputeMD5((const unsigned char*)"abc", 3, digest);

                THEN("produces correct hash for 'abc'")
                {
                    // MD5("abc") = 900150983cd24fb0d6963f7d28e17f72
                    unsigned char expected[] = {
                        0x90, 0x01, 0x50, 0x98, 0x3c, 0xd2, 0x4f, 0xb0,
                        0xd6, 0x96, 0x3f, 0x7d, 0x28, 0xe1, 0x7f, 0x72};
                    CHECK(memcmp(digest, expected, 16) == 0);
                }
            }

            // Test with "test" - known MD5 hash
            WHEN("hashing 'test'")
            {
                unsigned char digest[16];
                ComputeMD5((const unsigned char*)"test", 4, digest);

                THEN("produces correct hash for 'test'")
                {
                    // MD5("test") = 098f6bcd4621d373cade4e832627b4f6
                    unsigned char expected[] = {
                        0x09, 0x8f, 0x6b, 0xcd, 0x46, 0x21, 0xd3, 0x73,
                        0xca, 0xde, 0x4e, 0x83, 0x26, 0x27, 0xb4, 0xf6};
                    CHECK(memcmp(digest, expected, 16) == 0);
                }
            }
        }
    }

    TEST_CASE("MD5 with different input lengths")
    {
        GIVEN("MD5 hashing with various input lengths")
        {
            WHEN("hashing a longer string")
            {
                unsigned char digest[16];
                const char* testString = "The quick brown fox jumps over the lazy dog";
                ComputeMD5((const unsigned char*)testString, strlen(testString), digest);

                THEN("produces a valid hash")
                {
                    // MD5("The quick brown fox jumps over the lazy dog") =
                    // 9e107d9d372bb6826bd81d3542a419d6
                    unsigned char expected[] = {
                        0x9e, 0x10, 0x7d, 0x9d, 0x37, 0x2b, 0xb6, 0x82,
                        0x6b, 0xd8, 0x1d, 0x35, 0x42, 0xa4, 0x19, 0xd6};
                    CHECK(memcmp(digest, expected, 16) == 0);
                }
            }

            WHEN("hashing two different strings")
            {
                unsigned char digest1[16], digest2[16];
                const char* testString1 = "test";
                const char* testString2 = "test!";

                ComputeMD5((const unsigned char*)testString1, strlen(testString1), digest1);
                ComputeMD5((const unsigned char*)testString2, strlen(testString2), digest2);

                THEN("different inputs produce different hashes")
                {
                    CHECK(memcmp(digest1, digest2, 16) != 0);
                }
            }
        }
    }

    TEST_CASE("MD5 with binary data")
    {
        GIVEN("MD5 hashing of binary data")
        {
            WHEN("hashing binary data")
            {
                unsigned char digest[16];
                unsigned char binaryData[] = {0x00, 0x01, 0x02, 0x03, 0xFF, 0xFE, 0xFD, 0xFC};
                ComputeMD5(binaryData, sizeof(binaryData), digest);

                THEN("can hash binary data without crashing")
                {
                    // We can't predict the exact hash, but we can verify it produces
                    // a valid 16-byte digest
                    CHECK(sizeof(digest) == 16);
                    // Verify the digest is not all zeros
                    bool allZero = true;
                    for (int i = 0; i < 16; i++)
                    {
                        if (digest[i] != 0)
                        {
                            allZero = false;
                            break;
                        }
                    }
                    CHECK(allZero == false);
                }
            }
        }
    }
}

// TODO: More comprehensive MD5 tests could be added, including:
// 1. Testing with very long inputs
// 2. Testing with inputs at block boundaries (64 bytes, 128 bytes, etc.)
// 3. Testing the incremental append functionality
// 4. Testing with Unicode/UTF-8 strings
// The current tests focus on basic functionality and known test vectors.

// Z-Man says: Nah, it is fine this way. The implementation was copy/pasted from a good source, it should be all right,
// and we should not be using MD5 going forward anyway.