#include "FontController.h"

namespace sandbox
{
  void FontController::CreateGlyphAtlas(Font& InFont, const astl::Buffer<uint8>& Pixels)
  {
    InFont.Atlas.Hnd = pRenderer->CreateImage(InFont.Atlas.Width, InFont.Atlas.Height, 4, Texture_Type_2D, Texture_Format_Unorm);
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

  uint32 FontController::ConvertPixelsToPoints(uint32 Pixels, uint32 Dpi)
  {
    // 72 because 1 point is 1/72 of an inch.
    return (Pixels * 72) / Dpi;
  }

  uint32 FontController::ConvertPointsToPixels(uint32 Points, uint32 Dpi)
  {
    return (Points * Dpi) / 72;
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

  Handle<Font> FontController::LoadFont(const astl::FilePath& Path, EFontEncoding Encoding, uint32 FontSizeInPixels, uint32 Dpi)
  {
    astl::Ref<ResourceCache> pCache = pEngine->FetchResourceCacheForType(Sandbox_Asset_Font);
    uint32 id = pCache->Create();

    Font font = {};
    font.Dpi = Dpi;
    font.Encoding = Encoding;
    font.SizeInPt = ConvertPixelsToPoints(FontSizeInPixels, font.Dpi);
    font.Characters.Reserve((size_t)Encoding);

    FT_New_Face(Ft, Path.C_Str(), 0, &font.Face);
    FT_Set_Char_Size(font.Face, 0, font.SizeInPt << 6, font.Dpi, font.Dpi);

    FT_Face face = font.Face;
    uint32 maxHeight = face->size->metrics.height >> 6;
    uint32 maxHeightPx = ConvertPointsToPixels(maxHeight, font.Dpi);
    uint32 maxDim = (1 + maxHeightPx) * (uint32)(math::FCeil(math::Sqrt((float32)Encoding)));

    font.Atlas.Width = 1;
    while (font.Atlas.Width < maxDim)
    {
      font.Atlas.Width <<= 1;
    }
    font.Atlas.Height = font.Atlas.Width;

    astl::Buffer<uint8> atlas(font.Atlas.Width * font.Atlas.Height * sizeof(uint32));
    uint32 pen_x = 0, pen_y = 0;

    for (uint32 i = 0; i < static_cast<uint32>(Encoding); i++)
    {
      FT_Load_Char(face, i, FT_LOAD_RENDER);

      uint8* pixels = face->glyph->bitmap.buffer;
      auto& bitmap = face->glyph->bitmap;

      if (pen_x + bitmap.width >= font.Atlas.Width)
      {
        pen_x = 0;
        pen_y += (maxHeightPx + 1);
      }

      for (uint32 row = 0; row < bitmap.rows; row++)
      {
        for (uint32 col = 0; col < bitmap.width; col++)
        {
          size_t x = pen_x + (size_t)col;
          size_t y = pen_y + (size_t)row;
          const size_t pos = 4 * x + y * (font.Atlas.Width * 4);

          atlas[pos + 0] = 0xFF;
          atlas[pos + 1] = 0xFF;
          atlas[pos + 2] = 0xFF;
          atlas[pos + 3] = pixels[col + row * bitmap.width];
        }
      }

      Glyph info;
      info.TL.x = (float32)pen_x;
      info.TL.y = (float32)pen_y;
      info.BR.x = (float32)pen_x + (float32)bitmap.width;
      info.BR.y = (float32)pen_y + (float32)bitmap.rows;
      info.Bearing.x = (float32)face->glyph->bitmap_left;
      info.Bearing.y = (float32)face->glyph->bitmap_top;
      info.Advance = ConvertPointsToPixels(face->glyph->advance.x >> 6, font.Dpi);

      font.Characters.Push(astl::Move(info));

      pen_x += bitmap.width + 1;
    }

    CreateGlyphAtlas(font, atlas);
    Fonts.Insert(id, astl::Move(font));

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

  uint32 FontController::GetFontMaxSizePt(Handle<Font> Hnd)
  {
    const astl::Ref<Font> pFont = GetFont(Hnd);
    return pFont->SizeInPt;
  }

  uint32 FontController::GetFontMaxSizePx(Handle<Font> Hnd)
  {
    const astl::Ref<Font> pFont = GetFont(Hnd);
    return ConvertPointsToPixels(pFont->SizeInPt, pFont->Dpi);
  }

}
