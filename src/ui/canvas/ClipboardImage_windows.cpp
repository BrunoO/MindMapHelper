#include "ui/canvas/ClipboardImage.h"

#if defined(_WIN32)
#  include <windows.h>  // NOLINT(llvm-include-order)
#  include <gdiplus.h>
#  include <objidl.h>
#endif

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace mind_map::ui {
#if defined(_WIN32)
namespace {

struct GdiplusStartupGuard {
  GdiplusStartupGuard() {
    Gdiplus::GdiplusStartupInput input;
    available_ = (Gdiplus::GdiplusStartup(&token_, &input, nullptr) == Gdiplus::Ok);
  }

  ~GdiplusStartupGuard() {
    if (available_) {
      Gdiplus::GdiplusShutdown(token_);
    }
  }

  Gdiplus::ULONG_PTR token_ = 0;
  bool available_ = false;
};

const GdiplusStartupGuard& GetGdiplusStartupGuard() {
  static GdiplusStartupGuard guard_;
  return guard_;
}

bool IsGdiplusAvailable() {
  return GetGdiplusStartupGuard().available_;
}

std::optional<CLSID> GetPngEncoderClsid() {
  UINT count = 0U;
  UINT size = 0U;
  if (Gdiplus::GetImageEncodersSize(&count, &size) != Gdiplus::Ok || size == 0U) {
    return std::nullopt;
  }

  std::vector<BYTE> buffer(size);
  const auto* encoders = reinterpret_cast<const Gdiplus::ImageCodecInfo*>(buffer.data());
  if (Gdiplus::GetImageEncoders(count, size, const_cast<Gdiplus::ImageCodecInfo*>(encoders)) != Gdiplus::Ok) {
    return std::nullopt;
  }

  for (UINT i = 0U; i < count; ++i) {
    if (std::wcscmp(encoders[i].MimeType, L"image/png") == 0) {
      return encoders[i].Clsid;
    }
  }
  return std::nullopt;
}

std::optional<HBITMAP> ConvertDibToHBitmap(HGLOBAL dib_handle) {
  const void* dib_data = GlobalLock(dib_handle);
  if (dib_data == nullptr) {
    return std::nullopt;
  }

  const auto dib_size = static_cast<size_t>(GlobalSize(dib_handle));
  if (dib_size < sizeof(BITMAPINFOHEADER)) {
    GlobalUnlock(dib_handle);
    return std::nullopt;
  }

  const auto* header = static_cast<const BITMAPINFOHEADER*>(dib_data);
  if (header->biSize < sizeof(BITMAPINFOHEADER) || header->biSize > dib_size) {
    GlobalUnlock(dib_handle);
    return std::nullopt;
  }

  const bool is_bitmap_info_header = (header->biSize == sizeof(BITMAPINFOHEADER));
  size_t palette_size = 0U;
  if (is_bitmap_info_header) {
    if (header->biBitCount <= 8) {
      const uint32_t colors = header->biClrUsed != 0 ? header->biClrUsed : (1u << header->biBitCount);
      palette_size = static_cast<size_t>(colors) * sizeof(RGBQUAD);
    } else if (header->biCompression == BI_BITFIELDS || header->biCompression == BI_ALPHABITFIELDS) {
      palette_size = 3U * sizeof(DWORD);
    }
  }

  if (header->biSize > dib_size || dib_size - header->biSize < palette_size) {
    GlobalUnlock(dib_handle);
    return std::nullopt;
  }

  const BYTE* bits = static_cast<const BYTE*>(dib_data) + header->biSize + palette_size;
  const auto* bitmap_info = reinterpret_cast<const BITMAPINFO*>(dib_data);

  HBITMAP result = nullptr;
  const HDC dc = GetDC(nullptr);
  if (dc != nullptr) {
    result = CreateDIBitmap(dc, header, CBM_INIT, bits, bitmap_info, DIB_RGB_COLORS);
    ReleaseDC(nullptr, dc);
  }

  if (result == nullptr && dc != nullptr) {
    const HDC compatible_dc = CreateCompatibleDC(nullptr);
    if (compatible_dc != nullptr) {
      result = CreateDIBSection(compatible_dc, bitmap_info, DIB_RGB_COLORS, nullptr, nullptr, 0);
      if (result != nullptr) {
        const int height = std::abs(header->biHeight);
        const int copied = SetDIBits(compatible_dc, result, 0, height, bits, bitmap_info, DIB_RGB_COLORS);
        if (copied != height) {
          DeleteObject(result);
          result = nullptr;
        }
      }
      DeleteDC(compatible_dc);
    }
  }

  GlobalUnlock(dib_handle);
  return result;
}

std::optional<std::string> EncodeBitmapToPng(HBITMAP bitmap_handle) {
  const auto encoder_clsid = GetPngEncoderClsid();
  if (!encoder_clsid) {
    return std::nullopt;
  }

  Gdiplus::Bitmap bitmap(bitmap_handle, nullptr);
  IStream* stream = nullptr;
  if (CreateStreamOnHGlobal(nullptr, TRUE, &stream) != S_OK) {
    return std::nullopt;
  }

  const Gdiplus::Status status = bitmap.Save(stream, &*encoder_clsid, nullptr);
  if (status != Gdiplus::Ok) {
    stream->Release();
    return std::nullopt;
  }

  STATSTG stat = {};
  if (stream->Stat(&stat, STATFLAG_NONAME) != S_OK) {
    stream->Release();
    return std::nullopt;
  }

  LARGE_INTEGER zero = {};
  if (stream->Seek(zero, STREAM_SEEK_SET, nullptr) != S_OK) {
    stream->Release();
    return std::nullopt;
  }

  const auto png_size = static_cast<size_t>(stat.cbSize.QuadPart);
  if (png_size == 0U) {
    stream->Release();
    return std::nullopt;
  }

  std::string png_bytes;
  png_bytes.resize(png_size);
  ULONG read = 0U;
  if (stream->Read(png_bytes.data(), static_cast<ULONG>(png_size), &read) != S_OK || read != png_size) {
    stream->Release();
    return std::nullopt;
  }

  stream->Release();
  return png_bytes;
}

}  // namespace

std::optional<std::string> GetClipboardImagePng() {
  // FIXME: improve diagnostics for paste failures.
  // Currently every clipboard/encoding failure returns nullopt, so callers
  // cannot distinguish "no image available" from "internal conversion failed".
  if (!IsGdiplusAvailable()) {
    return std::nullopt;
  }

  if (OpenClipboard(nullptr) == FALSE) {
    return std::nullopt;
  }

  HBITMAP clipboard_bitmap = static_cast<HBITMAP>(GetClipboardData(CF_BITMAP));
  HBITMAP local_bitmap = nullptr;
  bool bitmap_owned = false;

  if (clipboard_bitmap != nullptr) {
    local_bitmap = clipboard_bitmap;
  } else {
    HANDLE dib_handle = GetClipboardData(CF_DIB);
    if (dib_handle == nullptr) {
#if defined(CF_DIBV5)
      dib_handle = GetClipboardData(CF_DIBV5);
#endif
    }
    if (dib_handle != nullptr) {
      const auto converted = ConvertDibToHBitmap(dib_handle);
      if (converted.has_value()) {
        local_bitmap = *converted;
        bitmap_owned = true;
      }
    }
  }

  CloseClipboard();
  if (local_bitmap == nullptr) {
    return std::nullopt;
  }

  const auto png_bytes = EncodeBitmapToPng(local_bitmap);
  if (bitmap_owned) {
    DeleteObject(local_bitmap);
  }
  return png_bytes;
}

#endif  // defined(_WIN32)

}  // namespace mind_map::ui
