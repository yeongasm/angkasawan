#include "TextController.h"

namespace sandbox
{
  void TextController::AllocateGlyphBuffer(size_t NumGlyphs)
  {
    BufferAllocateInfo allocInfo = {};
    allocInfo.Locality = Buffer_Locality_Cpu_To_Gpu;
    allocInfo.Type.Set(Buffer_Type_Vertex);
    allocInfo.Type.Set(Buffer_Type_Transfer_Dst);
    allocInfo.Size = sizeof(GlyphInstanceAttrib) * NumGlyphs; // Support only upto 1000 characters for now.
    allocInfo.FirstBinding = 2;

    GlyphIboHnd = pRenderer->AllocateNewBuffer(allocInfo);
    VKT_ASSERT(GlyphIboHnd != INVALID_HANDLE);
  }

  TextController::TextController(IRenderSystem& InRenderer, EngineImpl& InEngine) :
    Quad{},
    FontCtrl{ InRenderer, InEngine },
    GlyphIboHnd{},
    Texts{},
    InstanceAttrib{},
    DefaultFont{},
    pRenderer{ &InRenderer },
    MaxNumGlyphs{}
  {}

  TextController::~TextController() {}

  void TextController::Initialize(size_t InitialMaxGlyphCount)
  {
    MaxNumGlyphs = InitialMaxGlyphCount;
    InstanceAttrib.Reserve(MaxNumGlyphs);

    astl::Ref<IStagingManager> pStaging = &pRenderer->GetStagingManager();
    Quad.Vertices.NumVertices = 4;
    Quad.Vertices.VertexOffset = static_cast<uint32>(pStaging->GetVertexBufferOffset() / sizeof(Vertex));

    Quad.Indices.NumIndices = 6;
    Quad.Indices.IndexOffset = static_cast<uint32>(pStaging->GetIndexBufferOffset() / sizeof(uint32));

    Vertex quad[4] = {};
    //quad[0].Position = { -0.5f,  0.5f, 0.0f }; // TL
    //quad[1].Position = {  0.5f,  0.5f, 0.0f }; // TR
    //quad[2].Position = { -0.5f, -0.5f, 0.0f }; // BL
    //quad[3].Position = {  0.5f, -0.5f, 0.0f }; // BR
    quad[0].Position = { 0.0f,  0.0f, 0.0f }; // TL
    quad[1].Position = { 1.0f,  0.0f, 0.0f }; // TR
    quad[2].Position = { 0.0f, -1.0f, 0.0f }; // BL
    quad[3].Position = { 1.0f, -1.0f, 0.0f }; // BR

    uint32 indices[6] = { 0, 1, 2, 1, 3, 2 };

    pStaging->StageVertexData(quad, sizeof(Vertex) * 4);
    pStaging->StageIndexData(indices, sizeof(uint32) * 6);

    AllocateGlyphBuffer(MaxNumGlyphs);
  }

  void TextController::Terminate()
  {
    FontCtrl.~FontController();
    pRenderer->DestroyBuffer(GlyphIboHnd);
  }

  void TextController::SetDefaultFont(Handle<Font> Hnd)
  {
    astl::Ref<Font> pFont = FontCtrl.GetFont(Hnd);
    VKT_ASSERT(pFont && "Font handle specified is invalid");
    DefaultFont = pFont;
  }

  Handle<Font> TextController::LoadFont(const astl::FilePath& Path, uint32 Size)
  {
    return FontCtrl.LoadFont(Path, EFontEncoding::Font_Encoding_ASCII, Size);
  }

  Handle<SImage> TextController::GetFontAtlasHandle(Handle<Font> Hnd)
  {
    astl::Ref<Font> pFont = FontCtrl.GetFont(Hnd);
    return pFont->Atlas.Hnd;
  }

  Handle<SMemoryBuffer> TextController::GetGlyphInstanceBufferHandle() const
  {
    return GlyphIboHnd;
  }

  void TextController::Print(astl::String&& Text, const math::vec2 Pos, float32 SizePx, math::vec3 Color, Handle<Font> FontHnd)
  {
    TextEntry entry;
    entry.pFont = DefaultFont;
    if (FontHnd != INVALID_HANDLE)
    {
      entry.pFont = FontCtrl.GetFont(FontHnd);
      if (!entry.pFont) { entry.pFont = DefaultFont; }
      return;
    }
    entry.FontSizePx = SizePx;
    entry.Color = Color;
    entry.Pos = Pos;
    entry.Str = astl::Move(Text);

    Texts.Push(astl::Move(entry));
  }

  void TextController::Finalize(math::mat4& ProjConstant, uint32& NumInstances, float32 CanvasWidth, float32 CanvasHeight)
  {
    if (!FontCtrl.GetNumOfFonts()) { return; }
    InstanceAttrib.Empty();
    pRenderer->ResetBufferOffset(GlyphIboHnd);

    for (TextEntry& text : Texts)
    {
      const float32 fScale = text.FontSizePx / text.pFont->FSizePx;
      const float32 baseline = text.Pos.y + (text.pFont->Characters['A'].Dim.y * fScale);
      float32 pos_x = text.Pos.x;

      for (astl::String::ConstType c : text.Str)
      {
        const Glyph& ch = text.pFont->Characters[c];
        const float32 width = ch.Dim.x * fScale;
        const float32 height = ch.Dim.y * fScale;
        const float32 bx = ch.Bearing.x * fScale;
        const float32 by = ch.Bearing.y * fScale;
        const float32 advance = ch.Advance * fScale;

        pos_x += bx;

        math::mat4 transform(1.0f);
        math::Translate(transform, math::vec3(pos_x, -(baseline - by), 0.0f));
        math::Scale(transform, math::vec3(width, height, 0.0f));

        GlyphInstanceAttrib attrib;
        attrib.Transform = transform;
        attrib.Uv[0] = ch.TexCoords[Glyph_TexCoord_TL];
        attrib.Uv[1] = ch.TexCoords[Glyph_TexCoord_TR];
        attrib.Uv[2] = ch.TexCoords[Glyph_TexCoord_BL];
        attrib.Uv[3] = ch.TexCoords[Glyph_TexCoord_BR];
        attrib.Color = text.Color;

        InstanceAttrib.Push(astl::Move(attrib));

        pos_x += advance - bx;
      }
    }

    pRenderer->CopyDataToBuffer(GlyphIboHnd, InstanceAttrib.First(), sizeof(GlyphInstanceAttrib) * InstanceAttrib.Length(), 0);
    ProjConstant = math::OrthographicRH(0.0f, CanvasWidth, 0.0f, CanvasHeight, 0.0f, 1.0f); // Near and far values just to avoid NaNs;
    NumInstances = static_cast<uint32>(InstanceAttrib.Length());
    Texts.Empty();
  }

  VertexInformation& TextController::GetGlyphQuadVertexInformation()
  {
    return Quad.Vertices;
  }

  IndexInformation& TextController::GetGlyphQuadIndexInformation()
  {
    return Quad.Indices;
  }

}
