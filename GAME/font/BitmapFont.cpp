#include "BitmapFont.h"
#include "kEngine.h"
#include "Object/Sprite.h"
#include "GAME/json/JsonManager.h"

BitmapFont::~BitmapFont() {
	Cleanup();
}

void BitmapFont::Initialize(kEngine* system, const std::string& fontMapPath) {
	system_ = system;

	// JSON読み込み
	auto result = JsonManager::Load(fontMapPath);
	if (!result.has_value()) {
		return;
	}
	nlohmann::json root = result.value();

	colsPerRow_ = root.value("cols_per_row", 16);

	// テクスチャ読み込み
	auto& texJson = root["textures"];

	// kana (index 0)
	if (texJson.contains("kana")) {
		auto& kana = texJson["kana"];
		std::string path = kana["path"].get<std::string>();
		textures_[0].handle = system_->LoadTexture(path);
		textures_[0].cellWidth = kana["cell_width"].get<int>();
		textures_[0].cellHeight = kana["cell_height"].get<int>();
	}

	// ascii (index 1)
	if (texJson.contains("ascii")) {
		auto& ascii = texJson["ascii"];
		std::string path = ascii["path"].get<std::string>();
		textures_[1].handle = system_->LoadTexture(path);
		textures_[1].cellWidth = ascii["cell_width"].get<int>();
		textures_[1].cellHeight = ascii["cell_height"].get<int>();
	}

	// kanji (index 2)
	if (texJson.contains("kanji")) {
		auto& kanji = texJson["kanji"];
		std::string path = kanji["path"].get<std::string>();
		if (!path.empty()) {
			textures_[2].handle = system_->LoadTexture(path);
			textures_[2].cellWidth = kanji["cell_width"].get<int>();
			textures_[2].cellHeight = kanji["cell_height"].get<int>();
		}
	}

	// グリフマップ構築
	if (root.contains("kana_rows")) {
		std::vector<std::string> rows;
		for (auto& row : root["kana_rows"]) {
			rows.push_back(row.get<std::string>());
		}
		BuildGlyphMap(rows, 0, false);
	}

	if (root.contains("ascii_rows")) {
		std::vector<std::string> rows;
		for (auto& row : root["ascii_rows"]) {
			rows.push_back(row.get<std::string>());
		}
		BuildGlyphMap(rows, 1, true);
	}

	if (root.contains("kanji_rows")) {
		std::vector<std::string> rows;
		for (auto& row : root["kanji_rows"]) {
			rows.push_back(row.get<std::string>());
		}
		BuildGlyphMap(rows, 2, false);
	}

	// フォールバック文字の設定
	std::string fallbackStr = root.value("fallback_char", "＊");
	auto it = glyphMap_.find(fallbackStr);
	if (it != glyphMap_.end()) {
		fallbackGlyph_ = it->second;
		hasFallback_ = true;
	}
}

void BitmapFont::RenderText(const std::string& text,
	Vector2 position, float heightPx,
	Align align, float layer, Vector4 color) {

	// UTF-8を1文字ずつ分解
	std::vector<std::string> chars = SplitUTF8(text);
	if (chars.empty()) return;

	// 前回のスプライトを全部削除
	for (auto* sprite : spritePool_) {
		delete sprite;
	}
	spritePool_.clear();

	// スプライトプールの使用数をリセット
	spritePoolUsed_ = 0;

	// Align用に合計幅を計算
	float totalWidth = CalcTextWidth(chars, heightPx);

	// 開始X座標を計算
	float startX = position.x;
	switch (align) {
	case Align::Left:
		break;
	case Align::Center:
		startX -= totalWidth * 0.5f;
		break;
	case Align::Right:
		startX -= totalWidth;
		break;
	}

	// 1文字ずつ描画
	float cursorX = startX;

	for (const auto& ch : chars) {
		// グリフ検索
		const GlyphInfo* glyph = nullptr;
		auto it = glyphMap_.find(ch);
		if (it != glyphMap_.end()) {
			glyph = &it->second;
		} else if (hasFallback_) {
			glyph = &fallbackGlyph_;
		} else {
			// フォールバックもない場合はスキップ
			cursorX += heightPx;
			continue;
		}

		// この文字の表示幅
		float charWidth = glyph->isHalfWidth ? heightPx * 0.5f : heightPx;

		// テクスチャ情報取得
		const FontTexture& tex = textures_[glyph->textureIndex];

		// UV切り出し座標
		float cropX = static_cast<float>(glyph->col * tex.cellWidth);
		float cropY = static_cast<float>(glyph->row * tex.cellHeight);
		float cropW = static_cast<float>(tex.cellWidth);
		float cropH = static_cast<float>(tex.cellHeight);

		// スプライト取得
		SimpleSprite* sprite = GetPooledSprite();

		// 前回の設定をリセット
		sprite->mainPosition.transform = CreateDefaultTransform();

		// 位置設定
		sprite->mainPosition.transform.translate = {
			cursorX, position.y, layer
		};

		// テクスチャとUV切り出し設定
		sprite->objectParts_[0].materialConfig->textureHandle = tex.handle;
		sprite->objectParts_[0].materialConfig->useModelTexture = false;
		sprite->objectParts_[0].cropLT = { cropX, cropY };
		sprite->objectParts_[0].cropSize = { cropW, cropH };

		// 表示サイズをconerDataで直接指定
		sprite->objectParts_[0].conerData.coner[0] = { 0.0f, 0.0f };         // LT
		sprite->objectParts_[0].conerData.coner[1] = { 0.0f, heightPx };     // LB
		sprite->objectParts_[0].conerData.coner[2] = { charWidth, heightPx }; // RB
		sprite->objectParts_[0].conerData.coner[3] = { charWidth, 0.0f };     // RT

		// スケールは1.0固定
		sprite->mainPosition.transform.scale = { 1.0f, 1.0f, 1.0f };

		// アンカーポイント（左上基準）
		sprite->objectParts_[0].anchorPoint = { 0.0f, 0.0f };

		// TODO: color適用（MaterialConfigに対応したら実装）

		// 描画
		sprite->Draw();

		// カーソルを進める
		cursorX += charWidth;

		// 使わなかったスプライトを画面外に移動
		for (int i = spritePoolUsed_; i < static_cast<int>(spritePool_.size()); ++i) {
			spritePool_[i]->mainPosition.transform.translate = { -9999.0f, -9999.0f, 0.0f };
			spritePool_[i]->Update(nullptr);
			spritePool_[i]->Draw();
		}
	}
}

