#include "TextController.h"

namespace sandbox
{
  void TextController::AllocateGlyphBuffer(size_t NumGlyphs)
  {
    BufferAllocateInfo allocInfo = {};
    allocInfo.Locality = Buffer_Locality_Cpu_To_Gpu;
    allocInfo.Type.Set(Buffer_Type_Vertex);
    allocInfo.Type.Set(Buffer_Type_Transfer_Dst);
    allocInfo.Size = sizeof(GlyphBufferParams) * NumGlyphs; // Support only upto 1000 characters for now.
    allocInfo.FirstBinding = 2;

    GlyphIboHnd = pRenderer->AllocateNewBuffer(allocInfo);
    VKT_ASSERT(GlyphIboHnd != INVALID_HANDLE);
  }

  TextController::TextController(IRenderSystem& InRenderer, EngineImpl& InEngine) :
    Quad{},
    FontCtrl{ InRenderer, InEngine },
    GlyphIboHnd{},
    Texts{},
    DefaultFont{},
    pRenderer{ &InRenderer },
    MaxNumGlyphs{}
  {}

  TextController::~TextController() {}

  void TextController::Initialize(size_t InitialMaxGlyphCount)
  {
    MaxNumGlyphs = InitialMaxGlyphCount;

    astl::Ref<IStagingManager> pStaging = &pRenderer->GetStagingManager();
    Quad.Vertices.NumVertices = 4;
    Quad.Vertices.VertexOffset = pStaging->GetVertexBufferOffset() / sizeof(Vertex);

    Quad.Indices.NumIndices = 6;
    Quad.Indices.IndexOffset = pStaging->GetIndexBufferOffset() / sizeof(uint32);

    Vertex quad[4] = {};
    quad[0].Position = { -0.5f,  0.5f, 0.0f };
    quad[1].Position = {  0.5f,  0.5f, 0.0f };
    quad[2].Position = { -0.5f, -0.5f, 0.0f };
    quad[3].Position = {  0.5f, -0.5f, 0.0f };

    uint32 indices[6] = { 0, 1, 2, 1, 3, 2 };

    pStaging->StageVertexData(quad, sizeof(Vertex) * 4);
    pStaging->StageIndexData(indices, sizeof(uint32) * 6);

    //AllocateGlyphBuffer(MaxNumGlyphs);
  }

  void TextController::Terminate()
  {
    FontCtrl.~FontController();
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

  void TextController::Print(astl::String&& Text, const math::vec2 Pos, uint32 Size, math::vec3 Color, Handle<Font> FontHnd)
  {
    TextEntry entry;
    entry.pFont = DefaultFont;
    if (FontHnd != INVALID_HANDLE)
    {
      entry.pFont = FontCtrl.GetFont(FontHnd);
      if (!entry.pFont) { entry.pFont = DefaultFont; }
      return;
    }
    entry.FontSize = Size;
    entry.Color = Color;
    entry.Pos = Pos;
    entry.Str = astl::Move(Text);

    Texts.Push(astl::Move(entry));
  }

  void TextController::Finalize(astl::Array<math::mat4>& OutTransform, astl::Array<GlyphConstant>& OutConstants, float32 CanvasWidth, float32 CanvasHeight)
  {
    if (!FontCtrl.GetNumOfFonts()) { return; }

    size_t countOfChars = 0;
    IFrameGraph& frameGraph = pRenderer->GetFrameGraph();

    for (TextEntry& text : Texts)
    {
      float32 pos_x = text.Pos.x;
      float32 pos_y = text.Pos.y;

      float32 fontSizeInPt = static_cast<float32>(FontCtrl.ConvertPixelsToPoints(text.FontSize, text.pFont->Dpi));
      float32 maxSizePt = static_cast<float32>(text.pFont->SizeInPt);
      float32 fScale = fontSizeInPt / maxSizePt;

      countOfChars += text.Str.Length();
      if (countOfChars >= MaxNumGlyphs)
      {
        //pRenderer->DestroyBuffer(GlyphIboHnd);
        //AllocateGlyphBuffer(MaxNumGlyphs);
      }

      for (astl::String::ConstType c : text.Str)
      {
        const Glyph& character = text.pFont->Characters[c];
        float32 width = character.BR.x - character.TL.x;
        float32 height = (character.BR.y - character.TL.y);

        pos_x += character.Bearing.x * fScale;
        pos_y -= (height - character.Bearing.y) * fScale;

        math::mat4 transform(1.0f);
        math::Translate(transform, math::vec3(pos_x * fScale, -pos_y * fScale, 1.0f));
        math::Scale(transform, math::vec3(width * fScale, height * fScale, 0.f));
        OutTransform.Push(transform);

        astl::Ref<Font> pFont = text.pFont;

        GlyphConstant constant;
        constant.Projection = math::OrthographicRH(0.0f, CanvasWidth, 0.0f, CanvasHeight, 0.0f, 1.0f);
        constant.Uv[0] = character.TL / vec2(text.pFont->Atlas.Width, text.pFont->Atlas.Height);
        constant.Uv[1] = vec2(character.BR.x, character.TL.y) / vec2(text.pFont->Atlas.Width, text.pFont->Atlas.Height);
        constant.Uv[2] = vec2(character.TL.x, character.BR.y) / vec2(text.pFont->Atlas.Width, text.pFont->Atlas.Height);
        constant.Uv[3] = character.BR / vec2(text.pFont->Atlas.Width, text.pFont->Atlas.Height);
        constant.Color = text.Color;
        OutConstants.Push(astl::Move(constant));

        pos_x += ((float32)character.Advance * fScale);
        pos_y = text.Pos.y;
      }
    }
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
