// Copyright 2014 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxcodec/flate/flatemodule.h"

#include <stdint.h>
#include <string.h>

#include <algorithm>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include "core/fxcodec/scanlinedecoder.h"
#include "core/fxcrt/data_vector.h"
#include "core/fxcrt/fixed_zeroed_data_vector.h"
#include "core/fxcrt/fx_extension.h"
#include "core/fxcrt/fx_memory_wrappers.h"
#include "core/fxcrt/fx_safe_types.h"
#include "core/fxcrt/span_util.h"
#include "core/fxge/calculate_pitch.h"
#include "third_party/base/check.h"
#include "third_party/base/containers/span.h"
#include "third_party/base/notreached.h"
#include "third_party/base/numerics/safe_conversions.h"

#if defined(USE_SYSTEM_ZLIB)
#include <zlib.h>
#else
#include "third_party/zlib/zlib.h"
#endif

extern "C" {

static void* my_alloc_func(void* opaque,
                           unsigned int items,
                           unsigned int size) {
  return FX_Alloc2D(uint8_t, items, size);
}

static void my_free_func(void* opaque, void* address) {
  FX_Free(address);
}

}  // extern "C"

namespace fxcodec {

namespace {

static constexpr uint32_t kMaxTotalOutSize = 1024 * 1024 * 1024;  // 1 GiB

uint32_t FlateGetPossiblyTruncatedTotalOut(z_stream* context) {
  return std::min(pdfium::base::saturated_cast<uint32_t>(context->total_out),
                  kMaxTotalOutSize);
}

uint32_t FlateGetPossiblyTruncatedTotalIn(z_stream* context) {
  return pdfium::base::saturated_cast<uint32_t>(context->total_in);
}

bool FlateCompress(unsigned char* dest_buf,
                   unsigned long* dest_size,
                   const unsigned char* src_buf,
                   unsigned long src_size) {
  return compress(dest_buf, dest_size, src_buf, src_size) == Z_OK;
}

z_stream* FlateInit() {
  z_stream* p = FX_Alloc(z_stream, 1);
  p->zalloc = my_alloc_func;
  p->zfree = my_free_func;
  inflateInit(p);
  return p;
}

void FlateInput(z_stream* context, pdfium::span<const uint8_t> src_buf) {
  context->next_in = const_cast<unsigned char*>(src_buf.data());
  context->avail_in = static_cast<uint32_t>(src_buf.size());
}

uint32_t FlateOutput(z_stream* context,
                     unsigned char* dest_buf,
                     uint32_t dest_size) {
  context->next_out = dest_buf;
  context->avail_out = dest_size;
  uint32_t pre_pos = FlateGetPossiblyTruncatedTotalOut(context);
  int ret = inflate(static_cast<z_stream*>(context), Z_SYNC_FLUSH);

  uint32_t post_pos = FlateGetPossiblyTruncatedTotalOut(context);
  DCHECK(post_pos >= pre_pos);

  uint32_t written = post_pos - pre_pos;
  if (written < dest_size)
    memset(dest_buf + written, '\0', dest_size - written);

  return ret;
}

uint32_t FlateGetAvailOut(z_stream* context) {
  return context->avail_out;
}

void FlateEnd(z_stream* context) {
  inflateEnd(context);
  FX_Free(context);
}

// For use with std::unique_ptr<z_stream>.
struct FlateDeleter {
  inline void operator()(z_stream* context) { FlateEnd(context); }
};

class CLZWDecoder {
 public:
  CLZWDecoder(pdfium::span<const uint8_t> src_span, bool early_change);

  bool Decode();
  uint32_t GetSrcSize() const { return (src_bit_pos_ + 7) / 8; }
  uint32_t GetDestSize() const { return dest_byte_pos_; }
  std::unique_ptr<uint8_t, FxFreeDeleter> TakeDestBuf() {
    return std::move(dest_buf_);
  }

 private:
  void AddCode(uint32_t prefix_code, uint8_t append_char);
  void DecodeString(uint32_t code);
  void ExpandDestBuf(uint32_t additional_size);

