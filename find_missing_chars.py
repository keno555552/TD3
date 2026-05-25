import json, glob, os

with open('GAME/resources/font/font_map.json', 'r', encoding='utf-8') as f:
    font_map = json.load(f)

available_chars = set()
for key in ['kana_rows', 'ascii_rows', 'kanji_rows']:
    if key in font_map:
        for row in font_map[key]:
            available_chars.update(list(row))

missing_chars = set()
judge_files = glob.glob('GAME/resources/judges/*.json')

print('--- 不足している漢字リスト ---')
for jf in judge_files:
    with open(jf, 'r', encoding='utf-8') as f:
        jdata = json.load(f)
        text_to_check = jdata.get('judge_name', '') + jdata.get('judge_title', '')
        
        missing_in_this = set()
        for char in text_to_check:
            if char not in available_chars:
                missing_chars.add(char)
                missing_in_this.add(char)
        
        if missing_in_this:
            print(f"{os.path.basename(jf)}: {jdata.get('judge_name')} / {jdata.get('judge_title')} -> 不足: {' '.join(missing_in_this)}")

print('\nすべての不足文字(まとめて):', ''.join(sorted(list(missing_chars))))
