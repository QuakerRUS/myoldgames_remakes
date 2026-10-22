#include <doctest/doctest.h>

#include <cctype>
#include <string>

#include "hmac_sha256.hpp"
#include "save_codec.hpp"

using namespace piton_game;

namespace {

std::string hex_hmac(const std::string& key, const std::string& data) { return to_hex(hmac_sha256(key, data)); }

std::string bytes(std::size_t n, unsigned char b) { return std::string(n, static_cast<char>(b)); }

}  // namespace

TEST_CASE("SHA-256 (FIPS 180-4 examples)") {
    CHECK(to_hex(sha256("")) == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    CHECK(to_hex(sha256("abc")) == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    CHECK(to_hex(sha256("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq")) ==
          "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
    CHECK(to_hex(sha256(std::string(1000000, 'a'))) == "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
}

TEST_CASE("HMAC-SHA256: RFC 4231 test vectors") {
    // Test case 1
    CHECK(hex_hmac(bytes(20, 0x0b), "Hi There") == "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7");
    // Test case 2
    CHECK(hex_hmac("Jefe", "what do ya want for nothing?") ==
          "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843");
    // Test case 3
    CHECK(hex_hmac(bytes(20, 0xaa), bytes(50, 0xdd)) == "773ea91e36800e46854db8ebd09181a72959098b3ef8c122d9635514ced565fe");
    // Test case 4
    std::string key4;
    for (int i = 1; i <= 25; ++i) key4 += static_cast<char>(i);
    CHECK(hex_hmac(key4, bytes(50, 0xcd)) == "82558a389a443c0ea4cc819899f2083a85f0faa3e578f8077a2e3ff46729665b");
    // Test case 5 (output truncated to 128 bits)
    CHECK(hex_hmac(bytes(20, 0x0c), "Test With Truncation").substr(0, 32) == "a3b6167473100ee06e0c796c2955552b");
    // Test case 6
    CHECK(hex_hmac(bytes(131, 0xaa), "Test Using Larger Than Block-Size Key - Hash Key First") ==
          "60e431591ee0b67f0d8a26aacbf5b77f8e0bc6213728c5140546040f0ee37f54");
    // Test case 7
    CHECK(hex_hmac(bytes(131, 0xaa),
                   "This is a test using a larger than block-size key and a larger than block-size data. The key needs "
                   "to be hashed before being used by the HMAC algorithm.") ==
          "9b09ffa71b942fcb27635fbcd5b0e944bfdc63644f0713938a7f51535c3a35e2");
}

TEST_CASE("constant_time_equal") {
    CHECK(constant_time_equal("abc", "abc"));
    CHECK_FALSE(constant_time_equal("abc", "abd"));
    CHECK_FALSE(constant_time_equal("abc", "abcd"));
    CHECK(constant_time_equal("", ""));
}

TEST_CASE("save signature") {
    CHECK(save_signature_message(3, 1200) == "piton-save/1\nlevel_reached=3\nhigh_score=1200\n");
    const std::string sig = save_signature(3, 1200);
    CHECK(sig.size() == 64);
    CHECK(sig == save_signature(3, 1200));
    CHECK(sig != save_signature(4, 1200));
    CHECK(sig != save_signature(3, 1210));
    CHECK(sig != hex_hmac("", save_signature_message(3, 1200)));  // a real key is used
}

TEST_CASE("save round trip keeps everything") {
    SaveData s;
    s.level_reached = 7;
    s.high_score = 98765;
    s.sound_volume = 30;
    s.music_volume = 0;
    s.fullscreen = true;
    const nlohmann::json j = save_to_json(s);
    CHECK(j.at("sig").get<std::string>() == save_signature(7, 98765));
    // Through text, as on disk.
    const SaveData back = save_from_json(nlohmann::json::parse(j.dump(2)));
    CHECK_FALSE(back.tampered);
    CHECK(back.level_reached == 7);
    CHECK(back.high_score == 98765);
    CHECK(back.sound_volume == 30);
    CHECK(back.music_volume == 0);
    CHECK(back.fullscreen);
}

TEST_CASE("edited progress is detected: progress reset, settings kept") {
    SaveData s;
    s.level_reached = 2;
    s.high_score = 500;
    s.sound_volume = 40;
    s.music_volume = 60;
    s.fullscreen = true;
    const nlohmann::json good = save_to_json(s);

    auto check_reset = [](const nlohmann::json& j) {
        const SaveData r = save_from_json(j);
        CHECK(r.tampered);
        CHECK(r.level_reached == 1);
        CHECK(r.high_score == 0);
        CHECK(r.sound_volume == 40);
        CHECK(r.music_volume == 60);
        CHECK(r.fullscreen);
    };

    SUBCASE("level raised") {
        nlohmann::json j = good;
        j["level_reached"] = 10;
        check_reset(j);
    }
    SUBCASE("high score raised") {
        nlohmann::json j = good;
        j["high_score"] = 999999;
        check_reset(j);
    }
    SUBCASE("sig removed") {
        nlohmann::json j = good;
        j.erase("sig");
        check_reset(j);
    }
    SUBCASE("sig garbage / wrong type / upper-case") {
        nlohmann::json j = good;
        j["sig"] = "nope";
        check_reset(j);
        j["sig"] = 12345;
        check_reset(j);
        std::string upper = good.at("sig").get<std::string>();
        for (char& c : upper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        j["sig"] = upper;
        check_reset(j);
    }
    SUBCASE("progress fields of the wrong type") {
        nlohmann::json j = good;
        j["level_reached"] = "2";
        check_reset(j);
        j = good;
        j["high_score"] = 500.5;
        check_reset(j);
        j = good;
        j["high_score"] = 1e30;
        check_reset(j);
    }
    SUBCASE("signature copied from another save") {
        SaveData other;
        other.level_reached = 10;
        other.high_score = 500;
        nlohmann::json j = good;
        j["level_reached"] = 10;
        j["sig"] = save_to_json(other).at("sig");
        CHECK_FALSE(save_from_json(j).tampered);  // a genuine pair is accepted, of course
        j["high_score"] = 501;
        check_reset(j);
    }
    SUBCASE("settings changes alone are fine") {
        nlohmann::json j = good;
        j["sound_volume"] = 100;
        j["fullscreen"] = false;
        const SaveData r = save_from_json(j);
        CHECK_FALSE(r.tampered);
        CHECK(r.level_reached == 2);
        CHECK(r.high_score == 500);
        CHECK(r.sound_volume == 100);
        CHECK_FALSE(r.fullscreen);
    }
}

TEST_CASE("odd save contents never throw") {
    CHECK(save_from_json(nlohmann::json::array()).tampered);
    CHECK(save_from_json(nlohmann::json(42)).tampered);
    CHECK(save_from_json(nullptr).tampered);
    // Settings only (no progress, no sig): defaults, not tampered.
    const SaveData s = save_from_json(nlohmann::json{{"sound_volume", 20}, {"fullscreen", "yes"}});
    CHECK_FALSE(s.tampered);
    CHECK(s.sound_volume == 20);
    CHECK_FALSE(s.fullscreen);
    CHECK(s.level_reached == 1);
    // Bad settings values fall back to the defaults.
    const SaveData d = save_from_json(nlohmann::json{{"sound_volume", "loud"}, {"music_volume", -50}});
    CHECK(d.sound_volume == 80);
    CHECK(d.music_volume == 0);
}

TEST_CASE("values are clamped and the clamped values are signed") {
    SaveData s;
    s.level_reached = 99;
    s.high_score = -5;
    const nlohmann::json j = save_to_json(s);
    CHECK(j.at("level_reached") == 10);
    CHECK(j.at("high_score") == 0);
    const SaveData back = save_from_json(j);
    CHECK_FALSE(back.tampered);
    CHECK(back.level_reached == 10);
}