  pdfium::span<const uint8_t> const src_span_;
  std::unique_ptr<uint8_t, FxFreeDeleter> dest_buf_;
  uint32_t src_bit_pos_ = 0;
  uint32_t dest_buf_size_ = 0;  // Actual allocated size.
  uint32_t dest_byte_pos_ = 0;  // Size used.
  uint32_t stack_len_ = 0;
  FixedZeroedDataVector<uint8_t> decode_stack_;
  const uint8_t early_change_;
  uint8_t code_len_ = 9;
  uint32_t current_code_ = 0;
  FixedZeroedDataVector<uint32_t> codes_;
};

CLZWDecoder::CLZWDecoder(pdfium::span<const uint8_t> src_span,
                         bool early_change)
    : src_span_(src_span),
      decode_stack_(4000),
      early_change_(early_change ? 1 : 0),
      codes_(5021) {}

void CLZWDecoder::AddCode(uint32_t prefix_code, uint8_t append_char) {
  if (current_code_ + early_change_ == 4094)
    return;

  pdfium::span<uint32_t> codes_span = codes_.writable_span();
  codes_span[current_code_++] = (prefix_code << 16) | append_char;
  if (current_code_ + early_change_ == 512 - 258)
    code_len_ = 10;
  else if (current_code_ + early_change_ == 1024 - 258)
    code_len_ = 11;
  else if (current_code_ + early_change_ == 2048 - 258)
    code_len_ = 12;
}

void CLZWDecoder::DecodeString(uint32_t code) {
  pdfium::span<uint8_t> decode_span = decode_stack_.writable_span();
  pdfium::span<const uint32_t> codes_span = codes_.span();
  while (true) {
    int index = code - 258;
    if (index < 0 || static_cast<uint32_t>(index) >= current_code_)
      break;

    uint32_t data = codes_span[index];
    if (stack_len_ >= decode_span.size())
      return;

    decode_span[stack_len_++] = static_cast<uint8_t>(data);
    code = data >> 16;
  }
  if (stack_len_ >= decode_span.size())
    return;

  decode_span[stack_len_++] = static_cast<uint8_t>(code);
}

void CLZWDecoder::ExpandDestBuf(uint32_t additional_size) {
  FX_SAFE_UINT32 new_size = std::max(dest_buf_size_ / 2, additional_size);
  new_size += dest_buf_size_;
  if (!new_size.IsValid()) {
    dest_buf_.reset();
    return;
  }

  dest_buf_size_ = new_size.ValueOrDie();
  dest_buf_.reset(FX_Realloc(uint8_t, dest_buf_.release(), dest_buf_size_));
}

bool CLZWDecoder::Decode() {
  pdfium::span<uint8_t> decode_span = decode_stack_.writable_span();
  uint32_t old_code = 0xFFFFFFFF;
  uint8_t last_char = 0;

  // In one PDF test set, 40% of Decode() calls did not need to realloc with
  // this size.
  dest_buf_size_ = 512;
  dest_buf_.reset(FX_Alloc(uint8_t, dest_buf_size_));
  while (true) {
    if (src_bit_pos_ + code_len_ > src_span_.size() * 8)
      break;

    int byte_pos = src_bit_pos_ / 8;
    int bit_pos = src_bit_pos_ % 8;
    uint8_t bit_left = code_len_;
    uint32_t code = 0;
    if (bit_pos) {
      bit_left -= 8 - bit_pos;
      code = (src_span_[byte_pos++] & ((1 << (8 - bit_pos)) - 1)) << bit_left;
    }
    if (bit_left < 8) {
      code |= src_span_[byte_pos] >> (8 - bit_left);
    } else {
      bit_left -= 8;
      code |= src_span_[byte_pos++] << bit_left;
      if (bit_left)
        code |= src_span_[byte_pos] >> (8 - bit_left);
    }
    src_bit_pos_ += code_len_;

    if (code < 256) {
      if (dest_byte_pos_ >= dest_buf_size_) {
        ExpandDestBuf(dest_byte_pos_ - dest_buf_size_ + 1);
        if (!dest_buf_)
          return false;
      }

      dest_buf_.get()[dest_byte_pos_] = (uint8_t)code;
      dest_byte_pos_++;
      last_char = (uint8_t)code;
      if (old_code != 0xFFFFFFFF)
        AddCode(old_code, last_char);
      old_code = code;
      continue;
    }
    if (code == 256) {
      code_len_ = 9;
      current_code_ = 0;
      old_code = 0xFFFFFFFF;
      continue;
    }
    if (code == 257)
      break;

    // Case where |code| is 258 or greater.
    if (old_code == 0xFFFFFFFF)
      return false;

    DCHECK(old_code < 256 || old_code >= 258);
    stack_len_ = 0;
    if (code - 258 >= current_code_) {
      if (stack_len_ < decode_stack_.size())
        decode_span[stack_len_++] = last_char;
      DecodeString(old_code);
    } else {
      DecodeString(code);
    }

    FX_SAFE_UINT32 safe_required_size = dest_byte_pos_;
    safe_required_size += stack_len_;
    if (!safe_required_size.IsValid())
      return false;

    uint32_t required_size = safe_required_size.ValueOrDie();
    if (required_size > dest_buf_size_) {
      ExpandDestBuf(required_size - dest_buf_size_);
      if (!dest_buf_)
        return false;
    }

    for (uint32_t i = 0; i < stack_len_; i++)
      dest_buf_.get()[dest_byte_pos_ + i] = decode_span[stack_len_ - i - 1];
    dest_byte_pos_ += stack_len_;
    last_char = decode_span[stack_len_ - 1];
    if (old_code >= 258 && old_code - 258 >= current_code_)
      break;

    AddCode(old_code, last_char);
    old_code = code;
  }
  return dest_byte_pos_ != 0;
}

uint8_t PathPredictor(int a, int b, int c) {
  int p = a + b - c;
  int pa = abs(p - a);
  int pb = abs(p - b);
  int pc = abs(p - c);
  if (pa <= pb && pa <= pc)
    return (uint8_t)a;
  if (pb <= pc)
    return (uint8_t)b;
  return (uint8_t)c;
}

void PNG_PredictLine(pdfium::span<uint8_t> dest_span,
                     pdfium::span<const uint8_t> src_span,
                     pdfium::span<const uint8_t> last_span,
                     int bpc,
                     int nColors,
                     int nPixels) {
  uint8_t* pDestData = dest_span.data();
  const uint8_t* pSrcData = src_span.data();
  const uint8_t* pLastLine = last_span.data();
  const uint32_t row_size = fxge::CalculatePitch8OrDie(bpc, nColors, nPixels);
  const uint32_t BytesPerPixel = (bpc * nColors + 7) / 8;
  uint8_t tag = pSrcData[0];
  if (tag == 0) {
    memmove(pDestData, pSrcData + 1, row_size);
    return;
  }
  for (uint32_t byte = 0; byte < row_size; ++byte) {
    uint8_t raw_byte = pSrcData[byte + 1];
    switch (tag) {
      case 1: {
        uint8_t left = 0;
        if (byte >= BytesPerPixel) {
          left = pDestData[byte - BytesPerPixel];
        }
        pDestData[byte] = raw_byte + left;
        break;
      }
      case 2: {
        uint8_t up = 0;
        if (pLastLine) {
          up = pLastLine[byte];
        }
        pDestData[byte] = raw_byte + up;
        break;
      }
      case 3: {
        uint8_t left = 0;
        if (byte >= BytesPerPixel) {
          left = pDestData[byte - BytesPerPixel];
        }
        uint8_t up = 0;
        if (pLastLine) {
          up = pLastLine[byte];
        }
        pDestData[byte] = raw_byte + (up + left) / 2;
        break;
      }
      case 4: {
        uint8_t left = 0;
        if (byte >= BytesPerPixel) {
          left = pDestData[byte - BytesPerPixel];
        }
        uint8_t up = 0;
        if (pLastLine) {
          up = pLastLine[byte];
        }
        uint8_t upper_left = 0;
        if (byte >= BytesPerPixel && pLastLine) {
          upper_left = pLastLine[byte - BytesPerPixel];
        }
        pDestData[byte] = raw_byte + PathPredictor(left, up, upper_left);
        break;
      }
      default:
        pDestData[byte] = raw_byte;
        break;
    }
  }
}

bool PNG_Predictor(int Colors,
                   int BitsPerComponent,
                   int Columns,
                   std::unique_ptr<uint8_t, FxFreeDeleter>* data_buf,
                   uint32_t* data_size) {
  const uint32_t row_size =
      fxge::CalculatePitch8(BitsPerComponent, Colors, Columns).value_or(0);
  if (row_size == 0) {
    return false;
  }

  const uint32_t src_row_size = row_size + 1;
  if (src_row_size == 0) {
    // Avoid divide by 0.
    return false;
  }
  const uint32_t row_count = (*data_size + row_size) / src_row_size;
  if (row_count == 0) {
    return false;
  }

  const uint32_t last_row_size = *data_size % src_row_size;
  std::unique_ptr<uint8_t, FxFreeDeleter> dest_buf(
      FX_Alloc2D(uint8_t, row_size, row_count));
  uint32_t byte_cnt = 0;
  const uint8_t* pSrcData = data_buf->get();
  uint8_t* pDestData = dest_buf.get();
  const uint8_t* pPrevDestData = nullptr;
  for (uint32_t row = 0; row < row_count; row++) {
    uint8_t tag = pSrcData[0];
    byte_cnt++;
    if (tag == 0) {
      uint32_t move_size = row_size;
      if ((row + 1) * (move_size + 1) > *data_size) {
        move_size = last_row_size - 1;
      }
      memcpy(pDestData, pSrcData + 1, move_size);
      pSrcData += move_size + 1;
      pPrevDestData = pDestData;
      pDestData += move_size;
      byte_cnt += move_size;
      continue;
    }

    const uint32_t BytesPerPixel = (Colors * BitsPerComponent + 7) / 8;
    for (uint32_t byte = 0; byte < row_size && byte_cnt < *data_size;
         ++byte, ++byte_cnt) {
      uint8_t raw_byte = pSrcData[byte + 1];
      switch (tag) {
        case 1: {
          uint8_t left = 0;
          if (byte >= BytesPerPixel) {
            left = pDestData[byte - BytesPerPixel];
          }
          pDestData[byte] = raw_byte + left;
          break;
        }
        case 2: {
          uint8_t up = 0;
          if (pPrevDestData) {
            up = pPrevDestData[byte];
          }
          pDestData[byte] = raw_byte + up;
          break;
        }
        case 3: {
          uint8_t left = 0;
          if (byte >= BytesPerPixel) {
            left = pDestData[byte - BytesPerPixel];
          }
          uint8_t up = 0;
          if (pPrevDestData) {
            up = pPrevDestData[byte];
          }
          pDestData[byte] = raw_byte + (up + left) / 2;
          break;
        }
        case 4: {
          uint8_t left = 0;
          if (byte >= BytesPerPixel) {
            left = pDestData[byte - BytesPerPixel];
          }
          uint8_t up = 0;
          if (pPrevDestData) {
            up = pPrevDestData[byte];
          }
          uint8_t upper_left = 0;
          if (pPrevDestData && byte >= BytesPerPixel) {
            upper_left = pPrevDestData[byte - BytesPerPixel];
          }
          pDestData[byte] = raw_byte + PathPredictor(left, up, upper_left);
          break;
        }
        default:
          pDestData[byte] = raw_byte;
          break;
      }
    }
    pSrcData += src_row_size;
    pPrevDestData = pDestData;
    pDestData += row_size;
  }
  *data_buf = std::move(dest_buf);
  *data_size = row_size * row_count -
               (last_row_size > 0 ? (src_row_size - last_row_size) : 0);
  return true;
}

void TIFF_PredictLine(uint8_t* dest_buf,
                      uint32_t row_size,
                      int BitsPerComponent,
                      int Colors,
                      int Columns) {
  if (BitsPerComponent == 1) {
    int row_bits = std::min(BitsPerComponent * Colors * Columns,
                            pdfium::base::checked_cast<int>(row_size * 8));
    int index_pre = 0;
    int col_pre = 0;
    for (int i = 1; i < row_bits; i++) {
      int col = i % 8;
      int index = i / 8;
      if (((dest_buf[index] >> (7 - col)) & 1) ^
          ((dest_buf[index_pre] >> (7 - col_pre)) & 1)) {
        dest_buf[index] |= 1 << (7 - col);
      } else {
        dest_buf[index] &= ~(1 << (7 - col));
      }
      index_pre = index;
      col_pre = col;
    }
    return;
  }
  int BytesPerPixel = BitsPerComponent * Colors / 8;
  if (BitsPerComponent == 16) {
    for (uint32_t i = BytesPerPixel; i + 1 < row_size; i += 2) {
      uint16_t pixel =
          (dest_buf[i - BytesPerPixel] << 8) | dest_buf[i - BytesPerPixel + 1];
      pixel += (dest_buf[i] << 8) | dest_buf[i + 1];
      dest_buf[i] = pixel >> 8;
      dest_buf[i + 1] = (uint8_t)pixel;
    }
  } else {
    for (uint32_t i = BytesPerPixel; i < row_size; i++) {
      dest_buf[i] += dest_buf[i - BytesPerPixel];
    }
  }
}

bool TIFF_Predictor(int Colors,
                    int BitsPerComponent,
                    int Columns,
                    std::unique_ptr<uint8_t, FxFreeDeleter>* data_buf,
                    uint32_t* data_size) {
  uint32_t row_size =
      fxge::CalculatePitch8(BitsPerComponent, Colors, Columns).value_or(0);
  if (row_size == 0)
    return false;

  const uint32_t row_count = (*data_size + row_size - 1) / row_size;
  const uint32_t last_row_size = *data_size % row_size;
  for (uint32_t row = 0; row < row_count; row++) {
    uint8_t* scan_line = data_buf->get() + row * row_size;
    if ((row + 1) * row_size > *data_size) {
      row_size = last_row_size;
    }
    TIFF_PredictLine(scan_line, row_size, BitsPerComponent, Colors, Columns);
  }
  return true;
}

void FlateUncompress(pdfium::span<const uint8_t> src_buf,
                     uint32_t orig_size,
                     std::unique_ptr<uint8_t, FxFreeDeleter>* dest_buf,
                     uint32_t* dest_size,
                     uint32_t* offset) {
  dest_buf->reset();
  *dest_size = 0;

  std::unique_ptr<z_stream, FlateDeleter> context(FlateInit());
  if (!context)
    return;

  FlateInput(context.get(), src_buf);

  const uint32_t kMaxInitialAllocSize = 10000000;
  uint32_t guess_size =
      orig_size ? orig_size
                : pdfium::base::checked_cast<uint32_t>(src_buf.size() * 2);
  guess_size = std::min(guess_size, kMaxInitialAllocSize);

  uint32_t buf_size = guess_size;
  uint32_t last_buf_size = buf_size;
  std::unique_ptr<uint8_t, FxFreeDeleter> guess_buf(
      FX_Alloc(uint8_t, guess_size + 1));
  guess_buf.get()[guess_size] = '\0';

  std::vector<std::unique_ptr<uint8_t, FxFreeDeleter>> result_tmp_bufs;
  {
    std::unique_ptr<uint8_t, FxFreeDeleter> cur_buf = std::move(guess_buf);
    while (true) {
      uint32_t ret = FlateOutput(context.get(), cur_buf.get(), buf_size);
      uint32_t avail_buf_size = FlateGetAvailOut(context.get());
      if (ret != Z_OK || avail_buf_size != 0) {
        last_buf_size = buf_size - avail_buf_size;
        result_tmp_bufs.push_back(std::move(cur_buf));
        break;
      }
      result_tmp_bufs.push_back(std::move(cur_buf));
      cur_buf.reset(FX_Alloc(uint8_t, buf_size + 1));
      cur_buf.get()[buf_size] = '\0';
    }
  }

  // The TotalOut size returned from the library may not be big enough to
  // handle the content the library returns. We can only handle items
  // up to 4GB in size.
  *dest_size = FlateGetPossiblyTruncatedTotalOut(context.get());
  *offset = FlateGetPossiblyTruncatedTotalIn(context.get());
  if (result_tmp_bufs.size() == 1) {
    *dest_buf = std::move(result_tmp_bufs[0]);
    return;
  }

  std::unique_ptr<uint8_t, FxFreeDeleter> result_buf(
      FX_Alloc(uint8_t, *dest_size));
  uint32_t result_pos = 0;
  uint32_t remaining = *dest_size;
  for (size_t i = 0; i < result_tmp_bufs.size(); i++) {
    std::unique_ptr<uint8_t, FxFreeDeleter> tmp_buf =
        std::move(result_tmp_bufs[i]);
    uint32_t tmp_buf_size = buf_size;
    if (i == result_tmp_bufs.size() - 1)
      tmp_buf_size = last_buf_size;

    uint32_t cp_size = std::min(tmp_buf_size, remaining);
    memcpy(result_buf.get() + result_pos, tmp_buf.get(), cp_size);
    result_pos += cp_size;
    remaining -= cp_size;
  }
  *dest_buf = std::move(result_buf);
}

enum class PredictorType : uint8_t { kNone, kFlate, kPng };
static PredictorType GetPredictor(int predictor) {
  if (predictor >= 10)
    return PredictorType::kPng;
  if (predictor == 2)
    return PredictorType::kFlate;
  return PredictorType::kNone;
}

class FlateScanlineDecoder : public ScanlineDecoder {
 public:
  FlateScanlineDecoder(pdfium::span<const uint8_t> src_span,
                       int width,
                       int height,
                       int nComps,
                       int bpc);
  ~FlateScanlineDecoder() override;

