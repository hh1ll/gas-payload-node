#include "core/TelemetryReporter.hpp"

namespace core {

TelemetryReporter::TelemetryReporter(ITelemetrySink& sink, MavlinkEncoder& encoder)
    : sink_(sink),
      encoder_(encoder),
      last_reported_alert_(AlertState::Normal),
      first_report_(true) {}

void TelemetryReporter::send_heartbeat() {
    std::uint8_t buffer[MavlinkEncoder::kMaxFrame];
    const std::size_t n = encoder_.encode_heartbeat(buffer, sizeof(buffer));
    if (n > 0) sink_.send(buffer, n);
}

// ─────────────────────────────────────────────────────────────
//  À TOI DE JOUER
// ─────────────────────────────────────────────────────────────
//
// Deux règles. La première est mécanique, la seconde demande de réfléchir.

void TelemetryReporter::report(const PayloadService& service,
                               std::uint32_t time_boot_ms) {
    std::uint8_t buffer[MavlinkEncoder::kMaxFrame];

    // Règle 1 — télémétrie continue
    std::size_t n = encoder_.encode_named_value_float(
        time_boot_ms, "GAS_PPM",
        service.last_valid_sample().concentration_ppm,
        buffer, sizeof(buffer));
    if (n > 0) sink_.send(buffer, n);

    // Règle 2 — événement, uniquement au changement
    const AlertState current = service.alert();
    if (first_report_ || current != last_reported_alert_) {
        const std::uint8_t severity = (current == AlertState::Alert) ? 4 : 6;
        const char* text = (current == AlertState::Alert) ? "GAS ALERT" : "Gas normal";

        n = encoder_.encode_statustext(severity, text, buffer, sizeof(buffer));
        if (n > 0) sink_.send(buffer, n);

        last_reported_alert_ = current;
        first_report_        = false;
    }
}

}  // namespace core
