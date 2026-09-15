#include <espsand/sim/model.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>

namespace espsand::sim {
namespace {

constexpr std::uint64_t kFnvOffsetBasis = 14695981039346656037ULL;
constexpr std::uint64_t kFnvPrime = 1099511628211ULL;
constexpr std::uint32_t kStateHashMagic = 0x45535034U;
constexpr std::uint8_t kFixtureCenterX = static_cast<std::uint8_t>(kWorldWidth / 2U);
constexpr std::uint8_t kFixtureCenterY = static_cast<std::uint8_t>(kWorldHeight / 2U);

class StableHasher {
public:
  void add_u8(std::uint8_t value) noexcept {
    hash_ ^= value;
    hash_ *= kFnvPrime;
  }

  void add_u16(std::uint16_t value) noexcept {
    for (unsigned shift = 0; shift < 16U; shift += 8U) {
      add_u8(static_cast<std::uint8_t>(value >> shift));
    }
  }

  void add_u32(std::uint32_t value) noexcept {
    for (unsigned shift = 0; shift < 32U; shift += 8U) {
      add_u8(static_cast<std::uint8_t>(value >> shift));
    }
  }

  void add_u64(std::uint64_t value) noexcept {
    for (unsigned shift = 0; shift < 64U; shift += 8U) {
      add_u8(static_cast<std::uint8_t>(value >> shift));
    }
  }