  // ScanlineDecoder:
  bool Rewind() override;
  pdfium::span<uint8_t> GetNextLine() override;
  uint32_t GetSrcOffset() override;

 protected:
  std::unique_ptr<z_stream, FlateDeleter> m_pFlate;
  const pdfium::span<const uint8_t> m_SrcBuf;
  DataVector<uint8_t> m_Scanline;
};

FlateScanlineDecoder::FlateScanlineDecoder(pdfium::span<const uint8_t> src_span,
                                           int width,
                                           int height,
                                           int nComps,
                                           int bpc)
    : ScanlineDecoder(width,
                      height,
                      width,
                      height,
                      nComps,
                      bpc,
                      fxge::CalculatePitch8OrDie(bpc, nComps, width)),
      m_SrcBuf(src_span),
      m_Scanline(m_Pitch) {}

FlateScanlineDecoder::~FlateScanlineDecoder() {
  // Span in superclass can't outlive our buffer.
  m_pLastScanline = pdfium::span<uint8_t>();
}

bool FlateScanlineDecoder::Rewind() {
  m_pFlate.reset(FlateInit());
  if (!m_pFlate)
    return false;

  FlateInput(m_pFlate.get(), m_SrcBuf);
  return true;
}

pdfium::span<uint8_t> FlateScanlineDecoder::GetNextLine() {
  FlateOutput(m_pFlate.get(), m_Scanline.data(), m_Pitch);
  return m_Scanline;
}

uint32_t FlateScanlineDecoder::GetSrcOffset() {
  return FlateGetPossiblyTruncatedTotalIn(m_pFlate.get());
}

class FlatePredictorScanlineDecoder final : public FlateScanlineDecoder {
 public:
  FlatePredictorScanlineDecoder(pdfium::span<const uint8_t> src_span,
                                int width,
                                int height,
                                int comps,
                                int bpc,
                                PredictorType predictor,
                                int Colors,
                                int BitsPerComponent,
                                int Columns);
  ~FlatePredictorScanlineDecoder() override;

