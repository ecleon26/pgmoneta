/*
 * Copyright (C) 2026 The pgmoneta community
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list
 * of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or other
 * materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may
 * be used to endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <pgmoneta.h>
#include <mctf.h>
#include <utf8.h>

#include <stdbool.h>
#include <stddef.h>

#define UTF8_VALID(s) pgmoneta_utf8_valid((const unsigned char *)(s), sizeof(s) - 1)
#define UTF8_COUNT(s) pgmoneta_utf8_char_length((const unsigned char *)(s), sizeof(s) - 1)

MCTF_TEST(test_utf8_valid_sequences)
{
   MCTF_ASSERT(pgmoneta_utf8_valid((const unsigned char *)"", 0), cleanup, "empty buffer should be valid");
   MCTF_ASSERT(UTF8_VALID("password"), cleanup, "ASCII should be valid");
   MCTF_ASSERT(UTF8_VALID("\xC2\x80"), cleanup, "U+0080 should be valid");
   MCTF_ASSERT(UTF8_VALID("\xDF\xBF"), cleanup, "U+07FF should be valid");
   MCTF_ASSERT(UTF8_VALID("\xE0\xA0\x80"), cleanup, "U+0800 should be valid");
   MCTF_ASSERT(UTF8_VALID("\xED\x9F\xBF"), cleanup, "U+D7FF should be valid");
   MCTF_ASSERT(UTF8_VALID("\xEE\x80\x80"), cleanup, "U+E000 should be valid");
   MCTF_ASSERT(UTF8_VALID("\xF0\x90\x80\x80"), cleanup, "U+10000 should be valid");
   MCTF_ASSERT(UTF8_VALID("\xF4\x8F\xBF\xBF"), cleanup, "U+10FFFF should be valid");
   MCTF_ASSERT(UTF8_VALID("p\xC3\xA5ss\xE2\x82\xAC\xF0\x9F\x94\x91"), cleanup, "mixed sequence should be valid");

cleanup:
   MCTF_FINISH();
}

MCTF_TEST(test_utf8_invalid_sequences)
{
   /* Overlong encodings */
   MCTF_ASSERT(!UTF8_VALID("\xC0\xAF"), cleanup, "overlong 2-byte C0 should be invalid");
   MCTF_ASSERT(!UTF8_VALID("\xC1\xBF"), cleanup, "overlong 2-byte C1 should be invalid");
   MCTF_ASSERT(!UTF8_VALID("\xE0\x9F\xBF"), cleanup, "overlong 3-byte should be invalid");
   MCTF_ASSERT(!UTF8_VALID("\xF0\x8F\xBF\xBF"), cleanup, "overlong 4-byte should be invalid");

   /* Surrogates */
   MCTF_ASSERT(!UTF8_VALID("\xED\xA0\x80"), cleanup, "U+D800 should be invalid");
   MCTF_ASSERT(!UTF8_VALID("\xED\xBF\xBF"), cleanup, "U+DFFF should be invalid");

   /* Beyond U+10FFFF */
   MCTF_ASSERT(!UTF8_VALID("\xF4\x90\x80\x80"), cleanup, "U+110000 should be invalid");
   MCTF_ASSERT(!UTF8_VALID("\xF5\x80\x80\x80"), cleanup, "F5 lead byte should be invalid");
   MCTF_ASSERT(!UTF8_VALID("\xFF"), cleanup, "FF should be invalid");

   /* Bad lead / continuation bytes */
   MCTF_ASSERT(!UTF8_VALID("\x80"), cleanup, "lone continuation byte should be invalid");
   MCTF_ASSERT(!UTF8_VALID("\xC3\x28"), cleanup, "bad continuation byte should be invalid");
   MCTF_ASSERT(!UTF8_VALID("\xE2\x82\x28"), cleanup, "bad 3rd byte should be invalid");

   /* Truncated */
   MCTF_ASSERT(!UTF8_VALID("\xC3"), cleanup, "truncated 2-byte should be invalid");
   MCTF_ASSERT(!UTF8_VALID("\xE2\x82"), cleanup, "truncated 3-byte should be invalid");
   MCTF_ASSERT(!UTF8_VALID("\xF0\x9F\x94"), cleanup, "truncated 4-byte should be invalid");
   MCTF_ASSERT(!pgmoneta_utf8_valid((const unsigned char *)"\xE2\x82\xAC", 2), cleanup,
               "length cutting a sequence should be invalid");

cleanup:
   MCTF_FINISH();
}

MCTF_TEST(test_utf8_sequence_length)
{
   MCTF_ASSERT_INT_EQ(pgmoneta_utf8_sequence_length(0x41), 1, cleanup, "ASCII should be length 1");
   MCTF_ASSERT_INT_EQ(pgmoneta_utf8_sequence_length(0xC3), 2, cleanup, "C3 should be length 2");
   MCTF_ASSERT_INT_EQ(pgmoneta_utf8_sequence_length(0xE2), 3, cleanup, "E2 should be length 3");
   MCTF_ASSERT_INT_EQ(pgmoneta_utf8_sequence_length(0xF0), 4, cleanup, "F0 should be length 4");
   MCTF_ASSERT_INT_EQ(pgmoneta_utf8_sequence_length(0x80), -1, cleanup, "continuation byte should be -1");
   MCTF_ASSERT_INT_EQ(pgmoneta_utf8_sequence_length(0xF8), -1, cleanup, "F8 should be -1");

cleanup:
   MCTF_FINISH();
}

MCTF_TEST(test_utf8_char_length)
{
   MCTF_ASSERT(pgmoneta_utf8_char_length((const unsigned char *)"", 0) == 0, cleanup, "empty should be 0");
   MCTF_ASSERT(UTF8_COUNT("abc") == 3, cleanup, "ASCII count wrong");
   MCTF_ASSERT(UTF8_COUNT("p\xC3\xA5ss\xE2\x82\xAC\xF0\x9F\x94\x91") == 6, cleanup, "mixed count wrong");
   MCTF_ASSERT(UTF8_COUNT("\xED\xA0\x80") == (size_t)-1, cleanup, "surrogate should return -1");
   MCTF_ASSERT(UTF8_COUNT("ab\xC3") == (size_t)-1, cleanup, "truncated should return -1");

cleanup:
   MCTF_FINISH();
}

MCTF_TEST(test_utf8_is_ascii)
{
   MCTF_ASSERT(pgmoneta_is_ascii("", 0), cleanup, "empty should be ASCII");
   MCTF_ASSERT(pgmoneta_is_ascii("Hello \x7F", 7), cleanup, "0x00-0x7F should be ASCII");
   MCTF_ASSERT(!pgmoneta_is_ascii("caf\xC3\xA9", 5), cleanup, "UTF-8 should not be ASCII");
   MCTF_ASSERT(!pgmoneta_is_ascii("\x80", 1), cleanup, "0x80 should not be ASCII");
   MCTF_ASSERT(pgmoneta_is_ascii("ab\xC3\xA9", 2), cleanup, "len should limit the check");

cleanup:
   MCTF_FINISH();
}
