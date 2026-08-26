# gas-payload-node

![CI](https://github.com/hh1ll/gas-payload-node/actions/workflows/ci.yml/badge.svg)


# gas-payload-node

A methane-detection payload node for an autonomous aircraft — from the sensor
on its bus to the ground station, with the whole chain verified automatically
on every push.

---

## What it does

A gas sensor is read in a loop. The concentration feeds a decision layer that
raises an alert. The result is encoded as MAVLink telemetry and sent to
QGroundControl, where it can be plotted live.

```
[gas sensor] ──▶ [PayloadService] ──▶ [GasMonitor] ──▶ [TelemetryReporter]
   IGasSensor      reads, tracks       hysteresis        MAVLink frames
                   sensor health                              │
                                                    ITelemetrySink
                                                              │
                                                     [QGroundControl]
```

Two interfaces — `IGasSensor` and `ITelemetrySink` — sit at the boundaries.
The logic in between never touches hardware, which is what allows the entire
test suite to run on a GitHub runner where no sensor is connected.

---

## How this project was built

This is a **learning project**, and I want to be precise about what that means.

I am an electronics student. I started it after reading an embedded-software
internship posting whose requirements I did not meet: I had never used PX4, and
I had never set up a CI pipeline. Rather than claim to be "motivated to learn",
I wanted to find out what the chain actually looks like end to end — sensor,
decision, protocol, ground station, automated verification.

I built it with an AI assistant acting as a tutor.

C++ is not a language I am fluent in yet. I am comfortable with the constructs
this project uses, and I know where that comfort stops.

**So this repository is not evidence of C++ mastery.** It is evidence that I can
assemble a complete embedded pipeline, understand why each layer exists, and
explain the reasoning behind the design decisions. That was the point of
building it.

---

## What I actually learned

Four things I did not understand before, and now do:

**A hardware abstraction is not an elegance — it is what makes testing possible
at all.** I found this out the hard way. I added a real STM32 driver to the core
library, and CI broke instantly: a GitHub runner has no board and no vendor
headers. The driver was not the problem; its location was. Moving it behind an
interface and a build option turned CI green again, with the driver still in the
repository.

**A silent sensor does not say that everything is fine — it says nothing.** When
a read fails, no default value is passed to the decision layer. Zero ppm sits
below the low threshold, so the alert would clear itself, and a ground operator
would conclude the leak had been sealed when in fact we had gone blind. The
alert persists, with a degraded health flag alongside it.

**A single threshold makes an alarm flicker.** With one threshold, sensor noise
crossing it in both directions toggles the alert dozens of times per second, and
the operator learns to ignore it. Two thresholds — trigger high, release low —
fix it. The same reasoning applies to the radio link: a status message is only
sent when the state actually changes, never on every cycle.

**Not all tests belong in the same place.** Unit tests are deterministic and run
in seconds, so they run on every push. Anything that needs real hardware is
slow, scarce and occasionally flaky — and a CI that fails at random is a CI the
team learns to ignore. Those tests belong on a separate, slower tier.

---

## Build and run

```bash
cmake -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Stream simulated telemetry to QGroundControl (POSIX only):

```bash
./build/tools/qgc_stream            # or: qgc_stream <host> <port>
```

The demo simulates an aircraft flying over a leak: concentration rises, the
alert triggers, the sensor then fails for a few seconds — and the alert does
not clear.

The STM32 driver is not built by default, since it requires an ARM toolchain:

```bash
cmake -B build-stm32 -DBUILD_HAL_STM32=ON -DCMAKE_TOOLCHAIN_FILE=...
```

---

## Layout

```
include/core/   public interfaces
src/core/       logic — no hardware header, ever
src/hal/stm32/  real driver, built only when targeting the board
test/           unit tests and fake implementations
tools/          qgc_stream — full chain over UDP
demo/           a simulated overflight, printed to the console
```

---

## Roadmap

- [x] Decision logic, CMake build, unit tests on host
- [x] Continuous integration on GitHub Actions
- [x] Hardware abstraction layer and fake implementations
- [x] MAVLink telemetry, live in QGroundControl
- [ ] PX4 SITL in a container, automated integration test
- [ ] Real sensor on STM32, then hardware-in-the-loop testing

---

## A note on PX4 airship support

PX4 documents airship support as experimental — a single documented geometry,
and the project is looking for maintainers. There is no ready-made airship SITL
target, so integration testing here will use a standard simulated vehicle: the
subject of this project is the payload, not flight dynamics.
