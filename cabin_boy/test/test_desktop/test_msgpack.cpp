/**
 * CabinBoy msgpack Unit Tests
 *
 * These tests run on desktop (native platform) for fast feedback.
 * They verify msgpack encoding matches the Python MainDeck protocol.
 *
 * Run with: pio test -e native
 */

#include <unity.h>
#include <MsgPack.h>
#include <cstring>

// Test that we can pack a simple NOTIFY message
void test_pack_notify_message() {
    MsgPack::Packer packer;

    // Pack a NOTIFY message for property change
    packer.packMap(4);

    packer.packString("type");
    packer.packString("N");

    packer.packString("address");
    packer.packString("CANNON");

    packer.packString("from");
    packer.packString("CANNON");

    packer.packString("data");
    packer.packMap(2);
    packer.packString("prop");
    packer.packString("firing");
    packer.packString("val");
    packer.packBool(true);

    // Verify we got some output
    TEST_ASSERT_GREATER_THAN(0, packer.size());

    // Verify we can unpack it
    MsgPack::Unpacker unpacker;
    unpacker.feed(packer.data(), packer.size());

    TEST_ASSERT_TRUE(unpacker.isMap());
}

// Test that we can pack a COMMAND message
void test_pack_command_message() {
    MsgPack::Packer packer;

    packer.packMap(4);

    packer.packString("type");
    packer.packString("C");

    packer.packString("address");
    packer.packString("CANNON");

    packer.packString("from");
    packer.packString("MOBILE_UI");

    packer.packString("data");
    packer.packMap(1);
    packer.packString("command");
    packer.packString("FIRE");

    TEST_ASSERT_GREATER_THAN(0, packer.size());

    // Unpack and verify structure
    MsgPack::Unpacker unpacker;
    unpacker.feed(packer.data(), packer.size());

    size_t mapSize;
    TEST_ASSERT_TRUE(unpacker.isMap());
    unpacker.unpackMapSize(mapSize);
    TEST_ASSERT_EQUAL(4, mapSize);
}

// Test SET_ADDRESS message format
void test_pack_set_address_message() {
    MsgPack::Packer packer;

    packer.packMap(2);
    packer.packString("type");
    packer.packString("SA");
    packer.packString("address");
    packer.packString("CANNON");

    TEST_ASSERT_GREATER_THAN(0, packer.size());

    // Unpack and verify
    MsgPack::Unpacker unpacker;
    unpacker.feed(packer.data(), packer.size());

    size_t mapSize;
    unpacker.unpackMapSize(mapSize);
    TEST_ASSERT_EQUAL(2, mapSize);

    String key, value;
    unpacker.unpackString(key);
    TEST_ASSERT_EQUAL_STRING("type", key.c_str());
    unpacker.unpackString(value);
    TEST_ASSERT_EQUAL_STRING("SA", value.c_str());
}

// Test SUBSCRIBE message format
void test_pack_subscribe_message() {
    MsgPack::Packer packer;

    packer.packMap(2);
    packer.packString("type");
    packer.packString("S");
    packer.packString("address");
    packer.packString("CANNON");

    // Unpack and verify
    MsgPack::Unpacker unpacker;
    unpacker.feed(packer.data(), packer.size());

    size_t mapSize;
    unpacker.unpackMapSize(mapSize);
    TEST_ASSERT_EQUAL(2, mapSize);
}

// Test unpacking a command message (simulating received data)
void test_unpack_command_message() {
    // First pack a command
    MsgPack::Packer packer;
    packer.packMap(3);
    packer.packString("type");
    packer.packString("C");
    packer.packString("address");
    packer.packString("CANNON");
    packer.packString("data");
    packer.packMap(1);
    packer.packString("command");
    packer.packString("FIRE");

    // Now unpack it
    MsgPack::Unpacker unpacker;
    unpacker.feed(packer.data(), packer.size());

    size_t mapSize;
    unpacker.unpackMapSize(mapSize);

    String msgType, msgAddress;

    for (size_t i = 0; i < mapSize; i++) {
        String key;
        unpacker.unpackString(key);

        if (key == "type") {
            unpacker.unpackString(msgType);
        } else if (key == "address") {
            unpacker.unpackString(msgAddress);
        } else {
            unpacker.skip();
        }
    }

    TEST_ASSERT_EQUAL_STRING("C", msgType.c_str());
    TEST_ASSERT_EQUAL_STRING("CANNON", msgAddress.c_str());
}

// Test property values with different types
void test_property_types() {
    // Boolean property
    {
        MsgPack::Packer packer;
        packer.packBool(true);
        TEST_ASSERT_GREATER_THAN(0, packer.size());
    }

    // Integer property
    {
        MsgPack::Packer packer;
        packer.packInt32(42);
        TEST_ASSERT_GREATER_THAN(0, packer.size());
    }

    // Float property
    {
        MsgPack::Packer packer;
        packer.packFloat32(3.14f);
        TEST_ASSERT_GREATER_THAN(0, packer.size());
    }

    // String property
    {
        MsgPack::Packer packer;
        packer.packString("hello");
        TEST_ASSERT_GREATER_THAN(0, packer.size());
    }
}

// Required for native platform
#ifndef ARDUINO

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_pack_notify_message);
    RUN_TEST(test_pack_command_message);
    RUN_TEST(test_pack_set_address_message);
    RUN_TEST(test_pack_subscribe_message);
    RUN_TEST(test_unpack_command_message);
    RUN_TEST(test_property_types);

    return UNITY_END();
}

#else

void setup() {
    delay(2000);  // Wait for serial
    UNITY_BEGIN();

    RUN_TEST(test_pack_notify_message);
    RUN_TEST(test_pack_command_message);
    RUN_TEST(test_pack_set_address_message);
    RUN_TEST(test_pack_subscribe_message);
    RUN_TEST(test_unpack_command_message);
    RUN_TEST(test_property_types);

    UNITY_END();
}

void loop() {}

#endif
