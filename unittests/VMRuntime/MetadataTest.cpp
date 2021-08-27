/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "hermes/VM/Metadata.h"

#include "hermes/VM/BuildMetadata.h"
#include "hermes/VM/SymbolID.h"

#include "gtest/gtest.h"

namespace {

using namespace hermes::vm;

struct DummyCell final {
 public:
  static void buildMeta(const GCCell *cell, Metadata::Builder &mb);
  static const VTable vt;
  GCPointerBase x_;
  GCPointerBase y_;
  GCPointerBase z_;
  GCSymbolID sym_;
};

const VTable DummyCell::vt{CellKind::DummyObjectKind, sizeof(DummyCell)};

static_assert(
    std::is_standard_layout<DummyCell>::value,
    "DummyCell isn't a standard layout, offsetof won't work");

void DummyCell::buildMeta(const GCCell *cell, Metadata::Builder &mb) {
  const auto *self = reinterpret_cast<const DummyCell *>(cell);
  mb.setVTable(&DummyCell::vt);
  mb.addField("x", &self->x_);
  mb.addField("y", &self->y_);
  mb.addField("z", &self->z_);
  mb.addField("sym", &self->sym_);
}

struct DummyArrayCell {
 public:
  static const VTable vt;
  AtomicIfConcurrentGC<std::uint32_t> length_{3};
  GCPointerBase data_[3];

  static void buildMeta(const GCCell *cell, Metadata::Builder &mb);
};

const VTable DummyArrayCell::vt{
    CellKind::DummyObjectKind,
    sizeof(DummyArrayCell)};

static_assert(
    std::is_standard_layout<DummyArrayCell>::value,
    "DummyArrayCell isn't a standard layout, offsetof won't work");

void DummyArrayCell::buildMeta(const GCCell *cell, Metadata::Builder &mb) {
  const auto *self = reinterpret_cast<const DummyArrayCell *>(cell);
  mb.setVTable(&DummyArrayCell::vt);
  mb.addArray(
      "dummystorage", self->data_, &self->length_, sizeof(DummyArrayCell));
}

TEST(MetadataTest, TestNormalFields) {
  static const auto meta =
      buildMetadata(CellKind::UninitializedKind, DummyCell::buildMeta);
  ASSERT_FALSE(meta.array);

  EXPECT_EQ(meta.pointersEnd, 3u);
  EXPECT_STREQ(meta.names[0], "x");
  EXPECT_STREQ(meta.names[1], "y");
  EXPECT_STREQ(meta.names[2], "z");
  EXPECT_EQ(meta.offsets[0], offsetof(DummyCell, x_));
  EXPECT_EQ(meta.offsets[1], offsetof(DummyCell, y_));
  EXPECT_EQ(meta.offsets[2], offsetof(DummyCell, z_));

  EXPECT_EQ(meta.valuesEnd, 3u);
  EXPECT_EQ(meta.smallValuesEnd, 3u);

  EXPECT_EQ(meta.symbolsEnd, 4u);
  EXPECT_STREQ(meta.names[3], "sym");
  EXPECT_EQ(meta.offsets[3], offsetof(DummyCell, sym_));
}

TEST(MetadataTest, TestArray) {
  static const auto meta =
      buildMetadata(CellKind::UninitializedKind, DummyArrayCell::buildMeta);
  ASSERT_TRUE(meta.array);
  auto &array = *(meta.array);
  EXPECT_EQ(array.type, Metadata::ArrayData::ArrayType::Pointer);

  EXPECT_EQ(array.lengthOffset, offsetof(DummyArrayCell, length_));
  EXPECT_EQ(array.startOffset, offsetof(DummyArrayCell, data_));
  EXPECT_EQ(array.stride, sizeof(DummyCell));
}

} // namespace
