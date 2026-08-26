#include <gtest/gtest.h>

#include <vector>

#include "core/MavlinkEncoder.hpp"

using core::MavlinkEncoder;

// ══════════════════════════════════════════════════════════════
//  DES TESTS À VECTEURS CONNUS
// ══════════════════════════════════════════════════════════════
//
// Ces tests-là ne vérifient pas « la fonction a l'air de marcher ». Ils
// comparent la sortie, OCTET PAR OCTET, à celle de `pymavlink` —
// l'implémentation Python de référence du protocole, celle qu'utilisent
// PX4 et QGroundControl.
//
// Les vecteurs ci-dessous ont été produits ainsi :
//
//     from pymavlink.dialects.v20 import common as m
//     mav = m.MAVLink(None, srcSystem=1, srcComponent=25)
//     msg = m.MAVLink_named_value_float_message(12345, b'GAS_PPM', 1500.0)
//     print(msg.pack(mav).hex())
//
// C'est la seule façon honnête de valider une implémentation de
// protocole. Un test écrit à partir de ta propre compréhension ne
// prouve rien : si tu as mal lu la spécification, ton code ET ton test
// seront faux de la même façon, et ils s'accorderont parfaitement.
//
// Il faut une source de vérité EXTÉRIEURE. C'est une leçon qui dépasse
// largement MAVLink.

namespace {

void expect_bytes(const std::vector<std::uint8_t>& expected,
                  const std::uint8_t* actual, std::size_t n) {
    ASSERT_EQ(n, expected.size()) << "longueur de trame inattendue";
    for (std::size_t i = 0; i < n; ++i) {
        EXPECT_EQ(actual[i], expected[i])
            << "divergence à l'octet " << i;
    }
}

}  // namespace

TEST(MavlinkEncoder, named_value_float_octet_pour_octet) {
    MavlinkEncoder enc(1, 25);
    std::uint8_t out[MavlinkEncoder::kMaxFrame];

    const std::size_t n =
        enc.encode_named_value_float(12345, "GAS_PPM", 1500.0f, out, sizeof(out));

    // Référence pymavlink, séquence 0 :
    const std::vector<std::uint8_t> expected = {
        0xFD, 0x0F, 0x00, 0x00, 0x00, 0x01, 0x19, 0xFB, 0x00, 0x00,
        0x39, 0x30, 0x00, 0x00,                    // time_boot_ms = 12345
        0x00, 0x80, 0xBB, 0x44,                    // value = 1500.0f
        0x47, 0x41, 0x53, 0x5F, 0x50, 0x50, 0x4D,  // "GAS_PPM", zéros tronqués
        0xA3, 0x88                                 // CRC
    };
    expect_bytes(expected, out, n);
}

TEST(MavlinkEncoder, statustext_octet_pour_octet) {
    MavlinkEncoder enc(1, 25);
    std::uint8_t out[MavlinkEncoder::kMaxFrame];

    enc.encode_named_value_float(0, "X", 0.0f, out, sizeof(out));   // consomme seq 0
    const std::size_t n = enc.encode_statustext(4, "GAS ALERT", out, sizeof(out));

    // Référence pymavlink, séquence 1 :
    const std::vector<std::uint8_t> expected = {
        0xFD, 0x0A, 0x00, 0x00, 0x01, 0x01, 0x19, 0xFD, 0x00, 0x00,
        0x04,                                             // sévérité
        0x47, 0x41, 0x53, 0x20, 0x41, 0x4C, 0x45, 0x52, 0x54,   // "GAS ALERT"
        0x0B, 0x0D                                        // CRC
    };
    expect_bytes(expected, out, n);
}

TEST(MavlinkEncoder, la_troncature_supprime_les_zeros_de_fin) {
    MavlinkEncoder enc(1, 25);
    std::uint8_t out[MavlinkEncoder::kMaxFrame];

    // Le champ `name` fait 10 octets, mais "GAS_PPM" n'en occupe que 7.
    // Charge utile pleine : 4 + 4 + 10 = 18. Attendue après troncature : 15.
    const std::size_t n =
        enc.encode_named_value_float(12345, "GAS_PPM", 1500.0f, out, sizeof(out));

    EXPECT_EQ(out[1], 15);       // longueur annoncée
    EXPECT_EQ(n, 27u);           // 10 en-tête + 15 charge utile + 2 CRC
}

TEST(MavlinkEncoder, la_sequence_s_incremente_a_chaque_trame) {
    MavlinkEncoder enc(1, 25);
    std::uint8_t out[MavlinkEncoder::kMaxFrame];

    enc.encode_heartbeat(out, sizeof(out));
    EXPECT_EQ(out[4], 0);
    enc.encode_heartbeat(out, sizeof(out));
    EXPECT_EQ(out[4], 1);
    enc.encode_heartbeat(out, sizeof(out));
    EXPECT_EQ(out[4], 2);

    // C'est ce compteur qui permet au sol de savoir qu'il a perdu des
    // messages : s'il reçoit 5 puis 8, deux trames sont tombées.
}

TEST(MavlinkEncoder, refuse_d_ecrire_dans_un_tampon_trop_petit) {
    MavlinkEncoder enc(1, 25);
    std::uint8_t tiny[4];

    EXPECT_EQ(enc.encode_heartbeat(tiny, sizeof(tiny)), 0u);

    // Écrire au-delà d'un tampon est LA faille de sécurité classique en C
    // et en C++, et la cause de bien des plantages en vol. On vérifie la
    // capacité, on renvoie 0, et l'appelant sait que rien n'a été émis.
}