void BitmapFont::Cleanup() {
	for (auto* sprite : spritePool_) {
		delete sprite;
	}
	spritePool_.clear();
	spritePoolUsed_ = 0;
	glyphMap_.clear();
}

void BitmapFont::BuildGlyphMap(const std::vector<std::string>& rows,
	int textureIndex, bool isHalfWidth) {

	for (int row = 0; row < static_cast<int>(rows.size()); ++row) {
		// 行の文字列をUTF-8で1文字ずつ分解
		std::vector<std::string> chars = SplitUTF8(rows[row]);

		for (int col = 0; col < static_cast<int>(chars.size()); ++col) {
			GlyphInfo glyph;
			glyph.textureIndex = textureIndex;
			glyph.row = row;
			glyph.col = col;
			glyph.isHalfWidth = isHalfWidth;

			glyphMap_[chars[col]] = glyph;
		}
	}
}

std::vector<std::string> BitmapFont::SplitUTF8(const std::string& text) {
	std::vector<std::string> result;

	size_t i = 0;
	while (i < text.size()) {
		unsigned char c = static_cast<unsigned char>(text[i]);
		int byteCount = 1;

		if (c >= 0xF0) {
			byteCount = 4;
		} else if (c >= 0xE0) {
			byteCount = 3;
		} else if (c >= 0xC0) {
			byteCount = 2;
		}

		if (i + byteCount <= text.size()) {
			result.push_back(text.substr(i, byteCount));
		}
		i += byteCount;
	}

	return result;
}

float BitmapFont::CalcTextWidth(const std::vector<std::string>& chars,
	float heightPx) {

	float width = 0.0f;

	for (const auto& ch : chars) {
		auto it = glyphMap_.find(ch);
		if (it != glyphMap_.end()) {
			width += it->second.isHalfWidth ? heightPx * 0.5f : heightPx;
		} else {
			// 未登録文字はフォールバックの幅
			if (hasFallback_) {
				width += fallbackGlyph_.isHalfWidth ? heightPx * 0.5f : heightPx;
			} else {
				width += heightPx;
			}
		}
	}

	return width;
}

SimpleSprite* BitmapFont::GetPooledSprite() {
	if (spritePoolUsed_ < static_cast<int>(spritePool_.size())) {
		SimpleSprite* sprite = spritePool_[spritePoolUsed_++];
		sprite->mainPosition.transform = CreateDefaultTransform();
		// objectParts_が空の場合のみ再生成
		if (sprite->objectParts_.empty()) {
			sprite->CreateDefaultData();
		}
		return sprite;
	}

	// 新しいスプライトを生成してプールに追加
	SimpleSprite* sprite = new SimpleSprite;
	sprite->IntObject(system_);
	sprite->CreateDefaultData();
	sprite->mainPosition.transform = CreateDefaultTransform();

	spritePool_.push_back(sprite);
	spritePoolUsed_++;

	return sprite;
}
