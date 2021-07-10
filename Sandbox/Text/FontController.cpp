#include "FontController.h"

namespace sandbox
{
  void FontController::CreateGlyphAtlas(Font& InFont, const astl::Buffer<uint8>& Pixels)
  {
    InFont.Atlas.Hnd = pRenderer->CreateImage(InFont.Atlas.Width, InFont.Atlas.Height, 4, Texture_Type_2D, Texture_Format_Unorm, true);
    //pRenderer->BuildImage(InFont.Atlas.Hnd);

    astl::Ref<IStagingManager> pStaging = &pRenderer->GetStagingManager();

    pStaging->StageDataForImage((uint8*)Pixels.ConstData(), Pixels.Size(), InFont.Atlas.Hnd, EQueueType::Queue_Type_Graphics);
    pStaging->TransferImageOwnership(InFont.Atlas.Hnd);
  }

  void FontController::UnloadFont(Font& InFont)
  {
    pRenderer->DestroyImage(InFont.Atlas.Hnd);
    FT_Done_Face(InFont.Face);
    InFont.Characters.Release();
  }

  const uint32 FontController::GetNumOfFonts() const
  {
      return static_cast<uint32>(Fonts.Length());
  }

  FontController::FontController(IRenderSystem& InRenderer, EngineImpl& InEngine) :
    Ft{},
    Fonts{},
    pRenderer{ &InRenderer },
    pEngine{ &InEngine }
  {
    FT_Init_FreeType(&Ft);
    pEngine->CreateNewResourceCache(Sandbox_Asset_Font);
  }

  FontController::~FontController()
  {
    for (auto& [key, font] : Fonts)
    {
      UnloadFont(font);
    }
    FT_Done_FreeType(Ft);
    pEngine->DeleteResourceCacheForType(Sandbox_Asset_Font);
  }

  Handle<Font> FontController::LoadFont(const astl::FilePath& Path, EFontEncoding Encoding, uint32 FontSizeInPixels, uint32 Width, uint32 Height, uint32 HorzDpi, uint32 VertDpi)
  {
    astl::Ref<ResourceCache> pCache = pEngine->FetchResourceCacheForType(Sandbox_Asset_Font);
    uint32 id = pCache->Create();
    constexpr uint32 padding = 16; // pad between glyphs in the atlas

    Font& font = Fonts.Insert(id, Font());;
    font.Encoding = Encoding;
    font.SizePx = FontSizeInPixels;
    font.FSizePx = (float32)FontSizeInPixels;
    font.Characters.Reserve((size_t)Encoding);
    font.Atlas.Width = Width;
    font.Atlas.Height = Height;

    FT_New_Face(Ft, Path.C_Str(), 0, &font.Face);
    FT_Set_Char_Size(font.Face, 0, FontSizeInPixels << 6, HorzDpi, VertDpi);

    FT_Face face = font.Face;
    uint32 maxHeightPx = face->size->metrics.height >> 6;

    font.FMaxFontHeight = (float32)maxHeightPx;

    astl::Buffer<uint8> atlas((size_t)font.Atlas.Width * (size_t)font.Atlas.Height * sizeof(uint32));
    uint32 pen_x = padding, pen_y = padding;

    const float32 aw = (float32)font.Atlas.Width;   // Atlas width in float
    const float32 ah = (float32)font.Atlas.Height;  // Atlas height in float

    for (uint32 i = 0; i < static_cast<uint32>(Encoding); i++)
    {
      FT_Load_Char(face, i, FT_LOAD_RENDER);

      uint8* pixels = face->glyph->bitmap.buffer;
      auto& bitmap = face->glyph->bitmap;

      if (pen_x + bitmap.width >= font.Atlas.Width)
      {
        pen_x = padding;
        pen_y += maxHeightPx + padding;
      }

      for (uint32 row = 0; row < bitmap.rows; row++)
      {
        for (uint32 col = 0; col < bitmap.width; col++)
        {
          size_t x = pen_x + (size_t)col;
          size_t y = pen_y + (size_t)row;
          const size_t pos = 4 * x + y * ((size_t)font.Atlas.Width * 4);

          atlas[pos + 0] = 0xFF;
          atlas[pos + 1] = 0xFF;
          atlas[pos + 2] = 0xFF;
          atlas[pos + 3] = pixels[col + row * bitmap.width];
        }
      }

      Glyph info;
      info.Pos.x = static_cast<float32>(pen_x);
      info.Pos.y = static_cast<float32>(pen_y);
      info.Dim.x = static_cast<float32>(bitmap.width);
      info.Dim.y = static_cast<float32>(bitmap.rows);
      info.Bearing.x = static_cast<float32>(face->glyph->bitmap_left);
      info.Bearing.y = static_cast<float32>(face->glyph->bitmap_top);
      info.Advance = static_cast<float32>(face->glyph->advance.x >> 6);

      // Calculate texture coordinates.
      info.TexCoords[Glyph_TexCoord_TL] = math::vec2( info.Pos.x / aw, info.Pos.y / ah);
      info.TexCoords[Glyph_TexCoord_TR] = math::vec2((info.Pos.x + info.Dim.x) / aw, info.Pos.y / ah);
      info.TexCoords[Glyph_TexCoord_BL] = math::vec2( info.Pos.x / aw, (info.Pos.y + info.Dim.y) / ah);
      info.TexCoords[Glyph_TexCoord_BR] = math::vec2((info.Pos.x + info.Dim.x) / aw, (info.Pos.y + info.Dim.y) / ah);

      font.Characters.Push(astl::Move(info));

      pen_x += bitmap.width + (padding * 2);
    }

    CreateGlyphAtlas(font, atlas);

    return Handle<Font>(id);
  }

  const astl::Ref<Font> FontController::GetFont(Handle<Font> Hnd)
  {
    if (Hnd == INVALID_HANDLE)
    {
      VKT_ASSERT(Hnd != INVALID_HANDLE && "Handle supplied is invalid!");
      return NULLPTR;
    }
    return &Fonts[Hnd];
  }

  void FontController::UnloadFont(Handle<Font>& Hnd)
  {
    const astl::Ref<Font> pFont = GetFont(Hnd);
    astl::Ref<ResourceCache> pCache = pEngine->FetchResourceCacheForType(Sandbox_Asset_Font);
    pCache->Delete(Hnd);
    UnloadFont(const_cast<Font&>(*pFont));
  }

}