  // ScanlineDecoder:
  bool Rewind() override;
  pdfium::span<uint8_t> GetNextLine() override;

 private:
  void GetNextLineWithPredictedPitch();
  void GetNextLineWithoutPredictedPitch();

  const PredictorType m_Predictor;
  int m_Colors = 0;
  int m_BitsPerComponent = 0;
  int m_Columns = 0;
  uint32_t m_PredictPitch = 0;
  size_t m_LeftOver = 0;
  DataVector<uint8_t> m_LastLine;
  DataVector<uint8_t> m_PredictBuffer;
  DataVector<uint8_t> m_PredictRaw;
};

FlatePredictorScanlineDecoder::FlatePredictorScanlineDecoder(
    pdfium::span<const uint8_t> src_span,
    int width,
    int height,
    int comps,
    int bpc,
    PredictorType predictor,
    int Colors,
    int BitsPerComponent,
    int Columns)
    : FlateScanlineDecoder(src_span, width, height, comps, bpc),
      m_Predictor(predictor) {
  DCHECK(m_Predictor != PredictorType::kNone);
  if (BitsPerComponent * Colors * Columns == 0) {
    BitsPerComponent = m_bpc;
    Colors = m_nComps;
    Columns = m_OrigWidth;
  }
  m_Colors = Colors;
  m_BitsPerComponent = BitsPerComponent;
  m_Columns = Columns;
  m_PredictPitch =
      fxge::CalculatePitch8OrDie(m_BitsPerComponent, m_Colors, m_Columns);
  m_LastLine.resize(m_PredictPitch);
  m_PredictBuffer.resize(m_PredictPitch);
  m_PredictRaw.resize(m_PredictPitch + 1);
}

FlatePredictorScanlineDecoder::~FlatePredictorScanlineDecoder() {
  // Span in superclass can't outlive our buffer.
  m_pLastScanline = pdfium::span<uint8_t>();
}

bool FlatePredictorScanlineDecoder::Rewind() {
  if (!FlateScanlineDecoder::Rewind())
    return false;

  m_LeftOver = 0;
  return true;
}

pdfium::span<uint8_t> FlatePredictorScanlineDecoder::GetNextLine() {
  if (m_Pitch == m_PredictPitch)
    GetNextLineWithPredictedPitch();
  else
    GetNextLineWithoutPredictedPitch();
  return m_Scanline;
}

void FlatePredictorScanlineDecoder::GetNextLineWithPredictedPitch() {
  switch (m_Predictor) {
    case PredictorType::kPng:
      FlateOutput(m_pFlate.get(), m_PredictRaw.data(), m_PredictPitch + 1);
      PNG_PredictLine(m_Scanline, m_PredictRaw, m_LastLine, m_BitsPerComponent,
                      m_Colors, m_Columns);
      memcpy(m_LastLine.data(), m_Scanline.data(), m_PredictPitch);
      break;
    case PredictorType::kFlate:
      FlateOutput(m_pFlate.get(), m_Scanline.data(), m_Pitch);
      TIFF_PredictLine(m_Scanline.data(), m_PredictPitch, m_bpc, m_nComps,
                       m_OutputWidth);
      break;
    case PredictorType::kNone:
      NOTREACHED_NORETURN();
  }
}

void FlatePredictorScanlineDecoder::GetNextLineWithoutPredictedPitch() {
  size_t bytes_to_go = m_Pitch;
  size_t read_leftover = m_LeftOver > bytes_to_go ? bytes_to_go : m_LeftOver;
  if (read_leftover) {
    memcpy(m_Scanline.data(), &m_PredictBuffer[m_PredictPitch - m_LeftOver],
           read_leftover);
    m_LeftOver -= read_leftover;
    bytes_to_go -= read_leftover;
  }
  while (bytes_to_go) {
    switch (m_Predictor) {
      case PredictorType::kPng:
        FlateOutput(m_pFlate.get(), m_PredictRaw.data(), m_PredictPitch + 1);
        PNG_PredictLine(m_PredictBuffer, m_PredictRaw, m_LastLine,
                        m_BitsPerComponent, m_Colors, m_Columns);
        memcpy(m_LastLine.data(), m_PredictBuffer.data(), m_PredictPitch);
        break;
      case PredictorType::kFlate:
        FlateOutput(m_pFlate.get(), m_PredictBuffer.data(), m_PredictPitch);
        TIFF_PredictLine(m_PredictBuffer.data(), m_PredictPitch,
                         m_BitsPerComponent, m_Colors, m_Columns);
        break;
      case PredictorType::kNone:
        NOTREACHED_NORETURN();
    }
    size_t read_bytes =
        m_PredictPitch > bytes_to_go ? bytes_to_go : m_PredictPitch;
    fxcrt::spancpy(pdfium::make_span(m_Scanline).subspan(m_Pitch - bytes_to_go),
                   pdfium::make_span(m_PredictBuffer).first(read_bytes));
    m_LeftOver += m_PredictPitch - read_bytes;
    bytes_to_go -= read_bytes;
  }
}

}  // namespace

// static
std::unique_ptr<ScanlineDecoder> FlateModule::CreateDecoder(
    pdfium::span<const uint8_t> src_span,
    int width,
    int height,
    int nComps,
    int bpc,
    int predictor,
    int Colors,
    int BitsPerComponent,
    int Columns) {
  PredictorType predictor_type = GetPredictor(predictor);
  if (predictor_type == PredictorType::kNone) {
    return std::make_unique<FlateScanlineDecoder>(src_span, width, height,
                                                  nComps, bpc);
  }
  return std::make_unique<FlatePredictorScanlineDecoder>(
      src_span, width, height, nComps, bpc, predictor_type, Colors,
      BitsPerComponent, Columns);
}

// static
uint32_t FlateModule::FlateOrLZWDecode(
    bool bLZW,
    pdfium::span<const uint8_t> src_span,
    bool bEarlyChange,
    int predictor,
    int Colors,
    int BitsPerComponent,
    int Columns,
    uint32_t estimated_size,
    std::unique_ptr<uint8_t, FxFreeDeleter>* dest_buf,
    uint32_t* dest_size) {
  dest_buf->reset();
  uint32_t offset = 0;
  PredictorType predictor_type = GetPredictor(predictor);

  if (bLZW) {
    auto decoder = std::make_unique<CLZWDecoder>(src_span, bEarlyChange);
    if (!decoder->Decode())
      return FX_INVALID_OFFSET;

    offset = decoder->GetSrcSize();
    *dest_size = decoder->GetDestSize();
    *dest_buf = decoder->TakeDestBuf();
  } else {
    FlateUncompress(src_span, estimated_size, dest_buf, dest_size, &offset);
  }

  bool ret = false;
  switch (predictor_type) {
    case PredictorType::kNone:
      return offset;
    case PredictorType::kPng:
      ret =
          PNG_Predictor(Colors, BitsPerComponent, Columns, dest_buf, dest_size);
      break;
    case PredictorType::kFlate:
      ret = TIFF_Predictor(Colors, BitsPerComponent, Columns, dest_buf,
                           dest_size);
      break;
  }
  return ret ? offset : FX_INVALID_OFFSET;
}

// static
DataVector<uint8_t> FlateModule::Encode(pdfium::span<const uint8_t> src_span) {
  const unsigned long src_size =
      pdfium::base::checked_cast<unsigned long>(src_span.size());
  pdfium::base::CheckedNumeric<unsigned long> safe_dest_size = src_size;
  safe_dest_size += src_size / 1000;
  safe_dest_size += 12;
  unsigned long dest_size = safe_dest_size.ValueOrDie();
  DataVector<uint8_t> dest_buf(dest_size);
  if (!FlateCompress(dest_buf.data(), &dest_size, src_span.data(), src_size))
    return {};

  dest_buf.resize(pdfium::base::checked_cast<size_t>(dest_size));
  return dest_buf;
}

}  // namespace fxcodec
