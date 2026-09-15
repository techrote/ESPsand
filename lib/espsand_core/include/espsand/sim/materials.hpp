#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace espsand::sim {

enum class MaterialId : std::uint8_t {
  kEmpty = 0,
  kWall = 1,
  kCrust = 2,
  kWater = 3,
  kOil = 4,
  kLava = 5,
  kSteam = 6,
  kSmoke = 7,
  kFire = 8,
  kSodiumLike = 9,
  kTracer = 10,
  kMoss = 11,
  kCount = 12,
};

enum class MaterialCategory : std::uint8_t {
  kEmpty = 0,
  kSolid,
  kLiquid,
  kGas,
  kEnergy,
  kParticle,
  kScalar,
  kBiomass,
};

struct MaterialInfo {
  MaterialId id;
  MaterialCategory category;
  const char* name;
};

inline constexpr std::uint32_t kMaterialRegistryVersion = 1;
inline constexpr std::size_t kMaterialCount = static_cast<std::size_t>(MaterialId::kCount);

inline constexpr std::array<MaterialInfo, kMaterialCount> kMaterialRegistry{{
    {MaterialId::kEmpty, MaterialCategory::kEmpty, "empty"},
    {MaterialId::kWall, MaterialCategory::kSolid, "wall"},
    {MaterialId::kCrust, MaterialCategory::kSolid, "crust"},
    {MaterialId::kWater, MaterialCategory::kLiquid, "water"},
    {MaterialId::kOil, MaterialCategory::kLiquid, "oil"},
    {MaterialId::kLava, MaterialCategory::kLiquid, "lava"},
    {MaterialId::kSteam, MaterialCategory::kGas, "steam"},
    {MaterialId::kSmoke, MaterialCategory::kGas, "smoke"},
    {MaterialId::kFire, MaterialCategory::kEnergy, "fire"},
    {MaterialId::kSodiumLike, MaterialCategory::kParticle, "sodium_like"},
    {MaterialId::kTracer, MaterialCategory::kScalar, "tracer"},
    {MaterialId::kMoss, MaterialCategory::kBiomass, "moss"},
}};

constexpr bool is_valid_material(MaterialId id) noexcept {
  return static_cast<std::size_t>(id) < kMaterialCount;
}

constexpr std::size_t material_index(MaterialId id) noexcept {
  return static_cast<std::size_t>(id);
}

constexpr const MaterialInfo* material_info(MaterialId id) noexcept {
  return is_valid_material(id) ? &kMaterialRegistry[material_index(id)] : nullptr;
}

} // namespace espsand::sim
