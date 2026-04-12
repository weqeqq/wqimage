#include <gtest/gtest.h>
#include <weqeqq/image/decoding.h>
#include <weqeqq/image/encoding.h>

#include <cstdint>
#include <filesystem>
#include <vector>

namespace weqeqq::image {
namespace {

TEST(CodecErrorTest, DecodeUnsupportedFormatExposesStructuredMetadata) {
  const std::vector<std::uint8_t> data = {0x00, 0x11, 0x22, 0x33};

  try {
    (void)Decode(data, Color::kRgb);
    FAIL() << "Expected DecodeUnsupportedFormatError";
  } catch (const DecodeUnsupportedFormatError& error) {
    EXPECT_EQ(error.TypedCode(), ErrorCode::kDecodeUnsupportedFormat);

    const auto* bytes = error.FindField("input_bytes");
    ASSERT_NE(bytes, nullptr);
    EXPECT_EQ(bytes->value, "4");
    EXPECT_FALSE(error.Details().empty());
    EXPECT_FALSE(error.Hint().empty());
  } catch (...) {
    FAIL() << "Expected DecodeUnsupportedFormatError";
  }
}

TEST(CodecErrorTest, DecodeInputReadErrorExposesStructuredMetadata) {
  const auto filename = std::filesystem::temp_directory_path() /
                        "wqimage-tests-missing-input" / "missing.png";
  std::filesystem::remove_all(filename.parent_path());

  try {
    (void)Decode(filename, Color::kRgb);
    FAIL() << "Expected DecodeInputReadError";
  } catch (const DecodeInputReadError& error) {
    EXPECT_EQ(error.TypedCode(), ErrorCode::kDecodeInputRead);

    const auto* path = error.FindField("filename");
    ASSERT_NE(path, nullptr);
    EXPECT_EQ(path->value, filename.string());
    EXPECT_FALSE(error.Details().empty());
    EXPECT_FALSE(error.Hint().empty());
  } catch (...) {
    FAIL() << "Expected DecodeInputReadError";
  }
}

TEST(CodecErrorTest, DecodePngErrorExposesStructuredMetadata) {
  const std::vector<std::uint8_t> data = {
      0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A,
  };

  try {
    (void)Decode(data, Color::kRgb);
    FAIL() << "Expected DecodePngError";
  } catch (const DecodePngError& error) {
    EXPECT_EQ(error.TypedCode(), ErrorCode::kDecodePng);

    const auto* bytes = error.FindField("input_bytes");
    const auto* cause = error.FindField("cause");
    ASSERT_NE(bytes, nullptr);
    ASSERT_NE(cause, nullptr);
    EXPECT_EQ(bytes->value, "8");
    EXPECT_FALSE(cause->value.empty());
    EXPECT_FALSE(error.Details().empty());
    EXPECT_FALSE(error.Hint().empty());
  } catch (...) {
    FAIL() << "Expected DecodePngError";
  }
}

TEST(CodecErrorTest, EncodeUnsupportedPathExtensionExposesStructuredMetadata) {
  Buffer buffer(1, 1, Color::kRgba);

  try {
    Encode(buffer, std::filesystem::path("image.jpg"));
    FAIL() << "Expected EncodeUnsupportedPathExtensionError";
  } catch (const EncodeUnsupportedPathExtensionError& error) {
    EXPECT_EQ(error.TypedCode(), ErrorCode::kEncodeUnsupportedPathExtension);

    const auto* extension = error.FindField("extension");
    ASSERT_NE(extension, nullptr);
    EXPECT_EQ(extension->value, ".jpg");
    EXPECT_FALSE(error.Details().empty());
    EXPECT_FALSE(error.Hint().empty());
  } catch (...) {
    FAIL() << "Expected EncodeUnsupportedPathExtensionError";
  }
}

TEST(CodecErrorTest, EncodeUnsupportedFormatExposesStructuredMetadata) {
  Buffer buffer(1, 1, Color::kRgba);

  try {
    (void)Encode(buffer, static_cast<Format>(255));
    FAIL() << "Expected EncodeUnsupportedFormatError";
  } catch (const EncodeUnsupportedFormatError& error) {
    EXPECT_EQ(error.TypedCode(), ErrorCode::kEncodeUnsupportedFormat);

    const auto* format = error.FindField("format");
    ASSERT_NE(format, nullptr);
    EXPECT_EQ(format->value, "255");
    EXPECT_FALSE(error.Details().empty());
    EXPECT_FALSE(error.Hint().empty());
  } catch (...) {
    FAIL() << "Expected EncodeUnsupportedFormatError";
  }
}

TEST(CodecErrorTest, EncodeOutputWriteErrorExposesStructuredMetadata) {
  Buffer buffer(1, 1, Color::kRgba);
  const auto filename = std::filesystem::temp_directory_path() /
                        "wqimage-tests-missing-output-parent" / "nested" /
                        "image.png";
  std::filesystem::remove_all(filename.parent_path().parent_path());

  try {
    Encode(buffer, Format::kPng, filename);
    FAIL() << "Expected EncodeOutputWriteError";
  } catch (const EncodeOutputWriteError& error) {
    EXPECT_EQ(error.TypedCode(), ErrorCode::kEncodeOutputWrite);

    const auto* path = error.FindField("filename");
    ASSERT_NE(path, nullptr);
    EXPECT_EQ(path->value, filename.string());
    EXPECT_FALSE(error.Details().empty());
    EXPECT_FALSE(error.Hint().empty());
  } catch (...) {
    FAIL() << "Expected EncodeOutputWriteError";
  }
}

}  // namespace
}  // namespace weqeqq::image