  std::uint64_t value() const noexcept {
    return hash_;
  }

private:
  std::uint64_t hash_ = kFnvOffsetBasis;
};

std::uint8_t unit_to_q8(float value) noexcept {
  return static_cast<std::uint8_t>(value * 255.0F + 0.5F);
}

std::uint16_t saturating_add(std::uint16_t value, std::uint16_t increment) noexcept {
  const std::uint32_t sum = static_cast<std::uint32_t>(value) + increment;
  const auto maximum = std::numeric_limits<std::uint16_t>::max();
  return sum > maximum ? maximum : static_cast<std::uint16_t>(sum);
}

void add_budget_stats(StableHasher& hasher, const WorkBudgetStats& stats) noexcept {
  hasher.add_u16(stats.limit);
  hasher.add_u16(stats.used);
  hasher.add_u16(stats.dropped);
}

Cell fixture_marker_cell() noexcept {
  Cell cell{};
  cell.material = MaterialId::kTracer;
  cell.mass = 64;
  cell.aux = 1;
  return cell;
}

} // namespace

Model::Model() noexcept {
  init(ModelConfig{});
}

Model::Model(const ModelConfig& config) noexcept {
  init(config);
}

void Model::init(const ModelConfig& config) noexcept {
  config_ = config;
  if (config_.scene != SceneId::kDeterminismFixture) {
    config_.scene = SceneId::kDeterminismFixture;
  }
  reset();
}

void Model::reset() noexcept {
  tick_ = 0;
  prng_.reseed(config_.seed);
  event_budget_.reset(config_.event_budget);
  reaction_budget_.reset(config_.reaction_budget);
  world_.clear();
  fixture_ = FixtureStateSnapshot{};
  initialize_fixture();
}

void Model::reseed(std::uint64_t seed) noexcept {
  config_.seed = seed;
  reset();
}

void Model::step(const InputFrame& input) noexcept {
  const InputFrame frame = sanitize_input_frame(input);

  event_budget_.reset(config_.event_budget);
  reaction_budget_.reset(config_.reaction_budget);

  fixture_.cap_combo_q8 = unit_to_q8(frame.cap_combo);
  if (frame.slider_active) {
    fixture_.slider_position_q8 = unit_to_q8(frame.slider_position);
    fixture_.slider_strength_q8 = unit_to_q8(frame.slider_strength);
  }

  if (frame.cap_combo_event && event_budget_.try_consume()) {
    const std::uint16_t impulse =
        static_cast<std::uint16_t>(fixture_.cap_combo_q8 == 0 ? 1 : fixture_.cap_combo_q8);
    fixture_.external_impulse = saturating_add(fixture_.external_impulse, impulse);
  }

  if (frame.noise_event && frame.noise_impulse > 0.0F && event_budget_.try_consume()) {
    const std::uint8_t quantized = unit_to_q8(frame.noise_impulse);
    const std::uint16_t impulse = static_cast<std::uint16_t>(quantized == 0 ? 1 : quantized);
    fixture_.external_impulse = saturating_add(fixture_.external_impulse, impulse);
  }

  if (frame.tap_impulse >= 0.5F && event_budget_.try_consume()) {
    relocate_fixture_marker();
  }

  ++tick_;
}

TickWorkStats Model::tick_work_stats() const noexcept {
  return TickWorkStats{event_budget_.stats(), reaction_budget_.stats()};
}

FixtureStateSnapshot Model::fixture_state() const noexcept {
  return fixture_;
}

bool Model::invariants_hold() const noexcept {
  if (!world_.invariants_hold()) {
    return false;
  }

  if (!world_.in_bounds(fixture_.marker_x, fixture_.marker_y)) {
    return false;
  }

  const Cell* marker = world_.try_cell(fixture_.marker_x, fixture_.marker_y);
  return marker != nullptr && marker->material == MaterialId::kTracer && marker->mass == 64;
}

std::uint64_t Model::state_hash() const noexcept {
  StableHasher hasher;

  hasher.add_u32(kStateHashMagic);
  hasher.add_u32(kStateHashSchemaVersion);
  hasher.add_u32(kMaterialRegistryVersion);
  hasher.add_u32(kCellSchemaVersion);
  hasher.add_u16(static_cast<std::uint16_t>(kWorldWidth));
  hasher.add_u16(static_cast<std::uint16_t>(kWorldHeight));
  hasher.add_u8(static_cast<std::uint8_t>(config_.scene));

  hasher.add_u64(config_.seed);
  hasher.add_u64(tick_);
  hasher.add_u16(config_.event_budget);
  hasher.add_u16(config_.reaction_budget);
  hasher.add_u64(prng_.state());
  hasher.add_u64(prng_.increment());

  hasher.add_u8(fixture_.marker_x);
  hasher.add_u8(fixture_.marker_y);
  hasher.add_u8(fixture_.slider_position_q8);
  hasher.add_u8(fixture_.slider_strength_q8);
  hasher.add_u8(fixture_.cap_combo_q8);
  hasher.add_u16(fixture_.external_impulse);

  add_budget_stats(hasher, event_budget_.stats());
  add_budget_stats(hasher, reaction_budget_.stats());

  for (const Cell& cell : world_.cells()) {
    hasher.add_u8(static_cast<std::uint8_t>(cell.material));
    hasher.add_u8(cell.mass);
    hasher.add_u8(static_cast<std::uint8_t>(cell.motion_x));
    hasher.add_u8(static_cast<std::uint8_t>(cell.motion_y));
    hasher.add_u16(static_cast<std::uint16_t>(cell.temperature));
    hasher.add_u8(cell.aux);
    hasher.add_u8(cell.flags);
  }

  return hasher.value();
}

void Model::initialize_fixture() noexcept {
  for (std::size_t y = 0; y < kWorldHeight; ++y) {
    for (std::size_t x = 0; x < kWorldWidth; ++x) {
      if (x != 0 && y != 0 && x + 1U != kWorldWidth && y + 1U != kWorldHeight) {
        continue;
      }

      Cell wall{};
      wall.material = MaterialId::kWall;
      wall.mass = 255;
      static_cast<void>(
          world_.set_cell(static_cast<int>(x), static_cast<int>(y), wall));
    }
  }

  Cell water{};
  water.material = MaterialId::kWater;
  water.mass = 128;
  static_cast<void>(world_.set_cell(kFixtureCenterX, kFixtureCenterY, water));

  const auto interior_width = static_cast<std::uint32_t>(kWorldWidth - 2U);
  fixture_.marker_x = static_cast<std::uint8_t>(1U + prng_.bounded(interior_width));
  fixture_.marker_y = static_cast<std::uint8_t>(1U + prng_.bounded(interior_width));

  if (fixture_.marker_x == kFixtureCenterX && fixture_.marker_y == kFixtureCenterY) {
    const std::uint8_t maximum = static_cast<std::uint8_t>(kWorldWidth - 2U);
    fixture_.marker_x =
        fixture_.marker_x == maximum ? 1U : static_cast<std::uint8_t>(fixture_.marker_x + 1U);
  }

  static_cast<void>(
      world_.set_cell(fixture_.marker_x, fixture_.marker_y, fixture_marker_cell()));
}

void Model::relocate_fixture_marker() noexcept {
  static_cast<void>(world_.set_cell(fixture_.marker_x, fixture_.marker_y, Cell{}));

  const auto interior_width = static_cast<std::uint32_t>(kWorldWidth - 2U);
  fixture_.marker_x = static_cast<std::uint8_t>(1U + prng_.bounded(interior_width));
  fixture_.marker_y = static_cast<std::uint8_t>(1U + prng_.bounded(interior_width));

  if (fixture_.marker_x == kFixtureCenterX && fixture_.marker_y == kFixtureCenterY) {
    const std::uint8_t maximum = static_cast<std::uint8_t>(kWorldWidth - 2U);
    fixture_.marker_x =
        fixture_.marker_x == maximum ? 1U : static_cast<std::uint8_t>(fixture_.marker_x + 1U);
  }

  static_cast<void>(
      world_.set_cell(fixture_.marker_x, fixture_.marker_y, fixture_marker_cell()));
}

} // namespace espsand::sim
