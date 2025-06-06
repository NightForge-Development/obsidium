// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfapi/page/cpdf_textstate.h"

#include <math.h>

#include <utility>

#include "core/fpdfapi/font/cpdf_font.h"
#include "core/fpdfapi/page/cpdf_docpagedata.h"

CPDF_TextState::CPDF_TextState() = default;

CPDF_TextState::~CPDF_TextState() = default;

void CPDF_TextState::Emplace() {
  m_Ref.Emplace();
}

RetainPtr<CPDF_Font> CPDF_TextState::GetFont() const {
  return m_Ref.GetObject()->m_pFont;
}

void CPDF_TextState::SetFont(RetainPtr<CPDF_Font> pFont) {
  m_Ref.GetPrivateCopy()->SetFont(std::move(pFont));
}

float CPDF_TextState::GetFontSize() const {
  return m_Ref.GetObject()->m_FontSize;
}

void CPDF_TextState::SetFontSize(float size) {
  if (!m_Ref || GetFontSize() != size) {
    m_Ref.GetPrivateCopy()->m_FontSize = size;
  }
}

pdfium::span<const float> CPDF_TextState::GetMatrix() const {
  return m_Ref.GetObject()->m_Matrix;
}

pdfium::span<float> CPDF_TextState::GetMutableMatrix() {
  return m_Ref.GetPrivateCopy()->m_Matrix;
}

float CPDF_TextState::GetCharSpace() const {
  return m_Ref.GetObject()->m_CharSpace;
}

void CPDF_TextState::SetCharSpace(float sp) {
  if (!m_Ref || GetCharSpace() != sp) {
    m_Ref.GetPrivateCopy()->m_CharSpace = sp;
  }
}

float CPDF_TextState::GetWordSpace() const {
  return m_Ref.GetObject()->m_WordSpace;
}

void CPDF_TextState::SetWordSpace(float sp) {
  if (!m_Ref || GetWordSpace() != sp) {
    m_Ref.GetPrivateCopy()->m_WordSpace = sp;
  }
}

float CPDF_TextState::GetFontSizeH() const {
  return m_Ref.GetObject()->GetFontSizeH();
}

TextRenderingMode CPDF_TextState::GetTextMode() const {
  return m_Ref.GetObject()->m_TextMode;
}

void CPDF_TextState::SetTextMode(TextRenderingMode mode) {
  if (!m_Ref || GetTextMode() != mode) {
    m_Ref.GetPrivateCopy()->m_TextMode = mode;
  }
}

pdfium::span<const float> CPDF_TextState::GetCTM() const {
  return m_Ref.GetObject()->m_CTM;
}

pdfium::span<float> CPDF_TextState::GetMutableCTM() {
  return m_Ref.GetPrivateCopy()->m_CTM;
}

CPDF_TextState::TextData::TextData() = default;

CPDF_TextState::TextData::TextData(const TextData& that)
    : m_pFont(that.m_pFont),
      m_pDocument(that.m_pDocument),
      m_FontSize(that.m_FontSize),
      m_CharSpace(that.m_CharSpace),
      m_WordSpace(that.m_WordSpace),
      m_TextMode(that.m_TextMode) {
  for (int i = 0; i < 4; ++i)
    m_Matrix[i] = that.m_Matrix[i];

  for (int i = 0; i < 4; ++i)
    m_CTM[i] = that.m_CTM[i];

  if (m_pDocument && m_pFont) {
    auto* pPageData = CPDF_DocPageData::FromDocument(m_pDocument);
    m_pFont = pPageData->GetFont(m_pFont->GetMutableFontDict());
  }
}

CPDF_TextState::TextData::~TextData() = default;

RetainPtr<CPDF_TextState::TextData> CPDF_TextState::TextData::Clone() const {
  return pdfium::MakeRetain<CPDF_TextState::TextData>(*this);
}

void CPDF_TextState::TextData::SetFont(RetainPtr<CPDF_Font> pFont) {
  m_pDocument = pFont ? pFont->GetDocument() : nullptr;
  m_pFont = std::move(pFont);
}

float CPDF_TextState::TextData::GetFontSizeH() const {
  return fabs(FXSYS_sqrt2(m_Matrix[0], m_Matrix[2]) * m_FontSize);
}

bool SetTextRenderingModeFromInt(int iMode, TextRenderingMode* mode) {
  if (iMode < 0 || iMode > 7)
    return false;
  *mode = static_cast<TextRenderingMode>(iMode);
  return true;
}

bool TextRenderingModeIsClipMode(const TextRenderingMode& mode) {
  switch (mode) {
    case TextRenderingMode::MODE_FILL_CLIP:
    case TextRenderingMode::MODE_STROKE_CLIP:
    case TextRenderingMode::MODE_FILL_STROKE_CLIP:
    case TextRenderingMode::MODE_CLIP:
      return true;
    default:
      return false;
  }
}

bool TextRenderingModeIsStrokeMode(const TextRenderingMode& mode) {
  switch (mode) {
    case TextRenderingMode::MODE_STROKE:
    case TextRenderingMode::MODE_FILL_STROKE:
    case TextRenderingMode::MODE_STROKE_CLIP:
    case TextRenderingMode::MODE_FILL_STROKE_CLIP:
      return true;
    default:
      return false;
  }
}
